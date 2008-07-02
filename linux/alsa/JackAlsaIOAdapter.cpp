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
        //fAudioInterface.write();
        //fAudioInterface.write();
        fThread.AcquireRealTime();
jack_log("StartSync");
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

bool JackAlsaIOAdapter::Init()
{
    fAudioInterface.write();
    fAudioInterface.write();
    return true;
}
            
bool JackAlsaIOAdapter::Execute()
{
    if (fAudioInterface.read() < 0)
        return false;

    if (!fRunning) {
        fRunning = true;
        jack_time_t time = jack_get_time();
        fProducerDLL.Init(time);
        fConsumerDLL.Init(time);
    }

// DLL
    jack_time_t time = jack_get_time();
    fProducerDLL.IncFrame(time);
    jack_nframes_t time1 = fConsumerDLL.Time2Frames(time);
    jack_nframes_t time2 = fProducerDLL.Time2Frames(time);
     
    double src_ratio_output = double(time2) / double(time1);
    double src_ratio_input = double(time1) / double(time2);
   
    if (src_ratio_input < 0.7f || src_ratio_input > 1.3f) {
        jack_error("src_ratio_input = %f", src_ratio_input);
        src_ratio_input = 1;
        time1 = 1;
        time2 = 1;
    }
    
    if (src_ratio_output < 0.7f || src_ratio_output > 1.3f) {
        jack_error("src_ratio_output = %f", src_ratio_output);
        src_ratio_output = 1;
        time1 = 1;
        time2 = 1;
    }  
    
    src_ratio_input = Range(0.7f, 1.3f, src_ratio_input);
    src_ratio_output = Range(0.7f, 1.3f, src_ratio_output);
    jack_log("Callback resampler src_ratio_input = %f src_ratio_output = %f", src_ratio_input, src_ratio_output);
  
    for (int i = 0; i < fCaptureChannels; i++) {
        float* buffer = (float*)fAudioInterface.fInputSoftChannels[i];
        fCaptureRingBuffer[i]->SetRatio(time1, time2);
        fCaptureRingBuffer[i]->WriteResample(buffer, fBufferSize);
     }
    
    for (int i = 0; i < fPlaybackChannels; i++) {
        float* buffer = (float*)fAudioInterface.fOutputSoftChannels[i];
        fPlaybackRingBuffer[i]->SetRatio(time2, time1);
        fPlaybackRingBuffer[i]->ReadResample(buffer, fBufferSize); 
    }
        
    if (fAudioInterface.write() < 0)
        return false;
jack_log("execute");
    return true;
}

int JackAlsaIOAdapter::SetBufferSize(jack_nframes_t buffer_size)
{
    return 0;
}

        
}
