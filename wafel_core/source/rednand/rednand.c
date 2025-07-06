#include "config.h"
#include "patch.h"
#include "dynamic.h"
#include "ios/svc.h"
#include "trampoline.h"
#include "addrs_55x.h"
#include "rednand.h"
#include "rednand_config.h"
#include "hai.h"
#include "sal_partition.h"
#include "sal.h"
#include "wfs.h"

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

// this converts to string
#define STR_(X) #X

// this makes sure the argument is expanded before converting to string
#define STR(X) STR_(X)

bool disable_scfm = false;
bool scfm_on_slccmpt = false;
bool mlc_nocrypto = false;

static volatile bool learn_mlc_crypto_handle = false;
static volatile bool learn_usb_crypto_handle = false;

static FSSALAttachDeviceArg *sysmlc_attach_arg = NULL;
static int (*mlc_attach_fun)(FSSALAttachDeviceArg*) = NULL;

static bool hai_from_mlc(void){
    return hai_getdev() == DEVTYPE_MLC;
} 

void hai_write_file_patch(trampoline_t_state *s){
    uint32_t *buffer = (uint32_t*)s->r[1];
    debug_printf("HAI WRITE COMPANION\n");
    if(hai_from_mlc() && partition_size){
        hai_companion_add_offset(buffer, partition_offset);
    }
}

void rednand_patch_hai(void){
    if(hai_from_mlc() && partition_size){
    //    c2w_patch_mlc();
        hai_redirect_mlc2sd();
    }
}

typedef int read_write_fun(int*, u32, u32, u32, u32, void*, void*, void*);

static void *mlc_server_handle = 0;
void rednand_register_sd_as_mlc(trampoline_state* state){
    int* sd_attach_arg = (int*)state->r[6];

    FSSALAttachDeviceArg *extra_attach_arg = iosAlloc(LOCAL_HEAP_ID, sizeof(FSSALAttachDeviceArg));
    // int *red_mlc_server_handle = sd_handle + 0x5b; //iosAlloc(LOCAL_HEAP_ID, MDBLK_SERVER_HANDLE_SIZE);
    // if((u32)red_mlc_server_handle > 0x11c39fe4){
    //     debug_printf("sd_handle: %p, red_mlc_handle: %p\n", sd_handle, red_mlc_server_handle);
    //     debug_printf("All mdblk slots already allocated!!!\n");
    //     crash_and_burn();
    // }

    memcpy(extra_attach_arg, sd_attach_arg, sizeof(FSSALAttachDeviceArg));

    patch_partition_attach_arg(extra_attach_arg, DEVTYPE_MLC);

    mlc_server_handle = extra_attach_arg->server_handle;

    FSSALHandle sal_handle;
    if(disable_scfm)
        sal_handle = FSSAL_attach_device(extra_attach_arg);
    else
        sal_handle = FSSCFM_attach_device(extra_attach_arg);

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

static void *sysmlc_server_handle = 0;
static FSSALHandle sysmlc_attach_hook(FSSALAttachDeviceArg *attach_arg, int r1, int r2, int r3, FSSALHandle (*mlc_attach_ptr)(FSSALAttachDeviceArg*)){
    sysmlc_server_handle = attach_arg->server_handle;
    FSSALHandle sal_handle = mlc_attach_ptr(attach_arg);
    debug_printf("Attaching sysmlc returned %d\n", sal_handle);
    return sal_handle;
}

static void wfs_initDeviceParams_exit_hook(trampoline_state *regs){
    WFS_Device *wfs_device = (WFS_Device*)regs->r[5];
    FSSALDevice *sal_device = FSSAL_LookupDevice(wfs_device->handle);
    void *server_handle = sal_device->server_handle;
    debug_printf("wfs_initDeviceParams_exit_hook server_handle: %p\n", server_handle);
    if(server_handle == sysmlc_server_handle) {
        wfs_device->crypto_key_handle = WFS_KEY_HANDLE_MLC;
    } else if(mlc_nocrypto && server_handle == mlc_server_handle) {
        wfs_device->crypto_key_handle = WFS_KEY_HANDLE_NOCRYPTO;
    }
}

static void rednand_install_crypto_hooks(void){
    trampoline_hook_before(0x107435f4, wfs_initDeviceParams_exit_hook);
}

static void rednand_apply_mlc_patches(bool mount_sys){
    debug_printf("Enabeling MLC redirection\n");
    trampoline_hook_before(0x107bd9a8, rednand_register_sd_as_mlc);

    //patching offset for HAI on MLC in companion file
    trampoline_t_hook_before(0x050078AE, hai_write_file_patch);
    hai_apply_getdev_patch();

    //trampoline_hook_before(0x107bd7a0, print_attach);

    if(!mount_sys) {
        // Don't attach eMMC
        trampoline_hook_before(0x107bd754, skip_mlc_attch_hook);
        ASM_PATCH_K(0x107bdae0, "mov r0, #0xFFFFFFFF\n"); //make extra sure mlc doesn't attach
    }
}

static void rednand_apply_mount_sys_mlc_patches(void){
    debug_printf("Mount sysMLC as USB\n");
    // change mlc type to USB
    ASM_PATCH_K(0x107bdb00, "mov r3, #" STR(DEVTYPE_USB));
    // make SCFM look for usb instead of mlc
    ASM_PATCH_K(0x107d1f24, "cmp r3, #" STR(DEVTYPE_USB));
    ASM_PATCH_K(0x107d1f40, "cmp r3, #" STR(DEVTYPE_USB));

    trampoline_blreplace(0x107bdae0, sysmlc_attach_hook);

    hai_apply_force_mlc_patch();
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

    partition_offset = rednand_conf->mlc.lba_start;
    partition_size = rednand_conf->mlc.lba_length;

    disable_scfm = rednand_conf->disable_scfm;
    scfm_on_slccmpt = rednand_conf->scfm_on_slccmpt;


    if(config_size>sizeof(rednand_config)){
        debug_printf("WARNING: newer rednand config detected, not all features are supported in this stroopwafel!!!\n");
    }

    if(config_size>=sizeof(rednand_config_v2)){
        mlc_nocrypto = rednand_conf->mlc_nocrypto;
    } 
    
    bool mount_sys_mlc = false;
    if(config_size>=sizeof(rednand_config)) {
        mount_sys_mlc = rednand_conf->sys_mount_mlc;
    } else {
        debug_printf("Old redNAND config detected\n");
    }

    if(mount_sys_mlc && redslc_size_sectors) {
        debug_printf("Mounting sysMLC as USB isn't allowed with SLC redirection\n");
        mount_sys_mlc = false;
    }

    if(mount_sys_mlc && partition_size && !disable_scfm){
        debug_printf("Mounting sysMLC on redMLC enabled is only possible with redNAND SCFM and SLC redirection disabled\n");
        mount_sys_mlc = false;
    }

    if(disable_scfm && !mount_sys_mlc){
        // needs to run before the mlc patches
        apply_scfm_disable_patches();
    }

    if(redslc_size_sectors || redslccmpt_size_sectors){
        rednand_apply_slc_patches();
    }

    if(mount_sys_mlc) {
        rednand_apply_mount_sys_mlc_patches();
    }

    if(partition_size){
        debug_printf("mlc_nocrpyto: %d\n", mlc_nocrypto);
        rednand_apply_mlc_patches(mount_sys_mlc);
    }

    if(mlc_nocrypto || mount_sys_mlc){
        rednand_install_crypto_hooks();
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
        const char* panic = "BUILD without MLC_ACCELERATE\n";
        iosPanic(panic, strlen(panic)+1);
        while(1);
#endif
        }
}