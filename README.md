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

## building

```
# Needed for plugins
export STROOPWAFEL_ROOT=$(pwd)

# wafel_core
make wafel_core.ipx

# plugin example
make -C wafel_plugin_example
```

## plugins

Additional IOSU patches can be added trough plugins. A compiled plugin can be placed next to the wafel_core.ipx and it will be loaded by minute. Stroopwafel then calls two hooks in the plugin:

- `void kern_main()` - runs before everything else in kernel mode.

- `void mcp_main()` - runs before MCP's main thread under MCP.

Both functions must return, but `mcp_main()` can spawn new threads.

A skeleton can be found here: [wafel_plugin_example](wafel_plugin_example)

### patching IOSU

A plugin can apply through several macros provided by stroopwafel in [wafel/patch.h](wafel_core/include/wafel/patch.h):

- `U32_PATCH_K(_addr, _val)` overwrites the virtual address the provided 32bit value

- `ASM_PATCH_K(_addr, _str)` Takes a string containing assembly, assembles it at compile time and copies the binary at runtime at the given address. You can't do relative reference symbols outside the assembly string.

- `ASM_T_PATCH_K(_addr, _str)` same as `ASM_PATCH_K(_addr, _str)` but for thumb

- `BL_TRAMPOLINE_K(_addr, _dst)` generates a relative `BL` at the address to dst. You can't use that to jump to code in your plugin - the branch would be too far.

- `BL_T_TRAMPOLINE_K(_addr, _dst)` same as `BL_TRAMPOLINE_K(_addr, _dst)` but for thumb

- `BRANCH_PATCH_K(_addr, _dst)` generates a relative `B` at the address to dst. You can't use that to jump to code in your plugin - the branch would be too far.

### hooking IOSU using trampolines

Stroopwafel provides two ways to hook into IOSU to call your C or asm code. The functions and structs are declared in [wafel/trampoline.h](wafel_core/include/wafel/trampoline.h)

- `void trampoline_hook_before(uintptr_t addr, void *target)` 
  Creates a trampoline and a `BL` at address to call it. The trampoline will push all GP registers, call your function supplied in `target`, after your function is finished it will restore the registers and execute the orginal instruction that was overwritten.
  Since a `BL` is used to branch to the trampoline, the original `LR` will be overwritten. The orignal instruction is not allowed to use `PC` relative addressing, as that would be off. The only exception to this is a `BL`, in that case it will branch to the original `BL` target instead.
  The called function receives a pointer to a `trampoline_state` struct, which allows it to read and modify the register state that will be restored after the function finishes.
- `void trampoline_t_hook_before(uintptr_t addr, void *target)` 
  Same as the previous, but for thumb. The only difference is that two instructions will be overwritten, since thumb instructions are 2 bytes, but the `BL` will take 4. Both instructions will be relocated to the trampoline. The called function receives a pointer to a `trampoline_t_state`
- `void trampoline_blreplace(uintptr_t addr, void *target)`
  Replaces a `BL` function call. The new target function in `target` receives the first four original arguments (in r0-r3) in their original position, the fith argument is a pointer to the original `BL` target, the sixth is the saved `LR` (artifact of how the trampoline works) and after that the other original arguments (on the stack) follow.
  You can descide if, where and whith what argument's you call the original target function using the supplied pointer.
- `void trampoline_t_blreplace(uintptr_t addr, void *target)`
  Same as before, but for thumb
- `void trampoline_blreplace_with_regs(uintptr_t addr, void *target)`
  Same as `trampoline_blreplace` but also pushes `r4-r12` on the stack, so they are available as arguments. The signature of the target function looks like this:  `int ums_sync_hook(int r0, int r1, int r2, r3, int r4, int r5, int r6, int r7, int r8, int r9, int r10, int r11, int r12, int *(org_func)(int,..), const void *lr, int stack_arg1, ...)`
- `void trampoline_t_blreplace_with_regs(uintptr_t addr, void *target)`
  Same as `trampoline_t_blreplace` but also pushes `r4-r7` on the stack, so they are available as arguments. The signature of the target function looks like this:  `int ums_sync_hook(int r0, int r1, int r2, r3, int r4, int r5, int r6, int r7, int *(org_func)(int,..), const void *lr, int stack_arg1, ...)`

These trampolines support hooking the same address multiple times. With the `blreplace` the replacement function is responsible to call the previous function via the supplied pointer to not break the chain.

A demo of the trampolines can be found here: [wafel_trampoline_demo/source/main.c at main · jan-hofmeier/wafel_trampoline_demo · GitHub](https://github.com/jan-hofmeier/wafel_trampoline_demo/blob/main/source/main.c)

## technical details

stroopwafel operates similar to (and is based on) [SaltyNX](https://github.com/shinyquagsire23/SaltyNX): Instead of jumping to the reset handler at 0xFFFF0000, code execution is redirected to `wafel_core`. `wafel_core` patches the IOS kernel, modules, and segments, and then jumps to 0xFFFF0000. The memory for stroopwafel plugins is taken from the end of the 128MiB ramdisk at 0x20000000. Specifics on bootstrapping can be found [here](https://github.com/shinyquagsire23/minute_minute/blob/master/source/ancast.c#L624).

Plugins are mapped to all IOS modules, to allow for code patching and function replacement. Additional MMU mappings are also added to allow for single-instruction B/BL trampolines. All plugins are PIE and relocate themselves once on startup. Additionally, plugins may resolve symbols from other plugins. `wafel_core` provides several base APIs for patching and symbol lookup.

`wafel_core` currently provides all of the previously mentioned features (redNAND, semihosting, wupserver, etc).

## credits/attribution

- smealum for the original iosuhax
- WulfyStylez and Dazzozo for many of the SALT iosuhax patches
