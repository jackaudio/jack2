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

#ifndef __JackAlsaAdapter__
#define __JackAlsaAdapter__

#include <math.h>
#include <limits.h>
#include <assert.h>
#include <alsa/asoundlib.h>
#include "JackAudioAdapterInterface.h"
#include "JackPlatformPlug.h"
#include "JackError.h"
#include "jack.h"
#include "jslist.h"

namespace Jack
{

    inline void* aligned_calloc ( size_t nmemb, size_t size ) { return ( void* ) calloc ( nmemb, size ); }

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

#define check_error(err) if (err) { jack_error("%s:%d, alsa error %d : %s", __FILE__, __LINE__, err, snd_strerror(err)); return err; }
#define check_error_msg(err,msg) if (err) { jack_error("%s:%d, %s : %s(%d)", __FILE__, __LINE__, msg, snd_strerror(err), err); return err; }
#define display_error_msg(err,msg) if (err) { jack_error("%s:%d, %s : %s(%d)", __FILE__, __LINE__, msg, snd_strerror(err), err); }

    /**
     * A convenient class to pass parameters to AudioInterface
     */
    class AudioParam
    {
        public:
            const char*     fCardName;
            unsigned int    fFrequency;
            int             fBuffering;

            unsigned int    fSoftInputs;
            unsigned int    fSoftOutputs;

        public:
            AudioParam() :
                    fCardName ( "hw:0" ),
                    fFrequency ( 44100 ),
                    fBuffering ( 512 ),
                    fSoftInputs ( 2 ),
                    fSoftOutputs ( 2 )
            {}

            AudioParam ( jack_nframes_t buffer_size, jack_nframes_t sample_rate ) :
                    fCardName ( "hw:0" ),
                    fFrequency ( sample_rate ),
                    fBuffering ( buffer_size ),
                    fSoftInputs ( 2 ),
                    fSoftOutputs ( 2 )
            {}

            AudioParam& cardName ( const char* n )
            {
                fCardName = n;
                return *this;
            }

            AudioParam& frequency ( int f )
            {
                fFrequency = f;
                return *this;
            }

            AudioParam& buffering ( int fpb )
            {
                fBuffering = fpb;
                return *this;
            }

            void setInputs ( int inputs )
            {
                fSoftInputs = inputs;
            }

            AudioParam& inputs ( int n )
            {
                fSoftInputs = n;
                return *this;
            }

            void setOutputs ( int outputs )
            {
                fSoftOutputs = outputs;
            }

            AudioParam& outputs ( int n )
            {
                fSoftOutputs = n;
                return *this;
            }
    };

    /**
     * An ALSA audio interface
     */
    class AudioInterface : public AudioParam
    {
        public:
            //device info
            snd_pcm_t*  fOutputDevice;
            snd_pcm_t*  fInputDevice;
            snd_pcm_hw_params_t* fInputParams;
            snd_pcm_hw_params_t* fOutputParams;

            //samples info
            snd_pcm_format_t fSampleFormat;
            snd_pcm_access_t fSampleAccess;

            //channels
            const char*  fCaptureName;
            const char*  fPlaybackName;
            unsigned int fCardInputs;
            unsigned int fCardOutputs;

            //stream parameters
            unsigned int fPeriod;

            //interleaved mode audiocard buffers
            void* fInputCardBuffer;
            void* fOutputCardBuffer;

            //non-interleaved mode audiocard buffers
            void* fInputCardChannels[256];
            void* fOutputCardChannels[256];

            //non-interleaved mod, floating point software buffers
            jack_default_audio_sample_t* fInputSoftChannels[256];
            jack_default_audio_sample_t* fOutputSoftChannels[256];

            //public methods ---------------------------------------------------------

            const char* cardName()
            {
                return fCardName;
            }

            int frequency()
            {
                return fFrequency;
            }

            int buffering()
            {
                return fBuffering;
            }

            jack_default_audio_sample_t** inputSoftChannels()
            {
                return fInputSoftChannels;
            }

