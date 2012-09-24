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

#include "JackNetTool.h"
#include "JackError.h"

#ifdef __APPLE__

#include <mach/mach_time.h>

class HardwareClock
{
    public:

        HardwareClock();

        void Reset();
        void Update();

        float GetDeltaTime() const;
        double GetTime() const;

    private:

        double m_clockToSeconds;

        uint64_t m_startAbsTime;
        uint64_t m_lastAbsTime;

        double m_time;
        float m_deltaTime;
};

HardwareClock::HardwareClock()
{
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	m_clockToSeconds = (double)info.numer/info.denom/1000000000.0;
	Reset();
}

void HardwareClock::Reset()
{
	m_startAbsTime = mach_absolute_time();
	m_lastAbsTime = m_startAbsTime;
	m_time = m_startAbsTime*m_clockToSeconds;
	m_deltaTime = 1.0f/60.0f;
}

void HardwareClock::Update()
{
	const uint64_t currentTime = mach_absolute_time();
	const uint64_t dt = currentTime - m_lastAbsTime;

	m_time = currentTime*m_clockToSeconds;
	m_deltaTime = (double)dt*m_clockToSeconds;
	m_lastAbsTime = currentTime;
}

float HardwareClock::GetDeltaTime() const
{
	return m_deltaTime;
}

double HardwareClock::GetTime() const
{
	return m_time;
}

#endif

using namespace std;

namespace Jack
{
// NetMidiBuffer**********************************************************************************

    NetMidiBuffer::NetMidiBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
    {
        fNPorts = nports;
        fMaxBufsize = fNPorts * sizeof(sample_t) * params->fPeriodSize ;
        fMaxPcktSize = params->fMtu - sizeof(packet_header_t);
        fBuffer = new char[fMaxBufsize];
        fPortBuffer = new JackMidiBuffer* [fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            fPortBuffer[port_index] = NULL;
        }
        fNetBuffer = net_buffer;

        fCycleBytesSize = params->fMtu
                * (max(params->fSendMidiChannels, params->fReturnMidiChannels)
                * params->fPeriodSize * sizeof(sample_t) / (params->fMtu - sizeof(packet_header_t)));
    }

    NetMidiBuffer::~NetMidiBuffer()
    {
        delete[] fBuffer;
        delete[] fPortBuffer;
    }

    size_t NetMidiBuffer::GetCycleSize()
    {
        return fCycleBytesSize;
    }

    int NetMidiBuffer::GetNumPackets(int data_size, int max_size)
    {
        int res1 = data_size % max_size;
        int res2 = data_size / max_size;
        return (res1) ? res2 + 1 : res2;
    }

    void NetMidiBuffer::SetBuffer(int index, JackMidiBuffer* buffer)
    {
        fPortBuffer[index] = buffer;
    }

    JackMidiBuffer* NetMidiBuffer::GetBuffer(int index)
    {
        return fPortBuffer[index];
    }

    void NetMidiBuffer::DisplayEvents()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            for (uint event = 0; event < fPortBuffer[port_index]->event_count; event++) {
                if (fPortBuffer[port_index]->IsValid()) {
                    jack_info("port %d : midi event %u/%u -> time : %u, size : %u",
                                port_index + 1, event + 1, fPortBuffer[port_index]->event_count,
                                fPortBuffer[port_index]->events[event].time, fPortBuffer[port_index]->events[event].size);
                }
            }
        }
    }

    int NetMidiBuffer::RenderFromJackPorts()
    {
        int pos = 0;
        size_t copy_size;

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            char* write_pos = fBuffer + pos;
            copy_size = sizeof(JackMidiBuffer) + fPortBuffer[port_index]->event_count * sizeof(JackMidiEvent);
            memcpy(fBuffer + pos, fPortBuffer[port_index], copy_size);
            pos += copy_size;
            memcpy(fBuffer + pos,
                    fPortBuffer[port_index] + (fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos),
                    fPortBuffer[port_index]->write_pos);
            pos += fPortBuffer[port_index]->write_pos;

            JackMidiBuffer* midi_buffer = reinterpret_cast<JackMidiBuffer*>(write_pos);
            MidiBufferHToN(midi_buffer, midi_buffer);
        }
        return pos;
    }

    void NetMidiBuffer::RenderToJackPorts()
    {
        int pos = 0;
        size_t copy_size;

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            JackMidiBuffer* midi_buffer = reinterpret_cast<JackMidiBuffer*>(fBuffer + pos);
            MidiBufferNToH(midi_buffer, midi_buffer);
            copy_size = sizeof(JackMidiBuffer) + reinterpret_cast<JackMidiBuffer*>(fBuffer + pos)->event_count * sizeof(JackMidiEvent);
            memcpy(fPortBuffer[port_index], fBuffer + pos, copy_size);
            pos += copy_size;
            memcpy(fPortBuffer[port_index] + (fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos),
                    fBuffer + pos,
                    fPortBuffer[port_index]->write_pos);
            pos += fPortBuffer[port_index]->write_pos;
        }
    }

    void NetMidiBuffer::RenderFromNetwork(int sub_cycle, size_t copy_size)
    {
        memcpy(fBuffer + sub_cycle * fMaxPcktSize, fNetBuffer, copy_size);
    }

    int NetMidiBuffer::RenderToNetwork(int sub_cycle, size_t total_size)
    {
        int size = total_size - sub_cycle * fMaxPcktSize;
        int copy_size = (size <= fMaxPcktSize) ? size : fMaxPcktSize;
        memcpy(fNetBuffer, fBuffer + sub_cycle * fMaxPcktSize, copy_size);
        return copy_size;
    }

