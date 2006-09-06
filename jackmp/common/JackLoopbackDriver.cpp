/*
Copyright (C) 2001 Paul Davis 
Copyright (C) 2004-2006 Grame

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

#include "JackLoopbackDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include <iostream>
#include <assert.h>

namespace Jack
{

int JackLoopbackDriver::Open(jack_nframes_t nframes,
                             jack_nframes_t samplerate,
                             int capturing,
                             int playing,
                             int inchannels,
                             int outchannels,
                             bool monitor,
                             const char* capture_driver_name,
                             const char* playback_driver_name,
                             jack_nframes_t capture_latency,
                             jack_nframes_t playback_latency)
{
    return JackAudioDriver::Open(nframes,
                                 samplerate,
                                 capturing,
                                 playing,
                                 inchannels,
                                 outchannels,
                                 monitor,
                                 capture_driver_name,
                                 playback_driver_name,
                                 capture_latency,
                                 playback_latency);
}

int JackLoopbackDriver::Process()
{
    assert(fCaptureChannels == fPlaybackChannels);

    // Loopback copy
    for (int i = 0; i < fCaptureChannels; i++) {
        memcpy(fGraphManager->GetBuffer(fCapturePortList[i], fEngineControl->fBufferSize),
               fGraphManager->GetBuffer(fPlaybackPortList[i], fEngineControl->fBufferSize),
               sizeof(float) * fEngineControl->fBufferSize);
    }

    fGraphManager->ResumeRefNum(fClientControl, fSynchroTable); // Signal all clients
    if (fEngineControl->fSyncMode) {
        if (fGraphManager->SuspendRefNum(fClientControl, fSynchroTable, fEngineControl->fTimeOutUsecs) < 0)
            jack_error("JackLoopbackDriver::ProcessSync SuspendRefNum error");
    }
    return 0;
}

void JackLoopbackDriver::PrintState()
{
    int i;
    jack_port_id_t port_index;

    std::cout << "JackLoopbackDriver state" << std::endl;
    std::cout << "Input ports" << std::endl;

    for (i = 0; i < fPlaybackChannels; i++) {
        port_index = fCapturePortList[i];
        JackPort* port = fGraphManager->GetPort(port_index);
        std::cout << port->GetName() << std::endl;
        if (fGraphManager->GetConnectionsNum(port_index)) {}
    }

    std::cout << "Output ports" << std::endl;

    for (i = 0; i < fCaptureChannels; i++) {
        port_index = fPlaybackPortList[i];
        JackPort* port = fGraphManager->GetPort(port_index);
        std::cout << port->GetName() << std::endl;
        if (fGraphManager->GetConnectionsNum(port_index)) {}
    }
}

} // end of namespace

/*
#ifdef __cplusplus
extern "C" {
#endif
 
jack_driver_desc_t * driver_get_descriptor ()
{
	jack_driver_desc_t * desc;
	jack_driver_param_desc_t * params;
	unsigned int i;
 
	desc = (jack_driver_desc_t*)calloc (1, sizeof (jack_driver_desc_t));
	strcpy (desc->name, "dummy");
	desc->nparams = 5;
 
	params = (jack_driver_param_desc_t*)calloc (desc->nparams, sizeof (jack_driver_param_desc_t));
 
	i = 0;
	strcpy (params[i].name, "capture");
	params[i].character  = 'C';
	params[i].type       = JackDriverParamUInt;
	params[i].value.ui   = 2U;
	strcpy (params[i].short_desc, "Number of capture ports");
	strcpy (params[i].long_desc, params[i].short_desc);
 
	i++;
	strcpy (params[i].name, "playback");
	params[i].character  = 'P';
	params[i].type       = JackDriverParamUInt;
	params[1].value.ui   = 2U;
	strcpy (params[i].short_desc, "Number of playback ports");
	strcpy (params[i].long_desc, params[i].short_desc);
 
	i++;
	strcpy (params[i].name, "rate");
	params[i].character  = 'r';
	params[i].type       = JackDriverParamUInt;
	params[i].value.ui   = 48000U;
	strcpy (params[i].short_desc, "Sample rate");
	strcpy (params[i].long_desc, params[i].short_desc);
 
	i++;
	strcpy (params[i].name, "period");
	params[i].character  = 'p';
	params[i].type       = JackDriverParamUInt;
	params[i].value.ui   = 1024U;
	strcpy (params[i].short_desc, "Frames per period");
	strcpy (params[i].long_desc, params[i].short_desc);
 
	i++;
	strcpy (params[i].name, "wait");
	params[i].character  = 'w';
	params[i].type       = JackDriverParamUInt;
	params[i].value.ui   = 21333U;
	strcpy (params[i].short_desc,
		"Number of usecs to wait between engine processes");
	strcpy (params[i].long_desc, params[i].short_desc);
 
	desc->params = params;
 
	return desc;
}
 
Jack::JackDriverClientInterface* driver_initialize(Jack::JackEngine* engine, Jack::JackSynchro** table, const JSList* params)
{
	jack_nframes_t sample_rate = 48000;
	jack_nframes_t period_size = 1024;
	unsigned int capture_ports = 2;
	unsigned int playback_ports = 2;
	const JSList * node;
	const jack_driver_param_t * param;
 
	for (node = params; node; node = jack_slist_next (node)) {
  	        param = (const jack_driver_param_t *) node->data;
 
		switch (param->character) {
 
		case 'C':
		  capture_ports = param->value.ui;
		  break;
 
		case 'P':
		  playback_ports = param->value.ui;
		  break;
 
		case 'r':
		  sample_rate = param->value.ui;
		  break;
 
		case 'p':
		  period_size = param->value.ui;
		  break;
				
		}
	}
	Jack::JackDriverClientInterface* driver = new Jack::JackLoopbackDriver("loopback", engine, table);
	if (driver->Open(period_size, sample_rate, 1, 1, capture_ports, playback_ports, "loopback") == 0) {
		return driver;
	} else {
		delete driver;
		return NULL;
	}
}
 
#ifdef __cplusplus
}
#endif
*/