            jack_default_audio_sample_t** outputSoftChannels()
            {
                return fOutputSoftChannels;
            }

            AudioInterface ( const AudioParam& ap = AudioParam() ) : AudioParam ( ap )
            {
                fInputDevice    = 0;
                fOutputDevice   = 0;
                fInputParams    = 0;
                fOutputParams   = 0;
                fPeriod = 2;
                fCaptureName    = NULL;
                fPlaybackName   = NULL;

                fInputCardBuffer = 0;
                fOutputCardBuffer = 0;

                for ( int i = 0; i < 256; i++ )
                {
                    fInputCardChannels[i] = 0;
                    fOutputCardChannels[i] = 0;
                    fInputSoftChannels[i] = 0;
                    fOutputSoftChannels[i] = 0;
                }
            }

            AudioInterface ( jack_nframes_t buffer_size, jack_nframes_t sample_rate ) :
                    AudioParam ( buffer_size, sample_rate )
            {
                fInputCardBuffer = 0;
                fOutputCardBuffer = 0;
                fCaptureName    = NULL;
                fPlaybackName   = NULL;

                for ( int i = 0; i < 256; i++ )
                {
                    fInputCardChannels[i] = 0;
                    fOutputCardChannels[i] = 0;
                    fInputSoftChannels[i] = 0;
                    fOutputSoftChannels[i] = 0;
                }
            }

            /**
             * Open the audio interface
             */
            int open()
            {
                //open input/output streams
                check_error ( snd_pcm_open ( &fInputDevice,  (fCaptureName == NULL) ? fCardName : fCaptureName, SND_PCM_STREAM_CAPTURE, 0 ) );
                check_error ( snd_pcm_open ( &fOutputDevice, (fPlaybackName == NULL) ? fCardName : fPlaybackName, SND_PCM_STREAM_PLAYBACK, 0 ) );

                //get hardware input parameters
                check_error ( snd_pcm_hw_params_malloc ( &fInputParams ) );
                setAudioParams ( fInputDevice, fInputParams );

                //get hardware output parameters
                check_error ( snd_pcm_hw_params_malloc ( &fOutputParams ) )
                setAudioParams ( fOutputDevice, fOutputParams );

                // set the number of physical input and output channels close to what we need
                fCardInputs 	= fSoftInputs;
                fCardOutputs 	= fSoftOutputs;

                snd_pcm_hw_params_set_channels_near(fInputDevice, fInputParams, &fCardInputs);
                snd_pcm_hw_params_set_channels_near(fOutputDevice, fOutputParams, &fCardOutputs);

                //set input/output param
                check_error ( snd_pcm_hw_params ( fInputDevice,  fInputParams ) );
                check_error ( snd_pcm_hw_params ( fOutputDevice, fOutputParams ) );

                //set hardware buffers
                if ( fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED )
                {
                    fInputCardBuffer = aligned_calloc ( interleavedBufferSize ( fInputParams ), 1 );
                    fOutputCardBuffer = aligned_calloc ( interleavedBufferSize ( fOutputParams ), 1 );
                }
                else
                {
                    for ( unsigned int i = 0; i < fCardInputs; i++ )
                        fInputCardChannels[i] = aligned_calloc ( noninterleavedBufferSize ( fInputParams ), 1 );
                    for ( unsigned int i = 0; i < fCardOutputs; i++ )
                        fOutputCardChannels[i] = aligned_calloc ( noninterleavedBufferSize ( fOutputParams ), 1 );
                }

                //set floating point buffers needed by the dsp code
                fSoftInputs = max ( fSoftInputs, fCardInputs );
                assert ( fSoftInputs < 256 );
                fSoftOutputs = max ( fSoftOutputs, fCardOutputs );
                assert ( fSoftOutputs < 256 );

                for ( unsigned int i = 0; i < fSoftInputs; i++ )
                {
                    fInputSoftChannels[i] = ( jack_default_audio_sample_t* ) aligned_calloc ( fBuffering, sizeof ( jack_default_audio_sample_t ) );
                    for ( int j = 0; j < fBuffering; j++ )
                        fInputSoftChannels[i][j] = 0.0;
                }

                for ( unsigned int i = 0; i < fSoftOutputs; i++ )
                {
                    fOutputSoftChannels[i] = ( jack_default_audio_sample_t* ) aligned_calloc ( fBuffering, sizeof ( jack_default_audio_sample_t ) );
                    for ( int j = 0; j < fBuffering; j++ )
                        fOutputSoftChannels[i][j] = 0.0;
                }
                return 0;
            }

