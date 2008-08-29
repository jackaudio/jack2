/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2008 Romain Moret at Grame

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

#include "JackNetDriver.h"
#include "JackEngineControl.h"
#include "JackClientControl.h"
#include "JackGraphManager.h"
#include "JackDriverLoader.h"
#include "JackThreadedDriver.h"
#include "JackWaitThreadedDriver.h"

#define DEFAULT_MULTICAST_IP "225.3.19.154"
#define DEFAULT_PORT 19000

using namespace std;

namespace Jack
{
    JackNetDriver::JackNetDriver ( const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                   const char* ip, int port, int mtu, int midi_input_ports, int midi_output_ports,
                                   const char* net_name, uint transport_sync, char network_mode )
            : JackAudioDriver ( name, alias, engine, table ), JackNetSlaveInterface ( ip, port )
    {
        jack_log ( "JackNetDriver::JackNetDriver ip %s, port %d", ip, port );

        fParams.fMtu = mtu;
        fParams.fSendMidiChannels = midi_input_ports;
        fParams.fReturnMidiChannels = midi_output_ports;
        strcpy ( fParams.fName, net_name );
        fSocket.GetName ( fParams.fSlaveNetName );
        fParams.fTransportSync = transport_sync;
        fParams.fNetworkMode = network_mode;
        fLastTimebaseMaster = -1;
#ifdef JACK_MONITOR
        fNetTimeMon = NULL;
#endif
    }

    JackNetDriver::~JackNetDriver()
    {
        delete[] fMidiCapturePortList;
        delete[] fMidiPlaybackPortList;
#ifdef JACK_MONITOR
        delete fNetTimeMon;
#endif
    }

//open, close, attach and detach------------------------------------------------------
    int JackNetDriver::Open ( jack_nframes_t buffer_size, jack_nframes_t samplerate, bool capturing, bool playing,
                              int inchannels, int outchannels, bool monitor,
                              const char* capture_driver_name, const char* playback_driver_name,
                              jack_nframes_t capture_latency, jack_nframes_t playback_latency )
    {
        int res = JackAudioDriver::Open ( buffer_size, samplerate, capturing, playing, inchannels, outchannels, monitor,
                                          capture_driver_name, playback_driver_name, capture_latency, playback_latency );
        fEngineControl->fPeriod = 0;
        fEngineControl->fComputation = 500 * 1000;
        fEngineControl->fConstraint = 500 * 1000;
        return res;
    }

#ifdef JACK_MONITOR
    int JackNetDriver::Close()
    {
        if ( fNetTimeMon )
            fNetTimeMon->Save();
        return JackDriver::Close();
    }
#endif

    int JackNetDriver::Attach()
    {
        return 0;
    }

