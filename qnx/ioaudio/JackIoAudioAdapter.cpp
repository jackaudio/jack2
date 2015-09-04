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

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "JackIoAudioAdapter.h"
#include "JackGlobals.h"
#include "JackEngineControl.h"

#define check_error(err) if (err) { jack_error("%s:%d, io-audio error %d : %s", __FILE__, __LINE__, err, snd_strerror(err)); return err; }
#define check_error_msg(err,msg) if (err) { jack_error("%s:%d, %s : %s(%d)", __FILE__, __LINE__, msg, snd_strerror(err), err); return err; }
#define display_error_msg(err,msg) if (err) { jack_error("%s:%d, %s : %s(%d)", __FILE__, __LINE__, msg, snd_strerror(err), err); }

namespace Jack
{

    template<typename T>
    int snd_pcm_malloc_struct(
        T** ptr )
    {
        T* tmp = (T*)malloc( sizeof(T) );
        if( NULL == tmp )
            {
            *ptr = NULL;
            return -1;
            }
        *ptr = tmp;
        memset( tmp,
                0,
                sizeof(T) );
        return 0;
    }

    template<typename T>
    int snd_pcm_free_struct(
        T** ptr )
    {
        if( NULL != *ptr )
            {
            free( *ptr );
            *ptr = NULL;
            }

        return 0;
    }

    inline void* aligned_calloc(
        size_t nmemb,
        size_t size )
    {
        return (void*)calloc( nmemb,
                              size );
    }

    AudioInterface::AudioInterface(
        const AudioParam& ap ) :
            fParams( ap )
    {
        AudioInterface_common();
    }

    AudioInterface::AudioInterface(
        jack_nframes_t buffer_size,
        jack_nframes_t sample_rate ) :
            fParams( buffer_size,
                     sample_rate )
    {
        AudioInterface_common();
    }

    int AudioInterface::AudioInterface_common()
    {
        fInputDevice = NULL;
        check_error( snd_pcm_malloc_struct( &fInputParams ) );
        check_error( snd_pcm_malloc_struct( &fInputSetup ) );
        fInputCardBuffer = NULL;
        fJackInputBuffers =
            (jack_default_audio_sample_t**)calloc( NUM_BUFFERS,
                                                   sizeof(jack_default_audio_sample_t*) );

        fOutputDevice = NULL;
        check_error( snd_pcm_malloc_struct( &fOutputParams ) );
        check_error( snd_pcm_malloc_struct( &fOutputSetup ) );
        fOutputCardBuffer = NULL;
        fJackOutputBuffers =
            (jack_default_audio_sample_t**)calloc( NUM_BUFFERS,
                                                   sizeof(jack_default_audio_sample_t*) );

        return EOK;
    }

    AudioInterface::~AudioInterface()
    {
        snd_pcm_free_struct( &fInputParams );
        snd_pcm_free_struct( &fInputSetup );
        free( fJackInputBuffers );

        snd_pcm_free_struct( &fOutputParams );
        snd_pcm_free_struct( &fOutputSetup );
        free( fJackOutputBuffers );
    }

