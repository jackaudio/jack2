/*
Copyright (C) 2008-2011 Romain Moret at Grame

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

#include "JackNetInterface.h"
#include "JackException.h"
#include "JackError.h"

#include <assert.h>

using namespace std;

/*
 TODO : since midi buffers now uses up to BUFFER_SIZE_MAX frames,
 probably also use BUFFER_SIZE_MAX in everything related to MIDI events
 handling (see MidiBufferInit in JackMidiPort.cpp)
*/

namespace Jack
{
    // JackNetInterface*******************************************

    JackNetInterface::JackNetInterface() : fSocket()
    {
        Initialize();
    }

    JackNetInterface::JackNetInterface(const char* multicast_ip, int port) : fSocket(multicast_ip, port)
    {
        strcpy(fMulticastIP, multicast_ip);
        Initialize();
    }

    JackNetInterface::JackNetInterface(session_params_t& params, JackNetSocket& socket, const char* multicast_ip) : fSocket(socket)
    {
        fParams = params;
        strcpy(fMulticastIP, multicast_ip);
        Initialize();
    }

    void JackNetInterface::Initialize()
    {
        fSetTimeOut = false;
        fTxBuffer = NULL;
        fRxBuffer = NULL;
        fNetAudioCaptureBuffer = NULL;
        fNetAudioPlaybackBuffer = NULL;
        fNetMidiCaptureBuffer = NULL;
        fNetMidiPlaybackBuffer = NULL;
        memset(&fSendTransportData, 0, sizeof(net_transport_data_t));
        memset(&fReturnTransportData, 0, sizeof(net_transport_data_t));
    }

    void JackNetInterface::FreeNetworkBuffers()
    {
        delete fNetMidiCaptureBuffer;
        delete fNetMidiPlaybackBuffer;
        delete fNetAudioCaptureBuffer;
        delete fNetAudioPlaybackBuffer;
        fNetMidiCaptureBuffer = NULL;
        fNetMidiPlaybackBuffer = NULL;
        fNetAudioCaptureBuffer = NULL;
        fNetAudioPlaybackBuffer = NULL;
    }

    JackNetInterface::~JackNetInterface()
    {
        jack_log("JackNetInterface::~JackNetInterface");

        fSocket.Close();
        delete[] fTxBuffer;
        delete[] fRxBuffer;
        delete fNetAudioCaptureBuffer;
        delete fNetAudioPlaybackBuffer;
        delete fNetMidiCaptureBuffer;
        delete fNetMidiPlaybackBuffer;
    }

