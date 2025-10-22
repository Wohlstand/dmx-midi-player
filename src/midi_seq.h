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

#ifndef MIDI_SEQ_H
#define MIDI_SEQ_H

#include <stddef.h>

class midisynth;
// Rename class to avoid ABI collisions
#define BW_MidiSequencer AdlMidiSequencer
class BW_MidiSequencer;
typedef BW_MidiSequencer MidiSequencer;
typedef struct BW_MidiRtInterface BW_MidiRtInterface;

#ifndef HW_DOS_BUILD
struct _SDL_AudioStream;
typedef struct _SDL_AudioStream SDL_AudioStream;
#endif

class MIDI_Seq
{
    midisynth *m_synth = nullptr;
    BW_MidiRtInterface *m_interface = nullptr;
    MidiSequencer *m_sequencer = nullptr;
#ifndef HW_DOS_BUILD
    SDL_AudioStream *m_stream = nullptr;
    unsigned char m_buffer[4096];
    const size_t m_buffer_max_size = 4096;

    unsigned char m_gainBuffer[4096];
    const size_t m_gainBuffer_max_size = 4096;

    unsigned int m_rate = 0;
    int m_output_format = 0;
    float m_gain = 2.0f;
#endif

    void initSeq();

    static void debugMessageHook(void *userdata, const char *fmt, ...);

public:
    MIDI_Seq();
    ~MIDI_Seq();

#ifndef HW_DOS_BUILD
    bool initStream(int out_fmt, int out_rate, int out_channels);
#else
    void set_hw_addr(unsigned short addr);
#endif

    void setSetupString(const char *setup);
    bool openBank(const char *bank);
    bool openMusic(const char *music);

#ifndef HW_DOS_BUILD
    void setGain(float gain);
#endif

    int initSynth(int emu_type, unsigned int rate);

    const char *getEmuName();

    void setLoop(bool enable);

    double tick(double s, double granularity);

    double tell();
    double duration();

    double loopStart();
    double loopEnd();

    bool atEnd();

    void panic();

#ifndef HW_DOS_BUILD
    int playBuffer(unsigned char *out, size_t len);
#endif
};

#endif // MIDI_SEQ_H
