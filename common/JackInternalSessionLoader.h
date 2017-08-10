/*
Copyright (C) 2017 Timo Wischer

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

#ifndef __JackInternalSessionLoader__
#define __JackInternalSessionLoader__

#include <string>
#include <sstream>
#include "JackServer.h"


namespace Jack
{

class JackInternalSessionLoader
{
public:
    JackInternalSessionLoader(JackServer* const server);
    int Load(const char* file);

private:
    void LoadClient(std::istringstream& iss, const int linenr);
    void ConnectPorts(std::istringstream& iss, const int linenr);

    JackServer* const fServer;
};

} // end of namespace

#endif
