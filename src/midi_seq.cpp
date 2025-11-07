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

#ifndef HW_DOS_BUILD
#   include <SDL2/SDL_audio.h>
#endif

#include <cstdarg>
#include "flushout.h"
// Rename class to avoid ABI collisions
#define BW_MidiSequencer AdlMidiSequencer
#define BWMIDI_ENABLE_OPL_MUSIC_SUPPORT
// Inlucde MIDI sequencer class implementation
#include "seq/midi_sequencer_impl.hpp"
#include "midi_seq.h"

// Synth interface
#include "interface.h"

#define OPL_CHIP_RATE  50000 // 49716

/****************************************************
 *           Real-Time MIDI calls proxies           *
 ****************************************************/

static void rtNoteOn(void *userdata, uint8_t channel, uint8_t note, uint8_t velocity)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_key_on(channel, note, velocity);
}

static void rtNoteOff(void *userdata, uint8_t channel, uint8_t note)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_key_off(channel, note, 0);
}

static void rtNoteAfterTouch(void *userdata, uint8_t channel, uint8_t note, uint8_t atVal)
{
    // Unsupported
    (void)userdata;
    (void)channel;
    (void)note;
    (void)atVal;
    // midisynth *context = reinterpret_cast<midisynth *>(userdata);
    // context->realTime_NoteAfterTouch(channel, note, atVal);
}

static void rtChannelAfterTouch(void *userdata, uint8_t channel, uint8_t atVal)
{
    // Unsupported
    (void)userdata;
    (void)channel;
    (void)atVal;
    // midisynth *context = reinterpret_cast<midisynth *>(userdata);
    // context->realTime_ChannelAfterTouch(channel, atVal);
}

static void rtControllerChange(void *userdata, uint8_t channel, uint8_t type, uint8_t value)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_controller(channel, type, value);
}

static void rtPatchChange(void *userdata, uint8_t channel, uint8_t patch)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_program_change(channel, patch);
}

static void rtPitchBend(void *userdata, uint8_t channel, uint8_t msb, uint8_t lsb)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_pitch_bend(channel, msb, lsb);
}

static void rtSysEx(void *userdata, const uint8_t *msg, size_t size)
{
    // Unsupported
    (void)userdata;
    (void)msg;
    (void)size;
    // midisynth *context = reinterpret_cast<midisynth *>(userdata);
    // context->realTime_SysEx(msg, size);
}


/* NonStandard calls */
static void rtSongBegin(void *userdata)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_reset();
}

#ifndef HW_DOS_BUILD
static void playSynth(void *userdata, uint8_t *stream, size_t length)
{
    midisynth *context = reinterpret_cast<midisynth *>(userdata);
    context->midi_generate(reinterpret_cast<int*>(stream), length / (2 * sizeof(int)));
}
#endif

void MIDI_Seq::initSeq()
{
    std::memset(m_interface, 0, sizeof(BW_MidiRtInterface));
    m_interface->onDebugMessage             = debugMessageHook;
    m_interface->onDebugMessage_userData    = this;

    /* MIDI Real-Time calls */
    m_interface->rtUserData = m_synth;
    m_interface->rt_noteOn  = rtNoteOn;
    m_interface->rt_noteOff = rtNoteOff;
    m_interface->rt_noteAfterTouch = rtNoteAfterTouch;
    m_interface->rt_channelAfterTouch = rtChannelAfterTouch;
    m_interface->rt_controllerChange = rtControllerChange;
    m_interface->rt_patchChange = rtPatchChange;
    m_interface->rt_pitchBend = rtPitchBend;
    m_interface->rt_systemExclusive = rtSysEx;

    m_interface->onSongStart = rtSongBegin;
    m_interface->onSongStart_userData = m_synth;
    // m_interface->onloopStart = hooks.onLoopStart;
    // m_interface->onloopStart_userData = hooks.onLoopStart_userData;
    // m_interface->onloopEnd = hooks.onLoopEnd;
    // m_interface->onloopEnd_userData = hooks.onLoopEnd_userData;
    /* NonStandard calls End */

#ifndef HW_DOS_BUILD
    m_interface->onPcmRender = playSynth;
    m_interface->onPcmRender_userData = m_synth;
#endif

#ifndef HW_DOS_BUILD
    m_interface->pcmSampleRate = m_rate;
    m_interface->pcmFrameSize = sizeof(int) * 2;
#endif

    m_sequencer->setInterface(m_interface);
}

void MIDI_Seq::debugMessageHook(void */*userdata*/, const char *fmt, ...)
{
    // MIDI_Seq *self = reinterpret_cast<MIDI_Seq*>(userdata);
    char buffer[4096];
    std::va_list args;
    va_start(args, fmt);
    int rc = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if(rc > 0)
    {
        s_fprintf(stdout, " - Debug: %s\n", buffer);
        flushout(stdout);
    }
}

MIDI_Seq::MIDI_Seq() :
    m_synth(getsynth()),
    m_interface(new BW_MidiRtInterface),
    m_sequencer(new BW_MidiSequencer)
{}

