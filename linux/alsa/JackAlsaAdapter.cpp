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

#include "JackAlsaAdapter.h"
#include "JackServerGlobals.h"
#include "JackEngineControl.h"

namespace Jack
{

    JackAlsaAdapter::JackAlsaAdapter ( jack_nframes_t buffer_size, jack_nframes_t sample_rate, const JSList* params ) :
            JackAudioAdapterInterface ( buffer_size, sample_rate ),
            fThread ( this ),
            fAudioInterface ( buffer_size, sample_rate )
    {
        const JSList* node;
        const jack_driver_param_t* param;

        fCaptureChannels = 2;
        fPlaybackChannels = 2;

        fAudioInterface.fPeriod = 2;

        for ( node = params; node; node = jack_slist_next ( node ) )
        {
            param = ( const jack_driver_param_t* ) node->data;

            switch ( param->character )
            {
                case 'i':
                    fCaptureChannels = param->value.ui;
                    break;
                case 'o':
                    fPlaybackChannels = param->value.ui;
                    break;
                case 'C':
                    break;
                case 'P':
                    break;
                case 'D':
                    break;
                case 'n':
                    fAudioInterface.fPeriod = param->value.ui;
                    break;
                case 'd':
                    fAudioInterface.fCardName = strdup ( param->value.str );
                    break;
                case 'r':
                    SetAdaptedSampleRate ( param->value.ui );
                    break;
                case 'p':
                    SetAdaptedBufferSize ( param->value.ui );
                    break;
                case 'q':
                    fQuality = param->value.ui;
                    break;
            }
        }

        fAudioInterface.setInputs ( fCaptureChannels );
        fAudioInterface.setOutputs ( fPlaybackChannels );
    }

    int JackAlsaAdapter::Open()
    {
        //open audio interface
        if ( fAudioInterface.open() )
            return -1;

        //start adapter thread
        if ( fThread.StartSync() < 0 )
        {
            jack_error ( "Cannot start audioadapter thread" );
            return -1;
        }

        //display card info
        fAudioInterface.longinfo();

        //turn the thread realtime
        fThread.AcquireRealTime ( JackServerGlobals::fInstance->GetEngineControl()->fClientPriority );

        return 0;
    }

    int JackAlsaAdapter::Close()
    {
#ifdef JACK_MONITOR
        fTable.Save();
#endif
        switch ( fThread.GetStatus() )
        {

                // Kill the thread in Init phase
            case JackThread::kStarting:
            case JackThread::kIniting:
                if ( fThread.Kill() < 0 )
                {
                    jack_error ( "Cannot kill thread" );
                    return -1;
                }
                break;

                // Stop when the thread cycle is finished
            case JackThread::kRunning:
                if ( fThread.Stop() < 0 )
                {
                    jack_error ( "Cannot stop thread" );
                    return -1;
                }
                break;

            default:
                break;
        }
        return fAudioInterface.close();
    }

    bool JackAlsaAdapter::Init()
    {
        //fill the hardware buffers
        for ( unsigned int i = 0; i < fAudioInterface.fPeriod; i++ )
            fAudioInterface.write();
        return true;
    }

    bool JackAlsaAdapter::Execute()
    {
        //read data from audio interface
        if ( fAudioInterface.read() < 0 )
            return false;

        bool failure = false;

        //compute resampling factor
        jack_nframes_t time1, time2;
        ResampleFactor ( time1, time2 );

        //resample inputs
        for ( int i = 0; i < fCaptureChannels; i++ )
        {
            fCaptureRingBuffer[i]->SetRatio ( time1, time2 );
            if ( fCaptureRingBuffer[i]->WriteResample ( fAudioInterface.fInputSoftChannels[i], fAdaptedBufferSize ) < fAdaptedBufferSize )
                failure = true;
        }
        //resample outputs
        for ( int i = 0; i < fPlaybackChannels; i++ )
        {
            fPlaybackRingBuffer[i]->SetRatio ( time2, time1 );
            if ( fPlaybackRingBuffer[i]->ReadResample ( fAudioInterface.fOutputSoftChannels[i], fAdaptedBufferSize ) < fAdaptedBufferSize )
                failure = true;
        }

#ifdef JACK_MONITOR
        fTable.Write ( time1, time2, double ( time1 ) / double ( time2 ), double ( time2 ) / double ( time1 ),
                       fCaptureRingBuffer[0]->ReadSpace(), fPlaybackRingBuffer[0]->WriteSpace() );
#endif

        //write data to audio interface
        if ( fAudioInterface.write() < 0 )
            return false;

        //reset all ringbuffers in case of failure
        if ( failure )
        {
            jack_error ( "JackAlsaAdapter::Execute ringbuffer failure... reset" );
            ResetRingBuffers();
        }

        return true;
    }

    int JackAlsaAdapter::SetSampleRate ( jack_nframes_t sample_rate )
    {
        JackAudioAdapterInterface::SetHostSampleRate ( sample_rate );
        Close();
        return Open();
    }

    int JackAlsaAdapter::SetBufferSize ( jack_nframes_t buffer_size )
    {
        JackAudioAdapterInterface::SetHostBufferSize ( buffer_size );
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
        jack_driver_desc_t *desc;
        unsigned int i;
        desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );

        strcpy ( desc->name, "audioadapter" );                         // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy ( desc->desc, "netjack audio <==> net backend adapter" );  // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 10;
        desc->params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

        i = 0;
        strcpy ( desc->params[i].name, "capture" );
        desc->params[i].character = 'C';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "none" );
        strcpy ( desc->params[i].short_desc,
                 "Provide capture ports.  Optionally set device" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "playback" );
        desc->params[i].character = 'P';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "none" );
        strcpy ( desc->params[i].short_desc,
                 "Provide playback ports.  Optionally set device" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "device" );
        desc->params[i].character = 'd';
        desc->params[i].type = JackDriverParamString;
        strcpy ( desc->params[i].value.str, "hw:0" );
        strcpy ( desc->params[i].short_desc, "ALSA device name" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "rate" );
        desc->params[i].character = 'r';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 48000U;
        strcpy ( desc->params[i].short_desc, "Sample rate" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "periodsize" );
        desc->params[i].character = 'p';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 512U;
        strcpy ( desc->params[i].short_desc, "Period size" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "nperiods" );
        desc->params[i].character = 'n';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.ui = 2U;
        strcpy ( desc->params[i].short_desc, "Number of periods of playback latency" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "duplex" );
        desc->params[i].character = 'D';
        desc->params[i].type = JackDriverParamBool;
        desc->params[i].value.i = 1;
        strcpy ( desc->params[i].short_desc,
                 "Provide both capture and playback ports" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "inchannels" );
        desc->params[i].character = 'i';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy ( desc->params[i].short_desc,
                 "Number of capture channels (defaults to hardware max)" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

        i++;
        strcpy ( desc->params[i].name, "outchannels" );
        desc->params[i].character = 'o';
        desc->params[i].type = JackDriverParamUInt;
        desc->params[i].value.i = 0;
        strcpy ( desc->params[i].short_desc,
                 "Number of playback channels (defaults to hardware max)" );
        strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );
        
    
        i++;
        strcpy(desc->params[i].name, "quality");
        desc->params[i].character = 'q';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "Resample algorithm quality (0 - 4)");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        return desc;
    }

#ifdef __cplusplus
}
#endif

