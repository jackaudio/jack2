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

#ifndef __JackLibClient__
#define __JackLibClient__

#include "JackClient.h"
#include "JackShmMem.h"
#include "JackClientControl.h"
#include "JackEngineControl.h"

namespace Jack
{

/*!
\brief Client on the library side.
*/

class JackLibClient : public JackClient
{

    private:

        JackShmReadWritePtr1<JackClientControl> fClientControl; /*! Shared client control */

    public:

        JackLibClient(JackSynchro* table);
        virtual ~JackLibClient();

        int Open(const char* server_name, const char* name, int uuid, jack_options_t options, jack_status_t* status);
        void ShutDown();

        int ClientNotifyImp(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);

        JackGraphManager* GetGraphManager() const;
        JackEngineControl* GetEngineControl() const;
        JackClientControl* GetClientControl() const;
};


} // end of namespace

#endif

