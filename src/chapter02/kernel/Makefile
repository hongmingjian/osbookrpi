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

CPPFLAGS+=-nostdinc -I../include                      #C预处理的选项
ASFLAGS=-Wall -O2 -D__ASSEMBLY__                      #汇编器的选项
CFLAGS+=-Wall -O2 -ffreestanding -fleading-underscore #C编译器的选项
LDFLAGS+=-Tkernel.ld -nostdlib                        #链接器的选项

COBJS=machdep.o
OBJS=entry.o $(COBJS)  #entry.o必须是第一个目标文件

# kernel.img依赖于kernel.img
$(PROG).img: $(PROG).elf
	$(OBJCOPY) $^ -O binary $@  #如何从kernel.elf生成kernel.img
	                            #$^表示所有依赖，$@表示目标

$(PROG).elf: kernel.ld $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

kernel.ld: kernel.ld.in
	$(CC) -E -P -x c-header -o $@ $^

#目标可以没有依赖
clean:
	-$(RM) $(OBJS) $(PROG).{ld,img,elf} #动作前面的-号表示
	                                    #无论该动作是否成功执行,
	                                    #都当做成功了
