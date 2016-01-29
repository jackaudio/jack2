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
#include "memops.h"

#include "JackIoAudioDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackPort.h"
#include "JackGraphManager.h"
#include "JackLockedEngine.h"
#include "JackPosixThread.h"
#include "JackCompilerDeps.h"
#include "JackServerGlobals.h"

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

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// PROTOTYPES
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATIONS
///////////////////////////////////////////////////////////////////////////////

    class generic_hardware: public jack_hardware_t
    {
        virtual ~generic_hardware()
        {
        }

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

    JackIoAudioDriver::JackIoAudioDriver(
        Args args,
        JackLockedEngine* engine,
        JackSynchro* table ) :
            JackAudioDriver( args.jack_name,
                             args.jack_alias,
                             engine,
                             table ),
            fArgs( args )
    {

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
    }

    JackIoAudioDriver::~JackIoAudioDriver()
    {
    //if (fmidi)
    //        (midi->destroy)(midi);

    if( capture.handle )
        {
        snd_pcm_close( capture.handle );
        capture.handle = 0;
        }

    if( playback.handle )
        {
        snd_pcm_close( playback.handle );
        capture.handle = 0;
        }

    if( hw )
        {
        hw->release();
        delete hw;
        hw = 0;
        }

    delete[] playback.voices;
    delete[] capture.voices;
    }

    int JackIoAudioDriver::Attach()
    {
    JackPort* port;
    jack_port_id_t port_index;
    unsigned long port_flags = (unsigned long) CaptureDriverFlags;
    char name[REAL_JACK_PORT_NAME_SIZE];
    char alias[REAL_JACK_PORT_NAME_SIZE];

    assert( fCaptureChannels < DRIVER_PORT_NUM );
    assert( fPlaybackChannels < DRIVER_PORT_NUM );

    if( has_hw_monitoring )
        port_flags |= JackPortCanMonitor;

    // io-audio driver may have changed the values
    JackAudioDriver::SetBufferSize( fArgs.frames_per_interrupt );
    JackAudioDriver::SetSampleRate( frame_rate );

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

    update_latencies();

    //if (midi) {
    //    int err = (midi->attach)(midi);
    //    if (err)
    //        jack_error ("io-audio: cannot attach MIDI: %d", err);
    //}

    return 0;
    }

    int JackIoAudioDriver::Close()
    {
    // Generic audio driver close
    int res = JackAudioDriver::Close();

//    destroy();

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
    //if (midi)
    //    (midi->detach)(midi);

    return JackAudioDriver::Detach();
    }

    int JackIoAudioDriver::Open()
    {
    int err;

    // Generic JackAudioDriver Open
    if( JackAudioDriver::Open( fArgs.frames_per_interrupt,
                               fArgs.srate,
                               fArgs.capture,
                               fArgs.playback,
                               fArgs.user_capture_nchnls,
                               fArgs.user_playback_nchnls,
                               fArgs.monitor,
                               fArgs.capture_pcm_name,
                               fArgs.playback_pcm_name,
                               fArgs.systemic_input_latency,
                               fArgs.systemic_output_latency ) != 0 )
        {
        return -1;
        }

    //ioaudio_midi_t *midi = 0;
    //if (strcmp(midi_driver_name, "seq") == 0)
    //    midi = ioaudio_seqmidi_new((jack_client_t*)this, 0);
    //else if (strcmp(midi_driver_name, "raw") == 0)
    //    midi = ioaudio_rawmidi_new((jack_client_t*)this);

    //midi = midi_driver;

    err = check_card_type();
    if( err )
        {
        return err;
        }

    if( JackServerGlobals::on_device_acquire != NULL )
        {
        if( fArgs.capture )
            {
            if( !JackServerGlobals::on_device_acquire( fArgs.capture_pcm_name ) )
                {
                jack_error( "Audio device %s cannot be acquired...",
                            fArgs.capture_pcm_name );
                fArgs.capture = false;
                }
            }

        if( fArgs.playback )
            {
            if( !JackServerGlobals::on_device_acquire( fArgs.playback_pcm_name ) )
                {
                jack_error( "Audio device %s cannot be acquired...",
                            fArgs.playback_pcm_name );
                fArgs.playback = false;
                if( fArgs.capture )
                    {
                    JackServerGlobals::on_device_release( fArgs.capture_pcm_name );
                    }
                }
            }
        }

    hw_specific();

    if( fArgs.playback )
        {
        err =
            snd_pcm_open_name( &playback.handle,
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
                    return EBUSY;

                case EPERM:
                    jack_error( "you do not have permission to open "
                                "the audio device \"%s\" for playback",
                                fArgs.playback_pcm_name );
                    return EPERM;
                }

            playback.handle = NULL;
            }

        if( playback.handle )
            {
            snd_pcm_nonblock_mode( playback.handle,
                                   0 );
            }
        else
            {

            /* they asked for playback, but we can't do it */

            jack_error( "io-audio: Cannot open PCM device %s for "
                        "playback. Falling back to capture-only"
                        " mode",
                        fArgs.playback_pcm_name );
            fArgs.playback = false;
            }

        }

    if( fArgs.capture )
        {
        err = snd_pcm_open_name( &capture.handle,
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
                    break;

                case EPERM:
                    jack_error( "you do not have permission to open "
                                "the audio device \"%s\" for capture",
                                fArgs.capture_pcm_name );
                    break;
                }

            capture.handle = NULL;
            }

        if( capture.handle )
            {
            snd_pcm_nonblock_mode( capture.handle,
                                   0 );
            }
        else
            {

            /* they asked for capture, but we can't do it */

            jack_error( "io-audio: Cannot open PCM device %s for "
                        "capture. Falling back to playback-only"
                        " mode",
                        fArgs.capture_pcm_name );

            fArgs.capture = false;
            }
        }

    if( playback.handle == NULL && capture.handle == NULL )
        {
        /* can't do anything */
        return err;
        }

    err = set_parameters();
    if( 0 != err )
        {
        return err;
        }

    capture_and_playback_not_synced = false;

    if( capture.handle && playback.handle )
        {
        if( snd_pcm_link( playback.handle,
                          capture.handle ) != 0 )
            {
            capture_and_playback_not_synced = true;
            }
        }

    if( err )
        {
        JackAudioDriver::Close();
        return -1;
        }
    else
        {
        // io-audio driver may have changed the in/out values
        fCaptureChannels = capture.setup.format.voices;
        fPlaybackChannels = playback.setup.format.voices;
        return 0;
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

    int JackIoAudioDriver::SetBufferSize(
        jack_nframes_t buffer_size )
    {
    jack_log( "JackIoAudioDriver::SetBufferSize %ld",
              buffer_size );
    fArgs.frames_per_interrupt = buffer_size;
    int res = set_parameters();

    if( res == 0 )
        {
        // update fEngineControl and fGraphManager
        JackAudioDriver::SetBufferSize( buffer_size ); // Generic change, never fails
        // io-audio specific
        update_latencies();
        }
    else
        {
        // Restore old values
        fArgs.frames_per_interrupt = fEngineControl->fBufferSize;
        set_parameters();
        }

    return res;
    }

    int JackIoAudioDriver::Start()
    {
    int err;
    int res = 0;

    res = JackAudioDriver::Start();
    if( res < 0 )
        {
        return res;
        }

    poll_last = 0;
    poll_next = 0;

    if( playback.handle )
        {
        if( ( err = snd_pcm_playback_prepare( playback.handle ) ) < 0 )
            {
            jack_error( "io-audio: prepare error for playback on "
                        "\"%s\" (%s)",
                        fArgs.playback_pcm_name,
                        snd_strerror( err ) );
            res = -1;
            }
        }

    if( ( capture.handle && capture_and_playback_not_synced )
        || !playback.handle )
        {
        if( ( err = snd_pcm_capture_prepare( capture.handle ) ) < 0 )
            {
            jack_error( "io-audio: prepare error for capture on \"%s\""
                        " (%s)",
                        fArgs.capture_pcm_name,
                        snd_strerror( err ) );
            res = -1;
            }
        }

    res = JackIoAudioDriver::Write();

    if( res < 0 )
        {
        JackAudioDriver::Stop();
        }

    return res;
    }

    int JackIoAudioDriver::Stop()
    {
    int err;
    int res = 0;

    /* silence all capture port buffers, because we might
     be entering offline mode.
     */
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

    if( playback.handle )
        {
        err = snd_pcm_playback_flush( playback.handle );
        if( err < 0 )
            {
            jack_error( "io-audio: channel flush for playback "
                        "failed (%s)",
                        snd_strerror( err ) );
            res = -1;
            }
        }

    if( !playback.handle || capture_and_playback_not_synced )
        {
        if( capture.handle )
            {
            err = snd_pcm_capture_flush( capture.handle );
            if( err < 0 )
                {
                jack_error( "io-audio: channel flush for "
                            "capture failed (%s)",
                            snd_strerror( err ) );
                res = -1;
                }
            }
        }

    if( do_hw_monitoring )
        {
        hw->set_input_monitor_mask( 0 );
        }

    //if (midi && !xrun_recovery)
    //    (midi->stop)(midi);

    res = JackAudioDriver::Stop();

    return res;
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

    process_count++;

    if( !playback.handle )
        {
        return 0;
        }

    if( nframes > fArgs.frames_per_interrupt )
        {
        return -1;
        }

    //if (midi)
    //    (midi->write)(midi, nframes);

    nwritten = 0;
    contiguous = 0;
    orig_nframes = nframes;

    /* check current input monitor request status */

    input_monitor_mask = 0;

    for( int chn = 0; chn < fCaptureChannels; chn++ )
        {
        JackPort* port = fGraphManager->GetPort( fCapturePortList[chn] );
        if( port->MonitoringInput() )
            {
            input_monitor_mask |= ( 1 << chn );
            }
        }

    if( do_hw_monitoring )
        {
        if( hw->input_monitor_mask != input_monitor_mask )
            {
            hw->set_input_monitor_mask( input_monitor_mask );
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

        for( int chn = 0; chn < fPlaybackChannels; chn++ )
            {
            // Output ports
            if( fGraphManager->GetConnectionsNum( fPlaybackPortList[chn] ) > 0 )
                {
                jack_default_audio_sample_t* buf =
                    (jack_default_audio_sample_t*)fGraphManager->GetBuffer( fPlaybackPortList[chn],
                                                                            orig_nframes );
                playback.write( playback.voices[chn].addr,
                                buf + nwritten,
                                contiguous,
                                playback.voices[chn].interleave_skip,
                                &playback.voices[chn].dither_state );

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
            else
                {
                memset_interleave( playback.voices[chn].addr,
                                   0,
                                   playback.sample_bytes() * contiguous,
                                   interleave_unit,
                                   playback.voices[chn].interleave_skip );
                }

            }

        err = snd_pcm_write( playback.handle,
                             playback.buffer,
                             playback.frag_bytes() );
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
    quirk_bswap = snd_pcm_format_big_endian(formats[format]);
#elif defined(SND_BIG_ENDIAN)
    quirk_bswap = snd_pcm_format_little_endian( formats[format] );
#else
    quirk_bswap = 0;
#endif

    params->format.rate = frame_rate;

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

    int JackIoAudioDriver::get_channel_addresses(
        size_t *capture_avail,
        size_t *playback_avail,
        size_t *capture_offset,
        size_t *playback_offset )
    {
    int chn;

    if( capture_avail )
        {
        ssize_t samp_bytes = capture.sample_bytes();
        ssize_t frame_bytes = capture.frame_bytes();
        for( chn = 0; chn < capture.setup.format.voices; chn++ )
            {
            capture.voices[chn].addr = (char*)capture.buffer + chn * samp_bytes;
            capture.voices[chn].interleave_skip = frame_bytes;
            }
        }

    if( playback_avail )
        {
        ssize_t samp_bytes = playback.sample_bytes();
        ssize_t frame_bytes = playback.frame_bytes();
        for( chn = 0; chn < playback.setup.format.voices; chn++ )
            {
            playback.voices[chn].addr = (char*)playback.buffer
                + chn * samp_bytes;
            playback.voices[chn].interleave_skip = frame_bytes;

            }
        }

    return 0;
    }

    int JackIoAudioDriver::hw_specific()
    {
    hw = new Jack::generic_hardware;
    if( NULL == hw )
        {
        return -ENOMEM;
        }

    if( hw->capabilities & Cap_HardwareMonitoring )
        {
        has_hw_monitoring = true;
        /* XXX need to ensure that this is really false or
         * true or whatever*/
        do_hw_monitoring = fArgs.hw_monitoring;
        }
    else
        {
        has_hw_monitoring = false;
        do_hw_monitoring = false;
        }

    if( hw->capabilities & Cap_ClockLockReporting )
        {
        has_clock_sync_reporting = true;
        }
    else
        {
        has_clock_sync_reporting = false;
        }

    if( hw->capabilities & Cap_HardwareMetering )
        {
        has_hw_metering = true;
        do_hw_metering = fArgs.hw_metering;
        }
    else
        {
        has_hw_metering = false;
        do_hw_metering = false;
        }

    return 0;
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

    //if (midi)
    //    (midi->read)(midi, nframes);

    if( !capture.handle )
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

        err = snd_pcm_read( capture.handle,
                            capture.buffer,
                            capture.frag_bytes() );
        if( err < 0 )
            {
            jack_error( "io-audio: could not complete read of %"
                        PRIu32 " frames: error = %d",
                        contiguous,
                        err );
            return -1;
            }

        for( int chn = 0; chn < fCaptureChannels; chn++ )
            {
            if( fGraphManager->GetConnectionsNum( fCapturePortList[chn] ) > 0 )
                {
                jack_default_audio_sample_t* buf =
                    (jack_default_audio_sample_t*)fGraphManager->GetBuffer( fCapturePortList[chn],
                                                                            orig_nframes );
                capture.read( buf + nread,
                              capture.voices[chn].addr,
                              contiguous,
                              capture.voices[chn].interleave_skip );
                }
            }

        nframes -= contiguous;
        nread += contiguous;
        }

    return 0;
    }

    int JackIoAudioDriver::set_parameters()
    {
    ssize_t p_period_size = 0;
    size_t c_period_size = 0;
    unsigned int pr = 0;
    unsigned int cr = 0;
    int err;

    frame_rate = fArgs.srate;

    jack_log( "configuring for %" PRIu32 "Hz, period = %"
              PRIu32 " frames (%.1f ms), buffer = %" PRIu32 " periods",
              fArgs.srate,
              fArgs.frames_per_interrupt,
              ( ( (float)fArgs.frames_per_interrupt / (float)fArgs.srate )
                  * 1000.0f ),
              fArgs.user_nperiods );

    if( capture.handle )
        {
        memset( &capture.params,
                0,
                sizeof(snd_pcm_channel_params_t) );
        capture.params.channel = SND_PCM_CHANNEL_CAPTURE;
        if( configure_stream( fArgs.capture_pcm_name,
                              "capture",
                              capture.handle,
                              &capture.params,
                              &capture.nperiods ) )
            {
            jack_error( "io-audio: cannot configure capture channel" );
            return -1;
            }
        capture.setup.channel = SND_PCM_CHANNEL_CAPTURE;
        snd_pcm_channel_setup( capture.handle,
                               &capture.setup );
        cr = capture.setup.format.rate;

        /* check the fragment size, since thats non-negotiable */
        c_period_size = capture.setup.buf.block.frag_size;

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
        switch( capture.setup.format.format )
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

        if( capture.setup.format.interleave )
            {
            if( ( err = snd_pcm_mmap( capture.handle,
                                      SND_PCM_CHANNEL_CAPTURE,
                                      &capture.mmap,
                                      &capture.buffer ) ) < 0 )
                {
                jack_error( "io-audio: %s: mmap areas info error",
                            fArgs.capture_pcm_name );
                return -1;
                }
            }
        }

    if( playback.handle )
        {
        memset( &playback.params,
                0,
                sizeof(snd_pcm_channel_params_t) );
        playback.params.channel = SND_PCM_CHANNEL_PLAYBACK;

        if( configure_stream( fArgs.playback_pcm_name,
                              "playback",
                              playback.handle,
                              &playback.params,
                              &playback.nperiods ) )
            {
            jack_error( "io-audio: cannot configure playback channel" );
            return -1;
            }

        playback.setup.channel = SND_PCM_CHANNEL_PLAYBACK;
        snd_pcm_channel_setup( playback.handle,
                               &playback.setup );
        pr = playback.setup.format.rate;

        /* check the fragment size, since thats non-negotiable */
        p_period_size = playback.setup.buf.block.frag_size;

        if( p_period_size != playback.frag_bytes() )
            {
            jack_error( "ioaudio_pcm: requested an interrupt every %"
                        PRIu32
                        " frames but got %u frames for playback",
                        fArgs.frames_per_interrupt,
                        p_period_size );
            return -1;
            }

        /* check the sample format */
        switch( playback.setup.format.format )
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

        playback.buffer = malloc( fArgs.user_nperiods * p_period_size );
        if( playback.setup.format.interleave )
            {
            interleave_unit = playback.sample_bytes();
            }
        else
            {
            interleave_unit = 0; /* NOT USED */
            }
        }

    /* check the rate, since thats rather important */
    if( capture.handle && playback.handle )
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
        if( cr != frame_rate && pr != frame_rate )
            {
            jack_error( "sample rate in use (%d Hz) does not "
                        "match requested rate (%d Hz)",
                        cr,
                        frame_rate );
            frame_rate = cr;
            }

        }
    else if( capture.handle && cr != frame_rate )
        {
        jack_error( "capture sample rate in use (%d Hz) does not "
                    "match requested rate (%d Hz)",
                    cr,
                    frame_rate );
        frame_rate = cr;
        }
    else if( playback.handle && pr != frame_rate )
        {
        jack_error( "playback sample rate in use (%d Hz) does not "
                    "match requested rate (%d Hz)",
                    pr,
                    frame_rate );
        frame_rate = pr;
        }

    max_nchannels = std::max( playback.setup.format.voices,
                              capture.setup.format.voices );
    user_nchannels = std::min( playback.setup.format.voices,
                               capture.setup.format.voices );

    setup_io_function_pointers();

    if( playback.handle )
        {
        if( playback.voices )
            delete[] playback.voices;
        playback.voices = new Voice[max_nchannels];
        }

    if( capture.handle )
        {
        if( capture.voices )
            delete[] capture.voices;
        capture.voices = new Voice[max_nchannels];
        }

    float period_usecs = ( ( (float)fArgs.frames_per_interrupt ) / frame_rate )
        * 1000000.0f;

    poll_timeout_msecs = (int)floor( 1.5f * period_usecs / 1000.0f );

    return 0;
    }

    void JackIoAudioDriver::setup_io_function_pointers()
    {
    if( playback.handle )
        {
        if( SND_PCM_FMT_FLOAT_LE == playback.setup.format.format )
            {
            playback.write = sample_move_dS_floatLE;
            }
        else
            {
            ssize_t bytes = playback.sample_bytes();
            switch( bytes )
                {
                case 2:
                    switch( fArgs.dither )
                        {
                        case Rectangular:
                            jack_log( "Rectangular dithering at 16 bits" );
                            playback.write =
                                quirk_bswap ?
                                    sample_move_dither_rect_d16_sSs :
                                    sample_move_dither_rect_d16_sS;
                            break;

                        case Triangular:
                            jack_log( "Triangular dithering at 16 bits" );
                            playback.write =
                                quirk_bswap ?
                                    sample_move_dither_tri_d16_sSs :
                                    sample_move_dither_tri_d16_sS;
                            break;

                        case Shaped:
                            jack_log( "Noise-shaped dithering at 16 bits" );
                            playback.write =
                                quirk_bswap ?
                                    sample_move_dither_shaped_d16_sSs :
                                    sample_move_dither_shaped_d16_sS;
                            break;

                        default:
                            playback.write =
                                quirk_bswap ?
                                    sample_move_d16_sSs : sample_move_d16_sS;
                            break;
                        }
                    break;

                case 3: /* NO DITHER */
                    playback.write =
                        quirk_bswap ? sample_move_d24_sSs : sample_move_d24_sS;

                    break;

                case 4: /* NO DITHER */
                    playback.write =
                        quirk_bswap ?
                            sample_move_d32u24_sSs : sample_move_d32u24_sS;
                    break;

                default:
                    jack_error( "impossible sample width (%d) discovered!",
                                bytes );
                    exit( 1 );
                }
            }
        }

    if( capture.handle )
        {
        if( SND_PCM_FMT_FLOAT_LE == capture.setup.format.format )
            {
            capture.read = sample_move_floatLE_sSs;
            }
        else
            {
            ssize_t bytes = capture.sample_bytes();
            switch( bytes )
                {
                case 2:
                    capture.read =
                        quirk_bswap ? sample_move_dS_s16s : sample_move_dS_s16;
                    break;
                case 3:
                    capture.read =
                        quirk_bswap ? sample_move_dS_s24s : sample_move_dS_s24;
                    break;
                case 4:
                    capture.read =
                        quirk_bswap ?
                            sample_move_dS_s32u24s : sample_move_dS_s32u24;
                    break;
                }
            }
        }
    }

    void JackIoAudioDriver::update_latencies()
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

    need_capture = capture.handle ? true : false;

    need_playback = playback.handle ? true : false;

    again:

    while( need_playback || need_capture )
        {

        int poll_result;
        unsigned short revents;

        if( need_playback )
            {
            int fd = snd_pcm_file_descriptor( playback.handle,
                                              SND_PCM_CHANNEL_PLAYBACK );

            pfd[Playback].fd = fd;
            pfd[Playback].events = POLLOUT;
            }
        else
            {
            pfd[Playback].fd = -1;
            }

        if( need_capture )
            {
            int fd = snd_pcm_file_descriptor( capture.handle,
                                              SND_PCM_CHANNEL_CAPTURE );

            pfd[Capture].fd = fd;
            pfd[Capture].events = POLLIN;
            }
        else
            {
            pfd[Capture].fd = -1;
            }

        poll_enter = GetMicroSeconds();

        if( poll_enter > poll_next )
            {
            /*
             * This processing cycle was delayed past the
             * next due interrupt!  Do not account this as
             * a wakeup delay:
             */
            poll_next = 0;
            poll_late++;
            }

        poll_result = poll( pfd,
                            SND_PCM_CHANNEL_MAX,
                            poll_timeout_msecs );

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

        poll_ret = GetMicroSeconds();

        // JACK2
        JackDriver::CycleTakeBeginTime();

        if( need_playback )
            {
            snd_pcm_channel_status_t chstatus;
            chstatus.channel = SND_PCM_CHANNEL_PLAYBACK;
            snd_pcm_channel_status( playback.handle,
                                    &chstatus );

            revents = pfd[Playback].revents;

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
            snd_pcm_channel_status( playback.handle,
                                    &chstatus );

            revents = pfd[Capture].revents;

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

    if( capture.handle )
        {
        capture_avail = capture.frag_bytes() / capture.frame_bytes();
        }
    else
        {
        capture_avail = std::numeric_limits < ssize_t > ::max();
        }

    if( playback.handle )
        {
        playback_avail = playback.frag_bytes() / playback.frame_bytes();
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

    avail = capture_avail < playback_avail ? capture_avail : playback_avail;

#ifdef DEBUG_WAKEUP
    fprintf (stderr, "wakeup complete, avail = %lu, pavail = %lu "
        "cavail = %lu\n",
        avail, playback_avail, capture_avail);
#endif

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

    if( capture.handle )
        {
        status.channel = SND_PCM_CHANNEL_CAPTURE;
        if( ( res = snd_pcm_channel_status( capture.handle,
                                            &status ) ) < 0 )
            {
            jack_error( "status error: %s",
                        snd_strerror( res ) );
            }
        }
    else
        {
        status.channel = SND_PCM_CHANNEL_PLAYBACK;
        if( ( res = snd_pcm_channel_status( playback.handle,
                                            &status ) ) < 0 )
            {
            jack_error( "status error: %s",
                        snd_strerror( res ) );
            }
        }

    if( status.status == SND_PCM_STATUS_READY )
        {
        jack_log( "**** ioaudio_pcm: pcm in suspended state, resuming it" );
        if( capture.handle )
            {
            if( ( res = snd_pcm_capture_prepare( capture.handle ) ) < 0 )
                {
                jack_error( "error preparing after suspend: %s",
                            snd_strerror( res ) );
                }
            }
        else
            {
            if( ( res = snd_pcm_playback_prepare( playback.handle ) ) < 0 )
                {
                jack_error( "error preparing after suspend: %s",
                            snd_strerror( res ) );
                }
            }
        }

    if( status.status == SND_PCM_STATUS_OVERRUN
        && process_count > XRUN_REPORT_DELAY )
        {
        struct timeval now, diff, tstamp;
        xrun_count++;
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

    in_xrun_recovery = 1;
    if( ( res = Stop() ) == 0 )
        {
        res = Start();
        }
    in_xrun_recovery = 0;

    //if (res && midi)
    //    (midi->stop)(midi);

    if( 0 != res )
        {
        return -1;
        }
    return 0;
    }

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
                              card_no ) >= 0
                && snd_ctl_hw_info( handle,
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

        SERVER_EXPORT JackDriverClientInterface* driver_initialize(
            JackLockedEngine* engine,
            JackSynchro* table,
            const JSList* params )
        {
        JackIoAudioDriver::Args args;
        args.jack_name = "system";
        args.jack_alias = "ioaudio_pcm";
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

        JackIoAudioDriver* ioaudio_driver = new JackIoAudioDriver( args,
                                                                   engine,
                                                                   table );
        JackDriverClientInterface* threaded_driver =
            new JackThreadedDriver( ioaudio_driver );
        // Special open for io-audio driver
        if( ioaudio_driver->Open() == 0 )
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

}
// end of namespace
