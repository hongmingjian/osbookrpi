CROSS=arm-none-eabi-

SHELL=		/bin/bash
CC=		$(CROSS)gcc
AS=		$(CROSS)as
LD=		$(CROSS)ld
OBJCOPY=	$(CROSS)objcopy
AR=		$(CROSS)ar
STRIP=		$(CROSS)strip
SIZE=		$(CROSS)size
STRINGS=	$(CROSS)strings
READELF=	$(CROSS)readelf
RANLIB=		$(CROSS)ranlib
NM=		$(CROSS)nm
GDB=		$(CROSS)gdb
OBJDUMP= $(CROSS)objdump

PROG=		kernel

CPPFLAGS+=-nostdinc -I../include
ASFLAGS=-Wall -O2 -D__ASSEMBLY__
CFLAGS+=-Wall -O2 -ffreestanding  -fleading-underscore
LDFLAGS+=-Tldscript -nostdlib -nostartfiles -nodefaultlibs

COBJS=machdep.o printk.o kernel.o ../lib/snprintf.o ../lib/string.o ../lib/softfloat.o
OBJS=entry.o $(COBJS)

all: $(PROG).img

$(PROG).img: $(PROG).elf
	$(OBJCOPY) $^ -O binary $@

$(PROG).elf: ldscript $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@  

ldscript: ldscript.in
	$(CC) -E -P -x c-header -o $@ $^
	
.PHONY: clean
clean:
	-$(RM) ldscript $(OBJS) $(PROG).img $(PROG).elf $(PROG).map *.*~
