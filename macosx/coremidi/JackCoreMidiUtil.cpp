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

#include <sstream>

#include "JackError.h"
#include "JackCoreMidiUtil.h"

std::string
Jack::GetMacOSErrorString(OSStatus status)
{
    const char *message = GetMacOSStatusErrorString(status);
    if (! message) {
        std::stringstream stream;
        stream << "error (code: '" << status << "')";
        return stream.str();
    }
    return std::string(message);
}

void
Jack::WriteMacOSError(const char *jack_function, const char *mac_function,
                      OSStatus status)
{
    jack_error("%s - %s: %s", jack_function, mac_function,
               GetMacOSErrorString(status).c_str());
}
