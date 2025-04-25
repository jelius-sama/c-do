#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static inline void errorf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("ERROR: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

static inline void panicf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("PANIC: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    exit(1);
}

static inline void warnf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("WARN: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

static inline void infof(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("INFO: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

static inline void todof(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("TODO: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

#endif // LOG_H
