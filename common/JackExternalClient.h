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

#ifndef __JackExternalClient__
#define __JackExternalClient__

#include "JackClientInterface.h"
#include "JackPlatformPlug.h"

namespace Jack
{

struct JackClientControl;

/*!
\brief Server side implementation of library clients.
*/

class JackExternalClient : public JackClientInterface
{

    private:

        JackNotifyChannel fChannel;           /*! Server/client communication channel */
        JackClientControl* fClientControl;    /*! Client control in shared memory     */

    public:

        JackExternalClient();
        virtual ~JackExternalClient();

        int Open(const char* name, int pid, int refnum, int uuid, int* shared_client);
        int Close();

        int ClientNotify(int refnum, const char* name, int notify, int sync, const char* message, int value1, int value2);

        JackClientControl* GetClientControl() const;
};


} // end of namespace

#endif
