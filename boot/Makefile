all:
	nasm -felf64 -o stage2s.o stage2s.s
	gcc -Wall -c -I ../include -m32 -x c -fshort-wchar stage2.c -fno-stack-protector -nostartfiles -fno-builtin -nostdlib -nostdinc
	objcopy -I elf32-i386 -O elf64-x86-64 stage2.o stage2_64.o
	ld -Ttext 0x0 -Tdata 0x3E00 -Tbss 0x20000 -melf_x86_64 -e main -o stage2.b -s stage2s.o stage2_64.o
	objcopy -R .note -R .comment -R .eh_frame -S -O binary stage2.b stage2
	nasm -o stage1 stage1.s

clean:
	rm *.o
	rm stage1
	rm stage2
	rm *.b