    /**
     * Open the audio interface
     */
    int AudioInterface::open()
    {
        ///////////////////////////////////////////////////////////////////////
        // Declare Local Variables
        ///////////////////////////////////////////////////////////////////////
        int err;
        int card;
        int device;

        ///////////////////////////////////////////////////////////////////////
        // Input Device Setup
        ///////////////////////////////////////////////////////////////////////

        if( strncmp( fParams.fInputCardName,
                     "none",
                     4 ) != 0 )
            {

            check_error(
                         snd_pcm_open_name( &fInputDevice,
                                            fParams.fInputCardName,
                                            SND_PCM_OPEN_CAPTURE ) );

            // Disable plugin conversions for this device
            err = snd_pcm_plugin_set_disable( fInputDevice,
                                              PLUGIN_BUFFER_PARTIAL_BLOCKS );
            err = snd_pcm_plugin_set_disable( fInputDevice,
                                              PLUGIN_MMAP );
            err = snd_pcm_plugin_set_disable( fInputDevice,
                                              PLUGIN_ROUTING );
            err = snd_pcm_plugin_set_disable( fInputDevice,
                                              PLUGIN_CONVERSION );

            // get channel parameters
            snd_pcm_channel_info_t inputInfo;
            memset( &inputInfo,
                    0,
                    sizeof( inputInfo ) );
            inputInfo.channel = SND_PCM_CHANNEL_CAPTURE;
            check_error( snd_pcm_channel_info( fInputDevice,
                                               &inputInfo ) );

            //get hardware input parameters
            fInputParams->mode = SND_PCM_MODE_BLOCK;
            fInputParams->channel = SND_PCM_CHANNEL_CAPTURE;
            //fInputParams->sync = Not Supported by io-audio

            // Check supported formats, preferring in order: float, sint32, sint16, plugin converted
            if( inputInfo.formats & SND_PCM_FMT_FLOAT )
                {
                fInputParams->format.format = SND_PCM_SFMT_FLOAT;
                }
            else if( inputInfo.formats & SND_PCM_FMT_S32_LE )
                {
                fInputParams->format.format = SND_PCM_SFMT_S32_LE;
                }
            else if( inputInfo.formats & SND_PCM_FMT_S16_LE )
                {
                fInputParams->format.format = SND_PCM_SFMT_S16_LE;
                }
            else
                {
                // Re-enable format conversion plugin if device can't accept float, sint32 or sint16
                snd_pcm_plugin_set_enable( fInputDevice,
                                           PLUGIN_CONVERSION );
                fInputParams->format.format = SND_PCM_SFMT_FLOAT;
                }

            fInputParams->format.interleave = 1;
            fInputParams->format.rate = fParams.fFrequency;
            fInputParams->format.voices = fNumInputPorts;
            //fInputParams->digital = Not currently implemented in io-audio
            fInputParams->start_mode = SND_PCM_START_DATA;
            fInputParams->stop_mode = SND_PCM_STOP_STOP;
            //fInputParams->time = 1; // If set, the driver offers the time when the transfer began (gettimeofday() format)
            //fInputParams->ust_time = 1; // If set, the driver offers the time when the transfer began (UST format)
            fInputParams->buf.block.frag_size =
                snd_pcm_format_size( fInputParams->format.format,
                                     fParams.fBuffering * fNumInputPorts );
            ;
            fInputParams->buf.block.frags_max = fParams.fPeriod;
            fInputParams->buf.block.frags_min = 1;
            strcpy( fInputParams->sw_mixer_subchn_name,
                    "Jack Capture Channel" );

            //set params record with initial values
            if( ( err = snd_pcm_channel_params( fInputDevice,
                                                fInputParams ) ) != 0 )
                {
                jack_error( "%s:%d, io-audio error %d : %s, why_failed = %d",
                            __FILE__,
                            __LINE__,
                            err,
                            snd_strerror( err ),
                            fInputParams->why_failed );
                return err;
                }

            //get params record with actual values
            fInputSetup->channel = SND_PCM_CHANNEL_CAPTURE;
            check_error( snd_pcm_channel_setup( fInputDevice,
                                                fInputSetup ) );

            //set hardware buffers
            fInputFormat = fInputSetup->format;
            fNumInputPorts = fInputFormat.voices;
            ssize_t frameSize = snd_pcm_format_size( fInputFormat.format,
                                                     fInputFormat.voices );
            fInputBufferFrames = fInputSetup->buf.block.frag_size
                / ( frameSize );

            size_t nfrags = fInputSetup->buf.block.frags_max;
            size_t nframes = fInputBufferFrames;
            size_t framesize = frameSize;

            fInputCardBuffer = aligned_calloc( nfrags,
                                               nframes * framesize );

            //create floating point buffers needed by the JACK
            for( unsigned int i = 0; i < fNumInputPorts; i++ )
                {
                fJackInputBuffers[i] =
                    (jack_default_audio_sample_t*)aligned_calloc(
                                                                  fInputBufferFrames,
                                                                  sizeof(jack_default_audio_sample_t) );
                for( unsigned int j = 0; j < fInputBufferFrames; j++ )
                    fJackInputBuffers[i][j] = 0.0;
                }
            }
        ///////////////////////////////////////////////////////////////////////
        // Output Device Setup
        ///////////////////////////////////////////////////////////////////////

        if( strncmp( fParams.fOutputCardName,
                     "none",
                     4 ) != 0 )
            {

            check_error(
                         snd_pcm_open_name( &fOutputDevice,
                                            fParams.fOutputCardName,
                                            SND_PCM_OPEN_PLAYBACK ) );

            // Disable plugin conversions for this device
            err = snd_pcm_plugin_set_disable( fOutputDevice,
                                              PLUGIN_BUFFER_PARTIAL_BLOCKS );
            err = snd_pcm_plugin_set_disable( fOutputDevice,
                                              PLUGIN_MMAP );
            err = snd_pcm_plugin_set_disable( fOutputDevice,
                                              PLUGIN_ROUTING );
            err = snd_pcm_plugin_set_disable( fOutputDevice,
                                              PLUGIN_CONVERSION );

            snd_pcm_channel_info_t outputInfo;
            memset( &outputInfo,
                    0,
                    sizeof( outputInfo ) );
            outputInfo.channel = SND_PCM_CHANNEL_PLAYBACK;
            check_error( snd_pcm_channel_info( fOutputDevice,
                                               &outputInfo ) );

            //get hardware output parameters
            check_error( snd_pcm_malloc_struct( &fOutputParams ) );
            fOutputParams->channel = SND_PCM_CHANNEL_PLAYBACK;
            fOutputParams->mode = SND_PCM_MODE_BLOCK;
            //fOutputParams->sync = Not Supported by io-audio

            // Check supported formats, preferring in order: float, sint32, sint16, plugin converted
            if( outputInfo.formats & SND_PCM_FMT_FLOAT )
                {
                fOutputParams->format.format = SND_PCM_SFMT_FLOAT;
                }
            else if( outputInfo.formats & SND_PCM_FMT_S32_LE )
                {
                fOutputParams->format.format = SND_PCM_SFMT_S32_LE;
                }
            else if( outputInfo.formats & SND_PCM_FMT_S16_LE )
                {
                fOutputParams->format.format = SND_PCM_SFMT_S16_LE;
                }
            else
                {
                // Re-enable format conversion plugin if device can't accept float, sint32 or sint16
                snd_pcm_plugin_set_enable( fOutputDevice,
                                           PLUGIN_CONVERSION );
                fOutputParams->format.format = SND_PCM_SFMT_FLOAT;
                }

            fOutputParams->format.interleave = 1;
            fOutputParams->format.rate = fParams.fFrequency;
            fOutputParams->format.voices = fNumOutputPorts;
            //fOutputParams->digital = Not currently implemented by io-audio
            fOutputParams->start_mode = SND_PCM_START_DATA;
            fOutputParams->stop_mode = SND_PCM_STOP_STOP;
            //fOutputParams->time = 1; // If set, the driver offers the time when the transfer began (gettimeofday() format)
            //fOutputParams->ust_time = 1; // If set, the driver offers the time when the transfer began (UST Format)
            fOutputParams->buf.block.frag_size =
                snd_pcm_format_size( fOutputParams->format.format,
                                     fParams.fBuffering * fNumOutputPorts );
            fOutputParams->buf.block.frags_max = fParams.fPeriod;
            fOutputParams->buf.block.frags_min = 1;
            strcpy( fOutputParams->sw_mixer_subchn_name,
                    "Jack Playback Channel" );

            //set params record with initial values
            if( ( err = snd_pcm_channel_params( fOutputDevice,
                                                fOutputParams ) ) != 0 )
                {
                jack_error( "%s:%d, io-audio error %d : %s, why_failed = %d",
                            __FILE__,
                            __LINE__,
                            err,
                            snd_strerror( err ),
                            fOutputParams->why_failed );
                return err;
                }

            //get params record with actual values
            check_error( snd_pcm_channel_setup( fOutputDevice,
                                                fOutputSetup ) );

            //set hardware buffers
            fOutputFormat = fOutputSetup->format;

            fNumOutputPorts = fOutputFormat.voices;
            ssize_t frameSize = snd_pcm_format_size( fOutputFormat.format,
                                                     fOutputFormat.voices );
            fOutputBufferFrames = fOutputSetup->buf.block.frag_size
                / ( frameSize );

            size_t nfrags = fOutputSetup->buf.block.frags_max;
            size_t nframes = fOutputBufferFrames;
            size_t framesize = frameSize;

            fOutputCardBuffer = aligned_calloc( nfrags,
                                                nframes * framesize );

            //create floating point buffers needed by the JACK
            for( unsigned int i = 0; i < fNumOutputPorts; i++ )
                {
                fJackOutputBuffers[i] =
                    (jack_default_audio_sample_t*)aligned_calloc(
                                                                  fOutputBufferFrames,
                                                                  sizeof(jack_default_audio_sample_t) );
                for( unsigned int j = 0; j < fOutputBufferFrames; j++ )
                    fJackOutputBuffers[i][j] = 0.0;
                }
            }

        return 0;
    }

