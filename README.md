# de_Fuse iosuhax

Forked from [iosuhax](https://github.com/smealum/iosuhax) by smealum; Includes code from the old SALT mines by WulfyStylez and Dazzozo. Pretty sure this leaked at some point lol.

Notable additional features (see also: `config.s`):
  - de_Fuse support: Redirects all OTP reads to RAM. Requires minute_minute to patch in the data.
  - Decrypted fw.img loading in IOSU.
  - IOSU reloads passes through [minute](https://github.com/shinyquagsire23/minute_minute) to make patching easier
    - salt-patch can generates patches which go over the SLC fw.img
  - Loading `kernel.img` from sdcard
  - SEEPROM writes are disabled (for safety)
  - redNAND MLC acceleration -- moves MLC cache (SCFM) to SLCCMPT
  - Semihosting hook -- print kprintf and syslogs to DEBUG GPIOs
  - Disk drive disable (w/o SEEPROM writing)
  - `USB_SHRINKSHIFT` -- Allows having both the Wii U filesystem and a normal filesystem on a drive by shifting the Wii U portion after the MBR
  - `USB_SEED_SWAP` -- Override the SEEPROM USB key to allow easier system migration

WIP:
  - vWii de_Fuse support

Warnings:
  - redNAND formatting is different! use minute to format.
  - If you have existing USB drives, turn off `USB_SHRINKSHIFT` 
  - `PRINT_FSAOPEN` is useful, but extremely slow
  - This is only tested on 5.5.1 fw.img, I haven't ported to anything after that

---

original iosuhax readme

## iosuhax

this repo contains some of the tools I wrote when working on the wii u. iosuhax is essentially a set of patches for IOSU which provides extra features which can be useful for developers. I'm releasing this because I haven't really touched it since the beginning of january and don't plan on getting back to it.

iosuhax does not contain any exploits or information about vulns o anything. just a custom firmware kind of thing.

iosuhax current only supports fw 5.5.x i think.

i wrote all the code here but iosuhax would not have been possible without the support of plutoo, yellows8, naehrwert and derrek.

## features

iosuhax is pretty barebones, it's mainly just the following :
  - software nand dumping (bunch of ways to do this, dumps slc, slccmpt and mlc, either raw or filetree or something in between)
  - redNAND (redirection of nand read/writes to SD card instead of actual NAND chips)
  - remote shell for development and experimentation (cf wupserver and wupclient, it's super useful)
  - some basic ARM debugging stuff (guru meditation screen)

## todo

I don't plan on doing any of this at this point, but the next things I was going to do for this were :
  - make this version-independent by replacing hardcoded offset with auto-located ones, similar to my 3ds *hax ROP stuff
  - put together a menu for settings and nand-dumping so that there's no need to build with different things enabled (menu would have used power/eject buttons)

## how to use

honestly I don't even remember all the details so anyone who's serious about using this will probably have to ask me for help if they can't figure it out, but the gist of it is :
  - decrypt your ancast image, prepend the raw signature header stuff to it and place it in ./bin/fw.img.full.bin
  - open up ./scripts/anpack.py, add your ancast keys in there
  - make

might be missing some steps, especially for building wupserver. you're going to need devkitarm and latest armips (built from armips git probably, pretty sure this relies on some new features I asked Kingcom to add (blame nintendo for doing big endian arm)).

also, fair warning : do NOT blindly use this. read the patches. running this with the wrong options enabled can/will brick your console. this release is oriented towards devs, not end users.

## license

feel free to use any of this code for your own projects, as long as :
  - you give proper credit to the original creator
  - you don't use any of it for commercial purposes
  - you consider sharing your improvements by making the code available (not required but appreciated)

seriously though if you try to make money off this i will fucking end you

have fun !
