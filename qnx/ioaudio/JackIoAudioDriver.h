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
#include <sys/poll.h>

namespace Jack
{

///////////////////////////////////////////////////////////////////////////////
// CONSTANTS
///////////////////////////////////////////////////////////////////////////////

    enum IoAudioDriverChannels
    {
        Playback = SND_PCM_CHANNEL_PLAYBACK,
        Capture = SND_PCM_CHANNEL_CAPTURE,
        Extra = SND_PCM_CHANNEL_MAX,
        IoAudioDriverChannels_COUNT
    };

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

    /*!
     \brief The IoAudio driver.
     */

    class JackIoAudioDriver: public JackAudioDriver
    {
    public:
        typedef void (*ReadCopyFunction)(
            jack_default_audio_sample_t *dst,
            char *src,
            unsigned long src_bytes,
            unsigned long src_skip_bytes );
        typedef void (*WriteCopyFunction)(
            char *dst,
            jack_default_audio_sample_t *src,
            unsigned long src_bytes,
            unsigned long dst_skip_bytes,
            dither_state_t *state );

        struct Args
        {
            const char* jack_name;
            const char* jack_alias;

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
        };

        struct Voice
        {
            unsigned long interleave_skip;
            char* addr;
            unsigned long silent;
            dither_state_t dither_state;
        };

        struct Channel
        {
            Voice* voices;
            unsigned int nperiods;
            snd_pcm_t *handle;
            snd_pcm_channel_params_t params;
            snd_pcm_channel_setup_t setup;
            snd_pcm_mmap_control_t *mmap;
            void *mmap_buf;
            void *buffer;

            ReadCopyFunction read;
            WriteCopyFunction write;

            ssize_t sample_bytes()
            {
            return snd_pcm_format_size( setup.format.format,
                                        1 );
            }
            ssize_t frame_bytes()
            {
            return snd_pcm_format_size( setup.format.format,
                                        setup.format.voices );
            }

            ssize_t frag_bytes()
            {
            return setup.buf.block.frag_size;
            }

            ssize_t frag_frames()
            {
            return frag_bytes() / frame_bytes();
            }

        };

    public:

        JackIoAudioDriver(
            Args args,
            JackLockedEngine* engine,
            JackSynchro* table );

        virtual ~JackIoAudioDriver();

        int Attach();

        int Close();

        int Detach();

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
        return false;
        }

        int Open();

        int Read();

        int SetBufferSize(
            jack_nframes_t buffer_size );

        int Start();

        int Stop();

        int Write();

    private:
        Args fArgs;

        int poll_timeout_msecs;
        jack_time_t poll_last;
        jack_time_t poll_next;
        int poll_late;

        struct pollfd pfd[IoAudioDriverChannels_COUNT];
        unsigned long interleave_unit;
        unsigned int max_nchannels;
        unsigned int user_nchannels;

        jack_nframes_t frame_rate;

        Channel playback;

        Channel capture;

        jack_hardware_t *hw;

        bool capture_and_playback_not_synced;
        bool has_clock_sync_reporting;

        bool has_hw_monitoring;
        bool do_hw_monitoring;
        unsigned long input_monitor_mask;

        bool has_hw_metering;
        bool do_hw_metering;
        bool quirk_bswap;

        int xrun_count;
        int process_count;

        //ioaudio_midi_t *midi;
        //char *midi;
        int in_xrun_recovery;

        int check_capabilities(
            const char *devicename,
            int mode );

        int check_card_type();

        void clear_output_aux();

        int configure_stream(
            const char *device_name,
            const char *stream_name,
            snd_pcm_t *handle,
            snd_pcm_channel_params_t *params,
            unsigned int *nperiodsp );

        int create(
            jack_client_t *client );

        int generic_hardware();

        int get_channel_addresses(
            size_t *capture_avail,
            size_t *playback_avail,
            size_t *capture_offset,
            size_t *playback_offset );

        int hw_specific();

        void monitor_input_aux();

        int read(
            jack_nframes_t nframes );

        void read_input_aux(
            jack_nframes_t orig_nframes,
            ssize_t contiguous,
            ssize_t nread );

        int set_parameters();

        void setup_io_function_pointers();

        void update_latencies();

        jack_nframes_t wait(
            int *status,
            float *delayed_usecs );

        void write_output_aux(
            jack_nframes_t orig_nframes,
            ssize_t contiguous,
            ssize_t nwritten );

        int xrun_recovery(
            float *delayed_usecs );
    };

} // end of namespace

#endif
