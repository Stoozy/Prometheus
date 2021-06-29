IMAGE_ID=fcd425305881
CWD=C:\Users\mahin\Downloads\Dead_OS


container:
	docker container create --name cc --entrypoint bash $(IMAGE_ID)

build:
	docker run -v "$(CWD)":"/src/Dead-OS/" --name i386-elf-gcc -it cc
	docker cp i386-elf-gcc:/src/Dead-OS/dead_os.iso $(CWD)
	docker container stop i386-elf-gcc
	docker container rm i386-elf-gcc

run-debug:
	qemu-system-i386  -vga std -cpu qemu32,+pae -no-reboot -monitor stdio -d int -no-shutdown -m 4G  -cdrom dead_os.iso  -drive id=disk,file=dead.img\

run:
	qemu-system-i386  -vga std -cpu qemu32,+pae -no-reboot -no-shutdown -m 4G  -cdrom dead_os.iso  -drive id=disk,file=dead.img\

