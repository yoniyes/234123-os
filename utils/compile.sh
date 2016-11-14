#!/bin/bash

cd /usr/src/linux-2.4.18-14custom
make bzImage
echo
echo "Press any key to continue..."
read dummy
make modules
make modules_install
cd arch/i386/boot
cp bzImage /boot/vmlinuz-2.4.18-14custom
cd /boot
mkinitrd 2.4.18-14custom.img 2.4.18-14custom
reboot
