/*
Copyright (C) 2008-2011 Torben Horn

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



/*
 * "enp4s0"
 */
namespace Jack
{
JackAVBPDriver::JackAVBPDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table,
                                       char* stream_id, char* destination_mac, char* eth_dev,
                                       int sample_rate, int period_size, int num_periods)
    : JackWaiterDriver(name, alias, engine, table)
{
    jack_log("JackAVBPDriver::JackAVBPDriver Ethernet Device %s", eth_dev);

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

    printf("JackAVBPDriver::JackAVBPDriver Ethernet Device %s\n", eth_dev);
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

    init_1722_driver( &(this->ieee1722mc),
                  eth_dev,
                  stream_id,
                  destination_mac,
                  sample_rate,
                  period_size,
                  num_periods
                 );

                 this->ieee1722mc.period_usecs = period_size / sample_rate * 1000000;
}

JackAVBPDriver::~JackAVBPDriver()
{
    // No destructor yet.
}

//open, close, attach and detach------------------------------------------------------

int JackAVBPDriver::Close()
{
    // Generic audio driver close
    int res = JackWaiterDriver::Close();

    FreePorts();
    shutdown_1722_driver(&ieee1722mc);
    return res;
}


int JackAVBPDriver::AllocPorts()
{
    jack_port_id_t port_index;
    char buf[64];

    snprintf (buf, sizeof(buf) - 1, "system:capture_1");


    if (fEngine->PortRegister(fClientControl.fRefNum, buf, JACK_DEFAULT_AUDIO_TYPE,
                                    CaptureDriverFlags, fEngineControl->fBufferSize, &port_index) < 0) {
        jack_error("driver: cannot register port for %s", buf);
        return -1;
    }
    //port = fGraphManager->GetPort(port_index);


    ieee1722mc.capture_ports = jack_slist_append (ieee1722mc.capture_ports, (void *)(intptr_t)port_index);

    return 0;
}

//init and restart--------------------------------------------------------------------
bool JackAVBPDriver::Initialize()
{
    jack_log("JackAVBDriver::Init");

    FreePorts();


    //display some additional infos
    printf("AVB IEEE1722 AVTP driver started\n");

    if (startup_1722_driver(&ieee1722mc)) {

        return false;
    }

    //register jack ports
    if (AllocPorts() != 0) {
        jack_error("Can't allocate ports.");
        return false;
    }

    //monitor
    //driver parametering
    JackTimedDriver::SetBufferSize(ieee1722mc.period_size);
    JackTimedDriver::SetSampleRate(ieee1722mc.sample_rate);

    JackDriver::NotifyBufferSize(ieee1722mc.period_size);
    JackDriver::NotifySampleRate(ieee1722mc.sample_rate);

    return true;
}


//jack ports and buffers--------------------------------------------------------------

//driver processes--------------------------------------------------------------------

int JackAVBPDriver::Read()
{
    int ret = 0;
    JSList *node = ieee1722mc.capture_ports;


    /*
     *
     *      Even Odd Calc will cause xruns. cumulative_delay_ns must be calculated sample accurate in mediaclock listener
     *
     */
    //num_packets_even_odd ? num_packets_even_odd = 0 : num_packets_even_odd = 1;  // even = 0, odd = 1
    int num_packets = (int)( ieee1722mc.period_size / 6 ) + 1; // + num_packets_even_odd;


    uint64_t cumulative_ipg_ns = 0;

    for(int n=0; n<num_packets; n++){
        cumulative_ipg_ns += wait_recv_1722_mediaclockstream( &ieee1722mc );
    }


    jack_log("ipg: %lld ns", cumulative_ipg_ns );
    float cumulative_ipg_us = cumulative_ipg_ns / 1000;
    if ( cumulative_ipg_us > ieee1722mc.period_usecs) {
        ret = 1;
        NotifyXRun(fBeginDateUst, cumulative_ipg_us);
        jack_error("netxruns... duration: %fms", cumulative_ipg_us / 1000);
    }

    JackDriver::CycleTakeBeginTime();

    if ( ret ) {
        return -1;
    }

    while (node != NULL) {
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        JackPort *port = fGraphManager->GetPort(port_index);
        jack_default_audio_sample_t* buf = (jack_default_audio_sample_t*)fGraphManager->GetBuffer(port_index, fEngineControl->fBufferSize);
        memcpy(buf, 0, ieee1722mc.period_size * sizeof(jack_default_audio_sample_t));
        node = jack_slist_next (node);
    }

    return 0;
}

int JackAVBPDriver::Write()
{
    return 0;
}

void
JackAVBPDriver::FreePorts ()
{
    JSList *node = ieee1722mc.capture_ports;

    while (node != NULL) {
        JSList *this_node = node;
        jack_port_id_t port_index = (jack_port_id_t)(intptr_t) node->data;
        node = jack_slist_remove_link(node, this_node);
        jack_slist_free_1(this_node);
        fEngine->PortUnRegister(fClientControl.fRefNum, port_index);
    }
    ieee1722mc.capture_ports = NULL;

}

//driver loader-----------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif

    inline int argumentsSplitDelimiters(char* inputString, char* outputArray, int array_len)
    {
        int tokenCnt=0;
        char *token;
        char *der_string = strdup(inputString);

        for(int m=0;m<array_len;m++){
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

        value.ui = 1U;
        jack_driver_descriptor_add_parameter(desc, &filler, "audio-ins", 'i', JackDriverParamUInt, &value, NULL, "Number of capture channels (defaults to 2)", NULL);

        value.ui = 48000U;
        jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

        value.ui = 64U;
        jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

        value.ui = 1U;
        jack_driver_descriptor_add_parameter(desc, &filler, "num-periods", 'n', JackDriverParamUInt, &value, NULL, "Network latency setting in no. of periods", NULL);

        sprintf( value.str, "enp4s0");
        jack_driver_descriptor_add_parameter(desc, &filler, "ethernet-dev", 'e', JackDriverParamString, &value, NULL, "AVB Ethernet Device", NULL);

        sprintf( value.str, "00:22:97:00:41:2c:00:00");
        jack_driver_descriptor_add_parameter(desc, &filler, "stream-id", 's', JackDriverParamString, &value, NULL, "Stream ID for listening", NULL);

        sprintf( value.str, "91:e0:f0:11:11:11");
        jack_driver_descriptor_add_parameter(desc, &filler, "destination-mac", 'm', JackDriverParamString, &value, NULL, "Multicast Destination MAC Address for listening", NULL);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        jack_nframes_t sample_rate = 48000;
        jack_nframes_t period_size = 64;
        unsigned int capture_ports = 1;
        int num_periods = 2;
        char sid[8];
        char dmac[6];
        char eth_dev[32];


        int dont_htonl_floats = 0;
        int always_deadline = 0;
        int jitter_val = 0;
        const JSList * node;
        const jack_driver_param_t * param;

        printf("foo bar\n");fflush(stdout);

        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t*) node->data;
            switch (param->character) {
                case 'i':
                    capture_ports = param->value.ui;
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




                    printf("Stream ID: %s %02x %02x %02x %02x %02x %02x \n", param->value.str,
                                                            dmac[0], dmac[1], dmac[2], dmac[3], dmac[4], dmac[5]);fflush(stdout);
                    break;

            }
        }

        try {
            Jack::JackDriverClientInterface* driver = new Jack::JackWaitThreadedDriver (
                new Jack::JackAVBPDriver("system", "avb_mc", engine, table, sid, dmac, eth_dev,
                                             sample_rate, period_size, num_periods));

            if (driver->Open(period_size, sample_rate, 1, 1, capture_ports, 0,
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
