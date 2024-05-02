#include "config.h"
#include "patch.h"
#include "dynamic.h"
#include "svc.h"
#include "trampoline.h"
#include "addrs_55x.h"

extern void slcRead1_patch();
extern void slcRead2_patch();
extern void slcWrite1_patch();
extern void slcWrite2_patch();
extern void sdcardRead_patch();
extern void sdcardWrite_patch();
extern void getMdDeviceById_hook();
extern void registerMdDevice_hook();
extern void createDevThread_hook();

#define LOCAL_HEAP_ID 0xCAFE
#define MDBLK_SERVER_HANDLE_LEN 0xb5
#define MDBLK_SERVER_HANDLE_SIZE (MDBLK_SERVER_HANDLE_LEN * sizeof(int))

static int (*FSSCFM_Attach_fun)(int*) = (void*)FSSCFM_Attach;

static u32 mlc_size = 0;
static int* red_mlc_server_handle;

void rednand_register_sd_as_mlc(trampoline_state* state){
    red_mlc_server_handle = iosAlloc(LOCAL_HEAP_ID, MDBLK_SERVER_HANDLE_SIZE);
    memcpy(red_mlc_server_handle, (int*) state->r[6] -3, MDBLK_SERVER_HANDLE_SIZE);
    red_mlc_server_handle[3] = (int) red_mlc_server_handle;
    red_mlc_server_handle[5] = 5; // set device typte to mlc
    red_mlc_server_handle[10] = mlc_size;
    red_mlc_server_handle[14] = red_mlc_server_handle[10] - 0xffff;
    int mlc_attach = FSSCFM_Attach_fun(red_mlc_server_handle +3);
    debug_printf("Attaching sdcard as mlc returned %d\n", mlc_attach);
}

void rednand_apply_mlc_patches(uint32_t redmlc_size_sectors){
    debug_printf("Enabeling MLC redirection\n");
    //patching offset for HAI on MLC in companion file
    extern int hai_write_file_shim();
    BL_T_TRAMPOLINE_K(0x050078AE, MCP_ALTBASE_ADDR(hai_write_file_shim));
    extern int get_block_addr_patch1_shim();
    BL_TRAMPOLINE_K(0x10707BD0, FS_ALTBASE_ADDR(get_block_addr_patch1_shim));

    debug_printf("Setting mlc size to: %u LBAs\n", redmlc_size_sectors);
    ASM_PATCH_K(0x107bdb10,
        "nop\n"
        "nop\n"
        "nop\n"
        "ldr r4, [pc, #0xb8]\n"
    );
    U32_PATCH_K(0x107bdbdc, redmlc_size_sectors + 0xFFFF);
   
    mlc_size = redmlc_size_sectors + 0xFFFF;
    trampoline_hook_before(0x107bd9a8, rednand_register_sd_as_mlc);
}

void rednand_apply_slc_patches(void){
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


void rednand_apply_base_patches(void){
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