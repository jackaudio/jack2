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

#ifndef __JackMachPort__
#define __JackMachPort__

#include <mach/mach.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mach_error.h>
#include <servers/bootstrap.h>

namespace Jack
{

/*!
\brief Mach port.
*/

class JackMachPort
{

    protected:

        mach_port_t fBootPort;
        mach_port_t fServerPort;

    public:

        JackMachPort():fBootPort(0), fServerPort(0)
        {}
        virtual ~JackMachPort()
        {}

        virtual bool AllocatePort(const char* name);
        virtual bool AllocatePort(const char* name, int queue);
        virtual bool ConnectPort(const char* name);
        virtual bool DisconnectPort();
        virtual bool DestroyPort();
        virtual mach_port_t GetPort();
        virtual void SetPort(mach_port_t port);
};

/*!
\brief Mach port set.
*/

class JackMachPortSet : public JackMachPort
{

    private:

        mach_port_t fPortSet;

    public:

        JackMachPortSet():fPortSet(0)
        {}
        virtual ~JackMachPortSet()
        {}

        bool AllocatePort(const char* name);
        bool AllocatePort(const char* name, int queue);
        bool DisconnectPort();
        bool DestroyPort();
        mach_port_t GetPortSet();
        mach_port_t AddPort();
};

} // end of namespace

#endif

