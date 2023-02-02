/*
Copyright (C) 2003-2007 Jussi Laako <jussi@sonarnerd.net>
Copyright (C) 2008 Grame & RTL 2008

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

#ifndef __JackOSSDriver__
#define __JackOSSDriver__

#include "JackAudioDriver.h"
#include "JackOSSChannel.h"

namespace Jack
{

typedef jack_default_audio_sample_t jack_sample_t;

#define OSS_DRIVER_DEF_DEV "/dev/dsp"
#define OSS_DRIVER_DEF_FS 48000
#define OSS_DRIVER_DEF_BLKSIZE 1024
#define OSS_DRIVER_DEF_NPERIODS 1
#define OSS_DRIVER_DEF_BITS 16
#define OSS_DRIVER_DEF_INS 2
#define OSS_DRIVER_DEF_OUTS 2

/*!
\brief The OSS driver.
*/

class JackOSSDriver : public JackAudioDriver
{
    private:

        int fBits;
        int fNperiods;
        bool fCapture;
        bool fPlayback;
        bool fExcl;
        bool fIgnoreHW;

        std::int64_t fCycleEnd;
        std::int64_t fLastRun;
        std::int64_t fMaxRunGap;
        jack_sample_t** fSampleBuffers;

        JackOSSChannel fChannel;

        int OpenAux();
        void CloseAux();

    protected:
        virtual void UpdateLatencies();

    public:

        JackOSSDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
                : JackAudioDriver(name, alias, engine, table),
                fBits(0),
                fNperiods(0), fCapture(false), fPlayback(false), fExcl(false), fIgnoreHW(true),
                fCycleEnd(0), fLastRun(0), fMaxRunGap(0), fSampleBuffers(nullptr)
        {}

        virtual ~JackOSSDriver()
        {}

        int Open(jack_nframes_t frames_per_cycle,
                 int user_nperiods,
                 jack_nframes_t rate,
                 bool capturing,
                 bool playing,
                 int chan_in,
                 int chan_out,
                 bool vmix,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency,
                 int bits,
                 bool ignorehwbuf);

        int Close();

        int Read();
        int Write();

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
            return false;
        }

        int SetBufferSize(jack_nframes_t buffer_size);

};

} // end of namespace

#endif
