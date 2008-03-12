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

#include "JackExports.h"
#include "driver_interface.h"
#include "JackDriverLoader.h"
#include "JackConnectionManager.h"
#include "jslist.h"

namespace Jack
{

class JackGraphManager;
class JackDriverClientInterface;
class JackServerChannelInterface;
class JackSyncInterface;
struct JackEngineControl;
class JackEngine;

/*!
\brief The Jack server.
*/

class EXPORT JackServer
{

    private:

        jack_driver_info_t* fDriverInfo;
        JackDriverClientInterface* fAudioDriver;
        JackDriverClientInterface* fFreewheelDriver;
        JackDriverClientInterface* fLoopbackDriver;
        JackEngine* fEngine;
        JackEngineControl* fEngineControl;
        JackGraphManager* fGraphManager;
        JackServerChannelInterface* fChannel;
		JackConnectionManager fConnectionState;
        JackSynchro* fSynchroTable[CLIENT_NUM];
        bool fFreewheel;
        long fLoopback;

    public:

        JackServer(bool sync, bool temporary, long timeout, bool rt, long priority, long loopback, bool verbose, const char* server_name);
        virtual ~JackServer();

        int Open(jack_driver_desc_t* driver_desc, JSList* driver_params);
        int Close();

        int Start();
        int Stop();

        int SetBufferSize(jack_nframes_t buffer_size);
        int SetFreewheel(bool onoff);
        void Notify(int refnum, int notify, int value);

        int InternalClientLoad(const char* client_name, const char* so_name, const char* objet_data, int options, int* int_ref, int* status);

        // Transport management
        int ReleaseTimebase(int refnum);
        int SetTimebaseCallback(int refnum, int conditional);

        // Object access
        JackEngine* GetEngine();
        JackEngineControl* GetEngineControl();
        JackSynchro** GetSynchroTable();
        JackGraphManager* GetGraphManager();

        static JackServer* fInstance; // Unique instance
};

} // end of namespace


#endif