            int close()
            {
                snd_pcm_hw_params_free ( fInputParams );
                snd_pcm_hw_params_free ( fOutputParams );
                snd_pcm_close ( fInputDevice );
                snd_pcm_close ( fOutputDevice );

                for ( unsigned int i = 0; i < fSoftInputs; i++ )
                    if ( fInputSoftChannels[i] )
                        free ( fInputSoftChannels[i] );

                for ( unsigned int i = 0; i < fSoftOutputs; i++ )
                    if ( fOutputSoftChannels[i] )
                        free ( fOutputSoftChannels[i] );

                for ( unsigned int i = 0; i < fCardInputs; i++ )
                    if ( fInputCardChannels[i] )
                        free ( fInputCardChannels[i] );

                for ( unsigned int i = 0; i < fCardOutputs; i++ )
                    if ( fOutputCardChannels[i] )
                        free ( fOutputCardChannels[i] );

                if ( fInputCardBuffer )
                    free ( fInputCardBuffer );
                if ( fOutputCardBuffer )
                    free ( fOutputCardBuffer );

                return 0;
            }

            int setAudioParams ( snd_pcm_t* stream, snd_pcm_hw_params_t* params )
            {
                //set params record with initial values
                check_error_msg ( snd_pcm_hw_params_any ( stream, params ), "unable to init parameters" )

                //set alsa access mode (and fSampleAccess field) either to non interleaved or interleaved
                if ( snd_pcm_hw_params_set_access ( stream, params, SND_PCM_ACCESS_RW_NONINTERLEAVED ) )
                    check_error_msg ( snd_pcm_hw_params_set_access ( stream, params, SND_PCM_ACCESS_RW_INTERLEAVED ),
                                      "unable to set access mode neither to non-interleaved or to interleaved" );
                snd_pcm_hw_params_get_access ( params, &fSampleAccess );

                //search for 32-bits or 16-bits format
                if ( snd_pcm_hw_params_set_format ( stream, params, SND_PCM_FORMAT_S32 ) )
                    check_error_msg ( snd_pcm_hw_params_set_format ( stream, params, SND_PCM_FORMAT_S16 ),
                                      "unable to set format to either 32-bits or 16-bits" );
                snd_pcm_hw_params_get_format ( params, &fSampleFormat );

                //set sample frequency
                snd_pcm_hw_params_set_rate_near ( stream, params, &fFrequency, 0 );

                //set period and period size (buffering)
                check_error_msg ( snd_pcm_hw_params_set_period_size ( stream, params, fBuffering, 0 ), "period size not available" );
                check_error_msg ( snd_pcm_hw_params_set_periods ( stream, params, fPeriod, 0 ), "number of periods not available" );

                return 0;
            }

            ssize_t interleavedBufferSize ( snd_pcm_hw_params_t* params )
            {
                _snd_pcm_format format;
                unsigned int channels;
                snd_pcm_hw_params_get_format ( params, &format );
                snd_pcm_uframes_t psize;
                snd_pcm_hw_params_get_period_size ( params, &psize, NULL );
                snd_pcm_hw_params_get_channels ( params, &channels );
                ssize_t bsize = snd_pcm_format_size ( format, psize * channels );
                return bsize;
            }

