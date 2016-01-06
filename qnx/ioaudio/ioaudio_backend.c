/* -*- mode: c; c-file-style: "linux"; -*- */
/*
 Copyright (C) 2001 Paul Davis

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

#include "ioaudio_backend.h"
#include "generic.h"
#include "memops.h"
#include "JackError.h"

//#include "ioaudio_midi_impl.h"

extern void store_work_time(
    int );
extern void store_wait_time(
    int );
extern void show_wait_times();
extern void show_work_times();

#undef DEBUG_WAKEUP

char* strcasestr(
    const char* haystack,
    const char* needle );

/* Delay (in process calls) before jackd will report an xrun */
#define XRUN_REPORT_DELAY 0

void jack_driver_init(
    jack_driver_t *driver )
{
    memset( driver,
            0,
            sizeof( *driver ) );
}

void jack_driver_nt_init(
    jack_driver_nt_t * driver )
{
    memset( driver,
            0,
            sizeof( *driver ) );
}

static void ioaudio_driver_release_channel_dependent_memory(
    ioaudio_driver_t *driver )
{
    bitset_destroy( &driver->channels_done );
    bitset_destroy( &driver->channels_not_done );

    if( driver->playback_addr )
        {
        free( driver->playback_addr );
        driver->playback_addr = 0;
        }

    if( driver->capture_addr )
        {
        free( driver->capture_addr );
        driver->capture_addr = 0;
        }

    if( driver->playback_interleave_skip )
        {
        free( driver->playback_interleave_skip );
        driver->playback_interleave_skip = NULL;
        }

    if( driver->capture_interleave_skip )
        {
        free( driver->capture_interleave_skip );
        driver->capture_interleave_skip = NULL;
        }

    if( driver->silent )
        {
        free( driver->silent );
        driver->silent = 0;
        }

    if( driver->dither_state )
        {
        free( driver->dither_state );
        driver->dither_state = 0;
        }
}

static int ioaudio_driver_check_capabilities(
    ioaudio_driver_t *driver )
{
    return 0;
}

char*
get_control_device_name(
    const char * device_name )
{
    char * ctl_name;
    const char * comma;

    /* the user wants a hw or plughw device, the ctl name
     * should be hw:x where x is the card identification.
     * We skip the subdevice suffix that starts with comma */

    if( strncasecmp( device_name,
                     "plughw:",
                     7 ) == 0 )
        {
        /* skip the "plug" prefix" */
        device_name += 4;
        }

    comma = strchr( device_name,
                    ',' );
    if( comma == NULL )
        {
        ctl_name = strdup( device_name );
        if( ctl_name == NULL )
            {
            jack_error( "strdup(\"%s\") failed.",
                        device_name );
            }
        }
    else
        {
        ctl_name = strdup( device_name );
        if( ctl_name == NULL )
            {
            jack_error( "strdup(\"%s\") failed.",
                        device_name );
            return NULL;
            }
        ctl_name[comma - device_name] = '\0';
        }

    return ctl_name;
}

static int ioaudio_driver_check_card_type(
    ioaudio_driver_t *driver )
{
    int err;
    snd_ctl_hw_info_t card_info;
    int play_card;
    int cap_card;

    play_card = snd_card_name( driver->ioaudio_name_playback );
    cap_card = snd_card_name( driver->ioaudio_name_capture );

    // XXX: I don't know the "right" way to do this. Which to use
    // driver->ioaudio_name_playback or driver->ioaudio_name_capture.
    if( ( err = snd_ctl_open( &driver->ctl_handle,
                              play_card ) ) < 0 )
        {
        jack_error( "control open %d:\"%s\" (%s)",
                    play_card,
                    driver->ioaudio_name_playback,
                    snd_strerror( err ) );
        }
    else if( ( err = snd_ctl_hw_info( driver->ctl_handle,
                                      &card_info ) ) < 0 )
        {
        jack_error( "control hardware info %d:\"%s\" (%s)",
                    play_card,
                    driver->ioaudio_name_playback,
                    snd_strerror( err ) );
        snd_ctl_close( driver->ctl_handle );
        }

    driver->ioaudio_driver = strdup( card_info.longname );

    return ioaudio_driver_check_capabilities( driver );
}

static int ioaudio_driver_generic_hardware(
    ioaudio_driver_t *driver )
{
    driver->hw = jack_ioaudio_generic_hw_new( driver );
    return 0;
}

static int ioaudio_driver_hw_specific(
    ioaudio_driver_t *driver,
    int hw_monitoring,
    int hw_metering )
{
    int err;

    if( ( err = ioaudio_driver_generic_hardware( driver ) ) != 0 )
        {
        return err;
        }

    if( driver->hw->capabilities & Cap_HardwareMonitoring )
        {
        driver->has_hw_monitoring = TRUE;
        /* XXX need to ensure that this is really FALSE or
         * TRUE or whatever*/
        driver->hw_monitoring = hw_monitoring;
        }
    else
        {
        driver->has_hw_monitoring = FALSE;
        driver->hw_monitoring = FALSE;
        }

    if( driver->hw->capabilities & Cap_ClockLockReporting )
        {
        driver->has_clock_sync_reporting = TRUE;
        }
    else
        {
        driver->has_clock_sync_reporting = FALSE;
        }

    if( driver->hw->capabilities & Cap_HardwareMetering )
        {
        driver->has_hw_metering = TRUE;
        driver->hw_metering = hw_metering;
        }
    else
        {
        driver->has_hw_metering = FALSE;
        driver->hw_metering = FALSE;
        }

    return 0;
}

static void ioaudio_driver_setup_io_function_pointers(
    ioaudio_driver_t *driver )
{
    if( driver->playback_handle )
        {
        if( SND_PCM_FMT_FLOAT_LE == driver->playback_setup.format.format )
            {
            driver->write_via_copy = sample_move_dS_floatLE;
            }
        else
            {
            ssize_t bytes =
                snd_pcm_format_size(
                                     driver->playback_setup.format.format,
                                     1 );
            switch( bytes )
                {
                case 2:
                    switch( driver->dither )
                        {
                        case Rectangular:
                            jack_info( "Rectangular dithering at 16 bits" );
                            driver->write_via_copy =
                                driver->quirk_bswap ?
                                    sample_move_dither_rect_d16_sSs :
                                    sample_move_dither_rect_d16_sS;
                            break;

                        case Triangular:
                            jack_info( "Triangular dithering at 16 bits" );
                            driver->write_via_copy =
                                driver->quirk_bswap ?
                                    sample_move_dither_tri_d16_sSs :
                                    sample_move_dither_tri_d16_sS;
                            break;

                        case Shaped:
                            jack_info( "Noise-shaped dithering at 16 bits" );
                            driver->write_via_copy =
                                driver->quirk_bswap ?
                                    sample_move_dither_shaped_d16_sSs :
                                    sample_move_dither_shaped_d16_sS;
                            break;

                        default:
                            driver->write_via_copy =
                                driver->quirk_bswap ?
                                                      sample_move_d16_sSs :
                                                      sample_move_d16_sS;
                            break;
                        }
                    break;

                case 3: /* NO DITHER */
                    driver->write_via_copy =
                        driver->quirk_bswap ?
                                              sample_move_d24_sSs :
                                              sample_move_d24_sS;

                    break;

                case 4: /* NO DITHER */
                    driver->write_via_copy =
                        driver->quirk_bswap ?
                                              sample_move_d32u24_sSs :
                                              sample_move_d32u24_sS;
                    break;

                default:
                    jack_error( "impossible sample width (%d) discovered!",
                                bytes );
                    exit( 1 );
                }
            }
        }

    if( driver->capture_handle )
        {
        if( SND_PCM_FMT_FLOAT_LE == driver->capture_setup.format.format )
            {
            driver->read_via_copy = sample_move_floatLE_sSs;
            }
        else
            {
            ssize_t bytes =
                snd_pcm_format_size(
                                     driver->capture_setup.format.format,
                                     1 );
            switch( bytes )
                {
                case 2:
                    driver->read_via_copy =
                        driver->quirk_bswap ?
                                              sample_move_dS_s16s :
                                              sample_move_dS_s16;
                    break;
                case 3:
                    driver->read_via_copy =
                        driver->quirk_bswap ?
                                              sample_move_dS_s24s :
                                              sample_move_dS_s24;
                    break;
                case 4:
                    driver->read_via_copy =
                        driver->quirk_bswap ?
                                              sample_move_dS_s32u24s :
                                              sample_move_dS_s32u24;
                    break;
                }
            }
        }
}

