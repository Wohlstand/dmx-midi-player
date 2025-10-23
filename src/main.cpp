//
// Copyright(C) 2025-2025 Vitaliy Novichkov
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//


#include <math.h>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "utf8main/utf8main.h"
#include "wav/wave_writer.h"

#ifndef HW_DOS_BUILD
#   define SDL_MAIN_HANDLED
#   include <SDL2/SDL.h>
#   include "emu_list.h"
#else
#   include <conio.h>
#   include <dos.h>
#   include <errno.h>
#   include <stdio.h>
#   include <string.h>
#   include <stdint.h>
#   include <pc.h>
#   include <dpmi.h>
#   include <go32.h>
#   include <sys/farptr.h>
#   include <sys/exceptn.h>
#   define BIOStimer _farpeekl(_dos_ds, 0x46C)
typedef uint32_t Uint32;
typedef uint8_t Uint8;
#endif // HW_DOS_BUILD

#if defined(__WATCOMC__)
#   include <stdio.h> // snprintf is here!
#   define flushout(stream)
#else
#   define flushout(stream) std::fflush(stream)
#endif

#include "midi_seq.h"
#include <signal.h>

#ifndef HW_DOS_BUILD
enum { nch = 2 };
enum { buffer_size = 1024 };
#endif

/* variable declarations */
static Uint32 is_playing = 0; /* remaining length of the sample we have to play */

static void sig_playing(int)
{
    is_playing = 0;
}

#ifndef HW_DOS_BUILD
void my_audio_callback(void *midi_player, Uint8 *stream, int len)
{
    MIDI_Seq *mp = (MIDI_Seq*)midi_player;
    size_t got = mp->playBuffer(stream, len);

    if(got == 0)
        is_playing = 0;
}

static void *init_wave_writer(const SDL_AudioSpec &spec, const char *waveOutFile)
{
    void *ctx = NULL;
    int format = WAVE_FORMAT_PCM;
    int sampleSize = 2;
    int hasSign = 1;
    int isBigEndian = 0;

    switch(spec.format)
    {
    case AUDIO_U8:
        sampleSize = 1;
        format = WAVE_FORMAT_PCM;
        hasSign = 0;
        isBigEndian = 0;
        break;
    case AUDIO_S8:
        sampleSize = 1;
        format = WAVE_FORMAT_PCM;
        hasSign = 1;
        isBigEndian = 0;
        break;
    case AUDIO_U16LSB:
        sampleSize = 2;
        format = WAVE_FORMAT_PCM;
        hasSign = 0;
        isBigEndian = 0;
        break;
    case AUDIO_S16LSB:
        sampleSize = 2;
        format = WAVE_FORMAT_PCM;
        hasSign = 1;
        isBigEndian = 0;
        break;
    case AUDIO_U16MSB:
        sampleSize = 2;
        format = WAVE_FORMAT_PCM;
        hasSign = 0;
        isBigEndian = 1;
        break;
    case AUDIO_S16MSB:
        sampleSize = 2;
        format = WAVE_FORMAT_PCM;
        hasSign = 1;
        isBigEndian = 1;
        break;
    case AUDIO_S32MSB:
        sampleSize = 4;
        format = WAVE_FORMAT_PCM;
        hasSign = 1;
        isBigEndian = 1;
        break;
    case AUDIO_S32LSB:
        sampleSize = 4;
        format = WAVE_FORMAT_PCM;
        hasSign = 1;
        isBigEndian = 0;
        break;
    default:
    case AUDIO_F32MSB:
        sampleSize = 4;
        format = WAVE_FORMAT_IEEE_FLOAT;
        hasSign = 1;
        isBigEndian = 1;
        break;
    case AUDIO_F32LSB:
        sampleSize = 4;
        format = WAVE_FORMAT_IEEE_FLOAT;
        hasSign = 1;
        isBigEndian = 0;
        break;
    }

    ctx = ctx_wave_open(spec.channels,
                        spec.freq,
                        sampleSize,
                        format,
                        hasSign,
                        isBigEndian,
                        waveOutFile);

    return ctx;
}
#endif

