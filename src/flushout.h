#pragma once
#ifndef FLUSHOUT_H
#define FLUSHOUT_H

#include "dos_tman.h"

#if defined(__WATCOMC__)
#include <stdio.h> // snprintf is here!
#define flushout(stream)
#elif defined(__DJGPP__)
#define flushout(stream) \
    {\
        DosTaskman::suspend();\
        std::fflush(stream);\
        DosTaskman::resume();\
    }
#else
#define flushout(stream) std::fflush(stream)
#endif

#endif // FLUSHOUT_H
