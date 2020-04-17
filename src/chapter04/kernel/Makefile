#
#变量定义
#
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

#C预处理器的选项
CPPFLAGS+=-nostdinc -Iinclude -I../include

#汇编器的选项
ASFLAGS=-Wall -O2

#C编译器的选项
CFLAGS+=-Wall -O2 -ffreestanding -fleading-underscore

#链接器的选项
LDFLAGS+=-T$(PROG).ld -nostdlib

COBJS=machdep.o kernel.o timer.o printk.o bitmap.o frame.o page.o kmalloc.o
COBJS+=../lib/tlsf/tlsf.o ../lib/snprintf.o ../lib/string.o \
       ../lib/softfloat.o ../lib/memset.o ../lib/memcpy.o

#entry.o必须是第一个目标文件
OBJS=entry.o $(COBJS)

all: $(PROG).uue

$(PROG).img: $(PROG).elf
	$(OBJCOPY) $^ -O binary $@

$(PROG).elf: $(PROG).ld $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(PROG).ld: $(PROG).ld.in
	$(CC) -E -P -x c-header -o $@ $^

$(PROG).uue: $(PROG).img
	uuencode $^ $^ >$@

clean:
	-$(RM) $(OBJS) $(PROG).{ld,img,elf,map,uue}
