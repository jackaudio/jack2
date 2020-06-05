/*
Copyright (C) 2005 Grame

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

#ifndef __JackServerGlobals__
#define __JackServerGlobals__

#include "driver_interface.h"
#include "JackDriverLoader.h"
#include "JackCompilerDeps.h"
#include "JackServer.h"
#include "JackGlobals.h"
#include <map>

namespace Jack
{

class JackClient;

/*!
\brief Global server static structure: singleton kind of pattern.
*/

class SERVER_EXPORT JackServerGlobals : public JackGlobals
{

    /* This object is managed by JackGlobalsManager */
    friend class JackGlobalsManager;

    private:

    JackServerGlobals(const std::string &server_name) : JackGlobals(server_name) {}
    ~JackServerGlobals() override {}

    public:

        static JackServer* fInstance;
        static unsigned int fUserCount;
        static std::map<std::string, JackDriverInfo*> fSlavesList;
        static std::map<std::string, int> fInternalsList;

        static bool (* on_device_acquire)(const char* device_name);
        static void (* on_device_release)(const char* device_name);
        static void (* on_device_reservation_loop)(void);

        bool Init();
        void Deinit();
        int Start(const char* server_name,
                  jack_driver_desc_t* driver_desc,
                  JSList* driver_params,
                  int sync,
                  int temporary,
                  int time_out_ms,
                  int rt,
                  int priority,
                  int port_max,
                  int verbose,
                  jack_timer_type_t clock,
                  char self_connect_mode);
        void Stop();
        void Destroy();

        bool AddContext(const uint32_t &context_num, const std::string &server_name) override
        {
            return true;
        }

        bool DelContext(const uint32_t &context_num) override
        {
            return true;
        }

        JackGraphManager* GetGraphManager() override;
        JackEngineControl* GetEngineControl() override;
        JackSynchro* GetSynchroTable() override;
        JackServer* GetServer() override;

};

} // end of namespace

#endif