static int ioaudio_driver_configure_stream(
    ioaudio_driver_t *driver,
    char *device_name,
    const char *stream_name,
    snd_pcm_t *handle,
    snd_pcm_channel_params_t *params,
    unsigned int *nperiodsp )
{
    int err, format;
    snd_pcm_channel_info_t info;
    int32_t formats[] =
        {
        SND_PCM_SFMT_FLOAT_LE,
          SND_PCM_SFMT_S32_LE,
          SND_PCM_SFMT_S32_BE,
          SND_PCM_SFMT_S24_LE,
          SND_PCM_SFMT_S24_BE,
          SND_PCM_SFMT_S16_LE,
          SND_PCM_SFMT_S16_BE
        };
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
            jack_info( "io-audio: final selected sample format for %s: %s",
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
    driver->quirk_bswap = snd_pcm_format_big_endian(formats[format]);
#elif defined(SND_BIG_ENDIAN)
    driver->quirk_bswap = snd_pcm_format_little_endian( formats[format] );
#else
    driver->quick_bswap = 0;
#endif

    params->format.rate = driver->frame_rate;

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
                             driver->frames_per_cycle );

    *nperiodsp = driver->user_nperiods;
    params->buf.block.frags_min = driver->user_nperiods;
    params->buf.block.frags_max = driver->user_nperiods;

    jack_info( "io-audio: use %d periods for %s",
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
        jack_error(
                    "io-audio: cannot set hardware parameters for %s, why_failed=%d",
                    stream_name,
                    params->why_failed );
        return -1;
        }

    return 0;
}

static int ioaudio_driver_set_parameters(
    ioaudio_driver_t *driver,
    jack_nframes_t frames_per_cycle,
    jack_nframes_t user_nperiods,
    jack_nframes_t rate )
{
    size_t p_period_size = 0;
    size_t c_period_size = 0;
    channel_t chn;
    unsigned int pr = 0;
    unsigned int cr = 0;
    int err;

    driver->frame_rate = rate;
    driver->frames_per_cycle = frames_per_cycle;
    driver->user_nperiods = user_nperiods;

    jack_info( "configuring for %" PRIu32 "Hz, period = %"
               PRIu32 " frames (%.1f ms), buffer = %" PRIu32 " periods",
               rate,
               frames_per_cycle,
               ( ( (float)frames_per_cycle / (float)rate ) * 1000.0f ),
               user_nperiods );

    if( driver->capture_handle )
        {
        memset( &driver->capture_params,
                0,
                sizeof(snd_pcm_channel_params_t) );
        driver->capture_params.channel = SND_PCM_CHANNEL_CAPTURE;
        if( ioaudio_driver_configure_stream( driver,
                                             driver->ioaudio_name_capture,
                                             "capture",
                                             driver->capture_handle,
                                             &driver->capture_params,
                                             &driver->capture_nperiods ) )
            {
            jack_error( "io-audio: cannot configure capture channel" );
            return -1;
            }
        driver->capture_setup.channel = SND_PCM_CHANNEL_CAPTURE;
        snd_pcm_channel_setup( driver->capture_handle,
                               &driver->capture_setup );
        cr = driver->capture_setup.format.rate;

        /* check the fragment size, since thats non-negotiable */
        c_period_size = driver->capture_setup.buf.block.frag_size;

        if( c_period_size != driver->frames_per_cycle )
            {
            jack_error( "ioaudio_pcm: requested an interrupt every %"
                        PRIu32
                        " frames but got %uc frames for capture",
                        driver->frames_per_cycle,
                        p_period_size );
            return -1;
            }

        /* check the sample format */
        switch( driver->capture_setup.format.format )
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

        if( driver->capture_setup.format.interleave )
            {
            if( ( err = snd_pcm_mmap( driver->capture_handle,
                                      SND_PCM_CHANNEL_CAPTURE,
                                      &driver->capture_mmap,
                                      &driver->capture_buffer ) ) < 0 )
                {
                jack_error( "io-audio: %s: mmap areas info error",
                            driver->ioaudio_name_capture );
                return -1;
                }
            }
        }

    if( driver->playback_handle )
        {
        memset( &driver->playback_params,
                0,
                sizeof(snd_pcm_channel_params_t) );
        driver->playback_params.channel = SND_PCM_CHANNEL_PLAYBACK;

        if( ioaudio_driver_configure_stream( driver,
                                             driver->ioaudio_name_playback,
                                             "playback",
                                             driver->playback_handle,
                                             &driver->playback_params,
                                             &driver->playback_nperiods ) )
            {
            jack_error( "io-audio: cannot configure playback channel" );
            return -1;
            }

        driver->playback_setup.channel = SND_PCM_CHANNEL_PLAYBACK;
        snd_pcm_channel_setup( driver->playback_handle,
                               &driver->playback_setup );
        pr = driver->playback_setup.format.rate;

        /* check the fragment size, since thats non-negotiable */
        p_period_size = driver->playback_setup.buf.block.frag_size;

        if( p_period_size
            != snd_pcm_format_size( driver->playback_setup.format.format,
                                    driver->frames_per_cycle ) )
            {
            jack_error( "ioaudio_pcm: requested an interrupt every %"
                        PRIu32
                        " frames but got %u frames for playback",
                        driver->frames_per_cycle,
                        p_period_size );
            return -1;
            }

        /* check the sample format */
        switch( driver->playback_setup.format.format )
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

        if( driver->playback_setup.format.interleave )
            {
            if( ( err = snd_pcm_mmap( driver->playback_handle,
                                      SND_PCM_CHANNEL_PLAYBACK,
                                      &driver->playback_mmap,
                                      &driver->playback_buffer ) ) < 0 )
                {
                jack_error( "io-audio: %s: mmap areas info error",
                            driver->ioaudio_name_playback );
                return -1;
                }
            driver->interleave_unit =
                snd_pcm_format_size(
                                     driver->playback_setup.format.format,
                                     1 );
            }
        else
            {
            driver->interleave_unit = 0; /* NOT USED */
            }
        }

    /* check the rate, since thats rather important */
    if( driver->capture_handle && driver->playback_handle )
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
        if( cr != driver->frame_rate && pr != driver->frame_rate )
            {
            jack_error( "sample rate in use (%d Hz) does not "
                        "match requested rate (%d Hz)",
                        cr,
                        driver->frame_rate );
            driver->frame_rate = cr;
            }

        }
    else if( driver->capture_handle && cr != driver->frame_rate )
        {
        jack_error( "capture sample rate in use (%d Hz) does not "
                    "match requested rate (%d Hz)",
                    cr,
                    driver->frame_rate );
        driver->frame_rate = cr;
        }
    else if( driver->playback_handle && pr != driver->frame_rate )
        {
        jack_error( "playback sample rate in use (%d Hz) does not "
                    "match requested rate (%d Hz)",
                    pr,
                    driver->frame_rate );
        driver->frame_rate = pr;
        }

    if( driver->playback_setup.format.voices
        > driver->capture_setup.format.voices )
        {
        driver->max_nchannels = driver->playback_setup.format.voices;
        driver->user_nchannels = driver->capture_setup.format.voices;
        }
    else
        {
        driver->max_nchannels = driver->capture_setup.format.voices;
        driver->user_nchannels = driver->playback_setup.format.voices;
        }

    ioaudio_driver_setup_io_function_pointers( driver );

    /* Allocate and initialize structures that rely on the
     channels counts.

     Set up the bit pattern that is used to record which
     channels require action on every cycle. any bits that are
     not set after the engine's process() call indicate channels
     that potentially need to be silenced.
     */

    bitset_create( &driver->channels_done,
                   driver->max_nchannels );
    bitset_create( &driver->channels_not_done,
                   driver->max_nchannels );

    if( driver->playback_handle )
        {
        driver->playback_addr =
            (char **)malloc(
                             sizeof(char *)
                                 * driver->playback_setup.format.voices );
        memset( driver->playback_addr,
                0,
                sizeof(char *) * driver->playback_setup.format.voices );
        driver->playback_interleave_skip =
            (unsigned long *)malloc(
                                     sizeof(unsigned long *)
                                         * driver->playback_setup.format.voices );
        memset( driver->playback_interleave_skip,
                0,
                sizeof(unsigned long *)
                    * driver->playback_setup.format.voices );
        driver->silent =
            (unsigned long *)malloc(
                                     sizeof(unsigned long)
                                         * driver->playback_setup.format.voices );

        for( chn = 0; chn < driver->playback_setup.format.voices; chn++ )
            {
            driver->silent[chn] = 0;
            }

        for( chn = 0; chn < driver->playback_setup.format.voices; chn++ )
            {
            bitset_add( driver->channels_done,
                        chn );
            }

        driver->dither_state =
            (dither_state_t *)calloc(
                                      driver->playback_setup.format.voices,
                                      sizeof(dither_state_t) );
        }

    if( driver->capture_handle )
        {
        driver->capture_addr =
            (char **)malloc(
                             sizeof(char *)
                                 * driver->capture_setup.format.voices );
        memset( driver->capture_addr,
                0,
                sizeof(char *) * driver->capture_setup.format.voices );
        driver->capture_interleave_skip =
            (unsigned long *)malloc(
                                     sizeof(unsigned long *)
                                         * driver->capture_setup.format.voices );
        memset( driver->capture_interleave_skip,
                0,
                sizeof(unsigned long *) * driver->capture_setup.format.voices );
        }

    driver->clock_sync_data =
        (ClockSyncStatus *)malloc(
                                   sizeof(ClockSyncStatus)
                                       * driver->max_nchannels );

    driver->period_usecs =
        (jack_time_t)floor(
                            ( ( (float)driver->frames_per_cycle )
                                / driver->frame_rate )
                                * 1000000.0f );
    driver->poll_timeout_msecs = (int)floor( 1.5f * driver->period_usecs / 1000.0f );

    return 0;
}