            ssize_t noninterleavedBufferSize ( snd_pcm_hw_params_t* params )
            {
                _snd_pcm_format format;
                snd_pcm_hw_params_get_format ( params, &format );
                snd_pcm_uframes_t psize;
                snd_pcm_hw_params_get_period_size ( params, &psize, NULL );
                ssize_t bsize = snd_pcm_format_size ( format, psize );
                return bsize;
            }

            /**
             * Read audio samples from the audio card. Convert samples to floats and take
             * care of interleaved buffers
             */
            int read()
            {
                int count, s;
                unsigned int c;
                switch ( fSampleAccess )
                {
                    case SND_PCM_ACCESS_RW_INTERLEAVED :
                        count = snd_pcm_readi ( fInputDevice, fInputCardBuffer, fBuffering );
                        if ( count < 0 )
                        {
                            display_error_msg ( count, "reading samples" );
                            check_error_msg ( snd_pcm_prepare ( fInputDevice ), "preparing input stream" );
                        }
                        if ( fSampleFormat == SND_PCM_FORMAT_S16 )
                        {
                            short* buffer16b = ( short* ) fInputCardBuffer;
                            for ( s = 0; s < fBuffering; s++ )
                                for ( c = 0; c < fCardInputs; c++ )
                                    fInputSoftChannels[c][s] = jack_default_audio_sample_t(buffer16b[c + s*fCardInputs]) * (jack_default_audio_sample_t(1.0)/jack_default_audio_sample_t(SHRT_MAX));
                        }
                        else   // SND_PCM_FORMAT_S32
                        {
                            int32_t* buffer32b = ( int32_t* ) fInputCardBuffer;
                            for ( s = 0; s < fBuffering; s++ )
                                for ( c = 0; c < fCardInputs; c++ )
                                    fInputSoftChannels[c][s] = jack_default_audio_sample_t(buffer32b[c + s*fCardInputs]) * (jack_default_audio_sample_t(1.0)/jack_default_audio_sample_t(INT_MAX));
                        }
                        break;
                    case SND_PCM_ACCESS_RW_NONINTERLEAVED :
                        count = snd_pcm_readn ( fInputDevice, fInputCardChannels, fBuffering );
                        if ( count < 0 )
                        {
                            display_error_msg ( count, "reading samples" );
                            check_error_msg ( snd_pcm_prepare ( fInputDevice ), "preparing input stream" );
                        }
                        if ( fSampleFormat == SND_PCM_FORMAT_S16 )
                        {
                            short* chan16b;
                            for ( c = 0; c < fCardInputs; c++ )
                            {
                                chan16b = ( short* ) fInputCardChannels[c];
                                for ( s = 0; s < fBuffering; s++ )
                                    fInputSoftChannels[c][s] = jack_default_audio_sample_t(chan16b[s]) * (jack_default_audio_sample_t(1.0)/jack_default_audio_sample_t(SHRT_MAX));
                            }
                        }
                        else   // SND_PCM_FORMAT_S32
                        {
                            int32_t* chan32b;
                            for ( c = 0; c < fCardInputs; c++ )
                            {
                                chan32b = ( int32_t* ) fInputCardChannels[c];
                                for ( s = 0; s < fBuffering; s++ )
                                    fInputSoftChannels[c][s] = jack_default_audio_sample_t(chan32b[s]) * (jack_default_audio_sample_t(1.0)/jack_default_audio_sample_t(INT_MAX));
                            }
                        }
                        break;
                    default :
                        check_error_msg ( -10000, "unknow access mode" );
                        break;
                }
                return 0;
            }

