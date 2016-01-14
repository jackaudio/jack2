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

#define __STDC_FORMAT_MACROS   // For inttypes.h to work in C++
#define _GNU_SOURCE            /* for strcasestr() from string.h */

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <limits>

#include <sys/asoundlib.h>
#include <sys/poll.h>
#include "bitset.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LE 0
#define IS_BE 1
#elif __BYTE_ORDER == __BIG_ENDIAN
#define IS_LE 1
#define IS_BE 0
#endif

#include "types.h"
#include "hardware.h"
//#include "driver.h"
#include "memops.h"
//#include "ioaudio_midi.h"

#include <iostream>

#include "JackIoAudioDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackLockedEngine.h"
#include "JackPosixThread.h"
#include "JackCompilerDeps.h"
#include "JackServerGlobals.h"
#include "JackTime.h"

#include "JackError.h"

static struct jack_constraint_enum_str_descriptor midi_constraint_descr_array[] =
    { { "none", "no MIDI driver" }, { "seq", "io-audio Sequencer driver" }, {
        "raw", "io-audio RawMIDI driver" },
      { 0 } };

static struct jack_constraint_enum_char_descriptor dither_constraint_descr_array[] =
    { { 'n', "none" }, { 'r', "rectangular" }, { 's', "shaped" },
      { 't', "triangular" }, { 0 } };

namespace Jack
{

///////////////////////////////////////////////////////////////////////////////
// CONSTANTS
///////////////////////////////////////////////////////////////////////////////

#undef DEBUG_WAKEUP

    /* Delay (in process calls) before jackd will report an xrun */
#define XRUN_REPORT_DELAY 0

//    const char driver_client_name[] = "ioaudio_pcm";

    enum IoAudioDriverChannels
    {
        Playback = SND_PCM_CHANNEL_PLAYBACK,
        Capture = SND_PCM_CHANNEL_CAPTURE,
        Extra = SND_PCM_CHANNEL_MAX,
        IoAudioDriverChannels_COUNT
    };

//static Jack::JackIoAudioDriver* g_ioaudio_driver;

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////
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

    struct ioaudio_driver_t
    {

        //JACK_DRIVER_NT_DECL

        int poll_timeout_msecs;
        jack_time_t poll_last;
        jack_time_t poll_next;
        int poll_late;

        struct pollfd pfd[IoAudioDriverChannels_COUNT];
        unsigned long interleave_unit;
        unsigned int max_nchannels;
        unsigned int user_nchannels;
//        int user_capture_nchnls;
//        int user_playback_nchnls;

        jack_nframes_t frame_rate;
//        jack_nframes_t frames_per_cycle;
//        jack_nframes_t capture_frame_latency;
//        jack_nframes_t playback_frame_latency;

//        unsigned long *silent;
//        char *ioaudio_driver;
        bitset_t channels_not_done;
        bitset_t channels_done;
//        float max_sample_val;
//        unsigned long user_nperiods;
//        unsigned long last_mask;
//        snd_ctl_t *ctl_handle;

        Channel playback;

        Channel capture;

//        Voice* playback_voices;
////        char *ioaudio_name_playback;
////        unsigned long *playback_interleave_skip;
//        unsigned int playback_nperiods;
//        snd_pcm_t *playback_handle;
//        snd_pcm_channel_params_t playback_params;
//        snd_pcm_channel_setup_t playback_setup;
//        snd_pcm_mmap_control_t *playback_mmap;
//        void *playback_mmap_buf;
//        void *playback_buffer;
////        char **playback_addr;
//        WriteCopyFunction write_via_copy;
//
//        Voice* capture_voices;
////        char *ioaudio_name_capture;
////        unsigned long *capture_interleave_skip;
//        unsigned int capture_nperiods;
//        snd_pcm_t *capture_handle;
//        snd_pcm_channel_params_t capture_params;
//        snd_pcm_channel_setup_t capture_setup;
//        snd_pcm_mmap_control_t *capture_mmap;
//        void *capture_buffer;
////        char **capture_addr;
//        ReadCopyFunction read_via_copy;

        jack_hardware_t *hw;
//        ClockSyncStatus *clock_sync_data;
        jack_client_t *client;
//        JSList *capture_ports;
//        JSList *playback_ports;
//        JSList *monitor_ports;

//        char soft_mode;
//        bool all_monitor_in;
        bool capture_and_playback_not_synced;
//        char with_monitor_ports;
        bool has_clock_sync_reporting;

        bool has_hw_monitoring;
        bool do_hw_monitoring;
        unsigned long input_monitor_mask;

        bool has_hw_metering;
        bool do_hw_metering;
        bool quirk_bswap;

//        int dither;
//        dither_state_t *dither_state;

//        SampleClockMode clock_mode;
//        JSList *clock_sync_listeners;
//        pthread_mutex_t clock_sync_lock;
//        unsigned long next_clock_sync_listener_id;

//        int running;
//        int run;

        int xrun_count;
        int process_count;

//        ioaudio_midi_t *midi;
//        char *midi;
        int xrun_recovery;

    };

    class generic_hardware: public jack_hardware_t
    {
        virtual double get_hardware_peak(
            jack_port_t *port,
            jack_nframes_t frames )
        {
        return -1;
        }

        virtual double get_hardware_power(
            jack_port_t *port,
            jack_nframes_t frames )
        {
        return -1;
        }

        virtual int set_input_monitor_mask(
            unsigned long mask )
        {
        return -1;
        }

        virtual int change_sample_clock(
            SampleClockMode mode )
        {
        return -1;
        }

        virtual void release()
        {
        return;
        }

    };

///////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

//    jack_hardware_t *
//    jack_ioaudio_generic_hw_new(
//        ioaudio_driver_t *fDriver );
//
//    jack_time_t jack_get_microseconds(
//        void );

// Code implemented in JackioaudioDriver.cpp

//void ReadInput(jack_nframes_t orig_nframes, ssize_t contiguous,
    //        ssize_t nread);
//void MonitorInput();
//void ClearOutput();
//void WriteOutput(jack_nframes_t orig_nframes, ssize_t contiguous,
//        ssize_t nwritten);
//void SetTime(jack_time_t time);
//int Restart();

//    extern void store_work_time(
//        int );
//    extern void store_wait_time(
//        int );
//    extern void show_wait_times();
//    extern void show_work_times();
//
//    char* strcasestr(
//        const char* haystack,
//        const char* needle );

///////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

    /* DRIVER "PLUGIN" INTERFACE */

    int JackIoAudioDriver::Attach()
    {
    JackPort* port;
    jack_port_id_t port_index;
    unsigned long port_flags = (unsigned long) CaptureDriverFlags;
    char name[REAL_JACK_PORT_NAME_SIZE];
    char alias[REAL_JACK_PORT_NAME_SIZE];

    assert( fCaptureChannels < DRIVER_PORT_NUM );
    assert( fPlaybackChannels < DRIVER_PORT_NUM );

    if( fDriver->has_hw_monitoring )
        port_flags |= JackPortCanMonitor;

    // io-audio driver may have changed the values
    JackAudioDriver::SetBufferSize( fArgs.frames_per_interrupt );
    JackAudioDriver::SetSampleRate( fDriver->frame_rate );

    jack_log( "JackIoAudioDriver::Attach fBufferSize %ld fSampleRate %ld",
              fEngineControl->fBufferSize,
              fEngineControl->fSampleRate );

    for( int i = 0; i < fCaptureChannels; i++ )
        {
        snprintf( alias,
                  sizeof( alias ),
                  "%s:%s:out%d",
                  fAliasName,
                  fCaptureDriverName,
                  i + 1 );
        snprintf( name,
                  sizeof( name ),
                  "%s:capture_%d",
                  fClientControl.fName,
                  i + 1 );
        if( fEngine->PortRegister( fClientControl.fRefNum,
                                   name,
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   (JackPortFlags)port_flags,
                                   fEngineControl->fBufferSize,
                                   &port_index ) < 0 )
            {
            jack_error( "driver: cannot register port for %s",
                        name );
            return -1;
            }
        port = fGraphManager->GetPort( port_index );
        port->SetAlias( alias );
        fCapturePortList[i] = port_index;
        jack_log( "JackIoAudioDriver::Attach fCapturePortList[i] %ld ",
                  port_index );
        }

    port_flags = (unsigned long) PlaybackDriverFlags;

    for( int i = 0; i < fPlaybackChannels; i++ )
        {
        snprintf( alias,
                  sizeof( alias ),
                  "%s:%s:in%d",
                  fAliasName,
                  fPlaybackDriverName,
                  i + 1 );
        snprintf( name,
                  sizeof( name ),
                  "%s:playback_%d",
                  fClientControl.fName,
                  i + 1 );
        if( fEngine->PortRegister( fClientControl.fRefNum,
                                   name,
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   (JackPortFlags)port_flags,
                                   fEngineControl->fBufferSize,
                                   &port_index ) < 0 )
            {
            jack_error( "driver: cannot register port for %s",
                        name );
            return -1;
            }
        port = fGraphManager->GetPort( port_index );
        port->SetAlias( alias );
        fPlaybackPortList[i] = port_index;
        jack_log( "JackIoAudioDriver::Attach fPlaybackPortList[i] %ld ",
                  port_index );

        // Monitor ports
        if( fWithMonitorPorts )
            {
            jack_log( "Create monitor port" );
            snprintf( name,
                      sizeof( name ),
                      "%s:monitor_%d",
                      fClientControl.fName,
                      i + 1 );
            if( fEngine->PortRegister( fClientControl.fRefNum,
                                       name,
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       MonitorDriverFlags, fEngineControl->fBufferSize, &port_index) <0 )
                {
                jack_error("io-audio: cannot register monitor port for %s", name);
                }
            else
                {
                fMonitorPortList[i] = port_index;
                }
            }
        }

    UpdateLatencies();

    //if (fDriver->midi) {
    //    int err = (fDriver->midi->attach)(fDriver->midi);
    //    if (err)
    //        jack_error ("io-audio: cannot attach MIDI: %d", err);
    //}

    return 0;
    }