    int AudioInterface::close()
    {
        snd_pcm_close( fInputDevice );
        snd_pcm_close( fOutputDevice );

        for( unsigned int i = 0; i < fNumInputPorts; i++ )
            if( fJackInputBuffers[i] )
                free( fJackInputBuffers[i] );

        for( unsigned int i = 0; i < fNumOutputPorts; i++ )
            if( fJackOutputBuffers[i] )
                free( fJackOutputBuffers[i] );

        if( fInputCardBuffer )
            free( fInputCardBuffer );
        if( fOutputCardBuffer )
            free( fOutputCardBuffer );

        return 0;
    }

    ssize_t AudioInterface::interleavedBufferSize(
        snd_pcm_channel_params_t* params )
    {
        return params->buf.block.frag_size * params->format.voices;
    }

    ssize_t AudioInterface::noninterleavedBufferSize(
        snd_pcm_channel_params_t* params )
    {
        return params->buf.block.frag_size;
    }

    /**
     * Read audio samples from the audio card. Convert samples to floats and take
     * care of interleaved buffers
     */
    int AudioInterface::read()
    {
        ssize_t count;
        unsigned int s;
        unsigned int c;
        if( NULL != fInputDevice )
            {
            switch( fInputFormat.interleave )
                {
                case 1:
                    count = snd_pcm_read( fInputDevice,
                                          fInputCardBuffer,
                                          fInputBufferFrames );
                    if( count < 0 )
                        {
                        display_error_msg( count,
                                           "reading interleaved samples" );
                        check_error_msg(
                                         snd_pcm_channel_prepare( fInputDevice,
                                                                  SND_PCM_CHANNEL_CAPTURE ),
                                         "preparing input stream" );
                        }
                    jack_log( "JackIoAudioAdapter read %ld interleaved bytes",
                              count );
                    if( fInputFormat.format == SND_PCM_SFMT_S16 )
                        {
                        int16_t* buffer16b = (int16_t*)fInputCardBuffer;
                        for( s = 0; s < fInputBufferFrames; s++ )
                            for( c = 0; c < fNumInputPorts; c++ )
                                fJackInputBuffers[c][s] =
                                    jack_default_audio_sample_t(
                                                                 buffer16b[c
                                                                     + s
                                                                         * fNumInputPorts] )
                                        * ( jack_default_audio_sample_t( 1.0 )
                                            / jack_default_audio_sample_t(
                                            SHRT_MAX ) );
                        }
                    else   // SND_PCM_SFMT_S32
                        {
                        int32_t* buffer32b = (int32_t*)fInputCardBuffer;
                        for( s = 0; s < fInputBufferFrames; s++ )
                            for( c = 0; c < fNumInputPorts; c++ )
                                fJackInputBuffers[c][s] =
                                    jack_default_audio_sample_t(
                                                                 buffer32b[c
                                                                     + s
                                                                         * fNumInputPorts] )
                                        * ( jack_default_audio_sample_t( 1.0 )
                                            / jack_default_audio_sample_t( INT_MAX ) );
                        }
                    break;
                case 0:
                    count = snd_pcm_read( fInputDevice,
                                          fInputCardBuffer,
                                          fInputBufferFrames );
                    if( count < 0 )
                        {
                        display_error_msg( count,
                                           "reading non-interleaved samples" );
                        check_error_msg(
                                         snd_pcm_channel_prepare( fInputDevice,
                                                                  SND_PCM_CHANNEL_CAPTURE ),
                                         "preparing input stream" );
                        }
                    jack_log( "JackIoAudioAdapter read %ld non-interleaved bytes",
                              count );
                    if( fInputFormat.format == SND_PCM_SFMT_S16 )
                        {
                        int16_t* buffer16b;
                        for( c = 0; c < fNumInputPorts; c++ )
                            {
                            buffer16b = (int16_t*)fInputCardBuffer;
                            for( s = 0; s < fInputBufferFrames; s++ )
                                fJackInputBuffers[c][s] =
                                    jack_default_audio_sample_t( buffer16b[s] )
                                        * ( jack_default_audio_sample_t( 1.0 )
                                            / jack_default_audio_sample_t(
                                            SHRT_MAX ) );
                            }
                        }
                    else   // SND_PCM_SFMT_S32
                        {
                        int32_t* buffer32b;
                        for( c = 0; c < fNumInputPorts; c++ )
                            {
                            buffer32b = (int32_t*)fInputCardBuffer;
                            for( s = 0; s < fInputBufferFrames; s++ )
                                fJackInputBuffers[c][s] =
                                    jack_default_audio_sample_t(
                                                                 buffer32b[s] )
                                        * ( jack_default_audio_sample_t( 1.0 )
                                            / jack_default_audio_sample_t( INT_MAX ) );
                            }
                        }
                    break;
                default:
                    check_error_msg( -10000,
                                     "unknown access mode" )
                    ;
                    break;
                }
            }
        return 0;
    }