static inline void secondsToHMSM(double seconds_full, char *hmsm_buffer, size_t hmsm_buffer_size)
{
    double seconds_integral;
    double seconds_fractional = std::modf(seconds_full, &seconds_integral);
    unsigned int milliseconds = static_cast<unsigned int>(seconds_fractional * 1000.0);
    unsigned int seconds = static_cast<unsigned int>(std::fmod(seconds_full, 60.0));
    unsigned int minutes = static_cast<unsigned int>(std::fmod(seconds_full / 60, 60.0));
    unsigned int hours   = static_cast<unsigned int>(seconds_full / 3600);
    std::memset(hmsm_buffer, 0, hmsm_buffer_size);
    if (hours > 0)
        snprintf(hmsm_buffer, hmsm_buffer_size, "%02u:%02u:%02u,%03u", hours, minutes, seconds, milliseconds);
    else
        snprintf(hmsm_buffer, hmsm_buffer_size, "%02u:%02u,%03u", minutes, seconds, milliseconds);
}

static struct TimeCounter
{
    char posHMS[25];
    char totalHMS[25];
    char loopStartHMS[25];
    char loopEndHMS[25];
#ifdef HAS_S_GETTIME
    char realHMS[25];
#endif

    bool hasLoop;
    uint64_t milliseconds_prev;
    int printsCounter;
    int printsCounterPeriod;
    int complete_prev;
    double totalTime;

#ifdef HAS_S_GETTIME
    double realTimeStart;
#endif

#ifdef HW_DOS_BUILD
    volatile unsigned long newTimerFreq;
    volatile unsigned long timerPeriod;
    int haveYield;
    int haveDosIdle;
    volatile unsigned int ring;
    volatile unsigned long BIOStimer_begin;

    volatile unsigned long timerNext;

    enum wmethod
    {
        WM_NONE,
        WM_YIELD,
        WM_IDLE,
        WM_HLT
    } idleMethod;

#endif

    TimeCounter()
    {
        hasLoop = false;
        totalTime = 0.0;
        milliseconds_prev = ~0u;
        printsCounter = 0;
        complete_prev = -1;

#ifndef HW_DOS_BUILD
        printsCounterPeriod = 1;
#else
        printsCounterPeriod = 20;
        setDosTimerHZ(209);
        haveYield = 0;
        haveDosIdle = 0;
        ring = 0;
        idleMethod = WM_NONE;

        timerNext = 0;
#endif
    }

#ifdef HW_DOS_BUILD
    void initDosTimer()
    {
#   ifdef __DJGPP__
        /* determine protection ring */
        __asm__ ("mov %%cs, %0\n\t"
                 "and $3, %0" : "=r" (ring));

        errno = 0;
        __dpmi_yield();
        haveYield = errno ? 0 : 1;

        if(!haveYield)
        {
            __dpmi_regs regs;
            regs.x.ax = 0x1680;
            __dpmi_int(0x28, &regs);
            haveDosIdle = regs.h.al ? 0 : 1;

            if(haveDosIdle)
                idleMethod = WM_IDLE;
            else if(ring == 0)
                idleMethod = WM_HLT;
            else
                idleMethod = WM_NONE;
        }
        else
        {
            idleMethod = WM_YIELD;
        }

        const char *method;
        switch(idleMethod)
        {
        default:
        case WM_NONE:
            method = "none";
            break;
        case WM_YIELD:
            method = "yield";
            break;
        case WM_IDLE:
            method = "idle";
            break;
        case WM_HLT:
            method = "hlt";
            break;
        }

        std::fprintf(stdout, " - [DOS] Using idle method: %s\n", method);
#   endif
    }

    void setDosTimerHZ(unsigned timer)
    {
        newTimerFreq = timer;
        timerPeriod = 0x1234DDul / newTimerFreq;
    }

    void flushDosTimer()
    {
#   ifdef __DJGPP__
        outportb(0x43, 0x34);
        outportb(0x40, timerPeriod & 0xFF);
        outportb(0x40, timerPeriod >>   8);
#   endif

#   ifdef __WATCOMC__
        outp(0x43, 0x34);
        outp(0x40, TimerPeriod & 0xFF);
        outp(0x40, TimerPeriod >>   8);
#   endif

        BIOStimer_begin = BIOStimer;

        std::fprintf(stdout, " - [DOS] Running clock with %ld hz\n", newTimerFreq);
    }