    void JackIoAudioDriver::ClearOutputAux()
    {
    for( int chn = 0; chn < fPlaybackChannels; chn++ )
        {
        jack_default_audio_sample_t* buf =
            (jack_default_audio_sample_t*)fGraphManager->GetBuffer( fPlaybackPortList[chn],
                                                                    fEngineControl->fBufferSize );
        memset( buf,
                0,
                sizeof(jack_default_audio_sample_t)
                    * fEngineControl->fBufferSize );
        }
    }

    int JackIoAudioDriver::Close()
    {
    // Generic audio driver close
    int res = JackAudioDriver::Close();

    destroy();

    if( JackServerGlobals::on_device_release != NULL )
        {
        char audio_name[32];
        int capture_card = snd_card_name( fCaptureDriverName );
        if( capture_card >= 0 )
            {
            snprintf( audio_name,
                      sizeof( audio_name ),
                      "Audio%d",
                      capture_card );
            JackServerGlobals::on_device_release( audio_name );
            }

        int playback_card = snd_card_name( fPlaybackDriverName );
        if( playback_card >= 0 && playback_card != capture_card )
            {
            snprintf( audio_name,
                      sizeof( audio_name ),
                      "Audio%d",
                      playback_card );
            JackServerGlobals::on_device_release( audio_name );
            }
        }

    return res;
    }

    int JackIoAudioDriver::Detach()
    {
    //ioaudio_driver_t* ioaudio_driver = (ioaudio_driver_t*)fDriver;
    //if (fDriver->midi)
    //    (fDriver->midi->detach)(fDriver->midi);

    return JackAudioDriver::Detach();
    }

    void JackIoAudioDriver::MonitorInputAux()
    {
    for( int chn = 0; chn < fCaptureChannels; chn++ )
        {
        JackPort* port = fGraphManager->GetPort( fCapturePortList[chn] );
        if( port->MonitoringInput() )
            {
            ( (ioaudio_driver_t *)fDriver )->input_monitor_mask |= ( 1 << chn );
            }
        }
    }

    int JackIoAudioDriver::Open(
        ioaudio_driver_args_t args )
    {
    // Generic JackAudioDriver Open
    if( JackAudioDriver::Open( args.frames_per_interrupt,
                               args.srate,
                               args.capture,
                               args.playback,
                               args.user_capture_nchnls,
                               args.user_playback_nchnls,
                               args.monitor,
                               args.capture_pcm_name,
                               args.playback_pcm_name,
                               args.systemic_input_latency,
                               args.systemic_output_latency ) != 0 )
        {
        return -1;
        }

    memcpy( &fArgs,
            &args,
            sizeof(ioaudio_driver_args_t) );

    //ioaudio_midi_t *midi = 0;
    //if (strcmp(midi_driver_name, "seq") == 0)
    //    midi = ioaudio_seqmidi_new((jack_client_t*)this, 0);
    //else if (strcmp(midi_driver_name, "raw") == 0)
    //    midi = ioaudio_rawmidi_new((jack_client_t*)this);

    if( JackServerGlobals::on_device_acquire != NULL )
        {
        int capture_card = snd_card_name( args.capture_pcm_name );
        int playback_card = snd_card_name( args.playback_pcm_name );
        char audio_name[32];

        if( capture_card >= 0 )
            {
            snprintf( audio_name,
                      sizeof( audio_name ),
                      "Audio%d",
                      capture_card );
            if( !JackServerGlobals::on_device_acquire( audio_name ) )
                {
                jack_error( "Audio device %s cannot be acquired...",
                            args.capture_pcm_name );
                return -1;
                }
            }

        if( playback_card >= 0 && playback_card != capture_card )
            {
            snprintf( audio_name,
                      sizeof( audio_name ),
                      "Audio%d",
                      playback_card );
            if( !JackServerGlobals::on_device_acquire( audio_name ) )
                {
                jack_error( "Audio device %s cannot be acquired...",
                            args.playback_pcm_name );
                if( capture_card >= 0 )
                    {
                    snprintf( audio_name,
                              sizeof( audio_name ),
                              "Audio%d",
                              capture_card );
                    JackServerGlobals::on_device_release( audio_name );
                    }
                return -1;
                }
            }
        }

    create( (char*)"ioaudio_pcm",
            NULL );
    if( fDriver )
        {
        // io-audio driver may have changed the in/out values
        fCaptureChannels = fDriver->capture.setup.format.voices;
        fPlaybackChannels = fDriver->playback.setup.format.voices;
        return 0;
        }
    else
        {
        JackAudioDriver::Close();
        return -1;
        }
    }

    int JackIoAudioDriver::Read()
    {
    /* Taken from ioaudio_driver_run_cycle */
    int wait_status;
    jack_nframes_t nframes;
    fDelayedUsecs = 0.f;

    retry:

    nframes = wait( &wait_status,
                    &fDelayedUsecs );

    if( wait_status < 0 )
        return -1; /* driver failed */

    if( nframes == 0 )
        {
        /* we detected an xrun and restarted: notify
         * clients about the delay.
         */
        jack_log( "io-audio XRun wait_status = %d",
                  wait_status );
        NotifyXRun( fBeginDateUst,
                    fDelayedUsecs );
        goto retry;
        /* recoverable error*/
        }

    if( nframes != fEngineControl->fBufferSize )
        jack_log( "JackIoAudioDriver::Read warning fBufferSize = %ld nframes = %ld",
                  fEngineControl->fBufferSize,
                  nframes );

    // Has to be done before read
    JackDriver::CycleIncTime();

    return read( fEngineControl->fBufferSize );
    }

    void JackIoAudioDriver::ReadInputAux(
        jack_nframes_t orig_nframes,
        ssize_t contiguous,
        ssize_t nread )
    {
    for( int chn = 0; chn < fCaptureChannels; chn++ )
        {
        if( fGraphManager->GetConnectionsNum( fCapturePortList[chn] ) > 0 )
            {
            jack_default_audio_sample_t* buf =
                (jack_default_audio_sample_t*)fGraphManager->GetBuffer( fCapturePortList[chn],
                                                                        orig_nframes );
            fDriver->capture.read( buf + nread,
                                   fDriver->capture.voices[chn].addr,
                                   contiguous,
                                   fDriver->capture.voices[chn].interleave_skip );
            }
        }
    }

    int JackIoAudioDriver::SetBufferSize(
        jack_nframes_t buffer_size )
    {
    jack_log( "JackIoAudioDriver::SetBufferSize %ld",
              buffer_size );
    fArgs.frames_per_interrupt = buffer_size;
    int res = reset_parameters();

    if( res == 0 )
        {
        // update fEngineControl and fGraphManager
        JackAudioDriver::SetBufferSize( buffer_size ); // Generic change, never fails
        // io-audio specific
        UpdateLatencies();
        }
    else
        {
        // Restore old values
        fArgs.frames_per_interrupt = fEngineControl->fBufferSize;
        reset_parameters();
        }

    return res;
    }

//    void JackIoAudioDriver::SetTimetAux(
//        jack_time_t time )
//    {
//    fBeginDateUst = time;
//    }

    int JackIoAudioDriver::Start()
    {
    int res = JackAudioDriver::Start();
    if( res >= 0 )
        {
        res = start();
        if( res < 0 )
            {
            JackAudioDriver::Stop();
            }
        }
    return res;
    }

    int JackIoAudioDriver::Stop()
    {
    int res = stop();
    if( JackAudioDriver::Stop() < 0 )
        {
        res = -1;
        }
    return res;
    }

    void JackIoAudioDriver::UpdateLatencies()
    {
    jack_latency_range_t range;

    for( int i = 0; i < fCaptureChannels; i++ )
        {
        range.min = range.max = fArgs.frames_per_interrupt
            + fArgs.systemic_input_latency;
        fGraphManager->GetPort( fCapturePortList[i] )->SetLatencyRange( JackCaptureLatency,
                                                                        &range );
        }

    for( int i = 0; i < fPlaybackChannels; i++ )
        {
        // Add one buffer more latency if "async" mode is used...
        range.min = range.max =
            ( fArgs.frames_per_interrupt * ( fArgs.user_nperiods - 1 ) )
                + ( ( fEngineControl->fSyncMode ) ?
                    0 : fEngineControl->fBufferSize )
                + fArgs.systemic_output_latency;
        fGraphManager->GetPort( fPlaybackPortList[i] )->SetLatencyRange( JackPlaybackLatency,
                                                                         &range );
        // Monitor port
        if( fWithMonitorPorts )
            {
            range.min = range.max = fArgs.frames_per_interrupt;
            fGraphManager->GetPort( fMonitorPortList[i] )->SetLatencyRange( JackCaptureLatency,
                                                                            &range );
            }
        }
    }

