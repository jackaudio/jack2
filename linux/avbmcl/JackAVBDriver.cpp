/*
Copyright (C) 2016-2019 Christoph Kuhr

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

#include "JackAVBDriver.h"
#include "JackEngineControl.h"
#include "JackLockedEngine.h"
#include "JackGraphManager.h"
#include "JackWaitThreadedDriver.h"
#include "JackTools.h"
#include "driver_interface.h"

#define MIN(x,y) ((x)<(y) ? (x) : (y))

using namespace std;

namespace Jack
{
JackAVBDriver::JackAVBDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                       char* stream_id, char* destination_mac, char* eth_dev,
                                       int sample_rate, int period_size, int adjust,
                                       int num_periods, int capture_ports, int playback_ports)
    : JackWaiterDriver(name, alias, engine, table)
{
    jack_log("JackAVBDriver::JackAVBPDriver Ethernet Device %s", eth_dev);
    jack_log("Stream ID: %02x %02x %02x %02x %02x %02x %02x %02x",
                                        (uint8_t) stream_id[0],
                                        (uint8_t) stream_id[1],
                                        (uint8_t) stream_id[2],
                                        (uint8_t) stream_id[3],
                                        (uint8_t) stream_id[4],
                                        (uint8_t) stream_id[5],
                                        (uint8_t) stream_id[6],
                                        (uint8_t) stream_id[7]);
    jack_log("Destination MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
                                        (uint8_t) destination_mac[0],
                                        (uint8_t) destination_mac[1],
                                        (uint8_t) destination_mac[2],
                                        (uint8_t) destination_mac[3],
                                        (uint8_t) destination_mac[4],
                                        (uint8_t) destination_mac[5]);
    printf("JackAVBDriver::JackAVBPDriver Ethernet Device %s\n", eth_dev);
    printf("Stream ID: %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                        (uint8_t) stream_id[0],
                                        (uint8_t) stream_id[1],
                                        (uint8_t) stream_id[2],
                                        (uint8_t) stream_id[3],
                                        (uint8_t) stream_id[4],
                                        (uint8_t) stream_id[5],
                                        (uint8_t) stream_id[6],
                                        (uint8_t) stream_id[7]);
    printf("Destination MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                        (uint8_t) destination_mac[0],
                                        (uint8_t) destination_mac[1],
                                        (uint8_t) destination_mac[2],
                                        (uint8_t) destination_mac[3],
                                        (uint8_t) destination_mac[4],
                                        (uint8_t) destination_mac[5]);
    num_packets_even_odd = 0; // even = 0, odd = 1
    lastPeriodDuration = 0;
    timeCompensation = 0;
    monotonicTime = 0;
    preRunCnt = 3;

    init_avb_driver( &(this->avb_ctx),
                      eth_dev,
                      stream_id,
                      destination_mac,
                      sample_rate,
                      period_size,
                      num_periods,
                      adjust,
                      capture_ports,
                      playback_ports
                     );
}

JackAVBDriver::~JackAVBDriver()
{
    // No destructor yet.
}

int JackAVBDriver::Close()
{
    // Generic audio driver close
    int res = JackWaiterDriver::Close();
    FreePorts();
    shutdown_avb_driver(&avb_ctx);
    return res;
}

int JackAVBDriver::AllocPorts()
{
    jack_port_id_t port_index;
    char buf[64];
    int chn = 0;

    for (chn = 0; chn < (int)avb_ctx.capture_channels; chn++) {
        memset(buf, 0, sizeof(buf));
        snprintf (buf, sizeof(buf) - 1, "system:capture_%u", chn + 1);
        if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
                                        CaptureDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", buf);
            return -1;
        }

        avb_ctx.capture_ports = jack_slist_append (avb_ctx.capture_ports, (void *)(intptr_t)port_index);
    }

    for (chn = 0; chn < (int)avb_ctx.playback_channels; chn++) {
        memset(buf, 0, sizeof(buf));
        snprintf (buf, sizeof(buf) - 1, "system:playback_%u", chn + 1);

        if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
                                        PlaybackDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
            jack_error("driver: cannot register port for %s", buf);
            return -1;
        }

        avb_ctx.playback_ports = jack_slist_append (avb_ctx.playback_ports, (void *)(intptr_t)port_index);
    }
    //port = fGraphManager->GetPort(port_index);
    return 0;
}

bool JackAVBDriver::Initialize()
{
    jack_log("JackAVBDriver::Init");
    FreePorts();

    //display some additional infos
    printf("AVB driver started\n");

    if (startup_avb_driver(&avb_ctx)) {
        return false;
    }

    //register jack ports
    if (AllocPorts() != 0) {
        jack_error("Can't allocate ports.");
        return false;
    }

    //driver parametering
    JackTimedDriver::SetBufferSize(avb_ctx.period_size);
    JackTimedDriver::SetSampleRate(avb_ctx.sample_rate);

    JackDriver::NotifyBufferSize(avb_ctx.period_size);
    JackDriver::NotifySampleRate(avb_ctx.sample_rate);

    return true;
}

int JackAVBDriver::Read()
{
    int ret = 0;
    JSList *node = avb_ctx.capture_ports;
    uint64_t cumulative_rx_int_ns = 0;
    uint64_t lateness = 0;
    int n = 0;

    for(n=0; n<avb_ctx.num_packets; n++){
        cumulative_rx_int_ns += await_avtp_rx_ts( &avb_ctx, n, &lateness );
/*        
        if( n == 0 && --this->preRunCnt >= 0 ){
            cumulative_rx_int_ns -= this->timeCompensation;
        }*/
        //jack_errors("duration: %lld", cumulative_rx_int_ns);
    }
    this->monotonicTime += cumulative_rx_int_ns;
/*
    if( this->lastPeriodDuration != 0 ){
        this->timeCompensation = cumulative_rx_int_ns - this->lastPeriodDuration;
    }
    this->lastPeriodDuration = cumulative_rx_int_ns;
*/

    float cumulative_rx_int_us = (cumulative_rx_int_ns / 1000) - 0.5;
    if ( cumulative_rx_int_us > avb_ctx.period_usecs ) {
        ret = 1;
        NotifyXRun(fBeginDateUst, cumulative_rx_int_us);
        jack_error("avtp_xruns... duration: %.2f ms", lateness / 1000000);
    }

    JackDriver::CycleTakeBeginTime();

    if ( ret ) return -1;

    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);
        jack_default_audio_sample_t* buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);
        //memcpy(buf, 0, avb_ctx.period_size * sizeof(jack_default_audio_sample_t));
        node = jack_slist_next (node);
    }
    return 0;
}

int JackAVBDriver::Write()
{
    JSList *node = avb_ctx.playback_ports;
    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);
        jack_default_audio_sample_t* buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);
        //memcpy(buf, 0, avb_ctx.period_size * sizeof(jack_default_audio_sample_t));
        node = jack_slist_next (node);
    }
    return 0;
}

void JackAVBDriver::FreePorts ()
{
    JSList *node = avb_ctx.capture_ports;

    while (node != NULL) {
        JSList *this_node = node;
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        node = jack_slist_remove_link(node, this_node);
        jack_slist_free_1(this_node);
        fEngine->PortUnRegister(fClientControl.fRefNum, port_index);
    }
    avb_ctx.capture_ports = NULL;
    node = avb_ctx.playback_ports;

    while (node != NULL) {
        JSList *this_node = node;
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        node = jack_slist_remove_link(node, this_node);
        jack_slist_free_1(this_node);
        fEngine->PortUnRegister(fClientControl.fRefNum, port_index);
    }
    avb_ctx.playback_ports = NULL;
}

#ifdef __cplusplus
extern "C"
{
#endif

    inline int argumentsSplitDelimiters(char* inputString, char* outputArray, int array_len)
    {
        int tokenCnt=0;
        char *token;
        char *der_string = strdup(inputString);
        int m = 0;
        for( m=0;m<array_len;m++){
            if(( token = strsep(&der_string, ":")) != NULL ){
                outputArray[m] = (char)strtol(strdup(token), NULL, 16);       // number base 16
            } else {
                tokenCnt = m;
                break;
            }
        }
        free(der_string);
        return tokenCnt;
    }

    SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor ()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("avb", JackDriverMaster, "IEEE 1722 AVTP slave backend component", &filler);

        value.ui = 2U;
        jack_driver_descriptor_add_parameter(desc, &filler, "audio-ins", 'i', JackDriverParamUInt, &value, NULL, "Number of capture channels (defaults to 1)", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "audio-outs", 'o', JackDriverParamUInt, &value, NULL, "Number of playback channels (defaults to 1)", NULL);

        value.ui = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui = 64U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 1U;
        jack_driver_descriptor_add_parameter(desc, &filler, "num-periods", 'n', JackDriverParamUInt, &value, NULL, "Network latency setting in no. of periods", NULL);

        value.ui = 0U;
        jack_driver_descriptor_add_parameter(desc, &filler, "adjust", 'a', JackDriverParamUInt, &value, NULL, "Adjust Timestamps", NULL);

        sprintf( value.str, "enp4s0");
        jack_driver_descriptor_add_parameter(desc, &filler, "eth-dev", 'e', JackDriverParamString, &value, NULL, "AVB Ethernet Device", NULL);

        sprintf( value.str, "00:22:97:00:41:2c:00:00");
        jack_driver_descriptor_add_parameter(desc, &filler, "stream-id", 's', JackDriverParamString, &value, NULL, "Stream ID for listening", NULL);

        sprintf( value.str, "91:e0:f0:11:11:11");
        jack_driver_descriptor_add_parameter(desc, &filler, "dst-mac", 'm', JackDriverParamString, &value, NULL, "Multicast Destination MAC Address for listening", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        unsigned int sample_rate = 48000;
        jack_nframes_t period_size = 64;
        unsigned int capture_ports = 2;
        unsigned int playback_ports = 2;
        int num_periods = 2;
        int adjust = 0;
        char sid[8];
        char dmac[6];
        char eth_dev[32];
        const JSList * node;
        const jack_driver_param_t * param;

        printf("foo bar\n");fflush(stdout);

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t*) node->data;
            switch (param->character) {
                case 'i':
                    capture_ports = param->value.ui;
                    break;
                case 'o':
                    playback_ports = param->value.ui;
                    break;
                case 'r':
                    sample_rate = param->value.ui;
                    break;
                case 'p':
                    period_size = param->value.ui;
                    break;
                case 'n':
                    num_periods = param->value.ui;
                    break;
                case 'a':
                    adjust = param->value.ui;
                    break;
                case 'e':
                    sprintf(eth_dev, "%s", param->value.str);
                    printf("Eth Dev: %s %s\n", param->value.str, eth_dev);fflush(stdout);
                    break;
                case 's':
                    // split stream ID
                    argumentsSplitDelimiters((char *)param->value.str, sid, 8);
                    printf("Stream ID: %s %02x %02x %02x %02x %02x %02x %02x %02x \n", param->value.str,
                                        sid[0], sid[1], sid[2], sid[3], sid[4], sid[5], sid[6], sid[7]);fflush(stdout);
                    break;
                case 'm':
                    // split destination mac address
                    argumentsSplitDelimiters((char *)param->value.str, dmac, 6);
                    printf("Destination MAC Address: %s %02x %02x %02x %02x %02x %02x \n", param->value.str,
                                        dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);fflush(stdout);
                    break;
            }
        }

        try {
            Jack::JackDriverClientInterface* driver = new Jack::JackWaitThreadedDriver (
                new Jack::JackAVBDriver("system", "avb_mc", engine, table, sid, dmac, eth_dev,
                                             sample_rate, period_size, num_periods,
                                             adjust, capture_ports, playback_ports));

            if (driver->Open(period_size, sample_rate, 1, 1, capture_ports, playback_ports,
                                0, "from_master", "to_master", 0, 0) == 0) {
                return driver;
            } else {
                delete driver;
                return NULL;
            }

        } catch (...) {
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
}
