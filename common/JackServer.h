/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifndef __JackServer__
#define __JackServer__

#include "JackCompilerDeps.h"
#include "driver_interface.h"
#include "JackDriverLoader.h"
#include "JackDriverInfo.h"
#include "JackConnectionManager.h"
#include "JackGlobals.h"
#include "JackPlatformPlug.h"
#include "jslist.h"

namespace Jack
{

class JackGraphManager;
class JackDriverClientInterface;
struct JackEngineControl;
class JackLockedEngine;
class JackLoadableInternalClient;

/*!
\brief The Jack server.
*/

class SERVER_EXPORT JackServer
{

    private:

        JackDriverInfo* fDriverInfo;
        JackDriverClientInterface* fAudioDriver;
        JackDriverClientInterface* fFreewheelDriver;
        JackDriverClientInterface* fThreadedFreewheelDriver;
        JackLockedEngine* fEngine;
        JackEngineControl* fEngineControl;
        JackGraphManager* fGraphManager;
        JackServerChannel fChannel;
        JackConnectionManager fConnectionState;
        JackSynchro fSynchroTable[CLIENT_NUM];
        bool fFreewheel;

        int InternalClientLoadAux(JackLoadableInternalClient* client, const char* so_name, const char* client_name, int options, int* int_ref, int uuid, int* status);

    public:

        JackServer(bool sync, bool temporary, int timeout, bool rt, int priority, int port_max, bool verbose, jack_timer_type_t clock, const char* server_name);
        ~JackServer();

        int Open(jack_driver_desc_t* driver_desc, JSList* driver_params);
        int Close();

        int Start();
        int Stop();
        bool IsRunning();

        // RT thread
        void Notify(int refnum, int notify, int value);

        // Command thread : API
        int SetBufferSize(jack_nframes_t buffer_size);
        int SetFreewheel(bool onoff);
        int InternalClientLoad1(const char* client_name, const char* so_name, const char* objet_data, int options, int* int_ref, int uuid, int* status);
        int InternalClientLoad2(const char* client_name, const char* so_name, const JSList * parameters, int options, int* int_ref, int uuid, int* status);
        void ClientKill(int refnum);

        // Transport management
        int ReleaseTimebase(int refnum);
        int SetTimebaseCallback(int refnum, int conditional);

        // Backend management
        JackDriverInfo* AddSlave(jack_driver_desc_t* driver_desc, JSList* driver_params);
        void RemoveSlave(JackDriverInfo* info);
        int SwitchMaster(jack_driver_desc_t* driver_desc, JSList* driver_params);

        // Object access
        JackLockedEngine* GetEngine();
        JackEngineControl* GetEngineControl();
        JackSynchro* GetSynchroTable();
        JackGraphManager* GetGraphManager();

};

} // end of namespace


#endif