    int JackIoAudioDriver::Write()
    {
    jack_nframes_t nframes = fEngineControl->fBufferSize;

    jack_nframes_t orig_nframes;
    ssize_t nwritten;
    ssize_t contiguous;
    size_t offset = 0;
    //jack_port_t *port;
    int err;

    fDriver->process_count++;

    if( !fDriver->playback.handle )
        {
        return 0;
        }

    if( nframes > fArgs.frames_per_interrupt )
        {
        return -1;
        }

    //if (fDriver->midi)
    //    (fDriver->midi->write)(fDriver->midi, nframes);

    nwritten = 0;
    contiguous = 0;
    orig_nframes = nframes;

    /* check current input monitor request status */

    fDriver->input_monitor_mask = 0;

    MonitorInputAux();

    if( fDriver->do_hw_monitoring )
        {
        if( fDriver->hw->input_monitor_mask != fDriver->input_monitor_mask )
            {
            fDriver->hw->set_input_monitor_mask( fDriver->input_monitor_mask );
            }
        }

    while( nframes )
        {

        contiguous = nframes;

        if( get_channel_addresses( (size_t *)0,
                                   (size_t *)&contiguous,
                                   0,
                                   &offset ) < 0 )
            {
            return -1;
            }

        WriteOutputAux( orig_nframes,
                        contiguous,
                        nwritten );

        if( !bitset_empty( fDriver->channels_not_done ) )
            {
            silence_untouched_channels( contiguous );
            }

        err = snd_pcm_write( fDriver->playback.handle,
                             fDriver->playback.buffer,
                             fDriver->playback.frag_bytes() );
        if( err < 0 )
            {
            jack_error( "io-audio: could not complete playback of %"
                        PRIu32 " frames: error = %d",
                        contiguous,
                        err );
            if( err != -EPIPE && err != -ESTRPIPE )
                return -1;
            }

        nframes -= contiguous;
        nwritten += contiguous;
        }

    return 0;
    }

    void JackIoAudioDriver::WriteOutputAux(
        jack_nframes_t orig_nframes,
        ssize_t contiguous,
        ssize_t nwritten )
    {
    for( int chn = 0; chn < fPlaybackChannels; chn++ )
        {
        // Output ports
        if( fGraphManager->GetConnectionsNum( fPlaybackPortList[chn] ) > 0 )
            {
            jack_default_audio_sample_t* buf =
                (jack_default_audio_sample_t*)fGraphManager->GetBuffer( fPlaybackPortList[chn],
                                                                        orig_nframes );
            fDriver->playback.write( fDriver->playback.voices[chn].addr,
                                     buf + nwritten,
                                     contiguous,
                                     fDriver->playback.voices[chn].interleave_skip,
                                     &fDriver->playback.voices[chn].dither_state );
            mark_channel_done( chn );

            // Monitor ports
            if( fWithMonitorPorts
                && fGraphManager->GetConnectionsNum( fMonitorPortList[chn] )
                    > 0 )
                {
                jack_default_audio_sample_t* monbuf =
                    (jack_default_audio_sample_t*)fGraphManager->GetBuffer( fMonitorPortList[chn],
                                                                            orig_nframes );
                memcpy( monbuf + nwritten,
                        buf + nwritten,
                        contiguous * sizeof(jack_default_audio_sample_t) );
                }
            }
        }
    }
//
//    ssize_t JackIoAudioDriver::capture_sample_bytes()
//    {
//    return snd_pcm_format_size( fDriver->capture.setup.format.format,
//                                1 );
//    }
//    ssize_t JackIoAudioDriver::capture_frame_bytes()
//    {
//    return snd_pcm_format_size( fDriver->capture.setup.format.format,
//                                fDriver->capture.setup.format.voices );
//    }
//    ssize_t JackIoAudioDriver::capture_frag_bytes()
//    {
//    return snd_pcm_format_size( fDriver->capture.setup.format.format,
//                                fDriver->capture.setup.format.voices
//                                    * fArgs.frames_per_interrupt );
//    }
//    ssize_t JackIoAudioDriver::capture_frag_frames()
//    {
//    return capture_frag_bytes()
//        / snd_pcm_format_size( fDriver->capture.setup.format.format,
//                               fDriver->capture.setup.format.voices );
//    }
//
//    ssize_t JackIoAudioDriver::playback_sample_bytes()
//    {
//    return snd_pcm_format_size( fDriver->playback.setup.format.format,
//                                1 );
//    }
//    ssize_t JackIoAudioDriver::playback_frame_bytes()
//    {
//    return snd_pcm_format_size( fDriver->playback.setup.format.format,
//                                fDriver->playback.setup.format.voices );
//    }
//    ssize_t JackIoAudioDriver::playback_frag_bytes()
//    {
//    return snd_pcm_format_size( fDriver->playback.setup.format.format,
//                                fDriver->playback.setup.format.voices
//                                    * fArgs.frames_per_interrupt );
//    }
//    ssize_t JackIoAudioDriver::playback_frag_frames()
//    {
//    return playback_frag_bytes()
//        / snd_pcm_format_size( fDriver->playback.setup.format.format,
//                               fDriver->playback.setup.format.voices );
//    }

    int JackIoAudioDriver::check_capabilities(
        const char *devicename,
        int mode )
    {
    int ret = 0;
    int err;
    snd_pcm_t* device;
    snd_pcm_info_t info;
    snd_ctl_t* ctl;
    snd_ctl_hw_info_t hw_info;

    err = snd_pcm_open_name( &device,
                             devicename,
                             mode );
    if( err )
        {
        jack_error( "device open name:(%s) error:(%s)",
                    devicename,
                    snd_strerror( err ) );
        return err;
        }

    err = snd_pcm_info( device,
                        &info );
    if( err )
        {
        jack_error( "device info error:(%s)",
                    devicename,
                    snd_strerror( err ) );
        snd_pcm_close( device );
        return err;
        }

    err = snd_ctl_open( &ctl,
                        info.card );
    if( err )
        {
        jack_error( "control open card:(%d) error:(%s)",
                    info.card,
                    snd_strerror( err ) );
        snd_pcm_close( device );
        return err;
        }

    err = snd_ctl_hw_info( ctl,
                           &hw_info );
    if( err )
        {
        jack_error( "control hardware error:(%s)",
                    snd_strerror( err ) );
        snd_ctl_close( ctl );
        snd_pcm_close( device );
        return err;
        }

    if( ( SND_PCM_OPEN_CAPTURE == mode )
        && !( info.flags & SND_PCM_INFO_CAPTURE ) )
        {
        jack_error( "Capture channels required but not available" );
        ret = -EINVAL;
        }
    if( ( SND_PCM_OPEN_PLAYBACK == mode )
        && !( info.flags & SND_PCM_INFO_PLAYBACK ) )
        {
        jack_error( "Playback channels required but not available" );
        ret = -EINVAL;
        }

    snd_ctl_close( ctl );
    snd_pcm_close( device );
    return ret;
    }

    int JackIoAudioDriver::check_card_type()
    {
    int err;

    // Check capture device can be opened
    if( fArgs.capture )
        {
        err = check_capabilities( fArgs.capture_pcm_name,
                                  SND_PCM_OPEN_CAPTURE );
        if( err )
            return err;
        }

    // Check playback device can be opened
    if( fArgs.playback )
        {
        err = check_capabilities( fArgs.playback_pcm_name,
                                  SND_PCM_OPEN_PLAYBACK );
        if( err )
            return err;
        }

    return 0;
    }

    int JackIoAudioDriver::configure_stream(
        const char *device_name,
        const char *stream_name,
        snd_pcm_t *handle,
        snd_pcm_channel_params_t *params,
        unsigned int *nperiodsp )
    {
    int err;
    size_t format;
    snd_pcm_channel_info_t info;
    int32_t formats[] = {
    SND_PCM_SFMT_FLOAT_LE,
                          SND_PCM_SFMT_S32_LE,
                          SND_PCM_SFMT_S32_BE,
                          SND_PCM_SFMT_S24_LE,
                          SND_PCM_SFMT_S24_BE,
                          SND_PCM_SFMT_S16_LE,
                          SND_PCM_SFMT_S16_BE };
#define NUMFORMATS (sizeof(formats)/sizeof(formats[0]))

    info.channel = params->channel;

    err = snd_pcm_channel_info( handle,
                                &info );

    if( SND_PCM_CHNINFO_NONINTERLEAVE & info.flags )
        {
        params->format.interleave = 0;
        }
    else if( SND_PCM_CHNINFO_INTERLEAVE & info.flags )
        {
        params->format.interleave = 1;
        }

    if( !( SND_PCM_CHNINFO_MMAP & info.flags ) )
        {
        jack_error( "io-audio: mmap-based access is not possible"
                    " for the %s "
                    "stream of this audio interface",
                    stream_name );
        return -1;
        }

    params->mode = SND_PCM_MODE_BLOCK;

    for( format = 0; format < NUMFORMATS; ++format )
        {
        if( info.formats & ( 1 << formats[format] ) )
            {
            jack_log( "io-audio: final selected sample format for %s: %s",
                      stream_name,
                      snd_pcm_get_format_name( formats[format] ) );
            params->format.format = formats[format];
            break;
            }
        }
    if( NUMFORMATS == format )
        {
        jack_error( "Sorry. The audio interface \"%s\""
                    " doesn't support any of the"
                    " hardware sample formats that"
                    " JACK's ioaudio-driver can use.",
                    device_name );
        return -1;
        }

#if defined(SND_LITTLE_ENDIAN)
    fDriver->quirk_bswap = snd_pcm_format_big_endian(formats[format]);
#elif defined(SND_BIG_ENDIAN)
    fDriver->quirk_bswap = snd_pcm_format_little_endian( formats[format] );
#else
    fDriver->quirk_bswap = 0;
#endif

    params->format.rate = fDriver->frame_rate;

    if( 0 == params->format.voices )
        {
        /*if not user-specified, try to find the maximum
         * number of channels */
        params->format.voices = info.max_voices;
        }

    params->start_mode = SND_PCM_START_DATA;

    params->stop_mode = SND_PCM_STOP_STOP;

    params->time = 1;

    params->buf.block.frag_size =
        snd_pcm_format_size( formats[format],
                             params->format.voices
                                 * fArgs.frames_per_interrupt );

    *nperiodsp = fArgs.user_nperiods;
    params->buf.block.frags_min = fArgs.user_nperiods;
    params->buf.block.frags_max = fArgs.user_nperiods;

    jack_log( "io-audio: use %d periods for %s",
              *nperiodsp,
              stream_name );

    int subchn_len = sizeof( params->sw_mixer_subchn_name );
    strncpy( params->sw_mixer_subchn_name,
             stream_name,
             subchn_len );
    params->sw_mixer_subchn_name[subchn_len - 1] = '\0';

    if( ( err = snd_pcm_channel_params( handle,
                                        params ) ) < 0 )
        {
        jack_error( "io-audio: cannot set hardware parameters for %s, why_failed=%d",
                    stream_name,
                    params->why_failed );
        return -1;
        }

    return 0;
    }

