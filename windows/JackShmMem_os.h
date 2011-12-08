/*
 Copyright (C) 2004-2008 Grame

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */

#ifndef __JackShmMem_WIN32__
#define __JackShmMem_WIN32__

#include <windows.h>

inline bool CHECK_MLOCK(void* ptr, size_t size)
{
    if (!VirtualLock((ptr), (size))) {
        SIZE_T minWSS, maxWSS;
        HANDLE hProc = GetCurrentProcess();
        if (GetProcessWorkingSetSize(hProc, &minWSS, &maxWSS)) {
            const size_t increase = size + (10 * 4096);
            maxWSS += increase;
            minWSS += increase;
            if (!SetProcessWorkingSetSize(hProc, minWSS, maxWSS)) {
                jack_error("SetProcessWorkingSetSize error = %d", GetLastError());
                return false;
            } else if (!VirtualLock((ptr), (size))) {
                jack_error("VirtualLock error = %d", GetLastError());
                return false;
            } else {
                return true;
            }
        } else {
            return false;
        }
    } else {
        return true;
    }
}

#define CHECK_MUNLOCK(ptr, size) (VirtualUnlock((ptr), (size)) != 0)
#define CHECK_MLOCKALL()(false)
#define CHECK_MUNLOCKALL()(false)

#endif

