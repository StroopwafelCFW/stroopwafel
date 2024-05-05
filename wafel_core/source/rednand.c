#include "config.h"
#include "patch.h"
#include "dynamic.h"
#include "ios/svc.h"
#include "trampoline.h"
#include "addrs_55x.h"
#include "rednand.h"
#include "rednand_config.h"
#include "hai.h"

extern void slcRead1_patch();
extern void slcRead2_patch();
extern void slcWrite1_patch();
extern void slcWrite2_patch();
extern void sdcardRead_patch();
extern void sdcardWrite_patch();
extern void getMdDeviceById_hook();
extern void registerMdDevice_hook();
extern void createDevThread_hook();
extern void scfm_try_slc_cache_migration();

#define LOCAL_HEAP_ID 0xCAFE
#define MDBLK_SERVER_HANDLE_LEN 0xb5
#define MDBLK_SERVER_HANDLE_SIZE (MDBLK_SERVER_HANDLE_LEN * sizeof(int))

static int (*FSSCFM_Attach_fun)(int*) = (void*)FSSCFM_Attach;
static int (*FSSAL_attach_device_fun)(int*) = (void*)FSSAL_attach_device;


bool disable_scfm = false;
bool scfm_on_slccmpt = false;

u32 redmlc_off_sectors = 0;

static bool hai_from_mlc(void){
    return hai_getdev() == DEVTYPE_MLC;
} 

void hai_write_file_patch(trampoline_t_state *s){
    uint32_t *buffer = (uint32_t*)s->r[1];
    debug_printf("HAI WRITE COMPANION\n");
    if(hai_from_mlc() && redmlc_size_sectors){
        hai_companion_add_offset(buffer, redmlc_off_sectors);
    }
}

void rednand_patch_hai(void){
    if(hai_from_mlc() && redmlc_size_sectors){
    //    c2w_patch_mlc();
        hai_redirect_mlc2sd();
    }
}

typedef int read_write_fun(int*, u32, u32, u32, u32, void*, void*, void*);

read_write_fun *sd_real_read = (read_write_fun*)0x107bddd0;
read_write_fun *sd_real_write = (read_write_fun*)0x107bdd60;


static int redmlc_read_wrapper(void *device_handle, u32 lba_hi, u32 lba, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ctx){
    return sd_real_read(device_handle, lba_hi, lba + redmlc_off_sectors, blkCount, blockSize, buf, cb, cb_ctx);
}

static int redmlc_write_wrapper(void *device_handle, u32 lba_hi, u32 lba, u32 blkCount, u32 blockSize, void *buf, void *cb, void* cb_ctx){
    return sd_real_write(device_handle, lba_hi, lba + redmlc_off_sectors, blkCount, blockSize, buf, cb, cb_ctx);
}

void rednand_register_sd_as_mlc(trampoline_state* state){
    int* sd_handle = (int*)state->r[6] -3;
    sd_real_read = (void*)sd_handle[0x76];
    sd_real_write = (void*)sd_handle[0x78];

    // Somehow reusing the handle slot for the mlc doesn't work
    int *red_mlc_server_handle = iosAlloc(LOCAL_HEAP_ID, MDBLK_SERVER_HANDLE_SIZE);
    // int *red_mlc_server_handle = sd_handle + 0x5b; //iosAlloc(LOCAL_HEAP_ID, MDBLK_SERVER_HANDLE_SIZE);
    // if((u32)red_mlc_server_handle > 0x11c39fe4){
    //     debug_printf("sd_handle: %p, red_mlc_handle: %p\n", sd_handle, red_mlc_server_handle);
    //     debug_printf("All mdblk slots already allocated!!!\n");
    //     crash_and_burn();
    // }

    memcpy(red_mlc_server_handle, sd_handle, MDBLK_SERVER_HANDLE_SIZE);

    red_mlc_server_handle[0x76] = (int)redmlc_read_wrapper;
    red_mlc_server_handle[0x78] = (int)redmlc_write_wrapper;
    //red_mlc_server_handle[3] = (int) red_mlc_server_handle;
    red_mlc_server_handle[5] = DEVTYPE_MLC; // set device typte to mlc
    //adding + 0xFFFF would be closter to the original behaviour
    //red_mlc_server_handle[0xa] = redmlc_size_sectors + 0xFFFF;
    // -1 makes more sense and I don't like addressing outside the partition
    red_mlc_server_handle[0xa] = redmlc_size_sectors - 1;
    red_mlc_server_handle[0xe] = redmlc_size_sectors;

    int* sal_attach_device_arg = red_mlc_server_handle + 3;
    int sal_handle;
    if(disable_scfm)
        sal_handle = FSSAL_attach_device_fun(sal_attach_device_arg);
    else
        sal_handle = FSSCFM_Attach_fun(sal_attach_device_arg);
    red_mlc_server_handle[0x82] = sal_handle;
    debug_printf("Attaching sdcard as mlc returned %d\n", sal_handle);
}

