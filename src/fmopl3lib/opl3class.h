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

#include "../interface.h"

#ifndef HW_DOS_BUILD
class OPLChipBase;
#endif

class opl3class : public fm_chip {
private:
#ifndef HW_DOS_BUILD
    OPLChipBase *chip = nullptr;
#endif
public:
    opl3class();
    ~opl3class();

    const char *getEmuName();

    int fm_init(int chip_emu, unsigned int rate);
    void fm_writereg(unsigned short reg, unsigned char data);
#ifndef HW_DOS_BUILD
    void fm_generate(int *buffer, unsigned int length);
#endif
};
