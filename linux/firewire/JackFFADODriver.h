/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004 Grame
Copyright (C) 2007 Pieter Palmers
Copyright (C) 2012 Adrian Knoth

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

#ifndef __JackFFADODriver__
#define __JackFFADODriver__

#include "JackAudioDriver.h"
#include "JackThreadedDriver.h"
#include "JackTime.h"

#include "ffado_driver.h"

namespace Jack
{

/*!
\brief The FFADO driver.
*/

class JackFFADODriver : public JackAudioDriver
{

    private:

        // enable verbose messages
        int g_verbose;

        jack_driver_t* fDriver;
        int ffado_driver_attach (ffado_driver_t *driver);
        int ffado_driver_detach (ffado_driver_t *driver);
        int ffado_driver_read (ffado_driver_t * driver, jack_nframes_t nframes);
        int ffado_driver_write (ffado_driver_t * driver, jack_nframes_t nframes);
        jack_nframes_t ffado_driver_wait (ffado_driver_t *driver,
                                          int extra_fd, int *status,
                                          float *delayed_usecs);
        int ffado_driver_start (ffado_driver_t *driver);
        int ffado_driver_stop (ffado_driver_t *driver);
        int ffado_driver_restart (ffado_driver_t *driver);
        ffado_driver_t *ffado_driver_new (const char *name, ffado_jack_settings_t *params);
        void ffado_driver_delete (ffado_driver_t *driver);

        void jack_driver_init (jack_driver_t *driver);
        void jack_driver_nt_init (jack_driver_nt_t * driver);
        void UpdateLatencies();

    public:

        JackFFADODriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
            : JackAudioDriver(name, alias,engine, table)
        {}
        virtual ~JackFFADODriver()
        {}

        int Open(ffado_jack_settings_t *cmlparams);

        int Close();
        int Attach();
        int Detach();

        int Start();
        int Stop();

        int Read();
        int Write();

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
            return false;
        }

        int SetBufferSize(jack_nframes_t nframes);
};

} // end of namespace

#endif
