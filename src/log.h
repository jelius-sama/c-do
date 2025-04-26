#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

static inline void flush_stdout_if_needed() {
    if (!isatty(fileno(stdout))) {
        fflush(stdout);
    }
}

static inline void errorf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("ERROR: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    flush_stdout_if_needed();
}

static inline void panicf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("PANIC: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    flush_stdout_if_needed();
    exit(1);
}

static inline void warnf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("WARN: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    flush_stdout_if_needed();
}

static inline void infof(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("INFO: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    flush_stdout_if_needed();
}

static inline void todof(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("TODO: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    flush_stdout_if_needed();
}

#ifdef DEBUG
static inline void debugf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("DEBUG: ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
    fflush(stdout);
}
#else
// Compiles away to nothing in release builds
#define debugf(...) ((void)0)
#endif

#endif // LOG_H
