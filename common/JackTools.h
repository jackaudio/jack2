/*
  Copyright (C) 2006-2008 Grame

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

#ifndef __JackTools__
#define __JackTools__

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

#include <string>
#include <algorithm>
#include <vector>
#include "jslist.h"
#include "driver_interface.h"

#include "JackExports.h"

namespace Jack
{

struct EXPORT JackTools
{

    static int GetPID();
    static int GetUID();

    static char* UserDir();
    static char* ServerDir(const char* server_name, char* server_dir);
    static const char* DefaultServerName();
    static void CleanupFiles(const char* server_name);
    static int GetTmpdir();
    static void RewriteName(const char* name, char* new_name);

};

class EXPORT JackArgParser
{
    private:
        std::string fArgString;
        int fNumArgv;
        int fArgc;
        char** fArgv;

    public:
        JackArgParser(const char* arg);
        ~JackArgParser();
         std::string GetArgString();
        int GetNumArgv();
        int GetArgc();
        const char** GetArgv();
        int ParseParams ( jack_driver_desc_t* desc, JSList** param_list );
};

}

#endif