// net audio buffer *********************************************************************************

    NetAudioBuffer::NetAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
    {
        fNPorts = nports;
        fNetBuffer = net_buffer;

        fPortBuffer = new sample_t* [fNPorts];
        fConnectedPorts = new bool[fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            fPortBuffer[port_index] = NULL;
            fConnectedPorts[port_index] = true;
        }
    }

    NetAudioBuffer::~NetAudioBuffer()
    {
        delete [] fConnectedPorts;
        delete [] fPortBuffer;
    }

    void NetAudioBuffer::SetBuffer(int index, sample_t* buffer)
    {
        fPortBuffer[index] = buffer;
    }

    sample_t* NetAudioBuffer::GetBuffer(int index)
    {
        return fPortBuffer[index];
    }

    int NetAudioBuffer::CheckPacket(int cycle, int sub_cycle)
    {
        int res;

        if (sub_cycle != fLastSubCycle + 1) {
            jack_error("Packet(s) missing from... %d %d", fLastSubCycle, sub_cycle);
            res = NET_PACKET_ERROR;
        } else {
            res = 0;
        }

        fLastSubCycle = sub_cycle;
        return res;
    }

    void NetAudioBuffer::NextCycle()
    {
        // reset for next cycle
        fLastSubCycle = -1;
    }

    void NetAudioBuffer::Cleanup()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                memset(fPortBuffer[port_index], 0, fPeriodSize * sizeof(sample_t));
            }
        }
    }

    //network<->buffer

    int NetAudioBuffer::ActivePortsToNetwork(char* net_buffer)
    {
        int active_ports = 0;
        int* active_port_address = (int*)net_buffer;

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            // Write the active port number
            if (fPortBuffer[port_index]) {
                *active_port_address = htonl(port_index);
                active_port_address++;
                active_ports++;
                assert(active_ports < 256); 
            }
        }

        return active_ports;
    }

    void NetAudioBuffer::ActivePortsFromNetwork(char* net_buffer, uint32_t port_num)
    {
        int* active_port_address = (int*)net_buffer;

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            fConnectedPorts[port_index] = false;
        }

        for (uint port_index = 0; port_index < port_num; port_index++) {
            // Use -1 when port is actually connected on other side
            int active_port = ntohl(*active_port_address);
            if (active_port >= 0 && active_port < fNPorts) {
                fConnectedPorts[active_port] = true;
            } else {
                jack_error("ActivePortsFromNetwork: incorrect port = %d", active_port);
            }
            active_port_address++;
        }
    }

    int NetAudioBuffer::RenderFromJackPorts()
    {
        // Count active ports
        int active_ports = 0;
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                active_ports++;
            }
        }
        //jack_info("active_ports %d", active_ports);
        return active_ports;
    }

    void NetAudioBuffer::RenderToJackPorts()
    {
        // Nothing to do
        NextCycle();
    }

    // Float converter

    NetFloatAudioBuffer::NetFloatAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
        : NetAudioBuffer(params, nports, net_buffer)
    {
        fPeriodSize = params->fPeriodSize;
        fPacketSize = PACKET_AVAILABLE_SIZE(params);

        UpdateParams(max(params->fReturnAudioChannels, params->fSendAudioChannels));

        fSubPeriodBytesSize = fSubPeriodSize * sizeof(sample_t);

        fCycleDuration = float(fSubPeriodSize) / float(params->fSampleRate);
        fCycleBytesSize = params->fMtu * (fPeriodSize / fSubPeriodSize);

        fLastSubCycle = -1;
    }

    NetFloatAudioBuffer::~NetFloatAudioBuffer()
    {}

    // needed size in bytes for an entire cycle
    size_t NetFloatAudioBuffer::GetCycleSize()
    {
        return fCycleBytesSize;
    }

    // cycle duration in sec
    float NetFloatAudioBuffer::GetCycleDuration()
    {
        return fCycleDuration;
    }

    void NetFloatAudioBuffer::UpdateParams(int active_ports)
    {
        if (active_ports == 0) {
            fSubPeriodSize = fPeriodSize;
        } else {
            jack_nframes_t period = (int) powf(2.f, (int)(log(float(fPacketSize) / (active_ports * sizeof(sample_t))) / log(2.)));
            fSubPeriodSize = (period > fPeriodSize) ? fPeriodSize : period;
        }

        fSubPeriodBytesSize = fSubPeriodSize * sizeof(sample_t) + sizeof(int); // The port number in coded on 4 bytes
    }

    int NetFloatAudioBuffer::GetNumPackets(int active_ports)
    {
        UpdateParams(active_ports);

        /*
        jack_log("GetNumPackets packet = %d  fPeriodSize = %d fSubPeriodSize = %d fSubPeriodBytesSize = %d",
            fPeriodSize / fSubPeriodSize, fPeriodSize, fSubPeriodSize, fSubPeriodBytesSize);
        */
        return fPeriodSize / fSubPeriodSize; // At least one packet
    }

    //jack<->buffer

    int NetFloatAudioBuffer::RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num)
    {
        // Cleanup all JACK ports at the beginning of the cycle
        if (sub_cycle == 0) {
            Cleanup();
        }

        if (port_num > 0)  {
            UpdateParams(port_num);
            for (uint32_t port_index = 0; port_index < port_num; port_index++) {
                // Only copy to active ports : read the active port number then audio data
                int* active_port_address = (int*)(fNetBuffer + port_index * fSubPeriodBytesSize);
                int active_port = ntohl(*active_port_address);
                RenderFromNetwork((char*)(active_port_address + 1), active_port, sub_cycle);
            }
        }

        return CheckPacket(cycle, sub_cycle);
    }


    int NetFloatAudioBuffer::RenderToNetwork(int sub_cycle, uint32_t port_num)
    {
        int active_ports = 0;

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            // Only copy from active ports : write the active port number then audio data
            if (fPortBuffer[port_index]) {
                int* active_port_address = (int*)(fNetBuffer + active_ports * fSubPeriodBytesSize);
                *active_port_address = htonl(port_index);
                RenderToNetwork((char*)(active_port_address + 1), port_index, sub_cycle);
                active_ports++;
            }
        }

        return port_num * fSubPeriodBytesSize;
    }

