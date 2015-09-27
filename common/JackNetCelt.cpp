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

#include "JackNetCelt.h"
#include "JackError.h"

namespace Jack
{
    #define KPS 32
    #define KPS_DIV 8

    NetCeltAudioBuffer::NetCeltAudioBuffer(session_params_t* params, uint32_t nports, char* net_buffer, int kbps)
        :NetAudioBuffer(params, nports, net_buffer)
    {
        fCeltMode = new CELTMode*[fNPorts];
        fCeltEncoder = new CELTEncoder*[fNPorts];
        fCeltDecoder = new CELTDecoder*[fNPorts];

        memset(fCeltMode, 0, fNPorts * sizeof(CELTMode*));
        memset(fCeltEncoder, 0, fNPorts * sizeof(CELTEncoder*));
        memset(fCeltDecoder, 0, fNPorts * sizeof(CELTDecoder*));

        int error = CELT_OK;

        for (int i = 0; i < fNPorts; i++)  {
            fCeltMode[i] = celt_mode_create(params->fSampleRate, params->fPeriodSize, &error);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_mode_create err = %d", error);
                goto error;
            }

    #if HAVE_CELT_API_0_11

            fCeltEncoder[i] = celt_encoder_create_custom(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_encoder_create_custom err = %d", error);
                goto error;
            }
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create_custom(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_decoder_create_custom err = %d", error);
                goto error;
            }
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #elif HAVE_CELT_API_0_7 || HAVE_CELT_API_0_8

            fCeltEncoder[i] = celt_encoder_create(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_mode_create err = %d", error);
                goto error;
            }
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create(fCeltMode[i], 1, &error);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_decoder_create err = %d", error);
                goto error;
            }
            celt_decoder_ctl(fCeltDecoder[i], CELT_SET_COMPLEXITY(1));

    #else

            fCeltEncoder[i] = celt_encoder_create(fCeltMode[i]);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_encoder_create err = %d", error);
                goto error;
            }
            celt_encoder_ctl(fCeltEncoder[i], CELT_SET_COMPLEXITY(1));

            fCeltDecoder[i] = celt_decoder_create(fCeltMode[i]);
            if (error != CELT_OK) {
                jack_log("NetCeltAudioBuffer celt_decoder_create err = %d", error);
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

    int NetCeltAudioBuffer::RenderFromJackPorts(int nframes)
    {
        float buffer[BUFFER_SIZE_MAX];

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
                memcpy(buffer, fPortBuffer[port_index], fPeriodSize * sizeof(sample_t));
            } else {
                memset(buffer, 0, fPeriodSize * sizeof(sample_t));
            }
        #if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
            //int res = celt_encode_float(fCeltEncoder[port_index], buffer, fPeriodSize, fCompressedBuffer[port_index], fCompressedSizeByte);
            int res = celt_encode_float(fCeltEncoder[port_index], buffer, nframes, fCompressedBuffer[port_index], fCompressedSizeByte);
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

    void NetCeltAudioBuffer::RenderToJackPorts(int nframes)
    {
        for (int port_index = 0; port_index < fNPorts; port_index++) {
            if (fPortBuffer[port_index]) {
            #if HAVE_CELT_API_0_8 || HAVE_CELT_API_0_11
                //int res = celt_decode_float(fCeltDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizeByte, fPortBuffer[port_index], fPeriodSize);
                int res = celt_decode_float(fCeltDecoder[port_index], fCompressedBuffer[port_index], fCompressedSizeByte, fPortBuffer[port_index], nframes);
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

        if (port_num > 0) {

            int sub_period_bytes_size;

            // Last packet of the cycle
            if (sub_cycle == fNumPackets - 1) {
                sub_period_bytes_size = fLastSubPeriodBytesSize;
            } else {
                sub_period_bytes_size = fSubPeriodBytesSize;
            }

            for (int port_index = 0; port_index < fNPorts; port_index++) {
                memcpy(fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, fNetBuffer + port_index * sub_period_bytes_size, sub_period_bytes_size);
            }
        }

        return CheckPacket(cycle, sub_cycle);
    }

    int NetCeltAudioBuffer::RenderToNetwork(int sub_cycle, uint32_t port_num)
    {
        int sub_period_bytes_size;

        // Last packet of the cycle
        if (sub_cycle == fNumPackets - 1) {
            sub_period_bytes_size = fLastSubPeriodBytesSize;
        } else {
            sub_period_bytes_size = fSubPeriodBytesSize;
        }

        for (int port_index = 0; port_index < fNPorts; port_index++) {
            memcpy(fNetBuffer + port_index * sub_period_bytes_size, fCompressedBuffer[port_index] + sub_cycle * fSubPeriodBytesSize, sub_period_bytes_size);
        }
        return fNPorts * sub_period_bytes_size;
    }
}
