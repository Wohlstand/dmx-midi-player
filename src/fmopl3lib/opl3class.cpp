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

#include "opl3class.h"

#ifndef HW_DOS_BUILD
#include "../emu_list.h"
#include "chips/dosbox_opl3.h"
#include "chips/nuked_opl3.h"
#include "chips/java_opl3.h"
#include "chips/ymfm_opl3.h"
#include "chips/ymfm_opl2.h"
#include "chips/mame_opl2.h"
#include "chips/opal_opl3.h"
#include "chips/ym3812_lle.h"
#include "chips/ymf262_lle.h"
#endif

#ifdef HW_DOS_BUILD
#   include <stdint.h>
#   include <pc.h>
static uint16_t                 s_OPLBase       = 0x388;

void set_chip_hw_address(unsigned short addr)
{
    s_OPLBase = addr;
}
#endif

opl3class::opl3class() : fm_chip()
{}

opl3class::~opl3class()
{
#ifndef HW_DOS_BUILD
    delete chip;
#endif
}

const char *opl3class::getEmuName()
{
#ifndef HW_DOS_BUILD
    return chip->emulatorName();
#else
    return "Hardware OPL2/OPL3 chip";
#endif
}

int opl3class::fm_init(int chip_emu, unsigned int rate) {
#ifndef HW_DOS_BUILD
    if(chip)
        delete chip;

    switch(chip_emu)
    {
    default:
    case EMU_NUKED_OPL3:
        chip = new NukedOPL3;
        break;
    case EMU_DOSBOX_OPL3:
        chip = new DosBoxOPL3;
        break;
    case EMU_JAVA_OPL3:
        chip = new JavaOPL3;
        break;
    case EMU_OPAL_OPL3:
        chip = new OpalOPL3;
        break;
    case EMU_YMFM_OPL3:
        chip = new YmFmOPL3;
        break;
    case EMU_YMFM_OPL2:
        chip = new YmFmOPL2;
        break;
    case EMU_MAME_OPL2:
        chip = new MameOPL2;
        break;
    case EMU_OPL2_LLE:
        chip = new Ym3812LLEOPL2;
        break;
    case EMU_OPL3_LLE:
        chip = new Ymf262LLEOPL3;
        break;
    }

    chip->setChipId(0);
    chip->setRate(rate);
#else
    (void)chip_emu;
    (void)rate;
#endif

    return 1;
}

void opl3class::fm_writereg(unsigned short reg, unsigned char data) {
#ifndef HW_DOS_BUILD
    chip->writeReg(reg, data);
#else
    unsigned c, o = reg >> 8, port = s_OPLBase + o * 2;
    outportb(port, reg);

    for(c = 0; c < 6; ++c)
        inportb(port);

    outportb(port + 1, data);

    for(c = 0; c < 24; ++c)
        inportb(port);
#endif
}

#ifndef HW_DOS_BUILD
inline int32_t adl_cvtS16(int32_t x)
{
    x = (x < INT16_MIN) ? (INT16_MIN) : x;
    x = (x > INT16_MAX) ? (INT16_MAX) : x;
    return x;
}
#endif

#ifndef HW_DOS_BUILD
void opl3class::fm_generate(int *buffer, unsigned int len) {
    chip->generate32(buffer, len);
    for(unsigned int i = 0; i < len; ++i)
    {
        // Convert to int32_t
        *buffer = *buffer * 65536;
        ++buffer;
        *buffer = *buffer * 65536;
        ++buffer;
    }
}
#endif

fm_chip *getchip() {
    opl3class *chip = new opl3class;
    return chip;
}
