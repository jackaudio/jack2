/*
Copyright (C) 2009 Grame
Copyright (C) 2011 Devin Anderson

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

#include <stdexcept>

#include <mach/mach_time.h>

#include "JackCoreMidiDriver.h"
#include "JackCoreMidiUtil.h"
#include "JackEngineControl.h"

using Jack::JackCoreMidiDriver;

///////////////////////////////////////////////////////////////////////////////
// Static callbacks
///////////////////////////////////////////////////////////////////////////////

void
JackCoreMidiDriver::HandleInputEvent(const MIDIPacketList *packet_list,
                                     void *driver, void *port)
{
    ((JackCoreMidiPhysicalInputPort *) port)->ProcessCoreMidi(packet_list);
}

void
JackCoreMidiDriver::HandleNotificationEvent(const MIDINotification *message,
                                            void *driver)
{
    ((JackCoreMidiDriver *) driver)->HandleNotification(message);
}

///////////////////////////////////////////////////////////////////////////////
// Class
///////////////////////////////////////////////////////////////////////////////

JackCoreMidiDriver::JackCoreMidiDriver(const char *name, const char *alias,
                                       JackLockedEngine *engine,
                                       JackSynchro *table):
    JackMidiDriver(name, alias, engine, table)
{
    mach_timebase_info_data_t info;
    kern_return_t result = mach_timebase_info(&info);
    if (result != KERN_SUCCESS) {
        throw std::runtime_error(mach_error_string(result));
    }
    client = 0;
    fCaptureChannels = 0;
    fPlaybackChannels = 0;
    num_physical_inputs = 0;
    num_physical_outputs = 0;
    num_virtual_inputs = 0;
    num_virtual_outputs = 0;
    physical_input_ports = 0;
    physical_output_ports = 0;
    time_ratio = (((double) info.numer) / info.denom) / 1000.0;
    virtual_input_ports = 0;
    virtual_output_ports = 0;
}

JackCoreMidiDriver::~JackCoreMidiDriver()
{
    Stop();
    Close();
}

int
JackCoreMidiDriver::Attach()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    jack_port_id_t index;
    jack_nframes_t latency = buffer_size;
    jack_latency_range_t latency_range;
    const char *name;
    JackPort *port;
    JackCoreMidiPort *port_obj;
    latency_range.max = latency;
    latency_range.min = latency;

    // Physical inputs
    for (int i = 0; i < num_physical_inputs; i++) {
        port_obj = physical_input_ports[i];
        name = port_obj->GetName();
        index = fGraphManager->AllocatePort(fClientControl.fRefNum, name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            CaptureDriverFlags, buffer_size);
        if (index == NO_PORT) {
            jack_error("JackCoreMidiDriver::Attach - cannot register physical "
                       "input port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        port = fGraphManager->GetPort(index);
        port->SetAlias(port_obj->GetAlias());
        port->SetLatencyRange(JackCaptureLatency, &latency_range);
        fCapturePortList[i] = index;
    }

    // Virtual inputs
    for (int i = 0; i < num_virtual_inputs; i++) {
        port_obj = virtual_input_ports[i];
        name = port_obj->GetName();
        index = fGraphManager->AllocatePort(fClientControl.fRefNum, name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            CaptureDriverFlags, buffer_size);
        if (index == NO_PORT) {
            jack_error("JackCoreMidiDriver::Attach - cannot register virtual "
                       "input port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        port = fGraphManager->GetPort(index);
        port->SetAlias(port_obj->GetAlias());
        port->SetLatencyRange(JackCaptureLatency, &latency_range);
        fCapturePortList[num_physical_inputs + i] = index;
    }

    if (! fEngineControl->fSyncMode) {
        latency += buffer_size;
        latency_range.max = latency;
        latency_range.min = latency;
    }

    // Physical outputs
    for (int i = 0; i < num_physical_outputs; i++) {
        port_obj = physical_output_ports[i];
        name = port_obj->GetName();
        index = fGraphManager->AllocatePort(fClientControl.fRefNum, name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            PlaybackDriverFlags, buffer_size);
        if (index == NO_PORT) {
            jack_error("JackCoreMidiDriver::Attach - cannot register physical "
                       "output port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        port = fGraphManager->GetPort(index);
        port->SetAlias(port_obj->GetAlias());
        port->SetLatencyRange(JackPlaybackLatency, &latency_range);
        fPlaybackPortList[i] = index;
    }

    // Virtual outputs
    for (int i = 0; i < num_virtual_outputs; i++) {
        port_obj = virtual_output_ports[i];
        name = port_obj->GetName();
        index = fGraphManager->AllocatePort(fClientControl.fRefNum, name,
                                            JACK_DEFAULT_MIDI_TYPE,
                                            PlaybackDriverFlags, buffer_size);
        if (index == NO_PORT) {
            jack_error("JackCoreMidiDriver::Attach - cannot register virtual "
                       "output port with name '%s'.", name);
            // X: Do we need to deallocate ports?
            return -1;
        }
        port = fGraphManager->GetPort(index);
        port->SetAlias(port_obj->GetAlias());
        port->SetLatencyRange(JackPlaybackLatency, &latency_range);
        fPlaybackPortList[num_physical_outputs + i] = index;
    }

    return 0;
}

int
JackCoreMidiDriver::Close()
{
    int result = 0;
    OSStatus status;
    if (physical_input_ports) {
        for (int i = 0; i < num_physical_inputs; i++) {
            delete physical_input_ports[i];
        }
        delete[] physical_input_ports;
        num_physical_inputs = 0;
        physical_input_ports = 0;
        status = MIDIPortDispose(internal_input);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Close", "MIDIPortDispose",
                            status);
            result = -1;
        }
    }
    if (physical_output_ports) {
        for (int i = 0; i < num_physical_outputs; i++) {
            delete physical_output_ports[i];
        }
        delete[] physical_output_ports;
        num_physical_outputs = 0;
        physical_output_ports = 0;
        status = MIDIPortDispose(internal_output);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Close", "MIDIPortDispose",
                            status);
            result = -1;
        }
    }
    if (virtual_input_ports) {
        for (int i = 0; i < num_virtual_inputs; i++) {
            delete virtual_input_ports[i];
        }
        delete[] virtual_input_ports;
        num_virtual_inputs = 0;
        virtual_input_ports = 0;
    }
    if (virtual_output_ports) {
        for (int i = 0; i < num_virtual_outputs; i++) {
            delete virtual_output_ports[i];
        }
        delete[] virtual_output_ports;
        num_virtual_outputs = 0;
        virtual_output_ports = 0;
    }
    if (client) {
        status = MIDIClientDispose(client);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Close", "MIDIClientDispose",
                            status);
            result = -1;
        }
        client = 0;
    }
    return result;
}

void
JackCoreMidiDriver::HandleNotification(const MIDINotification *message)
{
    // Empty
}

int
JackCoreMidiDriver::Open(bool capturing, bool playing, int in_channels,
                         int out_channels, bool monitor,
                         const char* capture_driver_name,
                         const char* playback_driver_name,
                         jack_nframes_t capture_latency,
                         jack_nframes_t playback_latency)
{
    int pi_count = 0;
    int po_count = 0;
    int vi_count = 0;
    int vo_count = 0;
    ItemCount potential_po_count;
    ItemCount potential_pi_count;

    CFStringRef name = CFStringCreateWithCString(0, "JackMidi",
                                                 CFStringGetSystemEncoding());
    if (! name) {
        jack_error("JackCoreMidiDriver::Open - failed to allocate memory for "
                   "client name string");
        return -1;
    }
    OSStatus status = MIDIClientCreate(name, HandleNotificationEvent, this,
                                       &client);
    CFRelease(name);
    if (status != noErr) {
        WriteMacOSError("JackCoreMidiDriver::Close", "MIDIClientCreate",
                        status);
        return -1;
    }
    char *client_name = fClientControl.fName;
    int port_rt_priority = fEngineControl->fServerPriority + 1;

    // Allocate and connect virtual inputs
    if (in_channels) {
        try {
            virtual_input_ports =
                new JackCoreMidiVirtualInputPort*[in_channels];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating virtual "
                       "input port array: %s", e.what());
            goto destroy_client;
        }
        for (vi_count = 0; vi_count < in_channels; vi_count++) {
            try {
                virtual_input_ports[vi_count] =
                    new JackCoreMidiVirtualInputPort(fAliasName, client_name,
                                                     capture_driver_name,
                                                     vi_count, client,
                                                     time_ratio);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating virtual "
                           "input port: %s", e.what());
                goto destroy_virtual_input_ports;
            }
        }
    }

    // Allocate and connect virtual outputs
    if (out_channels) {
        try {
            virtual_output_ports =
                new JackCoreMidiVirtualOutputPort*[out_channels];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating virtual "
                       "output port array: %s", e.what());
            goto destroy_virtual_input_ports;
        }
        for (vo_count = 0; vo_count < out_channels; vo_count++) {
            try {
                virtual_output_ports[vo_count] =
                    new JackCoreMidiVirtualOutputPort(fAliasName, client_name,
                                                      playback_driver_name,
                                                      vo_count, client,
                                                      time_ratio,
                                                      port_rt_priority);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating virtual "
                           "output port: %s", e.what());
                goto destroy_virtual_output_ports;
            }
        }
    }

    // Allocate and connect physical inputs
    potential_pi_count = MIDIGetNumberOfSources();
    if (potential_pi_count) {
        status = MIDIInputPortCreate(client, CFSTR("Physical Input Port"),
                                     HandleInputEvent, this, &internal_input);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Open", "MIDIInputPortCreate",
                            status);
            goto destroy_virtual_output_ports;
        }
        try {
            physical_input_ports =
                new JackCoreMidiPhysicalInputPort*[potential_pi_count];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating physical "
                       "input port array: %s", e.what());
            goto destroy_internal_input_port;
        }
        for (ItemCount i = 0; i < potential_pi_count; i++) {
            try {
                physical_input_ports[pi_count] =
                    new JackCoreMidiPhysicalInputPort(fAliasName, client_name,
                                                      capture_driver_name, i,
                                                      client, internal_input,
                                                      time_ratio);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating "
                           "physical input port: %s", e.what());
                continue;
            }
            pi_count++;
        }
    }

    // Allocate and connect physical outputs
    potential_po_count = MIDIGetNumberOfDestinations();
    if (potential_po_count) {
        status = MIDIOutputPortCreate(client, CFSTR("Physical Output Port"),
                                      &internal_output);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Open", "MIDIOutputPortCreate",
                            status);
            goto destroy_physical_input_ports;
        }
        try {
            physical_output_ports =
                new JackCoreMidiPhysicalOutputPort*[potential_po_count];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating physical "
                       "output port array: %s", e.what());
            goto destroy_internal_output_port;
        }
        for (ItemCount i = 0; i < potential_po_count; i++) {
            try {
                physical_output_ports[po_count] =
                    new JackCoreMidiPhysicalOutputPort(fAliasName, client_name,
                                                       playback_driver_name, i,
                                                       client, internal_output,
                                                       time_ratio,
                                                       port_rt_priority);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating "
                           "physical output port: %s", e.what());
                continue;
            }
            po_count++;
        }
    }

    if (! (pi_count || po_count || in_channels || out_channels)) {
        jack_error("JackCoreMidiDriver::Open - no CoreMIDI inputs or outputs "
                   "found, and no virtual ports allocated.");
    } else if (! JackMidiDriver::Open(capturing, playing,
                                      in_channels + pi_count,
                                      out_channels + po_count, monitor,
                                      capture_driver_name,
                                      playback_driver_name, capture_latency,
                                      playback_latency)) {
        num_physical_inputs = pi_count;
        num_physical_outputs = po_count;
        num_virtual_inputs = in_channels;
        num_virtual_outputs = out_channels;
        return 0;
    }

    // Cleanup
    if (physical_output_ports) {
        for (int i = 0; i < po_count; i++) {
            delete physical_output_ports[i];
        }
        delete[] physical_output_ports;
        physical_output_ports = 0;
    }
 destroy_internal_output_port:
    status = MIDIPortDispose(internal_output);
    if (status != noErr) {
        WriteMacOSError("JackCoreMidiDriver::Open", "MIDIPortDispose", status);
    }
 destroy_physical_input_ports:
    if (physical_input_ports) {
        for (int i = 0; i < pi_count; i++) {
            delete physical_input_ports[i];
        }
        delete[] physical_input_ports;
        physical_input_ports = 0;
    }
 destroy_internal_input_port:
    status = MIDIPortDispose(internal_input);
    if (status != noErr) {
        WriteMacOSError("JackCoreMidiDriver::Open", "MIDIPortDispose", status);
    }
 destroy_virtual_output_ports:
    if (virtual_output_ports) {
        for (int i = 0; i < vo_count; i++) {
            delete virtual_output_ports[i];
        }
        delete[] virtual_output_ports;
        virtual_output_ports = 0;
    }
 destroy_virtual_input_ports:
    if (virtual_input_ports) {
        for (int i = 0; i < vi_count; i++) {
            delete virtual_input_ports[i];
        }
        delete[] virtual_input_ports;
        virtual_input_ports = 0;
    }
 destroy_client:
    status = MIDIClientDispose(client);
    if (status != noErr) {
        WriteMacOSError("JackCoreMidiDriver::Open", "MIDIClientDispose",
                        status);
    }
    client = 0;
    return -1;
}

int
JackCoreMidiDriver::Read()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    for (int i = 0; i < num_physical_inputs; i++) {
        physical_input_ports[i]->ProcessJack(GetInputBuffer(i), buffer_size);
    }
    for (int i = 0; i < num_virtual_inputs; i++) {
        virtual_input_ports[i]->
            ProcessJack(GetInputBuffer(num_physical_inputs + i), buffer_size);
    }
    return 0;
}

int
JackCoreMidiDriver::Start()
{
    jack_info("JackCoreMidiDriver::Start - Starting driver.");

    JackMidiDriver::Start();

    int pi_count = 0;
    int po_count = 0;
    int vi_count = 0;
    int vo_count = 0;

    jack_info("JackCoreMidiDriver::Start - Enabling physical input ports.");

    for (; pi_count < num_physical_inputs; pi_count++) {
        if (physical_input_ports[pi_count]->Start() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to enable physical "
                       "input port.");
            goto stop_physical_input_ports;
        }
    }

    jack_info("JackCoreMidiDriver::Start - Enabling physical output ports.");

    for (; po_count < num_physical_outputs; po_count++) {
        if (physical_output_ports[po_count]->Start() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to enable physical "
                       "output port.");
            goto stop_physical_output_ports;
        }
    }

    jack_info("JackCoreMidiDriver::Start - Enabling virtual input ports.");

    for (; vi_count < num_virtual_inputs; vi_count++) {
        if (virtual_input_ports[vi_count]->Start() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to enable virtual "
                       "input port.");
            goto stop_virtual_input_ports;
        }
    }

    jack_info("JackCoreMidiDriver::Start - Enabling virtual output ports.");

    for (; vo_count < num_virtual_outputs; vo_count++) {
        if (virtual_output_ports[vo_count]->Start() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to enable virtual "
                       "output port.");
            goto stop_virtual_output_ports;
        }
    }

    jack_info("JackCoreMidiDriver::Start - Driver started.");

    return 0;

 stop_virtual_output_ports:
    for (int i = 0; i < vo_count; i++) {
        if (virtual_output_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to disable virtual "
                       "output port.");
        }
    }
 stop_virtual_input_ports:
    for (int i = 0; i < vi_count; i++) {
        if (virtual_input_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to disable virtual "
                       "input port.");
        }
    }
 stop_physical_output_ports:
    for (int i = 0; i < po_count; i++) {
        if (physical_output_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to disable "
                       "physical output port.");
        }
    }
 stop_physical_input_ports:
    for (int i = 0; i < pi_count; i++) {
        if (physical_input_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Start - Failed to disable "
                       "physical input port.");
        }
    }

    return -1;
}

int
JackCoreMidiDriver::Stop()
{
    int result = 0;

    jack_info("JackCoreMidiDriver::Stop - disabling physical input ports.");

    for (int i = 0; i < num_physical_inputs; i++) {
        if (physical_input_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Stop - Failed to disable physical "
                       "input port.");
            result = -1;
        }
    }

    jack_info("JackCoreMidiDriver::Stop - disabling physical output ports.");

    for (int i = 0; i < num_physical_outputs; i++) {
        if (physical_output_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Stop - Failed to disable physical "
                       "output port.");
            result = -1;
        }
    }

    jack_info("JackCoreMidiDriver::Stop - disabling virtual input ports.");

    for (int i = 0; i < num_virtual_inputs; i++) {
        if (virtual_input_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Stop - Failed to disable virtual "
                       "input port.");
            result = -1;
        }
    }

    jack_info("JackCoreMidiDriver::Stop - disabling virtual output ports.");

    for (int i = 0; i < num_virtual_outputs; i++) {
        if (virtual_output_ports[i]->Stop() < 0) {
            jack_error("JackCoreMidiDriver::Stop - Failed to disable virtual "
                       "output port.");
            result = -1;
        }
    }

    return result;
}

int
JackCoreMidiDriver::Write()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    for (int i = 0; i < num_physical_outputs; i++) {
        physical_output_ports[i]->ProcessJack(GetOutputBuffer(i), buffer_size);
    }
    for (int i = 0; i < num_virtual_outputs; i++) {
        virtual_output_ports[i]->
            ProcessJack(GetOutputBuffer(num_physical_outputs + i),
                        buffer_size);
    }
    return 0;
}

#ifdef __cplusplus
extern "C" {
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