static void print_attach(trampoline_state *s){
    debug_printf("Attaching mdbBlk:\n");
    int* piVar8 = (int*) s->r[6];
    for(int i=0; i<0x1c/sizeof(int); i++){
        debug_printf("piVar8[2+%d]: %08X\n", i, piVar8[2+i]);
    }

}

static void skip_mlc_attch_hook(trampoline_state *s){
    int* piVar8 = (int*)s->r[6];
    if(piVar8[2] == 3) // eMMC
        s->lr = 0x107bd714; // jump over all the attach stuff
}

static void print_state(trampoline_state *s){
    debug_printf("10707b70: r0: %d, r1: %d, r2: %d, r3: %d\n", s->r[0], s->r[1], s->r[2], s->r[3]);
}

static void redmlc_crypto_disable_hook(trampoline_state* state){
    // hope that 0x11 stays constant for mlc
    if(state->r[5] == redmlc_size_sectors){
        // tells crypto to not do crypto (depends on stroopwafel patch)
        state->r[0] = 0xDEADBEEF;
    }
}

static void rednand_apply_mlc_patches(bool nocrypto){
    debug_printf("Enabeling MLC redirection\n");
    //patching offset for HAI on MLC in companion file
    trampoline_t_hook_before(0x050078AE, hai_write_file_patch);

    hai_apply_getdev_patch();

    //trampoline_hook_before(0x107bd7a0, print_attach);

    // Don't attach eMMC
    trampoline_hook_before(0x107bd754, skip_mlc_attch_hook);
    ASM_PATCH_K(0x107bdae0, "mov r0, #0xFFFFFFFF\n"); //make extra sure mlc doesn't attach
   
    trampoline_hook_before(0x107bd9a8, rednand_register_sd_as_mlc);

    if(nocrypto){
        debug_printf("REDNAND: disable MLC encryption\n");
        trampoline_hook_before(0x10740f48, redmlc_crypto_disable_hook); // hook decrypt call
        trampoline_hook_before(0x10740fe8, redmlc_crypto_disable_hook); // hook encrypt call
    }
}

static void rednand_apply_slc_patches(void){
    debug_printf("Enabeling SLC/SLCCMPT redirection\n");

    // null out references to slcSomething1 and slcSomething2
    // (nulling them out is apparently ok; more importantly, i'm not sure what they do and would rather get a crash than unwanted slc-writing)
    U32_PATCH_K(0x107B96B8, 0);
    U32_PATCH_K(0x107B96BC, 0);

    // slc redirection
    BRANCH_PATCH_K(FS_SLC_READ1, FS_ALTBASE_ADDR(slcRead1_patch));
    BRANCH_PATCH_K(FS_SLC_READ2, FS_ALTBASE_ADDR(slcRead2_patch));
    BRANCH_PATCH_K(FS_SLC_WRITE1, FS_ALTBASE_ADDR(slcWrite1_patch));
    BRANCH_PATCH_K(FS_SLC_WRITE2, FS_ALTBASE_ADDR(slcWrite2_patch));
    ASM_PATCH_K(0x107206F0, "mov r0, #0"); // nop out hmac memcmp

    // SEEPROM write disable is restricted to redNAND only:
    // - if a non-redNAND system upgrades boot1, the version in SEEPROM will become
    //   mismatched and the system will be boot0-bricked, I think
    // - if a redNAND system upgrades boot1, it will be written to the SD card,
    //   but the SEEPROM version should NOT be synced to the SD card version, because NAND
    //   has not changed

    // nop a function used for seeprom write enable, disable, nuking (will stay in write disable)
    ASM_PATCH_K(0xE600CF5C, 
        "mov r0, #0\n \
        bx lr\n"
    );

    // skip seeprom writes in eepromDrvWriteWord for safety
    ASM_PATCH_K(0xE600D010, 
        "mov r0, #0\n \
        bx lr\n"
    );

    // following were base patches before

    // in createDevThread
    BRANCH_PATCH_K(0x10700294, FS_ALTBASE_ADDR(createDevThread_hook));

    // FS_GETMDDEVICEBYID
    BL_TRAMPOLINE_K(FS_GETMDDEVICEBYID + 0x8, FS_ALTBASE_ADDR(getMdDeviceById_hook));
    // call to FS_REGISTERMDPHYSICALDEVICE in mdMainThreadEntrypoint
    BL_TRAMPOLINE_K(0x107BD81C, FS_ALTBASE_ADDR(registerMdDevice_hook));

    // sdio rw patches
    BL_TRAMPOLINE_K(FS_SDCARD_READ1, FS_ALTBASE_ADDR(sdcardRead_patch));
    BL_TRAMPOLINE_K(FS_SDCARD_WRITE1, FS_ALTBASE_ADDR(sdcardWrite_patch));

    // mdExit : patch out sdcard deinitialization
    ASM_PATCH_K(0x107BD374, "bx lr");
}