#ifdef __BIG_ENDIAN__

    static inline jack_default_audio_sample_t SwapFloat(jack_default_audio_sample_t f)
    {
          union
          {
            jack_default_audio_sample_t f;
            unsigned char b[4];
          } dat1, dat2;

          dat1.f = f;
          dat2.b[0] = dat1.b[3];
          dat2.b[1] = dat1.b[2];
          dat2.b[2] = dat1.b[1];
          dat2.b[3] = dat1.b[0];
          return dat2.f;
    }

    void NetFloatAudioBuffer::RenderFromNetwork(char* net_buffer, int active_port, int sub_cycle)
    {
        if (fPortBuffer[active_port]) {
            jack_default_audio_sample_t* src = (jack_default_audio_sample_t*)(net_buffer);
            jack_default_audio_sample_t* dst = (jack_default_audio_sample_t*)(fPortBuffer[active_port] + sub_cycle * fSubPeriodSize);
            for (unsigned int sample = 0; sample < (fSubPeriodBytesSize -  sizeof(int)) / sizeof(jack_default_audio_sample_t); sample++) {
                dst[sample] = SwapFloat(src[sample]);
            }
        }
    }

    void NetFloatAudioBuffer::RenderToNetwork(char* net_buffer, int active_port, int sub_cycle)
    {
        for (int port_index = 0; port_index < fNPorts; port_index++ ) {
            jack_default_audio_sample_t* src = (jack_default_audio_sample_t*)(fPortBuffer[active_port] + sub_cycle * fSubPeriodSize);
            jack_default_audio_sample_t* dst = (jack_default_audio_sample_t*)(net_buffer);
            for (unsigned int sample = 0; sample < (fSubPeriodBytesSize - sizeof(int)) / sizeof(jack_default_audio_sample_t); sample++) {
                dst[sample] = SwapFloat(src[sample]);
            }
        }
    }

#else

    void NetFloatAudioBuffer::RenderFromNetwork(char* net_buffer, int active_port, int sub_cycle)
    {
        if (fPortBuffer[active_port]) {
            memcpy(fPortBuffer[active_port] + sub_cycle * fSubPeriodSize, net_buffer, fSubPeriodBytesSize - sizeof(int));
        }
    }

    void NetFloatAudioBuffer::RenderToNetwork(char* net_buffer, int active_port, int sub_cycle)
    {
        memcpy(net_buffer, fPortBuffer[active_port] + sub_cycle * fSubPeriodSize, fSubPeriodBytesSize - sizeof(int));
    }

#endif
    // Celt audio buffer *********************************************************************************

#if HAVE_CELT

    #define KPS 32
    #define KPS_DIV 8

    NetCeltAudioBuffer::NetCeltAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps)
        :NetAudioBuffer(params, nports, net_buffer)
    {
        fCeltMode = new CELTMode *[fNPorts];
        fCeltEncoder = new CELTEncoder *[fNPorts];
        fCeltDecoder = new CELTDecoder *[fNPorts];

        memset(fCeltMode, 0, fNPorts * sizeof(CELTMode*));
        memset(fCeltEncoder, 0, fNPorts * sizeof(CELTEncoder*));
        memset(fCeltDecoder, 0, fNPorts * sizeof(CELTDecoder*));

        int error = CELT_OK;

        for (int i = 0; i < fNPorts; i++)  {
            fCeltMode[i] = celt_mode_create(params->fSampleRate, params->fPeriodSize, &error);
            if (error != CELT_OK) {
                goto error;
            }

    #if HAVE_CELT_API_0_11

            fCeltEncoder[i] = celt_encoder_create_custom(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                goto error;
            }
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create_custom(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                goto error;
            }
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8

            fCeltEncoder[i] = celt_encoder_create(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                goto error;
            }
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                goto error;
            }
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #else

            fCeltEncoder[i] = celt_encoder_create(fCeltMode[i]);
            if (error != CELT_OK) {
                goto error;
            }
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create(fCeltMode[i]);
            if (error != CELT_OK) {
                goto error;
            }
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #endif
        }

        {
            fPeriodSize = params->fPeriodSize;

            fCompressedSizeByte = (kbps * params->fPeriodSize * 1024) / (params->fSampleRate * 8);
            jack_log("NetCeltAudioBuffer fCompressedSizeByte %d", fCompressedSizeByte);

            fCompressedBuffer = new unsigned char* [fNPorts];
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                fCompressedBuffer[port_index] = new unsigned char[fCompressedSizeByte];
                memset(fCompressedBuffer[port_index], 0, fCompressedSizeByte * sizeof(char));
            }

            int res1 = (fNPorts * fCompressedSizeByte) % PACKET_AVAILABLE_SIZE(params);
            int res2 = (fNPorts * fCompressedSizeByte) / PACKET_AVAILABLE_SIZE(params);

            fNumPackets = (res1) ? (res2 + 1) : res2;

            jack_log("NetCeltAudioBuffer res1 = %d res2 = %d", res1, res2);

            fSubPeriodBytesSize = fCompressedSizeByte / fNumPackets;
            fLastSubPeriodBytesSize = fSubPeriodBytesSize + fCompressedSizeByte % fNumPackets;

            jack_log("NetCeltAudioBuffer fNumPackets = %d fSubPeriodBytesSize = %d, fLastSubPeriodBytesSize = %d", fNumPackets, fSubPeriodBytesSize, fLastSubPeriodBytesSize);

            fCycleDuration = float(fSubPeriodBytesSize / sizeof(sample_t)) / float(params->fSampleRate);
            fCycleBytesSize = params->fMtu * fNumPackets;

            fLastSubCycle = -1;
            return;
        }

    error:

        FreeCelt();
        throw std::bad_alloc();
    }

    NetCeltAudioBuffer::~NetCeltAudioBuffer()
    {
        FreeCelt();

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            delete [] fCompressedBuffer[port_index];
        }

        delete [] fCompressedBuffer;
    }

    void NetCeltAudioBuffer::FreeCelt()
    {
        for (int i = 0; i < fNPorts; i++)  {
            if (fCeltEncoder[i]) {
                celt_encoder_destroy(fCeltEncoder[i]);
            }
            if (fCeltDecoder[i]) {
                celt_decoder_destroy(fCeltDecoder[i]);
            }
            if (fCeltMode[i]) {
                celt_mode_destroy(fCeltMode[i]);
            }
        }

        delete [] fCeltMode;
        delete [] fCeltEncoder;
        delete [] fCeltDecoder;
    }

    size_t NetCeltAudioBuffer::GetCycleSize()
    {
        return fCycleBytesSize;
    }

    float NetCeltAudioBuffer::GetCycleDuration()
    {
        return fCycleDuration;
    }

    int NetCeltAudioBuffer::GetNumPackets(int active_ports)
    {
        return fNumPackets;
    }

    int NetCeltAudioBuffer::RenderFromJackPorts()
    {
        float buffer[BUFFER_SIZE_MAX];

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                memcpy(buffer, fPortBuffer[port_index], fPeriodSize * sizeof(sample_t));
            } else {
                memset(buffer, 0, fPeriodSize * sizeof(sample_t));
            }
        #if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
            int res = celt_encode_float(fCeltEncoder[port_index], buffer, fPeriodSize, fCompressedBuffer[port_index], fCompressedSizeByte);
        #else
            int res = celt_encode_float(fCeltEncoder[port_index], buffer, NULL, fCompressedBuffer[port_index], fCompressedSizeByte);
        #endif
            if (res != fCompressedSizeByte) {
                jack_error("celt_encode_float error fCompressedSizeByte = %d res = %d", fCompressedSizeByte, res);
            }
        }

        // All ports active
        return fNPorts;
    }

    void NetCeltAudioBuffer::RenderToJackPorts()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
            #if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
                int res = celt_decode_float(fCeltDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizeByte, fPortBuffer[port_index], fPeriodSize);
            #else
                int res = celt_decode_float(fCeltDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizeByte, fPortBuffer[port_index]);
            #endif
                if (res != CELT_OK) {
                    jack_error("celt_decode_float error fCompressedSizeByte = %d res = %d", fCompressedSizeByte, res);
                }
            }
        }

        NextCycle();
    }

    //network<->buffer
    int NetCeltAudioBuffer::RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num)
    {
        // Cleanup all JACK ports at the beginning of the cycle
        if (sub_cycle == 0) {
            Cleanup();
        }

        if (port_num > 0)  {
            // Last packet of the cycle
            if (sub_cycle == fNumPackets - 1) {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    memcpy(fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, fNetBuffer + port_index * fLastSubPeriodBytesSize, fLastSubPeriodBytesSize);
                }
            } else {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    memcpy(fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, fNetBuffer + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize);
                }
            }
        }

        return CheckPacket(cycle, sub_cycle);
    }

    int NetCeltAudioBuffer::RenderToNetwork(int sub_cycle, uint32_t port_num)
    {
        // Last packet of the cycle
        if (sub_cycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fNetBuffer + port_index * fLastSubPeriodBytesSize, fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, fLastSubPeriodBytesSize);
            }
            return fNPorts * fLastSubPeriodBytesSize;
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize, fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, fSubPeriodBytesSize);
            }
            return fNPorts * fSubPeriodBytesSize;
        }
    }

#endif

#if HAVE_OPUS
#define CDO (sizeof(short)) ///< compressed data offset (first 2 bytes are length)

    NetOpusAudioBuffer::NetOpusAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps)
        :NetAudioBuffer(params, nports, net_buffer)
    {
        fOpusMode = new OpusCustomMode *[fNPorts];
        fOpusEncoder = new OpusCustomEncoder *[fNPorts];
        fOpusDecoder = new OpusCustomDecoder *[fNPorts];
        fCompressedSizesByte = new unsigned short [fNPorts];

        memset(fOpusMode, 0, fNPorts * sizeof(OpusCustomMode*));
        memset(fOpusEncoder, 0, fNPorts * sizeof(OpusCustomEncoder*));
        memset(fOpusDecoder, 0, fNPorts * sizeof(OpusCustomDecoder*));
        memset(fCompressedSizesByte, 0, fNPorts * sizeof(int));

        int error = OPUS_OK;

        for (int i = 0; i < fNPorts; i++)  {
            /* Allocate en/decoders */
            fOpusMode[i] = opus_custom_mode_create(
            params->fSampleRate, params->fPeriodSize, &error);
            if (error != OPUS_OK) {
                goto error;
            }

            fOpusEncoder[i] = opus_custom_encoder_create(fOpusMode[i], 1,&error);
            if (error != OPUS_OK) {
                goto error;
            }

            fOpusDecoder[i] = opus_custom_decoder_create(fOpusMode[i], 1, &error);
            if (error != OPUS_OK) {
                goto error;
            }

            opus_custom_encoder_ctl(fOpusEncoder[i], OPUS_SET_BITRATE(kbps*1024)); // bits per second
            opus_custom_encoder_ctl(fOpusEncoder[i], OPUS_SET_COMPLEXITY(10));
            opus_custom_encoder_ctl(fOpusEncoder[i], OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC));
            opus_custom_encoder_ctl(fOpusEncoder[i], OPUS_SET_SIGNAL(OPUS_APPLICATION_RESTRICTED_LOWDELAY));
        }

        {
            fCompressedMaxSizeByte = (kbps * params->fPeriodSize * 1024) / (params->fSampleRate * 8);
            fPeriodSize = params->fPeriodSize;
            jack_log("NetOpusAudioBuffer fCompressedMaxSizeByte %d", fCompressedMaxSizeByte);

            fCompressedBuffer = new unsigned char* [fNPorts];
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                fCompressedBuffer[port_index] = new unsigned char[fCompressedMaxSizeByte];
                memset(fCompressedBuffer[port_index], 0, fCompressedMaxSizeByte * sizeof(char));
            }

            int res1 = (fNPorts * fCompressedMaxSizeByte + CDO) % PACKET_AVAILABLE_SIZE(params);
            int res2 = (fNPorts * fCompressedMaxSizeByte + CDO) / PACKET_AVAILABLE_SIZE(params);

            fNumPackets = (res1) ? (res2 + 1) : res2;

            jack_log("NetOpusAudioBuffer res1 = %d res2 = %d", res1, res2);

            fSubPeriodBytesSize = (fCompressedMaxSizeByte + CDO) / fNumPackets;
            fLastSubPeriodBytesSize = fSubPeriodBytesSize + (fCompressedMaxSizeByte + CDO) % fNumPackets;

						if (fNumPackets == 1) {
							fSubPeriodBytesSize = fLastSubPeriodBytesSize;
						}

            jack_log("NetOpusAudioBuffer fNumPackets = %d fSubPeriodBytesSize = %d, fLastSubPeriodBytesSize = %d", fNumPackets, fSubPeriodBytesSize, fLastSubPeriodBytesSize);

            fCycleDuration = float(fSubPeriodBytesSize / sizeof(sample_t)) / float(params->fSampleRate);
            fCycleBytesSize = params->fMtu * fNumPackets;

            fLastSubCycle = -1;
            return;
        }

    error:

        FreeOpus();
        throw std::bad_alloc();
    }

    NetOpusAudioBuffer::~NetOpusAudioBuffer()
    {
        FreeOpus();

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            delete [] fCompressedBuffer[port_index];
        }

        delete [] fCompressedBuffer;
        delete [] fCompressedSizesByte;
    }

    void NetOpusAudioBuffer::FreeOpus()
    {
        for (int i = 0; i < fNPorts; i++)  {
            if (fOpusEncoder[i]) {
                opus_custom_encoder_destroy(fOpusEncoder[i]);
                fOpusEncoder[i]=0;
            }
            if (fOpusDecoder[i]) {
                opus_custom_decoder_destroy(fOpusDecoder[i]);
                fOpusDecoder[i]=0;
            }
            if (fOpusMode[i]) {
                opus_custom_mode_destroy(fOpusMode[i]);
                fOpusMode[i]=0;
            }
        }

        delete [] fOpusEncoder;
        delete [] fOpusDecoder;
        delete [] fOpusMode;
    }

    size_t NetOpusAudioBuffer::GetCycleSize()
    {
        return fCycleBytesSize;
    }

    float NetOpusAudioBuffer::GetCycleDuration()
    {
        return fCycleDuration;
    }

    int NetOpusAudioBuffer::GetNumPackets(int active_ports)
    {
        return fNumPackets;
    }

    int NetOpusAudioBuffer::RenderFromJackPorts()
    {
        float buffer[BUFFER_SIZE_MAX];

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                memcpy(buffer, fPortBuffer[port_index], fPeriodSize * sizeof(sample_t));
            } else {
                memset(buffer, 0, fPeriodSize * sizeof(sample_t));
            }
            int res = opus_custom_encode_float(fOpusEncoder[port_index], buffer, fPeriodSize, fCompressedBuffer[port_index], fCompressedMaxSizeByte);
            if (res <0 || res >= 65535) {
                fCompressedSizesByte[port_index] = 0;
            } else {
                fCompressedSizesByte[port_index] = res;
            }
        }

        // All ports active
        return fNPorts;
    }

    void NetOpusAudioBuffer::RenderToJackPorts()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                int res = opus_custom_decode_float(fOpusDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizesByte[port_index], fPortBuffer[port_index], fPeriodSize);
                if (res < 0 || res != fPeriodSize) {
                    jack_error("opus_decode_float error fCompressedSizeByte = %d res = %d", fCompressedSizesByte[port_index], res);
                }
            }
        }

        NextCycle();
    }

    //network<->buffer
    int NetOpusAudioBuffer::RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num)
    {
        // Cleanup all JACK ports at the beginning of the cycle
        if (sub_cycle == 0) {
            Cleanup();
        }

        if (port_num > 0)  {
            if (sub_cycle == 0) {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    size_t len = *((size_t*)(fNetBuffer + port_index * fSubPeriodBytesSize));
                    fCompressedSizesByte[port_index] = ntohs(len);
                    memcpy(fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, fNetBuffer + CDO + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize - CDO);
                }
            } else if (sub_cycle == fNumPackets - 1) {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    memcpy(fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize - CDO, fNetBuffer + port_index * fLastSubPeriodBytesSize, fLastSubPeriodBytesSize);
                }
            } else {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    memcpy(fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize - CDO, fNetBuffer + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize);
                }
            }
        }

        return CheckPacket(cycle, sub_cycle);
    }

    int NetOpusAudioBuffer::RenderToNetwork(int sub_cycle, uint32_t port_num)
    {
        if (sub_cycle == 0) {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                unsigned short len = htons(fCompressedSizesByte[port_index]);
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize, &len, CDO);
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize + CDO, fCompressedBuffer[port_index], fSubPeriodBytesSize - CDO);
            }
            return fNPorts * fSubPeriodBytesSize;
        } else if (sub_cycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fNetBuffer + port_index * fLastSubPeriodBytesSize, fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize - CDO, fLastSubPeriodBytesSize);
            }
            return fNPorts * fLastSubPeriodBytesSize;
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize, fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize - CDO, fSubPeriodBytesSize);
            }
            return fNPorts * fSubPeriodBytesSize;
        }
    }

