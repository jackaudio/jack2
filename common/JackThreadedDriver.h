/*
 Copyright (C) 2001 Paul Davis
 Copyright (C) 2004-2008 Grame

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

#ifndef __JackThreadedDriver__
#define __JackThreadedDriver__

#include "JackDriver.h"
#include "JackPlatformPlug.h"

namespace Jack
{

/*!
\brief The base class for threaded drivers using a "decorator" pattern. Threaded drivers are used with blocking devices.
*/

class SERVER_EXPORT JackThreadedDriver : public JackDriverClientInterface, public JackRunnableInterface
{

    protected:

        JackThread fThread;
        JackDriver* fDriver;

        void SetRealTime();

    public:

        JackThreadedDriver(JackDriver* driver);
        virtual ~JackThreadedDriver();

        virtual int Open();

        virtual int Open (bool capturing,
                         bool playing,
                         int inchannels,
                         int outchannels,
                         bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency)
        {
            return -1;
        }
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
        virtual int Close();

        virtual int Process();

        virtual int Attach();
        virtual int Detach();

        virtual int Read();
        virtual int Write();

        virtual int Start();
        virtual int Stop();

        virtual bool IsFixedBufferSize();
        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetSampleRate(jack_nframes_t sample_rate);

        virtual void SetMaster(bool onoff);
        virtual bool GetMaster();

        virtual void AddSlave(JackDriverInterface* slave);
        virtual void RemoveSlave(JackDriverInterface* slave);

        virtual std::list<JackDriverInterface*> GetSlaves();

        virtual int ProcessReadSlaves();
        virtual int ProcessWriteSlaves();

        virtual int ProcessRead();
        virtual int ProcessWrite();

        virtual int ProcessReadSync();
        virtual int ProcessWriteSync();

        virtual int ProcessReadAsync();
        virtual int ProcessWriteAsync();

        virtual int ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);
        virtual JackClientControl* GetClientControl() const;
        virtual bool IsRealTime() const;
        virtual bool IsRunning() const;

        // JackRunnableInterface interface
        virtual bool Execute();
        virtual bool Init();

};

} // end of namespace


#endif
