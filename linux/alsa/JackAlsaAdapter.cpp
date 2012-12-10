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
#include "JackGlobals.h"
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
                    if (strncmp(param->value.str,"none",4) != 0) {
                        fAudioInterface.fCaptureName = strdup ( param->value.str );
                    }
                    break;
                case 'P':
                    if  (strncmp(param->value.str,"none",4) != 0) {
                        fAudioInterface.fPlaybackName = strdup ( param->value.str );
                    }
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
                    fAudioInterface.fFrequency = param->value.ui;
                    SetAdaptedSampleRate ( param->value.ui );
                    break;
                case 'p':
                    fAudioInterface.fBuffering = param->value.ui;
                    SetAdaptedBufferSize ( param->value.ui );
                    break;
                case 'q':
                    fQuality = param->value.ui;
                    break;
                case 'g':
                    fRingbufferCurSize = param->value.ui;
                    fAdaptative = false;
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
        fThread.AcquireRealTime(GetEngineControl()->fClientPriority);
        return 0;
    }

    int JackAlsaAdapter::Close()
    {
#ifdef JACK_MONITOR
        fTable.Save(fHostBufferSize, fHostSampleRate, fAdaptedSampleRate, fAdaptedBufferSize);
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
        if (fAudioInterface.read() < 0)
            return false;

        PushAndPull(fAudioInterface.fInputSoftChannels, fAudioInterface.fOutputSoftChannels, fAdaptedBufferSize);

        //write data to audio interface
        if (fAudioInterface.write() < 0)
            return false;

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
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("audioadapter", JackDriverNone, "netjack audio <==> net backend adapter", &filler);

        strcpy(value.str, "none");
        jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Provide capture ports.  Optionally set device", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Provide playback ports.  Optionally set device", NULL);

        strcpy(value.str, "hw:0");
        jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "ALSA device name", NULL);

        value.ui  = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui  = 512U;
        jack_driver_descriptor_add_parameter(desc, &filler, "periodsize", 'p', JackDriverParamUInt, &value, NULL, "Period size", NULL);

        value.ui  = 2U;
        jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods of playback latency", NULL);

        value.i  = true;
        jack_driver_descriptor_add_parameter(desc, &filler, "duplex", 'D', JackDriverParamBool, &value, NULL, "Provide both capture and playback ports", NULL);

        value.i  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "in-channels", 'i', JackDriverParamInt, &value, NULL, "Number of capture channels (defaults to hardware max)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "out-channels", 'o', JackDriverParamInt, &value, NULL, "Number of playback channels (defaults to hardware max)", NULL);

        value.ui  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "quality", 'q', JackDriverParamUInt, &value, NULL, "Resample algorithm quality (0 - 4)", NULL);

        value.ui = 32768;
        jack_driver_descriptor_add_parameter(desc, &filler, "ring-buffer", 'g', JackDriverParamUInt, &value, NULL, "Fixed ringbuffer size", "Fixed ringbuffer size (if not set => automatic adaptative)");

        return desc;
    }

#ifdef __cplusplus
}
#endif