#endif


    NetIntAudioBuffer::NetIntAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
        : NetAudioBuffer(params, nports, net_buffer)
    {
        fPeriodSize = params->fPeriodSize;

        fCompressedSizeByte = (params->fPeriodSize * sizeof(short));
        jack_log("NetIntAudioBuffer fCompressedSizeByte %d", fCompressedSizeByte);

        fIntBuffer = new short* [fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            fIntBuffer[port_index] = new short[fPeriodSize];
            memset(fIntBuffer[port_index], 0, fPeriodSize * sizeof(short));
        }

        int res1 = (fNPorts * fCompressedSizeByte) % PACKET_AVAILABLE_SIZE(params);
        int res2 = (fNPorts * fCompressedSizeByte) / PACKET_AVAILABLE_SIZE(params);

        jack_log("NetIntAudioBuffer res1 = %d res2 = %d", res1, res2);

        fNumPackets = (res1) ? (res2 + 1) : res2;

        fSubPeriodBytesSize = fCompressedSizeByte / fNumPackets;
        fLastSubPeriodBytesSize = fSubPeriodBytesSize + fCompressedSizeByte % fNumPackets;

        fSubPeriodSize = fSubPeriodBytesSize / sizeof(short);

        jack_log("NetIntAudioBuffer fNumPackets = %d fSubPeriodBytesSize = %d, fLastSubPeriodBytesSize = %d", fNumPackets, fSubPeriodBytesSize, fLastSubPeriodBytesSize);

        fCycleDuration = float(fSubPeriodBytesSize / sizeof(sample_t)) / float(params->fSampleRate);
        fCycleBytesSize = params->fMtu * fNumPackets;

        fLastSubCycle = -1;
        return;
    }

    NetIntAudioBuffer::~NetIntAudioBuffer()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            delete [] fIntBuffer[port_index];
        }

        delete [] fIntBuffer;
    }

    size_t NetIntAudioBuffer::GetCycleSize()
    {
        return fCycleBytesSize;
    }

    float NetIntAudioBuffer::GetCycleDuration()
    {
        return fCycleDuration;
    }

    int NetIntAudioBuffer::GetNumPackets(int active_ports)
    {
        return fNumPackets;
    }

    int NetIntAudioBuffer::RenderFromJackPorts()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                for (uint frame = 0; frame < fPeriodSize; frame++) {
                    fIntBuffer[port_index][frame] = short(fPortBuffer[port_index][frame] * 32768.f);
                }
            }
        }

        // All ports active
        return fNPorts;
    }

    void NetIntAudioBuffer::RenderToJackPorts()
    {
        float coef = 1.f / 32768.f;
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                for (uint frame = 0; frame < fPeriodSize; frame++) {
                    fPortBuffer[port_index][frame] = float(fIntBuffer[port_index][frame] * coef);
                }
            }
        }

        NextCycle();
     }

    //network<->buffer
    int NetIntAudioBuffer::RenderFromNetwork(int cycle, int sub_cycle, uint32_t port_num)
    {
        // Cleanup all JACK ports at the beginning of the cycle
        if (sub_cycle == 0) {
            Cleanup();
        }

        if (port_num > 0)  {
            if (sub_cycle == fNumPackets - 1) {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    memcpy(fIntBuffer[port_index] + sub_cycle * fSubPeriodSize, fNetBuffer + port_index * fLastSubPeriodBytesSize, fLastSubPeriodBytesSize);
                }
            } else {
                for (int port_index = 0; port_index < fNPorts; port_index++) {
                    memcpy(fIntBuffer[port_index] + sub_cycle * fSubPeriodSize, fNetBuffer + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize);
                }
            }
        }

        return CheckPacket(cycle, sub_cycle);
    }

    int NetIntAudioBuffer::RenderToNetwork(int sub_cycle, uint32_t port_num)
    {
        // Last packet of the cycle
        if (sub_cycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fNetBuffer + port_index * fLastSubPeriodBytesSize, fIntBuffer[port_index] + sub_cycle * fSubPeriodSize, fLastSubPeriodBytesSize);
            }
            return fNPorts * fLastSubPeriodBytesSize;
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize, fIntBuffer[port_index] + sub_cycle * fSubPeriodSize, fSubPeriodBytesSize);
            }
            return fNPorts * fSubPeriodBytesSize;
        }
    }

