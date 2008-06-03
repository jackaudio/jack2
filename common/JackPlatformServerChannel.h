/*
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

#ifndef __JackPlatformServerChannel__
#define __JackPlatformServerChannel___

#ifdef HAVE_CONFIG_H
#include "config.h" 
#endif 

// OSX
#if defined(__APPLE__)
#include "JackMachServerChannel.h"
#endif

// WINDOWS
#ifdef WIN32
#include "JackWinNamedPipeServerChannel.h"
#endif

// LINUX
#ifdef __linux__
#include "JackSocketServerChannel.h"
#endif

namespace Jack
{

#ifdef WIN32
typedef JackWinNamedPipeServerChannel JackServerChannel;

#elif defined(SOCKET_RPC_POSIX_SEMA) || defined(SOCKET_RPC_FIFO_SEMA) || defined(SOCKET_RPC_FIFO_SEMA_DUMMY) \
    || defined(SOCKET_RPC_MACH_SEMA)
typedef JackSocketServerChannel JackServerChannel;

#elif defined(MACH_RPC_MACH_SEMA) || defined(MACH_RPC_FIFO_SEMA) || defined(MACH_RPC_POSIX_SEMA)
typedef JackMachServerChannel JackServerChannel;

#endif

} // end of namespace

#endif /* __JackPlatformServerChannel__ */
