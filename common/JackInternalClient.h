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

#ifndef __JackInternalClient__
#define __JackInternalClient__

#include "JackClient.h"
#include "JackClientControl.h"
#include "driver_interface.h"

namespace Jack
{

struct JackEngineControl;

/*!
\brief Internal clients in the server.
*/

class JackInternalClient : public JackClient
{

    private:

        JackClientControl fClientControl;     /*! Client control */

    public:

        JackInternalClient(JackServer* server, JackSynchro* table);
        virtual ~JackInternalClient();

        int Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status);
        void ShutDown();

        JackGraphManager* GetGraphManager() const;
        JackEngineControl* GetEngineControl() const;
        JackClientControl* GetClientControl() const;

        static JackGraphManager* fGraphManager;         /*! Shared memory Port manager */
        static JackEngineControl* fEngineControl;       /*! Shared engine cotrol */
};

/*!
\brief Loadable internal clients in the server.
*/

typedef int (*InitializeCallback)(jack_client_t*, const char*);
typedef int (*InternalInitializeCallback)(jack_client_t*, const JSList* params);
typedef void (*FinishCallback)(void *);

class JackLoadableInternalClient : public JackInternalClient
{

    protected:

        JACK_HANDLE fHandle;
        FinishCallback fFinish;
        JackDriverDescFunction fDescriptor;

    public:

        JackLoadableInternalClient(JackServer* server, JackSynchro* table)
            :JackInternalClient(server, table), fHandle(NULL), fFinish(NULL), fDescriptor(NULL)
        {}
        virtual ~JackLoadableInternalClient();

        virtual int Init(const char* so_name);

};

class JackLoadableInternalClient1 : public JackLoadableInternalClient
{

    private:

        InitializeCallback fInitialize;
        char fObjectData[JACK_LOAD_INIT_LIMIT];

    public:

        JackLoadableInternalClient1(JackServer* server, JackSynchro* table, const char* object_data);
        virtual ~JackLoadableInternalClient1()
        {}

        int Init(const char* so_name);
        int Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status);

};

class JackLoadableInternalClient2 : public JackLoadableInternalClient
{

    private:

        InternalInitializeCallback fInitialize;
        const JSList* fParameters;

    public:

        JackLoadableInternalClient2(JackServer* server, JackSynchro* table, const JSList*  parameters);
        virtual ~JackLoadableInternalClient2()
        {}

        int Init(const char* so_name);
        int Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status);

};


} // end of namespace

#endif
