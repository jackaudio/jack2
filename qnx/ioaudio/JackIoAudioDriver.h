/*
 Copyright (C) 2001 Paul Davis
 Copyright (C) 2004 Grame

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

#ifndef __JackIoAudioDriver__
#define __JackIoAudioDriver__

#include "JackAudioDriver.h"
#include "JackThreadedDriver.h"
#include "JackTime.h"
#include "driver.h"
#include "memops.h"

typedef struct _ioaudio_driver_args
{
    char* device;

    bool capture;
    const char* capture_pcm_name;
    size_t user_capture_nchnls;

    bool playback;
    const char* playback_pcm_name;
    size_t user_playback_nchnls;

    size_t srate;
    size_t frames_per_interrupt;
    size_t user_nperiods;
    DitherAlgorithm dither;
    bool hw_monitoring;
    bool hw_metering;
    bool duplex;
    bool soft_mode;
    bool monitor;
    bool shorts_first;
    size_t systemic_input_latency;
    size_t systemic_output_latency;
    const char* midi_driver;
} ioaudio_driver_args_t;


namespace Jack
{

    struct ioaudio_driver_t;

    /*!
     \brief The IoAudio driver.
     */

    class JackIoAudioDriver: public JackAudioDriver
    {

    public:

        JackIoAudioDriver(
            const char* name,
            const char* alias,
            JackLockedEngine* engine,
            JackSynchro* table ) :
                JackAudioDriver( name,
                                 alias,
                                 engine,
                                 table ),
                fDriver( NULL )
        {
        }

        virtual ~JackIoAudioDriver()
        {
        }

        int Open(
            ioaudio_driver_args_t args );

        int Close();
        int Attach();
        int Detach();

        int Start();
        int Stop();

        int Read();
        int Write();

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
        return false;
        }

        int SetBufferSize(
            jack_nframes_t buffer_size );

        void ReadInputAux(
            jack_nframes_t orig_nframes,
            ssize_t contiguous,
            ssize_t nread );
        void MonitorInputAux();
        void ClearOutputAux();
        void WriteOutputAux(
            jack_nframes_t orig_nframes,
            ssize_t contiguous,
            ssize_t nwritten );
//        void SetTimetAux(
//            jack_time_t time );

    private:
        ioaudio_driver_args_t fArgs;
        ioaudio_driver_t* fDriver;

        void UpdateLatencies();

        ssize_t capture_sample_bytes();
        ssize_t capture_frame_bytes();
        ssize_t capture_frag_bytes();
        ssize_t capture_frag_frames();

        ssize_t playback_sample_bytes();
        ssize_t playback_frame_bytes();
        ssize_t playback_frag_bytes();
        ssize_t playback_frag_frames();

        void
        create(
            char *name,
            jack_client_t *client );

        void
        destroy();

        int
        start();

        int
        stop();

        jack_nframes_t
        wait(
            int *status,
            float *delayed_usecs );

        int
        read(
            jack_nframes_t nframes );

        void silence_untouched_channels(
            jack_nframes_t nframes );

        int
        reset_parameters();

        void mark_channel_done(
            int chn );

        void silence_on_channel(
            int chn,
            jack_nframes_t nframes );

        void silence_on_channel_no_mark(
            int chn,
            jack_nframes_t nframes );

//        void read_from_channel(
//            int chn,
//            jack_default_audio_sample_t *buf,
//            jack_nframes_t nsamples );

//        void write_to_channel(
//            int chn,
//            jack_default_audio_sample_t *buf,
//            jack_nframes_t nsamples );

        void release_channel_dependent_memory();

        int check_capabilities(
            const char *devicename,
            int mode );

        int check_card_type();

        int generic_hardware();

        int hw_specific();

        void setup_io_function_pointers();

        int configure_stream(
            const char *device_name,
            const char *stream_name,
            snd_pcm_t *handle,
            snd_pcm_channel_params_t *params,
            unsigned int *nperiodsp );

        int set_parameters();

        int get_channel_addresses(
            size_t *capture_avail,
            size_t *playback_avail,
            size_t *capture_offset,
            size_t *playback_offset );

        int restart();

        int xrun_recovery(
            float *delayed_usecs );
    };

} // end of namespace

#endif
