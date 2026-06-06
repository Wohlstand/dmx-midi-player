//
// Copyright(C) 2025-2026 Vitaliy Novichkov
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

#include <list>
#include <utility>
#include "dpmi_locks.h"

#include "adlmidi_dos.h"
#include "../midi_seq.h"
#include "../synthlib/i_oplmusic.h"

typedef std::list<std::pair<void*, size_t> > LockList_t;
static LockList_t m_lock_list;


void dpmi_add_to_lock(void *ptr, size_t size)
{
    m_lock_list.push_back(std::make_pair(ptr, size));
}

void dpmi_lock_classes()
{
    for(LockList_t::iterator it = m_lock_list.begin(); it != m_lock_list.end(); ++it)
        adl_dpmi_lock_memory(it->first, it->second);

    adl_dpmi_lock_class_code<fm_chip>();
    adl_dpmi_lock_class_code<midisynth>();

    adl_dpmi_lock_class_code<MIDI_Seq>();
    adl_dpmi_lock_code_region_fn(MIDI_Seq_dpmi_lock_begin, MIDI_Set_dpmi_lock_end);

    adl_dpmi_lock_class_code<DoomOPL>();
    adl_dpmi_lock_code_region_fn(DoomOPL_lockCodeBegin, DoomOPL_lockCodeEnd);

    adl_dpmi_lock_code_region_fn(main_dpmi_lock_begin, main_dpmi_lock_end);
}

void dpmi_unlock_classes()
{
    for(LockList_t::iterator it = m_lock_list.begin(); it != m_lock_list.end(); ++it)
        adl_dpmi_unlock_memory(it->first, it->second);

    adl_dpmi_unlock_class_code<fm_chip>();
    adl_dpmi_unlock_class_code<midisynth>();

    adl_dpmi_unlock_class_code<MIDI_Seq>();
    adl_dpmi_unlock_code_region_fn(MIDI_Seq_dpmi_lock_begin, MIDI_Set_dpmi_lock_end);

    adl_dpmi_unlock_class_code<DoomOPL>();
    adl_dpmi_unlock_code_region_fn(DoomOPL_lockCodeBegin, DoomOPL_lockCodeEnd);

    adl_dpmi_unlock_code_region_fn(main_dpmi_lock_begin, main_dpmi_lock_end);
}
