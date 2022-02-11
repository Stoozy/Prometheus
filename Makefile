ISO_IMAGE=disk.iso
SYSROOT=$(shell pwd)/sysroot

.PHONY: clean all run

all: $(ISO_IMAGE) 

monitor: $(ISO_IMAGE)
	qemu-system-x86_64  -smp cores=2 -monitor stdio -vga std -machine q35 -no-reboot -d int -M smm=off -no-shutdown -m 8G -cdrom $(ISO_IMAGE)

run: $(ISO_IMAGE)
	qemu-system-x86_64  -smp cores=2 -serial stdio -vga std -machine q35 -no-reboot -d int -M smm=off -no-shutdown -m 8G -cdrom $(ISO_IMAGE)

debug: $(ISO_IMAGE)
	qemu-system-x86_64 -s -S -no-reboot -d int -m 8G -cdrom $(ISO_IMAGE)

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
	make -C limine

kernel/kernel.elf:
	$(MAKE) -C kernel

libc:
	cd build-newlib && make && make DESTDIR=$(SYSROOT) install

initrd: 
	tar -C $(SYSROOT) -cvf initrd.tar fonts testfile


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
	$(MAKE) -C kernel clean
