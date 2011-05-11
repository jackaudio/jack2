/*
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

#include "JackNetTool.h"

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
        for (int port_index = 0; port_index < fNPorts; port_index++)
            fPortBuffer[port_index] = NULL;
        fNetBuffer = net_buffer;

        fCycleSize = params->fMtu * (max(params->fSendMidiChannels, params->fReturnMidiChannels) *
                                     params->fPeriodSize * sizeof(sample_t) / (params->fMtu - sizeof(packet_header_t)));
    }

    NetMidiBuffer::~NetMidiBuffer()
    {
        delete[] fBuffer;
        delete[] fPortBuffer;
    }

    size_t NetMidiBuffer::GetCycleSize()
    {
        return fCycleSize;
    }

    int NetMidiBuffer::GetNumPackets(int data_size, int max_size)
    {
        return (data_size % max_size)
                ? (data_size / max_size + 1)
                : data_size / max_size;
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
        for (int port_index = 0; port_index < fNPorts; port_index++)
        {
            for (uint event = 0; event < fPortBuffer[port_index]->event_count; event++)
                if (fPortBuffer[port_index]->IsValid())
                    jack_info("port %d : midi event %u/%u -> time : %u, size : %u",
                                port_index + 1, event + 1, fPortBuffer[port_index]->event_count,
                                fPortBuffer[port_index]->events[event].time, fPortBuffer[port_index]->events[event].size);
        }
    }

    int NetMidiBuffer::RenderFromJackPorts()
    {
        int pos = 0;
        size_t copy_size;
        for (int port_index = 0; port_index < fNPorts; port_index++)
        {
            char* write_pos = fBuffer + pos;
            copy_size = sizeof(JackMidiBuffer) + fPortBuffer[port_index]->event_count * sizeof(JackMidiEvent);
            memcpy(fBuffer + pos, fPortBuffer[port_index], copy_size);
            pos += copy_size;
            memcpy(fBuffer + pos, fPortBuffer[port_index] + (fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos),
                     fPortBuffer[port_index]->write_pos);
            pos += fPortBuffer[port_index]->write_pos;

            JackMidiBuffer* midi_buffer = reinterpret_cast<JackMidiBuffer*>(write_pos);
            MidiBufferHToN(midi_buffer, midi_buffer);
        }
        return pos;
    }

    int NetMidiBuffer::RenderToJackPorts()
    {
        int pos = 0;
        int copy_size;
        for (int port_index = 0; port_index < fNPorts; port_index++)
        {
            JackMidiBuffer* midi_buffer = reinterpret_cast<JackMidiBuffer*>(fBuffer + pos);
            MidiBufferNToH(midi_buffer, midi_buffer);
            copy_size = sizeof(JackMidiBuffer) + reinterpret_cast<JackMidiBuffer*>(fBuffer + pos)->event_count * sizeof(JackMidiEvent);
            memcpy(fPortBuffer[port_index], fBuffer + pos, copy_size);
            pos += copy_size;
            memcpy(fPortBuffer[port_index] + (fPortBuffer[port_index]->buffer_size - fPortBuffer[port_index]->write_pos),
                     fBuffer + pos, fPortBuffer[port_index]->write_pos);
            pos += fPortBuffer[port_index]->write_pos;
        }
        return pos;
    }

    int NetMidiBuffer::RenderFromNetwork(int subcycle, size_t copy_size)
    {
        memcpy(fBuffer + subcycle * fMaxPcktSize, fNetBuffer, copy_size);
        return copy_size;
    }

    int NetMidiBuffer::RenderToNetwork(int subcycle, size_t total_size)
    {
        int size = total_size - subcycle * fMaxPcktSize;
        int copy_size = (size <= fMaxPcktSize) ? size : fMaxPcktSize;
        memcpy(fNetBuffer, fBuffer + subcycle * fMaxPcktSize, copy_size);
        return copy_size;
    }

// net audio buffer *********************************************************************************

    NetFloatAudioBuffer::NetFloatAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
        : fPortBuffer(params, nports), fNetBuffer(net_buffer)
    {}

    NetFloatAudioBuffer::~NetFloatAudioBuffer()
    {}

    size_t NetFloatAudioBuffer::GetCycleSize()
    {
        return fPortBuffer.GetCycleSize();
    }

    void NetFloatAudioBuffer::SetBuffer(int index, sample_t* buffer)
    {
        fPortBuffer.SetBuffer(index, buffer);
    }

    sample_t* NetFloatAudioBuffer::GetBuffer(int index)
    {
        return fPortBuffer.GetBuffer(index);
    }

    int NetFloatAudioBuffer::RenderFromJackPorts ()
    {
        return fPortBuffer.RenderFromJackPorts();
    }

    int NetFloatAudioBuffer::RenderToJackPorts ()
    {
        return fPortBuffer.RenderToJackPorts();
    }

     //network<->buffer
    int NetFloatAudioBuffer::RenderFromNetwork(int cycle,  int subcycle, size_t copy_size)
    {
        return fPortBuffer.RenderFromNetwork(fNetBuffer, cycle, subcycle, copy_size);
    }

    int NetFloatAudioBuffer::RenderToNetwork (int subcycle, size_t total_size)
    {
        return fPortBuffer.RenderToNetwork(fNetBuffer, subcycle, total_size);
    }

    // Celt audio buffer *********************************************************************************

#if HAVE_CELT

    #define KPS 32
    #define KPS_DIV 8

    NetCeltAudioBuffer::NetCeltAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps)
        : fNetBuffer(net_buffer)
    {
        int res1, res2;

        fNPorts = nports;
        fPeriodSize = params->fPeriodSize;

        fCeltMode = new CELTMode *[fNPorts];
        fCeltEncoder = new CELTEncoder *[fNPorts];
        fCeltDecoder = new CELTDecoder *[fNPorts];

        memset(fCeltMode, 0, fNPorts * sizeof(CELTMode*));
        memset(fCeltEncoder, 0, fNPorts * sizeof(CELTEncoder*));
        memset(fCeltDecoder, 0, fNPorts * sizeof(CELTDecoder*));

        int error = CELT_OK;

        for (int i = 0; i < fNPorts; i++)  {
            fCeltMode[i] = celt_mode_create(params->fSampleRate, params->fPeriodSize, &error);
            if (error != CELT_OK)
                goto error;

    #if HAVE_CELT_API_0_11

            fCeltEncoder[i] = celt_encoder_create_custom(fCeltMode[i], 1, &error);
            if (error != CELT_OK)
                goto error;
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create_custom(fCeltMode[i], 1, &error);
            if (error != CELT_OK)
                goto error;
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8

            fCeltEncoder[i] = celt_encoder_create(fCeltMode[i], 1, &error);
            if (error != CELT_OK)
                goto error;
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create(fCeltMode[i], 1, &error);
            if (error != CELT_OK)
                goto error;
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #else

            fCeltEncoder[i] = celt_encoder_create(fCeltMode[i]);
            if (error != CELT_OK)
                goto error;
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create(fCeltMode[i]);
            if (error != CELT_OK)
                goto error;
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #endif
        }

        fPortBuffer = new sample_t* [fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++)
            fPortBuffer[port_index] = NULL;

        /*
        celt_int32 lookahead;
        celt_mode_info(celt_mode, CELT_GET_LOOKAHEAD, &lookahead);
        */

        fCompressedSizeByte = (kbps * params->fPeriodSize * 1024) / (params->fSampleRate * 8);
        //fCompressedSizeByte = (params->fPeriodSize * sizeof(sample_t)) / KPS_DIV;   // TODO

        fCompressedBuffer = new unsigned char* [fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++)
            fCompressedBuffer[port_index] = new unsigned char[fCompressedSizeByte];

        jack_log("NetCeltAudioBuffer fCompressedSizeByte %d", fCompressedSizeByte);

        res1 = (fNPorts * fCompressedSizeByte) % (params->fMtu - sizeof(packet_header_t));
        res2 = (fNPorts * fCompressedSizeByte) / (params->fMtu - sizeof(packet_header_t));

        jack_log("NetCeltAudioBuffer res1 = %d res2 = %d", res1, res2);

        fNumPackets = (res1) ? (res2 + 1) : res2;

        fSubPeriodBytesSize = fCompressedSizeByte / fNumPackets;
        fLastSubPeriodBytesSize = fSubPeriodBytesSize + fCompressedSizeByte % fNumPackets;

        jack_log("NetCeltAudioBuffer fNumPackets = %d fSubPeriodBytesSize = %d, fLastSubPeriodBytesSize = %d", fNumPackets, fSubPeriodBytesSize, fLastSubPeriodBytesSize);

        fCycleDuration = float(fSubPeriodBytesSize / sizeof(sample_t)) / float(params->fSampleRate);
        fCycleSize = params->fMtu * fNumPackets;

        fLastSubCycle = -1;
        return;

    error:

        FreeCelt();
        throw std::bad_alloc();
    }

    NetCeltAudioBuffer::~NetCeltAudioBuffer()
    {
        FreeCelt();

        for (int port_index = 0; port_index < fNPorts; port_index++)
            delete [] fCompressedBuffer[port_index];

        delete [] fCompressedBuffer;
        delete [] fPortBuffer;
    }

    void NetCeltAudioBuffer::FreeCelt()
    {
        for (int i = 0; i < fNPorts; i++)  {
            if (fCeltEncoder[i])
                celt_encoder_destroy(fCeltEncoder[i]);
            if (fCeltDecoder[i])
                celt_decoder_destroy(fCeltDecoder[i]);
            if (fCeltMode[i])
                celt_mode_destroy(fCeltMode[i]);
        }

        delete [] fCeltMode;
        delete [] fCeltEncoder;
        delete [] fCeltDecoder;
    }

    size_t NetCeltAudioBuffer::GetCycleSize()
    {
        return fCycleSize;
    }

    float NetCeltAudioBuffer::GetCycleDuration()
    {
        return fCycleDuration;
    }

    int NetCeltAudioBuffer::GetNumPackets()
    {
        return fNumPackets;
    }

    void NetCeltAudioBuffer::SetBuffer(int index, sample_t* buffer)
    {
        assert(fPortBuffer);
        fPortBuffer[index] = buffer;
    }

    sample_t* NetCeltAudioBuffer::GetBuffer(int index)
    {
        assert(fPortBuffer);
        return fPortBuffer[index];
    }

    int NetCeltAudioBuffer::RenderFromJackPorts()
    {
        float floatbuf[fPeriodSize];

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            memcpy(floatbuf, fPortBuffer[port_index], fPeriodSize * sizeof(float));
#if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
            int res = celt_encode_float(fCeltEncoder[port_index], floatbuf, fPeriodSize, fCompressedBuffer[port_index], fCompressedSizeByte);
#else
            int res = celt_encode_float(fCeltEncoder[port_index], floatbuf, NULL, fCompressedBuffer[port_index], fCompressedSizeByte);
#endif
            if (res != fCompressedSizeByte) {
                jack_error("celt_encode_float error fCompressedSizeByte = %d  res = %d", fCompressedSizeByte, res);
            }
        }

        return fNPorts * fCompressedSizeByte;  // in bytes
    }

    int NetCeltAudioBuffer::RenderToJackPorts()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
