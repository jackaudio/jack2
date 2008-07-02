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

#include "JackIOAdapter.h"

namespace Jack
{

int JackIOAdapterInterface::Open()
{
    int error;
    
    jack_log("fCaptureChannels %ld, fPlaybackChannels %ld", fCaptureChannels, fPlaybackChannels);
    
    for (int i = 0; i < fCaptureChannels; i++) {
        fCaptureResampler[i] = src_new(SRC_LINEAR, 1, &error);
        if (error != 0) {
            jack_error("JackIOAdapterInterface::Open err = %s", src_strerror(error));
            goto fail;
        }
    }
    
    for (int i = 0; i < fPlaybackChannels; i++) {
        fPlaybackResampler[i] = src_new(SRC_LINEAR, 1, &error);
        if (error != 0) {
            jack_error("JackIOAdapterInterface::Open err = %s", src_strerror(error));
            goto fail;
        }    
    }
    
    return 0;
    
fail:
    Close();
    return -1;
}


int JackIOAdapterInterface::Close()
{
    for (int i = 0; i < fCaptureChannels; i++) {
        if (fCaptureResampler[i] != NULL)
            src_delete(fCaptureResampler[i]);
    }   
    
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fPlaybackResampler[i] != NULL)
            src_delete(fPlaybackResampler[i]);
    }
    
    return 0;
}
 
} // namespace