            /**
             * write the output soft channels to the audio card. Convert sample
             * format and interleaves buffers when needed
             */
            int write()
            {
                int count, f;
                unsigned int c;
            recovery:
                switch ( fSampleAccess )
                {
                    case SND_PCM_ACCESS_RW_INTERLEAVED :
                        if ( fSampleFormat == SND_PCM_FORMAT_S16 )
                        {
                            short* buffer16b = ( short* ) fOutputCardBuffer;
                            for ( f = 0; f < fBuffering; f++ )
                            {
                                for ( unsigned int c = 0; c < fCardOutputs; c++ )
                                {
                                    jack_default_audio_sample_t x = fOutputSoftChannels[c][f];
                                    buffer16b[c + f * fCardOutputs] = short(max(min (x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0)) * jack_default_audio_sample_t(SHRT_MAX));
                                }
                            }
                        }
                        else   // SND_PCM_FORMAT_S32
                        {
                            int32_t* buffer32b = ( int32_t* ) fOutputCardBuffer;
                            for ( f = 0; f < fBuffering; f++ )
                            {
                                for ( unsigned int c = 0; c < fCardOutputs; c++ )
                                {
                                    jack_default_audio_sample_t x = fOutputSoftChannels[c][f];
                                    buffer32b[c + f * fCardOutputs] = int32_t(max(min(x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0)) * jack_default_audio_sample_t(INT_MAX));
                                }
                            }
                        }
                        count = snd_pcm_writei ( fOutputDevice, fOutputCardBuffer, fBuffering );
                        if ( count < 0 )
                        {
                            display_error_msg ( count, "w3" );
                            int err = snd_pcm_prepare ( fOutputDevice );
                            check_error_msg ( err, "preparing output stream" );
                            goto recovery;
                        }
                        break;
                    case SND_PCM_ACCESS_RW_NONINTERLEAVED :
                        if ( fSampleFormat == SND_PCM_FORMAT_S16 )
                        {
                            for ( c = 0; c < fCardOutputs; c++ )
                            {
                                short* chan16b = ( short* ) fOutputCardChannels[c];
                                for ( f = 0; f < fBuffering; f++ )
                                {
                                    jack_default_audio_sample_t x = fOutputSoftChannels[c][f];
                                    chan16b[f] = short(max(min (x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0)) * jack_default_audio_sample_t(SHRT_MAX));
                                }
                            }
                        }
                        else
                        {
                            for ( c = 0; c < fCardOutputs; c++ )
                            {
                                int32_t* chan32b = ( int32_t* ) fOutputCardChannels[c];
                                for ( f = 0; f < fBuffering; f++ )
                                {
                                    jack_default_audio_sample_t x = fOutputSoftChannels[c][f];
                                    chan32b[f] = int32_t(max(min(x, jack_default_audio_sample_t(1.0)), jack_default_audio_sample_t(-1.0)) * jack_default_audio_sample_t(INT_MAX));
                                }
                            }
                        }
                        count = snd_pcm_writen ( fOutputDevice, fOutputCardChannels, fBuffering );
                        if ( count<0 )
                        {
                            display_error_msg ( count, "w3" );
                            int err = snd_pcm_prepare ( fOutputDevice );
                            check_error_msg ( err, "preparing output stream" );
                            goto recovery;
                        }
                        break;
                    default :
                        check_error_msg ( -10000, "unknow access mode" );
                        break;
                }
                return 0;
            }

            /**
             *  print short information on the audio device
             */
            int shortinfo()
            {
                int err;
                snd_ctl_card_info_t* card_info;
                snd_ctl_t* ctl_handle;
                err = snd_ctl_open ( &ctl_handle, fCardName, 0 );   check_error ( err );
                snd_ctl_card_info_alloca ( &card_info );
                err = snd_ctl_card_info ( ctl_handle, card_info );  check_error ( err );
                jack_info ( "%s|%d|%d|%d|%d|%s",
                            snd_ctl_card_info_get_driver ( card_info ),
                            fCardInputs, fCardOutputs,
                            fFrequency, fBuffering,
                            snd_pcm_format_name ( ( _snd_pcm_format ) fSampleFormat ) );
                snd_ctl_close(ctl_handle);
            }

