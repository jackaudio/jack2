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

#include "JackProfiler.h"
#include "JackServerGlobals.h"
#include "JackEngineControl.h"
#include "JackLockedEngine.h"
#include "JackArgParser.h"
#include <assert.h>
#include <string>

namespace Jack
{

    JackProfilerClient::JackProfilerClient(jack_client_t* client, const char* name)
        :fClient(client)
    {
        char port_name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
        fRefNum = JackServerGlobals::fInstance->GetEngine()->GetClientRefNum(name);
        
        snprintf(port_name, sizeof(port_name) - 1, "%s:scheduling", name);
        fSchedulingPort = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        
        snprintf(port_name, sizeof(port_name) - 1, "%s:duration", name);
        fDurationPort = jack_port_register(client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }
    
    JackProfilerClient::~JackProfilerClient()
    {
        jack_port_unregister(fClient, fSchedulingPort);
        jack_port_unregister(fClient, fDurationPort);
    }
    
#ifdef JACK_MONITOR
    JackProfiler::JackProfiler(jack_client_t* client, const JSList* params)
        :fClient(client), fLastMeasure(NULL)
#else
    JackProfiler::JackProfiler(jack_client_t* client, const JSList* params)
        :fClient(client)
#endif
    {
        jack_log("JackProfiler::JackProfiler");
        
        fCPULoadPort = fDriverPeriodPort = fDriverEndPort = NULL;
      
        const JSList* node;
        const jack_driver_param_t* param;
        for (node = params; node; node = jack_slist_next(node)) {
            param = (const jack_driver_param_t*)node->data;
            
            switch (param->character) {
                case 'c':
                    fCPULoadPort = jack_port_register(client, "cpu_load", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    break;
                    
                case 'p':
                    fDriverPeriodPort = jack_port_register(client, "driver_period", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    break;
                     
                case 'e':
                    fDriverEndPort = jack_port_register(client, "driver_end_time", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
                    break;
            }
        }
       
        // Resigster all running clients
        const char **ports = jack_get_ports(client, NULL, NULL, 0);
        if (ports) {
            for (int i = 0; ports[i]; ++i) {
                std::string str = std::string(ports[i]);
                ClientRegistration(str.substr(0, str.find_first_of(':')).c_str(), 1, this);
            }
            free(ports);
        }
     
        jack_set_process_callback(client, Process, this);
        jack_set_client_registration_callback(client, ClientRegistration, this);
        jack_activate(client);
    }

    JackProfiler::~JackProfiler()
    {
        jack_log("JackProfiler::~JackProfiler");
    }
    
    void JackProfiler::ClientRegistration(const char* name, int val, void *arg)
    {
    #ifdef JACK_MONITOR
        JackProfiler* profiler = static_cast<JackProfiler*>(arg);
        
        // Filter client or "system" name
        if (strcmp(name, jack_get_client_name(profiler->fClient)) == 0 || strcmp(name, "system") == 0)
            return;
        
        profiler->fMutex.Lock();
        if (val) {
            std::map<std::string, JackProfilerClient*>::iterator it = profiler->fClientTable.find(name);
            if (it == profiler->fClientTable.end()) {
                jack_log("Client %s added", name);
                profiler->fClientTable[name] = new JackProfilerClient(profiler->fClient, name);
            }
        } else {
            std::map<std::string, JackProfilerClient*>::iterator it = profiler->fClientTable.find(name);
            if (it != profiler->fClientTable.end()) {
                jack_log("Client %s removed", name);
                profiler->fClientTable.erase(it);
                delete((*it).second);
            }
        }
        profiler->fMutex.Unlock();
    #endif
    }

    int JackProfiler::Process(jack_nframes_t nframes, void* arg)
    {
        JackProfiler* profiler = static_cast<JackProfiler*>(arg);
        
        if (profiler->fCPULoadPort) {
            float* buffer_cpu_load = (float*)jack_port_get_buffer(profiler->fCPULoadPort, nframes);
            float cpu_load = jack_cpu_load(profiler->fClient);
            for (unsigned int i = 0; i < nframes; i++) {
                buffer_cpu_load[i] = cpu_load / 100.f;
            }
        }
 
    #ifdef JACK_MONITOR      
        
        JackEngineControl* control = JackServerGlobals::fInstance->GetEngineControl();
        JackEngineProfiling* engine_profiler = &control->fProfiler;
        JackTimingMeasure* measure = engine_profiler->GetCurMeasure();
        
       if (profiler->fLastMeasure && profiler->fMutex.Trylock()) {
        
            if (profiler->fDriverPeriodPort) {
                float* buffer_driver_period = (float*)jack_port_get_buffer(profiler->fDriverPeriodPort, nframes);
                float value1 = (float(measure->fPeriodUsecs) - float(measure->fCurCycleBegin - profiler->fLastMeasure->fCurCycleBegin)) / float(measure->fPeriodUsecs);
                for (unsigned int i = 0; i < nframes; i++) {
                    buffer_driver_period[i] = value1;
                }
            }
            
            if (profiler->fDriverEndPort) {
                float* buffer_driver_end_time = (float*)jack_port_get_buffer(profiler->fDriverEndPort, nframes);
                float value2 = (float(measure->fPrevCycleEnd - profiler->fLastMeasure->fCurCycleBegin)) / float(measure->fPeriodUsecs);
                for (unsigned int i = 0; i < nframes; i++) {
                    buffer_driver_end_time[i] = value2;
                }
            }
            
            std::map<std::string, JackProfilerClient*>::iterator it;
            for (it = profiler->fClientTable.begin(); it != profiler->fClientTable.end(); it++) {
                int ref = (*it).second->fRefNum;
                long d5 = long(measure->fClientTable[ref].fSignaledAt - profiler->fLastMeasure->fCurCycleBegin);
                long d6 = long(measure->fClientTable[ref].fAwakeAt - profiler->fLastMeasure->fCurCycleBegin);
                long d7 = long(measure->fClientTable[ref].fFinishedAt - profiler->fLastMeasure->fCurCycleBegin);
                  
                float* buffer_scheduling = (float*)jack_port_get_buffer((*it).second->fSchedulingPort, nframes);
                float value3 = float(d6 - d5) / float(measure->fPeriodUsecs);
                jack_log("Scheduling %f", value3);
                for (unsigned int i = 0; i < nframes; i++) {
                    buffer_scheduling[i] = value3;
                }
                  
                float* buffer_duration = (float*)jack_port_get_buffer((*it).second->fDurationPort, nframes);
                float value4 = float(d7 - d6) / float(measure->fPeriodUsecs);
                jack_log("Duration %f", value4);
                for (unsigned int i = 0; i < nframes; i++) {
                    buffer_duration[i] = value4;
                }
            }
            
            profiler->fMutex.Unlock();
        }
        profiler->fLastMeasure = measure;
    #endif
        return 0;
    }
    
} // namespace Jack

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver_interface.h"

    using namespace Jack;
    
    static Jack::JackProfiler* profiler = NULL;

    SERVER_EXPORT jack_driver_desc_t* jack_get_descriptor()
    {
        jack_driver_desc_t * desc;
        jack_driver_desc_filler_t filler;
        jack_driver_param_value_t value;

        desc = jack_driver_descriptor_construct("profiler", JackDriverNone, "real-time server profiling", &filler);

        value.i = FALSE;
        jack_driver_descriptor_add_parameter(desc, &filler, "cpu-load", 'c', JackDriverParamBool, &value, NULL, "Show DSP CPU load", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "driver-period", 'p', JackDriverParamBool, &value, NULL, "Show driver period", NULL);
        jack_driver_descriptor_add_parameter(desc, &filler, "driver-end-time", 'e', JackDriverParamBool, &value, NULL, "Show driver end time", NULL);

        return desc;
    }

    SERVER_EXPORT int jack_internal_initialize(jack_client_t* jack_client, const JSList* params)
    {
        if (profiler) {
            jack_info("profiler already loaded");
            return 1;
        }
        
        jack_log("Loading profiler");
        try {
            profiler = new Jack::JackProfiler(jack_client, params);
            assert(profiler);
            return 0;
        } catch (...) {
            return 1;
        }
    }

    SERVER_EXPORT int jack_initialize(jack_client_t* jack_client, const char* load_init)
    {
        JSList* params = NULL;
        bool parse_params = true;
        int res = 1;
        jack_driver_desc_t* desc = jack_get_descriptor();

        Jack::JackArgParser parser ( load_init );
        if ( parser.GetArgc() > 0 )
            parse_params = parser.ParseParams ( desc, &params );

        if (parse_params) {
            res = jack_internal_initialize ( jack_client, params );
            parser.FreeParams ( params );
        }
        return res;
    }

    SERVER_EXPORT void jack_finish(void* arg)
    {
        Jack::JackProfiler* profiler = static_cast<Jack::JackProfiler*>(arg);

        if (profiler) {
            jack_log("Unloading profiler");
            delete profiler;
        }
    }

#ifdef __cplusplus
}
#endif
