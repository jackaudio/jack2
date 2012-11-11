/*
Copyright (C) 2010 Grame

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

#ifndef __TiPhoneCoreAudioRenderer__
#define __TiPhoneCoreAudioRenderer__

#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/AudioServices.h>
#include <AudioUnit/AudioUnit.h>

#define MAX_CHANNELS 256
#define OPEN_ERR -1
#define NO_ERR 0

typedef void (*AudioCallback) (int frames, float** inputs, float** outputs, void* arg);

class TiPhoneCoreAudioRenderer
{

    private:

		AudioUnit fAUHAL;
        AudioCallback fAudioCallback;
        void* fCallbackArg;
        
        int	fDevNumInChans;
        int	fDevNumOutChans;
        
        AudioBufferList* fCAInputData;
     
        float* fInChannel[MAX_CHANNELS];
        float* fOutChannel[MAX_CHANNELS];
	
		static OSStatus Render(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber,
                               UInt32 inNumberFrames,
                               AudioBufferList *ioData);
                               
        static void InterruptionListener(void *inClientData, UInt32 inInterruption);

    public:

        TiPhoneCoreAudioRenderer(int input, int output)
            :fAudioCallback(NULL), fCallbackArg(NULL), fDevNumInChans(input), fDevNumOutChans(output), fCAInputData(NULL)
        {
            memset(fInChannel, 0, sizeof(float*) * MAX_CHANNELS);
            memset(fOutChannel, 0, sizeof(float*) * MAX_CHANNELS);
            
            for (int i = 0; i < fDevNumInChans; i++) {
                fInChannel[i] = new float[8192];
            }
    
            for (int i = 0; i < fDevNumOutChans; i++) {
                fOutChannel[i] = new float[8192];
            }
        }
        
        virtual ~TiPhoneCoreAudioRenderer()
        {
            for (int i = 0; i < fDevNumInChans; i++) {
                delete[] fInChannel[i];
            }
    
            for (int i = 0; i < fDevNumOutChans; i++) {
                delete[] fOutChannel[i]; 
            }
            
            if (fCAInputData) {
                for (int i = 0; i < fDevNumInChans; i++) {
                    free(fCAInputData->mBuffers[i].mData);
                }
                free(fCAInputData);
            }
        }

        int Open(int bufferSize, int sampleRate);
        int Close();

        int Start();
        int Stop();
        
        void SetAudioCallback(AudioCallback callback, void* arg)
        {
            fAudioCallback = callback;
            fCallbackArg = arg;
        }
        
        void PerformAudioCallback(int frames)
        {
            if (fAudioCallback) {
                fAudioCallback(frames, fInChannel, fOutChannel, fCallbackArg);
            }
        }

};

typedef TiPhoneCoreAudioRenderer * TiPhoneCoreAudioRendererPtr;

#endif