int ioaudio_driver_reset_parameters(
    ioaudio_driver_t *driver,
    jack_nframes_t frames_per_cycle,
    jack_nframes_t user_nperiods,
    jack_nframes_t rate )
{
    /* XXX unregister old ports ? */
    ioaudio_driver_release_channel_dependent_memory( driver );
    return ioaudio_driver_set_parameters( driver,
                                          frames_per_cycle,
                                          user_nperiods,
                                          rate );
}

static int ioaudio_driver_get_channel_addresses(
    ioaudio_driver_t *driver,
    size_t *capture_avail,
    size_t *playback_avail,
    size_t *capture_offset,
    size_t *playback_offset )
{
    int err;
    channel_t chn;

    if( capture_avail )
        {
        if( ( err = snd_pcm_mmap( driver->capture_handle,
                                  SND_PCM_CHANNEL_CAPTURE,
                                  &driver->capture_mmap,
                                  &driver->capture_buffer ) ) < 0 )
            {
            jack_error( "io-audio: %s: mmap areas info error",
                        driver->ioaudio_name_capture );
            return -1;
            }

        for( chn = 0; chn < driver->capture_setup.format.voices; chn++ )
            {
            driver->capture_addr[chn] = driver->capture_buffer;
            driver->capture_interleave_skip[chn] =
                (unsigned long)driver->capture_mmap->status.voices;
            }
        }

    if( playback_avail )
        {
        if( ( err = snd_pcm_mmap( driver->playback_handle,
                                  SND_PCM_CHANNEL_PLAYBACK,
                                  &driver->playback_mmap,
                                  &driver->playback_buffer ) ) < 0 )
            {
            jack_error( "io-audio: %s: mmap areas info error ",
                        driver->ioaudio_name_playback );
            return -1;
            }

        for( chn = 0; chn < driver->playback_setup.format.voices; chn++ )
            {
            driver->playback_addr[chn] = driver->playback_buffer;
            driver->playback_interleave_skip[chn] =
                (unsigned long)driver->playback_mmap->status.voices;
            }
        }

    return 0;
}