    void restoreDosTimer()
    {
#   ifdef __DJGPP__
        // Fix the skewed clock and reset BIOS tick rate
        _farpokel(_dos_ds, 0x46C, BIOStimer_begin + (BIOStimer - BIOStimer_begin) * (0x1234DD / 65536.0) / newTimerFreq);

        //disable();
        outportb(0x43, 0x34);
        outportb(0x40, 0);
        outportb(0x40, 0);
        //enable();
#   endif

#   ifdef __WATCOMC__
        outp(0x43, 0x34);
        outp(0x40, 0);
        outp(0x40, 0);
#   endif
    }

    void waitDosTimer()
    {
//__asm__ volatile("sti\nhlt");
//usleep(10000);
#       ifdef __DJGPP__
        switch(idleMethod)
        {
        default:
        case WM_NONE:
            if(timerNext != 0)
            {
                while(BIOStimer < timerNext)
                    delay(1);
            }

            timerNext = BIOStimer + 1;
            break;

        case WM_YIELD:
            __dpmi_yield();
            break;

        case WM_IDLE:
        {
            __dpmi_regs regs;

            /* the DOS Idle call is documented to return immediately if no other
             * program is ready to run, therefore do one HLT if we can */
            if(ring == 0)
                __asm__ volatile ("hlt");

            regs.x.ax = 0x1680;
            __dpmi_int(0x28, &regs);
            if (regs.h.al)
                errno = ENOSYS;
            break;
        }

        case WM_HLT:
            __asm__ volatile("hlt");
            break;
        }
#       endif
#       ifdef __WATCOMC__
        //dpmi_dos_yield();
        mch_delay((unsigned int)(tick_delay * 1000.0));
#       endif
    }
#endif

    void setTotal(double total)
    {
        totalTime = total;
        secondsToHMSM(total, totalHMS, 25);
#ifdef HAS_S_GETTIME
        realTimeStart = s_getTime();
        secondsToHMSM(s_getTime() - realTimeStart, realHMS, 25);
#endif
    }

    void setLoop(double loopStart, double loopEnd)
    {
        hasLoop = false;

        if(loopStart >= 0.0 && loopEnd >= 0.0)
        {
            secondsToHMSM(loopStart, loopStartHMS, 25);
            secondsToHMSM(loopEnd, loopEndHMS, 25);
            hasLoop = true;
        }
    }

    void clearLineR()
    {
        std::fprintf(stdout, "                                               \r");
        flushout(stdout);
    }

    void printTime(double pos)
    {
        uint64_t milliseconds = static_cast<uint64_t>(pos * 1000.0);

        if(milliseconds != milliseconds_prev)
        {
            if(printsCounter >= printsCounterPeriod)
            {
                printsCounter = -1;
                secondsToHMSM(pos, posHMS, 25);
#ifdef HAS_S_GETTIME
                secondsToHMSM(s_getTime() - realTimeStart, realHMS, 25);
#endif
                std::fprintf(stdout, "                                               \r");
#ifdef HAS_S_GETTIME
                std::fprintf(stdout, "Time position: %s / %s [Real time: %s]\r", posHMS, totalHMS, realHMS);
#else
                std::fprintf(stdout, "Time position: %s / %s\r", posHMS, totalHMS);
#endif
                flushout(stdout);
                milliseconds_prev = milliseconds;
            }
            printsCounter++;
        }
    }

    void printProgress(double pos)
    {
        int complete = static_cast<int>(std::floor(100.0 * pos / totalTime));

        if(complete_prev != complete)
        {
            std::fprintf(stdout, "                                               \r");
            std::fprintf(stdout, "Recording WAV... [%d%% completed]\r", complete);
            flushout(stdout);
            complete_prev = complete;
        }
    }

    void clearLine()
    {
        std::fprintf(stdout, "                                               \n\n");
        flushout(stdout);
    }

} s_timeCounter;