    void JackIoAudioDriver::create(
        char *name,
        jack_client_t *client )
    {
    int err;

    jack_log( "creating ioaudio driver ... %s|%s|%" PRIu32 "|%" PRIu32
              "|%" PRIu32"|%" PRIu32"|%" PRIu32 "|%s|%s|%s|%s",
              fArgs.playback ? fArgs.playback_pcm_name : "-",
              fArgs.capture ? fArgs.capture_pcm_name : "-",
              fArgs.frames_per_interrupt,
              fArgs.user_nperiods,
              fArgs.srate,
              fArgs.user_capture_nchnls,
              fArgs.user_playback_nchnls,
              fArgs.hw_monitoring ? "hwmon" : "nomon",
              fArgs.hw_metering ? "hwmeter" : "swmeter",
              fArgs.soft_mode ? "soft-mode" : "-",
              fArgs.shorts_first ? "16bit" : "32bit" );

//    fDriver = (ioaudio_driver_t *)calloc( 1,
//                                          sizeof(ioaudio_driver_t) );
    fDriver = new ioaudio_driver_t;

//    fDriver->user_playback_nchnls = fArgs.user_playback_nchnls;
//    fDriver->user_capture_nchnls = fArgs.user_capture_nchnls;
//    fDriver->capture.frame_latency = fArgs.systemic_input_latency;
//    fDriver->playback.frame_latency = fArgs.systemic_output_latency;

//    fDriver->with_monitor_ports = fArgs.monitor;

//    fDriver->clock_mode = ClockMaster; /* XXX is it? */

//    fDriver->dither = fArgs.dither;
//    fDriver->soft_mode = fArgs.soft_mode;

//    pthread_mutex_init( &fDriver->clock_sync_lock,
//                        0 );

//        fDriver->ioaudio_name_playback = strdup(fArgs.playback_pcm_name);
//        fDriver->ioaudio_name_capture = strdup(fArgs.capture_pcm_name);

    //fDriver->midi = midi_driver;

    if( check_card_type() )
        {
        destroy();
        return;
        }

    hw_specific();

    if( fArgs.playback )
        {
        err =
            snd_pcm_open_name( &fDriver->playback.handle,
                               fArgs.playback_pcm_name,
                               SND_PCM_OPEN_PLAYBACK | SND_PCM_OPEN_NONBLOCK );
        if( err < 0 )
            {
            switch( errno )
                {
                case EBUSY:
                    jack_error( "\n\nATTENTION: The playback device \"%s\" is "
                                "already in use. Please stop the"
                                " application using it and "
                                "run JACK again",
                                fArgs.playback_pcm_name );
                    destroy();
                    return;

                case EPERM:
                    jack_error( "you do not have permission to open "
                                "the audio device \"%s\" for playback",
                                fArgs.playback_pcm_name );
                    destroy();
                    return;
                }

            fDriver->playback.handle = NULL;
            }

        if( fDriver->playback.handle )
            {
            snd_pcm_nonblock_mode( fDriver->playback.handle,
                                   0 );
            }
        else
            {

            /* they asked for playback, but we can't do it */

            jack_error( "io-audio: Cannot open PCM device %s for "
                        "playback. Falling back to capture-only"
                        " mode",
                        name );
            fArgs.playback = false;
            }

        }

    if( fArgs.capture )
        {
        err = snd_pcm_open_name( &fDriver->capture.handle,
                                 fArgs.capture_pcm_name,
                                 SND_PCM_OPEN_CAPTURE | SND_PCM_OPEN_NONBLOCK );
        if( err < 0 )
            {
            switch( errno )
                {
                case EBUSY:
                    jack_error( "\n\nATTENTION: The capture (recording) device \"%s\" is "
                                "already in use. Please stop the"
                                " application using it and "
                                "run JACK again",
                                fArgs.capture_pcm_name );
                    destroy();
                    return;

                case EPERM:
                    jack_error( "you do not have permission to open "
                                "the audio device \"%s\" for capture",
                                fArgs.capture_pcm_name );
                    destroy();
                    return;
                }

            fDriver->capture.handle = NULL;
            }

        if( fDriver->capture.handle )
            {
            snd_pcm_nonblock_mode( fDriver->capture.handle,
                                   0 );
            }
        else
            {

            /* they asked for capture, but we can't do it */

            jack_error( "io-audio: Cannot open PCM device %s for "
                        "capture. Falling back to playback-only"
                        " mode",
                        name );

            fArgs.capture = false;
            }
        }

    if( fDriver->playback.handle == NULL && fDriver->capture.handle == NULL )
        {
        /* can't do anything */
        destroy();
        return;
        }

    err = set_parameters();
    if( 0 != err )
        {
        destroy();
        return;
        }

    fDriver->capture_and_playback_not_synced = false;

    if( fDriver->capture.handle && fDriver->playback.handle )
        {
        if( snd_pcm_link( fDriver->playback.handle,
                          fDriver->capture.handle ) != 0 )
            {
            fDriver->capture_and_playback_not_synced = true;
            }
        }

    fDriver->client = client;
    }

    void JackIoAudioDriver::destroy()
    {
//    JSList *node;

    //if (ffDriver->midi)
    //        (fDriver->midi->destroy)(fDriver->midi);

//    for( node = fDriver->clock_sync_listeners; node;
//        node = jack_slist_next( node ) )
//        {
//        free( node->data );
//        }
//    jack_slist_free( fDriver->clock_sync_listeners );

//        if (fDriver->ctl_handle)
//            {
//            snd_ctl_close(fDriver->ctl_handle);
//            fDriver->ctl_handle = 0;
//            }

    if( fDriver->capture.handle )
        {
        snd_pcm_close( fDriver->capture.handle );
        fDriver->capture.handle = 0;
        }

    if( fDriver->playback.handle )
        {
        snd_pcm_close( fDriver->playback.handle );
        fDriver->capture.handle = 0;
        }

    if( fDriver->hw )
        {
        fDriver->hw->release();
        delete fDriver->hw;
        fDriver->hw = 0;
        }
//        free(fDriver->ioaudio_name_playback);
//        free(fDriver->ioaudio_name_capture);
//        free(fDriver->ioaudio_driver);

    release_channel_dependent_memory();
//    free( fDriver );
    delete fDriver;
    fDriver = NULL;
    }

//    int JackIoAudioDriver::generic_hardware()
//    {
//    jack_hardware_t *hw;
//
//    hw = (jack_hardware_t *)malloc( sizeof(jack_hardware_t) );
//
//    hw->capabilities = 0;
//    hw->input_monitor_mask = 0;
//
//    hw->set_input_monitor_mask = generic_set_input_monitor_mask;
//    hw->change_sample_clock = generic_change_sample_clock;
//    hw->release = generic_release;
//
//    fDriver->hw = hw;
//
//    return 0;
//    }

    int JackIoAudioDriver::get_channel_addresses(
        size_t *capture_avail,
        size_t *playback_avail,
        size_t *capture_offset,
        size_t *playback_offset )
    {
    int chn;

    if( capture_avail )
        {
        ssize_t samp_bytes = fDriver->capture.sample_bytes();
        ssize_t frame_bytes = fDriver->capture.frame_bytes();
        for( chn = 0; chn < fDriver->capture.setup.format.voices; chn++ )
            {
            fDriver->capture.voices[chn].addr = (char*)fDriver->capture.buffer
                + chn * samp_bytes;
            fDriver->capture.voices[chn].interleave_skip = frame_bytes;
            }
        }

    if( playback_avail )
        {
        ssize_t samp_bytes = fDriver->playback.sample_bytes();
        ssize_t frame_bytes = fDriver->playback.frame_bytes();
        for( chn = 0; chn < fDriver->playback.setup.format.voices; chn++ )
            {
            fDriver->playback.voices[chn].addr = (char*)fDriver->playback.buffer
                + chn * samp_bytes;
            fDriver->playback.voices[chn].interleave_skip = frame_bytes;

            }
        }

    return 0;
    }

    int JackIoAudioDriver::hw_specific()
    {
    int err;

    fDriver->hw = new Jack::generic_hardware;
    if( NULL == fDriver->hw )
        {
        return -ENOMEM;
        }

    if( fDriver->hw->capabilities & Cap_HardwareMonitoring )
        {
        fDriver->has_hw_monitoring = true;
        /* XXX need to ensure that this is really false or
         * true or whatever*/
        fDriver->do_hw_monitoring = fArgs.hw_monitoring;
        }
    else
        {
        fDriver->has_hw_monitoring = false;
        fDriver->do_hw_monitoring = false;
        }

    if( fDriver->hw->capabilities & Cap_ClockLockReporting )
        {
        fDriver->has_clock_sync_reporting = true;
        }
    else
        {
        fDriver->has_clock_sync_reporting = false;
        }

    if( fDriver->hw->capabilities & Cap_HardwareMetering )
        {
        fDriver->has_hw_metering = true;
        fDriver->do_hw_metering = fArgs.hw_metering;
        }
    else
        {
        fDriver->has_hw_metering = false;
        fDriver->do_hw_metering = false;
        }

    return 0;
    }

    void JackIoAudioDriver::mark_channel_done(
        int chn )
    {
    bitset_remove( fDriver->channels_not_done,
                   chn );
    fDriver->playback.voices[chn].silent = 0;
    }

    int JackIoAudioDriver::read(
        jack_nframes_t nframes )
    {
    ssize_t contiguous;
    ssize_t nread;
    size_t offset = 0;
    jack_nframes_t orig_nframes;
    int err;

    if( nframes > fArgs.frames_per_interrupt )
        {
        return -1;
        }

    //if (fDriver->midi)
    //    (fDriver->midi->read)(fDriver->midi, nframes);

    if( !fDriver->capture.handle )
        {
        return 0;
        }

    nread = 0;
    contiguous = 0;
    orig_nframes = nframes;

    while( nframes )
        {

        contiguous = nframes;

        if( get_channel_addresses( (size_t *)&contiguous,
                                   (size_t *)0,
                                   &offset,
                                   0 ) < 0 )
            {
            return -1;
            }

        err = snd_pcm_read( fDriver->capture.handle,
                            fDriver->capture.buffer,
                            fDriver->capture.frag_bytes() );
        if( err < 0 )
            {
            jack_error( "io-audio: could not complete read of %"
                        PRIu32 " frames: error = %d",
                        contiguous,
                        err );
            return -1;
            }

        ReadInputAux( orig_nframes,
                      contiguous,
                      nread );

        nframes -= contiguous;
        nread += contiguous;
        }

    return 0;
    }

    void JackIoAudioDriver::release_channel_dependent_memory()
    {
    bitset_destroy( &fDriver->channels_done );
    bitset_destroy( &fDriver->channels_not_done );

    delete[] fDriver->capture.voices;
    delete[] fDriver->playback.voices;

//    if( fDriver->playback.addr )
//        {
//        free( fDriver->playback.addr );
//        fDriver->playback.addr = 0;
//        }
//
//    if( fDriver->capture.addr )
//        {
//        free( fDriver->capture.addr );
//        fDriver->capture.addr = 0;
//        }
//
//    if( fDriver->playback.interleave_skip )
//        {
//        free( fDriver->playback.interleave_skip );
//        fDriver->playback.interleave_skip = NULL;
//        }
//
//    if( fDriver->capture.interleave_skip )
//        {
//        free( fDriver->capture.interleave_skip );
//        fDriver->capture.interleave_skip = NULL;
//        }
//
//    if( fDriver->silent )
//        {
//        free( fDriver->silent );
//        fDriver->silent = 0;
//        }
//
//    if( fDriver->dither_state )
//        {
//        free( fDriver->dither_state );
//        fDriver->dither_state = 0;
//        }
    }

    int JackIoAudioDriver::reset_parameters()
    {
    /* XXX unregister old ports ? */
    release_channel_dependent_memory();
    return set_parameters();
    }

    int JackIoAudioDriver::restart()
    {
    int res;

    fDriver->xrun_recovery = 1;
    if( ( res = Stop() ) == 0 )
        {
        res = Start();
        }
    fDriver->xrun_recovery = 0;

    //if (res && fDriver->midi)
    //    (fDriver->midi->stop)(fDriver->midi);

    return res;
    }

    int JackIoAudioDriver::set_parameters()
    {
    ssize_t p_period_size = 0;
    size_t c_period_size = 0;
    int chn;
    unsigned int pr = 0;
    unsigned int cr = 0;
    int err;

    fDriver->frame_rate = fArgs.srate;
//        fArgs.frames_per_interrupt = fArgs.frames_per_interrupt;
//        fArgs.user_nperiods = fArgs.user_nperiods;

    jack_log( "configuring for %" PRIu32 "Hz, period = %"
              PRIu32 " frames (%.1f ms), buffer = %" PRIu32 " periods",
              fArgs.srate,
              fArgs.frames_per_interrupt,
              ( ( (float)fArgs.frames_per_interrupt / (float)fArgs.srate )
                  * 1000.0f ),
              fArgs.user_nperiods );

    if( fDriver->capture.handle )
        {
        memset( &fDriver->capture.params,
                0,
                sizeof(snd_pcm_channel_params_t) );
        fDriver->capture.params.channel = SND_PCM_CHANNEL_CAPTURE;
        if( configure_stream( fArgs.capture_pcm_name,
                              "capture",
                              fDriver->capture.handle,
                              &fDriver->capture.params,
                              &fDriver->capture.nperiods ) )
            {
            jack_error( "io-audio: cannot configure capture channel" );
            return -1;
            }
        fDriver->capture.setup.channel = SND_PCM_CHANNEL_CAPTURE;
        snd_pcm_channel_setup( fDriver->capture.handle,
                               &fDriver->capture.setup );
        cr = fDriver->capture.setup.format.rate;

        /* check the fragment size, since thats non-negotiable */
        c_period_size = fDriver->capture.setup.buf.block.frag_size;

        if( c_period_size != fArgs.frames_per_interrupt )
            {
            jack_error( "ioaudio_pcm: requested an interrupt every %"
                        PRIu32
                        " frames but got %uc frames for capture",
                        fArgs.frames_per_interrupt,
                        p_period_size );
            return -1;
            }

        /* check the sample format */
        switch( fDriver->capture.setup.format.format )
            {
            case SND_PCM_SFMT_FLOAT_LE:
            case SND_PCM_SFMT_S32_LE:
            case SND_PCM_SFMT_S24_LE:
            case SND_PCM_SFMT_S24_BE:
            case SND_PCM_SFMT_S16_LE:
            case SND_PCM_SFMT_S32_BE:
            case SND_PCM_SFMT_S16_BE:
                break;

            default:
                jack_error( "programming error: unhandled format "
                            "type for capture" );
                return -1;
            }

        if( fDriver->capture.setup.format.interleave )
            {
            if( ( err = snd_pcm_mmap( fDriver->capture.handle,
                                      SND_PCM_CHANNEL_CAPTURE,
                                      &fDriver->capture.mmap,
                                      &fDriver->capture.buffer ) ) < 0 )
                {
                jack_error( "io-audio: %s: mmap areas info error",
                            fArgs.capture_pcm_name );
                return -1;
                }
            }
        }

    if( fDriver->playback.handle )
        {
        memset( &fDriver->playback.params,
                0,
                sizeof(snd_pcm_channel_params_t) );
        fDriver->playback.params.channel = SND_PCM_CHANNEL_PLAYBACK;

        if( configure_stream( fArgs.playback_pcm_name,
                              "playback",
                              fDriver->playback.handle,
                              &fDriver->playback.params,
                              &fDriver->playback.nperiods ) )
            {
            jack_error( "io-audio: cannot configure playback channel" );
            return -1;
            }

        fDriver->playback.setup.channel = SND_PCM_CHANNEL_PLAYBACK;
        snd_pcm_channel_setup( fDriver->playback.handle,
                               &fDriver->playback.setup );
        pr = fDriver->playback.setup.format.rate;

        /* check the fragment size, since thats non-negotiable */
        p_period_size = fDriver->playback.setup.buf.block.frag_size;

        if( p_period_size != fDriver->playback.frag_bytes() )
            {
            jack_error( "ioaudio_pcm: requested an interrupt every %"
                        PRIu32
                        " frames but got %u frames for playback",
                        fArgs.frames_per_interrupt,
                        p_period_size );
            return -1;
            }

        /* check the sample format */
        switch( fDriver->playback.setup.format.format )
            {
            case SND_PCM_SFMT_FLOAT_LE:
            case SND_PCM_SFMT_S32_LE:
            case SND_PCM_SFMT_S24_LE:
            case SND_PCM_SFMT_S24_BE:
            case SND_PCM_SFMT_S16_LE:
            case SND_PCM_SFMT_S32_BE:
            case SND_PCM_SFMT_S16_BE:
                break;

            default:
                jack_error( "programming error: unhandled format "
                            "type for playback" );
                return -1;
            }

        fDriver->playback.buffer = malloc( fArgs.user_nperiods
            * p_period_size );
        if( fDriver->playback.setup.format.interleave )
            {
            fDriver->interleave_unit = fDriver->playback.sample_bytes();
            }
        else
            {
            fDriver->interleave_unit = 0; /* NOT USED */
            }
        }

    /* check the rate, since thats rather important */
    if( fDriver->capture.handle && fDriver->playback.handle )
        {
        if( cr != pr )
            {
            jack_error( "playback and capture sample rates do "
                        "not match (%d vs. %d)",
                        pr,
                        cr );
            }

        /* only change if *both* capture and playback rates
         * don't match requested certain hardware actually
         * still works properly in full-duplex with slightly
         * different rate values between adc and dac
         */
        if( cr != fDriver->frame_rate && pr != fDriver->frame_rate )
            {
            jack_error( "sample rate in use (%d Hz) does not "
                        "match requested rate (%d Hz)",
                        cr,
                        fDriver->frame_rate );
            fDriver->frame_rate = cr;
            }

        }
    else if( fDriver->capture.handle && cr != fDriver->frame_rate )
        {
        jack_error( "capture sample rate in use (%d Hz) does not "
                    "match requested rate (%d Hz)",
                    cr,
                    fDriver->frame_rate );
        fDriver->frame_rate = cr;
        }
    else if( fDriver->playback.handle && pr != fDriver->frame_rate )
        {
        jack_error( "playback sample rate in use (%d Hz) does not "
                    "match requested rate (%d Hz)",
                    pr,
                    fDriver->frame_rate );
        fDriver->frame_rate = pr;
        }

    fDriver->max_nchannels = std::max( fDriver->playback.setup.format.voices,
                                       fDriver->capture.setup.format.voices );
    fDriver->user_nchannels = std::min( fDriver->playback.setup.format.voices,
                                        fDriver->capture.setup.format.voices );

    setup_io_function_pointers();

    /* Allocate and initialize structures that rely on the
     channels counts.

     Set up the bit pattern that is used to record which
     channels require action on every cycle. any bits that are
     not set after the engine's process() call indicate channels
     that potentially need to be silenced.
     */

    bitset_create( &fDriver->channels_done,
                   fDriver->max_nchannels );
    bitset_create( &fDriver->channels_not_done,
                   fDriver->max_nchannels );

    if( fDriver->playback.handle )
        {
        fDriver->playback.voices = new Voice[fDriver->max_nchannels];
//        fDriver->playback.addr = (char **)calloc( fDriver->max_nchannels,
//                                                  sizeof(char *) );

//        fDriver->playback.interleave_skip =
//            (unsigned long *)calloc( fDriver->max_nchannels,
//                                     sizeof(unsigned long) );

//        fDriver->silent = (unsigned long *)calloc( fDriver->max_nchannels,
//                                                   sizeof(unsigned long) );

        for( chn = 0; chn < fDriver->playback.setup.format.voices; chn++ )
            {
            bitset_add( fDriver->channels_done,
                        chn );
            }

//        fDriver->dither_state =
//            (dither_state_t *)calloc( fDriver->max_nchannels,
//                                      sizeof(dither_state_t) );
        }

    if( fDriver->capture.handle )
        {
        fDriver->capture.voices = new Voice[fDriver->max_nchannels];
//        fDriver->capture.addr = (char **)calloc( fDriver->max_nchannels,
//                                                 sizeof(char *) );

//        fDriver->capture.interleave_skip =
//            (unsigned long *)calloc( fDriver->max_nchannels,
//                                     sizeof(unsigned long) );
        }

//    fDriver->clock_sync_data =
//        (ClockSyncStatus *)calloc( fDriver->max_nchannels,
//                                   sizeof(ClockSyncStatus) );

    float period_usecs = ( ( (float)fArgs.frames_per_interrupt )
        / fDriver->frame_rate ) * 1000000.0f;

    fDriver->poll_timeout_msecs = (int)floor( 1.5f * period_usecs / 1000.0f );

    return 0;
    }