// SessionParams ************************************************************************************

    SERVER_EXPORT void SessionParamsHToN(session_params_t* src_params, session_params_t* dst_params)
    {
        memcpy(dst_params, src_params, sizeof(session_params_t));
        dst_params->fProtocolVersion = htonl(src_params->fProtocolVersion);
        dst_params->fPacketID = htonl(src_params->fPacketID);
        dst_params->fMtu = htonl(src_params->fMtu);
        dst_params->fID = htonl(src_params->fID);
        dst_params->fTransportSync = htonl(src_params->fTransportSync);
        dst_params->fSendAudioChannels = htonl(src_params->fSendAudioChannels);
        dst_params->fReturnAudioChannels = htonl(src_params->fReturnAudioChannels);
        dst_params->fSendMidiChannels = htonl(src_params->fSendMidiChannels);
        dst_params->fReturnMidiChannels = htonl(src_params->fReturnMidiChannels);
        dst_params->fSampleRate = htonl(src_params->fSampleRate);
        dst_params->fPeriodSize = htonl(src_params->fPeriodSize);
        dst_params->fSampleEncoder = htonl(src_params->fSampleEncoder);
        dst_params->fKBps = htonl(src_params->fKBps);
        dst_params->fSlaveSyncMode = htonl(src_params->fSlaveSyncMode);
        dst_params->fNetworkLatency = htonl(src_params->fNetworkLatency);
    }

    SERVER_EXPORT void SessionParamsNToH(session_params_t* src_params, session_params_t* dst_params)
    {
        memcpy(dst_params, src_params, sizeof(session_params_t));
        dst_params->fProtocolVersion = ntohl(src_params->fProtocolVersion);
        dst_params->fPacketID = ntohl(src_params->fPacketID);
        dst_params->fMtu = ntohl(src_params->fMtu);
        dst_params->fID = ntohl(src_params->fID);
        dst_params->fTransportSync = ntohl(src_params->fTransportSync);
        dst_params->fSendAudioChannels = ntohl(src_params->fSendAudioChannels);
        dst_params->fReturnAudioChannels = ntohl(src_params->fReturnAudioChannels);
        dst_params->fSendMidiChannels = ntohl(src_params->fSendMidiChannels);
        dst_params->fReturnMidiChannels = ntohl(src_params->fReturnMidiChannels);
        dst_params->fSampleRate = ntohl(src_params->fSampleRate);
        dst_params->fPeriodSize = ntohl(src_params->fPeriodSize);
        dst_params->fSampleEncoder = ntohl(src_params->fSampleEncoder);
        dst_params->fKBps = ntohl(src_params->fKBps);
        dst_params->fSlaveSyncMode = ntohl(src_params->fSlaveSyncMode);
        dst_params->fNetworkLatency = ntohl(src_params->fNetworkLatency);
    }

    SERVER_EXPORT void SessionParamsDisplay(session_params_t* params)
    {
        char encoder[16];
        switch (params->fSampleEncoder)
        {
            case JackFloatEncoder:
                strcpy(encoder, "float");
                break;
            case JackIntEncoder:
                strcpy(encoder, "integer");
                break;
            case JackCeltEncoder:
                strcpy(encoder, "CELT");
                break;
            case JackOpusEncoder:
                strcpy(encoder, "OPUS");
                break;
        }

        jack_info("**************** Network parameters ****************");
        jack_info("Name : %s", params->fName);
        jack_info("Protocol revision : %d", params->fProtocolVersion);
        jack_info("MTU : %u", params->fMtu);
        jack_info("Master name : %s", params->fMasterNetName);
        jack_info("Slave name : %s", params->fSlaveNetName);
        jack_info("ID : %u", params->fID);
        jack_info("Transport Sync : %s", (params->fTransportSync) ? "yes" : "no");
        jack_info("Send channels (audio - midi) : %d - %d", params->fSendAudioChannels, params->fSendMidiChannels);
        jack_info("Return channels (audio - midi) : %d - %d", params->fReturnAudioChannels, params->fReturnMidiChannels);
        jack_info("Sample rate : %u frames per second", params->fSampleRate);
        jack_info("Period size : %u frames per period", params->fPeriodSize);
        jack_info("Network latency : %u cycles", params->fNetworkLatency);
        switch (params->fSampleEncoder) {
            case (JackFloatEncoder):
                jack_info("SampleEncoder : %s", "Float");
                break;
            case (JackIntEncoder):
                jack_info("SampleEncoder : %s", "16 bits integer");
                break;
            case (JackCeltEncoder):
                jack_info("SampleEncoder : %s", "CELT");
                jack_info("kBits : %d", params->fKBps);
                break;
            case (JackOpusEncoder):
                jack_info("SampleEncoder : %s", "OPUS");
                jack_info("kBits : %d", params->fKBps);
                break;
        };
        jack_info("Slave mode : %s", (params->fSlaveSyncMode) ? "sync" : "async");
        jack_info("****************************************************");
    }

    SERVER_EXPORT sync_packet_type_t GetPacketType(session_params_t* params)
    {
        switch (params->fPacketID)
        {
            case 0:
                return SLAVE_AVAILABLE;
            case 1:
                return SLAVE_SETUP;
            case 2:
                return START_MASTER;
            case 3:
                return START_SLAVE;
            case 4:
                return KILL_MASTER;
        }
        return INVALID;
    }

    SERVER_EXPORT int SetPacketType(session_params_t* params, sync_packet_type_t packet_type)
    {
        switch (packet_type)
        {
            case INVALID:
                return -1;
            case SLAVE_AVAILABLE:
                params->fPacketID = 0;
                break;
            case SLAVE_SETUP:
                params->fPacketID = 1;
                break;
            case START_MASTER:
                params->fPacketID = 2;
                break;
            case START_SLAVE:
                params->fPacketID = 3;
                break;
            case KILL_MASTER:
                params->fPacketID = 4;
        }
        return 0;
    }

