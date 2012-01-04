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

#include "JackCompilerDeps.h"
#include "JackCoreMidiDriver.h"
#include "JackCoreMidiUtil.h"
#include "JackEngineControl.h"
#include "driver_interface.h"

#include <stdexcept>
#include <mach/mach_time.h>

using Jack::JackCoreMidiDriver;

static char capture_driver_name[256];
static char playback_driver_name[256];

static int in_channels, out_channels;
static bool capturing, playing, monitor;

static jack_nframes_t capture_latency, playback_latency;

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
    JackMidiDriver(name, alias, engine, table),fThread(this)
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
    internal_input = 0;
    internal_output = 0;
}

JackCoreMidiDriver::~JackCoreMidiDriver()
{}

bool JackCoreMidiDriver::Init()
{
    return OpenAux();
}

bool JackCoreMidiDriver::OpenAux()
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
        return false;
    }

    OSStatus status = MIDIClientCreate(name, HandleNotificationEvent, this,
                                       &client);

    CFRelease(name);

    if (status != noErr) {
        WriteMacOSError("JackCoreMidiDriver::Open", "MIDIClientCreate",
                        status);
        return false;
    }

    char *client_name = fClientControl.fName;

    // Allocate and connect physical inputs
    potential_pi_count = MIDIGetNumberOfSources();
    if (potential_pi_count) {
        status = MIDIInputPortCreate(client, CFSTR("Physical Input Port"),
                                     HandleInputEvent, this, &internal_input);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Open", "MIDIInputPortCreate",
                            status);
            goto destroy;
        }

        try {
            physical_input_ports =
                new JackCoreMidiPhysicalInputPort*[potential_pi_count];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating physical "
                       "input port array: %s", e.what());
            goto destroy;
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
                goto destroy;
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
            goto destroy;
        }

        try {
            physical_output_ports =
                new JackCoreMidiPhysicalOutputPort*[potential_po_count];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating physical "
                       "output port array: %s", e.what());
            goto destroy;
        }

        for (ItemCount i = 0; i < potential_po_count; i++) {
            try {
                physical_output_ports[po_count] =
                    new JackCoreMidiPhysicalOutputPort(fAliasName, client_name,
                                                       playback_driver_name, i,
                                                       client, internal_output,
                                                       time_ratio);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating "
                           "physical output port: %s", e.what());
                goto destroy;
            }
            po_count++;
        }
    }

    // Allocate and connect virtual inputs
    if (in_channels) {
        try {
            virtual_input_ports =
                new JackCoreMidiVirtualInputPort*[in_channels];
        } catch (std::exception e) {
            jack_error("JackCoreMidiDriver::Open - while creating virtual "
                       "input port array: %s", e.what());
            goto destroy;

        }
        for (vi_count = 0; vi_count < in_channels; vi_count++) {
            try {
                virtual_input_ports[vi_count] =
                    new JackCoreMidiVirtualInputPort(fAliasName, client_name,
                                                     capture_driver_name,
                                                     vi_count + pi_count, client,
                                                     time_ratio);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating virtual "
                           "input port: %s", e.what());
                goto destroy;
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
            goto destroy;
        }
        for (vo_count = 0; vo_count < out_channels; vo_count++) {
            try {
                virtual_output_ports[vo_count] =
                    new JackCoreMidiVirtualOutputPort(fAliasName, client_name,
                                                      playback_driver_name,
                                                      vo_count + po_count, client,
                                                      time_ratio);
            } catch (std::exception e) {
                jack_error("JackCoreMidiDriver::Open - while creating virtual "
                           "output port: %s", e.what());
                goto destroy;
            }
        }
    }


    if (! (pi_count || po_count || in_channels || out_channels)) {
        jack_error("JackCoreMidiDriver::Open - no CoreMIDI inputs or outputs "
                   "found, and no virtual ports allocated.");
    }

    if (! JackMidiDriver::Open(capturing, playing,
                              in_channels + pi_count,
                              out_channels + po_count, monitor,
                              capture_driver_name,
                              playback_driver_name, capture_latency,
                              playback_latency)) {
        num_physical_inputs = pi_count;
        num_physical_outputs = po_count;
        num_virtual_inputs = in_channels;
        num_virtual_outputs = out_channels;
        return true;
    }

 destroy:

    if (physical_input_ports) {
        for (int i = 0; i < pi_count; i++) {
            delete physical_input_ports[i];
        }
        delete[] physical_input_ports;
        physical_input_ports = 0;
    }

    if (physical_output_ports) {
        for (int i = 0; i < po_count; i++) {
            delete physical_output_ports[i];
        }
        delete[] physical_output_ports;
        physical_output_ports = 0;
    }

    if (virtual_input_ports) {
        for (int i = 0; i < vi_count; i++) {
            delete virtual_input_ports[i];
        }
        delete[] virtual_input_ports;
        virtual_input_ports = 0;
    }

    if (virtual_output_ports) {
        for (int i = 0; i < vo_count; i++) {
            delete virtual_output_ports[i];
        }
        delete[] virtual_output_ports;
        virtual_output_ports = 0;
    }

    if (internal_output) {
        status = MIDIPortDispose(internal_output);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Open", "MIDIPortDispose", status);
        }
    }

    if (internal_input) {
        status = MIDIPortDispose(internal_input);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Open", "MIDIPortDispose", status);
        }
    }

    if (client) {
        status = MIDIClientDispose(client);
        if (status != noErr) {
            WriteMacOSError("JackCoreMidiDriver::Open", "MIDIClientDispose",
                            status);
        }
    }

    // Default open
    if (! JackMidiDriver::Open(capturing, playing,
                              in_channels + pi_count,
                              out_channels + po_count, monitor,
                              capture_driver_name,
                              playback_driver_name, capture_latency,
                              playback_latency)) {
        client = 0;
        num_physical_inputs = 0;
        num_physical_outputs = 0;
        num_virtual_inputs = 0;
        num_virtual_outputs = 0;
       return true;
    } else {
        return false;
    }
}