    int JackNetInterface::SetNetBufferSize()
    {
        // audio
        float audio_size = (fNetAudioCaptureBuffer)
                        ? fNetAudioCaptureBuffer->GetCycleSize()
                        : (fNetAudioPlaybackBuffer) ? fNetAudioPlaybackBuffer->GetCycleSize() : 0;
        jack_log("audio_size %f", audio_size);

        // midi
        float midi_size = (fNetMidiCaptureBuffer)
                        ? fNetMidiCaptureBuffer->GetCycleSize()
                        : (fNetMidiPlaybackBuffer) ? fNetMidiPlaybackBuffer->GetCycleSize() : 0;
        jack_log("midi_size %f", midi_size);

        // bufsize = sync + audio + midi
        int bufsize = NETWORK_MAX_LATENCY * (fParams.fMtu + (int)audio_size + (int)midi_size);
        jack_log("SetNetBufferSize bufsize = %d", bufsize);

        // tx buffer
        if (fSocket.SetOption(SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        // rx buffer
        if (fSocket.SetOption(SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }

        return 0;
    }

    bool JackNetInterface::SetParams()
    {
        // TX header init
        strcpy(fTxHeader.fPacketType, "header");
        fTxHeader.fID = fParams.fID;
        fTxHeader.fCycle = 0;
        fTxHeader.fSubCycle = 0;
        fTxHeader.fIsLastPckt = 0;

        // RX header init
        strcpy(fRxHeader.fPacketType, "header");
        fRxHeader.fID = fParams.fID;
        fRxHeader.fCycle = 0;
        fRxHeader.fSubCycle = 0;
        fRxHeader.fIsLastPckt = 0;

        // network buffers
        fTxBuffer = new char[fParams.fMtu];
        fRxBuffer = new char[fParams.fMtu];
        assert(fTxBuffer);
        assert(fRxBuffer);

        // net audio/midi buffers'addresses
        fTxData = fTxBuffer + HEADER_SIZE;
        fRxData = fRxBuffer + HEADER_SIZE;

        return true;
    }

    int JackNetInterface::MidiSend(NetMidiBuffer* buffer, int midi_channnels, int audio_channels)
    {
        if (midi_channnels > 0) {
            // set global header fields and get the number of midi packets
            fTxHeader.fDataType = 'm';
            uint data_size = buffer->RenderFromJackPorts();
            fTxHeader.fNumPacket = buffer->GetNumPackets(data_size, PACKET_AVAILABLE_SIZE(&fParams));

            for (uint subproc = 0; subproc < fTxHeader.fNumPacket; subproc++) {
                fTxHeader.fSubCycle = subproc;
                fTxHeader.fIsLastPckt = ((subproc == (fTxHeader.fNumPacket - 1)) && audio_channels == 0) ? 1 : 0;
                fTxHeader.fPacketSize = HEADER_SIZE + buffer->RenderToNetwork(subproc, data_size);
                memcpy(fTxBuffer, &fTxHeader, HEADER_SIZE);
                if (Send(fTxHeader.fPacketSize, 0) == SOCKET_ERROR) {
                    return SOCKET_ERROR;
                }
            }
        }
        return 0;
    }

    int JackNetInterface::AudioSend(NetAudioBuffer* buffer, int audio_channels)
    {
        // audio
        if (audio_channels > 0) {
            fTxHeader.fDataType = 'a';
            fTxHeader.fActivePorts = buffer->RenderFromJackPorts();
            fTxHeader.fNumPacket = buffer->GetNumPackets(fTxHeader.fActivePorts);

            for (uint subproc = 0; subproc < fTxHeader.fNumPacket; subproc++) {
                fTxHeader.fSubCycle = subproc;
                fTxHeader.fIsLastPckt = (subproc == (fTxHeader.fNumPacket - 1)) ? 1 : 0;
                fTxHeader.fPacketSize = HEADER_SIZE + buffer->RenderToNetwork(subproc, fTxHeader.fActivePorts);
                memcpy(fTxBuffer, &fTxHeader, HEADER_SIZE);
                // PacketHeaderDisplay(&fTxHeader);
                if (Send(fTxHeader.fPacketSize, 0) == SOCKET_ERROR) {
                    return SOCKET_ERROR;
                }
            }
        }
        return 0;
    }

    int JackNetInterface::MidiRecv(packet_header_t* rx_head, NetMidiBuffer* buffer, uint& recvd_midi_pckt)
    {
        int rx_bytes = Recv(rx_head->fPacketSize, 0);
        fRxHeader.fCycle = rx_head->fCycle;
        fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
        buffer->RenderFromNetwork(rx_head->fSubCycle, rx_bytes - HEADER_SIZE);
        
        // Last midi packet is received, so finish rendering...
        if (++recvd_midi_pckt == rx_head->fNumPacket) {
            buffer->RenderToJackPorts();
        }
        return rx_bytes;
    }

    int JackNetInterface::AudioRecv(packet_header_t* rx_head, NetAudioBuffer* buffer)
    {
        int rx_bytes = Recv(rx_head->fPacketSize, 0);
        fRxHeader.fCycle = rx_head->fCycle;
        fRxHeader.fSubCycle = rx_head->fSubCycle;
        fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
        fRxHeader.fActivePorts = rx_head->fActivePorts;
        rx_bytes = buffer->RenderFromNetwork(rx_head->fCycle, rx_head->fSubCycle, fRxHeader.fActivePorts);
        
        // Last audio packet is received, so finish rendering...
        if (fRxHeader.fIsLastPckt) {
            buffer->RenderToJackPorts();
        }
        return rx_bytes;
    }

    int JackNetInterface::FinishRecv(NetAudioBuffer* buffer)
    {
        buffer->RenderToJackPorts();
        return NET_PACKET_ERROR;
    }

    NetAudioBuffer* JackNetInterface::AudioBufferFactory(int nports, char* buffer)
    {
         switch (fParams.fSampleEncoder) {

            case JackFloatEncoder:
                return new NetFloatAudioBuffer(&fParams, nports, buffer);

            case JackIntEncoder:
                return new NetIntAudioBuffer(&fParams, nports, buffer);

            #if HAVE_CELT
            case JackCeltEncoder:
                return new NetCeltAudioBuffer(&fParams, nports, buffer, fParams.fKBps);
            #endif
            #if HAVE_OPUS
            case JackOpusEncoder:
                return new NetOpusAudioBuffer(&fParams, nports, buffer, fParams.fKBps);
            #endif
        }
        return NULL;
    }
    
    void JackNetInterface::SetRcvTimeOut()
    {
        if (!fSetTimeOut) {
            if (fSocket.SetTimeOut(PACKET_TIMEOUT) == SOCKET_ERROR) {
                jack_error("Can't set rx timeout : %s", StrError(NET_ERROR_CODE));
                return;
            }
            fSetTimeOut = true;
        }
    }

    // JackNetMasterInterface ************************************************************************************

    bool JackNetMasterInterface::Init()
    {
        jack_log("JackNetMasterInterface::Init : ID %u", fParams.fID);

        session_params_t host_params;
        uint attempt = 0;
        int rx_bytes = 0;

        // socket
        if (fSocket.NewSocket() == SOCKET_ERROR) {
            jack_error("Can't create socket : %s", StrError(NET_ERROR_CODE));
            return false;
        }

        // timeout on receive (for init)
        if (fSocket.SetTimeOut(MASTER_INIT_TIMEOUT) < 0) {
            jack_error("Can't set init timeout : %s", StrError(NET_ERROR_CODE));
        }

        // connect
        if (fSocket.Connect() == SOCKET_ERROR) {
            jack_error("Can't connect : %s", StrError(NET_ERROR_CODE));
            return false;
        }

        // send 'SLAVE_SETUP' until 'START_MASTER' received
        jack_info("Sending parameters to %s...", fParams.fSlaveNetName);
        do
        {
            session_params_t net_params;
            memset(&net_params, 0, sizeof(session_params_t));
            SetPacketType(&fParams, SLAVE_SETUP);
            SessionParamsHToN(&fParams, &net_params);

            if (fSocket.Send(&net_params, sizeof(session_params_t), 0) == SOCKET_ERROR) {
                jack_error("Error in send : %s", StrError(NET_ERROR_CODE));
            }

            memset(&net_params, 0, sizeof(session_params_t));
            if (((rx_bytes = fSocket.Recv(&net_params, sizeof(session_params_t), 0)) == SOCKET_ERROR) && (fSocket.GetError() != NET_NO_DATA)) {
                jack_error("Problem with network");
                return false;
            }

            SessionParamsNToH(&net_params, &host_params);
        }
        while ((GetPacketType(&host_params) != START_MASTER) && (++attempt < SLAVE_SETUP_RETRY));
        
        if (attempt == SLAVE_SETUP_RETRY) {
            jack_error("Slave doesn't respond, exiting");
            return false;
        }

        return true;
    }

    bool JackNetMasterInterface::SetParams()
    {
        jack_log("JackNetMasterInterface::SetParams audio in = %d audio out = %d MIDI in = %d MIDI out = %d",
            fParams.fSendAudioChannels, fParams.fReturnAudioChannels,
            fParams.fSendMidiChannels, fParams.fReturnMidiChannels);

        JackNetInterface::SetParams();

        fTxHeader.fDataStream = 's';
        fRxHeader.fDataStream = 'r';

        fMaxCycleOffset = fParams.fNetworkLatency;

        // midi net buffers
        if (fParams.fSendMidiChannels > 0) {
            fNetMidiCaptureBuffer = new NetMidiBuffer(&fParams, fParams.fSendMidiChannels, fTxData);
        }

        if (fParams.fReturnMidiChannels > 0) {
            fNetMidiPlaybackBuffer = new NetMidiBuffer(&fParams, fParams.fReturnMidiChannels, fRxData);
        }

        try {

            // audio net buffers
            if (fParams.fSendAudioChannels > 0) {
                fNetAudioCaptureBuffer = AudioBufferFactory(fParams.fSendAudioChannels, fTxData);
                assert(fNetAudioCaptureBuffer);
            }

            if (fParams.fReturnAudioChannels > 0) {
                fNetAudioPlaybackBuffer = AudioBufferFactory(fParams.fReturnAudioChannels, fRxData);
                assert(fNetAudioPlaybackBuffer);
            }

        } catch (exception&) {
            jack_error("NetAudioBuffer allocation error...");
            return false;
        }

        // set the new rx buffer size
        if (SetNetBufferSize() == SOCKET_ERROR) {
            jack_error("Can't set net buffer sizes : %s", StrError(NET_ERROR_CODE));
            goto error;
        }

        return true;

    error:
        FreeNetworkBuffers();
        return false;
    }

    void JackNetMasterInterface::Exit()
    {
        jack_log("JackNetMasterInterface::Exit, ID %u", fParams.fID);

        // stop process
        fRunning = false;

        // send a 'multicast euthanasia request' - new socket is required on macosx
        jack_info("Exiting '%s'", fParams.fName);
        SetPacketType(&fParams, KILL_MASTER);
        JackNetSocket mcast_socket(fMulticastIP, fSocket.GetPort());

        session_params_t net_params;
        memset(&net_params, 0, sizeof(session_params_t));
        SessionParamsHToN(&fParams, &net_params);

        if (mcast_socket.NewSocket() == SOCKET_ERROR) {
            jack_error("Can't create socket : %s", StrError(NET_ERROR_CODE));
        }
        if (mcast_socket.SendTo(&net_params, sizeof(session_params_t), 0, fMulticastIP) == SOCKET_ERROR) {
            jack_error("Can't send suicide request : %s", StrError(NET_ERROR_CODE));
        }

        mcast_socket.Close();
    }

    void JackNetMasterInterface::FatalRecvError()
    {
        // fatal connection issue, exit
        jack_error("Recv connection lost error = %s, '%s' exiting", StrError(NET_ERROR_CODE), fParams.fName);
        // ask to the manager to properly remove the master
        Exit();
        // UGLY temporary way to be sure the thread does not call code possibly causing a deadlock in JackEngine.
        ThreadExit();
    }

     void JackNetMasterInterface::FatalSendError()
    {
        // fatal connection issue, exit
        jack_error("Send connection lost error = %s, '%s' exiting", StrError(NET_ERROR_CODE), fParams.fName);
        // ask to the manager to properly remove the master
        Exit();
        // UGLY temporary way to be sure the thread does not call code possibly causing a deadlock in JackEngine.
        ThreadExit();
    }

    int JackNetMasterInterface::Recv(size_t size, int flags)
    {
        int rx_bytes;

        if (((rx_bytes = fSocket.Recv(fRxBuffer, size, flags)) == SOCKET_ERROR) && fRunning) {
            FatalRecvError();
        }

        packet_header_t* header = reinterpret_cast<packet_header_t*>(fRxBuffer);
        PacketHeaderNToH(header, header);
        return rx_bytes;
    }

    int JackNetMasterInterface::Send(size_t size, int flags)
    {
        int tx_bytes;
        packet_header_t* header = reinterpret_cast<packet_header_t*>(fTxBuffer);
        PacketHeaderHToN(header, header);

        if (((tx_bytes = fSocket.Send(fTxBuffer, size, flags)) == SOCKET_ERROR) && fRunning) {
            FatalSendError();
        }
        return tx_bytes;
    }

    bool JackNetMasterInterface::IsSynched()
    {
        return (fCurrentCycleOffset <= fMaxCycleOffset);
    }

    int JackNetMasterInterface::SyncSend()
    {
        SetRcvTimeOut();
        
        fTxHeader.fCycle++;
        fTxHeader.fSubCycle = 0;
        fTxHeader.fDataType = 's';
        fTxHeader.fIsLastPckt = (fParams.fSendMidiChannels == 0 && fParams.fSendAudioChannels == 0) ? 1 : 0;
        fTxHeader.fPacketSize = fParams.fMtu;

        memcpy(fTxBuffer, &fTxHeader, HEADER_SIZE);
        // PacketHeaderDisplay(&fTxHeader);
        return Send(fTxHeader.fPacketSize, 0);
    }

    int JackNetMasterInterface::DataSend()
    {
        if (MidiSend(fNetMidiCaptureBuffer, fParams.fSendMidiChannels, fParams.fSendAudioChannels) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        return AudioSend(fNetAudioCaptureBuffer, fParams.fSendAudioChannels);
    }

    int JackNetMasterInterface::SyncRecv()
    {
        int rx_bytes = 0;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*>(fRxBuffer);

        // receive sync (launch the cycle)
        do {
            rx_bytes = Recv(fParams.fMtu, MSG_PEEK);
            // connection issue, send will detect it, so don't skip the cycle (return 0)
            if (rx_bytes == SOCKET_ERROR) {
                return SOCKET_ERROR;
            }
        }
        while ((strcmp(rx_head->fPacketType, "header") != 0) && (rx_head->fDataType != 's'));

        fCurrentCycleOffset = fTxHeader.fCycle - rx_head->fCycle;

        if (fCurrentCycleOffset < fMaxCycleOffset) {
            jack_info("Synching with latency = %d", fCurrentCycleOffset);
            return 0;
        } else {
            rx_bytes = Recv(rx_head->fPacketSize, 0);
            fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
            return rx_bytes;
        }
    }

    int JackNetMasterInterface::DataRecv()
    {
        int rx_bytes = 0;
        uint recvd_midi_pckt = 0;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*>(fRxBuffer);

        while (!fRxHeader.fIsLastPckt) {
            // how much data is queued on the rx buffer ?
            rx_bytes = Recv(fParams.fMtu, MSG_PEEK);
         
            // error here, problem with recv, just skip the cycle (return -1)
            if (rx_bytes == SOCKET_ERROR) {
                return rx_bytes;
            }

            if (rx_bytes && (rx_head->fDataStream == 'r') && (rx_head->fID == fParams.fID)) {
                // read data
                switch (rx_head->fDataType) {

                    case 'm':   // midi
                        rx_bytes = MidiRecv(rx_head, fNetMidiPlaybackBuffer, recvd_midi_pckt);
                        break;

                    case 'a':   // audio
                        rx_bytes = AudioRecv(rx_head, fNetAudioPlaybackBuffer);
                        break;

                    case 's':   // sync
                        jack_info("NetMaster : overloaded, skipping receive from '%s'", fParams.fName);
                        return FinishRecv(fNetAudioPlaybackBuffer);
                }
            }
        }
   
        return rx_bytes;
    }

    void JackNetMasterInterface::EncodeSyncPacket()
    {
        // This method contains every step of sync packet informations coding
        // first of all, clear sync packet
        memset(fTxData, 0, PACKET_AVAILABLE_SIZE(&fParams));

        // then, first step : transport
        if (fParams.fTransportSync) {
            EncodeTransportData();
            TransportDataHToN(&fSendTransportData, &fSendTransportData);
            // copy to TxBuffer
            memcpy(fTxData, &fSendTransportData, sizeof(net_transport_data_t));
        }
        // then others (freewheel etc.)
        // ...

        // Transport not used for now...

        // Write active ports list
        fTxHeader.fActivePorts = (fNetAudioPlaybackBuffer) ? fNetAudioPlaybackBuffer->ActivePortsToNetwork(fTxData) : 0;
    }

    void JackNetMasterInterface::DecodeSyncPacket()
    {
        // This method contains every step of sync packet informations decoding process
        // first : transport
        if (fParams.fTransportSync) {
            // copy received transport data to transport data structure
            memcpy(&fReturnTransportData, fRxData, sizeof(net_transport_data_t));
            TransportDataNToH(&fReturnTransportData,  &fReturnTransportData);
            DecodeTransportData();
        }
        // then others
        // ...

        // Transport not used for now...
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*>(fRxBuffer);

        // Read active ports list
        if (fNetAudioCaptureBuffer) {
            fNetAudioCaptureBuffer->ActivePortsFromNetwork(fRxData, rx_head->fActivePorts);
        }
    }

// JackNetSlaveInterface ************************************************************************************************

    uint JackNetSlaveInterface::fSlaveCounter = 0;

    void JackNetSlaveInterface::InitAPI()
    {
        // open Socket API with the first slave
        if (fSlaveCounter++ == 0) {
            if (SocketAPIInit() < 0) {
                jack_error("Can't init Socket API, exiting...");
                throw std::bad_alloc();
            }
        }
    }

    bool JackNetSlaveInterface::Init()
    {
        jack_log("JackNetSlaveInterface::Init()");

        // set the parameters to send
        strcpy(fParams.fPacketType, "params");
        fParams.fProtocolVersion = SLAVE_PROTOCOL;
        SetPacketType(&fParams, SLAVE_AVAILABLE);

        // init loop : get a master and start, do it until connection is ok
        net_status_t status;
        do {
            // first, get a master, do it until a valid connection is running
            do {
                status = SendAvailableToMaster();
                if (status == NET_SOCKET_ERROR) {
                    return false;
                }
            }
            while (status != NET_CONNECTED);

            // then tell the master we are ready
            jack_info("Initializing connection with %s...", fParams.fMasterNetName);
            status = SendStartToMaster();
            if (status == NET_ERROR) {
                return false;
            }
        }
        while (status != NET_ROLLING);

        return true;
    }

    // Separate the connection protocol into two separated step
    bool JackNetSlaveInterface::InitConnection(int time_out_sec)
    {
        jack_log("JackNetSlaveInterface::InitConnection()");
        uint try_count = (time_out_sec > 0) ? ((1000000 * time_out_sec) / SLAVE_INIT_TIMEOUT) : LONG_MAX;

        // set the parameters to send
        strcpy(fParams.fPacketType, "params");
        fParams.fProtocolVersion = SLAVE_PROTOCOL;
        SetPacketType(&fParams, SLAVE_AVAILABLE);

        net_status_t status;
        do {
            // get a master
            status = SendAvailableToMaster(try_count);
            if (status == NET_SOCKET_ERROR) {
                return false;
            }
        }
        while (status != NET_CONNECTED && --try_count > 0);

        return (try_count != 0);
    }

    bool JackNetSlaveInterface::InitRendering()
    {
        jack_log("JackNetSlaveInterface::InitRendering()");

        net_status_t status;
        do {
            // then tell the master we are ready
            jack_info("Initializing connection with %s...", fParams.fMasterNetName);
            status = SendStartToMaster();
            if (status == NET_ERROR) {
                return false;
            }
        }
        while (status != NET_ROLLING);

        return true;
    }

    net_status_t JackNetSlaveInterface::SendAvailableToMaster(long try_count)
    {
        jack_log("JackNetSlaveInterface::SendAvailableToMaster()");
        // utility
        session_params_t host_params;
        int rx_bytes = 0;

        // socket
        if (fSocket.NewSocket() == SOCKET_ERROR) {
            jack_error("Fatal error : network unreachable - %s", StrError(NET_ERROR_CODE));
            return NET_SOCKET_ERROR;
        }

        if (fSocket.IsLocal(fMulticastIP)) {
            jack_info("Local IP is used...");
        } else {
            // bind the socket
            if (fSocket.Bind() == SOCKET_ERROR) {
                jack_error("Can't bind the socket : %s", StrError(NET_ERROR_CODE));
                return NET_SOCKET_ERROR;
            }
        }

        // timeout on receive (for init)
        if (fSocket.SetTimeOut(SLAVE_INIT_TIMEOUT) == SOCKET_ERROR) {
            jack_error("Can't set init timeout : %s", StrError(NET_ERROR_CODE));
        }

        // disable local loop
        if (fSocket.SetLocalLoop() == SOCKET_ERROR) {
            jack_error("Can't disable multicast loop : %s", StrError(NET_ERROR_CODE));
        }

        // send 'AVAILABLE' until 'SLAVE_SETUP' received
        jack_info("Waiting for a master...");
        do {
            // send 'available'
            session_params_t net_params;
            memset(&net_params, 0, sizeof(session_params_t));
            SessionParamsHToN(&fParams, &net_params);
            if (fSocket.SendTo(&net_params, sizeof(session_params_t), 0, fMulticastIP) == SOCKET_ERROR) {
                jack_error("Error in data send : %s", StrError(NET_ERROR_CODE));
            }

            // filter incoming packets : don't exit while no error is detected
            memset(&net_params, 0, sizeof(session_params_t));
            rx_bytes = fSocket.CatchHost(&net_params, sizeof(session_params_t), 0);
            SessionParamsNToH(&net_params, &host_params);
            if ((rx_bytes == SOCKET_ERROR) && (fSocket.GetError() != NET_NO_DATA)) {
                jack_error("Can't receive : %s", StrError(NET_ERROR_CODE));
                return NET_RECV_ERROR;
            }
        }
        while (strcmp(host_params.fPacketType, fParams.fPacketType)  && (GetPacketType(&host_params) != SLAVE_SETUP)  && (--try_count > 0));

        // Time out failure..
        if (try_count == 0) {
            jack_error("Time out error in connect");
            return NET_CONNECT_ERROR;
        }

        // everything is OK, copy parameters
        fParams = host_params;

        // connect the socket
        if (fSocket.Connect() == SOCKET_ERROR) {
            jack_error("Error in connect : %s", StrError(NET_ERROR_CODE));
            return NET_CONNECT_ERROR;
        }
        return NET_CONNECTED;
    }

    net_status_t JackNetSlaveInterface::SendStartToMaster()
    {
        jack_log("JackNetSlaveInterface::SendStartToMaster");

        // tell the master to start
        session_params_t net_params;
        memset(&net_params, 0, sizeof(session_params_t));
        SetPacketType(&fParams, START_MASTER);
        SessionParamsHToN(&fParams, &net_params);
        if (fSocket.Send(&net_params, sizeof(session_params_t), 0) == SOCKET_ERROR) {
            jack_error("Error in send : %s", StrError(NET_ERROR_CODE));
            return (fSocket.GetError() == NET_CONN_ERROR) ? NET_ERROR : NET_SEND_ERROR;
        }
        return NET_ROLLING;
    }

    bool JackNetSlaveInterface::SetParams()
    {
        jack_log("JackNetSlaveInterface::SetParams audio in = %d audio out = %d MIDI in = %d MIDI out = %d",
                fParams.fSendAudioChannels, fParams.fReturnAudioChannels,
                fParams.fSendMidiChannels, fParams.fReturnMidiChannels);

        JackNetInterface::SetParams();

        fTxHeader.fDataStream = 'r';
        fRxHeader.fDataStream = 's';

        // midi net buffers
        if (fParams.fSendMidiChannels > 0) {
            fNetMidiCaptureBuffer = new NetMidiBuffer(&fParams, fParams.fSendMidiChannels, fRxData);
        }

        if (fParams.fReturnMidiChannels > 0) {
            fNetMidiPlaybackBuffer = new NetMidiBuffer(&fParams, fParams.fReturnMidiChannels, fTxData);
        }

        try {

            // audio net buffers
            if (fParams.fSendAudioChannels > 0) {
                fNetAudioCaptureBuffer = AudioBufferFactory(fParams.fSendAudioChannels, fRxData);
                assert(fNetAudioCaptureBuffer);
            }

            if (fParams.fReturnAudioChannels > 0) {
                fNetAudioPlaybackBuffer = AudioBufferFactory(fParams.fReturnAudioChannels, fTxData);
                assert(fNetAudioPlaybackBuffer);
            }

        } catch (exception&) {
            jack_error("NetAudioBuffer allocation error...");
            return false;
        }

        // set the new buffer sizes
        if (SetNetBufferSize() == SOCKET_ERROR) {
            jack_error("Can't set net buffer sizes : %s", StrError(NET_ERROR_CODE));
            goto error;
        }

        return true;

    error:
        FreeNetworkBuffers();
        return false;
    }

    void JackNetSlaveInterface::FatalRecvError()
    {
        jack_error("Recv connection lost error = %s", StrError(NET_ERROR_CODE));
        throw JackNetException();
    }

    void JackNetSlaveInterface::FatalSendError()
    {
        jack_error("Send connection lost error = %s", StrError(NET_ERROR_CODE));
        throw JackNetException();
    }

    int JackNetSlaveInterface::Recv(size_t size, int flags)
    {
        int rx_bytes = fSocket.Recv(fRxBuffer, size, flags);
        
        // handle errors
        if (rx_bytes == SOCKET_ERROR) {
            FatalRecvError();
        }

        packet_header_t* header = reinterpret_cast<packet_header_t*>(fRxBuffer);
        PacketHeaderNToH(header, header);
        return rx_bytes;
    }

    int JackNetSlaveInterface::Send(size_t size, int flags)
    {
        packet_header_t* header = reinterpret_cast<packet_header_t*>(fTxBuffer);
        PacketHeaderHToN(header, header);
        int tx_bytes = fSocket.Send(fTxBuffer, size, flags);

        // handle errors
        if (tx_bytes == SOCKET_ERROR) {
            FatalSendError();
        }
        
        return tx_bytes;
    }

    int JackNetSlaveInterface::SyncRecv()
    {
        int rx_bytes = 0;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*>(fRxBuffer);
     
        // receive sync (launch the cycle)
        do {
            rx_bytes = Recv(fParams.fMtu, 0);
            // connection issue, send will detect it, so don't skip the cycle (return 0)
            if (rx_bytes == SOCKET_ERROR) {
                return rx_bytes;
            }
        }
        while ((strcmp(rx_head->fPacketType, "header") != 0) && (rx_head->fDataType != 's'));

        fRxHeader.fIsLastPckt = rx_head->fIsLastPckt;
        
        SetRcvTimeOut();
        return rx_bytes;
    }

    int JackNetSlaveInterface::DataRecv()
    {
        int rx_bytes = 0;
        uint recvd_midi_pckt = 0;
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*>(fRxBuffer);

        while (!fRxHeader.fIsLastPckt) {
            // how much data is queued on the rx buffer ?
            rx_bytes = Recv(fParams.fMtu, MSG_PEEK);

            // error here, problem with recv, just skip the cycle (return -1)
            if (rx_bytes == SOCKET_ERROR) {
                return rx_bytes;
            }

            if (rx_bytes && (rx_head->fDataStream == 's') && (rx_head->fID == fParams.fID)) {
                // read data
                switch (rx_head->fDataType) {

                    case 'm':   // midi
                        rx_bytes = MidiRecv(rx_head, fNetMidiCaptureBuffer, recvd_midi_pckt);
                        break;

                    case 'a':   // audio
                        rx_bytes = AudioRecv(rx_head, fNetAudioCaptureBuffer);
                        break;

                    case 's':   // sync
                        jack_info("NetSlave : overloaded, skipping receive");
                        return FinishRecv(fNetAudioCaptureBuffer);
                }
            }
        }

        fRxHeader.fCycle = rx_head->fCycle;
        return rx_bytes;
    }

    int JackNetSlaveInterface::SyncSend()
    {
        // tx header
        if (fParams.fSlaveSyncMode) {
            fTxHeader.fCycle = fRxHeader.fCycle;
        } else {
            fTxHeader.fCycle++;
        }
        fTxHeader.fSubCycle = 0;
        fTxHeader.fDataType = 's';
        fTxHeader.fIsLastPckt = (fParams.fReturnMidiChannels == 0 && fParams.fReturnAudioChannels == 0) ? 1 : 0;
        fTxHeader.fPacketSize = fParams.fMtu;

        memcpy(fTxBuffer, &fTxHeader, HEADER_SIZE);
        // PacketHeaderDisplay(&fTxHeader);
        return Send(fTxHeader.fPacketSize, 0);
    }

    int JackNetSlaveInterface::DataSend()
    {
        if (MidiSend(fNetMidiPlaybackBuffer, fParams.fReturnMidiChannels, fParams.fReturnAudioChannels) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        return AudioSend(fNetAudioPlaybackBuffer, fParams.fReturnAudioChannels);
    }

    // network sync------------------------------------------------------------------------
    void JackNetSlaveInterface::EncodeSyncPacket()
    {
        // This method contains every step of sync packet informations coding
        // first of all, clear sync packet
        memset(fTxData, 0, PACKET_AVAILABLE_SIZE(&fParams));

        // then first step : transport
        if (fParams.fTransportSync) {
            EncodeTransportData();
            TransportDataHToN(&fReturnTransportData, &fReturnTransportData);
            // copy to TxBuffer
            memcpy(fTxData, &fReturnTransportData, sizeof(net_transport_data_t));
        }
        // then others
        // ...

        // Transport is not used for now...

        // Write active ports list
        fTxHeader.fActivePorts = (fNetAudioCaptureBuffer) ? fNetAudioCaptureBuffer->ActivePortsToNetwork(fTxData) : 0;
    }

    void JackNetSlaveInterface::DecodeSyncPacket()
    {
        // This method contains every step of sync packet informations decoding process
        // first : transport
        if (fParams.fTransportSync) {
            // copy received transport data to transport data structure
            memcpy(&fSendTransportData, fRxData, sizeof(net_transport_data_t));
            TransportDataNToH(&fSendTransportData, &fSendTransportData);
            DecodeTransportData();
        }
        // then others
        // ...

        // Transport not used for now...
        packet_header_t* rx_head = reinterpret_cast<packet_header_t*>(fRxBuffer);

        // Read active ports list
        if (fNetAudioPlaybackBuffer) {
            fNetAudioPlaybackBuffer->ActivePortsFromNetwork(fRxData, rx_head->fActivePorts);
        }
    }

}
