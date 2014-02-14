/*
Copyright (C) 2013 Matt Flax <flatmax@>

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

#ifndef JACKIIODRIVER_H
#define JACKIIODRIVER_H

#include "JackAudioDriver.h"
#include "JackThreadedDriver.h"

#include <IIO/IIO.H>

namespace Jack {

/** The Linux Industrial IO (IIO) subsystem driver for Jack.
Currently this driver only supports capture.
*/
class JackIIODriver : public JackAudioDriver {

    IIO iio; ///< The actual IIO device

public:
    /** Constructor
    */
    JackIIODriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table) : JackAudioDriver(name, alias, engine, table) {
    }

    /** Destructor
    */
    virtual ~JackIIODriver() {
    }

    /**
    */
    virtual int Open(jack_nframes_t buffer_size,
                    jack_nframes_t samplerate,
                    bool capturing,
                    bool playing,
                    int inchannels,
                    int outchannels,
                    bool monitor,
                    const char* capture_driver_name,
                    const char* playback_driver_name,
                    jack_nframes_t capture_latency,
                    jack_nframes_t playback_latency);

};

} // end of Jack namespace
#endif // JACKIIODRIVER_H
