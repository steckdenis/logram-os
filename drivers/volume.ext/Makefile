OBJS = volume.o

EXT = volume

all: $(OBJS)
	$(LD) $(LDFLAGS) -Tdata 0x0 -Ttext 0x1000 -o $(EXT).b $(OBJS)
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