bool JackCoreMidiDriver::Execute()
{
    CFRunLoopRun();
    return false;
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
        if (fEngine->PortRegister(fClientControl.fRefNum, name,
                                JACK_DEFAULT_MIDI_TYPE,
                                CaptureDriverFlags, buffer_size, &index) < 0) {
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
        if (fEngine->PortRegister(fClientControl.fRefNum, name,
                                JACK_DEFAULT_MIDI_TYPE,
                                CaptureDriverFlags, buffer_size, &index) < 0) {
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
        fEngine->PortRegister(fClientControl.fRefNum, name,
                            JACK_DEFAULT_MIDI_TYPE,
                            PlaybackDriverFlags, buffer_size, &index);
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
        fEngine->PortRegister(fClientControl.fRefNum, name,
                            JACK_DEFAULT_MIDI_TYPE,
                            PlaybackDriverFlags, buffer_size, &index);
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
    fThread.Kill();
    return CloseAux();
}

int
JackCoreMidiDriver::CloseAux()
{
    // Generic MIDI driver close
    int result = JackMidiDriver::Close();

    OSStatus status;
    if (physical_input_ports) {
        for (int i = 0; i < num_physical_inputs; i++) {
            delete physical_input_ports[i];
        }
        delete[] physical_input_ports;
        num_physical_inputs = 0;
        physical_input_ports = 0;
        if (internal_input) {
            status = MIDIPortDispose(internal_input);
            if (status != noErr) {
                WriteMacOSError("JackCoreMidiDriver::Close", "MIDIPortDispose",
                                status);
                result = -1;
            }
            internal_input = 0;
        }
    }
    if (physical_output_ports) {
        for (int i = 0; i < num_physical_outputs; i++) {
            delete physical_output_ports[i];
        }
        delete[] physical_output_ports;
        num_physical_outputs = 0;
        physical_output_ports = 0;
        if (internal_output) {
            status = MIDIPortDispose(internal_output);
            if (status != noErr) {
                WriteMacOSError("JackCoreMidiDriver::Close", "MIDIPortDispose",
                                status);
                result = -1;
            }
            internal_output = 0;
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
JackCoreMidiDriver::Restart()
{
    JackLock lock(this);

    SaveConnections();
    Stop();
    Detach();
    CloseAux();
    OpenAux();
    Attach();
    Start();
    RestoreConnections();
}

void
JackCoreMidiDriver::HandleNotification(const MIDINotification *message)
{
    switch (message->messageID) {

        case kMIDIMsgSetupChanged:
            Restart();
            break;

        case kMIDIMsgObjectAdded:
            break;

        case kMIDIMsgObjectRemoved:
            break;

    }
}

int
JackCoreMidiDriver::Open(bool capturing_aux, bool playing_aux, int in_channels_aux,
                         int out_channels_aux, bool monitor_aux,
                         const char* capture_driver_name_aux,
                         const char* playback_driver_name_aux,
                         jack_nframes_t capture_latency_aux,
                         jack_nframes_t playback_latency_aux)
{

    strcpy(capture_driver_name, capture_driver_name_aux);
    strcpy(playback_driver_name, playback_driver_name_aux);

    capturing = capturing_aux;
    playing = playing_aux;
    in_channels = in_channels_aux;
    out_channels = out_channels_aux;
    monitor = monitor_aux;
    capture_latency = capture_latency_aux;
    playback_latency = playback_latency_aux;

    fThread.StartSync();

    int count = 0;
    while (fThread.GetStatus() != JackThread::kRunning && ++count < WAIT_COUNTER) {
        JackSleep(100000);
        jack_log("JackCoreMidiDriver::Open wait count = %d", count);

    }
    if (count == WAIT_COUNTER) {
        jack_info("Cannot open CoreMIDI driver");
        fThread.Kill();
        return -1;
    } else {
        JackSleep(10000);
        jack_info("CoreMIDI driver is running...");
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

    JackMidiDriver::Stop();

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
JackCoreMidiDriver::ProcessRead()
{
    int res;
    if (Trylock()) {
        res = (fEngineControl->fSyncMode) ? ProcessReadSync() : ProcessReadAsync();
        Unlock();
    } else {
        res = -1;
    }
    return res;
}

int
JackCoreMidiDriver::ProcessWrite()
{
    int res;
    if (Trylock()) {
        res = (fEngineControl->fSyncMode) ? ProcessWriteSync() : ProcessWriteAsync();
        Unlock();
    } else {
        res = -1;
    }
    return res;
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
JackCoreMidiDriver::Write()
{
    jack_nframes_t buffer_size = fEngineControl->fBufferSize;
    for (int i = 0; i < num_physical_outputs; i++) {
        physical_output_ports[i]->ProcessJack(GetOutputBuffer(i), buffer_size);
    }
    for (int i = 0; i < num_virtual_outputs; i++) {
        virtual_output_ports[i]->
            ProcessJack(GetOutputBuffer(num_physical_outputs + i), buffer_size);
    }
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

    // singleton kind of driver
    static Jack::JackDriverClientInterface* driver = NULL;

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("coremidi", JackDriverSlave, "Apple CoreMIDI API based MIDI backend", &filler);

        value.ui  = 0;
        jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "CoreMIDI virtual bus", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "CoreMIDI virtual bus", NULL);

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

        // singleton kind of driver
        if (!driver) {
            driver = new Jack::JackCoreMidiDriver("system_midi", "coremidi", engine, table);
            if (driver->Open(1, 1, virtual_in, virtual_out, false, "in", "out", 0, 0) == 0) {
                return driver;
            } else {
                delete driver;
                return NULL;
            }
        } else {
            jack_info("JackCoreMidiDriver already allocated, cannot be loaded twice");
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif
