//
// Copyright (C) 2015 Alexey Khokholov (Nuke.YKT)
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

class fm_chip {
public:
    fm_chip() = default;
    virtual ~fm_chip() = default;

    virtual const char *getEmuName() = 0;

    virtual int fm_init(int chip_emu, unsigned int rate) = 0;
    virtual void fm_writereg(unsigned short reg, unsigned char data) = 0;
#ifndef HW_DOS_BUILD
    virtual void fm_generate(int *buffer, unsigned int length) = 0;
#endif
};

class midisynth {
public:
    midisynth() = default;
    virtual ~midisynth() = default;

    virtual int midi_init(int emu_type, unsigned int rate) = 0;

    virtual const char *getEmuName() = 0;

    virtual void setup_string(const char *setup) = 0;
    virtual bool load_bank_file(const char *bank_path) = 0;

    virtual void midi_write(unsigned int data) = 0;

    virtual void midi_panic() = 0;
    virtual void midi_reset() = 0;

    // Direct submission of event
    virtual void midi_key_on(unsigned char chan, unsigned char key, unsigned char velocity) = 0;
    virtual void midi_key_off(unsigned char chan, unsigned char key, unsigned char velocity) = 0;
    virtual void midi_controller(unsigned char chan, unsigned char type, unsigned char value) = 0;
    virtual void midi_pitch_bend(unsigned char chan, unsigned char msb, unsigned char lsb) = 0;
    virtual void midi_program_change(unsigned char chan, unsigned char value) = 0;

#ifndef HW_DOS_BUILD
    virtual void midi_generate(int *buffer, unsigned int length) = 0;
#endif
};

midisynth* getsynth();
fm_chip* getchip();

#ifdef __DJGPP__
extern void set_chip_hw_address(unsigned short addr);
#endif
