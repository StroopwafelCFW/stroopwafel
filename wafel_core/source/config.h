#ifndef _CONFIG_H
#define _CONFIG_H

// *****************************
// SALT IOSUHAX CONFIGURATION
// *****************************

// Please note: This will largely be succeeded by an ini-based realtime config system
// similar to that used with firmloader+firmbuild on 3DS.

#define MCP_ALT_BASE (0x05200000)
#define FS_ALT_BASE (0x10600000)

// *****************************
// Filesystem config
// *****************************
// Enable NAND redirection (use minute to prepare your SD card)
#define USE_REDNAND 0

// OTP does not exist, and should be loaded from memory instead
#define OTP_IN_MEM 1

// WIP vWii junk that doesn't really work quite yet
#define WIP_VWII_JUNK 0

// Experimental vWii PPC overclock
#define VWII_OVERCLOCK 0

// Hooks semihosting operations with GPIO output
#define HOOK_SEMIHOSTING 1

// Mirrors syslog to semihosting write
#define SYSLOG_SEMIHOSTING_WRITE 1

// Enables serial output over the debug (NDEV_LED) GPIOs
#define GPIO_SERIAL_CONSOLE 1

// Disables the disk drive by overriding the SEEPROM configuration to 0x0002 (None)
#define DISABLE_DISK_DRIVE 0

// Set the size of mlc storage, in sectors 
// (TODO: autoconfig for this! we need to support arbitrary sizes anyway)
#define MLC_SIZE (0x03A20000) // 32GB
// MLC_SIZE (0x00E50000) // 8GB

// Use sysnand vWii slc instead of redirecting it to the SD card.
// This allows vWii and eShop Wii games to boot, and is basically 100% safe.
#define USE_SYS_SLCCMPT 1

// Dramatically accelerate emulated MLC speed by moving the MLC cache from SLC (on SD)
// to compat-SLC (on system, when USE_SYS_SLCCMPT is set).
// This removes significant bottlenecks from MLC I/O and reduces wear on the SD card.
// This will use just over 128MB of free space on vWii NAND.
//
// BACK UP YOUR REDNAND BEFORE ENABLING THIS! The migration process is stable and safe
// but is inherently dangerous.
#define MLC_ACCELERATE 0

// This setting enables a number of features for (manually selected) USB disks.
// In short: The beginning of the WiiU-formatted data on disk will begin 1MB in
// instead of at sector 0, the size of the partition can be set, and all disk 
// encryption will be disabled for better performance and accessibility of data.
// A shrunken disk can have a normal MBR and filesystem around the Wii U data,
// allowing for dual-format disks.
//
// Make sure to set the config for your disks below!
#define USB_SHRINKSHIFT 0

// Configure up to 3 different disks for the above. If a disk is not specified
// here, it will use the normal (offset +0, not shrunken, encrypted) format.
// This is useful for initial migration (e.g. copy from an encrypted device to a new, decrypted one)
#define USB_SHRINK_ORIG_SECTOR_COUNT1   (0x74706DB0)
#define USB_SHRINK_SECTOR_COUNT1        (0x38000000)
#define USB_SHRINK_ORIG_SECTOR_COUNT2   (-1)
#define USB_SHRINK_SECTOR_COUNT2        (-1)
#define USB_SHRINK_ORIG_SECTOR_COUNT3   (-1)
#define USB_SHRINK_SECTOR_COUNT3        (-1)

// Allow USB drives to host FAT32 filesystems
#define FAT32_USB 0

// Don't use MCP patches (useful for MenuChangers)
#define MCP_PATCHES 1

// *****************************
// Crypto config
// *****************************
// If set, use this seed instead of the one from SEEPROM+0xB0 for usb crypto key generation.
// This can help with manually migrating between systems.
#define USB_SEED_SWAP 0
#define USB_SEED_0 0xDEADBEEF
#define USB_SEED_1 0xCAFEBABE
#define USB_SEED_2 0x42042069
#define USB_SEED_3 0x0BADBEEF

// *****************************
// Filesystem debug
// *****************************
#define PRINT_FSAOPEN       0 // Print FSAOpenFile and FSAOpenDirectory calls

#define PRINT_FSAREADWRITE  0 // More intense FSA prints
#define PRINT_FSASEEKFILE   0
// pids out of these bounds won't have their FSA r/w or seekfile printed
// print all = 0, 21. print ios = 0,13. print cos = 14,21
#define FSAPRINT_MIN_PID    0
#define FSAPRINT_MAX_PID    13

#define PRINT_MLC_READ      0 // Print MLC read calls
#define PRINT_MLC_WRITE     0 // Print MLC write calls
#define PRINT_USB_RW        0 // Print USB read/write calls

// dump current syslog contents to SYSLOG_OFFS_SECTORS on every FSA log write.
// (debugging feature, will cause slowdown and hammer on the SD card)
#define DUMP_LOG 0

// sanity-check some config things
#if MLC_ACCELERATE & !USE_SYS_SLCCMPT
#error "MLC_ACCELERATE is only supported with USE_SYS_SLCCMPT enabled!!"
#endif
#if MLC_ACCELERATE & !USE_REDNAND
#error "MLC_ACCELERATE on sysnand will brick on reboot without bootloader hax!!"
#endif

#endif // _CONFIG_H