MIDI_Seq::~MIDI_Seq()
{
#ifndef HW_DOS_BUILD
    if(m_stream)
        SDL_FreeAudioStream(m_stream);
#endif

    delete m_sequencer;
    delete m_synth;
    delete m_interface;
}

#ifdef HW_DOS_BUILD
void MIDI_Seq::set_hw_addr(unsigned short addr)
{
    set_chip_hw_address(addr);
}
#endif

#ifndef HW_DOS_BUILD
bool MIDI_Seq::initStream(int out_fmt, int out_rate, int out_channels)
{
    if(m_stream)
        SDL_FreeAudioStream(m_stream);

    m_output_format = out_fmt;
    m_stream = SDL_NewAudioStream(AUDIO_S32SYS, 2, m_rate, out_fmt, out_channels, out_rate);
    return m_stream != nullptr;
}
#endif

void MIDI_Seq::setIgnoreEnv(bool ignore)
{
    m_synth->ignore_env(ignore);
}

void MIDI_Seq::setSetupString(const char *setup)
{
    m_synth->setup_string(setup);
}

bool MIDI_Seq::openBank(const char *bank)
{
    return m_synth->load_bank_file(bank);
}

bool MIDI_Seq::openMusic(const char *music)
{
    if(!music)
        return false;

    return m_sequencer->loadMIDI(music);
}

#ifndef HW_DOS_BUILD
void MIDI_Seq::setGain(float gain)
{
    m_gain = gain;
}
#endif

int MIDI_Seq::initSynth(int emu_type, unsigned int rate)
{
#ifndef HW_DOS_BUILD
    m_rate = rate;
#endif
    initSeq();
    return m_synth->midi_init(emu_type, rate);
}

const char *MIDI_Seq::getEmuName()
{
    return m_synth->getEmuName();
}

void MIDI_Seq::setLoop(bool enable)
{
    m_sequencer->setLoopEnabled(enable);
}

void MIDI_Seq::setSoloTrack(size_t solo)
{
    m_sequencer->setSoloTrack(solo);
}

size_t MIDI_Seq::getTracksCount()
{
    return m_sequencer->getTrackCount();
}

void MIDI_Seq::setTrackEnabled(size_t track, bool enabled)
{
    m_sequencer->setTrackEnabled(track, enabled);
}

int MIDI_Seq::songsNum()
{
    return m_sequencer->getSongsCount();
}

int MIDI_Seq::curSong()
{
    return m_cur_song;
}

void MIDI_Seq::selectSong(int song)
{
    if(m_sequencer->getSongsCount() <= 1)
        return;

    m_sequencer->setSongNum(song);
    m_cur_song = song;
}

void MIDI_Seq::nextSong()
{
    if(m_sequencer->getSongsCount() <= 1)
        return;

    ++m_cur_song;
    if(m_cur_song >= m_sequencer->getSongsCount())
        m_cur_song = 0;

    m_sequencer->setSongNum(m_cur_song);
}

void MIDI_Seq::prevSong()
{
    if(m_sequencer->getSongsCount() <= 1)
        return;

    --m_cur_song;
    if(m_cur_song < 0)
        m_cur_song = m_sequencer->getSongsCount() - 1;

    m_sequencer->setSongNum(m_cur_song);
}

void MIDI_Seq::rewind()
{
    m_sequencer->rewind();
}

double MIDI_Seq::tick(double s, double granularity)
{
    return m_sequencer->Tick(s, granularity);
}

double MIDI_Seq::tell()
{
    return m_sequencer->tell();
}

double MIDI_Seq::duration()
{
    return m_sequencer->timeLength();
}

double MIDI_Seq::loopStart()
{
    return m_sequencer->getLoopStart();
}

double MIDI_Seq::loopEnd()
{
    return m_sequencer->getLoopEnd();
}

bool MIDI_Seq::atEnd()
{
    return m_sequencer->positionAtEnd();
}

void MIDI_Seq::panic()
{
    m_synth->midi_panic();
}

#ifndef HW_DOS_BUILD
size_t MIDI_Seq::playBuffer(unsigned char *out, size_t len)
{
    const size_t init_len = len;
    size_t out_written = 0;
    int ret, filled;
    int attempts = 0;

    SDL_memset(out, 0, len);

retry:
    if(len == 0 || len > init_len || attempts > 10)
        return out_written;

    filled = SDL_AudioStreamGet(m_stream, m_gainBuffer, len > m_gainBuffer_max_size ? m_gainBuffer_max_size : len);

    if(filled != 0)
    {
        if(filled < 0)
            return 0; // FAIL!

        SDL_MixAudioFormat(out, m_gainBuffer, m_output_format, filled, static_cast<int>(SDL_MIX_MAXVOLUME * m_gain));

        out_written += filled;

        if(out_written == init_len)
            return out_written;

        out += filled;
        len -= filled;
    }

    ret = m_sequencer->playStream(m_buffer, m_buffer_max_size);

    if(ret > 0)
        SDL_AudioStreamPut(m_stream, m_buffer, ret);
    else
        ++attempts;

    goto retry;
}

#endif