    int JackNetDriver::Detach()
    {
        return 0;
    }

//init and restart--------------------------------------------------------------------
    bool JackNetDriver::Init()
    {
        jack_log ( "JackNetDriver::Init()" );

        //new loading, but existing socket, restart the driver
        if ( fSocket.IsSocket() )
            Restart();

        //set the parameters to send
        fParams.fSendAudioChannels = fCaptureChannels;
        fParams.fReturnAudioChannels = fPlaybackChannels;
        fParams.fSlaveSyncMode = fEngineControl->fSyncMode;

        //display some additional infos
        jack_info ( "Net driver started in %s mode %s Master's transport sync.",
                    ( fParams.fSlaveSyncMode ) ? "sync" : "async", ( fParams.fTransportSync ) ? "with" : "without" );

        //init network
        if ( !JackNetSlaveInterface::Init() )
            return false;

        //set global paramaters
        SetParams();

        //driver parametering
        JackAudioDriver::SetBufferSize ( fParams.fPeriodSize );
        JackAudioDriver::SetSampleRate ( fParams.fSampleRate );

        JackDriver::NotifyBufferSize ( fParams.fPeriodSize );
        JackDriver::NotifySampleRate ( fParams.fSampleRate );

        //transport engine parametering
        fEngineControl->fTransport.SetNetworkSync ( true );

        //allocate midi ports lists
        fMidiCapturePortList = new jack_port_id_t [fParams.fSendMidiChannels];
        fMidiPlaybackPortList = new jack_port_id_t [fParams.fReturnMidiChannels];

        //register jack ports
        if ( AllocPorts() != 0 )
        {
            jack_error ( "Can't allocate ports." );
            return false;
        }

        //init done, display parameters
        SessionParamsDisplay ( &fParams );

        //monitor
#ifdef JACK_MONITOR
        string plot_name;
        //NetTimeMon
        plot_name = string ( fParams.fName );
        plot_name += string ( "_slave" );
        plot_name += ( fEngineControl->fSyncMode ) ? string ( "_sync" ) : string ( "_async" );
        switch ( fParams.fNetworkMode )
        {
            case 's' :
                plot_name += string ( "_slow" );
                break;
            case 'n' :
                plot_name += string ( "_normal" );
                break;
            case 'f' :
                plot_name += string ( "_fast" );
                break;
        }
        fNetTimeMon = new JackGnuPlotMonitor<float> ( 128, 5, plot_name );
        string net_time_mon_fields[] =
        {
            string ( "sync decoded" ),
            string ( "end of read" ),
            string ( "start of write" ),
            string ( "sync send" ),
            string ( "end of write" )
        };
        string net_time_mon_options[] =
        {
            string ( "set xlabel \"audio cycles\"" ),
            string ( "set ylabel \"% of audio cycle\"" )
        };
        fNetTimeMon->SetPlotFile ( net_time_mon_options, 2, net_time_mon_fields, 5 );
#endif

        return true;
    }

