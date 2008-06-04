/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004 Grame
Copyright (C) 2007 Pieter Palmers

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

#ifndef __JackFreebobDriver__
#define __JackFreebobDriver__

#include "JackAudioDriver.h"
#include "JackThreadedDriver.h"
#include "JackTime.h"

#include "freebob_driver.h"

namespace Jack
{

/*!
\brief The FreeBoB driver.
*/

class JackFreebobDriver : public JackAudioDriver
{

    private:

        // enable verbose messages
        int g_verbose;

        jack_driver_t* fDriver;
        int freebob_driver_attach (freebob_driver_t *driver);
        int freebob_driver_detach (freebob_driver_t *driver);
        int freebob_driver_read (freebob_driver_t * driver, jack_nframes_t nframes);
        int freebob_driver_write (freebob_driver_t * driver, jack_nframes_t nframes);
        jack_nframes_t freebob_driver_wait (freebob_driver_t *driver,
                                            int extra_fd, int *status,
                                            float *delayed_usecs);
        int freebob_driver_start (freebob_driver_t *driver);
        int freebob_driver_stop (freebob_driver_t *driver);
        int freebob_driver_restart (freebob_driver_t *driver);
        freebob_driver_t *freebob_driver_new (char *name, freebob_jack_settings_t *params);
        void freebob_driver_delete (freebob_driver_t *driver);

#ifdef FREEBOB_DRIVER_WITH_MIDI
        freebob_driver_midi_handle_t *freebob_driver_midi_init(freebob_driver_t *driver);
        void freebob_driver_midi_finish (freebob_driver_midi_handle_t *m);
        int freebob_driver_midi_start (freebob_driver_midi_handle_t *m);
        int freebob_driver_midi_stop (freebob_driver_midi_handle_t *m);
#endif

        void jack_driver_init (jack_driver_t *driver);
        void jack_driver_nt_init (jack_driver_nt_t * driver);

    public:

        JackFreebobDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
            :JackAudioDriver(name, alias, engine, table)
        {}
        virtual ~JackFreebobDriver()
        {}

        int Open(freebob_jack_settings_t *cmlparams);

        int Close();
        int Attach();
        int Detach();

        int Start();
        int Stop();

        int Read();
        int Write();

        int SetBufferSize(jack_nframes_t nframes);
};

} // end of namespace

#endif