int ioaudio_driver_start(
    ioaudio_driver_t *driver )
{
    int err;
    size_t poffset, pavail;
    channel_t chn;

    driver->poll_last = 0;
    driver->poll_next = 0;

    if( driver->playback_handle )
        {
        if( ( err = snd_pcm_playback_prepare( driver->playback_handle ) ) < 0 )
            {
            jack_error( "io-audio: prepare error for playback on "
                        "\"%s\" (%s)",
                        driver->ioaudio_name_playback,
                        snd_strerror( err ) );
            return -1;
            }
        }

    if( ( driver->capture_handle && driver->capture_and_playback_not_synced )
        || !driver->playback_handle )
        {
        if( ( err = snd_pcm_capture_prepare( driver->capture_handle ) ) < 0 )
            {
            jack_error( "io-audio: prepare error for capture on \"%s\""
                        " (%s)",
                        driver->ioaudio_name_capture,
                        snd_strerror( err ) );
            return -1;
            }
        }

    if( driver->hw_monitoring )
        {
        if( driver->input_monitor_mask || driver->all_monitor_in )
            {
            if( driver->all_monitor_in )
                {
                driver->hw->set_input_monitor_mask( driver->hw,
                                                    ~0U );
                }
            else
                {
                driver->hw->set_input_monitor_mask( driver->hw,
                                                    driver->input_monitor_mask );
                }
            }
        else
            {
            driver->hw->set_input_monitor_mask( driver->hw,
                                                driver->input_monitor_mask );
            }
        }

    if( driver->playback_handle )
        {
        /* fill playback buffer with zeroes, and mark
         all fragments as having data.
         */

#if 0
        pavail = snd_pcm_avail_update (driver->playback_handle);
#else
        pavail = driver->frames_per_cycle * driver->playback_nperiods;
#endif

        if( pavail != driver->frames_per_cycle * driver->playback_nperiods )
            {
            jack_error( "io-audio: full buffer not available at start" );
            return -1;
            }

        if( ioaudio_driver_get_channel_addresses( driver,
                                                  0,
                                                  &pavail,
                                                  0,
                                                  &poffset ) )
            {
            return -1;
            }

        /* XXX this is cheating. io-audio offers no guarantee that
         we can access the entire buffer at any one time. It
         works on most hardware tested so far, however, buts
         its a liability in the long run. I think that
         ioaudio-lib may have a better function for doing this
         here, where the goal is to silence the entire
         buffer.
         */

        for( chn = 0; chn < driver->playback_setup.format.voices; chn++ )
            {
            ioaudio_driver_silence_on_channel( driver,
                                               chn,
                                               driver->user_nperiods
                                                   * driver->frames_per_cycle );
            }

        msync( driver->playback_addr,
               snd_pcm_format_size( driver->playback_setup.format.format,
                                    driver->user_nperiods
                                        * driver->frames_per_cycle ),
               MS_SYNC );

//        if( ( err = snd_pcm_playback_go( driver->playback_handle ) ) < 0 )
//            {
//            jack_error( "io-audio: could not start playback (%s)",
//                        snd_strerror( err ) );
//            return -1;
//            }
        }

//    if( ( driver->capture_handle && driver->capture_and_playback_not_synced )
//        || !driver->playback_handle )
//        {
//        if( ( err = snd_pcm_capture_go( driver->capture_handle ) ) < 0 )
//            {
//            jack_error( "io-audio: could not start capture (%s)",
//                        snd_strerror( err ) );
//            return -1;
//            }
//        }

    return 0;
}

int ioaudio_driver_stop(
    ioaudio_driver_t *driver )
{
    int err;
//      JSList* node;
//      int chn;

    /* silence all capture port buffers, because we might
     be entering offline mode.
     */

// JACK2
    /*
     for (chn = 0, node = driver->capture_ports; node;
     node = jack_slist_next (node), chn++) {

     jack_port_t* port;
     char* buf;
     jack_nframes_t nframes = driver->engine->control->buffer_size;

     port = (jack_port_t *) node->data;
     buf = jack_port_get_buffer (port, nframes);
     memset (buf, 0, sizeof (jack_default_audio_sample_t) * nframes);
     }
     */

// JACK2
    ClearOutput();

    if( driver->playback_handle )
        {
        if( ( err = snd_pcm_playback_flush( driver->playback_handle ) ) < 0 )
            {
            jack_error( "io-audio: channel flush for playback "
                        "failed (%s)",
                        snd_strerror( err ) );
            return -1;
            }
        }

    if( !driver->playback_handle || driver->capture_and_playback_not_synced )
        {
        if( driver->capture_handle )
            {
            if( ( err = snd_pcm_capture_flush( driver->capture_handle ) ) < 0 )
                {
                jack_error( "io-audio: channel flush for "
                            "capture failed (%s)",
                            snd_strerror( err ) );
                return -1;
                }
            }
        }

    if( driver->hw_monitoring )
        {
        driver->hw->set_input_monitor_mask( driver->hw,
                                            0 );
        }

//        if (driver->midi && !driver->xrun_recovery)
//                (driver->midi->stop)(driver->midi);

    return 0;
}

static int ioaudio_driver_restart(
    ioaudio_driver_t *driver )
{
    int res;

    driver->xrun_recovery = 1;
    // JACK2
    /*
     if ((res = driver->nt_stop((struct _jack_driver_nt *) driver))==0)
     res = driver->nt_start((struct _jack_driver_nt *) driver);
     */
    res = Restart();
    driver->xrun_recovery = 0;

//        if (res && driver->midi)
//                (driver->midi->stop)(driver->midi);

    return res;
}

