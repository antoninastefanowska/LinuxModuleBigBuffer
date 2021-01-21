MODULE = big_buffer.o
MODFILES = driver.o buffer.o
IOCTL = driver_settings.c
IOCTL.o = driver_settings.o
MMAP = buffer_map.c
MMAP.o = buffer_map.o

MODCFLAGS := -D__KERNEL__ -DMODULE -O2

compile:
	gcc $(MODCFLAGS) -c buffer.c
	gcc $(MODCFLAGS) -c driver.c

assemble:
	ld -m elf_i386 -r -o ${MODULE} ${MODFILES}

ioctl:
	gcc ${IOCTL} -o ${IOCTL.o}

mmap:
	gcc ${MMAP} -o ${MMAP.o}

clean:
	rm -fr ${MODULE} ${MODFILES} ${IOCTL.o} ${MMAP.o}