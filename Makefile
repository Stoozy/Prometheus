ISO_IMAGE = disk.iso
SYSROOT=sysroot

.PHONY: clean all run

all: $(ISO_IMAGE)

monitor: $(ISO_IMAGE)
	qemu-system-x86_64  -monitor stdio -vga std -machine q35 -no-reboot -d int -M smm=off -no-shutdown -m 8G -cdrom $(ISO_IMAGE)

run: $(ISO_IMAGE)
	qemu-system-x86_64  -serial stdio -vga std -machine q35 -no-reboot -d int -M smm=off -no-shutdown -m 8G -cdrom $(ISO_IMAGE)

debug: $(ISO_IMAGE)
	qemu-system-x86_64 -s -S -no-reboot -d int -m 8G -cdrom $(ISO_IMAGE)

limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=v2.0-branch-binary --depth=1
	make -C limine

kernel/src/dead_kernel.elf:
	$(MAKE) -C kernel/src

libc:
	mkdir $(SYSROOT)
	#$(MAKE) -C build-newlib/
	#$(MAKE) -C build-newlib/ DESTDIR=../$(SYSROOT) install


$(ISO_IMAGE): limine kernel/src/dead_kernel.elf
	rm -rf iso_root
	mkdir -p iso_root
	cp kernel/src/dead_kernel.elf \
		limine.cfg limine/limine.sys limine/limine-cd.bin limine/limine-eltorito-efi.bin modules/unscii-16.sfn modules/test-elf/a.out modules/tmpfs.tar iso_root/
	xorriso -as mkisofs -b limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-eltorito-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMAGE)
	limine/limine-install $(ISO_IMAGE)
	rm -rf iso_root

clean:
	rm -f $(ISO_IMAGE) 
	rm -rf $(SYSROOT)
	$(MAKE) -C kernel/src clean
	#$(MAKE) -C build-newlib/ clean
