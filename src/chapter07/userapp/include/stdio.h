#ifndef _STDIO_H
#define _STDIO_H

#include <sys/types.h>
#include <stdarg.h>

int snprintf (char *str, size_t count, const char *fmt, ...);
int vsnprintf (char *str, size_t count, const char *fmt, va_list arg);

int printf(const char *fmt,...);

#endif /* _STDIO_H */
