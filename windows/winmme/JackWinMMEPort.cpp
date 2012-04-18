/*
Copyright (C) 2011 Devin Anderson

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

#include <memory>
#include <stdexcept>
#include <stdio.h>

#include "JackWinMMEPort.h"
#include "JackError.h"

using Jack::JackWinMMEPort;

///////////////////////////////////////////////////////////////////////////////
// Class
///////////////////////////////////////////////////////////////////////////////

JackWinMMEPort::JackWinMMEPort()
{}

JackWinMMEPort::~JackWinMMEPort()
{}

const char *
JackWinMMEPort::GetAlias()
{
     return alias;
}

const char *
JackWinMMEPort::GetName()
{
     return name;
}

void
JackWinMMEPort::GetOSErrorString(LPTSTR text)
{
    DWORD error = GetLastError();
    if (! FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), text,
                        MAXERRORLENGTH, NULL)) {
        snprintf(text, MAXERRORLENGTH, "Unknown OS error code '%ld'", error);
    }
}

void
JackWinMMEPort::WriteOSError(const char *jack_func, const char *os_func)
{
    char error_message[MAXERRORLENGTH];
    GetOSErrorString(error_message);
    jack_error("%s - %s: %s", jack_func, os_func, error_message);
}