    void JackIoAudioDriver::setup_io_function_pointers()
    {
    if( fDriver->playback.handle )
        {
        if( SND_PCM_FMT_FLOAT_LE == fDriver->playback.setup.format.format )
            {
            fDriver->playback.write = sample_move_dS_floatLE;
            }
        else
            {
            ssize_t bytes = fDriver->playback.sample_bytes();
            switch( bytes )
                {
                case 2:
                    switch( fArgs.dither )
                        {
                        case Rectangular:
                            jack_log( "Rectangular dithering at 16 bits" );
                            fDriver->playback.write =
                                fDriver->quirk_bswap ?
                                    sample_move_dither_rect_d16_sSs :
                                    sample_move_dither_rect_d16_sS;
                            break;

                        case Triangular:
                            jack_log( "Triangular dithering at 16 bits" );
                            fDriver->playback.write =
                                fDriver->quirk_bswap ?
                                    sample_move_dither_tri_d16_sSs :
                                    sample_move_dither_tri_d16_sS;
                            break;

                        case Shaped:
                            jack_log( "Noise-shaped dithering at 16 bits" );
                            fDriver->playback.write =
                                fDriver->quirk_bswap ?
                                    sample_move_dither_shaped_d16_sSs :
                                    sample_move_dither_shaped_d16_sS;
                            break;

                        default:
                            fDriver->playback.write =
                                fDriver->quirk_bswap ?
                                    sample_move_d16_sSs : sample_move_d16_sS;
                            break;
                        }
                    break;

                case 3: /* NO DITHER */
                    fDriver->playback.write =
                        fDriver->quirk_bswap ?
                            sample_move_d24_sSs : sample_move_d24_sS;

                    break;

                case 4: /* NO DITHER */
                    fDriver->playback.write =
                        fDriver->quirk_bswap ?
                            sample_move_d32u24_sSs : sample_move_d32u24_sS;
                    break;

                default:
                    jack_error( "impossible sample width (%d) discovered!",
                                bytes );
                    exit( 1 );
                }
            }
        }

    if( fDriver->capture.handle )
        {
        if( SND_PCM_FMT_FLOAT_LE == fDriver->capture.setup.format.format )
            {
            fDriver->capture.read = sample_move_floatLE_sSs;
            }
        else
            {
            ssize_t bytes = fDriver->capture.sample_bytes();
            switch( bytes )
                {
                case 2:
                    fDriver->capture.read =
                        fDriver->quirk_bswap ?
                            sample_move_dS_s16s : sample_move_dS_s16;
                    break;
                case 3:
                    fDriver->capture.read =
                        fDriver->quirk_bswap ?
                            sample_move_dS_s24s : sample_move_dS_s24;
                    break;
                case 4:
                    fDriver->capture.read =
                        fDriver->quirk_bswap ?
                            sample_move_dS_s32u24s : sample_move_dS_s32u24;
                    break;
                }
            }
        }
    }

    void JackIoAudioDriver::silence_on_channel(
        int chn,
        jack_nframes_t nframes )
    {
    silence_on_channel_no_mark( chn,
                                nframes );
    mark_channel_done( chn );
    }

    void JackIoAudioDriver::silence_on_channel_no_mark(
        int chn,
        jack_nframes_t nframes )
    {
    if( fDriver->playback.setup.format.interleave )
        {
        memset_interleave( fDriver->playback.voices[chn].addr,
                           0,
                           fDriver->playback.sample_bytes() * nframes,
                           fDriver->interleave_unit,
                           fDriver->playback.voices[chn].interleave_skip );
        }
    else
        {
        memset( fDriver->playback.voices[chn].addr,
                0,
                fDriver->playback.sample_bytes() * nframes );
        }
    }

    void JackIoAudioDriver::silence_untouched_channels(
        jack_nframes_t nframes )
    {
    int chn;
    jack_nframes_t buffer_frames = fArgs.frames_per_interrupt
        * fDriver->playback.nperiods;

    for( chn = 0; chn < fDriver->playback.setup.format.voices; chn++ )
        {
        if( bitset_contains( fDriver->channels_not_done,
                             chn ) )
            {
            if( fDriver->playback.voices[chn].silent < buffer_frames )
                {
                silence_on_channel_no_mark( chn,
                                            nframes );
                fDriver->playback.voices[chn].silent += nframes;
                }
            }
        }
    }

    int JackIoAudioDriver::start()
    {
    int err;
    size_t poffset, pavail;
    int chn;

    fDriver->poll_last = 0;
    fDriver->poll_next = 0;

    if( fDriver->playback.handle )
        {
        if( ( err = snd_pcm_playback_prepare( fDriver->playback.handle ) ) < 0 )
            {
            jack_error( "io-audio: prepare error for playback on "
                        "\"%s\" (%s)",
                        fArgs.playback_pcm_name,
                        snd_strerror( err ) );
            return -1;
            }
        }

    if( ( fDriver->capture.handle && fDriver->capture_and_playback_not_synced )
        || !fDriver->playback.handle )
        {
        if( ( err = snd_pcm_capture_prepare( fDriver->capture.handle ) ) < 0 )
            {
            jack_error( "io-audio: prepare error for capture on \"%s\""
                        " (%s)",
                        fArgs.capture_pcm_name,
                        snd_strerror( err ) );
            return -1;
            }
        }

    if( fDriver->do_hw_monitoring )
        {
        fDriver->hw->set_input_monitor_mask( fDriver->input_monitor_mask );
        }

    if( fDriver->playback.handle )
        {
        /* fill playback buffer with zeroes, and mark
         all fragments as having data.
         */

        pavail = fArgs.frames_per_interrupt * fDriver->playback.nperiods;

        if( pavail != fArgs.frames_per_interrupt * fDriver->playback.nperiods )
            {
            jack_error( "io-audio: full buffer not available at start" );
            return -1;
            }

        if( get_channel_addresses( 0,
                                   &pavail,
                                   0,
                                   &poffset ) )
            {
            return -1;
            }

        for( chn = 0; chn < fDriver->playback.setup.format.voices; chn++ )
            {
            silence_on_channel( chn,
                                fArgs.user_nperiods
                                    * fArgs.frames_per_interrupt );
            }

        snd_pcm_write( fDriver->playback.handle,
                       fDriver->playback.buffer,
                       fDriver->playback.frag_bytes() * fArgs.user_nperiods );

        }

    return 0;
    }

