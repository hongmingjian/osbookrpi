CROSS=arm-none-eabi-

SHELL=   /bin/bash
CC=      $(CROSS)gcc
AS=      $(CROSS)as
LD=      $(CROSS)ld
OBJCOPY= $(CROSS)objcopy
AR=      $(CROSS)ar
STRIP=   $(CROSS)strip
SIZE=    $(CROSS)size
STRINGS= $(CROSS)strings
READELF= $(CROSS)readelf
RANLIB=  $(CROSS)ranlib
NM=      $(CROSS)nm
GDB=     $(CROSS)gdb
OBJDUMP= $(CROSS)objdump

PROG=    kernel

CPPFLAGS+=-nostdinc -I../include
ASFLAGS=-Wall -O2 -D__ASSEMBLY__
CFLAGS+=-Wall -O2 -ffreestanding -fleading-underscore
LDFLAGS+=-Tkernel.ld -nostdlib -nostartfiles -nodefaultlibs

COBJS=machdep.o
OBJS=entry.o $(COBJS)

$(PROG).img: $(PROG).elf
	$(OBJCOPY) $^ -O binary $@

$(PROG).elf: kernel.ld $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

kernel.ld: kernel.ld.in
	$(CC) -E -P -x c-header -o $@ $^

clean:
	-$(RM) kernel.ld $(OBJS) $(PROG).img $(PROG).elf