// Packet header **********************************************************************************

    SERVER_EXPORT void PacketHeaderHToN(packet_header_t* src_header, packet_header_t* dst_header)
    {
        memcpy(dst_header, src_header, sizeof(packet_header_t));
        dst_header->fDataType = htonl(src_header->fDataType);
        dst_header->fDataStream = htonl(src_header->fDataStream);
        dst_header->fID = htonl(src_header->fID);
        dst_header->fNumPacket = htonl(src_header->fNumPacket);
        dst_header->fPacketSize = htonl(src_header->fPacketSize);
        dst_header->fActivePorts = htonl(src_header->fActivePorts);
        dst_header->fCycle = htonl(src_header->fCycle);
        dst_header->fSubCycle = htonl(src_header->fSubCycle);
        dst_header->fIsLastPckt = htonl(src_header->fIsLastPckt);
    }

    SERVER_EXPORT void PacketHeaderNToH(packet_header_t* src_header, packet_header_t* dst_header)
    {
        memcpy(dst_header, src_header, sizeof(packet_header_t));
        dst_header->fDataType = ntohl(src_header->fDataType);
        dst_header->fDataStream = ntohl(src_header->fDataStream);
        dst_header->fID = ntohl(src_header->fID);
        dst_header->fNumPacket = ntohl(src_header->fNumPacket);
        dst_header->fPacketSize = ntohl(src_header->fPacketSize);
        dst_header->fActivePorts = ntohl(src_header->fActivePorts);
        dst_header->fCycle = ntohl(src_header->fCycle);
        dst_header->fSubCycle = ntohl(src_header->fSubCycle);
        dst_header->fIsLastPckt = ntohl(src_header->fIsLastPckt);
    }

    SERVER_EXPORT void PacketHeaderDisplay(packet_header_t* header)
    {
        char bitdepth[16];
        jack_info("********************Header********************");
        jack_info("Data type : %c", header->fDataType);
        jack_info("Data stream : %c", header->fDataStream);
        jack_info("ID : %u", header->fID);
        jack_info("Cycle : %u", header->fCycle);
        jack_info("SubCycle : %u", header->fSubCycle);
        jack_info("Active ports : %u", header->fActivePorts);
        jack_info("DATA packets : %u", header->fNumPacket);
        jack_info("DATA size : %u", header->fPacketSize);
        jack_info("Last packet : '%s'", (header->fIsLastPckt) ? "yes" : "no");
        jack_info("Bitdepth : %s", bitdepth);
        jack_info("**********************************************");
    }

    SERVER_EXPORT void NetTransportDataDisplay(net_transport_data_t* data)
    {
        jack_info("********************Network Transport********************");
        jack_info("Transport new state : %u", data->fNewState);
        jack_info("Transport timebase master : %u", data->fTimebaseMaster);
        jack_info("Transport cycle state : %u", data->fState);
        jack_info("**********************************************");
    }

    SERVER_EXPORT void MidiBufferHToN(JackMidiBuffer* src_buffer, JackMidiBuffer* dst_buffer)
    {
        dst_buffer->magic = htonl(src_buffer->magic);
        dst_buffer->buffer_size = htonl(src_buffer->buffer_size);
        dst_buffer->nframes = htonl(src_buffer->nframes);
        dst_buffer->write_pos = htonl(src_buffer->write_pos);
        dst_buffer->event_count = htonl(src_buffer->event_count);
        dst_buffer->lost_events = htonl(src_buffer->lost_events);
        dst_buffer->mix_index = htonl(src_buffer->mix_index);
    }

    SERVER_EXPORT void MidiBufferNToH(JackMidiBuffer* src_buffer, JackMidiBuffer* dst_buffer)
    {
        dst_buffer->magic = ntohl(src_buffer->magic);
        dst_buffer->buffer_size = ntohl(src_buffer->buffer_size);
        dst_buffer->nframes = ntohl(src_buffer->nframes);
        dst_buffer->write_pos = ntohl(src_buffer->write_pos);
        dst_buffer->event_count = ntohl(src_buffer->event_count);
        dst_buffer->lost_events = ntohl(src_buffer->lost_events);
        dst_buffer->mix_index = ntohl(src_buffer->mix_index);
    }

    SERVER_EXPORT void TransportDataHToN(net_transport_data_t* src_params, net_transport_data_t* dst_params)
    {
        dst_params->fNewState = htonl(src_params->fNewState);
        dst_params->fTimebaseMaster = htonl(src_params->fTimebaseMaster);
        dst_params->fState = htonl(src_params->fState);
        dst_params->fPosition.unique_1 = htonll(src_params->fPosition.unique_1);
        dst_params->fPosition.usecs = htonl(src_params->fPosition.usecs);
        dst_params->fPosition.frame_rate = htonl(src_params->fPosition.frame_rate);
        dst_params->fPosition.frame = htonl(src_params->fPosition.frame);
        dst_params->fPosition.valid = (jack_position_bits_t)htonl((uint32_t)src_params->fPosition.valid);
        dst_params->fPosition.bar = htonl(src_params->fPosition.bar);
        dst_params->fPosition.beat = htonl(src_params->fPosition.beat);
        dst_params->fPosition.tick = htonl(src_params->fPosition.tick);
        dst_params->fPosition.bar_start_tick = htonll((uint64_t)src_params->fPosition.bar_start_tick);
        dst_params->fPosition.beats_per_bar = htonl((uint32_t)src_params->fPosition.beats_per_bar);
        dst_params->fPosition.beat_type = htonl((uint32_t)src_params->fPosition.beat_type);
        dst_params->fPosition.ticks_per_beat = htonll((uint64_t)src_params->fPosition.ticks_per_beat);
        dst_params->fPosition.beats_per_minute = htonll((uint64_t)src_params->fPosition.beats_per_minute);
        dst_params->fPosition.frame_time = htonll((uint64_t)src_params->fPosition.frame_time);
        dst_params->fPosition.next_time = htonll((uint64_t)src_params->fPosition.next_time);
        dst_params->fPosition.bbt_offset = htonl(src_params->fPosition.bbt_offset);
        dst_params->fPosition.audio_frames_per_video_frame = htonl((uint32_t)src_params->fPosition.audio_frames_per_video_frame);
        dst_params->fPosition.video_offset = htonl(src_params->fPosition.video_offset);
        dst_params->fPosition.unique_2 = htonll(src_params->fPosition.unique_2);
    }

    SERVER_EXPORT void TransportDataNToH(net_transport_data_t* src_params, net_transport_data_t* dst_params)
    {
        dst_params->fNewState = ntohl(src_params->fNewState);
        dst_params->fTimebaseMaster =  ntohl(src_params->fTimebaseMaster);
        dst_params->fState = ntohl(src_params->fState);
        dst_params->fPosition.unique_1 = ntohll(src_params->fPosition.unique_1);
        dst_params->fPosition.usecs = ntohl(src_params->fPosition.usecs);
        dst_params->fPosition.frame_rate = ntohl(src_params->fPosition.frame_rate);
        dst_params->fPosition.frame = ntohl(src_params->fPosition.frame);
        dst_params->fPosition.valid = (jack_position_bits_t)ntohl((uint32_t)src_params->fPosition.valid);
        dst_params->fPosition.bar = ntohl(src_params->fPosition.bar);
        dst_params->fPosition.beat = ntohl(src_params->fPosition.beat);
        dst_params->fPosition.tick = ntohl(src_params->fPosition.tick);
        dst_params->fPosition.bar_start_tick = ntohll((uint64_t)src_params->fPosition.bar_start_tick);
        dst_params->fPosition.beats_per_bar = ntohl((uint32_t)src_params->fPosition.beats_per_bar);
        dst_params->fPosition.beat_type = ntohl((uint32_t)src_params->fPosition.beat_type);
        dst_params->fPosition.ticks_per_beat = ntohll((uint64_t)src_params->fPosition.ticks_per_beat);
        dst_params->fPosition.beats_per_minute = ntohll((uint64_t)src_params->fPosition.beats_per_minute);
        dst_params->fPosition.frame_time = ntohll((uint64_t)src_params->fPosition.frame_time);
        dst_params->fPosition.next_time = ntohll((uint64_t)src_params->fPosition.next_time);
        dst_params->fPosition.bbt_offset = ntohl(src_params->fPosition.bbt_offset);
        dst_params->fPosition.audio_frames_per_video_frame = ntohl((uint32_t)src_params->fPosition.audio_frames_per_video_frame);
        dst_params->fPosition.video_offset = ntohl(src_params->fPosition.video_offset);
        dst_params->fPosition.unique_2 = ntohll(src_params->fPosition.unique_2);
    }

// Utility *******************************************************************************************************

    SERVER_EXPORT int SocketAPIInit()
    {
#ifdef WIN32
        WORD wVersionRequested = MAKEWORD(2, 2);
        WSADATA wsaData;

        if (WSAStartup(wVersionRequested, &wsaData) != 0) {
            jack_error("WSAStartup error : %s", strerror(NET_ERROR_CODE));
            return -1;
        }

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            jack_error("Could not find a useable version of Winsock.dll\n");
            WSACleanup();
            return -1;
        }
#endif
        return 0;
    }

    SERVER_EXPORT int SocketAPIEnd()
    {
#ifdef WIN32
        return WSACleanup();
#endif
        return 0;
    }

    SERVER_EXPORT const char* GetTransportState(int transport_state)
    {
        switch (transport_state)
        {
            case JackTransportRolling:
                return "rolling";
            case JackTransportStarting:
                return "starting";
            case JackTransportStopped:
                return "stopped";
            case JackTransportNetStarting:
                return "netstarting";
        }
        return NULL;
    }
}
