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

#include "JackNetOpus.h"
#include "JackError.h"

namespace Jack
{
#define CDO (sizeof(short)) ///< compressed data offset (first 2 bytes are length)
    NetOpusAudioBuffer::NetOpusAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps)
        :NetAudioBuffer(params, nports, net_buffer)
    {
        fOpusMode = new OpusCustomMode*[fNPorts];
        fOpusEncoder = new OpusCustomEncoder*[fNPorts];
        fOpusDecoder = new OpusCustomDecoder*[fNPorts];
        fCompressedSizesByte = new unsigned short[fNPorts];

        memset(fOpusMode, 0, fNPorts * sizeof(OpusCustomMode*));
        memset(fOpusEncoder, 0, fNPorts * sizeof(OpusCustomEncoder*));
        memset(fOpusDecoder, 0, fNPorts * sizeof(OpusCustomDecoder*));
        memset(fCompressedSizesByte, 0, fNPorts * sizeof(short));

        int error = OPUS_OK;

        for (int i = 0; i < fNPorts; i++)  {
            /* Allocate en/decoders */
            fOpusMode[i] = opus_custom_mode_create(params->fSampleRate, params->fPeriodSize, &error);
            if (error != OPUS_OK) {
                jack_log("NetOpusAudioBuffer opus_custom_mode_create err = %d", error);
                goto error;
            }

            fOpusEncoder[i] = opus_custom_encoder_create(fOpusMode[i], 1, &error);
            if (error != OPUS_OK) {
                jack_log("NetOpusAudioBuffer opus_custom_encoder_create err = %d", error);
                goto error;
            }

            fOpusDecoder[i] = opus_custom_decoder_create(fOpusMode[i], 1, &error);
            if (error != OPUS_OK) {
                jack_log("NetOpusAudioBuffer opus_custom_decoder_create err = %d", error);
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
                fOpusEncoder[i] = 0;
            }
            if (fOpusDecoder[i]) {
                opus_custom_decoder_destroy(fOpusDecoder[i]);
                fOpusDecoder[i] = 0;
            }
            if (fOpusMode[i]) {
                opus_custom_mode_destroy(fOpusMode[i]);
                fOpusMode[i] = 0;
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

    int NetOpusAudioBuffer::RenderFromJackPorts(int nframes)
    {
        float buffer[BUFFER_SIZE_MAX];

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                memcpy(buffer, fPortBuffer[port_index], fPeriodSize * sizeof(sample_t));
            } else {
                memset(buffer, 0, fPeriodSize * sizeof(sample_t));
            }
            int res = opus_custom_encode_float(fOpusEncoder[port_index], buffer, ((nframes == -1) ? fPeriodSize : nframes), fCompressedBuffer[port_index], fCompressedMaxSizeByte);
            if (res < 0 || res >= 65535) {
                jack_error("opus_custom_encode_float error res = %d", res);
                fCompressedSizesByte[port_index] = 0;
            } else {
                fCompressedSizesByte[port_index] = res;
            }
        }

        // All ports active
        return fNPorts;
    }

    void NetOpusAudioBuffer::RenderToJackPorts(int nframes)
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                int res = opus_custom_decode_float(fOpusDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizesByte[port_index], fPortBuffer[port_index], ((nframes == -1) ? fPeriodSize : nframes));
                if (res < 0 || res != ((nframes == -1) ? fPeriodSize : nframes)) {
                    jack_error("opus_custom_decode_float error fCompressedSizeByte = %d res = %d", fCompressedSizesByte[port_index], res);
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
}
