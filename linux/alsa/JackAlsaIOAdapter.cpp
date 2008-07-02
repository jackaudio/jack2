/*
Copyright (C) 2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "JackAlsaIOAdapter.h"

namespace Jack
{

int JackAlsaIOAdapter::Open()
{
    if (fAudioInterface.open() == 0) {
        fAudioInterface.longinfo();
        fAudioInterface.write();
        fAudioInterface.write();
        fThread.AcquireRealTime();
        fThread.StartSync();
        return 0;
    } else {
        return -1;
    }
}

int JackAlsaIOAdapter::Close()
{
    fThread.Stop();
    return fAudioInterface.close();
}
            
bool JackAlsaIOAdapter::Execute()
{
    if (fAudioInterface.read() < 0)
        return false;
    if (fAudioInterface.write() < 0)
        return false;
    return true;
}

int JackAlsaIOAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    return 0;
}

        
}
