/*
Copyright (C) 2008 Romain Moret at Grame

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

#ifndef __JackPlatformNetSocket__
#define __JackPlatformNetSocket__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// OSX and LINUX
#if defined(__APPLE__) || defined (__linux__)
#include "JackNetUnixSocket.h"
#endif

// WINDOWS
#ifdef WIN32
#include "JackNetWinSocket.h"
#endif

namespace Jack
{

#if defined(__APPLE__) || defined(__linux__)
	typedef JackNetUnixSocket JackNetSocket;
#endif

#ifdef WIN32
	typedef JackNetWinSocket JackNetSocket;
#endif

} // end of namespace

#endif /* __JackPlatformNetSocket__ */
