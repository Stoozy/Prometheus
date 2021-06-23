
debug:
	qemu-system-x86_64 -no-reboot  -m 4G -no-shutdown -cdrom dead_os.iso -s -S -drive id=disk,file=dead.img,format=raw &
	exec gdb --ex "target remote localhost:1234"  --ex "set architecture i386:x86-64" --ex "file kernel/dead.kernel"

run:
	exec qemu-system-x86_64  -vga std -cpu qemu32,+pae -no-reboot -monitor stdio -d int -no-shutdown -m 4G  -cdrom dead_os.iso  -drive id=disk,file=dead.img\


