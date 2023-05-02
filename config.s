; *****************************
; SALT IOSUHAX CONFIGURATION
; *****************************

; Please note: This will largely be succeeded by an ini-based realtime config system
; similar to that used with firmloader+firmbuild on 3DS.

; *****************************
; Filesystem config
; *****************************
; Enable NAND redirection (use minute to prepare your SD card)
USE_REDNAND equ 0

; OTP does not exist, and should be loaded from memory instead
OTP_IN_MEM equ 1

; WIP vWii junk that doesn't really work quite yet
WIP_VWII_JUNK equ 0

; Experimental vWii PPC overclock
VWII_PPC_OC equ 0

; Hooks semihosting operations with GPIO output
HOOK_SEMIHOSTING equ 1

; Mirrors syslog to semihosting write
SYSLOG_SEMIHOSTING_WRITE equ 1

; Enables serial output over the debug (NDEV_LED) GPIOs
GPIO_SERIAL_CONSOLE equ 1

; Disables the disk drive by overriding the SEEPROM configuration to 0x0002 (None)
DISABLE_DISK_DRIVE equ 1

; Set the size of mlc storage, in sectors 
; (TODO: autoconfig for this! we need to support arbitrary sizes anyway)
MLC_SIZE equ (0x03A20000) ; 32GB
; MLC_SIZE equ (0x00E50000) ; 8GB

; Use sysnand vWii slc instead of redirecting it to the SD card.
; This allows vWii and eShop Wii games to boot, and is basically 100% safe.
USE_SYS_SLCCMPT equ 1

; Dramatically accelerate emulated MLC speed by moving the MLC cache from SLC (on SD)
; to compat-SLC (on system, when USE_SYS_SLCCMPT is set).
; This removes significant bottlenecks from MLC I/O and reduces wear on the SD card.
; This will use just over 128MB of free space on vWii NAND.
;
; BACK UP YOUR REDNAND BEFORE ENABLING THIS! The migration process is stable and safe
; but is inherently dangerous.
MLC_ACCELERATE equ 0

; This setting enables a number of features for (manually selected) USB disks.
; In short: The beginning of the WiiU-formatted data on disk will begin 1MB in
; instead of at sector 0, the size of the partition can be set, and all disk 
; encryption will be disabled for better performance and accessibility of data.
; A shrunken disk can have a normal MBR and filesystem around the Wii U data,
; allowing for dual-format disks.
;
; Make sure to set the config for your disks below!
USB_SHRINKSHIFT equ 0

; Configure up to 3 different disks for the above. If a disk is not specified
; here, it will use the normal (offset +0, not shrunken, encrypted) format.
; This is useful for initial migration (e.g. copy from an encrypted device to a new, decrypted one)
USB_SHRINK_ORIG_SECTOR_COUNT1 equ 0x74706DB0
USB_SHRINK_SECTOR_COUNT1 equ 0x38000000
USB_SHRINK_ORIG_SECTOR_COUNT2 equ -1
USB_SHRINK_SECTOR_COUNT2 equ -1
USB_SHRINK_ORIG_SECTOR_COUNT3 equ -1
USB_SHRINK_SECTOR_COUNT3 equ -1

; Allow USB drives to host FAT32 filesystems
FAT32_USB equ 0

; Don't use MCP patches (useful for MenuChangers)
MCP_PATCHES equ 1

; *****************************
; Crypto config
; *****************************
; If set, use this seed instead of the one from SEEPROM+0xB0 for usb crypto key generation.
; This can help with manually migrating between systems.
USB_SEED_SWAP equ 0
USB_SEED_0 equ 0xDEADBEEF
USB_SEED_1 equ 0xCAFEBABE
USB_SEED_2 equ 0x42042069
USB_SEED_3 equ 0x0BADBEEF

; *****************************
; Filesystem debug
; *****************************
PRINT_FSAOPEN equ 0 ; Print FSAOpenFile and FSAOpenDirectory calls

PRINT_FSAREADWRITE equ 0 ; More intense FSA prints
PRINT_FSASEEKFILE equ 0
; pids out of these bounds won't have their FSA r/w or seekfile printed
; print all = 0, 21. print ios = 0,13. print cos = 14,21
FSAPRINT_MIN_PID equ 0
FSAPRINT_MAX_PID equ 13

PRINT_MLC_READ equ 0 ; Print MLC read calls
PRINT_MLC_WRITE equ 0 ; Print MLC write calls
PRINT_USB_RW equ 0 ; Print USB read/write calls

; dump current syslog contents to SYSLOG_OFFS_SECTORS on every FSA log write.
; (debugging feature, will cause slowdown and hammer on the SD card)
DUMP_LOG equ 0