static int ioaudio_driver_xrun_recovery(
    ioaudio_driver_t *driver,
    float *delayed_usecs )
{
    snd_pcm_channel_status_t status;
    int res;

    if( driver->capture_handle )
        {
        status.channel = SND_PCM_CHANNEL_CAPTURE;
        if( ( res = snd_pcm_channel_status( driver->capture_handle,
                                            &status ) ) < 0 )
            {
            jack_error( "status error: %s",
                        snd_strerror( res ) );
            }
        }
    else
        {
        status.channel = SND_PCM_CHANNEL_PLAYBACK;
        if( ( res = snd_pcm_channel_status( driver->playback_handle,
                                            &status ) )
            < 0 )
            {
            jack_error( "status error: %s",
                        snd_strerror( res ) );
            }
        }

    if( status.status == SND_PCM_STATUS_READY )
        {
        jack_log( "**** ioaudio_pcm: pcm in suspended state, resuming it" );
        if( driver->capture_handle )
            {
            if( ( res = snd_pcm_capture_prepare( driver->capture_handle ) )
                < 0 )
                {
                jack_error( "error preparing after suspend: %s",
                            snd_strerror( res ) );
                }
            }
        else
            {
            if( ( res = snd_pcm_playback_prepare( driver->playback_handle ) )
                < 0 )
                {
                jack_error( "error preparing after suspend: %s",
                            snd_strerror( res ) );
                }
            }
        }

    if( status.status == SND_PCM_STATUS_OVERRUN
        && driver->process_count > XRUN_REPORT_DELAY )
        {
        struct timeval now, diff, tstamp;
        driver->xrun_count++;
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

    if( ioaudio_driver_restart( driver ) )
        {
        return -1;
        }
    return 0;
}

void ioaudio_driver_silence_untouched_channels(
    ioaudio_driver_t *driver,
    jack_nframes_t nframes )
{
    channel_t chn;
    jack_nframes_t buffer_frames = driver->frames_per_cycle
        * driver->playback_nperiods;

    for( chn = 0; chn < driver->playback_setup.format.voices; chn++ )
        {
        if( bitset_contains( driver->channels_not_done,
                             chn ) )
            {
            if( driver->silent[chn] < buffer_frames )
                {
                ioaudio_driver_silence_on_channel_no_mark( driver,
                                                           chn,
                                                           nframes );
                driver->silent[chn] += nframes;
                }
            }
        }
}

void ioaudio_driver_set_clock_sync_status(
    ioaudio_driver_t *driver,
    channel_t chn,
    ClockSyncStatus status )
{
    driver->clock_sync_data[chn] = status;
    ioaudio_driver_clock_sync_notify( driver,
                                      chn,
                                      status );
}

static int under_gdb = FALSE;

jack_nframes_t ioaudio_driver_wait(
    ioaudio_driver_t *driver,
    int extra_fd,
    int *status,
    float *delayed_usecs )
{
    ssize_t avail = 0;
    ssize_t capture_avail = 0;
    ssize_t playback_avail = 0;
    int xrun_detected = FALSE;
    int need_capture;
    int need_playback;
    jack_time_t poll_enter;
    jack_time_t poll_ret = 0;

    *status = -1;
    *delayed_usecs = 0;

    need_capture = driver->capture_handle ? 1 : 0;

    if( extra_fd >= 0 )
        {
        need_playback = 0;
        }
    else
        {
        need_playback = driver->playback_handle ? 1 : 0;
        }

    again:

    while( need_playback || need_capture )
        {

        int poll_result;
        unsigned short revents;

        if( need_playback )
            {
            int fd = snd_pcm_file_descriptor( driver->playback_handle,
                                              SND_PCM_CHANNEL_PLAYBACK );

            driver->pfd[SND_PCM_CHANNEL_PLAYBACK].fd = fd;
            driver->pfd[SND_PCM_CHANNEL_PLAYBACK].events = POLLOUT;
            }
        else
            {
            driver->pfd[SND_PCM_CHANNEL_PLAYBACK].fd = -1;
            }

        if( extra_fd >= 0 )
            {
            driver->pfd[SND_PCM_CHANNEL_PLAYBACK].fd = extra_fd;
            driver->pfd[SND_PCM_CHANNEL_PLAYBACK].events = POLLIN;
            }

        if( need_capture )
            {
            int fd = snd_pcm_file_descriptor( driver->capture_handle,
                                              SND_PCM_CHANNEL_CAPTURE );

            driver->pfd[SND_PCM_CHANNEL_CAPTURE].fd = fd;
            driver->pfd[SND_PCM_CHANNEL_PLAYBACK].events = POLLIN;
            }
        else
            {
            driver->pfd[SND_PCM_CHANNEL_CAPTURE].fd = -1;
            }

        poll_enter = jack_get_microseconds();

        if( poll_enter > driver->poll_next )
            {
            /*
             * This processing cycle was delayed past the
             * next due interrupt!  Do not account this as
             * a wakeup delay:
             */
            driver->poll_next = 0;
            driver->poll_late++;
            }

        poll_result = poll( driver->pfd,
                            SND_PCM_CHANNEL_MAX,
                            driver->poll_timeout_msecs );

        if( poll_result < 0 )
            {

            if( errno == EINTR )
                {
                jack_info( "poll interrupt" );
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
        SetTime( poll_ret );

        if( extra_fd < 0 )
            {
            if( driver->poll_next && poll_ret > driver->poll_next )
                {
                *delayed_usecs = poll_ret - driver->poll_next;
                }
            driver->poll_last = poll_ret;
            driver->poll_next = poll_ret + driver->period_usecs;
// JACK2
            /*
             driver->engine->transport_cycle_start (driver->engine,
             poll_ret);
             */
            }

#ifdef DEBUG_WAKEUP
        fprintf (stderr, "%" PRIu64 ": checked %d fds, started at %" PRIu64 " %" PRIu64 "  usecs since poll entered\n",
            poll_ret, nfds, poll_enter, poll_ret - poll_enter);
#endif

        /* check to see if it was the extra FD that caused us
         * to return from poll */

        if( extra_fd >= 0 )
            {

            if( driver->pfd[SND_PCM_CHANNEL_PLAYBACK].revents == 0 )
            //            if( !FD_ISSET( extra_fd, &writefds ) )
                {
                /* we timed out on the extra fd */

                *status = -4;
                return -1;
                }

            /* if POLLIN was the only bit set, we're OK */

            *status = 0;
            return
            ( driver->pfd[SND_PCM_CHANNEL_PLAYBACK].revents == POLLIN ) ?
                                                                          0 :
                                                                          -1;
            }

        if( need_playback )
            {
            snd_pcm_channel_status_t chstatus;
            chstatus.channel = SND_PCM_CHANNEL_PLAYBACK;
            snd_pcm_channel_status( driver->playback_handle,
                                    &chstatus );

            revents = driver->pfd[SND_PCM_CHANNEL_PLAYBACK].revents;

            if( revents & POLLERR )
                {
                xrun_detected = TRUE;
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
            snd_pcm_channel_status( driver->playback_handle,
                                    &chstatus );

            revents = driver->pfd[SND_PCM_CHANNEL_CAPTURE].revents;

            if( revents & POLLERR )
                {
                xrun_detected = TRUE;
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

    if( driver->capture_handle )
        {
        capture_avail = driver->capture_setup.buf.block.frag_size
            / snd_pcm_format_size( driver->capture_setup.format.format,
                                   1 );
        }
    else
        {
        capture_avail = INT_MAX;
        }

    if( driver->playback_handle )
        {
        playback_avail = driver->playback_setup.buf.block.frag_size
            / snd_pcm_format_size( driver->playback_setup.format.format,
                                   1 );
        }
    else
        {
        /* odd, but see min() computation below */
        playback_avail = INT_MAX;
        }

    if( xrun_detected )
        {
        *status = ioaudio_driver_xrun_recovery( driver,
                                                delayed_usecs );
        return 0;
        }

    *status = 0;
    driver->last_wait_ust = poll_ret;

    avail = capture_avail < playback_avail ? capture_avail : playback_avail;

#ifdef DEBUG_WAKEUP
    fprintf (stderr, "wakeup complete, avail = %lu, pavail = %lu "
        "cavail = %lu\n",
        avail, playback_avail, capture_avail);
#endif

    /* mark all channels not done for now. read/write will change this */

    bitset_copy( driver->channels_not_done,
                 driver->channels_done );

    /* constrain the available count to the nearest (round down) number of
     periods.
     */

    return avail - ( avail % driver->frames_per_cycle );
}

int ioaudio_driver_read(
    ioaudio_driver_t *driver,
    jack_nframes_t nframes )
{
    ssize_t contiguous;
    ssize_t nread;
    size_t offset = 0;
    jack_nframes_t orig_nframes;
//      jack_default_audio_sample_t* buf;
//      channel_t chn;
//      JSList *node;
//      jack_port_t* port;
    int err;

    if( nframes > driver->frames_per_cycle )
        {
        return -1;
        }

// JACK2
    /*
     if (driver->engine->freewheeling) {
     return 0;
     }
     */
//        if (driver->midi)
//                (driver->midi->read)(driver->midi, nframes);
    if( !driver->capture_handle )
        {
        return 0;
        }

    nread = 0;
    contiguous = 0;
    orig_nframes = nframes;

    while( nframes )
        {

        contiguous = nframes;

        if( ioaudio_driver_get_channel_addresses( driver,
                                                  (size_t *)&contiguous,
                                                  (size_t *)0,
                                                  &offset,
                                                  0 ) < 0 )
            {
            return -1;
            }
// JACK2
        /*
         for (chn = 0, node = driver->capture_ports; node;
         node = jack_slist_next (node), chn++) {

         port = (jack_port_t *) node->data;

         if (!jack_port_connected (port)) {
         // no-copy optimization
         continue;
         }
         buf = jack_port_get_buffer (port, orig_nframes);
         ioaudio_driver_read_from_channel (driver, chn,
         buf + nread, contiguous);
         }
         */
        ReadInput( orig_nframes,
                   contiguous,
                   nread );

        if( ( err = msync( driver->capture_addr + offset,
                           contiguous,
                           MS_SYNC ) )
            < 0 )
            {
            jack_error( "io-audio: could not complete read of %"
                        PRIu32 " frames: error = %d",
                        contiguous,
                        err );
            return -1;
            }

        nframes -= contiguous;
        nread += contiguous;
        }

    return 0;
}

int ioaudio_driver_write(
    ioaudio_driver_t* driver,
    jack_nframes_t nframes )
{
//      channel_t chn;
//      JSList *node;
//      JSList *mon_node;
//      jack_default_audio_sample_t* buf;
//      jack_default_audio_sample_t* monbuf;
    jack_nframes_t orig_nframes;
    ssize_t nwritten;
    ssize_t contiguous;
    size_t offset = 0;
//      jack_port_t *port;
    int err;

    driver->process_count++;

// JACK2
    /*
     if (!driver->playback_handle || driver->engine->freewheeling) {
     return 0;
     }
     */
    if( !driver->playback_handle )
        {
        return 0;
        }

    if( nframes > driver->frames_per_cycle )
        {
        return -1;
        }

//        if (driver->midi)
//                (driver->midi->write)(driver->midi, nframes);

    nwritten = 0;
    contiguous = 0;
    orig_nframes = nframes;

    /* check current input monitor request status */

    driver->input_monitor_mask = 0;

// JACK2
    /*
     for (chn = 0, node = driver->capture_ports; node;
     node = jack_slist_next (node), chn++) {
     if (((jack_port_t *) node->data)->shared->monitor_requests) {
     driver->input_monitor_mask |= (1<<chn);
     }
     }
     */
    MonitorInput();

    if( driver->hw_monitoring )
        {
        if( ( driver->hw->input_monitor_mask != driver->input_monitor_mask )
            && !driver->all_monitor_in )
            {
            driver->hw->set_input_monitor_mask( driver->hw,
                                                driver->input_monitor_mask );
            }
        }

    while( nframes )
        {

        contiguous = nframes;

        if( ioaudio_driver_get_channel_addresses( driver,
                                                  (size_t *)0,
                                                  (size_t *)&contiguous,
                                                  0,
                                                  &offset ) < 0 )
            {
            return -1;
            }

// JACK2
        /*
         for (chn = 0, node = driver->playback_ports, mon_node=driver->monitor_ports;
         node;
         node = jack_slist_next (node), chn++) {

         port = (jack_port_t *) node->data;

         if (!jack_port_connected (port)) {
         continue;
         }
         buf = jack_port_get_buffer (port, orig_nframes);
         ioaudio_driver_write_to_channel (driver, chn,
         buf + nwritten, contiguous);

         if (mon_node) {
         port = (jack_port_t *) mon_node->data;
         if (!jack_port_connected (port)) {
         continue;
         }
         monbuf = jack_port_get_buffer (port, orig_nframes);
         memcpy (monbuf + nwritten, buf + nwritten, contiguous * sizeof(jack_default_audio_sample_t));
         mon_node = jack_slist_next (mon_node);
         }
         }
         */

        // JACK2
        WriteOutput( orig_nframes,
                     contiguous,
                     nwritten );

        if( !bitset_empty( driver->channels_not_done ) )
            {
            ioaudio_driver_silence_untouched_channels( driver,
                                                       contiguous );
            }

        if( ( err = msync( driver->playback_buffer + offset,
                           contiguous,
                           MS_SYNC ) )
            < 0 )
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

#if 0
static int /* UNUSED */
ioaudio_driver_change_sample_clock (ioaudio_driver_t *driver, SampleClockMode mode)
    {
    return driver->hw->change_sample_clock (driver->hw, mode);
    }

static void /* UNUSED */
ioaudio_driver_request_all_monitor_input (ioaudio_driver_t *driver, int yn)

    {
    if (driver->hw_monitoring)
        {
        if (yn)
            {
            driver->hw->set_input_monitor_mask (driver->hw, ~0U);
            }
        else
            {
            driver->hw->set_input_monitor_mask (
                driver->hw, driver->input_monitor_mask);
            }
        }

    driver->all_monitor_in = yn;
    }

static void /* UNUSED */
ioaudio_driver_set_hw_monitoring (ioaudio_driver_t *driver, int yn)
    {
    if (yn)
        {
        driver->hw_monitoring = TRUE;

        if (driver->all_monitor_in)
            {
            driver->hw->set_input_monitor_mask (driver->hw, ~0U);
            }
        else
            {
            driver->hw->set_input_monitor_mask (
                driver->hw, driver->input_monitor_mask);
            }
        }
    else
        {
        driver->hw_monitoring = FALSE;
        driver->hw->set_input_monitor_mask (driver->hw, 0);
        }
    }

static ClockSyncStatus /* UNUSED */
ioaudio_driver_clock_sync_status (channel_t chn)
    {
    return Lock;
    }
#endif

void ioaudio_driver_delete(
    ioaudio_driver_t *driver )
{
    JSList *node;

//        if (driver->midi)
//                (driver->midi->destroy)(driver->midi);

    for( node = driver->clock_sync_listeners; node;
        node = jack_slist_next( node ) )
        {
        free( node->data );
        }
    jack_slist_free( driver->clock_sync_listeners );

    if( driver->ctl_handle )
        {
        snd_ctl_close( driver->ctl_handle );
        driver->ctl_handle = 0;
        }

    if( driver->capture_handle )
        {
        snd_pcm_close( driver->capture_handle );
        driver->capture_handle = 0;
        }

    if( driver->playback_handle )
        {
        snd_pcm_close( driver->playback_handle );
        driver->capture_handle = 0;
        }

//    if( driver->pfd )
//        {
//        free( driver->pfd );
//        }

    if( driver->hw )
        {
        driver->hw->release( driver->hw );
        driver->hw = 0;
        }
    free( driver->ioaudio_name_playback );
    free( driver->ioaudio_name_capture );
    free( driver->ioaudio_driver );

    ioaudio_driver_release_channel_dependent_memory( driver );
    //JACK2
    //jack_driver_nt_finish ((jack_driver_nt_t *) driver);
    free( driver );
}

//static char*
//discover_ioaudio_using_apps()
//{
//    char found[2048];
//    char command[5192];
//    char* path = getenv( "PATH" );
//    char* dir;
//    size_t flen = 0;
//    int card;
//    int device;
//    size_t cmdlen = 0;
//
//    if( !path )
//        {
//        return NULL;
//        }
//
//    /* look for lsof and give up if its not in PATH */
//
//    path = strdup( path );
//    dir = strtok( path,
//                  ":" );
//    while( dir )
//        {
//        char maybe[PATH_MAX + 1];
//        snprintf( maybe,
//                  sizeof( maybe ),
//                  "%s/lsof",
//                  dir );
//        if( access( maybe,
//                    X_OK ) )
//            {
//            break;
//            }
//        dir = strtok( NULL,
//                      ":" );
//        }
//    free( path );
//
//    if( !dir )
//        {
//        return NULL;
//        }
//
//    snprintf( command,
//              sizeof( command ),
//              "lsof -Fc0 " );
//    cmdlen = strlen( command );
//
//    for( card = 0; card < 8; ++card )
//        {
//        for( device = 0; device < 8; ++device )
//            {
//            char buf[32];
//
//            snprintf( buf,
//                      sizeof( buf ),
//                      "/dev/snd/pcmC%dD%dp",
//                      card,
//                      device );
//            if( access( buf,
//                        F_OK ) == 0 )
//                {
//                snprintf( command + cmdlen,
//                          sizeof( command ) - cmdlen,
//                          "%s ",
//                          buf );
//                }
//            cmdlen = strlen( command );
//
//            snprintf( buf,
//                      sizeof( buf ),
//                      "/dev/snd/pcmC%dD%dc",
//                      card,
//                      device );
//            if( access( buf,
//                        F_OK ) == 0 )
//                {
//                snprintf( command + cmdlen,
//                          sizeof( command ) - cmdlen,
//                          "%s ",
//                          buf );
//                }
//            cmdlen = strlen( command );
//            }
//        }
//
//    FILE* f = popen( command,
//                     "r" );
//
//    if( !f )
//        {
//        return NULL;
//        }
//
//    while( !feof( f ) )
//        {
//        char buf[1024]; /* lsof doesn't output much */
//
//        if( !fgets( buf,
//                    sizeof( buf ),
//                    f ) )
//            {
//            break;
//            }
//
//        if( *buf != 'p' )
//            {
//            return NULL;
//            }
//
//        /* buf contains NULL as a separator between the process field and the command field */
//        char *pid = buf;
//        ++pid; /* skip leading 'p' */
//        char *cmd = pid;
//
//        /* skip to NULL */
//        while( *cmd )
//            {
//            ++cmd;
//            }
//        ++cmd; /* skip to 'c' */
//        ++cmd; /* skip to first character of command */
//
//        snprintf( found + flen,
//                  sizeof( found ) - flen,
//                  "%s (process ID %s)\n",
//                  cmd,
//                  pid );
//        flen = strlen( found );
//
//        if( flen >= sizeof( found ) )
//            {
//            break;
//            }
//        }
//
//    pclose( f );
//
//    if( flen )
//        {
//        return strdup( found );
//        }
//    else
//        {
//        return NULL;
//        }
//}

jack_driver_t *
ioaudio_driver_new(
    char *name,
    jack_client_t *client,
    ioaudio_driver_args_t args )
//ioaudio_driver_new(
//    char *name,
//    char *playback_pcm_name,
//    char *capture_pcm_name,
//    jack_client_t *client,
//    jack_nframes_t frames_per_interrupt,
//    jack_nframes_t user_nperiods,
//    jack_nframes_t srate,
//    int hw_monitoring,
//    int hw_metering,
//    int capture,
//    int playback,
//    DitherAlgorithm dither,
//    int soft_mode,
//    int monitor,
//    int user_capture_nchnls,
//    int user_playback_nchnls,
//    int shorts_first,
//    jack_nframes_t systemic_input_latency,
//    jack_nframes_t systemic_output_latency /*,
//     ioaudio_midi_t *midi_driver     */
//    )
{
//        int err;
    char* current_apps;
    ioaudio_driver_t *driver;

    jack_info( "creating ioaudio driver ... %s|%s|%" PRIu32 "|%" PRIu32
               "|%" PRIu32"|%" PRIu32"|%" PRIu32 "|%s|%s|%s|%s",
               args.playback ? args.playback_pcm_name : "-",
               args.capture ? args.capture_pcm_name : "-",
               args.frames_per_interrupt,
               args.user_nperiods,
               args.srate,
               args.user_capture_nchnls,
               args.user_playback_nchnls,
               args.hw_monitoring ? "hwmon" : "nomon",
               args.hw_metering ? "hwmeter" : "swmeter",
               args.soft_mode ? "soft-mode" : "-",
               args.shorts_first ? "16bit" : "32bit" );

    driver = (ioaudio_driver_t *)calloc( 1,
                                         sizeof(ioaudio_driver_t) );

    jack_driver_nt_init( (jack_driver_nt_t *)driver );

    // JACK2
    /*
     driver->nt_attach = (JackDriverNTAttachFunction) ioaudio_driver_attach;
     driver->nt_detach = (JackDriverNTDetachFunction) ioaudio_driver_detach;
     driver->read = (JackDriverReadFunction) ioaudio_driver_read;
     driver->write = (JackDriverReadFunction) ioaudio_driver_write;
     driver->null_cycle = (JackDriverNullCycleFunction) ioaudio_driver_null_cycle;
     driver->nt_bufsize = (JackDriverNTBufSizeFunction) ioaudio_driver_bufsize;
     driver->nt_start = (JackDriverNTStartFunction) ioaudio_driver_start;
     driver->nt_stop = (JackDriverNTStopFunction) ioaudio_driver_stop;
     driver->nt_run_cycle = (JackDriverNTRunCycleFunction) ioaudio_driver_run_cycle;
     */

    driver->playback_handle = NULL;
    driver->capture_handle = NULL;
    driver->ctl_handle = 0;
    driver->hw = 0;
    driver->capture_and_playback_not_synced = FALSE;
    driver->max_nchannels = 0;
    driver->user_nchannels = 0;
    driver->user_playback_nchnls = args.user_playback_nchnls;
    driver->user_capture_nchnls = args.user_capture_nchnls;
    driver->capture_frame_latency = args.systemic_input_latency;
    driver->playback_frame_latency = args.systemic_output_latency;

    driver->playback_addr = 0;
    driver->capture_addr = 0;
    driver->playback_interleave_skip = NULL;
    driver->capture_interleave_skip = NULL;

    driver->silent = 0;
    driver->all_monitor_in = FALSE;
    driver->with_monitor_ports = args.monitor;

    driver->clock_mode = ClockMaster; /* XXX is it? */
    driver->input_monitor_mask = 0; /* XXX is it? */

    driver->capture_ports = 0;
    driver->playback_ports = 0;
    driver->monitor_ports = 0;

    memset( driver->pfd,
            0,
            sizeof( driver->pfd ) );

    driver->dither = args.dither;
    driver->soft_mode = args.soft_mode;

    driver->quirk_bswap = 0;

    pthread_mutex_init( &driver->clock_sync_lock,
                        0 );
    driver->clock_sync_listeners = 0;

    driver->poll_late = 0;
    driver->xrun_count = 0;
    driver->process_count = 0;

    driver->ioaudio_name_playback = strdup( args.playback_pcm_name );
    driver->ioaudio_name_capture = strdup( args.capture_pcm_name );

//        driver->midi = midi_driver;
    driver->xrun_recovery = 0;

    if( ioaudio_driver_check_card_type( driver ) )
        {
        ioaudio_driver_delete( driver );
        return NULL;
        }

    ioaudio_driver_hw_specific( driver,
                                args.hw_monitoring,
                                args.hw_metering );

    if( args.playback )
        {
        if( snd_pcm_open_name( &driver->playback_handle,
                               args.playback_pcm_name,
                               SND_PCM_OPEN_PLAYBACK | SND_PCM_OPEN_NONBLOCK )
            < 0 )
            {
            switch(
            errno )
                {
                case EBUSY:
                    //#ifdef __ANDROID__
//                    jack_error( "\n\nATTENTION: The playback device \"%s\" is "
//                                "already in use. Please stop the"
//                                " application using it and "
//                                "run JACK again",
//                                args.playback_pcm_name );
//#else
//                    current_apps = discover_ioaudio_using_apps();
//                    if( current_apps )
//                        {
//                        jack_error(
//                                    "\n\nATTENTION: The playback device \"%s\" is "
//                                    "already in use. The following applications "
//                                    " are using your soundcard(s) so you should "
//                                    " check them and stop them as necessary before "
//                                    " trying to start JACK again:\n\n%s",
//                                    playback_pcm_name,
//                                    current_apps );
//                        free( current_apps );
//                        }
//                    else
//                        {
//                        jack_error(
//                                    "\n\nATTENTION: The playback device \"%s\" is "
//                                    "already in use. Please stop the"
//                                    " application using it and "
//                                    "run JACK again",
//                                    playback_pcm_name );
//                        }
                    jack_error( "\n\nATTENTION: The playback device \"%s\" is "
                                "already in use. Please stop the"
                                " application using it and "
                                "run JACK again",
                                args.playback_pcm_name );
                    ioaudio_driver_delete( driver );
                    return NULL;
//#endif

                case EPERM:
                    jack_error( "you do not have permission to open "
                                "the audio device \"%s\" for playback",
                                args.playback_pcm_name );
                    ioaudio_driver_delete( driver );
                    return NULL;
                    break;
                }

            driver->playback_handle = NULL;
            }

        if( driver->playback_handle )
            {
            snd_pcm_nonblock_mode( driver->playback_handle,
                                   0 );
            }
        }

    if( args.capture )
        {
        if( snd_pcm_open_name( &driver->capture_handle,
                               args.capture_pcm_name,
                               SND_PCM_OPEN_CAPTURE | SND_PCM_OPEN_NONBLOCK )
            < 0 )
            {
            switch(
            errno )
                {
                case EBUSY:
                    //#ifdef __ANDROID__
//                    jack_error ("\n\nATTENTION: The capture (recording) device \"%s\" is "
//                        "already in use",
//                        capture_pcm_name);
//#else
//                    current_apps = discover_ioaudio_using_apps();
//                    if( current_apps )
//                        {
//                        jack_error(
//                                    "\n\nATTENTION: The capture device \"%s\" is "
//                                    "already in use. The following applications "
//                                    " are using your soundcard(s) so you should "
//                                    " check them and stop them as necessary before "
//                                    " trying to start JACK again:\n\n%s",
//                                    args.capture_pcm_name,
//                                    current_apps );
//                        free( current_apps );
//                        }
//                    else
//                        {
//                        jack_error(
//                                    "\n\nATTENTION: The capture (recording) device \"%s\" is "
//                                    "already in use. Please stop the"
//                                    " application using it and "
//                                    "run JACK again",
//                                    args.capture_pcm_name );
//                        }
//#endif
                    jack_error(
                                "\n\nATTENTION: The capture (recording) device \"%s\" is "
                                "already in use. Please stop the"
                                " application using it and "
                                "run JACK again",
                                args.capture_pcm_name );
                    ioaudio_driver_delete( driver );
                    return NULL;
                    break;

                case EPERM:
                    jack_error( "you do not have permission to open "
                                "the audio device \"%s\" for capture",
                                args.capture_pcm_name );
                    ioaudio_driver_delete( driver );
                    return NULL;
                    break;
                }

            driver->capture_handle = NULL;
            }

        if( driver->capture_handle )
            {
            snd_pcm_nonblock_mode( driver->capture_handle,
                                   0 );
            }
        }

    if( driver->playback_handle == NULL )
        {
        if( args.playback )
            {

            /* they asked for playback, but we can't do it */

            jack_error( "io-audio: Cannot open PCM device %s for "
                        "playback. Falling back to capture-only"
                        " mode",
                        name );

            if( driver->capture_handle == NULL )
                {
                /* can't do anything */
                ioaudio_driver_delete( driver );
                return NULL;
                }

            args.playback = FALSE;
            }
        }

    if( driver->capture_handle == NULL )
        {
        if( args.capture )
            {

            /* they asked for capture, but we can't do it */

            jack_error( "io-audio: Cannot open PCM device %s for "
                        "capture. Falling back to playback-only"
                        " mode",
                        name );

            if( driver->playback_handle == NULL )
                {
                /* can't do anything */
                ioaudio_driver_delete( driver );
                return NULL;
                }

            args.capture = FALSE;
            }
        }

    if( ioaudio_driver_set_parameters( driver,
                                       args.frames_per_interrupt,
                                       args.user_nperiods,
                                       args.srate ) )
        {
        ioaudio_driver_delete( driver );
        return NULL;
        }

    driver->capture_and_playback_not_synced = FALSE;

    if( driver->capture_handle && driver->playback_handle )
        {
        if( snd_pcm_link( driver->playback_handle,
                          driver->capture_handle ) != 0 )
            {
            driver->capture_and_playback_not_synced = TRUE;
            }
        }

    driver->client = client;

    return (jack_driver_t *)driver;
}

int ioaudio_driver_listen_for_clock_sync_status(
    ioaudio_driver_t *driver,
    ClockSyncListenerFunction func,
    void *arg )
{
    ClockSyncListener *csl;

    csl = (ClockSyncListener *)malloc( sizeof(ClockSyncListener) );
    csl->function = func;
    csl->arg = arg;
    csl->id = driver->next_clock_sync_listener_id++;

    pthread_mutex_lock( &driver->clock_sync_lock );
    driver->clock_sync_listeners =
        jack_slist_prepend(
                            driver->clock_sync_listeners,
                            csl );
    pthread_mutex_unlock( &driver->clock_sync_lock );
    return csl->id;
}

int ioaudio_driver_stop_listening_to_clock_sync_status(
    ioaudio_driver_t *driver,
    unsigned int which )

{
    JSList *node;
    int ret = -1;
    pthread_mutex_lock( &driver->clock_sync_lock );
    for( node = driver->clock_sync_listeners; node;
        node = jack_slist_next( node ) )
        {
        if( ( (ClockSyncListener *)node->data )->id == which )
            {
            driver->clock_sync_listeners =
                jack_slist_remove_link(
                                        driver->clock_sync_listeners,
                                        node );
            free( node->data );
            jack_slist_free_1( node );
            ret = 0;
            break;
            }
        }
    pthread_mutex_unlock( &driver->clock_sync_lock );
    return ret;
}

void ioaudio_driver_clock_sync_notify(
    ioaudio_driver_t *driver,
    channel_t chn,
    ClockSyncStatus status )
{
    JSList *node;

    pthread_mutex_lock( &driver->clock_sync_lock );
    for( node = driver->clock_sync_listeners; node;
        node = jack_slist_next( node ) )
        {
        ClockSyncListener *csl = (ClockSyncListener *)node->data;
        csl->function( chn,
                       status,
                       csl->arg );
        }
    pthread_mutex_unlock( &driver->clock_sync_lock );

}

/* DRIVER "PLUGIN" INTERFACE */

const char driver_client_name[] = "ioaudio_pcm";

void driver_finish(
    jack_driver_t *driver )
{
    ioaudio_driver_delete( (ioaudio_driver_t *)driver );
}