#if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
            int res = celt_decode_float(fCeltDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizeByte, fPortBuffer[port_index], fPeriodSize);
#else
            int res = celt_decode_float(fCeltDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizeByte, fPortBuffer[port_index]);
#endif
            if (res != CELT_OK) {
                jack_error("celt_decode_float error res = %d", fCompressedSizeByte, res);
            }
        }

        fLastSubCycle = -1;
        //return fPeriodSize * sizeof(sample_t);  // in bytes; TODO
        return 0;
    }

    //network<->buffer
    int NetCeltAudioBuffer::RenderFromNetwork(int cycle, int subcycle, size_t copy_size)
    {
        if (subcycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fCompressedBuffer[port_index] + subcycle * fSubPeriodBytesSize, fNetBuffer + port_index * fLastSubPeriodBytesSize, fLastSubPeriodBytesSize);
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fCompressedBuffer[port_index] + subcycle * fSubPeriodBytesSize, fNetBuffer + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize);
        }

        if (subcycle != fLastSubCycle + 1)
            jack_error("Packet(s) missing from... %d %d", fLastSubCycle, subcycle);

        fLastSubCycle = subcycle;
        return copy_size;
    }

    int NetCeltAudioBuffer::RenderToNetwork(int subcycle, size_t total_size)
    {
        if (subcycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fNetBuffer + port_index * fLastSubPeriodBytesSize, fCompressedBuffer[port_index] + subcycle * fSubPeriodBytesSize, fLastSubPeriodBytesSize);
            return fNPorts * fLastSubPeriodBytesSize;
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize, fCompressedBuffer[port_index] + subcycle * fSubPeriodBytesSize, fSubPeriodBytesSize);
            return fNPorts * fSubPeriodBytesSize;
        }

        return fNPorts * fSubPeriodBytesSize;
    }

