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


#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>

#define VERSION "1.0.0"

#ifndef HW_DOS_BUILD
#   include "utf8main/utf8main.h"
#   include "wav/wave_writer.h"
#   define SDL_MAIN_HANDLED
#   include <SDL2/SDL.h>
#   include "emu_list.h"
#else
#   include <conio.h>   // getch/kbhit
#   include <dos.h>     // delay
#   include "dos_tman.h"
typedef uint32_t Uint32;
typedef uint8_t Uint8;
#endif // HW_DOS_BUILD

#if defined(__WATCOMC__)
#include <stdio.h> // snprintf is here!
#endif
#include "flushout.h"

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
    char linebuff[81];
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

    TimeCounter()
    {
        hasLoop = false;
        totalTime = 0.0;
        milliseconds_prev = ~0u;
        printsCounter = 0;
        complete_prev = -1;
        printsCounterPeriod = 1;
    }

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

#ifdef HW_DOS_BUILD
    void waitDosTimerTick()
    {
        volatile unsigned long timer = DosTaskman::getCurTicks();
        while(timer == DosTaskman::getCurTicks());
    }

    void delay(int ticks)
    {
        volatile unsigned long timer = DosTaskman::getCurTicks() + ticks;
        while(timer >= DosTaskman::getCurTicks());
    }
