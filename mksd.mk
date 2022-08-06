SD_IMG ?=
KERN_IMG ?=

BOOT_IMG := $(BUILD_DIR)/boot.img
FS_IMG := $(BUILD_DIR)/fs.img
EXT2_IMG := $(BUILD_DIR)/ext2.img

MKFS := /usr/local/opt/e2fsprogs/sbin/mkfs.ext2

SECTOR_SIZE := 512

# The total sd card image is 1024 MB, 64 MB for boot sector and 480 MB for file system (480 MB).
# sectors = 1GB = 512 * 2 * 1024 * 1024
SECTORS := 2048*1024
# boot offset = 1MB = 512 * 2 * 1024
BOOT_OFFSET := 2048
# boot sectors = 64MB = 512 * 128 * 1024
BOOT_SECTORS= 128*1024
# FS1 (v6) = 480 MB = 512 * 2 * 480 * 1024
FS1_OFFSET := $(shell echo $$(($(BOOT_OFFSET) + $(BOOT_SECTORS))))
FS1_SECTORS = 960*1024
# FS2 (ext2)
FS2_OFFSET := $(shell echo $$(($(FS1_OFFSET) + $(FS1_SECTORS))))
FS2_SECTORS := $(shell echo $$(($(SECTORS) - $(FS1_OFFSET) - $(FS1_SECTORS))))

.DELETE_ON_ERROR: $(BOOT_IMG) $(SD_IMG)

# TODO: Detect img size automatically
$(BOOT_IMG): $(KERN_IMG) $(shell find boot/*)
	dd if=/dev/zero of=$@ seek=$$(($(BOOT_SECTORS) - 1)) bs=$(SECTOR_SIZE) count=1
	# -F specify FAT32
	# -c 1 specify one sector per cluster so that we can create a smaller one
	mformat -F -c 1 -i $@ ::
	# Copy files into boot partition
	$(foreach x, $^, mcopy -i $@ $(x) ::$(notdir $(x));)

$(FS_IMG): $(shell find obj/usr/bin -type f) $(shell find obj/dyn/bin -type f)
	echo $^
	cc $(shell find usr/src/mkfs/ -name "*.c") -o obj/mkfs -Iusr/inc
	./obj/mkfs $@ $^

$(EXT2_IMG):
	dd if=/dev/zero of=$@ bs=30*1024*1024 count=1
	$(MKFS) -b 4096 -T "EXT2_TEST" $@

$(SD_IMG): $(BOOT_IMG) $(FS_IMG) $(EXT2_IMG)
	dd if=/dev/zero of=$@ seek=$$(($(SECTORS) - 1)) bs=$(SECTOR_SIZE) count=1
	printf "                                                                \
	  $(BOOT_OFFSET), $$(($(BOOT_SECTORS)*$(SECTOR_SIZE) / 1024))K, c,\n      \
	  $(FS_OFFSET), $$(($(FS1_SECTORS)*$(SECTOR_SIZE) / 1024))K, L,\n          \
	  $(FS2_OFFSET), $$(($(FS2_SECTORS)*$(SECTOR_SIZE) / 1024))K, L,\n  \
	" | sfdisk $@
	dd if=$(BOOT_IMG) of=$@ seek=$(BOOT_OFFSET) conv=notrunc
	dd if=$(FS_IMG) of=$@ seek=$(FS1_OFFSET) conv=notrunc
	dd if=$(EXT2_IMG) of=$@ seek=$(FS2_OFFSET) conv=notrunc
