#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>

int printf(const char *fmt,...)
{
    char buf[256];
    va_list args;
    int i;

    va_start(args, fmt);
    i=vsnprintf(buf,sizeof(buf), fmt, args);
    va_end(args);

    return write(STDOUT_FILENO, buf, i);
}