            /**
             *  print more detailled information on the audio device
             */
            int longinfo()
            {
                snd_ctl_card_info_t* card_info;
                snd_ctl_t* ctl_handle;

                //display info
                jack_info ( "Audio Interface Description :" );
                jack_info ( "Sampling Frequency : %d, Sample Format : %s, buffering : %d, nperiod : %d",
                            fFrequency, snd_pcm_format_name ( ( _snd_pcm_format ) fSampleFormat ), fBuffering, fPeriod );
                jack_info ( "Software inputs : %2d, Software outputs : %2d", fSoftInputs, fSoftOutputs );
                jack_info ( "Hardware inputs : %2d, Hardware outputs : %2d", fCardInputs, fCardOutputs );

                //get audio card info and display
                check_error ( snd_ctl_open ( &ctl_handle, fCardName, 0 ) );
                snd_ctl_card_info_alloca ( &card_info );
                check_error ( snd_ctl_card_info ( ctl_handle, card_info ) );
                printCardInfo ( card_info );

                //display input/output streams info
                if ( fSoftInputs > 0 )
                    printHWParams ( fInputParams );
                if ( fSoftOutputs > 0 )
                    printHWParams ( fOutputParams );
                snd_ctl_close(ctl_handle);
                return 0;
            }

            void printCardInfo ( snd_ctl_card_info_t* ci )
            {
                jack_info ( "Card info (address : %p)", ci );
                jack_info ( "\tID         = %s", snd_ctl_card_info_get_id ( ci ) );
                jack_info ( "\tDriver     = %s", snd_ctl_card_info_get_driver ( ci ) );
                jack_info ( "\tName       = %s", snd_ctl_card_info_get_name ( ci ) );
                jack_info ( "\tLongName   = %s", snd_ctl_card_info_get_longname ( ci ) );
                jack_info ( "\tMixerName  = %s", snd_ctl_card_info_get_mixername ( ci ) );
                jack_info ( "\tComponents = %s", snd_ctl_card_info_get_components ( ci ) );
                jack_info ( "--------------" );
            }

            void printHWParams ( snd_pcm_hw_params_t* params )
            {
                jack_info ( "HW Params info (address : %p)\n", params );
#if 0
                jack_info ( "\tChannels    = %d", snd_pcm_hw_params_get_channels ( params, NULL ) );
                jack_info ( "\tFormat      = %s", snd_pcm_format_name ( ( _snd_pcm_format ) snd_pcm_hw_params_get_format ( params, NULL ) ) );
                jack_info ( "\tAccess      = %s", snd_pcm_access_name ( ( _snd_pcm_access ) snd_pcm_hw_params_get_access ( params, NULL ) ) );
                jack_info ( "\tRate        = %d", snd_pcm_hw_params_get_rate ( params, NULL, NULL ) );
                jack_info ( "\tPeriods     = %d", snd_pcm_hw_params_get_periods ( params, NULL, NULL ) );
                jack_info ( "\tPeriod size = %d", ( int ) snd_pcm_hw_params_get_period_size ( params, NULL, NULL ) );
                jack_info ( "\tPeriod time = %d", snd_pcm_hw_params_get_period_time ( params, NULL, NULL ) );
                jack_info ( "\tBuffer size = %d", ( int ) snd_pcm_hw_params_get_buffer_size ( params, NULL ) );
                jack_info ( "\tBuffer time = %d", snd_pcm_hw_params_get_buffer_time ( params, NULL, NULL ) );
#endif
                jack_info ( "--------------" );
            }
    };

    /*!
    \brief Audio adapter using ALSA API.
    */

    class JackAlsaAdapter : public JackAudioAdapterInterface, public JackRunnableInterface
    {

        private:
            JackThread fThread;
            AudioInterface fAudioInterface;

        public:
            JackAlsaAdapter ( jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params );
            ~JackAlsaAdapter()
            {}

            virtual int Open();
            virtual int Close();

            virtual int SetSampleRate ( jack_nframes_t sample_rate );
            virtual int SetBufferSize ( jack_nframes_t buffer_size );

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