#ifdef HW_DOS_BUILD
static void runDOSLoop(MIDI_Seq *myDevice)
{
    double tick_delay = 0.0;

    s_timeCounter.clearLineR();

    while(is_playing)
    {
        const double mindelay = 1.0 / s_timeCounter.newTimerFreq;

#   ifndef DEBUG_TRACE_ALL_EVENTS
        s_timeCounter.printTime(myDevice->tell());
#   endif

        s_timeCounter.waitDosTimer();

        static unsigned long PrevTimer = BIOStimer;
        const unsigned long CurTimer = BIOStimer;
        const double eat_delay = (CurTimer - PrevTimer) / (double)s_timeCounter.newTimerFreq;
        PrevTimer = CurTimer;

        tick_delay = myDevice->tick(eat_delay, mindelay);

        if(myDevice->atEnd() && tick_delay <= 0)
            is_playing = false;

        if(kbhit())
        {   // Quit on ESC key!
            int c = getch();
            if(c == 27)
                is_playing = false;
        }
    }

    s_timeCounter.clearLine();

    myDevice->panic(); //Shut up all sustaining notes
}
#else

static int runWaveOutLoopLoop(MIDI_Seq &player, const char *musPath, const char *wavPath, const struct SDL_AudioSpec &spec)
{
    const size_t buffer_size = 1024 * sizeof(float) * 2;
    uint8_t buffer[buffer_size];
    size_t got = 0;
    void *wave = NULL;
    std::fprintf(stdout, " - Recording %s to WAV file %s...\n", musPath, wavPath);
    std::fprintf(stdout, "\n==========================================\n");
    flushout(stdout);

    wave = init_wave_writer(spec, wavPath);
    if(!wave)
    {
        fprintf(stderr, "ERROR: Couldn't open wave writer for output %s\n", wavPath);
        fflush(stdout);
        return 1;
    }

    while(is_playing)
    {
        got = player.playBuffer(buffer, buffer_size);
        if(got == 0)
            break;

        ctx_wave_write(wave, buffer, got);
        s_timeCounter.printProgress(player.tell());
    }

    ctx_wave_close(wave);

    return 0;
}
#endif

struct Args
{
    const char *song = nullptr;
    const char *setup = nullptr;
    char setupString[50] = "";
    const char *bank = "genmidi.op2";
#ifndef HW_DOS_BUILD
    int emu_type = EMU_NUKED_OPL3;
    float gain = 2.0f;
    bool wave = false;
    const char *waveFile = nullptr;
    char wavePath[2048] = "";
#endif

    bool loop = false;

#ifdef HW_DOS_BUILD
    uint16_t hw_addr = 0x388;
#endif

    bool printArgFail(const char *arg)
    {
        printf("ERROR: Argument %s requires an option!", arg);
        fflush(stdout);
        return false;
    }

    bool printArgNoSup(const char *arg)
    {
        printf("ERROR: Argument %s is not supported on this platform\n", arg);
        return false;
    }

