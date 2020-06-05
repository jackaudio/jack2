/*
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackGlobals__
#define __JackGlobals__

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <mutex>

#include "JackPlatformPlug.h"
#include "JackSystemDeps.h"
#include "JackConstants.h"
#include "JackError.h"

#ifdef __CLIENTDEBUG__
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#endif

namespace Jack
{
    class JackGlobals;
    class JackGraphManager;
    class JackServer;
    struct JackEngineControl;

// Globals used for client management on server or library side.
class JackGlobalsManager
{

    /* This object is managed by JackGlobalsManager */
    private:

        static JackGlobalsManager fInstance;

        JackGlobalsManager() {}
        ~JackGlobalsManager() {}

    public:

        std::vector<JackGlobals*> fContexts;

        inline static JackGlobalsManager* Instance()
        {
            return &fInstance;
        }

        template <class T>
        inline T* CreateGlobal(const std::string &server_name);

        inline void DestroyGlobal(const std::string &server_name);

};

// Globals used for client management on server or library side.
class JackGlobals
{

    friend class JackGlobalsManager;

    protected:

        JackGlobals(const std::string &server_name);
        virtual ~JackGlobals();

    public:

        static jack_tls_key fRealTimeThread;
        static jack_tls_key fNotificationThread;
        static jack_tls_key fKeyLogFunction;
        static JackMutex* fOpenMutex;
        static bool fVerbose;
#ifndef WIN32
        static jack_thread_creator_t fJackThreadCreator;
#endif

#ifdef __CLIENTDEBUG__
        static std::ofstream* fStream;
#endif
        static void CheckContext(const char* name);

        volatile bool fServerRunning;
        JackMutex* fSynchroMutex;
        std::mutex fMutex;
        std::string fServerName;
        JackClient* fClientTable[CLIENT_NUM];

        /* is called each time a context for specific server is added */
        virtual bool AddContext(const uint32_t &cntx_num, const std::string &server_name) = 0;

        /* is called each time a context for specific server is removed */
        virtual bool DelContext(const uint32_t &cntx_num) = 0;

        virtual JackGraphManager* GetGraphManager() = 0;
        virtual JackEngineControl* GetEngineControl() = 0;
        virtual JackSynchro* GetSynchroTable() = 0;
        virtual JackServer* GetServer() = 0;

        inline static jack_port_id_t PortId(const jack_port_t* port)
        {
            uintptr_t port_aux = (uintptr_t)port;
            jack_port_id_t port_masked = (jack_port_id_t)port_aux;
            jack_port_id_t port_id = port_masked & ~PORT_SERVER_CONTEXT_MASK;
            return port_id;
        }

        inline static JackGlobals* PortGlobal(const jack_port_t* port)
        {
            jack_port_id_t context_id = PortContext(port);

            if (context_id >= JackGlobalsManager::Instance()->fContexts.size())
            {
                jack_error("invalid context for port '%p' requested", port);
                return nullptr;
            }

            return JackGlobalsManager::Instance()->fContexts[context_id];
        }

        inline static jack_port_id_t PortContext(const jack_port_t* port)
        {
            uintptr_t port_aux = (uintptr_t)port;
            jack_port_id_t port_masked = (jack_port_id_t)port_aux;
            jack_port_id_t port_context = (port_masked & PORT_SERVER_CONTEXT_MASK) >> PORT_SERVER_CONTEXT_SHIFT;
            return port_context;
        }

        inline jack_port_id_t PortContext()
        {
            std::vector<JackGlobals*> &contexts = JackGlobalsManager::Instance()->fContexts;

            auto it = std::find_if(contexts.begin(), contexts.end(), [this] (JackGlobals* i) {
                return (i && i->fServerName.compare(this->fServerName) == 0);
            });

            if (it == contexts.end()) {
                return PORT_SERVER_CONTEXT_MAX;
            }

            return it - contexts.begin();
        }

        inline jack_port_t* PortById(const jack_port_id_t &port_id)
        {
            jack_port_id_t context_id = PortContext();
            if (context_id > PORT_SERVER_CONTEXT_MAX) {
                return NULL;
            }

            /* cast to uintptr_t required to avoid [-Wint-to-pointer-cast] warning */
            const uintptr_t port = ((context_id << PORT_SERVER_CONTEXT_SHIFT) | port_id);
            return (jack_port_t*)port;


        }

};

class JackGlobalsInterface
{

    private:

        JackGlobals *fGlobal;

    public:

        JackGlobalsInterface(JackGlobals *global = nullptr)
            : fGlobal(global)
        {

        }

        ~JackGlobalsInterface()
        {

        }

        inline JackGlobals* GetGlobal() const
        {
            return fGlobal;
        }

        inline void SetGlobal(JackGlobals *global)
        {
            fGlobal = global;
        }

};

template <class T>
inline T* JackGlobalsManager::CreateGlobal(const std::string &server_name)
{
    uint32_t context_num = fContexts.size();

    std::vector<JackGlobals*>::iterator it = std::find_if(fContexts.begin(), fContexts.end(), [&server_name] (JackGlobals* i) {
        return (i && (i->fServerName.compare(server_name) == 0));
    });

    T* global = nullptr;

    if (it == fContexts.end()) {
        global = new T(server_name);
        it = std::find(fContexts.begin(), fContexts.end(), nullptr);
        if (it == fContexts.end()) {
            fContexts.push_back(static_cast<JackGlobals*>(global));
        } else {
            *it = global;
        }
    } else {
        global = dynamic_cast<T*>(*it);
    }

    if (global == nullptr) {
        return global;
    }

    bool success = false;
    {
        std::lock_guard<std::mutex> lock(global->fMutex);
        success = global->AddContext(context_num, server_name);
    }

    if (success == false) {
        DestroyGlobal(server_name);
        return nullptr;
    }

    return global;
}

inline void JackGlobalsManager::DestroyGlobal(const std::string &server_name)
{
    std::vector<JackGlobals*>::iterator it = std::find_if(fContexts.begin(), fContexts.end(), [&server_name] (JackGlobals* i) {
        return (i && (i->fServerName.compare(server_name) == 0));
    });

    if (it == fContexts.end()) {
        return;
    }

    JackGlobals *global = *it;

    {
        std::lock_guard<std::mutex> lock(global->fMutex);

        if (global->DelContext(fContexts.size()) == false) {
            return;
        }
    }

    delete global;
    *it = nullptr;
}

} // end of namespace

#endif
