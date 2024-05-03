#include "config.h"
#include "patch.h"
#include "dynamic.h"
#include "ios/svc.h"
#include "trampoline.h"
#include "addrs_55x.h"
#include "rednand.h"
#include "rednand_config.h"

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


bool disable_scfm = false;
bool scfm_on_slccmpt = false;

static int haidev = 0;

static int* red_mlc_server_handle;


static const char mlc_pattern[] = { 0x00, 0xa3, 0x60, 0xb7, 0x58, 0x98, 0x21, 0x00};

static void c2w_patch_mlc()
{
    void* ios_paddr = (void*)read32(0x1018);
    void* ios_end = ios_paddr + 0x260000;

    for (void* a = ios_paddr; a < ios_end; a += 2)
    {
        if (!memcmp(a, mlc_pattern, sizeof(mlc_pattern))) {
            write16(a, 0x2300);
            debug_printf("MLC stuff ptr at: 0x%08x\n", a);
            break;
        }
    }

}

static const char hai_mlc_str[] = "/dev/sdio/MLC01";
static void c2w_patch_mlc_str(void){
    void* ios_paddr = (void*)read32(0x1018);
    void* ios_end = ios_paddr + 0x260000;

    for (void* a = ios_paddr; a < ios_end; a += 2)
    {
        if (!memcmp(a, hai_mlc_str, sizeof(hai_mlc_str))) {
            strcpy(a,"/dev/sdio/slot0");
            debug_printf("HAI MLC dev str at: %p\n", a);
            //break;
        }
    }
}

int hai_write_file_patch(int fsa_handle, uint32_t *buffer, size_t size, size_t count, int (*mcpcompat_fwrite)(int, uint32_t*, size_t, size_t, int, int),int fh, int flags){
    debug_printf("HAI WRITE COMPANION\n");
    if(haidev==5 && redmlc_size_sectors){
        uint32_t number_extends = buffer[0];
        debug_printf("number extends: %u\n", number_extends);
        uint32_t address_unit_base = (buffer[1] >> 16);
        debug_printf("address_unit_base: %u\n", address_unit_base);
        uint32_t address_unit_base_lbas = address_unit_base - 9; // 2^9 = 512byte sector
        debug_printf("address_unit_base_lbas: %u\n", address_unit_base_lbas);
        debug_printf("redmlc_off_sectors: 0x%X\n", redmlc_off_sectors);
        uint32_t offset_in_address_units = redmlc_off_sectors >> address_unit_base_lbas;
        debug_printf("offset_in_address_units: %X\n", offset_in_address_units);
        for(size_t i = 2; i < number_extends*2 + 2; i+=2){
            debug_printf("buffer[%u]: %X", i, buffer[i]);
            buffer[i]+=offset_in_address_units;
            debug_printf(" => %X\n", buffer[i]);
            debug_printf("buffer[%u]: %X\n", i+1, buffer[i+1]);
        }
    }

    return mcpcompat_fwrite(fsa_handle, buffer, size, count, fh, flags);
}

void rednand_patch_hai(void){
    if(haidev == DEVTYPE_MLC && redmlc_size_sectors){
    //    c2w_patch_mlc();
        c2w_patch_mlc_str();
        //ASM_PATCH_K(0x10733de8, "nop");
    }
}

void rednand_register_sd_as_mlc(trampoline_state* state){
    red_mlc_server_handle = iosAlloc(LOCAL_HEAP_ID, MDBLK_SERVER_HANDLE_SIZE);
    memcpy(red_mlc_server_handle, (int*) state->r[6] -3, MDBLK_SERVER_HANDLE_SIZE);
    red_mlc_server_handle[3] = (int) red_mlc_server_handle;
    red_mlc_server_handle[5] = DEVTYPE_MLC; // set device typte to mlc
    red_mlc_server_handle[10] = redmlc_size_sectors + 0xFFFF;
    red_mlc_server_handle[14] = red_mlc_server_handle[10] = redmlc_size_sectors;
    int mlc_attach = FSSCFM_Attach_fun(red_mlc_server_handle +3);
    debug_printf("Attaching sdcard as mlc returned %d\n", mlc_attach);
}

/**
 * @brief sets the device type for hai, by hooking FSA_GetFileBlockAddress, which is called to create the compangion file
 */
static void get_block_addr_haidev_patch(trampoline_state* s){
    debug_printf("FSA GET FILE BLOCK ADDRESS\n");
    debug_printf("devid: %d\n", s->r[0]);
    haidev = s->r[0];
    //debug_printf("\na1[1]: %s", r0[1]);
    // char *a2_40 = s->r[2] + 4*39;
    // debug_printf("a2+40: %p: %s", a2_40, a2_40);
    // debug_printf("\nv17+1: %p, %s", s->r[3], s->r[3]);
    // debug_printf("\ncb: %p\n", s->r[2]);
}

static void rednand_apply_mlc_patches(uint32_t redmlc_size_sectors){
    debug_printf("Enabeling MLC redirection\n");
    //patching offset for HAI on MLC in companion file
    trampoline_t_blreplace(0x050078AE, hai_write_file_patch);

    trampoline_hook_before(0x10707BD0, get_block_addr_haidev_patch);

    // Disable eMMC
    ASM_PATCH_K(0x107bd4a8,
        "movs    r2, #1\n"
        "mov     r0, #1\n"
        "rsbs    r2, r2, #0\n"
    );

    debug_printf("Setting mlc size to: %u LBAs\n", redmlc_size_sectors);
    ASM_PATCH_K(0x107bdb10,
        "nop\n"
        "nop\n"
        "nop\n"
        "ldr r4, [pc, #0xb8]\n"
    );
    U32_PATCH_K(0x107bdbdc, redmlc_size_sectors + 0xFFFF);
   
    trampoline_hook_before(0x107bd9a8, rednand_register_sd_as_mlc);
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
}


static int rednand_apply_base_patches(void){
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


void rednand_init(rednand_config* rednand_conf){

    redslc_off_sectors = rednand_conf->slc.lba_start;
    redslc_size_sectors = rednand_conf->slc.lba_length;
    
    redslccmpt_off_sectors = rednand_conf->slccmpt.lba_start;
    redslccmpt_size_sectors = rednand_conf->slccmpt.lba_length;

    redmlc_off_sectors = rednand_conf->mlc.lba_start;
    redmlc_size_sectors = rednand_conf->mlc.lba_length;

    disable_scfm = rednand_conf->disable_scfm;
    scfm_on_slccmpt = rednand_conf->scfm_on_slccmpt;


    rednand_apply_base_patches();

    if(redslc_size_sectors || redslccmpt_size_sectors){
        rednand_apply_slc_patches();
    }
    if(redmlc_size_sectors){
        rednand_apply_mlc_patches(redmlc_size_sectors);
    }
    if(disable_scfm){
        debug_printf("Disableing SCFM\n");
        ASM_PATCH_K(0x107d1f28, "nop\n");
        ASM_PATCH_K(0x107d1e08,"nop\n");
        ASM_PATCH_K(0x107e7628,"mov r3, #0x0\nstr r3, [r10]\n");
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