    int JackIoAudioDriver::stop()
    {
    int err;

    /* silence all capture port buffers, because we might
     be entering offline mode.
     */
    ClearOutputAux();

    if( fDriver->playback.handle )
        {
        err = snd_pcm_playback_flush( fDriver->playback.handle );
        if( err < 0 )
            {
            jack_error( "io-audio: channel flush for playback "
                        "failed (%s)",
                        snd_strerror( err ) );
            return -1;
            }
        }

    if( !fDriver->playback.handle || fDriver->capture_and_playback_not_synced )
        {
        if( fDriver->capture.handle )
            {
            err = snd_pcm_capture_flush( fDriver->capture.handle );
            if( err < 0 )
                {
                jack_error( "io-audio: channel flush for "
                            "capture failed (%s)",
                            snd_strerror( err ) );
                return -1;
                }
            }
        }

    if( fDriver->do_hw_monitoring )
        {
        fDriver->hw->set_input_monitor_mask( 0 );
        }

    //if (fDriver->midi && !fDriver->xrun_recovery)
    //    (fDriver->midi->stop)(fDriver->midi);

    return 0;
    }

    jack_nframes_t JackIoAudioDriver::wait(
        int *status,
        float *delayed_usecs )
    {
    static int under_gdb = 0;
    ssize_t avail = 0;
    ssize_t capture_avail = 0;
    ssize_t playback_avail = 0;
    bool xrun_detected = false;
    bool need_capture;
    bool need_playback;
    jack_time_t poll_enter;
    jack_time_t poll_ret = 0;

    *status = -1;
    *delayed_usecs = 0;

    need_capture = fDriver->capture.handle ? true : false;

    need_playback = fDriver->playback.handle ? true : false;

    again:

    while( need_playback || need_capture )
        {

        int poll_result;
        unsigned short revents;

        if( need_playback )
            {
            int fd = snd_pcm_file_descriptor( fDriver->playback.handle,
                                              SND_PCM_CHANNEL_PLAYBACK );

            fDriver->pfd[Playback].fd = fd;
            fDriver->pfd[Playback].events = POLLOUT;
            }
        else
            {
            fDriver->pfd[Playback].fd = -1;
            }

        if( need_capture )
            {
            int fd = snd_pcm_file_descriptor( fDriver->capture.handle,
                                              SND_PCM_CHANNEL_CAPTURE );

            fDriver->pfd[Capture].fd = fd;
            fDriver->pfd[Capture].events = POLLIN;
            }
        else
            {
            fDriver->pfd[Capture].fd = -1;
            }

        poll_enter = jack_get_microseconds();

        if( poll_enter > fDriver->poll_next )
            {
            /*
             * This processing cycle was delayed past the
             * next due interrupt!  Do not account this as
             * a wakeup delay:
             */
            fDriver->poll_next = 0;
            fDriver->poll_late++;
            }

        poll_result = poll( fDriver->pfd,
                            SND_PCM_CHANNEL_MAX,
                            fDriver->poll_timeout_msecs );

        if( poll_result < 0 )
            {

            if( errno == EINTR )
                {
                jack_log( "poll interrupt" );
                // this happens mostly when run
                // under gdb, or when exiting due to a signal
                if( under_gdb )
                    {
                    goto again;
                    }
                *status = -2;
                return 0;
                }

            jack_error( "io-audio: poll call failed (%s)",
                        strerror( errno ) );
            *status = -3;
            return 0;

            }

        poll_ret = jack_get_microseconds();

        // JACK2
        JackDriver::CycleTakeBeginTime();
//        SetTimetAux( poll_ret );

        if( need_playback )
            {
            snd_pcm_channel_status_t chstatus;
            chstatus.channel = SND_PCM_CHANNEL_PLAYBACK;
            snd_pcm_channel_status( fDriver->playback.handle,
                                    &chstatus );

            revents = fDriver->pfd[Playback].revents;

            if( revents & POLLERR )
                {
                xrun_detected = true;
                }

            if( revents & POLLOUT )
                {
                need_playback = 0;
#ifdef DEBUG_WAKEUP
                fprintf (stderr, "%" PRIu64
                    " playback stream ready\n",
                    poll_ret);
#endif
                }
            }

        if( need_capture )
            {
            snd_pcm_channel_status_t chstatus;
            chstatus.channel = SND_PCM_CHANNEL_CAPTURE;
            snd_pcm_channel_status( fDriver->playback.handle,
                                    &chstatus );

            revents = fDriver->pfd[Capture].revents;

            if( revents & POLLERR )
                {
                xrun_detected = true;
                }

            if( revents & POLLIN )
                {
                need_capture = 0;
#ifdef DEBUG_WAKEUP
                fprintf (stderr, "%" PRIu64
                    " capture stream ready\n",
                    poll_ret);
#endif
                }
            }

        if( poll_result == 0 )
            {
            jack_error( "io-audio: poll time out, polled for %" PRIu64
                        " usecs",
                        poll_ret - poll_enter );
            *status = -5;
            return 0;
            }

        }

    if( fDriver->capture.handle )
        {
        capture_avail = fDriver->capture.frag_bytes() / fDriver->capture.frame_bytes();
        }
    else
        {
        capture_avail = std::numeric_limits < ssize_t > ::max();
        }

    if( fDriver->playback.handle )
        {
        playback_avail = fDriver->playback.frag_bytes() / fDriver->playback.frame_bytes();
        }
    else
        {
        /* odd, but see min() computation below */
        playback_avail = std::numeric_limits < ssize_t > ::max();
        }

    if( xrun_detected )
        {
        *status = xrun_recovery( delayed_usecs );
        return 0;
        }

    *status = 0;
//        fDriver->last_wait_ust = poll_ret;

    avail = capture_avail < playback_avail ? capture_avail : playback_avail;

#ifdef DEBUG_WAKEUP
    fprintf (stderr, "wakeup complete, avail = %lu, pavail = %lu "
        "cavail = %lu\n",
        avail, playback_avail, capture_avail);
#endif

    /* mark all channels not done for now. read/write will change this */

    bitset_copy( fDriver->channels_not_done,
                 fDriver->channels_done );

    /* constrain the available count to the nearest (round down) number of
     periods.
     */

    return avail - ( avail % fArgs.frames_per_interrupt );
    }