#endif

    NetIntAudioBuffer::NetIntAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
        : fNetBuffer(net_buffer)
    {
        int res1, res2;

        fNPorts = nports;
        fPeriodSize = params->fPeriodSize;

        fPortBuffer = new sample_t* [fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++)
            fPortBuffer[port_index] = NULL;

        fIntBuffer = new short* [fNPorts];
        for (int port_index = 0; port_index < fNPorts; port_index++)
            fIntBuffer[port_index] = new short[fPeriodSize];

        fCompressedSizeByte = (params->fPeriodSize * sizeof(short));

        jack_log("fCompressedSizeByte %d", fCompressedSizeByte);

        res1 = (fNPorts * fCompressedSizeByte) % (params->fMtu - sizeof(packet_header_t));
        res2 = (fNPorts * fCompressedSizeByte) / (params->fMtu - sizeof(packet_header_t));

        jack_log("res1 = %d res2 = %d", res1, res2);

        fNumPackets = (res1) ? (res2 + 1) : res2;

        fSubPeriodBytesSize = fCompressedSizeByte / fNumPackets;
        fSubPeriodSize = fSubPeriodBytesSize / sizeof(short);

        fLastSubPeriodBytesSize = fSubPeriodBytesSize + fCompressedSizeByte % fNumPackets;
        fLastSubPeriodSize = fLastSubPeriodBytesSize / sizeof(short);

        jack_log("fNumPackets = %d fSubPeriodBytesSize = %d, fLastSubPeriodBytesSize = %d", fNumPackets, fSubPeriodBytesSize, fLastSubPeriodBytesSize);

        fCycleDuration = float(fSubPeriodBytesSize / sizeof(sample_t)) / float(params->fSampleRate);
        fCycleSize = params->fMtu * fNumPackets;

        fLastSubCycle = -1;
        return;
    }

    NetIntAudioBuffer::~NetIntAudioBuffer()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++)
            delete [] fIntBuffer[port_index];

        delete [] fIntBuffer;
        delete [] fPortBuffer;
    }

    size_t NetIntAudioBuffer::GetCycleSize()
    {
        return fCycleSize;
    }

    float NetIntAudioBuffer::GetCycleDuration()
    {
        return fCycleDuration;
    }

    int NetIntAudioBuffer::GetNumPackets()
    {
        return fNumPackets;
    }

    void NetIntAudioBuffer::SetBuffer(int index, sample_t* buffer)
    {
        fPortBuffer[index] = buffer;
    }

    sample_t* NetIntAudioBuffer::GetBuffer(int index)
    {
        return fPortBuffer[index];
    }

    int NetIntAudioBuffer::RenderFromJackPorts()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            for (unsigned int frame = 0; frame < fPeriodSize; frame++)
                fIntBuffer[port_index][frame] = short(fPortBuffer[port_index][frame] * 32768.f);
        }

        return fNPorts * fCompressedSizeByte;  // in bytes
    }

    int NetIntAudioBuffer::RenderToJackPorts()
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            float coef = 1.f / 32768.f;
            for (unsigned int frame = 0; frame < fPeriodSize; frame++)
                fPortBuffer[port_index][frame] = float(fIntBuffer[port_index][frame] * coef);
        }

        fLastSubCycle = -1;
        //return fPeriodSize * sizeof(sample_t);  // in bytes; TODO
        return 0;
    }

     //network<->buffer
    int NetIntAudioBuffer::RenderFromNetwork(int cycle, int subcycle, size_t copy_size)
    {
        if (subcycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fIntBuffer[port_index] + subcycle * fSubPeriodSize, fNetBuffer + port_index * fLastSubPeriodBytesSize, fLastSubPeriodBytesSize);
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fIntBuffer[port_index] + subcycle * fSubPeriodSize, fNetBuffer + port_index * fSubPeriodBytesSize, fSubPeriodBytesSize);
        }

        if (subcycle != fLastSubCycle + 1)
            jack_error("Packet(s) missing from... %d %d", fLastSubCycle, subcycle);

        fLastSubCycle = subcycle;
        return copy_size;
    }

    int NetIntAudioBuffer::RenderToNetwork(int subcycle, size_t total_size)
    {
        if (subcycle == fNumPackets - 1) {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fNetBuffer + port_index * fLastSubPeriodBytesSize, fIntBuffer[port_index] + subcycle * fSubPeriodSize, fLastSubPeriodBytesSize);
            return fNPorts * fLastSubPeriodBytesSize;
        } else {
            for (int port_index = 0; port_index < fNPorts; port_index++)
                memcpy(fNetBuffer + port_index * fSubPeriodBytesSize, fIntBuffer[port_index] + subcycle * fSubPeriodSize, fSubPeriodBytesSize);
            return fNPorts * fSubPeriodBytesSize;
        }
    }

