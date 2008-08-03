OBJS = starts.o kernels.o kernel.o console.o interrupts.o interruptss.o mem.o boot.o

EXT = kernel

all: $(OBJS)
	$(LD) $(LDFLAGS) -Tdata 0x800000 -Ttext 0x801000 -o $(EXT).b $(OBJS)
	objcopy -R .note -R .comment -R .eh_frame -S -O binary $(EXT).b $(EXT).ext
	cp $(EXT).ext $(DESTDIR)/Logram/sys64/$(EXT).ext

%.o: %.c
	$(CC) $(CFLAGS) -o $*.o $*.c

%.o: %.s
	$(AS) $(ASMFLAGS) -o $*.o $*.s

clean:
	rm -f *.b 
	rm -f *.o
	rm -f *.ext
