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

#ifndef __JackOSSChannel__
#define __JackOSSChannel__

#include "JackMutex.h"
#include "JackThread.h"
#include "sosso/Correction.hpp"
#include "sosso/DoubleBuffer.hpp"
#include "sosso/FrameClock.hpp"
#include "sosso/ReadChannel.hpp"
#include "sosso/WriteChannel.hpp"

namespace Jack
{

typedef jack_default_audio_sample_t jack_sample_t;

/*!
\brief The OSS driver.
*/

class JackOSSChannel : public JackRunnableInterface
{

    private:
        JackThread fAssistThread;
        JackProcessSync fMutex;
        sosso::FrameClock fFrameClock;
        sosso::DoubleBuffer<sosso::ReadChannel> fReadChannel;
        sosso::DoubleBuffer<sosso::WriteChannel> fWriteChannel;
        sosso::Correction fCorrection;

        std::int64_t fFrameStamp = 0;

    public:

        JackOSSChannel() : fAssistThread(this)
        {}
        virtual ~JackOSSChannel()
        {}

        sosso::DoubleBuffer<sosso::ReadChannel> &Capture()
        {
            return fReadChannel;
        }

        sosso::DoubleBuffer<sosso::WriteChannel> &Playback()
        {
            return fWriteChannel;
        }

        sosso::FrameClock &FrameClock()
        {
            return fFrameClock;
        }

        bool Lock()
        {
            return fMutex.Lock();
        }

        bool Unlock()
        {
            return fMutex.Unlock();
        }

        void SignalWork()
        {
            fMutex.SignalAll();
        }

        bool InitialSetup(unsigned sample_rate);

        bool OpenCapture(const char* device, bool exclusive, int bits, int &channels);
        bool OpenPlayback(const char* device, bool exclusive, int bits, int &channels);

        bool Read(jack_sample_t** sample_buffers, jack_nframes_t length, std::int64_t end);
        bool Write(jack_sample_t** sample_buffers, jack_nframes_t length, std::int64_t end);

        bool StartChannels(unsigned buffer_frames);
        bool StopChannels();

        bool StartAssistThread(bool realtime, int priority);
        bool StopAssistThread();

        bool CheckTimeAndRun();

        bool Sleep() const;

        bool CaptureFinished() const;
        bool PlaybackFinished() const;

        std::int64_t PlaybackCorrection();

        virtual bool Init();

        virtual bool Execute();

        std::int64_t XRunGap() const;

        void ResetBuffers(std::int64_t offset);

        std::int64_t FrameStamp() const
        {
            return fFrameStamp;
        }

        std::int64_t NextWakeup() const;
};

} // end of namespace

#endif
