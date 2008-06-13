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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include "JackThreadedDriver.h"
#include "JackError.h"
#include "JackGlobals.h"
#include "JackClient.h"
#include "JackEngineControl.h"
#include "JackException.h"

namespace Jack
{

JackThreadedDriver::JackThreadedDriver(JackDriverClient* driver):fThread(this)
{
    fDriver = driver;
}

JackThreadedDriver::~JackThreadedDriver()
{
    delete fDriver;
}

int JackThreadedDriver::Open()
{
    return fDriver->Open();
}

int JackThreadedDriver::Open(jack_nframes_t nframes,
                 jack_nframes_t samplerate,
                 bool capturing,
                 bool playing,
                 int inchannels,
                 int outchannels,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency)
{
    return fDriver->Open(nframes, samplerate, capturing, playing, inchannels, outchannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency);
}

int JackThreadedDriver::Close()
{
    return fDriver->Close();
}

int JackThreadedDriver::Process()
{
    return fDriver->Process();
}

int JackThreadedDriver::ProcessNull()
{
    return fDriver->ProcessNull();
}

int JackThreadedDriver::Attach()
{
    return fDriver->Attach();
}

int JackThreadedDriver::Detach()
{
    return fDriver->Detach();
}

int JackThreadedDriver::Read()
{
    return fDriver->Read();
}

int JackThreadedDriver::Write()
{
    return fDriver->Write();
}

int JackThreadedDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    return fDriver->SetBufferSize(buffer_size);
}

int JackThreadedDriver::SetSampleRate(jack_nframes_t sample_rate)
{
    return fDriver->SetSampleRate(sample_rate);
}

void JackThreadedDriver::SetMaster(bool onoff)
{
    fDriver->SetMaster(onoff);
}

bool JackThreadedDriver::GetMaster()
{
    return fDriver->GetMaster();
}

void JackThreadedDriver::AddSlave(JackDriverInterface* slave)
{
    fDriver->AddSlave(slave);
}

void JackThreadedDriver::RemoveSlave(JackDriverInterface* slave)
{
    fDriver->RemoveSlave(slave);
}

int JackThreadedDriver::ProcessSlaves()
{
    return fDriver->ProcessSlaves();
}

int JackThreadedDriver::ClientNotify(int refnum, const char* name, int notify, int sync, int value1, int value2)
{
    return fDriver->ClientNotify(refnum, name, notify, sync, value1, value2);
}

JackClientControl* JackThreadedDriver::GetClientControl() const
{
    return fDriver->GetClientControl();
}

bool JackThreadedDriver::IsRealTime() const
{
    return fDriver->IsRealTime();
}

int JackThreadedDriver::Start()
{
    jack_log("JackThreadedDriver::Start");

    if (fDriver->Start() < 0) {
        jack_error("Cannot start driver");
        return -1;
    }
    if (fThread.StartSync() < 0) {
        jack_error("Cannot start thread");
        return -1;
    }

    return 0;
}

int JackThreadedDriver::Stop()
{
    jack_log("JackThreadedDriver::Stop");
    
    switch (fThread.GetStatus()) {
            
        // Kill the thread in Init phase
        case JackThread::kStarting:
        case JackThread::kIniting:
            if (fThread.Kill() < 0) {
                jack_error("Cannot kill thread");
                return -1;
            }
            break;
           
        // Stop when the thread cycle is finished
        case JackThread::kRunning:
            if (fThread.Stop() < 0) {
                jack_error("Cannot stop thread"); 
                return -1;
            }
            break;
            
        default:
            break;
    }

    if (fDriver->Stop() < 0) {
        jack_error("Cannot stop driver");
        return -1;
    }
    return 0;
}

bool JackThreadedDriver::Execute()
{
    return (Process() == 0);
}

bool JackThreadedDriver::Init()
{
    if (fDriver->Init())  {
        if (fDriver->IsRealTime()) {
            jack_log("JackThreadedDriver::Init IsRealTime");
            // Will do "something" on OSX only...
            fThread.SetParams(GetEngineControl()->fPeriod, GetEngineControl()->fComputation, GetEngineControl()->fConstraint);
            if (fThread.AcquireRealTime(GetEngineControl()->fPriority) < 0) {
                jack_error("AcquireRealTime error");
            } else {
                set_threaded_log_function(); 
            }
        }
        return true;
    } else {
        return false;
    }
}

} // end of namespace
