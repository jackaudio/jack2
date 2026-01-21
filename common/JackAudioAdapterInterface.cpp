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

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#include "JackAudioAdapter.h"
#ifndef MY_TARGET_OS_IPHONE
#include "JackLibSampleRateResampler.h"
#endif
#include "JackTime.h"
#include "JackError.h"
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

    void MeasureTable::Save(unsigned int fHostBufferSize, unsigned int fHostSampleRate, unsigned int fAdaptedSampleRate, unsigned int fAdaptedBufferSize)
    {
        FILE* file = fopen("JackAudioAdapter.log", "w");

        int max = (fCount) % TABLE_MAX - 1;
        for (int i = 1; i < max; i++) {
            fprintf(file, "%d \t %d \t %d  \t %f \t %f \t %d \t %d \n",
                    fTable[i].delta, fTable[i].time1, fTable[i].time2,
                    fTable[i].r1, fTable[i].r2, fTable[i].pos1, fTable[i].pos2);
        }
        fclose(file);

        // No used for now
        // Adapter timing 1
        file = fopen("AdapterTiming1.plot", "w");
        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing: host [rate = %.1f kHz buffer = %d frames] adapter [rate = %.1f kHz buffer = %d frames] \"\n"
            ,float(fHostSampleRate)/1000.f, fHostBufferSize, float(fAdaptedSampleRate)/1000.f, fAdaptedBufferSize);
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"frames\"\n");
        fprintf(file, "plot ");
        fprintf(file, "\"JackAudioAdapter.log\" using 2 title \"Ringbuffer error\" with dots,");
        fprintf(file, "\"JackAudioAdapter.log\" using 3 title \"Ringbuffer error with timing correction\" with dots");

        fprintf(file, "\n unset multiplot\n");
        fprintf(file, "set output 'AdapterTiming1.svg\n");
        fprintf(file, "set terminal svg\n");

        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing: host [rate = %.1f kHz buffer = %d frames] adapter [rate = %.1f kHz buffer = %d frames] \"\n"
            ,float(fHostSampleRate)/1000.f, fHostBufferSize, float(fAdaptedSampleRate)/1000.f, fAdaptedBufferSize);
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"frames\"\n");
        fprintf(file, "plot ");
        fprintf(file, "\"JackAudioAdapter.log\" using 2 title \"Consumer interrupt period\" with dots,");
        fprintf(file, "\"JackAudioAdapter.log\" using 3 title \"Producer interrupt period\" with dots\n");
        fprintf(file, "unset multiplot\n");
        fprintf(file, "unset output\n");

        fclose(file);

        // Adapter timing 2
        file = fopen("AdapterTiming2.plot", "w");
        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing: host [rate = %.1f kHz buffer = %d frames] adapter [rate = %.1f kHz buffer = %d frames] \"\n"
            ,float(fHostSampleRate)/1000.f, fHostBufferSize, float(fAdaptedSampleRate)/1000.f, fAdaptedBufferSize);
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"resampling ratio\"\n");
        fprintf(file, "plot ");
        fprintf(file, "\"JackAudioAdapter.log\" using 4 title \"Ratio 1\" with dots,");
        fprintf(file, "\"JackAudioAdapter.log\" using 5 title \"Ratio 2\" with dots");

        fprintf(file, "\n unset multiplot\n");
        fprintf(file, "set output 'AdapterTiming2.svg\n");
        fprintf(file, "set terminal svg\n");

        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing: host [rate = %.1f kHz buffer = %d frames] adapter [rate = %.1f kHz buffer = %d frames] \"\n"
            ,float(fHostSampleRate)/1000.f, fHostBufferSize, float(fAdaptedSampleRate)/1000.f, fAdaptedBufferSize);
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"resampling ratio\"\n");
        fprintf(file, "plot ");
        fprintf(file, "\"JackAudioAdapter.log\" using 4 title \"Ratio 1\" with dots,");
        fprintf(file, "\"JackAudioAdapter.log\" using 5 title \"Ratio 2\" with dots\n");
        fprintf(file, "unset multiplot\n");
        fprintf(file, "unset output\n");

        fclose(file);

        // Adapter timing 3
        file = fopen("AdapterTiming3.plot", "w");
        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing: host [rate = %.1f kHz buffer = %d frames] adapter [rate = %.1f kHz buffer = %d frames] \"\n"
            ,float(fHostSampleRate)/1000.f, fHostBufferSize, float(fAdaptedSampleRate)/1000.f, fAdaptedBufferSize);
         fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"frames\"\n");
        fprintf(file, "plot ");
        fprintf(file, "\"JackAudioAdapter.log\" using 6 title \"Frames position in consumer ringbuffer\" with dots,");
        fprintf(file, "\"JackAudioAdapter.log\" using 7 title \"Frames position in producer ringbuffer\" with dots");

        fprintf(file, "\n unset multiplot\n");
        fprintf(file, "set output 'AdapterTiming3.svg\n");
        fprintf(file, "set terminal svg\n");

        fprintf(file, "set multiplot\n");
        fprintf(file, "set grid\n");
        fprintf(file, "set title \"Audio adapter timing: host [rate = %.1f kHz buffer = %d frames] adapter [rate = %.1f kHz buffer = %d frames] \"\n"
            ,float(fHostSampleRate)/1000.f, fHostBufferSize, float(fAdaptedSampleRate)/1000.f, fAdaptedBufferSize);
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"frames\"\n");
        fprintf(file, "plot ");
        fprintf(file, "\"JackAudioAdapter.log\" using 6 title \"Frames position in consumer ringbuffer\" with dots,");
        fprintf(file, "\"JackAudioAdapter.log\" using 7 title \"Frames position in producer ringbuffer\" with dots\n");
        fprintf(file, "unset multiplot\n");
        fprintf(file, "unset output\n");

        fclose(file);
    }

