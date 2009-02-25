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

#include "JackAudioAdapter.h"
#include "JackTime.h"  
#include <stdio.h>

namespace Jack
{

#ifdef JACK_MONITOR

    void MeasureTable::Write(int time1, int time2, float r1, float r2, int pos1, int pos2)
    {
        int pos = (++fCount) % TABLE_MAX;
        fTable[pos].time1 = time1;
        fTable[pos].time2 = time2;
        fTable[pos].r1 = r1;
        fTable[pos].r2 = r2;
        fTable[pos].pos1 = pos1;
        fTable[pos].pos2 = pos2;
    }

    void MeasureTable::Save()
    {
        char buffer[1024];
        FILE* file = fopen("JackAudioAdapter.log", "w");

        int max = (fCount) % TABLE_MAX - 1;
        for (int i = 1; i < max; i++)
        {
            fprintf(file, "%d \t %d \t %d  \t %f \t %f \t %d \t %d \n",
                    fTable[i].delta, fTable[i+1].time1 - fTable[i].time1,
                    fTable[i+1].time2 - fTable[i].time2,
                    fTable[i].r1, fTable[i].r2, fTable[i].pos1, fTable[i].pos2);
        }
        fclose(file);

        // Adapter timing 1
        file = fopen("AdapterTiming1.plot", "w");
        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"frames\"\n");
        fprintf(file, "plot ");
        sprintf(buffer, "\"JackAudioAdapter.log\" using 2 title \"Consumer interrupt period\" with lines,");
        fprintf(file, buffer);
        sprintf(buffer, "\"JackAudioAdapter.log\" using 3 title \"Producer interrupt period\" with lines");
        fprintf(file, buffer);
        fclose(file);

        // Adapter timing 2
        file = fopen("AdapterTiming2.plot", "w");
        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"resampling ratio\"\n");
        fprintf(file, "plot ");
        sprintf(buffer, "\"JackAudioAdapter.log\" using 4 title \"Ratio 1\" with lines,");
        fprintf(file, buffer);
        sprintf(buffer, "\"JackAudioAdapter.log\" using 5 title \"Ratio 2\" with lines");
        fprintf(file, buffer);
        fclose(file);

        // Adapter timing 3
        file = fopen("AdapterTiming3.plot", "w");
        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"frames\"\n");
        fprintf(file, "plot ");
        sprintf(buffer, "\"JackAudioAdapter.log\" using 6 title \"Frames position in consumer ringbuffer\" with lines,");
        fprintf(file, buffer);
        sprintf(buffer, "\"JackAudioAdapter.log\" using 7 title \"Frames position in producer ringbuffer\" with lines");
        fprintf(file, buffer);
        fclose(file);
    }

#endif

    void JackAudioAdapterInterface::ResetRingBuffers()
    {
        int i;
        for (i = 0; i < fCaptureChannels; i++)
            fCaptureRingBuffer[i]->Reset();
        for (i = 0; i < fPlaybackChannels; i++)
            fPlaybackRingBuffer[i]->Reset();
    }

    void JackAudioAdapterInterface::ResampleFactor ( jack_time_t& frame1, jack_time_t& frame2 )
    {
        jack_time_t time = GetMicroSeconds();

        if ( !fRunning )
        {
            // Init DLL
            fRunning = true;
            fHostDLL.Init ( time );
            fAdaptedDLL.Init ( time );
            frame1 = 1;
            frame2 = 1;
        }
        else
        {
            // DLL
            fAdaptedDLL.IncFrame(time);
            jack_nframes_t time1 = fHostDLL.Time2Frames(time);
            jack_nframes_t time2 = fAdaptedDLL.Time2Frames(time);
            frame1 = time1;
            frame2 = time2;
            jack_log("JackAudioAdapterInterface::ResampleFactor time1 = %ld time2 = %ld src_ratio_input = %f src_ratio_output = %f",
                     long(time1), long(time2), double(time1) / double(time2), double(time2) / double(time1));
        }
    }

    int JackAudioAdapterInterface::Open()
    {
        return 0;
    }

    int JackAudioAdapterInterface::Close()
    {
        return 0;
    }
    
    void JackAudioAdapterInterface::PushAndPull(float** inputBuffer, float** outputBuffer, unsigned int inNumberFrames)
    {
        bool failure = false;

        jack_time_t time1, time2;
        ResampleFactor(time1, time2);

        for (int i = 0; i < fCaptureChannels; i++) {
            fCaptureRingBuffer[i]->SetRatio(time1, time2);
            if (fCaptureRingBuffer[i]->WriteResample(inputBuffer[i], inNumberFrames) < inNumberFrames)
                failure = true;
        }

        for (int i = 0; i < fPlaybackChannels; i++) {
            fPlaybackRingBuffer[i]->SetRatio(time2, time1);
            if (fPlaybackRingBuffer[i]->ReadResample(outputBuffer[i], inNumberFrames) < inNumberFrames)
                 failure = true;
        }

    #ifdef JACK_MONITOR
        fTable.Write(time1, time2, double(time1) / double(time2), double(time2) / double(time1),
             fCaptureRingBuffer[0]->ReadSpace(), fPlaybackRingBuffer[0]->WriteSpace());
    #endif

        // Reset all ringbuffers in case of failure
        if (failure) {
            jack_error("JackAudioAdapterInterface::PushAndPull ringbuffer failure... reset");
            ResetRingBuffers();
        }
    }



} // namespace