#endif

    void initLineBuff()
    {
        std::memset(linebuff, ' ', sizeof(linebuff));
        linebuff[80] = '\0';
        linebuff[79] = '\r';
    }

    void clearLineR()
    {
        initLineBuff();
        std::fprintf(stdout, "%s", linebuff);
        flushout(stdout);
    }

    void printTime(double pos)
    {
        uint64_t milliseconds = static_cast<uint64_t>(pos * 1000.0);

        initLineBuff();
        int len;

        if(milliseconds != milliseconds_prev)
        {
            if(printsCounter >= printsCounterPeriod)
            {
                printsCounter = -1;
                secondsToHMSM(pos, posHMS, 25);
#ifdef HAS_S_GETTIME
                secondsToHMSM(s_getTime() - realTimeStart, realHMS, 25);
#endif
                // std::fprintf(stdout, "                                                        \r");
#ifdef HAS_S_GETTIME
                len = std::snprintf(linebuff, 79, "Time position: %s / %s [Real time: %s]\r", posHMS, totalHMS, realHMS);
#else
                len = std::snprintf(linebuff, 79, "Time position: %s / %s", posHMS, totalHMS);
#endif
                if(len > 0)
                    memset(linebuff + len, ' ',  79 - len);
                linebuff[79] = '\r';
                std::fprintf(stdout, "%s", linebuff);
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
        initLineBuff();
        std::fprintf(stdout, "%s\n\n", linebuff);
        flushout(stdout);
    }

} s_timeCounter;

#ifdef HW_DOS_BUILD

static inline void keyWait()
{
    std::printf("<press any key to continue...>");
    getch();
    std::printf("\r                              \r");
}

static double s_extra_delay = 0.0;
static DosTaskman *s_taskman = NULL;
static bool s_pause = false;

static void s_midiLoop(DosTaskman::DosTask *task)
{
    if(!is_playing || s_pause)
        return;

    MIDI_Seq *player = reinterpret_cast<MIDI_Seq *>(task->getData());
    const double mindelay = 1.0 / task->getFreq();
    double tickDelay;

    s_extra_delay = 0;

    if(task->getCount() >= task->getRate())
        s_extra_delay = mindelay;

    tickDelay = player->tick(mindelay + s_extra_delay, mindelay / 10.0);

    if(player->atEnd() || tickDelay <= 0)
        is_playing = false;
}

static void setCursorVisibility(bool visible)
{
    union REGS regs;

    regs.w.ax = 0x0100;
    regs.w.cx = visible ? 0x0708 : 0x2000;
#ifdef __FLAT__
    int386(0x10, &regs, &regs);
#else
    int86(0x10, &regs, &regs);
#endif
}

static void runDOSLoop(MIDI_Seq *myDevice)
{
    s_timeCounter.clearLineR();

    setCursorVisibility(false);

    while(is_playing)
    {
#   ifndef DEBUG_TRACE_ALL_EVENTS
        s_timeCounter.printTime(myDevice->tell());
#   endif

        if(kbhit())
        {   // Quit on ESC key!
            int c = getch();
            switch(c)
            {
            case 27:
                is_playing = false;
                break;
            case 'p':
            case 'P':
            {
                if(myDevice->songsNum() <= 1)
                    break; // Nothing to do
                s_pause = true;
                myDevice->prevSong();
                s_timeCounter.clearLineR();
                fprintf(stdout, " - Selecting song %d / %d\n", myDevice->curSong() + 1, myDevice->songsNum());
                flushout(stdout);
                s_pause = false;
                break;
            }
            case 'n':
            case 'N':
            {
                if(myDevice->songsNum() <= 1)
                    break; // Nothing to do
                s_pause = true;
                myDevice->nextSong();
                s_timeCounter.clearLineR();
                fprintf(stdout, " - Selecting song %d / %d\n", myDevice->curSong() + 1, myDevice->songsNum());
                flushout(stdout);
                s_pause = false;
                break;
            }
            case 'r':
            case 'R':
            {
                s_pause = true;
                myDevice->panic();
                myDevice->rewind();
                s_timeCounter.clearLineR();
                fprintf(stdout, " - Rewind song to begin...\n");
                flushout(stdout);
                s_pause = false;
                break;
            }
            }
        }

        s_timeCounter.waitDosTimerTick();
    }

    setCursorVisibility(true);

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
        flushout(stdout);
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

    size_t              soloTrack = ~(size_t)0;
    int                 songNumLoad = -1;
    std::vector<size_t> onlyTracks;

#ifdef HW_DOS_BUILD
    uint16_t hw_addr = 0x388;
    unsigned clock_freq = 209;
#endif

    bool printArgFail(const char *arg)
    {
        fprintf(stderr, "ERROR: Argument %s requires an option!\n", arg);
        flushout(stderr);
        return false;
    }

    bool printArgNoSup(const char *arg)
    {
        fprintf(stderr, "ERROR: Argument %s is not supported on this platform\n", arg);
        flushout(stderr);
        return false;
    }

#define ARG_SHIFT() \
    --argc; \
    ++argv

    class ArgsS
    {
        int m_argc;
        char **m_argv;
    public:
        explicit ArgsS(int argc, char **argv) :
            m_argc(argc), m_argv(argv)
        {}

        bool end()
        {
            return m_argc <= 0;
        }

        void shift()
        {
            --m_argc;
            ++m_argv;
        }

        const char *arg()
        {
            return *m_argv;
        }
    };

    bool parseArgs(int argc, char **argv)
    {
        ArgsS a(argc, argv);
        // Skip first arg (path to self application)
        a.shift();

        while(!a.end())
        {
            const char *cur = a.arg();

            if(!std::strcmp(cur, "-bank"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);
                bank = a.arg();
            }
            else if(!std::strcmp(cur, "-song"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);
                songNumLoad = std::strtol(a.arg(), NULL, 10);
            }
            else if(!std::strcmp(cur, "-solo"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);
                soloTrack = std::strtoul(a.arg(), NULL, 10);
            }
            else if(!std::strcmp(cur, "-only"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);

                const char *strp = a.arg();
                unsigned long value;
                unsigned size;
                bool err = std::sscanf(strp, "%lu%n", &value, &size) != 1;

                while(!err && *(strp += size))
                {
                    onlyTracks.push_back(value);
                    err = std::sscanf(strp, ",%lu%n", &value, &size) != 1;
                }
                if(err)
                {
                    fprintf(stderr, "Invalid argument to -only!\n");
                    return 1;
                }

                onlyTracks.push_back(value);
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
                a.shift();
                if(a.end())
                    return printArgFail(cur);
                setup = a.arg();
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
                a.shift();
                if(a.end())
                    return printArgFail(cur);

                unsigned timerFreq = std::strtoul(a.arg(), NULL, 0);
                if(timerFreq == 0)
                {
                    printf("The option -freq requires a non-zero integer argument!\n");
                    return false;
                }

                clock_freq = timerFreq;
            }
            else if(!std::strcmp(cur, "-addr"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);

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
                a.shift();
                if(a.end())
                    return printArgFail(cur);

                gain = std::atof(a.arg());
            }
            else if(!std::strcmp(cur, "-wave"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);

                wave = true;
                loop = false;
                waveFile = a.arg();
            }
            else if(!std::strcmp(cur, "-towave"))
            {
                wave = true;
                loop = false;
            }
            else if(!std::strcmp(cur, "-emu"))
            {
                a.shift();
                if(a.end())
                    return printArgFail(cur);

                if(!std::strcmp(a.arg(), "nuked"))
                    emu_type = EMU_NUKED_OPL3;
                else if(!std::strcmp(a.arg(), "dosbox"))
                    emu_type = EMU_DOSBOX_OPL3;
                else if(!std::strcmp(a.arg(), "java"))
                    emu_type = EMU_JAVA_OPL3;
                else if(!std::strcmp(a.arg(), "opal"))
                    emu_type = EMU_OPAL_OPL3;
                else if(!std::strcmp(a.arg(), "ymfm-opl2"))
                    emu_type = EMU_YMFM_OPL2;
                else if(!std::strcmp(a.arg(), "ymfm-opl3"))
                    emu_type = EMU_YMFM_OPL3;
                else if(!std::strcmp(a.arg(), "mame-opl2"))
                    emu_type = EMU_MAME_OPL2;
                else if(!std::strcmp(a.arg(), "lle-opl2"))
                    emu_type = EMU_OPL2_LLE;
                else if(!std::strcmp(a.arg(), "lle-opl3"))
                    emu_type = EMU_OPL3_LLE;
                else
                {
                    printf("ERROR: Invalid emulator name: %s\n", a.arg());
                    return false;
                }
            }
#endif
            else
            {
                song = a.arg();
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

            a.shift();
        }

        return true;
    }
};

int main(int argc, char **argv)
{
    int ret = 0;
#ifndef HW_DOS_BUILD
    static SDL_AudioSpec spec, obtained;
#else
    DosTaskman taskMan;
#endif
    MIDI_Seq player;
    Args args;

    printf("==============================================================\n"
           "DMX-like MIDI player by Vitaliy Novichkov, version " VERSION "\n"
           "Based on Nuke.Ykt's WinMM driver\n"
           "==============================================================\n"
           "(c) 2025 Vitaliy Novichkov, licensed under GNU GPLv2+\n"
           "Soure code: https://github.com/Wohlstand/dmx-midi-player/\n"
           "==============================================================\n");
    flushout(stdout);

    if(argc < 2 || !args.parseArgs(argc, argv))
    {
        const char *help_text =
            "\n"
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
            "  -song <NUM>      - Select song to play from 0 to N-1 (XMI only).\n"
            "  -solo <TRACK>    - Set MIDI track number to play solo.\n"
            "  -only <T1,..Tn>  - Set a comma-separated list of MIDI tracks to play solo.\n"
            "  -opl3            - Enable OPL3 mode (by default the OPL2 mode).\n"
            "  -doom1           - Enable the Doom1 v1.666 mode (by default the v1.9 mode).\n"
            "  -doom2           - Enable the Doom2 v1.666 mode (by default the v1.9 mode).\n"
            "  -setup \"string\"  - Set a quoted space-separated setup string for synth\n"
            "                     in same as DMXOPTION environment variable.\n"
            "\n";

#ifdef HW_DOS_BUILD
        int lines = 8; // Lines of banner
        const char *cur = help_text;

        for(; *cur != '\0'; ++cur)
        {
            char c = *cur;
            std::putc(c, stdout);
            if(c == '\n')
                lines++;

            if(lines >= 24)
            {
                keyWait();
                lines = 0;
            }
        }
#else
        std::printf("%s", help_text);
        flushout(stdout);
#endif
        return 1;
    }

    player.openBank(args.bank);
    player.setSetupString(args.setup);
    player.setLoop(args.loop);

    std::fprintf(stdout, " - Use bank [%s]\n", args.bank);
    flushout(stdout);

    signal(SIGINT, &sig_playing);
    signal(SIGTERM, &sig_playing);

#ifndef HW_DOS_BUILD
    /* Initialize SDL.*/
    if(SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("Failed to initialize the SDL2! %s\n", SDL_GetError());
        flushout(stdout);
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
            flushout(stdout);
            return 1;
        }
    }

    printf(" - Gain factor: %g\n", args.gain);
    flushout(stdout);

    player.setGain(args.gain);
#else
    player.set_hw_addr(args.hw_addr);
    std::fprintf(stdout, " - [DOS] Running clock with %u hz\n", args.clock_freq);
#endif

#ifdef HW_DOS_BUILD
    if(!player.initSynth(0, 0))
#else
    if(!player.initSynth(args.emu_type, obtained.freq))
#endif
    {
        printf("Failed to initialize the synth!\n");
        flushout(stdout);
        return 2;
    }

#ifndef HW_DOS_BUILD
    if(!player.initStream(obtained.format, obtained.freq, obtained.channels))
    {
        printf("Failed to initialize the stream! %s\n", SDL_GetError());
        flushout(stdout);
        return 2;
    }
#endif

    if(!player.openMusic(args.song))
    {
        printf("Can't open music %s\n", args.song);
        flushout(stdout);
        return 1;
    }

    s_timeCounter.setTotal(player.duration());

    std::fprintf(stdout, " - %s in use\n", player.getEmuName());

    if(args.songNumLoad >= 0)
        player.selectSong(args.songNumLoad);

    int songsCount = player.songsNum();
    if(args.songNumLoad >= 0)
        std::fprintf(stdout, " - Attempt to load song number: %d / %d\n", args.songNumLoad + 1, songsCount);
    else if(songsCount > 0)
        std::fprintf(stdout, " - File contains %d song(s)\n", songsCount);

    if(args.soloTrack != ~static_cast<size_t>(0))
    {
        std::fprintf(stdout, " - Solo track: %lu\n", static_cast<unsigned long>(args.soloTrack));
        player.setSoloTrack(args.soloTrack);
    }

    if(!args.onlyTracks.empty())
    {
        size_t count = player.getTracksCount();
        for(size_t track = 0; track < count; ++track)
            player.setTrackEnabled(track, false);

        std::fprintf(stdout, " - Only tracks:");
        for(size_t i = 0, n = args.onlyTracks.size(); i < n; ++i)
        {
            size_t track = args.onlyTracks[i];
            player.setTrackEnabled(track, true);
            std::fprintf(stdout, " %lu", static_cast<unsigned long>(track));
        }

        std::fprintf(stdout, "\n");
    }

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
    printf("Playing... Hit Ctrl+C or ESC to quit!\n"
           "  R - Rewind song to begin\n");
    if(songsCount > 1)
        printf("  N - Next song, P - Previous\n");
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
    DosTaskman::DosTask *midiTask = taskMan.addTask(s_midiLoop, args.clock_freq, 1, &player);
    s_taskman = &taskMan;
    taskMan.dispatch();
    runDOSLoop(&player);
    taskMan.terminate(midiTask);
    // s_timeCounter.restoreDosTimer();
#endif

    printf("\n");
    flushout(stdout);

    return ret;
}
