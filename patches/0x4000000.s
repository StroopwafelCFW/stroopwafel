.arm.big

.open "sections/0x4000000.bin","patched_sections/0x4000000.bin",0x04000000

SECTION_BASE equ 0x04000000
SECTION_SIZE equ 0x00017020
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)

.include "config.s"

; nop out memcmp hash checks
.org 0x040017E0
	mov r0, #0
.org 0x040019C4
	mov r0, #0
.org 0x04001BB0
	mov r0, #0
.org 0x04001D40
	mov r0, #0
    
; hook USB key generation's call to seeprom read to use a replacement seed
.if USB_SEED_SWAP
.org 0x04004584
    .arm
    bl usb_seedswap
.endif

.org 0x040045A8 ; allow seeds which don't match otp
    .arm
    mov r2, #0
    mov r3, #0

; hook IOSC_Decrypt after it sanity-checks input (mov r0, r5)
.org 0x04002D5C
.arm
    bl crypto_keychange_hook
; hook IOSC_Decrypt after crypto engine acquired, after r3 setup (mov r1, r6)
.org 0x04002EE8
.arm
    bl crypto_disable_hook
    
; hook IOSC_Encrypt after it sanity-checks input (mov r0, r5)
.org 0x04003120
.arm
    bl crypto_keychange_hook
; hook IOSC_Encrypt after crypto engine acquired, after r3 setup (mov r1, r6)
.org 0x040032A0
.arm
    bl crypto_disable_hook

    
; custom code starts here
.org CODE_BASE
.area (0x04020000 - CODE_BASE )

; These hooks cause key 0xDEADBEEF to do no crypto, copying if needed.
; crypto_keychange_hook sets the key to a valid handle all processes have access to, and
; sets a flag at crypt_info + 0xC (unused) so the later hook knows to not use crypto.
; crypto_disable_hook checks the flag we set and sets crypto mode in crypt_data+4 to 0 if needed
crypto_keychange_hook:
    ldr     r0, [r7, #8] ; get key in use
    ldr     r1, =0xDEADBEEF
    mov     r2, #0x6    ; wii sd key, all pids can use this
    cmp     r0, r1
    streq   r2, [r7,#8] ; update 
    streq   r1, [r7,#12]; set hax flag
    
    mov     r0, r5      ; replaced
    bx      lr
    
crypto_disable_hook:
    ldr     r0, [r7, #12]
    ldr     r1, =0xDEADBEEF
    cmp     r0, r1
    moveq   r0, #0
    streq   r0, [r3]
    
    mov     r1, r6     ;replaced
    bx lr

usb_seedswap:
    .arm
    push {r4,r5}
    mov r0, r2
    adr r1, usb_seed
    ldmia r1, {r2-r5}
    stmia r0, {r2-r5}
    pop {r4,r5}
    
    mov r0, #0
    bx lr
    
usb_seed:
.word USB_SEED_0
.word USB_SEED_1
.word USB_SEED_2
.word USB_SEED_3
.pool

.endarea
.Close