// Buffered

/*
    NetBufferedAudioBuffer::NetBufferedAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer)
    {
        fMaxCycle = 0;
        fNetBuffer = net_buffer;

        for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
            fPortBuffer[i].Init(params, nports);
        }

        fJackPortBuffer = new sample_t* [nports];
        for (uint32_t port_index = 0; port_index < nports; port_index++)
            fJackPortBuffer[port_index] = NULL;
    }

    NetBufferedAudioBuffer::~NetBufferedAudioBuffer()
    {
        delete [] fJackPortBuffer;
    }

    size_t NetBufferedAudioBuffer::GetCycleSize()
    {
        return fPortBuffer[0].GetCycleSize();
    }

    void NetBufferedAudioBuffer::SetBuffer(int index, sample_t* buffer)
    {
        fJackPortBuffer[index] = buffer;
    }

    sample_t* NetBufferedAudioBuffer::GetBuffer(int index)
    {
        return fJackPortBuffer[index];
    }

    void NetBufferedAudioBuffer::RenderFromJackPorts (int subcycle)
    {
        fPortBuffer[0].RenderFromJackPorts(fNetBuffer, subcycle);  // Always use first buffer...
    }

    void NetBufferedAudioBuffer::RenderToJackPorts (int cycle, int subcycle)
    {
        if (cycle < fMaxCycle) {
            jack_info("Wrong order fCycle %d subcycle %d fMaxCycle %d", cycle, subcycle, fMaxCycle);
        }
        fPortBuffer[cycle % AUDIO_BUFFER_SIZE].RenderToJackPorts(fNetBuffer, subcycle);
    }

    void NetBufferedAudioBuffer::FinishRenderToJackPorts (int cycle)
    {
        fMaxCycle = std::max(fMaxCycle, cycle);
        fPortBuffer[(cycle + 1) % AUDIO_BUFFER_SIZE].Copy(fJackPortBuffer);  // Copy internal buffer in JACK ports
    }
    */

