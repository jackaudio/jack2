/*
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

        JackMachPort()
        {}
        virtual ~JackMachPort()
        {}

        virtual bool AllocatePort(const char* name);
        virtual bool AllocatePort(const char* name, int queue);
        virtual bool ConnectPort(const char* name);
        virtual bool DisconnectPort();
        virtual bool DestroyPort();
        virtual mach_port_t GetPort();
};

/*!
\brief Mach port set.
*/

class JackMachPortSet : public JackMachPort
{

    private:

        mach_port_t fPortSet;

    public:

        JackMachPortSet()
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