    /**
     * write the output soft channels to the audio card. Convert sample
     * format and interleaves buffers when needed
     */
    int AudioInterface::write()
    {
        ssize_t amount;
        ssize_t count;
        unsigned int f;
        unsigned int c;
        if( NULL != fOutputDevice )
            {
            recovery: switch( fOutputFormat.interleave )
                {
                case 1:
                    if( fOutputFormat.format == SND_PCM_SFMT_S16 )
                        {
                        int16_t* buffer16b = (int16_t*)fOutputCardBuffer;
                        for( f = 0; f < fOutputBufferFrames; f++ )
                            {
                            for( c = 0; c < fNumOutputPorts; c++ )
                                {
                                jack_default_audio_sample_t x =
                                    fJackOutputBuffers[c][f];
                                buffer16b[c + ( f * fNumOutputPorts )] =
                                    short(
                                    max(min (x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0))
                                        * jack_default_audio_sample_t(
                                        SHRT_MAX ) );
                                }
                            }
                        }
                    else   // SND_PCM_FORMAT_S32
                        {
                        int32_t* buffer32b = (int32_t*)fOutputCardBuffer;
                        for( f = 0; f < fOutputBufferFrames; f++ )
                            {
                            for( c = 0; c < fNumOutputPorts; c++ )
                                {
                                jack_default_audio_sample_t x =
                                    fJackOutputBuffers[c][f];
                                buffer32b[c + f * fNumOutputPorts] =
                                    int32_t(
                                    max(min(x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0))
                                        * jack_default_audio_sample_t(
                                        INT_MAX ) );
                                }
                            }
                        }
                    amount = snd_pcm_format_size( fOutputFormat.format,
                                                  fOutputBufferFrames
                                                      * fNumOutputPorts );
                    count = snd_pcm_write( fOutputDevice,
                                           fOutputCardBuffer,
                                           amount );
                    if( count <= 0 )
                        {
                        display_error_msg( count,
                                           "writing interleaved" );
                        int err =
                            snd_pcm_channel_prepare( fOutputDevice,
                                                     SND_PCM_CHANNEL_PLAYBACK );
                        check_error_msg( err,
                                         "preparing output stream" );
                        goto recovery;
                        }
                    jack_log( "JackIoAudioAdapter wrote %ld interleaved bytes",
                              count );
                    break;
                case 0:
                    if( fOutputFormat.format == SND_PCM_SFMT_S16 )
                        {
                        int16_t* buffer16b = (int16_t*)fOutputCardBuffer;
                        for( c = 0; c < fNumOutputPorts; c++ )
                            {
                            for( f = 0; f < fOutputBufferFrames; f++ )
                                {
                                jack_default_audio_sample_t x =
                                    fJackOutputBuffers[c][f];
                                buffer16b[f] =
                                    short(
                                    max(min (x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0))
                                        * jack_default_audio_sample_t(
                                        SHRT_MAX ) );
                                }
                            }
                        }
                    else    // SND_PCM_FORMAT_S32
                        {
                        int32_t* buffer32b = (int32_t*)fOutputCardBuffer;
                        for( c = 0; c < fNumOutputPorts; c++ )
                            {
                            for( f = 0; f < fOutputBufferFrames; f++ )
                                {
                                jack_default_audio_sample_t x =
                                    fJackOutputBuffers[c][f];
                                buffer32b[f + ( c * fNumOutputPorts )] =
                                    int32_t(
                                    max(min(x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0))
                                        * jack_default_audio_sample_t(
                                        INT_MAX ) );
                                }
                            }
                        }
                    amount = snd_pcm_format_size( fOutputFormat.format,
                                                  fOutputBufferFrames
                                                      * fNumOutputPorts );
                    count = snd_pcm_write( fOutputDevice,
                                           fOutputCardBuffer,
                                           fOutputBufferFrames );
                    if( count <= 0 )
                        {
                        display_error_msg( count,
                                           "writing non-interleaved" );
                        int err =
                            snd_pcm_channel_prepare( fOutputDevice,
                                                     SND_PCM_CHANNEL_PLAYBACK );
                        check_error_msg( err,
                                         "preparing output stream" );
                        goto recovery;
                        }
                    jack_log( "JackIoAudioAdapter wrote %ld non-interleaved bytes",
                              count );
                    break;
                default:
                    check_error_msg( -10000,
                                     "unknown access mode" )
                    ;
                    break;
                }
            }
        return 0;
    }