static void apply_scfm_disable_patches(void){
    debug_printf("Disabeling SCFM\n");
    ASM_PATCH_K(FSSCFMInit, "bx lr");
    ASM_PATCH_K(FSSCFMExit, "bx lr");
    // change call to SCFM attach (in case MLC redirection is not enabled)
    BL_TRAMPOLINE_K(0x107bdae0, FSSAL_attach_device);
}


void rednand_init(rednand_config* rednand_conf, size_t config_size){

    if(config_size < sizeof(rednand_config_v1)){
        debug_printf("ERROR: wrong rednand_config size!!!!\n");
        crash_and_burn();
    }

    redslc_off_sectors = rednand_conf->slc.lba_start;
    redslc_size_sectors = rednand_conf->slc.lba_length;
    
    redslccmpt_off_sectors = rednand_conf->slccmpt.lba_start;
    redslccmpt_size_sectors = rednand_conf->slccmpt.lba_length;

    redmlc_off_sectors = rednand_conf->mlc.lba_start;
    redmlc_size_sectors = rednand_conf->mlc.lba_length;

    disable_scfm = rednand_conf->disable_scfm;
    scfm_on_slccmpt = rednand_conf->scfm_on_slccmpt;

    if(config_size>sizeof(rednand_config)){
        debug_printf("WARNING: newer rednand config detected, not all features are supported in this stroopwafel!!!\n");
    }

    bool mlc_nocrypto = false;
    if(config_size>=sizeof(rednand_config)){
        mlc_nocrypto = rednand_conf->mlc_nocrypto;
    } else {
        debug_printf("Old redNAND config detected\n");
    }

    if(disable_scfm){
        apply_scfm_disable_patches();
    }

    if(redslc_size_sectors || redslccmpt_size_sectors){
        rednand_apply_slc_patches();
    }
    if(redmlc_size_sectors){
        debug_printf("mlc_nocrpyto: %d\n", mlc_nocrypto);
        rednand_apply_mlc_patches(mlc_nocrypto);
    }
    
    if(scfm_on_slccmpt){
#if MLC_ACCELERATE
        // hooks for supporting scfm.img on sys-slccmpt instead of on the sd card
        // e.g. doing sd->slc caching instead of sd->sd caching which dramatically slows down ALL i/o
        
        // disable scfmInit's slc format on name/partition type error
        ASM_PATCH_K(0x107E8178, 
            "mov r0, #0xFFFFFFFE\n"
        );
    
        //hook scfmInit right after fsa initialization, before main thread creation
        BL_TRAMPOLINE_K(0x107E7B88, FS_ALTBASE_ADDR(scfm_try_slc_cache_migration));
#else
        const char* panic = "BUIDL without MLC_ACCELERATE\n";
        iosPanic(panic, strlen(panic)+1);
        while(1);
#endif
        }
}