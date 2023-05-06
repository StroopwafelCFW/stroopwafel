#!/bin/zsh

rm ios_process/ios_process.elf
make ios_process/ios_process.elf
make ios.img
make ios.patch

disk=$(hdiutil attach -imagekey diskimage-class=CRawDiskImage -nomount /Users/maxamillion/workspace/Wii-U-Firmware-Emulator/files/sdcard.bin | head -n1 | cut -d " " -f1)
disk_part=$disk
disk_part+="s1"

mount -t msdos $disk_part fat_mnt
cp ios.img fat_mnt/
cp ios.patch fat_mnt/
cp otp.bin fat_mnt/
umount fat_mnt
hdiutil detach $disk


disk="invalid"
for d in /dev/disk*; do
    val=$(diskutil info $d | grep -c 'Built In SDXC Reader')
    if [ "$val" -eq "1" ]; then
        disk=$d
        diskutil info $disk
        break
        #echo $disk
    fi
done

if [[ "$disk" == "invalid" ]]; then
    echo "No disk found to flash."
    exit -1
fi

disk_partition=$disk
disk_partition+="s1"

echo "Copying to: $disk_partition"
diskutil mount $disk_partition
#sudo dd if=boot1.img of="$disk" status=progress
cp ios.img /Volumes/UNTITLED/ios.img
cp ios.patch /Volumes/UNTITLED/ios.patch
diskutil eject $disk