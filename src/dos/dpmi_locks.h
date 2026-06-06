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

#ifndef DPMI_LOCKS_H
#define DPMI_LOCKS_H

#include <stddef.h>

extern void main_dpmi_lock_begin();
extern void main_dpmi_lock_end();

void dpmi_add_to_lock(void *ptr, size_t size);

#define dpmi_add_obj_to_lock(x) dpmi_add_to_lock((void*)&x, sizeof(x))

void dpmi_lock_classes();
void dpmi_unlock_classes();

#endif // DPMI_LOCKS_H