#endif

    void JackAudioAdapterInterface::GrowRingBufferSize()
    {
        fRingbufferCurSize *= 2;
    }

    void JackAudioAdapterInterface::AdaptRingBufferSize()
    {
        if (fHostBufferSize > fAdaptedBufferSize) {
            fRingbufferCurSize = 4 * fHostBufferSize;
        } else {
            fRingbufferCurSize = 4 * fAdaptedBufferSize;
        }
    }

    void JackAudioAdapterInterface::ResetRingBuffers()
    {
        if (fRingbufferCurSize > DEFAULT_RB_SIZE) {
            fRingbufferCurSize = DEFAULT_RB_SIZE;
        }

        for (int i = 0; i < fCaptureChannels; i++) {
            fCaptureRingBuffer[i]->Reset(fRingbufferCurSize);
            fPIControllerCapture[i]->Reset();
        }
        for (int i = 0; i < fPlaybackChannels; i++) {
            fPlaybackRingBuffer[i]->Reset(fRingbufferCurSize);
            fPIControllerPlayback[i]->Reset();
        }
    }

    void JackAudioAdapterInterface::Reset()
    {
        ResetRingBuffers();
        fRunning = false;
    }

#ifdef MY_TARGET_OS_IPHONE
    void JackAudioAdapterInterface::Create()
    {}
#else
    void JackAudioAdapterInterface::Create()
    {
        fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
        fPIControllerCapture = new JackPIController*[fCaptureChannels];

        fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
        fPIControllerPlayback = new JackPIController*[fPlaybackChannels];

        if (fAdaptative) {
            AdaptRingBufferSize();
            jack_info("Ringbuffer automatic adaptative mode size = %d frames", fRingbufferCurSize);
        } else {
            if (fRingbufferCurSize > DEFAULT_RB_SIZE) {
                fRingbufferCurSize = DEFAULT_RB_SIZE;
            }
            jack_info("Fixed ringbuffer size = %d frames", fRingbufferCurSize);
        }

        if (fResampleRatioPerChannel){
            jack_info("Resample ratio tracked independently for each capture and playback channel");
        } else {
            jack_info("Shared resample ratio tracking for all capture and playback channels");
        }


        for (int i = 0; i < fCaptureChannels; i++ ) {
            fCaptureRingBuffer[i] = new JackLibSampleRateResampler(fQuality);
            fCaptureRingBuffer[i]->Reset(fRingbufferCurSize);
            fPIControllerCapture[i] = new JackPIController(double(fHostSampleRate) / double(fAdaptedSampleRate));
        }
        for (int i = 0; i < fPlaybackChannels; i++ ) {
            fPlaybackRingBuffer[i] = new JackLibSampleRateResampler(fQuality);
            fPlaybackRingBuffer[i]->Reset(fRingbufferCurSize);
            fPIControllerPlayback[i] = new JackPIController(double(fHostSampleRate) / double(fAdaptedSampleRate));
        }

        if (fCaptureChannels > 0) {
            jack_log("ReadSpace = %ld", fCaptureRingBuffer[0]->ReadSpace());
        }
        if (fPlaybackChannels > 0) {
            jack_log("WriteSpace = %ld", fPlaybackRingBuffer[0]->WriteSpace());
        }
    }
