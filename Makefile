ISO_IMAGE=disk.iso
SYSROOT=$(shell pwd)/sysroot
QEMU_RUN_FLAGS= -smp cores=2 -serial stdio  -vga std -machine q35 -no-reboot -d int  -M smm=off -no-shutdown -m 8G
QEMU_MONITOR_FLAGS =  -smp cores=2 -monitor stdio  -vga std -machine q35 -no-reboot -d int -M smm=off -no-shutdown -m 8G 

.PHONY: clean all run libc

all: $(ISO_IMAGE) 

monitor: $(ISO_IMAGE)
	qemu-system-x86_64  $(QEMU_MONITOR_FLAGS) -cdrom $(ISO_IMAGE)

run: $(ISO_IMAGE)
	qemu-system-x86_64 $(QEMU_RUN_FLAGS) -cdrom $(ISO_IMAGE)

debug: $(ISO_IMAGE)
	#gdb -x "qemu-system-x86_64 -S -gdb stdio  -no-reboot -d int -m 8G -cdrom $(ISO_IMAGE)"
	gdb -x script.gdb

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
	make -C limine

kernel/kernel.elf:
	$(MAKE) -C kernel

libc:
	$(MAKE) -C libc 

	#cd libc  && make && make install
	#cd mlibc && mkdir build && meson . build --cross-file crossfile.ini && ninja -C build && yes | cp build/*.so $(SYSROOT)/lib && yes | cp build/sysdeps/atlas/crt1.o $(SYSROOT)/lib

initrd: 
	tar -C $(SYSROOT) -cvf initrd.tar lib fonts hello


$(ISO_IMAGE): limine kernel/kernel.elf initrd
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/kernel.elf initrd.tar \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	limine/limine-install $(ISO_IMAGE)
	rm -rf iso_root

clean:
	rm -f $(ISO_IMAGE) 
	rm -rf initrd.tar
	$(MAKE) -C libc clean
	$(MAKE) -C kernel clean
	rm -rf mlibc/build