    /**
     *  print short information on the audio device
     */
    int AudioInterface::shortinfo()
    {
        snd_ctl_hw_info_t card_info;
        snd_ctl_t* ctl_handle;

        int card = snd_card_name( fParams.fInputCardName );
        if( card < 0 )
            return card;

        check_error( snd_ctl_open( &ctl_handle,
                                   card ) );
        check_error( snd_ctl_hw_info( ctl_handle,
                                      &card_info ) );
        jack_info( "%s|%d|%d|%d|%d|%s",
                   card_info.name,
                   fNumInputPorts,
                   fNumOutputPorts,
                   fParams.fFrequency,
                   fOutputBufferFrames,
                   snd_pcm_get_format_name( fInputFormat.format ) );
        snd_ctl_close( ctl_handle );

        return 0;
    }

    /**
     *  print more detailled information on the audio device
     */
    int AudioInterface::longinfo()
    {
        snd_ctl_hw_info_t card_info;
        snd_ctl_t* ctl_handle;

        //display info
        jack_info( "Audio Interface Description :" );
        jack_info(
                   "Sampling Frequency : %d, Sample Format : %s, buffering : %d, nperiod : %d",
                   fParams.fFrequency,
                   snd_pcm_get_format_name( fInputFormat.format ),
                   fOutputBufferFrames,
                   fParams.fPeriod );
        jack_info( "Software inputs : %2d, Software outputs : %2d",
                   fNumInputPorts,
                   fNumOutputPorts );
        jack_info( "Hardware inputs : %2d, Hardware outputs : %2d",
                   fNumInputPorts,
                   fNumOutputPorts );

        //get audio card info and display
        int card = snd_card_name( fParams.fInputCardName );
        check_error( snd_ctl_open( &ctl_handle,
                                   card ) );
        check_error( snd_ctl_hw_info( ctl_handle,
                                      &card_info ) );
        printCardInfo( &card_info );

        //display input/output streams info
        if( fNumInputPorts > 0 )
            printHWParams( fInputParams );
        if( fNumOutputPorts > 0 )
            printHWParams( fOutputParams );
        snd_ctl_close( ctl_handle );
        return 0;
    }

    void AudioInterface::printCardInfo(
        snd_ctl_hw_info_t* ci )
    {
        jack_info( "Card info (address : %p)",
                   ci );
        jack_info( "\tID         = %s",
                   ci->id );
        jack_info( "\tDriver     = %s",
                   ci->abbreviation );
        jack_info( "\tName       = %s",
                   ci->name );
        jack_info( "\tLongName   = %s",
                   ci->longname );
        jack_info( "\tMixerName  = %s",
                   ci );
        jack_info( "\tComponents = %s",
                   ci );
        jack_info( "--------------" );
    }

    void AudioInterface::printHWParams(
        snd_pcm_channel_params_t* params )
    {
        jack_info( "HW Params info (address : %p)\n",
                   params );
        jack_info( "\tChannels    = %d",
                   params->format.voices );
        jack_info( "\tFormat      = %s",
                   snd_pcm_get_format_name( params->format.format ) );
        jack_info( "\tAccess      = %s",
                   "" );
        jack_info( "\tRate        = %d",
                   params->format.rate );
        jack_info( "\tPeriods     = %d",
                   0 );
        jack_info( "\tPeriod size = %d",
                   0 );
        jack_info( "\tPeriod time = %d",
                   0 );
        jack_info( "\tBuffer size = %d",
                   0 );
        jack_info( "\tBuffer time = %d",
                   0 );
        jack_info( "--------------" );
    }

    JackIoAudioAdapter::JackIoAudioAdapter(
        jack_nframes_t buffer_size,
        jack_nframes_t sample_rate,
        const JSList* params ) :
            JackAudioAdapterInterface( buffer_size,
                                       sample_rate ),
            fThread( this ),
            fAudioInterface(
                             buffer_size,
                             sample_rate )
    {
        const JSList* node;
        const jack_driver_param_t* param;

        fCaptureChannels = 2;
        fPlaybackChannels = 2;

        fAudioInterface.fParams.fPeriod = 2;

        for( node = params; node; node = jack_slist_next( node ) )
            {
            param = (const jack_driver_param_t*)node->data;

            switch( param->character )
                {
                case 'i':
                    jack_info( "fCardInputVoices = %d",
                               param->value.ui );
                    fAudioInterface.fParams.fCardInputVoices = param->value.ui;
                    break;
                case 'o':
                    jack_info( "fCardOutputVoices = %d",
                               param->value.ui );
                    fAudioInterface.fParams.fCardOutputVoices = param->value.ui;
                    break;
                case 'C':
                    jack_info( "fInputCardName = %s",
                               param->value.str );
                    fAudioInterface.fParams.fInputCardName =
                        strdup( param->value.str );
                    break;
                case 'P':
                    jack_info( "fOutputCardName = %s",
                               param->value.str );
                    fAudioInterface.fParams.fOutputCardName =
                        strdup( param->value.str );
                    break;
                case 'D':
                    break;
                case 'n':
                    jack_info( "fPeriod = %d",
                               param->value.ui );
                    fAudioInterface.fParams.fPeriod = param->value.ui;
                    break;
                case 'r':
                    jack_info( "fFrequency = %d",
                               param->value.ui );
                    fAudioInterface.fParams.fFrequency = param->value.ui;
                    SetAdaptedSampleRate( param->value.ui );
                    break;
                case 'p':
                    jack_info( "fBuffering = %d",
                               param->value.ui );
                    fAudioInterface.fParams.fBuffering = param->value.ui;
                    SetAdaptedBufferSize( param->value.ui );
                    break;
                case 'q':
                    jack_info( "fQuality = %d",
                               param->value.ui );
                    fQuality = param->value.ui;
                    break;
                case 'g':
                    jack_info( "fRingbufferCurSize = %d",
                               param->value.ui );
                    fRingbufferCurSize = param->value.ui;
                    fAdaptative = false;
                    break;
                }
            }
    }