    int JackIoAudioDriver::xrun_recovery(
        float *delayed_usecs )
    {
    snd_pcm_channel_status_t status;
    int res;

    if( fDriver->capture.handle )
        {
        status.channel = SND_PCM_CHANNEL_CAPTURE;
        if( ( res = snd_pcm_channel_status( fDriver->capture.handle,
                                            &status ) ) < 0 )
            {
            jack_error( "status error: %s",
                        snd_strerror( res ) );
            }
        }
    else
        {
        status.channel = SND_PCM_CHANNEL_PLAYBACK;
        if( ( res = snd_pcm_channel_status( fDriver->playback.handle,
                                            &status ) ) < 0 )
            {
            jack_error( "status error: %s",
                        snd_strerror( res ) );
            }
        }

    if( status.status == SND_PCM_STATUS_READY )
        {
        jack_log( "**** ioaudio_pcm: pcm in suspended state, resuming it" );
        if( fDriver->capture.handle )
            {
            if( ( res = snd_pcm_capture_prepare( fDriver->capture.handle ) )
                < 0 )
                {
                jack_error( "error preparing after suspend: %s",
                            snd_strerror( res ) );
                }
            }
        else
            {
            if( ( res = snd_pcm_playback_prepare( fDriver->playback.handle ) )
                < 0 )
                {
                jack_error( "error preparing after suspend: %s",
                            snd_strerror( res ) );
                }
            }
        }

    if( status.status == SND_PCM_STATUS_OVERRUN
        && fDriver->process_count > XRUN_REPORT_DELAY )
        {
        struct timeval now, diff, tstamp;
        fDriver->xrun_count++;
        gettimeofday( &now,
                      NULL );
        tstamp = status.stop_time;
        timersub( &now,
                  &tstamp,
                  &diff );
        *delayed_usecs = diff.tv_sec * 1000000.0 + diff.tv_usec;
        jack_log( "**** ioaudio_pcm: xrun of at least %.3f msecs",
                  *delayed_usecs / 1000.0 );
        }

    if( restart() )
        {
        return -1;
        }
    return 0;
    }

}
// end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

    static int dither_opt(
        char c,
        DitherAlgorithm* dither )
    {
    switch( c )
        {
        case '-':
        case 'n':
            *dither = None;
            break;

        case 'r':
            *dither = Rectangular;
            break;

        case 's':
            *dither = Shaped;
            break;

        case 't':
            *dither = Triangular;
            break;

        default:
            fprintf( stderr,
                     "io-audio driver: illegal dithering mode %c\n",
                     c );
            return -1;
        }
    return 0;
    }

    static jack_driver_param_constraint_desc_t *
    enum_ioaudio_devices()
    {
    snd_ctl_t * handle;
    snd_ctl_hw_info_t hwinfo;
    snd_pcm_info_t pcminfo;
    int card_no = -1;
    jack_driver_param_value_t card_id;
    jack_driver_param_value_t device_id;
    char description[64];
    uint32_t device_no;
    bool has_capture;
    bool has_playback;
    jack_driver_param_constraint_desc_t * constraint_ptr;
    uint32_t array_size = 0;

    constraint_ptr = NULL;

    int cards_over = 0;
    int numcards = snd_cards_list( NULL,
                                   0,
                                   &cards_over );
    int* cards = new int[cards_over];
    numcards = snd_cards_list( cards,
                               cards_over,
                               &cards_over );

    for( int c = 0; c < numcards; ++c )
        {
        card_no = cards[c];

        if( snd_ctl_open( &handle,
                          card_no ) >= 0 && snd_ctl_hw_info( handle,
                                                             &hwinfo ) >= 0 )
            {
            strncpy( card_id.str,
                     hwinfo.id,
                     sizeof( card_id.str ) );
            strncpy( description,
                     hwinfo.longname,
                     sizeof( description ) );
            if( !jack_constraint_add_enum( &constraint_ptr,
                                           &array_size,
                                           &card_id,
                                           description ) )
                goto fail;

            device_no = -1;

            for( device_no = 0; device_no < hwinfo.pcmdevs; ++device_no )
                {
                snprintf( device_id.str,
                          sizeof( device_id.str ),
                          "%s,%d",
                          card_id.str,
                          device_no );

                snd_ctl_pcm_info( handle,
                                  device_no,
                                  &pcminfo );
                has_capture = pcminfo.flags & SND_PCM_INFO_CAPTURE;
                has_playback = pcminfo.flags & SND_PCM_INFO_PLAYBACK;

                if( has_capture && has_playback )
                    {
                    snprintf( description,
                              sizeof( description ),
                              "%s (duplex)",
                              pcminfo.name );
                    }
                else if( has_capture )
                    {
                    snprintf( description,
                              sizeof( description ),
                              "%s (capture)",
                              pcminfo.name );
                    }
                else if( has_playback )
                    {
                    snprintf( description,
                              sizeof( description ),
                              "%s (playback)",
                              pcminfo.name );
                    }
                else
                    {
                    continue;
                    }

                if( !jack_constraint_add_enum( &constraint_ptr,
                                               &array_size,
                                               &device_id,
                                               description ) )
                    goto fail;
                }

            snd_ctl_close( handle );
            }
        }

    delete[] cards;
    return constraint_ptr;

    fail: jack_constraint_free( constraint_ptr );
    delete[] cards;
    return NULL;
    }

    SERVER_EXPORT const jack_driver_desc_t* driver_get_descriptor()
    {
    jack_driver_desc_t * desc;
    jack_driver_desc_filler_t filler;
    jack_driver_param_value_t value;

    desc =
        jack_driver_descriptor_construct( "io-audio",
                                          JackDriverMaster,
                                          "QNX io-audio API based audio backend",
                                          &filler );

    strcpy( value.str,
            "pcmPreferredp" );
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "device",
                                          'd',
                                          JackDriverParamString,
                                          &value,
                                          enum_ioaudio_devices(),
                                          "io-audio device name",
                                          NULL );

    strcpy( value.str,
            "none" );
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "capture",
                                          'C',
                                          JackDriverParamString,
                                          &value,
                                          NULL,
                                          "Provide capture ports.  Optionally set device",
                                          NULL );
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "playback",
                                          'P',
                                          JackDriverParamString,
                                          &value,
                                          NULL,
                                          "Provide playback ports.  Optionally set device",
                                          NULL );

    value.ui = 48000U;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "rate",
                                          'r',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Sample rate",
                                          NULL );

    value.ui = 1024U;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "period",
                                          'p',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Frames per period",
                                          NULL );

    value.ui = 2U;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "nperiods",
                                          'n',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Number of periods of playback latency",
                                          NULL );

    value.i = 0;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "hwmon",
                                          'H',
                                          JackDriverParamBool,
                                          &value,
                                          NULL,
                                          "Hardware monitoring, if available",
                                          NULL );

    value.i = 0;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "hwmeter",
                                          'M',
                                          JackDriverParamBool,
                                          &value,
                                          NULL,
                                          "Hardware metering, if available",
                                          NULL );

    value.i = 1;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "duplex",
                                          'D',
                                          JackDriverParamBool,
                                          &value,
                                          NULL,
                                          "Provide both capture and playback ports",
                                          NULL );

    value.i = 0;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "softmode",
                                          's',
                                          JackDriverParamBool,
                                          &value,
                                          NULL,
                                          "Soft-mode, no xrun handling",
                                          NULL );

    value.i = 0;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "monitor",
                                          'm',
                                          JackDriverParamBool,
                                          &value,
                                          NULL,
                                          "Provide monitor ports for the output",
                                          NULL );

    value.c = 'n';
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "dither",
                                          'z',
                                          JackDriverParamChar,
                                          &value,
                                          jack_constraint_compose_enum_char(
                                                                             JACK_CONSTRAINT_FLAG_STRICT
                                                                                 | JACK_CONSTRAINT_FLAG_FAKE_VALUE,
                                                                             dither_constraint_descr_array ),
                                          "Dithering mode",
                                          NULL );

    value.ui = 0;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "inchannels",
                                          'i',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Number of capture channels (defaults to hardware max)",
                                          NULL );
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "outchannels",
                                          'o',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Number of playback channels (defaults to hardware max)",
                                          NULL );

    value.i = false;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "shorts",
                                          'S',
                                          JackDriverParamBool,
                                          &value,
                                          NULL,
                                          "Try 16-bit samples before 32-bit",
                                          NULL );

    value.ui = 0;
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "input-latency",
                                          'I',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Extra input latency (frames)",
                                          NULL );
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "output-latency",
                                          'O',
                                          JackDriverParamUInt,
                                          &value,
                                          NULL,
                                          "Extra output latency (frames)",
                                          NULL );

    strcpy( value.str,
            "none" );
    jack_driver_descriptor_add_parameter( desc,
                                          &filler,
                                          "midi-driver",
                                          'X',
                                          JackDriverParamString,
                                          &value,
                                          jack_constraint_compose_enum_str(
                                                                            JACK_CONSTRAINT_FLAG_STRICT
                                                                                | JACK_CONSTRAINT_FLAG_FAKE_VALUE,
                                                                            midi_constraint_descr_array ),
                                          "io-audio MIDI driver",
                                          NULL );

    return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(
        Jack::JackLockedEngine* engine,
        Jack::JackSynchro* table,
        const JSList* params )
    {
    ioaudio_driver_args_t args;
    args.srate = 48000;
    args.frames_per_interrupt = 1024;
    args.user_nperiods = 2;
    args.playback_pcm_name = "pcmPreferredp";
    args.capture_pcm_name = "pcmPreferredc";
    args.hw_monitoring = false;
    args.hw_metering = false;
    args.capture = false;
    args.playback = false;
    args.soft_mode = false;
    args.monitor = false;
    args.dither = None;
    args.user_capture_nchnls = 0;
    args.user_playback_nchnls = 0;
    args.shorts_first = false;
    args.systemic_input_latency = 0;
    args.systemic_output_latency = 0;
    args.midi_driver = "none";

    const JSList * node;
    const jack_driver_param_t * param;

    for( node = params; node; node = jack_slist_next( node ) )
        {
        param = (const jack_driver_param_t *)node->data;

        switch( param->character )
            {

            case 'C':
                if( strcmp( param->value.str,
                            "none" ) != 0 )
                    {
                    args.capture = true;
                    args.capture_pcm_name = strdup( param->value.str );
                    jack_log( "capture device %s",
                              args.capture_pcm_name );
                    }
                break;

            case 'P':
                if( strcmp( param->value.str,
                            "none" ) != 0 )
                    {
                    args.playback = true;
                    args.playback_pcm_name = strdup( param->value.str );
                    jack_log( "playback device %s",
                              args.playback_pcm_name );
                    }
                break;

            case 'D':
                args.playback = true;
                args.capture = true;
                break;

            case 'd':
                if( strcmp( param->value.str,
                            "none" ) != 0 )
                    {
                    args.playback = true;
                    args.playback_pcm_name = strdup( param->value.str );
                    args.capture = true;
                    args.capture_pcm_name = strdup( param->value.str );
                    jack_log( "playback device %s",
                              args.playback_pcm_name );
                    jack_log( "capture device %s",
                              args.capture_pcm_name );
                    }
                break;

            case 'H':
                args.hw_monitoring = param->value.i;
                break;

            case 'm':
                args.monitor = param->value.i;
                break;

            case 'M':
                args.hw_metering = param->value.i;
                break;

            case 'r':
                args.srate = param->value.ui;
                jack_log( "apparent rate = %d",
                          args.srate );
                break;

            case 'p':
                args.frames_per_interrupt = param->value.ui;
                jack_log( "frames per period = %d",
                          args.frames_per_interrupt );
                break;

            case 'n':
                args.user_nperiods = param->value.ui;
                if( args.user_nperiods < 2 )
                    { /* enforce minimum value */
                    args.user_nperiods = 2;
                    }
                break;

            case 's':
                args.soft_mode = param->value.i;
                break;

            case 'z':
                if( dither_opt( param->value.c,
                                &args.dither ) )
                    {
                    return NULL;
                    }
                break;

            case 'i':
                args.user_capture_nchnls = param->value.ui;
                break;

            case 'o':
                args.user_playback_nchnls = param->value.ui;
                break;

            case 'S':
                args.shorts_first = param->value.i;
                break;

            case 'I':
                args.systemic_input_latency = param->value.ui;
                break;

            case 'O':
                args.systemic_output_latency = param->value.ui;
                break;

            case 'X':
                args.midi_driver = strdup( param->value.str );
                break;
            }
        }

    /* duplex is the default */
    if( !args.capture && !args.playback )
        {
        args.capture = true;
        args.playback = true;
        }

    Jack::JackIoAudioDriver* ioaudio_driver =
        new Jack::JackIoAudioDriver( "system",
                                     "ioaudio_pcm",
                                     engine,
                                     table );
    Jack::JackDriverClientInterface* threaded_driver =
        new Jack::JackThreadedDriver( ioaudio_driver );
    // Special open for io-audio driver
    if( ioaudio_driver->Open( args ) == 0 )
        {
        return threaded_driver;
        }
    else
        {
        delete threaded_driver; // Delete the decorated driver
        return NULL;
        }
    }

#ifdef __cplusplus
}
#endif