#endif

    void JackAudioAdapterInterface::Destroy()
    {
        for (int i = 0; i < fCaptureChannels; i++) {
            delete(fCaptureRingBuffer[i]);
            delete(fPIControllerCapture[i]);
        }
        for (int i = 0; i < fPlaybackChannels; i++) {
            delete (fPlaybackRingBuffer[i]);
            delete(fPIControllerPlayback[i]);
        }

        delete[] fCaptureRingBuffer;
        delete [] fPIControllerCapture;
        delete[] fPlaybackRingBuffer;
        delete [] fPIControllerPlayback;
    }

    double JackAudioAdapterInterface::GetSharedRatio(int delta_frames)
    {
        if (0 < fCaptureChannels) {
            return fPIControllerCapture[0]->GetRatio(fCaptureRingBuffer[0]->GetReadBalance() - delta_frames);
        }
        else if (0 < fPlaybackChannels) {
            return fPIControllerPlayback[0]->GetRatio(fPlaybackRingBuffer[0]->GetReadBalance() - delta_frames);
        }
        return 1;
    }

    int JackAudioAdapterInterface::PushAndPull(float** inputBuffer, float** outputBuffer, unsigned int frames)
    {
        // Most of the time we can use a shared ratio calculated on a single input or output channel ringbuffer and apply this ratio also to all other channels.
        // In some situations we see that there is a drift between the channels coming from or going to the same device.
        // On those devices we need to activate the fResampleRatioPerChannel option so that every channel will calculate and keep track of its own resample ratio
        // in order to keep his ringbuffer in a good balance which is half used for reading and half used for writing.
        // Not using this option when it is needed will result in buffer errors followed by resets for all the buffers.
        // cfr. https://github.com/jackaudio/jack2/issues/534

        bool failure = false;
        fRunning = true;

        // Finer estimation of the position in the ringbuffer
        int delta_frames = (fPullAndPushTime > 0) ? (int)((float(long(GetMicroSeconds() - fPullAndPushTime)) * float(fAdaptedSampleRate)) / 1000000.f) : 0;

        double ratioC[fCaptureChannels];
        double ratioP[fPlaybackChannels];
        if(!fResampleRatioPerChannel) {
            double shared_ratio = GetSharedRatio(delta_frames);
            for (int i = 0; i < fCaptureChannels; i++) {
                ratioC[i] = shared_ratio;
            }
            for (int i = 0; i < fPlaybackChannels; i++) {
                ratioP[i] = shared_ratio;
            }
#ifdef JACK_MONITOR
            if (fCaptureRingBuffer && fCaptureRingBuffer[0] != NULL) {
                fTable.Write(fCaptureRingBuffer[0]->GetReadBalance(), fCaptureRingBuffer[0]->GetReadBalance() - delta_frames, shared_ratio, 1/shared_ratio, fCaptureRingBuffer[0]->ReadSpace(), fCaptureRingBuffer[0]->ReadSpace());
            }
#endif // JACK_MONITOR
        }
        else {
            for (int i = 0; i < fCaptureChannels; i++) {
                ratioC[i] = fPIControllerCapture[i]->GetRatio(fCaptureRingBuffer[i]->GetReadBalance() - delta_frames);
            }
            for (int i = 0; i < fPlaybackChannels; i++) {
                ratioP[i] = fPIControllerPlayback[i]->GetRatio(fPlaybackRingBuffer[i]->GetWriteBalance() + delta_frames);
            }
#ifdef JACK_MONITOR
            if (fCaptureRingBuffer && fCaptureRingBuffer[0] != NULL) {
                fTable.Write(fCaptureRingBuffer[0]->GetReadBalance(), fCaptureRingBuffer[0]->GetReadBalance() - delta_frames, ratioC[0], 1/ratioC[0], fCaptureRingBuffer[0]->ReadSpace(), fCaptureRingBuffer[0]->ReadSpace());
            }
#endif // JACK_MONITOR
        }


        // Push/pull from ringbuffer
        for (int i = 0; i < fCaptureChannels; i++) {
            fCaptureRingBuffer[i]->SetRatio(ratioC[i]);
            if (inputBuffer[i]) {
                if (fCaptureRingBuffer[i]->WriteResample(inputBuffer[i], frames) < frames) {
                    failure = true;
                }
            }
        }

        for (int i = 0; i < fPlaybackChannels; i++) {
            fPlaybackRingBuffer[i]->SetRatio(1 / ratioP[i]);
            if (outputBuffer[i]) {
                if (fPlaybackRingBuffer[i]->ReadResample(outputBuffer[i], frames) < frames) {
                     failure = true;
                }
            }
        }
        // Reset all ringbuffers in case of failure
        if (failure) {
            jack_error("JackAudioAdapterInterface::PushAndPull ringbuffer failure... reset");
            if (fAdaptative) {
                GrowRingBufferSize();
                jack_info("Ringbuffer size = %d frames", fRingbufferCurSize);
            }
            ResetRingBuffers();
            return -1;
        } else {
            return 0;
        }
    }

    int JackAudioAdapterInterface::PullAndPush(float** inputBuffer, float** outputBuffer, unsigned int frames)
    {
        fPullAndPushTime = GetMicroSeconds();
        if (!fRunning) {
            return 0;
        }

        int res = 0;

        // Push/pull from ringbuffer
        for (int i = 0; i < fCaptureChannels; i++) {
            if (inputBuffer[i]) {
                if (fCaptureRingBuffer[i]->Read(inputBuffer[i], frames) < frames) {
                    res = -1;
                }
            }
        }

        for (int i = 0; i < fPlaybackChannels; i++) {
            if (outputBuffer[i]) {
                if (fPlaybackRingBuffer[i]->Write(outputBuffer[i], frames) < frames) {
                    res = -1;
                }
            }
        }

        return res;
    }

    int JackAudioAdapterInterface::SetHostBufferSize(jack_nframes_t buffer_size)
    {
        fHostBufferSize = buffer_size;
        if (fAdaptative) {
            AdaptRingBufferSize();
        }
        return 0;
    }

    int JackAudioAdapterInterface::SetAdaptedBufferSize(jack_nframes_t buffer_size)
    {
        fAdaptedBufferSize = buffer_size;
        if (fAdaptative) {
            AdaptRingBufferSize();
        }
        return 0;
    }

    int JackAudioAdapterInterface::SetBufferSize(jack_nframes_t buffer_size)
    {
        SetHostBufferSize(buffer_size);
        SetAdaptedBufferSize(buffer_size);
        return 0;
    }

    int JackAudioAdapterInterface::SetHostSampleRate(jack_nframes_t sample_rate)
    {
        fHostSampleRate = sample_rate;
        if (NULL != fPIControllerCapture) {
            for (int i = 0; i < fCaptureChannels; i++) {
                fPIControllerCapture[i]->Init(double(fHostSampleRate) / double(fAdaptedSampleRate));
            }
        }
        if (NULL != fPIControllerPlayback) {
            for (int i = 0; i < fPlaybackChannels; i++) {
                fPIControllerPlayback[i]->Init(double(fHostSampleRate) / double(fAdaptedSampleRate));
            }
        }
        return 0;
    }

    int JackAudioAdapterInterface::SetAdaptedSampleRate(jack_nframes_t sample_rate)
    {
        fAdaptedSampleRate = sample_rate;
        if (NULL != fPIControllerCapture) {
            for (int i = 0; i < fCaptureChannels; i++) {
                fPIControllerCapture[i]->Init(double(fHostSampleRate) / double(fAdaptedSampleRate));
            }
        }
        if (NULL != fPIControllerPlayback) {
            for (int i = 0; i < fPlaybackChannels; i++ ) {
                fPIControllerPlayback[i]->Init(double(fHostSampleRate) / double(fAdaptedSampleRate));
            }
        }
        return 0;
    }

    int JackAudioAdapterInterface::SetSampleRate(jack_nframes_t sample_rate)
    {
        SetHostSampleRate(sample_rate);
        SetAdaptedSampleRate(sample_rate);
        return 0;
    }

    void JackAudioAdapterInterface::SetInputs(int inputs)
    {
        jack_log("JackAudioAdapterInterface::SetInputs %d", inputs);
        fCaptureChannels = inputs;
    }

    void JackAudioAdapterInterface::SetOutputs(int outputs)
    {
        jack_log("JackAudioAdapterInterface::SetOutputs %d", outputs);
        fPlaybackChannels = outputs;
    }

    int JackAudioAdapterInterface::GetInputs()
    {
        //jack_log("JackAudioAdapterInterface::GetInputs %d", fCaptureChannels);
        return fCaptureChannels;
    }

    int JackAudioAdapterInterface::GetOutputs()
    {
        //jack_log ("JackAudioAdapterInterface::GetOutputs %d", fPlaybackChannels);
        return fPlaybackChannels;
    }


} // namespace
