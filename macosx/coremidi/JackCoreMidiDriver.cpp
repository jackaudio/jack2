/*
Copyright (C) 2009 Grame

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

#include "JackCoreMidiDriver.h"
#include "JackGraphManager.h"
#include "JackServer.h"
#include "JackEngineControl.h"
#include "JackDriverLoader.h"

#include <mach/mach_time.h>
#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>

namespace Jack
{

static MIDITimeStamp MIDIGetCurrentHostTime()
{
	return mach_absolute_time();
}

void JackCoreMidiDriver::ReadProcAux(const MIDIPacketList *pktlist, jack_ringbuffer_t* ringbuffer)
{
    // Write the number of packets
    size_t size = jack_ringbuffer_write_space(ringbuffer);
    if (size < sizeof(UInt32)) {
        jack_error("ReadProc : ring buffer is full, skip events...");
        return;
    }

    jack_ringbuffer_write(ringbuffer, (char*)&pktlist->numPackets, sizeof(UInt32));

    for (unsigned int i = 0; i < pktlist->numPackets; ++i) {

        MIDIPacket *packet = (MIDIPacket *)pktlist->packet;

        // TODO : use timestamp

        // Check available size first..
        size = jack_ringbuffer_write_space(ringbuffer);
        if (size < (sizeof(UInt16) + packet->length)) {
           jack_error("ReadProc : ring buffer is full, skip events...");
           return;
        }
        // Write length of each packet first
        jack_ringbuffer_write(ringbuffer, (char*)&packet->length, sizeof(UInt16));
        // Write event actual data
        jack_ringbuffer_write(ringbuffer, (char*)packet->data, packet->length);

        packet = MIDIPacketNext(packet);
    }
}

void JackCoreMidiDriver::ReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    jack_ringbuffer_t* ringbuffer = (jack_ringbuffer_t*)connRefCon;
    ReadProcAux(pktlist, ringbuffer);
}

void JackCoreMidiDriver::ReadVirtualProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon)
{
    jack_ringbuffer_t* ringbuffer = (jack_ringbuffer_t*)refCon;
    ReadProcAux(pktlist, ringbuffer);
}

void JackCoreMidiDriver::NotifyProc(const MIDINotification *message, void *refCon)
{
	jack_log("NotifyProc %d", message->messageID);
}

JackCoreMidiDriver::JackCoreMidiDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
    : JackMidiDriver(name, alias, engine, table), fMidiClient(NULL), fInputPort(NULL), fOutputPort(NULL), fRealCaptureChannels(0), fRealPlaybackChannels(0)
{}

JackCoreMidiDriver::~JackCoreMidiDriver()
{}

int JackCoreMidiDriver::Open(bool capturing,
                             bool playing,
                             int inchannels,
                             int outchannels,
                             bool monitor,
                             const char* capture_driver_name,
                             const char* playback_driver_name,
                             jack_nframes_t capture_latency,
                             jack_nframes_t playback_latency)
                    {
    OSStatus err;
	CFStringRef coutputStr;
	std::string str;

    // Get real input/output number
    fRealCaptureChannels = MIDIGetNumberOfSources();
    fRealPlaybackChannels = MIDIGetNumberOfDestinations();

    // Generic JackMidiDriver Open
    if (JackMidiDriver::Open(capturing, playing, inchannels + fRealCaptureChannels, outchannels + fRealPlaybackChannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency) != 0)
        return -1;

    coutputStr = CFStringCreateWithCString(0, "JackMidi", CFStringGetSystemEncoding());
	err = MIDIClientCreate(coutputStr, NotifyProc, this, &fMidiClient);
	CFRelease(coutputStr);
	if (!fMidiClient) {
        jack_error("Cannot create CoreMidi client");
		goto error;
	}

   	err = MIDIInputPortCreate(fMidiClient, CFSTR("Input port"), ReadProc, this, &fInputPort);
   	if (!fInputPort) {
		jack_error("Cannot open CoreMidi in port\n");
		goto error;
	}

	err = MIDIOutputPortCreate(fMidiClient, CFSTR("Output port"), &fOutputPort);
	if (!fOutputPort) {
		jack_error("Cannot open CoreMidi out port\n");
		goto error;
	}

    fMidiDestination = new MIDIEndpointRef[inchannels + fRealCaptureChannels];
    assert(fMidiDestination);

    // Virtual input
    for (int i = 0; i < inchannels; i++)  {
        std::stringstream num;
        num << i;
        str = "JackMidi" + num.str();
        coutputStr = CFStringCreateWithCString(0, str.c_str(), CFStringGetSystemEncoding());
        err = MIDIDestinationCreate(fMidiClient, coutputStr, ReadVirtualProc, fRingBuffer[i], &fMidiDestination[i]);
        CFRelease(coutputStr);
        if (!fMidiDestination[i]) {
            jack_error("Cannot create CoreMidi destination");
            goto error;
        }
    }

    // Real input
    for (int i = 0; i < fRealCaptureChannels; i++)  {
        fMidiDestination[i + inchannels] = MIDIGetSource(i);
        MIDIPortConnectSource(fInputPort, fMidiDestination[i + inchannels], fRingBuffer[i + inchannels]);
    }

    fMidiSource = new MIDIEndpointRef[outchannels + fRealPlaybackChannels];
    assert(fMidiSource);

    // Virtual output
    for (int i = 0; i < outchannels; i++)  {
        std::stringstream num;
        num << i;
        str = "JackMidi" + num.str();
        coutputStr = CFStringCreateWithCString(0, str.c_str(), CFStringGetSystemEncoding());
        err = MIDISourceCreate(fMidiClient, coutputStr, &fMidiSource[i]);
        CFRelease(coutputStr);
        if (!fMidiSource[i]) {
            jack_error("Cannot create CoreMidi source");
            goto error;
        }
    }

     // Real output
    for (int i = 0; i < fRealPlaybackChannels; i++)  {
        fMidiSource[i + outchannels] = MIDIGetDestination(i);
    }

    return 0;

error:
    Close();
	return -1;
}

int JackCoreMidiDriver::Close()
{
    // Generic midi driver close
    int res = JackMidiDriver::Close();

    if (fInputPort)
		 MIDIPortDispose(fInputPort);

    if (fOutputPort)
		MIDIPortDispose(fOutputPort);

    // Only dispose "virtual" endpoints
    for (int i = 0; i < fCaptureChannels - fRealCaptureChannels; i++)  {
        if (fMidiDestination)
            MIDIEndpointDispose(fMidiDestination[i]);
    }
    delete[] fMidiDestination;

    // Only dispose "virtual" endpoints
    for (int i = 0; i < fPlaybackChannels - fRealPlaybackChannels; i++)  {
        if (fMidiSource[i])
            MIDIEndpointDispose(fMidiSource[i]);
    }
    delete[] fMidiSource;

	if (fMidiClient)
        MIDIClientDispose(fMidiClient);

    return res;
}

int JackCoreMidiDriver::Attach()
{
    OSStatus err;
    JackPort* port;
    CFStringRef pname;
    jack_port_id_t port_index;
    char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char endpoint_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    int i;

    jack_log("JackCoreMidiDriver::Attach fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (i = 0; i < fCaptureChannels; i++) {

        err = MIDIObjectGetStringProperty(fMidiDestination[i], kMIDIPropertyName, &pname);
        if (err == noErr) {
            CFStringGetCString(pname, endpoint_name, sizeof(endpoint_name), 0);
            CFRelease(pname);
            snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, endpoint_name, i + 1);
        } else {
            snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, i + 1);
        }

        snprintf(name, sizeof(name) - 1, "%s:capture_%d", fClientControl.fName, i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE, CaptureDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fCapturePortList[i] = port_index;
        jack_log("JackCoreMidiDriver::Attach fCapturePortList[i] port_index = %ld", port_index);
    }

    for (i = 0; i < fPlaybackChannels; i++) {

        err = MIDIObjectGetStringProperty(fMidiSource[i], kMIDIPropertyName, &pname);
        if (err == noErr) {
            CFStringGetCString(pname, endpoint_name, sizeof(endpoint_name), 0);
            CFRelease(pname);
            snprintf(alias, sizeof(alias) - 1, "%s:%s:in%d", fAliasName, endpoint_name, i + 1);
        } else {
            snprintf(alias, sizeof(alias) - 1, "%s:%s:in%d", fAliasName, fPlaybackDriverName, i + 1);
        }

        snprintf(name, sizeof(name) - 1, "%s:playback_%d", fClientControl.fName, i + 1);
        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE, PlaybackDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fPlaybackPortList[i] = port_index;
        jack_log("JackCoreMidiDriver::Attach fPlaybackPortList[i] port_index = %ld", port_index);
    }

    return 0;
}
int JackCoreMidiDriver::Read()
{
    for (int chan = 0; chan < fCaptureChannels; chan++)  {

        if (fGraphManager->GetConnectionsNum(fCapturePortList[chan]) > 0) {

            // Get JACK port
            JackMidiBuffer* midi_buffer = GetInputBuffer(chan);

            if (jack_ringbuffer_read_space(fRingBuffer[chan]) == 0) {
                // Reset buffer
                midi_buffer->Reset(midi_buffer->nframes);
            } else {

                while (jack_ringbuffer_read_space(fRingBuffer[chan]) > 0) {

                    // Read event number
                    int ev_count = 0;
                    jack_ringbuffer_read(fRingBuffer[chan], (char*)&ev_count, sizeof(int));

                    for (int j = 0; j < ev_count; j++)  {
                        // Read event length
                        UInt16 event_len;
                        jack_ringbuffer_read(fRingBuffer[chan], (char*)&event_len, sizeof(UInt16));
                        // Read event actual data
                        jack_midi_data_t* dest = midi_buffer->ReserveEvent(0, event_len);
                        jack_ringbuffer_read(fRingBuffer[chan], (char*)dest, event_len);
                    }
                }
            }

        } else {
            // Consume ring buffer
            jack_ringbuffer_read_advance(fRingBuffer[chan], jack_ringbuffer_read_space(fRingBuffer[chan]));
        }
    }
    return 0;
}

int JackCoreMidiDriver::Write()
{
    MIDIPacketList* pktlist = (MIDIPacketList*)fMIDIBuffer;

    for (int chan = 0; chan < fPlaybackChannels; chan++)  {

         if (fGraphManager->GetConnectionsNum(fPlaybackPortList[chan]) > 0) {

            MIDIPacket* packet = MIDIPacketListInit(pktlist);
            JackMidiBuffer* midi_buffer = GetOutputBuffer(chan);

            // TODO : use timestamp

            for (unsigned int j = 0; j < midi_buffer->event_count; j++) {
                JackMidiEvent* ev = &midi_buffer->events[j];
                packet = MIDIPacketListAdd(pktlist, sizeof(fMIDIBuffer), packet, MIDIGetCurrentHostTime(), ev->size, ev->GetData(midi_buffer));
            }

            if (packet) {
                if (chan < fPlaybackChannels - fRealPlaybackChannels) {
                    OSStatus err = MIDIReceived(fMidiSource[chan], pktlist);
                    if (err != noErr)
                        jack_error("MIDIReceived error");
                } else {
                    OSStatus err = MIDISend(fOutputPort, fMidiSource[chan], pktlist);
                    if (err != noErr)
                        jack_error("MIDISend error");
                }
            }
        }
    }

    return 0;
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor()
    {
        jack_driver_desc_t * desc;
        unsigned int i;

        desc = (jack_driver_desc_t*)calloc (1, sizeof (jack_driver_desc_t));
        strcpy(desc->name, "coremidi");                                     // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "Apple CoreMIDI API based MIDI backend");      // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 2;
        desc->params = (jack_driver_param_desc_t*)calloc (desc->nparams, sizeof (jack_driver_param_desc_t));

        i = 0;
        strcpy(desc->params[i].name, "inchannels");
        desc->params[i].character = 'i';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "CoreMIDI virtual bus");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        i++;
        strcpy(desc->params[i].name, "outchannels");
        desc->params[i].character = 'o';
        desc->params[i].type = JackDriverParamInt;
        desc->params[i].value.ui = 0;
        strcpy(desc->params[i].short_desc, "CoreMIDI virtual bus");
        strcpy(desc->params[i].long_desc, desc->params[i].short_desc);

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        const JSList * node;
        const jack_driver_param_t * param;
        int virtual_in = 0;
        int virtual_out = 0;

        for (node = params; node; node = jack_slist_next (node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'i':
                    virtual_in = param->value.ui;
                    break;

                case 'o':
                    virtual_out = param->value.ui;
                    break;
                }
        }

        Jack::JackDriverClientInterface* driver = new Jack::JackCoreMidiDriver("system_midi", "coremidi", engine, table);
        if (driver->Open(1, 1, virtual_in, virtual_out, false, "in", "out", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif

