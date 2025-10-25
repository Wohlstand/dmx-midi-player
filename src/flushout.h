#ifndef FLUSHOUT_H
#define FLUSHOUT_H

#if defined(__WATCOMC__)
#include <stdio.h> // snprintf is here!
#define flushout(stream)
#elif defined(__DJGPP__)
extern volatile bool g_flushing;
#ifdef FLUSHOUT_IMPL
volatile bool g_flushing = false;
#endif
#define flushout(stream) \
    {\
        g_flushing = true;\
        std::fflush(stream);\
        g_flushing = false;\
    }
#else
#define flushout(stream) std::fflush(stream)
#endif

#endif // FLUSHOUT_H
