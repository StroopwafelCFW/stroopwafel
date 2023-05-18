#!/bin/zsh

make -C wafel_core clean
make -C wafel_plugin_example clean
make wafel_core.ipx
make -C wafel_plugin_example
cp wafel_plugin_example/wafel_plugin_example.elf wafel_plugin_example.ipx

disk=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount /Users/maxamillion/workspace/Wii-U-Firmware-Emulator/files/sdcard.bin | head -n1 | cut -d " " -f1)
disk_part=$disk
disk_part+="s1"

mount -t msdos $disk_part fat_mnt
mkdir -p fat_mnt/wiiu/ios_plugins/
cp wafel_core.ipx fat_mnt/wiiu/ios_plugins/
cp otp.bin fat_mnt/
rm -f fat_mnt/ios.img
rm -f fat_mnt/ios.patch
umount fat_mnt
hdiutil detach $disk


#disk="invalid"
#for d in /dev/disk*; do
#    val=$(diskutil info $d | grep -c 'Built In SDXC Reader')
#    if [ "$val" -eq "1" ]; then
#        disk=$d
#        diskutil info $disk
#        break
#        #echo $disk
#    fi
#done
#
#if [[ "$disk" == "invalid" ]]; then
#    echo "No disk found to flash."
#    exit -1
#fi
#
#disk_partition=$disk
#disk_partition+="s1"
#
#echo "Copying to: $disk_partition"
#diskutil mount $disk_partition
##sudo dd if=boot1.img of="$disk" status=progress
#cp wafel_core.ipx /Volumes/UNTITLED/wiiu/ios_plugins/
#diskutil eject $disk