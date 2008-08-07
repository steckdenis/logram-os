export TOPDIR = $(CURDIR)
export DESTDIR = $(CURDIR)/disk
export CC = gcc
export LD = ld
export AS = nasm
export CFLAGS = -c -x c -g -O0 -fshort-wchar -I $(TOPDIR)/include/ -nostartfiles -fno-stack-protector -fno-builtin -nostdlib -nostdinc -m64
export LDFLAGS = -N -e main -melf_x86_64
export ASMFLAGS = -felf64

$(VERBOSE).SILENT:

all: 
	@(cd extensions && $(MAKE)) 
	@(cd boot && $(MAKE)) 
	@(cd drivers && $(MAKE)) 

clean:
	@(cd extensions && $(MAKE) clean) 
	@(cd boot && $(MAKE) clean) 
	@(cd drivers && $(MAKE) clean) 

disk: all
	./diskmaker/diskmaker

makedsk:
	@(cd diskmaker && $(MAKE)) 

bl:
	@(cd bootloader && $(MAKE))

cleanbl:
	@(cd bootloader && $(MAKE) clean)

qemu:
	qemu-system-x86_64 -boot c -s -hda disk.img -m 64 -d in_asm