    void JackNetDriver::Restart()
    {
        jack_log ( "JackNetDriver::Restart" );

        jack_info ( "Restarting driver..." );
        delete[] fTxBuffer;
        fTxBuffer = NULL;
        delete[] fRxBuffer;
        fRxBuffer = NULL;
        delete fNetAudioCaptureBuffer;
        fNetAudioCaptureBuffer = NULL;
        delete fNetAudioPlaybackBuffer;
        fNetAudioPlaybackBuffer = NULL;
        delete fNetMidiCaptureBuffer;
        fNetMidiCaptureBuffer = NULL;
        delete fNetMidiPlaybackBuffer;
        fNetMidiPlaybackBuffer = NULL;
        FreePorts();
        delete[] fMidiCapturePortList;
        fMidiCapturePortList = NULL;
        delete[] fMidiPlaybackPortList;
        fMidiPlaybackPortList = NULL;
#ifdef JACK_MONITOR
        delete fNetTimeMon;
        fNetTimeMon = NULL;
#endif
    }

//jack ports and buffers--------------------------------------------------------------
    int JackNetDriver::AllocPorts()
    {
        jack_log ( "JackNetDriver::AllocPorts fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate );

        JackPort* port;
        jack_port_id_t port_id;
        char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        unsigned long port_flags;
        int audio_port_index;
        uint midi_port_index;

        //audio
        port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
        for ( audio_port_index = 0; audio_port_index < fCaptureChannels; audio_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, audio_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:capture_%d", fClientControl.fName, audio_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            port = fGraphManager->GetPort ( port_id );
            port->SetAlias ( alias );
            //port latency
            port->SetLatency ( fEngineControl->fBufferSize );
            fCapturePortList[audio_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fCapturePortList[%d] audio_port_index = %ld fPortLatency = %ld", audio_port_index, port_id, port->GetLatency() );
        }
        port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
        for ( audio_port_index = 0; audio_port_index < fPlaybackChannels; audio_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, audio_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:playback_%d",fClientControl.fName, audio_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_AUDIO_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            port = fGraphManager->GetPort ( port_id );
            port->SetAlias ( alias );
            //port latency
            switch ( fParams.fNetworkMode )
            {
                case 'f' :
                    port->SetLatency ( ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize );
                    break;
                case 'n' :
                    port->SetLatency ( fEngineControl->fBufferSize + ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize );
                    break;
                case 's' :
                    port->SetLatency ( 2 * fEngineControl->fBufferSize + ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize );
                    break;
            }
            fPlaybackPortList[audio_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fPlaybackPortList[%d] audio_port_index = %ld fPortLatency = %ld", audio_port_index, port_id, port->GetLatency() );
        }
        //midi
        port_flags = JackPortIsOutput | JackPortIsPhysical | JackPortIsTerminal;
        for ( midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, midi_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:midi_capture_%d", fClientControl.fName, midi_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            port = fGraphManager->GetPort ( port_id );
            //port latency
            port->SetLatency ( fEngineControl->fBufferSize );
            fMidiCapturePortList[midi_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fMidiCapturePortList[%d] midi_port_index = %ld fPortLatency = %ld", midi_port_index, port_id, port->GetLatency() );
        }

        port_flags = JackPortIsInput | JackPortIsPhysical | JackPortIsTerminal;
        for ( midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++ )
        {
            snprintf ( alias, sizeof ( alias ) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, midi_port_index + 1 );
            snprintf ( name, sizeof ( name ) - 1, "%s:midi_playback_%d", fClientControl.fName, midi_port_index + 1 );
            if ( ( port_id = fGraphManager->AllocatePort ( fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE,
                             static_cast<JackPortFlags> ( port_flags ), fEngineControl->fBufferSize ) ) == NO_PORT )
            {
                jack_error ( "driver: cannot register port for %s", name );
                return -1;
            }
            port = fGraphManager->GetPort ( port_id );
            //port latency
            switch ( fParams.fNetworkMode )
            {
                case 'f' :
                    port->SetLatency ( ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize );
                    break;
                case 'n' :
                    port->SetLatency ( fEngineControl->fBufferSize + ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize ) ;
                    break;
                case 's' :
                    port->SetLatency ( 2 * fEngineControl->fBufferSize + ( fEngineControl->fSyncMode ) ? 0 : fEngineControl->fBufferSize );
                    break;
            }
            fMidiPlaybackPortList[midi_port_index] = port_id;
            jack_log ( "JackNetDriver::AllocPorts() fMidiPlaybackPortList[%d] midi_port_index = %ld fPortLatency = %ld", midi_port_index, port_id, port->GetLatency() );
        }

        return 0;
    }

    int JackNetDriver::FreePorts()
    {
        jack_log ( "JackNetDriver::FreePorts" );

        int audio_port_index;
        uint midi_port_index;
        for ( audio_port_index = 0; audio_port_index < fCaptureChannels; audio_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fCapturePortList[audio_port_index] );
        for ( audio_port_index = 0; audio_port_index < fPlaybackChannels; audio_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fPlaybackPortList[audio_port_index] );
        for ( midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fMidiCapturePortList[midi_port_index] );
        for ( midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++ )
            fGraphManager->ReleasePort ( fClientControl.fRefNum, fMidiPlaybackPortList[midi_port_index] );
        return 0;
    }

    JackMidiBuffer* JackNetDriver::GetMidiInputBuffer ( int port_index )
    {
        return static_cast<JackMidiBuffer*> ( fGraphManager->GetBuffer ( fMidiCapturePortList[port_index], fEngineControl->fBufferSize ) );
    }

    JackMidiBuffer* JackNetDriver::GetMidiOutputBuffer ( int port_index )
    {
        return static_cast<JackMidiBuffer*> ( fGraphManager->GetBuffer ( fMidiPlaybackPortList[port_index], fEngineControl->fBufferSize ) );
    }

//transport---------------------------------------------------------------------------
    int JackNetDriver::DecodeTransportData()
    {
        //is there a new timebase master on the net master ?
        // - release timebase master only if it's a non-conditional request
        // - no change or no request : don't do anything
        // - conditional request : don't change anything too, the master will know if this slave is actually the timebase master
        int refnum;
        bool conditional;
        if ( fSendTransportData.fTimebaseMaster == TIMEBASEMASTER )
        {
            fEngineControl->fTransport.GetTimebaseMaster ( refnum, conditional );
            if ( refnum != -1 )
                fEngineControl->fTransport.ResetTimebase ( refnum );
            jack_info ( "The NetMaster is now the new timebase master." );
        }

        //is there a tranport state change to handle ?
        if ( fSendTransportData.fNewState && ( fSendTransportData.fState != fEngineControl->fTransport.GetState() ) )
        {
            switch ( fSendTransportData.fState )
            {
                case JackTransportStopped :
                    fEngineControl->fTransport.SetCommand ( TransportCommandStop );
                    jack_info ( "Master stops transport." );
                    break;
                case JackTransportStarting :
                    fEngineControl->fTransport.RequestNewPos ( &fSendTransportData.fPosition );
                    fEngineControl->fTransport.SetCommand ( TransportCommandStart );
                    jack_info ( "Master starts transport." );
                    break;
                case JackTransportRolling :
                    fEngineControl->fTransport.SetState ( JackTransportRolling );
                    jack_info ( "Master is rolling." );
                    break;
            }
        }

        return 0;
    }

    int JackNetDriver::EncodeTransportData()
    {
        //is there a timebase master change ?
        int refnum;
        bool conditional;
        fEngineControl->fTransport.GetTimebaseMaster ( refnum, conditional );
        if ( refnum != fLastTimebaseMaster )
        {
            //timebase master has released its function
            if ( refnum == -1 )
            {
                fReturnTransportData.fTimebaseMaster = RELEASE_TIMEBASEMASTER;
                jack_info ( "Sending a timebase master release request." );
            }
            //there is a new timebase master
            else
            {
                fReturnTransportData.fTimebaseMaster = ( conditional ) ? CONDITIONAL_TIMEBASEMASTER : TIMEBASEMASTER;
                jack_info ( "Sending a %s timebase master request.", ( conditional ) ? "conditional" : "non-conditional" );
            }
            fLastTimebaseMaster = refnum;
        }
        else
            fReturnTransportData.fTimebaseMaster = NO_CHANGE;

        //update transport state and position
        fReturnTransportData.fState = fEngineControl->fTransport.Query ( &fReturnTransportData.fPosition );

        //is it a new state (that the master need to know...) ?
        fReturnTransportData.fNewState = ( ( fReturnTransportData.fState != fLastTransportState ) &&
                                           ( fReturnTransportData.fState != fSendTransportData.fState ) );
        if ( fReturnTransportData.fNewState )
            jack_info ( "Sending '%s'.", GetTransportState ( fReturnTransportData.fState ) );
        fLastTransportState = fReturnTransportData.fState;

        return 0;
    }

//network sync------------------------------------------------------------------------
    int JackNetDriver::DecodeSyncPacket()
    {
        //this method contains every step of sync packet informations decoding process
        //first : transport
        if ( fParams.fTransportSync )
        {
            //copy received transport data to transport data structure
            memcpy ( &fSendTransportData, fRxData, sizeof ( net_transport_data_t ) );
            if ( DecodeTransportData() < 0 )
                return -1;
        }
        //then others
        //...
        return 0;
    }

    int JackNetDriver::EncodeSyncPacket()
    {
        //this method contains every step of sync packet informations coding
        //first of all, reset sync packet
        memset ( fTxData, 0, fPayloadSize );
        //then first step : transport
        if ( fParams.fTransportSync )
        {
            if ( EncodeTransportData() < 0 )
                return -1;
            //copy to TxBuffer
            memcpy ( fTxData, &fReturnTransportData, sizeof ( net_transport_data_t ) );
        }
        //then others
        //...
        return 0;
    }

//driver processes--------------------------------------------------------------------
    int JackNetDriver::Read()
    {
        uint midi_port_index;
        uint audio_port_index;

        //buffers
        for ( midi_port_index = 0; midi_port_index < fParams.fSendMidiChannels; midi_port_index++ )
            fNetMidiCaptureBuffer->SetBuffer ( midi_port_index, GetMidiInputBuffer ( midi_port_index ) );
        for ( audio_port_index = 0; audio_port_index < fParams.fSendAudioChannels; audio_port_index++ )
            fNetAudioCaptureBuffer->SetBuffer ( audio_port_index, GetInputBuffer ( audio_port_index ) );

#ifdef JACK_MONITOR
        fNetTimeMon->New();
#endif

        //receive sync (launch the cycle)
        if ( SyncRecv() == SOCKET_ERROR )
            return 0;

        //take the time at the beginning of the cycle
        JackDriver::CycleTakeBeginTime();

        //decode sync
        //if there is an error, don't return -1, it will skip Write() and the network error probably won't be identified
        if ( DecodeSyncPacket() < 0 )
            return 0;

#ifdef JACK_MONITOR
        fNetTimeMon->Add ( ( ( float ) ( GetMicroSeconds() - JackDriver::fBeginDateUst ) / ( float ) fEngineControl->fPeriodUsecs ) * 100.f );
#endif
        //audio, midi or sync if driver is late
        if ( DataRecv() == SOCKET_ERROR )
            return SOCKET_ERROR;

#ifdef JACK_MONITOR
        fNetTimeMon->Add ( ( ( float ) ( GetMicroSeconds() - JackDriver::fBeginDateUst ) / ( float ) fEngineControl->fPeriodUsecs ) * 100.f );
#endif

        return 0;
    }

    int JackNetDriver::Write()
    {
        uint midi_port_index;
        int audio_port_index;

        //buffers
        for ( midi_port_index = 0; midi_port_index < fParams.fReturnMidiChannels; midi_port_index++ )
            fNetMidiPlaybackBuffer->SetBuffer ( midi_port_index, GetMidiOutputBuffer ( midi_port_index ) );
        for ( audio_port_index = 0; audio_port_index < fPlaybackChannels; audio_port_index++ )
            fNetAudioPlaybackBuffer->SetBuffer ( audio_port_index, GetOutputBuffer ( audio_port_index ) );

#ifdef JACK_MONITOR
        fNetTimeMon->Add ( ( ( float ) ( GetMicroSeconds() - JackDriver::fBeginDateUst ) / ( float ) fEngineControl->fPeriodUsecs ) * 100.f );
#endif

        //sync
        if ( EncodeSyncPacket() < 0 )
            return 0;

        //send sync
        if ( SyncSend() == SOCKET_ERROR )
            return SOCKET_ERROR;

#ifdef JACK_MONITOR
        fNetTimeMon->Add ( ( ( float ) ( GetMicroSeconds() - JackDriver::fBeginDateUst ) / ( float ) fEngineControl->fPeriodUsecs ) * 100.f );
#endif

        //send data
        if ( DataSend() == SOCKET_ERROR )
            return SOCKET_ERROR;

#ifdef JACK_MONITOR
        fNetTimeMon->AddLast ( ( ( float ) ( GetMicroSeconds() - JackDriver::fBeginDateUst ) / ( float ) fEngineControl->fPeriodUsecs ) * 100.f );
#endif

        return 0;
    }

//driver loader-----------------------------------------------------------------------

#ifdef __cplusplus
    extern "C"
    {
#endif
        EXPORT jack_driver_desc_t* driver_get_descriptor ()
        {
            jack_driver_desc_t* desc = ( jack_driver_desc_t* ) calloc ( 1, sizeof ( jack_driver_desc_t ) );
            strcpy ( desc->name, "net" );
            desc->nparams = 10;
            desc->params = ( jack_driver_param_desc_t* ) calloc ( desc->nparams, sizeof ( jack_driver_param_desc_t ) );

            int i = 0;
            strcpy ( desc->params[i].name, "multicast_ip" );
            desc->params[i].character = 'a';
            desc->params[i].type = JackDriverParamString;
            strcpy ( desc->params[i].value.str, DEFAULT_MULTICAST_IP );
            strcpy ( desc->params[i].short_desc, "Multicast Address" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "udp_net_port" );
            desc->params[i].character = 'p';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 19000;
            strcpy ( desc->params[i].short_desc, "UDP port" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "mtu" );
            desc->params[i].character = 'M';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 1500;
            strcpy ( desc->params[i].short_desc, "MTU to the master" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "input_ports" );
            desc->params[i].character = 'C';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 2;
            strcpy ( desc->params[i].short_desc, "Number of audio input ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "output_ports" );
            desc->params[i].character = 'P';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 2;
            strcpy ( desc->params[i].short_desc, "Number of audio output ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "midi_in_ports" );
            desc->params[i].character = 'i';
            desc->params[i].type = JackDriverParamInt;
            desc->params[i].value.i = 0;
            strcpy ( desc->params[i].short_desc, "Number of midi input ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "midi_out_ports" );
            desc->params[i].character = 'o';
            desc->params[i].type = JackDriverParamUInt;
            desc->params[i].value.i = 0;
            strcpy ( desc->params[i].short_desc, "Number of midi output ports" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "client_name" );
            desc->params[i].character = 'n';
            desc->params[i].type = JackDriverParamString;
            strcpy ( desc->params[i].value.str, "'hostname'" );
            strcpy ( desc->params[i].short_desc, "Name of the jack client" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "transport_sync" );
            desc->params[i].character  = 't';
            desc->params[i].type = JackDriverParamUInt;
            desc->params[i].value.ui = 1U;
            strcpy ( desc->params[i].short_desc, "Sync transport with master's" );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            i++;
            strcpy ( desc->params[i].name, "mode" );
            desc->params[i].character  = 'm';
            desc->params[i].type = JackDriverParamString;
            strcpy ( desc->params[i].value.str, "normal" );
            strcpy ( desc->params[i].short_desc, "Slow, Normal or Fast mode." );
            strcpy ( desc->params[i].long_desc, desc->params[i].short_desc );

            return desc;
        }

        EXPORT Jack::JackDriverClientInterface* driver_initialize ( Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params )
        {
            if ( SocketAPIInit() < 0 )
            {
                jack_error ( "Can't init Socket API, exiting..." );
                return NULL;
            }
            char multicast_ip[16];
            strcpy ( multicast_ip, DEFAULT_MULTICAST_IP );
            char name[JACK_CLIENT_NAME_SIZE];
            GetHostName ( name, JACK_CLIENT_NAME_SIZE );
            int udp_port = DEFAULT_PORT;
            int mtu = 1500;
            uint transport_sync = 1;
            jack_nframes_t period_size = 128;
            jack_nframes_t sample_rate = 48000;
            int audio_capture_ports = 2;
            int audio_playback_ports = 2;
            int midi_input_ports = 0;
            int midi_output_ports = 0;
            bool monitor = false;
            char network_mode = 'n';

            const JSList* node;
            const jack_driver_param_t* param;
            for ( node = params; node; node = jack_slist_next ( node ) )
            {
                param = ( const jack_driver_param_t* ) node->data;
                switch ( param->character )
                {
                    case 'a' :
                        strncpy ( multicast_ip, param->value.str, 15 );
                        break;
                    case 'p':
                        udp_port = param->value.ui;
                        break;
                    case 'M':
                        mtu = param->value.i;
                        break;
                    case 'C':
                        audio_capture_ports = param->value.i;
                        break;
                    case 'P':
                        audio_playback_ports = param->value.i;
                        break;
                    case 'i':
                        midi_input_ports = param->value.i;
                        break;
                    case 'o':
                        midi_output_ports = param->value.i;
                        break;
                    case 'n' :
                        strncpy ( name, param->value.str, JACK_CLIENT_NAME_SIZE );
                        break;
                    case 't' :
                        transport_sync = param->value.ui;
                        break;
                    case 'm' :
                        if ( strcmp ( param->value.str, "normal" ) == 0 )
                            network_mode = 'n';
                        else if ( strcmp ( param->value.str, "slow" ) == 0 )
                            network_mode = 's';
                        else if ( strcmp ( param->value.str, "fast" ) == 0 )
                            network_mode = 'f';
                        else
                            jack_error ( "Unknown network mode, using 'normal' mode." );
                        break;
                }
            }

            Jack::JackDriverClientInterface* driver = new Jack::JackWaitThreadedDriver (
                new Jack::JackNetDriver ( "system", "net_pcm", engine, table, multicast_ip, udp_port, mtu,
                                          midi_input_ports, midi_output_ports, name, transport_sync, network_mode ) );
            if ( driver->Open ( period_size, sample_rate, 1, 1, audio_capture_ports, audio_playback_ports,
                                monitor, "from_master_", "to_master_", 0, 0 ) == 0 )
                return driver;

            delete driver;
            return NULL;
        }

#ifdef __cplusplus
    }
#endif
}
