# stroopwafel

[iosuhax](https://github.com/smealum/iosuhax) successor (based on SALT iosuhax)

Notable additional features (see also: `ios_process/source/config.h`):
  - de_Fuse support: Redirects all OTP reads to RAM. Requires minute_minute to patch in the data.
  - Decrypted fw.img loading in IOSU.
  - IOSU reloads passes through [minute](https://github.com/shinyquagsire23/minute_minute) to make patching easier
  - SEEPROM writes are disabled (for safety)
  - redNAND MLC acceleration -- moves MLC cache (SCFM) to SLCCMPT
  - Semihosting hooks -- print kprintf and syslogs to [pico de_Fuse](https://github.com/shinyquagsire23/wii_u_modchip/tree/main/pico_defuse) serial
  - Disk drive disable (w/o SEEPROM writing)
  - `USB_SHRINKSHIFT` -- Allows having both the Wii U filesystem and a normal filesystem on a drive by shifting the Wii U portion after the MBR
  - `USB_SEED_SWAP` -- Override the SEEPROM USB key to allow easier system migration

Experimental/Unstable features:
  - Loading `kernel.img` from sdcard

Warnings:
  - redNAND formatting is incompatible with other implementations! use minute to format.
  - If you have existing USB drives, keep `USB_SHRINKSHIFT` set to 0
  - `PRINT_FSAOPEN` is useful, but extremely slow
  - This is only tested on 5.5.x fw.imgs, I haven't ported to anything before that.

## plugins
TODO

## technical details

stroopwafel operates similar to (and is based on) [SaltyNX](https://github.com/shinyquagsire23/SaltyNX): Instead of jumping to the reset handler at 0xFFFF0000, code execution is redirected to `wafel_core`. `wafel_core` patches the IOS kernel, modules, and segments, and then jumps to 0xFFFF0000. The memory for stroopwafel plugins is taken from the end of the 128MiB ramdisk at 0x20000000. Specifics on bootstrapping can be found [here](https://github.com/shinyquagsire23/minute_minute/blob/master/source/ancast.c#L624).

Plugins are mapped to all IOS modules, to allow for code patching and function replacement. Additional MMU mappings are also added to allow for single-instruction B/BL trampolines. All plugins are PIE and relocate themselves once on startup. Additionally, plugins may resolve symbols from other plugins. `wafel_core` provides several base APIs for patching and symbol lookup.

`wafel_core` currently provides all of the previously mentioned features (redNAND, semihosting, wupserver, etc).

## credits/attribution
 - smealum for the original iosuhax
 - WulfyStylez and Dazzozo for many of the SALT iosuhax patches
