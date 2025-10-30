#pragma once
#ifndef FLUSHOUT_H
#define FLUSHOUT_H

#include <cstdio>

#if defined(__WATCOMC__)
#include <stdio.h> // snprintf is here!
// Use normal fprintf, but don't call fflush since this ruins the runtime
#   define s_fprintf        std::fprintf
#   define flushout(stream)
#   elif defined(__DJGPP__)
// Use printf and flush with interrupt triggering suspension to avoid execution of interrupt just in a middle of the fprintf and fflush execution
#   include <cstdarg>
#   include "dos_tman.h"

static int s_fprintf(FILE *stream, const char *format, ...)
{
    int ret;
    va_list args;
    DosTaskman::suspend();
    va_start(args, format);
    if(DosTaskman::isInsideInterrupt())
        ret = DosTaskman::reserve_fprintf(stream, format, args);
    else
        ret = vfprintf(stream, format, args);
    va_end(args);
    DosTaskman::resume();
    return ret;
}

#define flushout(stream) \
    {\
        DosTaskman::suspend();\
        DosTaskman::reserve_flush(stream);\
        std::fflush(stream);\
        DosTaskman::resume();\
    }
#else
// Use normal fprintf and fflush
#   define s_fprintf        std::fprintf
#   define flushout(stream) std::fflush(stream)
#endif

#endif // FLUSHOUT_H