    int JackIoAudioAdapter::Open()
    {
        //open audio interface
        if( fAudioInterface.open() )
            return -1;

        //start adapter thread
        if( fThread.StartSync() < 0 )
            {
            jack_error( "Cannot start audioadapter thread" );
            return -1;
            }

        //display card info
        fAudioInterface.longinfo();

        //turn the thread realtime
        fThread.AcquireRealTime( GetEngineControl()->fClientPriority );
        return 0;
    }

    int JackIoAudioAdapter::Close()
    {
#ifdef JACK_MONITOR
        fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
#endif
        switch( fThread.GetStatus() )
            {

            // Kill the thread in Init phase
            case JackThread::kStarting:
            case JackThread::kIniting:
                if( fThread.Kill() < 0 )
                    {
                    jack_error( "Cannot kill thread" );
                    return -1;
                    }
                break;

            // Stop when the thread cycle is finished
            case JackThread::kRunning:
                if( fThread.Stop() < 0 )
                    {
                    jack_error( "Cannot stop thread" );
                    return -1;
                    }
                break;

            default:
                break;
            }
        return fAudioInterface.close();
    }

    void JackIoAudioAdapter::Create()
    {
        int err;
        snd_pcm_channel_info_t channel_info;
        snd_pcm_t* device;

        jack_info( "Fixed ringbuffer size = %d frames",
                   fRingbufferCurSize );

        if( strncmp( fAudioInterface.fParams.fInputCardName,
                     "none",
                     4 ) != 0 )
            {
            err = snd_pcm_open_name( &device,
                                     fAudioInterface.fParams.fInputCardName,
                                     SND_PCM_OPEN_CAPTURE );

            // get channel parameters
            memset( &channel_info,
                    0,
                    sizeof( channel_info ) );
            channel_info.channel = SND_PCM_CHANNEL_CAPTURE;
            err = snd_pcm_channel_info( device,
                                        &channel_info );
            snd_pcm_close( device );

            // Determine number of capture channels from card or parameter
            if( 0 == fAudioInterface.fParams.fCardInputVoices )
                {
                fCaptureChannels = channel_info.max_voices;
                }
            else
                {
                fCaptureChannels =
                    max( channel_info.min_voices,
                         (int )fAudioInterface.fParams.fCardInputVoices );
                fCaptureChannels = min( channel_info.max_voices,
                                        fCaptureChannels );
                }

            fAudioInterface.fNumInputPorts = fCaptureChannels;

            // Create capture channel ringbuffers
            fCaptureRingBuffer = new JackResampler*[fCaptureChannels];
            for( int i = 0; i < fCaptureChannels; i++ )
                {
                fCaptureRingBuffer[i] = new JackResampler();
                fCaptureRingBuffer[i]->Reset( fRingbufferCurSize );
                }
            jack_log( "ReadSpace = %ld",
                      fCaptureRingBuffer[0]->ReadSpace() );
            }
        else
            {
            fCaptureChannels = 0;
            fCaptureRingBuffer = NULL;
            fAudioInterface.fNumInputPorts = 0;
            }

        if( strncmp( fAudioInterface.fParams.fOutputCardName,
                     "none",
                     4 ) != 0 )
            {
            int err;

            err = snd_pcm_open_name( &device,
                                     fAudioInterface.fParams.fOutputCardName,
                                     SND_PCM_OPEN_PLAYBACK );

            // get channel parameters
            memset( &channel_info,
                    0,
                    sizeof( channel_info ) );
            channel_info.channel = SND_PCM_CHANNEL_PLAYBACK;
            err = snd_pcm_channel_info( device,
                                        &channel_info );
            snd_pcm_close( device );

            // Determine number of playback channels from card or parameter
            if( 0 == fAudioInterface.fParams.fCardOutputVoices )
                {
                fPlaybackChannels = channel_info.max_voices;
                }
            else
                {
                fPlaybackChannels =
                    max( channel_info.min_voices,
                         (int )fAudioInterface.fParams.fCardOutputVoices );
                fPlaybackChannels =
                    min( channel_info.max_voices,
                         fPlaybackChannels );
                }

            fAudioInterface.fNumOutputPorts = fPlaybackChannels;

            // Create playback channel ringbuffers
            fPlaybackRingBuffer = new JackResampler*[fPlaybackChannels];
            for( int i = 0; i < fPlaybackChannels; i++ )
                {
                fPlaybackRingBuffer[i] = new JackResampler();
                fPlaybackRingBuffer[i]->Reset( fRingbufferCurSize );
                }
            jack_log( "ReadSpace = %ld",
                      fPlaybackRingBuffer[0]->ReadSpace() );
            }
        else
            {
            fPlaybackChannels = 0;
            fPlaybackRingBuffer = NULL;
            fAudioInterface.fNumOutputPorts = 0;
            }
    }

