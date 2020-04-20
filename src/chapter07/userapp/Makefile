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

PROG=	a

all: $(PROG).uue

#C预处理器的选项
CPPFLAGS=-nostdinc -Iinclude -I../include

#汇编器的选项
ASFLAGS=-Wall -O2

#C编译器的选项
CFLAGS=-Wall -O2 -ffreestanding -fleading-underscore

#链接器的选项
LDFLAGS=-nostdlib -Wl,-e,_start,-Ttext,0x00400000

COBJS=main.o
COBJS+=lib/stdio.o
COBJS+=../lib/softfloat.o ../lib/string.o ../lib/memcpy.o \
	../lib/memset.o ../lib/snprintf.o

OBJS=lib/crt0.o lib/syscall-wrapper.o $(COBJS)

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(PROG).uue: $(PROG).out
	uuencode $^ $^ >$@

clean:
	-$(RM) $(OBJS) $(PROG).{out,map,uue} make.dep *.*~

.PHONY: .FORCE
.FORCE:
../lib/softfloat.o: .FORCE
../lib/string.o: .FORCE
../lib/memcpy.o: .FORCE
../lib/memset.o: .FORCE
../lib/snprintf.o: .FORCE
