.arm.big

.open "sections/0x10700000.bin","patched_sections/0x10700000.bin",0x10700000

.include "config.s"

; sanity-check some config things
.if MLC_ACCELERATE & !USE_SYS_SLCCMPT
.error "MLC_ACCELERATE is only supported with USE_SYS_SLCCMPT enabled!!"
.endif
.if MLC_ACCELERATE & !USE_REDNAND
.error "MLC_ACCELERATE on sysnand will brick on reboot without bootloader hax!!"
.endif

; dev note: example first partition entry at MBR+0x1BE: 00202100AEFEFFFF00080000 0000001C. last u32 is shrunken sector count, little-endian
NOCRYPTO_WFS_SECTOR_COUNT1 equ (USB_SHRINK_SECTOR_COUNT1 & 0xFFFF0000)
NOCRYPTO_WFS_SECTOR_COUNT2 equ (USB_SHRINK_SECTOR_COUNT2 & 0xFFFF0000)
NOCRYPTO_WFS_SECTOR_COUNT3 equ (USB_SHRINK_SECTOR_COUNT3 & 0xFFFF0000)

SECTION_BASE equ 0x10700000
SECTION_SIZE equ 0x000F8200
BSS_START    equ 0x10835000
BSS_SIZE     equ 0x01406554
CODE_BASE    equ (SECTION_BASE + SECTION_SIZE)
FS_BSS_START equ (0x10835000 + 0x1406554)

FRAMEBUFFER_ADDRESS equ (0x14000000+0x38C0000)
FRAMEBUFFER_STRIDE  equ (0xE00)
CHARACTER_MULT equ (2)
CHARACTER_SIZE equ (8*CHARACTER_MULT)

; NUM_SECTORS in these calculations is total (sd_sectors & 0xFFFF0000)
; fatfs size to use (mb) is ((NUM_SECTORS - MLC_SIZE - 0x200800) / 0x800) - 1
; OFFS_SECTORS is count subtracted from num_sectors
USB_OFFS_SECTORS equ (0) ; unused atm
DUMPDATA_OFFS_SECTORS equ (SYSLOG_OFFS_SECTORS + 0x600) ; sectors at NUM_SECTORS-0x03C20800
SYSLOG_OFFS_SECTORS equ (MLC_OFFS_SECTORS + 0x200)      ; sectors at NUM_SECTORS-0x03C20200
MLC_OFFS_SECTORS equ (SLC_OFFS_SECTORS + MLC_SIZE)      ; sectors at NUM_SECTORS-0x03C20000
SLC_OFFS_SECTORS equ (SLCCMPT_OFFS_SECTORS + 0x100000)  ; sectors at NUM_SECTORS-0x00200000
SLCCMPT_OFFS_SECTORS equ (0x100000)                     ; sectors at NUM_SECTORS-0x00100000

FS_MMC_SDCARD_STRUCT equ (0x1089B9F8)
FS_MMC_MLC_STRUCT equ (0x1089B948)

FS_PANIC equ 0x107F6D44
FS_SNPRINTF equ 0x107F5FB4
FS_MEMCPY equ 0x107F4F7C
FS_MEMSET equ 0x107F5018
FS_MEMCMP equ 0x107F4E94
FS_SYSLOG_OUTPUT equ 0x107F0C84
FS_SLEEP equ 0x1071D668
FS_GETMDDEVICEBYID equ 0x107187C4
FS_SDIO_DOREADWRITECOMMAND equ 0x10718A8C
FS_SDIO_DOCOMMAND equ 0x1071C958
FS_MMC_DEVICEINITSOMETHING equ 0x1071992C
FS_MMC_DEVICEINITSOMETHING2 equ 0x1071A4A4
FS_MMC_DEVICEINITSOMETHING3 equ 0x10719F60
FS_SVC_CREATEMUTEX equ 0x107F6BBC
FS_SVC_ACQUIREMUTEX equ 0x107F6BC4
FS_SVC_RELEASEMUTEX equ 0x107F6BCC
FS_SVC_DESTROYMUTEX equ 0x107F6BD4
FS_USB_READ equ 0x1077F1C0
FS_USB_WRITE equ 0x1077F35C
FS_USB_SECTOR_SPOOF equ 0x10781228
FS_SLC_READ1 equ 0x107B998C
FS_SLC_READ2 equ 0x107B98FC
FS_SLC_WRITE1 equ 0x107B9870
FS_SLC_WRITE2 equ 0x107B97E4
FS_SLC_HANDLEMESSAGE equ 0x107B9DE4
FS_MLC_READ1 equ 0x107DC760
FS_MLC_READ2 equ 0x107DCDE4
FS_MLC_WRITE1 equ 0x107DC0C0
FS_MLC_WRITE2 equ 0x107DC73C
FS_SDCARD_READ1 equ 0x107BDDD0
FS_SDCARD_WRITE1 equ 0x107BDD60
FS_ISFS_READWRITEBLOCKS equ 0x10720324
FS_CRYPTO_HMAC equ 0x107F3798
FS_RAW_READ1 equ 0x10732BC0
FS_REGISTER_FS_DRIVER equ 0x10732D70
FS_REGISTERMDPHYSICALDEVICE equ 0x10718860

; patches start here

