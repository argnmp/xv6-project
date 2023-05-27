make clean
make
make fs.img
qemu-system-i386 -nographic -serial mon:stdio -hdb fs.img xv6.img -m 512