    void JackIoAudioAdapter::Destroy()
    {
        for( int i = 0; i < fCaptureChannels; i++ )
            {
            delete ( fCaptureRingBuffer[i] );
            }
        for( int i = 0; i < fPlaybackChannels; i++ )
            {
            delete ( fPlaybackRingBuffer[i] );
            }

        delete[] fCaptureRingBuffer;
        delete[] fPlaybackRingBuffer;
    }

    bool JackIoAudioAdapter::Init()
    {
        //fill the hardware buffers
        for( unsigned int i = 0; i < fAudioInterface.fParams.fPeriod; i++ )
            fAudioInterface.write();
        return true;
    }

    bool JackIoAudioAdapter::Execute()
    {
        //read data from audio interface
        if( fAudioInterface.read() < 0 )
            return false;

        PushAndPull( fAudioInterface.fJackInputBuffers,
                     fAudioInterface.fJackOutputBuffers,
                     fAdaptedBufferSize );

        //write data to audio interface
        if( fAudioInterface.write() < 0 )
            return false;

        return true;
    }

    int JackIoAudioAdapter::SetSampleRate(
        jack_nframes_t sample_rate )
    {
        JackAudioAdapterInterface::SetHostSampleRate( sample_rate );
        Close();
        return Open();
    }

    int JackIoAudioAdapter::SetBufferSize(
        jack_nframes_t buffer_size )
    {
        JackAudioAdapterInterface::SetHostBufferSize( buffer_size );
        Close();
        return Open();
    }

} // namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc =
            jack_driver_descriptor_construct( "audioadapter",
                                              JackDriverNone,
                                              "netjack audio <==> net backend adapter",
                                              &filler );

        strcpy( value.str,
                "pcmPreferredc" );
        jack_driver_descriptor_add_parameter( desc,
                                              &filler,
                                              "capture",
                                              'C',
                                              JackDriverParamString,
                                              &value,
                                              NULL,
                                              "Provide capture ports for this io-audio device",
                                              "Opens the given io-audio device for capture and provides ports for its voices.\n"
                                              "Pass \"none\" to disable." );

        strcpy( value.str,
                "pcmPreferredp" );
        jack_driver_descriptor_add_parameter( desc,
                                              &filler,
                                              "playback",
                                              'P',
                                              JackDriverParamString,
                                              &value,
                                              NULL,
                                              "Provide playback ports for this io-audio device",
                                              "Opens the given io-audio device for playback and provides ports for its voices.\n"
                                              "Pass \"none\" to disable." );

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

        value.ui = 512U;
        jack_driver_descriptor_add_parameter( desc,
                                              &filler,
                                              "periodsize",
                                              'p',
                                              JackDriverParamUInt,
                                              &value,
                                              NULL,
                                              "Period size",
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

        value.i = true;
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
                                              "in-channels",
                                              'i',
                                              JackDriverParamInt,
                                              &value,
                                              NULL,
                                              "Number of capture voices (defaults to hardware max)",
                                              "Provides up to this many input ports corresponding to voices on the capture device." );

        jack_driver_descriptor_add_parameter( desc,
                                              &filler,
                                              "out-channels",
                                              'o',
                                              JackDriverParamInt,
                                              &value,
                                              NULL,
                                              "Number of playback voices (defaults to hardware max)",
                                              "Provides up to this many output ports corresponding to voices on the playback device." );

        value.ui = 0;
        jack_driver_descriptor_add_parameter( desc,
                                              &filler,
                                              "quality",
                                              'q',
                                              JackDriverParamUInt,
                                              &value,
                                              NULL,
                                              "Resample algorithm quality (0 - 4)",
                                              NULL );

        value.ui = 32768;
        jack_driver_descriptor_add_parameter( desc,
                                              &filler,
                                              "ring-buffer",
                                              'g',
                                              JackDriverParamUInt,
                                              &value,
                                              NULL,
                                              "Fixed ringbuffer size",
                                              "Fixed ringbuffer size (if not set => automatic adaptative)" );

        return desc;
    }

#ifdef __cplusplus
}
#endif