// SessionParams ************************************************************************************

    SERVER_EXPORT void SessionParamsHToN(session_params_t* src_params, session_params_t* dst_params)
    {
        memcpy(dst_params, src_params, sizeof(session_params_t));
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
        dst_params->fSlaveSyncMode = htonl(src_params->fSlaveSyncMode);
    }

    SERVER_EXPORT void SessionParamsNToH(session_params_t* src_params, session_params_t* dst_params)
    {
        memcpy(dst_params, src_params, sizeof(session_params_t));
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
        dst_params->fSlaveSyncMode = ntohl(src_params->fSlaveSyncMode);
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
        }

        char mode[8];
        switch (params->fNetworkMode)
        {
            case 's' :
                strcpy(mode, "slow");
                break;
            case 'n' :
                strcpy(mode, "normal");
                break;
            case 'f' :
                strcpy(mode, "fast");
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
        };
        jack_info("Slave mode : %s", (params->fSlaveSyncMode) ? "sync" : "async");
        jack_info("Network mode : %s", mode);
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
        dst_header->fID = htonl(src_header->fID);
        dst_header->fNumPacket = htonl(src_header->fNumPacket);
        dst_header->fPacketSize = htonl(src_header->fPacketSize);
        dst_header->fCycle = htonl(src_header->fCycle);
        dst_header->fSubCycle = htonl(src_header->fSubCycle);
        dst_header->fIsLastPckt = htonl(src_header->fIsLastPckt);
    }

    SERVER_EXPORT void PacketHeaderNToH(packet_header_t* src_header, packet_header_t* dst_header)
    {
        memcpy(dst_header, src_header, sizeof(packet_header_t));
        dst_header->fID = ntohl(src_header->fID);
        dst_header->fNumPacket = ntohl(src_header->fNumPacket);
        dst_header->fPacketSize = ntohl(src_header->fPacketSize);
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
        jack_info("DATA packets : %u", header->fNumPacket);
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

        if (WSAStartup(wVersionRequested, &wsaData) != 0)
        {
            jack_error("WSAStartup error : %s", strerror(NET_ERROR_CODE));
            return -1;
        }

        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
        {
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