    bool parseArgs(int argc, char **argv)
    {
        // Skip first arg
        --argc;
        ++argv;

        while(argc > 0)
        {
            char *cur = *argv;

            if(!std::strcmp(cur, "-bank"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-bank");
                bank = *argv;
            }
            else if(!std::strcmp(cur, "-loop"))
            {
#ifdef HW_DOS_BUILD
                loop = true;
#else
                loop = !wave;
#endif
            }
            else if(!std::strcmp(cur, "-setup"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-setup");
                setup = *argv;
            }
            else if(!std::strcmp(cur, "-opl3"))
            {
                setup = setupString;
                std::strncat(setupString, "-opl3 ", 50);
            }
            else if(!std::strcmp(cur, "-doom1"))
            {
                setup = setupString;
                std::strncat(setupString, "-doom1 ", 50);
            }
            else if(!std::strcmp(cur, "-doom2"))
            {
                setup = setupString;
                std::strncat(setupString, "-doom2 ", 50);
            }
#ifdef HW_DOS_BUILD
            else if(!std::strcmp(cur, "-freq"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-freq");

                unsigned timerFreq = std::strtoul(*argv, NULL, 0);
                if(timerFreq == 0)
                {
                    printf("The option -freq requires a non-zero integer argument!\n");
                    return false;
                }

                s_timeCounter.setDosTimerHZ(timerFreq);
            }
            else if(!std::strcmp(cur, "-addr"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-addr");

                hw_addr = std::strtoul(argv[3], NULL, 0);
                if(hw_addr == 0)
                {
                    printf("The option --addr requires a non-zero integer argument!\n");
                    return false;
                }

            }
            else if(!std::strcmp(cur, "-emu"))
                return printArgNoSup("-emu");
            else if(!std::strcmp(cur, "-gain"))
                return printArgNoSup("-gain");
            else if(!std::strcmp(cur, "-towave"))
                return printArgNoSup("-towave");
#else
            else if(!std::strcmp(cur, "-freq"))
                return printArgNoSup("-freq");
            else if(!std::strcmp(cur, "-gain"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-gain");

                gain = std::atof(*argv);
            }
            else if(!std::strcmp(cur, "-wave"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-wave");

                wave = true;
                loop = false;
                waveFile = *argv;
            }
            else if(!std::strcmp(cur, "-towave"))
            {
                wave = true;
                loop = false;
            }
            else if(!std::strcmp(cur, "-emu"))
            {
                --argc;
                ++argv;
                if(argc == 0)
                    return printArgFail("-emu");

                if(!std::strcmp(*argv, "nuked"))
                    emu_type = EMU_NUKED_OPL3;
                else if(!std::strcmp(*argv, "dosbox"))
                    emu_type = EMU_DOSBOX_OPL3;
                else if(!std::strcmp(*argv, "java"))
                    emu_type = EMU_JAVA_OPL3;
                else if(!std::strcmp(*argv, "opal"))
                    emu_type = EMU_OPAL_OPL3;
                else if(!std::strcmp(*argv, "ymfm-opl2"))
                    emu_type = EMU_YMFM_OPL2;
                else if(!std::strcmp(*argv, "ymfm-opl3"))
                    emu_type = EMU_YMFM_OPL3;
                else if(!std::strcmp(*argv, "mame-opl2"))
                    emu_type = EMU_MAME_OPL2;
                else if(!std::strcmp(*argv, "lle-opl2"))
                    emu_type = EMU_OPL2_LLE;
                else if(!std::strcmp(*argv, "lle-opl3"))
                    emu_type = EMU_OPL3_LLE;
                else
                {
                    printf("ERROR: Invalid emulator name: %s\n", *argv);
                    return false;
                }
            }
#endif
            else
            {
                song = *argv;
#ifndef HW_DOS_BUILD
                if(wave && !waveFile)
                {
                    std::strncpy(wavePath, song, 2048);
                    std::strncat(wavePath, ".wav", 2048);
                    waveFile = wavePath;
                }
#endif
                break; // Finish parse
            }

            --argc;
            ++argv;
        }

        return true;
    }
};

int main(int argc, char **argv)
{
    int ret = 0;
#ifndef HW_DOS_BUILD
    static SDL_AudioSpec spec, obtained;
#endif
    MIDI_Seq player;
    Args args;

    printf("=========================================\n"
           "DMX-like MIDI player by Vitaliy Novichkov\n"
           "Baded on Nuked.Ykt's WinMM driver\n"
           "=========================================\n");
    fflush(stdout);

    if(argc < 2 || !args.parseArgs(argc, argv))
    {
        printf("\n"
               "USAGE:\n\n"
               "  dmxplay [options] <filename>\n"
               "\n"
               "  <filename>       - Path to music file to play. Required.\n"
               "\n"
               "Supported options:\n"
               "  -bank <file.op2> - Path to custom OP2 bank file.\n"
               "  -loop            - Enable looping of the opened music file.\n"
#ifdef HW_DOS_BUILD
               "  -addr <0xVAL>    - [DOS ONLY] Set the hardware OPL2/OPL3 address.\n"
               "                     Default is 0x388.\n"
#endif
#ifndef HW_DOS_BUILD
               "  -gain <value>    - [Non-DOS ONLY] Set the gaining factor (default 2.0).\n"
               "  -wave <path.wav> - [Non-DOS ONLY] Record output into WAV file of spcified path.\n"
               "  -towave          - [Non-DOS ONLY] Record output into WAV file in a place.\n"
               "  -emu <name>      - [Non-DOS ONLY] Select playback chip emulator:\n"
               "                     nuked, dosbox, java, opal, ymfm-opl2, ymfm-opl3,\n"
               "                     mame-opl2, lle-opl2, lle-opl3\n"
#endif
               "  -opl3            - Enable OPL3 mode (by default the OPL2 mode).\n"
               "  -doom1           - Enable the Doom1 v1.666 mode (by default the v1.9 mode).\n"
               "  -doom2           - Enable the Doom2 v1.666 mode (by default the v1.9 mode).\n"
               "  -setup \"string\"  - Set a quoted space-separated setup string for synth\n"
               "                     same as DMXOPTION environment variable.\n"
               "\n");
        fflush(stdout);
        return 1;
    }

    player.openBank(args.bank);
    player.setSetupString(args.setup);
    player.setLoop(args.loop);

    std::fprintf(stdout, " - Use bank [%s]\n", args.bank);
    flushout(stdout);

#ifdef HW_DOS_BUILD
    s_timeCounter.initDosTimer();
    s_timeCounter.flushDosTimer();
#endif

    signal(SIGINT, &sig_playing);
    signal(SIGTERM, &sig_playing);

#ifndef HW_DOS_BUILD
    /* Initialize SDL.*/
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("Failed to initialize the SDL2! %s\n", SDL_GetError());
        fflush(stdout);
        return 1;
    }
#endif

    signal(SIGINT, &sig_playing);
    signal(SIGTERM, &sig_playing);

#ifndef HW_DOS_BUILD
    SDL_memset(&spec, 0, sizeof(SDL_AudioSpec));
    spec.freq = 48000;
    spec.format = AUDIO_F32SYS;
    spec.channels = 2;
    spec.samples = buffer_size;

    /* Open the target wave file */
    if(args.wave)
        SDL_memcpy(&obtained, &spec, sizeof(SDL_AudioSpec));
    else
    {
        /* set the callback function */
        spec.callback = my_audio_callback;
        /* set ADLMIDI's descriptor as userdata to use it for sound generation */
        spec.userdata = &player;

        /* Open the audio device if not writing WAV file */
        if(SDL_OpenAudio(&spec, &obtained) < 0)
        {
            fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
            fflush(stdout);
            return 1;
        }
    }

    printf(" - Gain factor: %g\n", args.gain);
    fflush(stdout);

    player.setGain(args.gain);
#else
    player.set_hw_addr(args.hw_addr);
#endif

#ifdef HW_DOS_BUILD
    if(!player.initSynth(0, 0))
#else
    if(!player.initSynth(args.emu_type, obtained.freq))
#endif
    {
        printf("Failed to initialize the synth!\n");
        fflush(stdout);
        return 2;
    }

#ifndef HW_DOS_BUILD
    if(!player.initStream(obtained.format, obtained.freq, obtained.channels))
    {
        printf("Failed to initialize the stream! %s\n", SDL_GetError());
        fflush(stdout);
        return 2;
    }
#endif

    if(!player.openMusic(args.song))
    {
        printf("Can't open music %s\n", args.song);
        fflush(stdout);
        return 1;
    }

    s_timeCounter.setTotal(player.duration());

    std::fprintf(stdout, " - %s in use\n", player.getEmuName());

    std::fprintf(stdout, " - Loop is turned %s\n", args.loop ? "ON" : "OFF");

    s_timeCounter.setLoop(player.loopStart(), player.loopEnd());
    if(s_timeCounter.hasLoop)
        std::fprintf(stdout, " - Has loop points: %s ... %s\n", s_timeCounter.loopStartHMS, s_timeCounter.loopEndHMS);
    std::fprintf(stdout, "\n==========================================\n");
    flushout(stdout);

    is_playing = 1;

#ifndef HW_DOS_BUILD
    /* Start playing */
    if(!args.wave)
        SDL_PauseAudio(0);
#endif

#ifndef HW_DOS_BUILD
    if(!args.wave)
        printf("Playing... Hit Ctrl+C to quit!\n");
#else
    printf("Playing... Hit Ctrl+C or ESC to quit!\n");
#endif

#ifndef HW_DOS_BUILD
    /* wait until we're don't playing */
    s_timeCounter.clearLineR();

    if(args.wave)
    {
        ret = runWaveOutLoopLoop(player, args.song, args.waveFile, spec);
    }
    else while(is_playing)
    {
        s_timeCounter.printTime(player.tell());
        SDL_Delay(1);
    }

    /* shut everything down */
    if(!args.wave)
        SDL_CloseAudio();
    SDL_Quit();
#else
    runDOSLoop(&player);
    s_timeCounter.restoreDosTimer();
#endif

    printf("\n");
    fflush(stdout);

    return ret;
}