; null out references to slcSomething1 and slcSomething2
; (nulling them out is apparently ok; more importantly, i'm not sure what they do and would rather get a crash than unwanted slc-writing)
.if USE_REDNAND
.org 0x107B96B8
	.word 0x00000000
	.word 0x00000000
.endif
    
.org 0x10740EF8
    b wfs_crypto_hook

.if USE_REDNAND
; in createDevThread
	.org 0x10700294
		b createDevThread_hook
.endif

; ; usb redirection
 	.org FS_USB_READ
 		b usbRead_patch
 	.org FS_USB_WRITE
 		b usbWrite_patch
    .org FS_USB_SECTOR_SPOOF
        b usb_sector_spoof

; slc redirection
.if USE_REDNAND
	.org FS_SLC_READ1
		b slcRead1_patch
	.org FS_SLC_READ2
		b slcRead2_patch
	.org FS_SLC_WRITE1
		b slcWrite1_patch
	.org FS_SLC_WRITE2
		b slcWrite2_patch
	.org 0x107206F0
		mov r0, #0 ; nop out hmac memcmp
; mlc redirection
	.org FS_SDCARD_READ1
		b sdcardRead_patch
	.org FS_SDCARD_WRITE1
		b sdcardWrite_patch
; FS_GETMDDEVICEBYID
	.org FS_GETMDDEVICEBYID + 0x8
		bl getMdDeviceById_hook
; call to FS_REGISTERMDPHYSICALDEVICE in mdMainThreadEntrypoint
.org 0x107BD81C
		bl registerMdDevice_hook
; mdExit : patch out sdcard deinitialization
	.org 0x107BD374
		bx lr
.endif

; mlcRead1 logging
.if PRINT_MLC_READ
.org FS_MLC_READ1 + 0x4
    bl mlcRead1_dbg
.endif
.if PRINT_MLC_WRITE
.org FS_MLC_WRITE1 + 0x4
    bl mlcWrite1_dbg
.endif

; some selective logging function : enable all the logging !
.org 0x107F5720
	b FS_SYSLOG_OUTPUT

open_base equ 0x1070AF0C
opendir_base equ 0x1070B7F4
.if PRINT_FSAOPEN
.org 0x1070AC94
    bl fsaopen_fullstr_dump_hook

.org opendir_base
    b opendir_hook
.endif
    
; FSA general permissions patch
.org 0x107043E4
.arm
    mov r3, #-1
    mov r2, #-1
    
.if PRINT_FSAREADWRITE
.org 0x1070AA1C
.arm
    bl fread_hook
    
.org 0x1070A7A8
.arm
    bl fwrite_hook
.endif

.if PRINT_FSASEEKFILE
.org 0x1070A46C
.arm
    bl seekfile_hook
.endif

.if MLC_ACCELERATE
; hooks for supporting scfm.img on sys-slccmpt instead of on the sd card
; e.g. doing sd->slc caching instead of sd->sd caching which dramatically slows down ALL i/o
.org 0x107E8178 ; disable scfmInit's slc format on name/partition type error
    mov r0, #0xFFFFFFFE
    
; hook scfmInit right after fsa initialization, before main thread creation
.org 0x107E7B88
    bl scfm_try_slc_cache_migration
.endif

; our own custom code starts here
.org CODE_BASE
.area (0x10800000-CODE_BASE)
.arm

.if MLC_ACCELERATE
SCFM_FSA_HANDLE_PTR equ 0x1108B8B4

FS_FSAMOUNT equ 0x107F0430
FS_FSAUNMOUNT equ 0x107F030C
FS_FSAOPENFILE equ 0x107EFC14 
FS_FSACLOSEFILE equ 0x107EEC20
FS_FSAREMOVEFILE equ 0x107EEB20
FS_FSAREADFILEOFFSET equ 0x107EF594
FS_FSAWRITEFILEOFFSET equ 0x107EEF14
FS_IOS_MEMALIGN equ 0x107F6A44
FS_IOS_FREE equ 0x107F6A4C

FSA_STATUS_NOT_FOUND equ 0xFFFCFFE9
.align 4
scfm_try_slc_cache_migration:
    ldr r0, [r3] ; do replaced instruction and push result for later
    push {r0, r4-r6, lr}
    sub sp, #0x10
    ldr r4, =SCFM_FSA_HANDLE_PTR
    ldr r4, [r4]
    
    ; try to mount slcs
    mov r0, r4
    bl scfm_mount_slcs
    cmp r0, #0
    movne r1, #1 ; Stage 1 error: mounting SLCs
    bne scfm_migrate_error
    
    ; try to open scfm.img on slccmpt. if it does not exist, try to migrate it.
    mov r3, sp ; u32* file_handle
    ldr r2, =str_r ; mode
    ldr r1, =str_cmpt_scfm_img ; fname
    mov r0, r4  ; fsa handle
    bl FS_FSAOPENFILE
    
    cmp r0, #0
    beq scfm_migrate_cmpt_exists
    
    ; throw an exception for any err that's not file not found
    ldr r1, =FSA_STATUS_NOT_FOUND
    cmp r0, r1
    movne r1, #2 ; Stage 2 error: FSAOpenFile of /vol/scfm_cmpt/scfm.img failed
    bne scfm_migrate_error
    
scfm_try_do_migration:
    ; file not found on cmpt - try to migrate!
    mov r0, r4
    bl scfm_do_migration
    cmp r0, #0
    movne r1, #9    ; stage 9: panic for migration fail, see syslog
    bne scfm_migrate_error
    
scfm_migrate_patch_for_slccmpt:
    ; Migration succeeded! patch up paths and file names
    
    ; patch /dev/slc01 to /dev/slccmpt01
    ldr r0, =0x10831510
    ldr r1, =str_dev_slccmpt
    bl scfm_migration_strcpy
    
    ; have scfmInit look for slccmpt instead of slc by tweaking char*'s
    ; (slccmpt appears 0x8 before the existing char*, so cheat by just subtracting)
    ldr r0, =0x107E8630
    ldr r1, [r0]
    sub r1, #8
    str r1, [r0]
    
    ; Try deleting slc scfm.img, as it's not needed at this point
    ldr r1, =str_mnt_scfm_img
    mov r0, r4
    bl FS_FSAREMOVEFILE
    
    ldr r0, =str_migrate_success
    bl FS_SYSLOG_OUTPUT
    
    ; scfm on slccmpt is ready to go!
    b scfm_migrate_return
    
scfm_migrate_cmpt_exists:
    ; file exists, verify it and migrate if it's bad
    ; (TODO: more checking? size check is sufficient for restarting partial migrations...)
    
    ; try to read the highest offset u32 to make sure file size is ok
    ldr r5, [sp] ; file handle in r5
    ldr r0, =0x08003FFC
    str r0, [sp]   ; offset
    str r5, [sp, #4] ; file (src)
    mov r0, #0x20 ; nocrypto flag
    str r0, [sp, #8]
    mov r0, r4  ; fsa handle
    add r1, sp, 0xC  ; buf
    mov r2, 1   ; size
    mov r3, #4  ; count
    bl FS_FSAREADFILEOFFSET
    mov r6, r0
    
    mov r1, r5
    mov r0, r4
    bl FS_FSACLOSEFILE
    cmp r6, #4
    beq scfm_migrate_patch_for_slccmpt
    
    ; bad file read size (probably partial migration.) try deleting and migrating again.
    ldr r1, =str_cmpt_scfm_img
    mov r0, r4
    bl FS_FSAREMOVEFILE
    cmp r0, #0
    beq scfm_try_do_migration
    movne r1, #20       ; Stage 20: slccmpt scfm.img exists, but is invalid and cannot be deleted

; return here to silently default back to slc scfm.img
scfm_migrate_error:
    bl scfm_migration_throw_error
    
scfm_migrate_return: 
    mov r0, r4
    bl scfm_unmount_slcs
    add sp, #0x10
    pop {r0, r4-r6, pc}
    
; main function doing the actual migration
scfm_do_migration:
    push {r4-r9,lr}
    sub sp, #0x10
    mov r4, r0 ; fsa handle in r4
    mov r5, #-1  ; invalid file handles and null file data ptr by default
    mov r6, #-1
    mov r7, #0
    
    ; Open scfm.img on slc for read
    mov r3, sp ; u32* file_handle
    ldr r2, =str_r ; mode
    ldr r1, =str_mnt_scfm_img ; fname
    mov r0, r4  ; fsa handle
    bl FS_FSAOPENFILE
    cmp r0, #0
    movne r1, #4        ; Stage 4: couldn't open scfm.img on slc for transfer
    bne scfm_do_migration_fail_ret
    
    ; Open scfm.img on slccmpt for write
    add r3, sp, #4 ; u32* file_handle
    ldr r2, =str_wplus ; mode
    ldr r1, =str_cmpt_scfm_img ; fname
    mov r0, r4  ; fsa handle
    bl FS_FSAOPENFILE
    cmp r0, #0
    movne r1, #5        ; Stage 5: couldn't open scfm.img on slccmpt for transfer
    bne scfm_do_migration_fail_ret
    
    ldr r5, [sp]    ; source handle in r5
    ldr r6, [sp, #4]; dst handle in r6
    
SCFM_MIGRATION_BLK_SIZE equ (1*1024*1024)

    ; acquire a buffer for our block copy
    mov r0, #1  ; shared heap
    mov r1, #SCFM_MIGRATION_BLK_SIZE
    mov r2, #0x40
    bl FS_IOS_MEMALIGN
    cmp r0, #0
    moveq r1, #6        ; Stage 6: failed to alloc 4MB for scfm.img copy
    beq scfm_do_migration_fail_ret
    
    mov r7, r0  ; allocated buffer in r7
    
    ; TODO: read file size instead of assuming
    ldr r8, =0x8004000  ; size left in r8
    mov r10, #0         ; current offset in r10
    
scfm_migrate_copy_loop:
    ; first: get read size for block
    mov r9, r8  ; current block size in r9
    cmp r9, #SCFM_MIGRATION_BLK_SIZE
    movhi r9, #SCFM_MIGRATION_BLK_SIZE ; use smaller size if over 4mb left
    
    ; read the block
    str r10, [sp]   ; offset
    str r5, [sp, #4] ; file (src)
    mov r0, #0x20 ; nocrypto flag
    str r0, [sp, #8]
    mov r0, r4  ; fsa handle
    mov r1, r7  ; buf
    mov r2, 1   ; size
    mov r3, r9  ; count
    bl FS_FSAREADFILEOFFSET
    cmp r0, r9
    movne r1, #7        ; Stage 7: failed to read source scfm.img
    bne scfm_do_migration_fail_ret
    
    ; write the block
    str r10, [sp]   ; offset
    str r6, [sp, #4] ; file (src)
    mov r0, #0x20 ; nocrypto flag
    str r0, [sp, #8]
    mov r0, r4  ; fsa handle
    mov r1, r7  ; buf
    mov r2, 1   ; size
    mov r3, r9  ; count
    bl FS_FSAWRITEFILEOFFSET
    cmp r0, r9
    movne r1, #8        ; Stage 8: failed to write dst scfm.img
    bne scfm_do_migration_fail_ret
    
    sub r8, r9  ; left -= cur_size
    add r10, r9 ; offset += cur_size
    
    ; print bytes left
    mov r1, r8
    ldr r0, =str_migrate_progress_format
    bl FS_SYSLOG_OUTPUT
    
    cmp r8, #0
    bhi scfm_migrate_copy_loop
    
    mov r0, #0
    b scfm_do_migration_ret

scfm_do_migration_fail_ret:
    bl scfm_migration_throw_error
    mov r0, #-1
    
scfm_do_migration_ret:
    push {r0} ; back up result
    
    ; Clean up all the stuff we (potentially) used
    mov r1, r5
    mov r0, r4
    bl FS_FSACLOSEFILE
    mov r1, r6
    mov r0, r4
    bl FS_FSACLOSEFILE
    
    cmp r7, #0
    beq scfm_do_migration_after_free
    mov r1, r7
    mov r0, #1
    bl FS_IOS_FREE
    
    pop {r0} ; get result
scfm_do_migration_after_free:
    add sp, #0x10
    pop {r4-r9,pc}

; r0 fsa handle
scfm_mount_slcs:
    push {r4, lr}
    sub sp, #8
    mov r4, r0 ; fsa handle in r4
    
    ; mount /dev/slc at /vol/scfm_mnt
    mov r0, r4
    ldr r1, =str_dev_slc
    ldr r2, =str_vol_scfm_mnt
    mov r3, #0
    str r3, [sp]
    str r3, [sp, #4]
    bl FS_FSAMOUNT
    cmp r0, #0
    movne r0, #-1        ; -1: failed to mount /dev/slc
    bne scfm_slc_mount_ret
    
    ; mount /dev/slccmpt at /vol/scfm_cmpt
    mov r0, r4
    ldr r1, =str_dev_slccmpt
    ldr r2, =str_vol_scfm_cmpt
    mov r3, #0
    str r3, [sp]
    str r3, [sp, #4]
    bl FS_FSAMOUNT
    cmp r0, #0
    movne r0, #-2        ; -2: failed to mount /dev/slccmpt
    
scfm_slc_mount_ret:
    add sp, #8
    pop {r4, pc}

scfm_unmount_slcs:
    push {r4, lr}
    mov r4, r0
    
    mov r2, #0
    ldr r1, =str_vol_scfm_cmpt
    mov r0, r4
    bl FS_FSAUNMOUNT
    
    mov r2, #0
    ldr r1, =str_vol_scfm_mnt
    mov r0, r4
    bl FS_FSAUNMOUNT
    pop {r4, pc}
    
; r0 error, r1 stage (where we errored), code is stage-error
scfm_migration_throw_error:
    mov r2, r0
    ldr r0, =str_migration_fail_format
    b FS_SYSLOG_OUTPUT
    
scfm_migration_strcpy:
    mov r2, r0
scfm_migration_strcpy_loop:
    ldrb r3, [r1],#1
    strb r3, [r2],#1
    cmp r3, #0
    bne scfm_migration_strcpy_loop
    bx lr
str_migration_fail_format: .ascii 0xA,"SCFM-SALT: migration failed, %02d-%08X",0xA,0
str_migrate_progress_format: 
    .ascii "SCFM-SALT: Migrating scfm.img, %d bytes remaining...",0xA, 0
str_migrate_success: .ascii "SCFM-SALT: Using scfm.img on /dev/slccmpt.",0xA,0
str_dev_slccmpt: .ascii "/dev/slccmpt01",0
str_dev_slc: .ascii "/dev/slc01",0
str_vol_scfm_cmpt: .ascii "/vol/scfm_cmpt",0
str_cmpt_scfm_img: .ascii "/vol/scfm_cmpt/scfm.img",0
str_vol_scfm_mnt: .ascii "/vol/scfm_mnt",0
str_mnt_scfm_img: .ascii "/vol/scfm_mnt/scfm.img",0
str_r: .ascii "r",0
str_wplus: .ascii "w+",0
.align 4
.endif

.align 4
seekfile_hook:
    push {r0-r3, lr}
    ldr r0, [r0,#4]
    ldr r0, [r0, #0xC] ; get pid
    cmp r0, #FSAPRINT_MAX_PID
    bhi seekfile_return
    
    cmp r0, #FSAPRINT_MIN_PID
    bls seekfile_return
    
    mov r1, r3  ; seek offset
    mov r3, r2  ; handle
    mov r2, r0  ; pid
    adr r0, str_seekfile_format
    bl FS_SYSLOG_OUTPUT
    
seekfile_return:
    pop {r0-r3, lr}
    subs r6, r0, #0
    bx lr
    
str_seekfile_format: 
.ascii "SALT: fileseek to 0x%X pid %d handle 0x%X",0xA,0
.align 4

fread_hook:
    push {r0-r3, lr}
    adr r1, str_read
    b fread_write_print
    
fwrite_hook:
    push {r0-r3, lr}
    adr r1, str_write
    b fread_write_print
    
fread_write_print:
    ldr r0, [r0,#4]
    ldr r0, [r0, #0xC] ; get pid
    cmp r0, #FSAPRINT_MAX_PID
    bhi fread_write_return
    
    cmp r0, #FSAPRINT_MIN_PID
    bls fread_write_return
    
    sub sp, #4
    str r0, [sp]
    
    adr r0, str_read_write_print_format
    ldr r2, [sp, #(4*(9+5+1))]    ; base's sp+0, cnt
    mul r2, r3                  ; cnt * size = r/w size
    ldr r3, [sp, #(4*(9+5+1+2))]  ; base's sp+0x8, handle
    bl FS_SYSLOG_OUTPUT
    add sp, #4
fread_write_return:
    pop {r0-r3, lr}
    subs r10, r0, #0    ; replaced
    bx lr
    
str_read: .ascii "read", 0
str_write: .ascii "write", 0
str_read_write_print_format: 
.ascii "SALT: file %s size 0x%X handle 0x%X pid %d",0xA,0
.align 4
    



wfs_crypto_hook:
    ldrb r3, [lr,#0x19] ; replaced
.if USB_SHRINKSHIFT
    push {r4-r7}
    ldr r4, =NOCRYPTO_WFS_SECTOR_COUNT1
    ldr r6, =NOCRYPTO_WFS_SECTOR_COUNT2
    ldr r7, =NOCRYPTO_WFS_SECTOR_COUNT3
    cmp r5, r4  ; sector count in r5
    cmpne r5, r6
    cmpne r5, r7
    bne wfs_crypto_hook_ret
    
    ; set the key to 0xDEADBEEF to skip crypto
    ldr r5, =0xDEADBEEF
    ldr r4, [lr, #0x24]
    str r5, [r4]
    
wfs_crypto_hook_ret:
    pop {r4-r7}
.endif
    b 0x10740EFC
.align 4
    
fsaopen_fullstr_dump_hook:
    push {r4, lr}
    bl 0x1070E91C   ; fsa_get_full_dir_str
    mov r4, r0
    mov r1, r5
    mov r2, r6
    adr r0, fullstr_str
    bl FS_SYSLOG_OUTPUT
    mov r0, r4
    pop {r4, pc}

fullstr_str: .ascii "SALT: FSAOpen %s mode:'%s'",0xA,0
.align 4

opendir_hook:
    push {r0-r3, lr}
    mov r1, r2
    adr r0, fsaopendir_str
    bl FS_SYSLOG_OUTPUT
    pop {r0-r3, lr}
    
    subs r6, r0, #0
    b (opendir_base+4)

fsaopendir_str: 
.ascii "SALT: FSAOpenDir %s",0xA,0
.align 4
    
sdcard_init:
	; this should run *after* /dev/mmc thread is created
	push {lr}

	; first we create our synchronization stuff
	mov r0, #1
	mov r1, #1
	bl FS_SVC_CREATEMUTEX
	ldr r1, =sdcard_access_mutex
	str r0, [r1]

	ldr r1, =dumpdata_offset
	mov r0, #0
	str r0, [r1]

	; then we sleep until /dev/mmc is done initializing sdcard (TODO : better synchronization here)
	mov r0, #1000
	bl FS_SLEEP

	; finally we set some flags to indicate sdcard is ready for use
	ldr r0, =FS_MMC_SDCARD_STRUCT
	ldr r1, [r0, #0x24]
	orr r1, #0x20
	str r1, [r0, #0x24]
	ldr r1, [r0, #0x28]
	bic r1, #0x4
	str r1, [r0, #0x28]
    
    ; set sdcard size for emu hooks
    ldr r1, [r0, #0x30]
    mov r0, #0
    sub r0, #0x10000
    and r1, r0  ; sdcard_size &= 0xFFFF0000
    ldr r0, =sdcard_num_sectors
    str r1, [r0]

    ; TODO: spoof MLC sector count for resized emunand (this patch is ready)
    ; ldr r0, =FS_MMC_MLC_STRUCT
    ; ldr r1, =MLC_SIZE
    ; str r1, [r0, #0x30]

	pop {pc}

mlc_init:
	; this should run *after* /dev/mmc thread is created (and after sdcard_init)
	; this should also only be run when you want to dump mlc; this will cause the physical mlc device to be inaccessible to regular FS code
	push {lr}

	; finally we set some flags to indicate sdcard is ready for use
	ldr r0, =FS_MMC_MLC_STRUCT
	ldr r1, [r0, #0x24]
	orr r1, #0x20
	str r1, [r0, #0x24]
	ldr r1, [r0, #0x28]
	bic r1, #0x4
	str r1, [r0, #0x28]

	pop {pc}

; r0 : bool read (0 = read, not 0 = write), r1 : data_ptr, r2 : cnt, r3 : block_size, sparg0 : offset_blocks, sparg1 : out_callback_arg2, sparg2 : device_id
sdcard_readwrite:
	sdcard_readwrite_stackframe_size equ (4*4+12*4)
	push {r4,r5,r6,lr}
	sub sp, #12*4

	; pointer for command paramstruct
	add r4, sp, #0xC
	; pointer for mutex (sp + 0x8 will be callback's arg2)
	add r5, sp, #0x4
	; offset_blocks
	ldr r6, [sp, #sdcard_readwrite_stackframe_size]

	; first of all, grab sdcard mutex
	push {r0,r1,r2,r3}
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	; also create a mutex for synchronization with end of operation...
	mov r0, #1
	mov r1, #1
	bl FS_SVC_CREATEMUTEX
	str r0, [r5]
	; ...and acquire it
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	pop {r0,r1,r2,r3}

	; block_size needs to be equal to sector_size (0x200)
	sdcard_readwrite_block_size_adjust:
		cmp r3, #0x200
		movgt r3, r3, lsr 1 ; block_size >>= 1;
		movgt r2, r2, lsl 1 ; cnt <<= 1;
		movgt r6, r6, lsl 1 ; offset_blocks <<= 1;
		bgt sdcard_readwrite_block_size_adjust

	; build rw command paramstruct 
	str r2, [r4, #0x00] ; cnt
	str r3, [r4, #0x04] ; block_size
	cmp r0, #0
	movne r0, #0x3 ; read operation
	str r0, [r4, #0x08] ; command type
	str r1, [r4, #0x0C] ; data_ptr
	mov r0, #0
	str r0, [r4, #0x10] ; offset_high
	str r6, [r4, #0x14] ; offset_low
	str r0, [r4, #0x18] ; callback
	str r0, [r4, #0x1C] ; callback_arg
	mvn r0, #0
	str r0, [r4, #0x20] ; -1

	; setup parameters
	ldr r0, [sp, #sdcard_readwrite_stackframe_size+0x8] ; device_identifier : sdcard (real one is 0x43, but patch makes 0xDA valid)
	mov r1, r4 ; paramstruct
	mov r2, r6 ; offset_blocks
	adr r3, sdcard_readwrite_callback ; callback
	str r5, [sp] ; callback_arg (mutex ptr)

	; call readwrite function
	bl FS_SDIO_DOREADWRITECOMMAND
	mov r4, r0
	cmp r0, #0
	bne sdcard_readwrite_skip_wait

	; wait for callback to give the go-ahead
	ldr r0, [r5]
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	ldr r0, [r5, #0x4]
	ldr r1, [sp, #sdcard_readwrite_stackframe_size+0x4]
	cmp r1, #0
	strne r0, [r1]
	sdcard_readwrite_skip_wait:

	; finally, release sdcard mutexes
	ldr r0, [r5]
	bl FS_SVC_DESTROYMUTEX
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	bl FS_SVC_RELEASEMUTEX

	; return
	mov r0, r4
	add sp, #12*4
	pop {r4,r5,r6,pc}

	; release mutex to let everyone know we're done
sdcard_readwrite_callback:
    str r1, [r0, #4]
    ldr r0, [r0]
    b FS_SVC_RELEASEMUTEX

createDevThread_hook:
	push {r0}
	; check if we were initializing /dev/mmc
	ldr r0, [r4, #0x8]
	cmp r0, #0xD
	bne createDevThread_hook_skip1

	bl sdcard_init

	bl clear_screen

	mov r0, #20
	mov r1, #20
	adr r2, createDevThread_hook_welcome
	bl _printf

	createDevThread_hook_skip1:

	; check if we were initializing /dev/fla
	;ldr r0, [r4, #0x8]
	;cmp r0, #0x7
	;bne createDevThread_hook_skip2

	;createDevThread_hook_skip2:
	pop {r0,r4-r8,pc}
	createDevThread_hook_welcome:
	.ascii "welcome to redNAND: SALT edition",0
	.align 0x4
    

getMdDeviceById_hook:
	mov r4, r0
	cmp r0, #0xDA ; magic id (sdcard)
	ldreq r0, =FS_MMC_SDCARD_STRUCT
	popeq {r4,r5,pc}
	cmp r0, #0xAB ; magic id (mlc)
	ldreq r0, =FS_MMC_MLC_STRUCT
	popeq {r4,r5,pc}
	bx lr ; return if different

registerMdDevice_hook:
	push {r4,lr}

	cmp r0, #0
	beq registerMdDevice_hook_skip

	ldr r3, [r0, #0x8] ; mmc device struct ptr
	ldr r4, =FS_MMC_SDCARD_STRUCT
	cmp r3, r4
	bne registerMdDevice_hook_skip

	; sdcard ! fix stuff up so that registration can happen ok

	push {r0-r3}
	
	; first lock that mutex
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	
	; then, clear the flag we set earlier (that FS_REGISTERMDPHYSICALDEVICE will set back anyway)
	ldr r0, =FS_MMC_SDCARD_STRUCT
	ldr r1, [r0, #0x24]
	bic r1, #0x20
	str r1, [r0, #0x24]

	pop {r0-r3}

	; register it !
	bl FS_REGISTERMDPHYSICALDEVICE
	mov r4, r0

	; finally, release the mutex
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	bl FS_SVC_RELEASEMUTEX

	mov r0, r4
	pop {r4,pc}

	registerMdDevice_hook_skip:
	; not sdcard
	bl FS_REGISTERMDPHYSICALDEVICE
	pop {r4,pc}

; read1(void *physical_device_info, int offset_high, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
; readWriteCallback_patch(bool read, int offset_offset, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
readWriteCallback_patch:
	readWriteCallback_patch_stackframe_size equ (7*4)
	push {r0,r1,r2,r3,r4,r5,lr}
	mov r5, #0xDA
	str r5, [sp, #0x8] ; device id (sdcard)
	add r5, sp, #0xC ; out_callback_arg2 dst
	add r2, r1
	str r2, [sp] ; offset
	str r5, [sp, #4] ; out_callback_arg2
	ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0x4] ; data_ptr
	mov r2, r3 ; cnt
	ldr r3, [sp, #readWriteCallback_patch_stackframe_size] ; block_size
	bl sdcard_readwrite
	mov r4, r0
	cmp r0, #0
	bne readWriteCallback_patch_skip_callback

	ldr r12, [sp, #readWriteCallback_patch_stackframe_size+0x8] ; callback
	ldr r0, [r5]
	ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0xC] ; callback_parameter
	cmp r12, #0
	blxne r12

	readWriteCallback_patch_skip_callback:
	mov r0, r4
	add sp, #4
	pop {r1,r2,r3,r4,r5,pc}

; ; ; ; ; ; ; ; ; ;
;    USB TWEAKS   ;
; ; ; ; ; ; ; ; ; ;

; spoof mass storage sector count, if it matches
usb_sector_spoof:
    ldr r11, [r7, #0x20]
.if USB_SHRINKSHIFT
    add r9, r11, 0x20
    ldr r2, [r9]
    ldr r1, =(USB_SHRINK_ORIG_SECTOR_COUNT1-1)
    ldr r3, =(USB_SHRINK_SECTOR_COUNT1-1) ; these numbers are technically 'last sector'/'highest lba'
    cmp r2, r1
    ldrne r1, =(USB_SHRINK_ORIG_SECTOR_COUNT2-1)
    ldrne r3, =(USB_SHRINK_SECTOR_COUNT2-1)
    cmpne r2, r1
    ldrne r1, =(USB_SHRINK_ORIG_SECTOR_COUNT3-1)
    ldrne r3, =(USB_SHRINK_SECTOR_COUNT3-1)
    cmpne r2, r1
    streq r3, [r9]
.endif
    b (FS_USB_SECTOR_SPOOF+4)

; add an offset to mass storage sector reads/writes
usbRead_patch:
    push {r4-r11, lr}
    
.if PRINT_USB_RW
    push {r0-r3}
    ldr r0, [sp,#((9+4)*4)]
    mul r3, r0
    adr r0, usb_str
    adr r1, read_str
    bl FS_SYSLOG_OUTPUT
    pop {r0-r3}
.endif
    
.if USB_SHRINKSHIFT
    ; get max lba from *(*(ctx+0x20)+0x20)
    ldr r5, [r0, #0x20]
    ldr r6, [r5, #0x20]
    ldr r4, =(USB_SHRINK_SECTOR_COUNT1-1)   ; see if it matches any of the sector counts
    cmp r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT2-1)
    cmpne r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT3-1)
    cmpne r4, r6
    moveq r7, #0x800     ; if so, add our offset of 0x800 sectors (1MB)
    addeq  r2, r7
.endif
    b (FS_USB_READ+4)

usbWrite_patch:
    push {r4-r11, lr}
    
.if PRINT_USB_RW
    push {r0-r3}
    ldr r0, [sp,#((9+4)*4)]
    mul r3, r0
    adr r0, usb_str
    adr r1, write_str
    bl FS_SYSLOG_OUTPUT
    pop {r0-r3}
.endif
    
.if USB_SHRINKSHIFT
    ; get max lba from *(*(ctx+0x20)+0x20)
    ldr r5, [r0, #0x20]
    ldr r6, [r5, #0x20]
    ldr r4, =(USB_SHRINK_SECTOR_COUNT1-1)   ; see if it matches any of the sector counts
    cmp r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT2-1)
    cmpne r4, r6
    ldrne r4, =(USB_SHRINK_SECTOR_COUNT3-1)
    cmpne r4, r6
    moveq r7, #0x800     ; if so, add our offset of 0x800 sectors (1MB)
    addeq  r2, r7
.endif
    b (FS_USB_WRITE+4)
usb_str:.ascii "SALT: USB %s offset 0x%X size 0x%X", 0xA, 0
read_str:.ascii "read", 0
write_str:.ascii "write",0
.align 4


; ; ; ; ; ; ; ; ; ; ;
; SDIO REDIRECTION  ;
; ; ; ; ; ; ; ; ; ; ;

sdcardReadWrite_patch:
	push {r2}
	ldr r2, [r0, #0x14]
	mov r0, r1
	cmp r2, #6 ; DEVICETYPE_SDCARD
    moveq r1, #0
    beq sdcardReadWrite_patch_cont
	ldr r2, =MLC_OFFS_SECTORS ; mlc
    ldr r1, =sdcard_num_sectors
    ldr r1, [r1]
    sub r1, r2
    
sdcardReadWrite_patch_cont:
	pop {r2}
	b readWriteCallback_patch

sdcardRead_patch:
	mov r1, #1 ; read
	b sdcardReadWrite_patch

sdcardWrite_patch:
	mov r1, #0 ; write
	b sdcardReadWrite_patch

; ; ; ; ; ; ; ; ; ;
; SLC REDIRECTION ;
; ; ; ; ; ; ; ; ; ;

slcReadWrite_patch:
	push {r2}
	ldr r2, [r0, #4]
	mov r0, r1
	cmp r2, #0
	ldrne r2, =SLC_OFFS_SECTORS
	ldreq r2, =SLCCMPT_OFFS_SECTORS
    ldr r1, =sdcard_num_sectors
    ldr r1, [r1]
    
    sub r1, r2
    lsr r1, 2   ; 4 sd sectors -> 1 slc sector
	pop {r2}
	b readWriteCallback_patch

slcRead1_patch:
.if USE_SYS_SLCCMPT
    push {r0, r1}
	ldr r1, [r0, #4]
	cmp r1, #0
    pop {r0, r1}
	bne slcRead1_patch_cont
    
    ; slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b FS_SLC_READ1+4
    
.endif
slcRead1_patch_cont:
	mov r1, #1 ; read
	b slcReadWrite_patch

slcWrite1_patch:
.if USE_SYS_SLCCMPT
    push {r0, r1}
	ldr r1, [r0, #4]
	cmp r1, #0
    pop {r0, r1}
	bne slcWrite1_patch_cont
    
    ; slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b FS_SLC_WRITE1+4
.endif
slcWrite1_patch_cont:
	mov r1, #0 ; write
	b slcReadWrite_patch

slcRead2_patch:
.if USE_SYS_SLCCMPT
    push {r0, r1}
	ldr r1, [r0, #4]
	cmp r1, #0
    pop {r0, r1}
	bne slcRead2_patch_cont
    
    ; slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b FS_SLC_READ2+4
.endif
slcRead2_patch_cont:
	slcRead2_patch_stackframe_size equ (0x10+7*4)
	push {r0-r5,lr}
	sub sp, #0x10
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x00]
	str r4, [sp, #0x0] ; block_size
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x08]
	str r4, [sp, #0x4] ; data_outptr
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x10]
	str r4, [sp, #0x8] ; callback
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x14]
	str r4, [sp, #0xC] ; callback_parameter
	bl slcRead1_patch_cont
	add sp, #0x14
	pop {r1-r5,pc}

    
slcWrite2_patch:
.if USE_SYS_SLCCMPT
    push {r0, r1}
	ldr r1, [r0, #4]
	cmp r1, #0
    pop {r0, r1}
	bne slcWrite2_patch_cont
    
    ; slccmpt, do replaced instruction and return to use syscmpt
    stmfd sp!, {r4-r8,lr}
    b FS_SLC_WRITE2+4
.endif
slcWrite2_patch_cont:
	slcWrite2_patch_stackframe_size equ (0x10+6*4)
	push {r0-r4,lr}
	sub sp, #0x10
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x00]
	str r4, [sp, #0x0] ; block_size
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x08]
	str r4, [sp, #0x4] ; data_outptr
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x10]
	str r4, [sp, #0x8] ; callback
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x14]
	str r4, [sp, #0xC] ; callback_parameter
	bl slcWrite1_patch_cont
	add sp, #0x14
	pop {r1-r4,pc}

; ; ; ; ; ; ; ; ; ;
;   DEBUG STUFF   ;
; ; ; ; ; ; ; ; ; ;

mlcRead1_dbg:
	mlcRead1_dbg_stackframe equ (4*6)
	mov r12, r0
	push {r0-r3,r12,lr}
	adr r0, mlcRead1_dbg_format
	ldr r1, [sp, #mlcRead1_dbg_stackframe+9*4]
	bl FS_SYSLOG_OUTPUT
	pop {r0-r3,lr,pc} ; replaces mov lr, r0
	mlcRead1_dbg_format:
		.ascii "mlcRead1 : %08X %08X %08X",0xA,0
		.align 0x4
    
mlcWrite1_dbg:
	mlcWrite1_dbg_stackframe equ (4*6)
	mov r12, r0
	push {r0-r3,r12,lr}
	adr r0, mlcWrite1_dbg_format
	ldr r1, [sp, #mlcWrite1_dbg_stackframe+9*4]
	bl FS_SYSLOG_OUTPUT
	pop {r0-r3,lr,pc} ; replaces mov lr, r0
	mlcWrite1_dbg_format:
		.ascii "mlcWrite1 : %08X %08X %08X",0xA,0
		.align 0x4

; r0 : data ptr
; r1 : size
; r2 : offset_blocks
; r3 : read
rw_data_offset:
	push {r1,r2,r3,r4,lr}
    mov r4, r3
	mov r3, r1, lsr 9 ; size /= 0x200
	cmp r3, #0
	moveq r3, #1
	mov r1, r0 ; data_ptr
	str r2, [sp] ; offset
	mov r0, #0
	str r0, [sp, #0x4] ; out_callback_arg2
	mov r0, #0xDA
	str r0, [sp, #0x8] ; device id
    cmp r3, #1
	moveq r0, #1 ; read
	movne r0, #0 ; write
	mov r2, r3 ; num_sectors
	mov r3, #0x200 ; block_size
	bl sdcard_readwrite
	add sp, #0xC
	pop {r4,pc}

; r0 : data ptr
; r1 : size
dump_data:
	push {r1,r2,r3,r4,lr}
	mov r3, r1, lsr 9 ; size /= 0x200
	cmp r3, #0
	moveq r3, #1
	mov r1, r0 ; data_ptr
	ldr r2, =DUMPDATA_OFFS_SECTORS ; offset_sectors
    ldr r0, =sdcard_num_sectors
    ldr r0, [r0]
    sub r0, r2
	ldr r4, =dumpdata_offset
	ldr r2, [r4]
	add r0, r2
	str r0, [sp]
	add r2, r3
	str r2, [r4]
	mov r0, #0
	str r0, [sp, #0x4] ; out_callback_arg2
	mov r0, #0xDA
	str r0, [sp, #0x8] ; device id
	mov r0, #0 ; write
	mov r2, r3 ; num_sectors
	mov r3, #0x200 ; block_size
	bl sdcard_readwrite
	add sp, #0xC
	pop {r4,pc}

dump_lots_data:
	push {r4-r6,lr}
	mov r4, r0 ; addr
	mov r5, r1 ; size
	dump_lots_data_loop:
		mov r6, #0x40000 ; cur_size
		cmp r6, r5
		movgt r6, r5
		ldr r0, =sdcard_read_buffer
		mov r1, r4
		mov r2, r6
		bl FS_MEMCPY
		ldr r0, =sdcard_read_buffer ; data_ptr
		mov r1, r6 ; size
		bl dump_data
		add r4, r6
		sub r5, r6
		cmp r6, #0
		bne dump_lots_data_loop
	pop {r4-r6,pc}

; r0 : device id
getPhysicalDeviceHandle:
	ldr r1, =0x1091C2EC
	mov r2, #0x204
	mla r1, r2, r0, r1
	ldrh r1, [r1, #6]
	orr r0, r1, r0, lsl 16
	bx lr

; r0 : dst, r1 : offset, r2 : sectors, r3 : device id
; rawRead1PhysicalDevice_(int physical_device_handle, unsigned int offset_high, unsigned int offset_low, unsigned int size, void *outptr, int (__fastcall *callback)(unsigned int, int), int callback_arg)
read_slc:
	push {lr}
	sub sp, #0xC
	str r0, [sp] ; outptr
	mov r0, #0
	str r0, [sp, #4] ; callback
	str r0, [sp, #8] ; callback_arg
	push {r1-r3}
	mov r0, r3
	bl getPhysicalDeviceHandle
	pop {r1-r3}
	mov r3, r2 ; cnt
	mov r2, r1 ; offset_low
	mov r1, #0 ; offset_high
	BL FS_RAW_READ1
	add sp, #0xC
	pop {pc}

; r0 : data ptr
; r1 : num_sectors
; r2 : offset_sectors
read_mlc:
	push {r1,r2,r3,r4,lr}
	str r2, [sp]
	mov r2, r1 ; num_sectors
	mov r1, r0 ; data_ptr
	ldr r0, =mlc_out_callback_arg2
	str r0, [sp, #0x4] ; out_callback_arg2
	mov r0, #0xAB
	str r0, [sp, #0x8] ; device id
	mov r0, #1 ; read
	mov r3, #0x200 ; block_size
	bl sdcard_readwrite
	add sp, #0xC
	pop {r4,pc}
.pool

clear_screen:
	push {lr}
	ldr r0, =FRAMEBUFFER_ADDRESS ; data_ptr
	ldr r1, =0x00
	ldr r2, =FRAMEBUFFER_STRIDE*504
	bl FS_MEMSET
	pop {pc}

; r0 : x, r1 : y, r2 : format, ...
; NOT threadsafe so dont even try you idiot
_printf:
	ldr r12, =_printf_xylr
	str r0, [r12]
	str r1, [r12, #4]
	str lr, [r12, #8]
	ldr r0, =_printf_string
	mov r1, #_printf_string_end-_printf_string
	bl FS_SNPRINTF
	ldr r12, =_printf_xylr
	ldr r1, [r12]
	ldr r2, [r12, #4]
	ldr lr, [r12, #8]
	push {lr}
	ldr r0, =_printf_string
	bl drawString
	pop {pc}


; r0 : str, r1 : x, r2 : y
drawString:
	push {r4-r6,lr}
	mov r4, r0
	mov r5, r1
	mov r6, r2
	drawString_loop:
		ldrb r0, [r4], #1
		cmp r0, #0x00
		beq drawString_end
		mov r1, r5
		mov r2, r6
		bl drawCharacter
		add r5, #CHARACTER_SIZE
		b drawString_loop
	drawString_end:
	pop {r4-r6,pc}

; r0 : char, r1 : x, r2 : y
drawCharacter:
	subs r0, #32
	; bxlt lr
	push {r4-r7,lr}
	ldr r4, =FRAMEBUFFER_ADDRESS ; r4 : framebuffer address
	add r4, r1, lsl 2 ; add x * 4
	mov r3, #FRAMEBUFFER_STRIDE
	mla r4, r2, r3, r4
	adr r5, font_data ; r5 : character data
	add r5, r0, lsl 3 ; font is 1bpp, 8x8 => 8 bytes represents one character
	mov r1, #0xFFFFFFFF ; color
	mov r2, #0x0 ; empty color
	mov r6, #8 ; i
	drawCharacter_loop1:
		mov r3, #CHARACTER_MULT
		drawCharacter_loop3:
			mov r7, #8 ; j
			ldrb r0, [r5]
			drawCharacter_loop2:
				tst r0, #1
				; as many STRs as CHARACTER_MULT (would be nice to do this in preproc...)
				streq r1, [r4], #4
				streq r1, [r4], #4
				strne r2, [r4], #4
				strne r2, [r4], #4
				mov r0, r0, lsr #1
				subs r7, #1
				bne drawCharacter_loop2
			add r4, #FRAMEBUFFER_STRIDE-CHARACTER_SIZE*4
			subs r3, #1
			bne drawCharacter_loop3
		add r5, #1
		subs r6, #1
		bne drawCharacter_loop1
	pop {r4-r7,pc}

.pool

font_data:
	.incbin "patches/font.bin"

.endarea
.Close

.create "patched_sections/0x10835000.bin",0x10835000

.org FS_BSS_START
.area (0x11f00000-FS_BSS_START)
    sdcard_num_sectors:
        .word 0x00000000
	sdcard_access_mutex:
		.word 0x00000000
	dumpdata_offset:
		.word 0x00000000
	mlc_out_callback_arg2:
		.word 0x00000000
	_printf_xylr:
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
	_printf_string:
		.fill ((_printf_string + 0x100) - .), 0x00
	_printf_string_end:
	.align 0x40
	syslog_buffer:
		.fill ((syslog_buffer + 0x40000) - .), 0x00
	.align 0x40
	sdcard_read_buffer:
		.fill ((sdcard_read_buffer + 0x100000) - .), 0x00
	sdcard_read_back_buffer:
		.fill ((sdcard_read_back_buffer + 0x100000) - .), 0x00

.endarea
.Close
