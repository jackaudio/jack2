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

#ifndef __JackIoAudioAdapter__
#define __JackIoAudioAdapter__

#include <cmath>
#include <climits>
#include <cassert>
#include <sys/asoundlib.h>
#include "JackAudioAdapterInterface.h"
#include "JackPlatformPlug.h"
#include "JackError.h"
#include "jack.h"
#include "jslist.h"

namespace Jack
    {

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))
#define NUM_BUFFERS 256

    /**
     * A convenient class to pass parameters to AudioInterface
     */
    class AudioParam
        {
    public:
        const char* fInputCardName;
        const char* fOutputCardName;
        unsigned int fFrequency;
        int fBuffering;

        unsigned int fCardInputVoices;
        unsigned int fCardOutputVoices;

        unsigned int fPeriod;

    public:
        AudioParam(
                jack_nframes_t buffer_size = 512,
                jack_nframes_t sample_rate = 44100,
                const char* input_card = "pcmPreferredc",
                int input_ports = 2,
                const char* output_card = "pcmPreferredp",
                int output_ports = 2,
                int periods = 2) :
                        fInputCardName(input_card),
                        fOutputCardName(output_card),
                        fFrequency(sample_rate),
                        fBuffering(buffer_size),
                        fCardInputVoices(input_ports),
                        fCardOutputVoices(output_ports),
                        fPeriod(periods)
            {
            }

        AudioParam& inputCardName(const char* n)
            {
            fInputCardName = n;
            return *this;
            }

        AudioParam& outputCardName(const char* n)
            {
            fOutputCardName = n;
            return *this;
            }

        AudioParam& frequency(int f)
            {
            fFrequency = f;
            return *this;
            }

        AudioParam& buffering(int fpb)
            {
            fBuffering = fpb;
            return *this;
            }

        void setInputs(int inputs)
            {
            fCardInputVoices = inputs;
            }

        AudioParam& inputs(int n)
            {
            fCardInputVoices = n;
            return *this;
            }

        void setOutputs(int outputs)
            {
            fCardOutputVoices = outputs;
            }

        AudioParam& outputs(int n)
            {
            fCardOutputVoices = n;
            return *this;
            }
        };

    /**
     * An io-audio client interface
     */
    class AudioInterface
        {
    public:
        AudioParam fParams;

        //device info
        snd_pcm_t* fOutputDevice;
        snd_pcm_t* fInputDevice;
        snd_pcm_channel_params_t *fInputParams;
        snd_pcm_channel_params_t *fOutputParams;
        snd_pcm_channel_setup_t *fInputSetup;
        snd_pcm_channel_setup_t *fOutputSetup;

        //samples info
        snd_pcm_format_t fInputFormat;
        snd_pcm_format_t fOutputFormat;

        unsigned int fNumInputPorts;
        unsigned int fNumOutputPorts;

        //Number of frames for one voice/port
        jack_nframes_t fInputBufferFrames;
        jack_nframes_t fOutputBufferFrames;

        // audiocard buffers
        void* fInputCardBuffer;
        void* fOutputCardBuffer;

        // floating point JACK buffers
        jack_default_audio_sample_t** fJackInputBuffers;
        jack_default_audio_sample_t** fJackOutputBuffers;

        //public methods ---------------------------------------------------------

        AudioInterface(const AudioParam& ap = AudioParam());

        AudioInterface(jack_nframes_t buffer_size, jack_nframes_t sample_rate);

        ~AudioInterface();

        /**
         * Open the audio interface
         */
        int open();

        int close();

        int setAudioParams(snd_pcm_t* stream, snd_pcm_channel_params_t* params);

        ssize_t interleavedBufferSize(snd_pcm_channel_params_t* params);

        ssize_t noninterleavedBufferSize(snd_pcm_channel_params_t* params);

        /**
         * Read audio samples from the audio card. Convert samples to floats and take
         * care of interleaved buffers
         */
        int read();

        /**
         * write the output soft channels to the audio card. Convert sample
         * format and interleaves buffers when needed
         */
        int write();

        /**
         *  print short information on the audio device
         */
        int shortinfo();

        /**
         *  print more detailled information on the audio device
         */
        int longinfo();

        void printCardInfo(snd_ctl_hw_info_t* ci);

        void printHWParams(snd_pcm_channel_params_t* params);

    private:
        int AudioInterface_common();
        };

    /*!
     \brief Audio adapter using io-audio API.
     */

    class JackIoAudioAdapter: public JackAudioAdapterInterface,
            public JackRunnableInterface
        {

    private:
        JackThread fThread;
        AudioInterface fAudioInterface;

    public:
        JackIoAudioAdapter(
                jack_nframes_t buffer_size,
                jack_nframes_t sample_rate,
                const JSList* params);
        ~JackIoAudioAdapter()
            {
            }

        virtual int Open();
        virtual int Close();

        virtual void Create();
        virtual void Destroy();

        virtual int SetSampleRate(jack_nframes_t sample_rate);
        virtual int SetBufferSize(jack_nframes_t buffer_size);

        virtual bool Init();
        virtual bool Execute();

        };

    }

#ifdef __cplusplus
extern "C"
    {
#endif
#include "JackCompilerDeps.h"
#include "driver_interface.h"
    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor();

#ifdef __cplusplus
    }
#endif

#endif
