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

#ifndef __JackDriver__
#define __JackDriver__

#include "types.h"
#include "JackClientInterface.h"
#include "JackConstants.h"
#include "JackPlatformPlug.h"
#include "JackClientControl.h"
#include <list>

namespace Jack
{

class JackLockedEngine;
class JackGraphManager;
struct JackEngineControl;
class JackSlaveDriverInterface;

/*!
\brief The base interface for drivers.
*/

class SERVER_EXPORT JackDriverInterface
{

    public:

        JackDriverInterface()
        {}
        virtual ~JackDriverInterface()
        {}

        virtual int Open() = 0;

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
                         jack_nframes_t playback_latency) = 0;

        virtual int Attach() = 0;
        virtual int Detach() = 0;

        virtual int Read() = 0;
        virtual int Write() = 0;

        virtual int Start() = 0;
        virtual int Stop() = 0;

        virtual bool IsFixedBufferSize() = 0;
        virtual int SetBufferSize(jack_nframes_t buffer_size) = 0;
        virtual int SetSampleRate(jack_nframes_t sample_rate) = 0;

        virtual int Process() = 0;

        virtual void SetMaster(bool onoff) = 0;
        virtual bool GetMaster() = 0;

        virtual void AddSlave(JackDriverInterface* slave) = 0;
        virtual void RemoveSlave(JackDriverInterface* slave) = 0;

        virtual std::list<JackDriverInterface*> GetSlaves() = 0;

        // For "master" driver
        virtual int ProcessReadSlaves() = 0;
        virtual int ProcessWriteSlaves() = 0;

        // For "slave" driver
        virtual int ProcessRead() = 0;
        virtual int ProcessWrite() = 0;

        // For "slave" driver in "synchronous" mode
        virtual int ProcessReadSync() = 0;
        virtual int ProcessWriteSync() = 0;

        // For "slave" driver in "asynchronous" mode
        virtual int ProcessReadAsync() = 0;
        virtual int ProcessWriteAsync() = 0;

        virtual bool IsRealTime() const = 0;
        virtual bool IsRunning() const = 0;
};

/*!
 \brief The base interface for drivers clients.
 */

class SERVER_EXPORT JackDriverClientInterface : public JackDriverInterface, public JackClientInterface
{};

/*!
 \brief The base class for drivers.
 */

#define CaptureDriverFlags  static_cast<JackPortFlags>(JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal)
#define PlaybackDriverFlags static_cast<JackPortFlags>(JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal)
#define MonitorDriverFlags static_cast<JackPortFlags>(JackPortIsOutput)

typedef std::list<std::pair<std::string, std::pair<std::string, std::string> > > driver_connections_list_t; // [type : (src, dst)]

class SERVER_EXPORT JackDriver : public JackDriverClientInterface
{

    protected:

        char fCaptureDriverName[JACK_CLIENT_NAME_SIZE+1];
        char fPlaybackDriverName[JACK_CLIENT_NAME_SIZE+1];
        char fAliasName[JACK_CLIENT_NAME_SIZE+1];

        jack_nframes_t fCaptureLatency;
        jack_nframes_t fPlaybackLatency;

        int fCaptureChannels;
        int fPlaybackChannels;

        jack_time_t fBeginDateUst;
        jack_time_t fEndDateUst;
        float fDelayedUsecs;

        // Pointers to engine state
        JackLockedEngine* fEngine;
        JackGraphManager* fGraphManager;
        JackSynchro* fSynchroTable;
        JackEngineControl* fEngineControl;
        JackClientControl fClientControl;

        std::list<JackDriverInterface*> fSlaveList;

        bool fIsMaster;
        bool fIsRunning;
        bool fWithMonitorPorts;

        // Static tables since the actual number of ports may be changed by the real driver
        // thus dynamic allocation is more difficult to handle
        jack_port_id_t fCapturePortList[DRIVER_PORT_NUM];
        jack_port_id_t fPlaybackPortList[DRIVER_PORT_NUM];
        jack_port_id_t fMonitorPortList[DRIVER_PORT_NUM];

        driver_connections_list_t fConnections;		// Connections list 

        void CycleIncTime();
        void CycleTakeBeginTime();
        void CycleTakeEndTime();

        void SetupDriverSync(int ref, bool freewheel);
     
        void NotifyXRun(jack_time_t callback_usecs, float delayed_usecs);   // XRun notification sent by the driver
        void NotifyBufferSize(jack_nframes_t buffer_size);                  // BufferSize notification sent by the driver
        void NotifySampleRate(jack_nframes_t sample_rate);                  // SampleRate notification sent by the driver
        void NotifyFailure(int code, const char* reason);                   // Failure notification sent by the driver

        virtual void SaveConnections(int alias);
        virtual void LoadConnections(int alias, bool full_name = true);
        std::string MatchPortName(const char* name, const char** ports, int alias, const std::string& type);

        virtual int StartSlaves();
        virtual int StopSlaves();

        virtual int ResumeRefNum();
        virtual int SuspendRefNum();

    public:

        JackDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        virtual ~JackDriver();

        void SetMaster(bool onoff);
        bool GetMaster();

        void AddSlave(JackDriverInterface* slave);
        void RemoveSlave(JackDriverInterface* slave);

        std::list<JackDriverInterface*> GetSlaves()
        {
            return fSlaveList;
        }

        virtual int Open();

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

        // For "master" driver
        int ProcessReadSlaves();
        int ProcessWriteSlaves();

        // For "slave" driver with typically decompose a given cycle in separated Read and Write parts.
        virtual int ProcessRead();
        virtual int ProcessWrite();

        // For "slave" driver in "synchronous" mode
        virtual int ProcessReadSync();
        virtual int ProcessWriteSync();

        // For "slave" driver in "asynchronous" mode
        virtual int ProcessReadAsync();
        virtual int ProcessWriteAsync();

        virtual bool IsFixedBufferSize();
        virtual int SetBufferSize(jack_nframes_t buffer_size);
        virtual int SetSampleRate(jack_nframes_t sample_rate);

        virtual int ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);
        virtual JackClientControl* GetClientControl() const;

        virtual bool IsRealTime() const;
        virtual bool IsRunning() const { return fIsRunning; }
        virtual bool Initialize();  // To be called by the wrapping thread Init method when the driver is a "blocking" one

};

} // end of namespace

#endif
