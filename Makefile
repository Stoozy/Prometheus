CONTAINER_ID=15f169c39a75fe662b7f85af12a5ce60eca6a83d25a9c4ac0e82feec2037ef22
IMAGE_ID=fcd425305881
CWD=C:\Users\mahin\Downloads\Dead-OS


container:
	docker container create --name cc --entrypoint bash $(IMAGE_ID)

build:
	docker run --name  i386-elf-gcc -itd cc /bin/bash -c "cd /src/Dead-OS/ && ./build.sh  && ./iso.sh"
	docker cp i386-elf-gcc:/src/Dead-OS/dead_os.iso $(CWD)
	docker container stop i386-elf-gcc
	docker container rm i386-elf-gcc

run:
	qemu-system-i386  -vga std -cpu qemu32,+pae -no-reboot -monitor stdio -d int -no-shutdown -m 4G  -cdrom dead_os.iso  -drive id=disk,file=dead.img\


