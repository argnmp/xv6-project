# make clean TOOLPREFIX="x86_64-elf-"
make TOOLPREFIX="x86_64-elf-"
make fs.img TOOLPREFIX="x86_64-elf-"
# qemu-system-i386 -nographic -serial mon:stdio -hdb fs.img xv6.img -smp 1 -m 512
qemu-system-i386 -nographic -serial mon:stdio -hdb fs.img xv6.img -m 512
