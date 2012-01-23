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
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#endif

#include "jslist.h"
#include "JackCompilerDeps.h"

namespace Jack
{

    /*!
    \brief Utility functions.
    */

    struct SERVER_EXPORT JackTools
    {
        static int GetPID();
        static int GetUID();

        static void KillServer();

        static int MkDir(const char* path);
        /* returns the name of the per-user subdirectory of jack_tmpdir */
        static char* UserDir();
        /* returns the name of the per-server subdirectory of jack_user_dir() */
        static char* ServerDir(const char* server_name, char* server_dir);
        static const char* DefaultServerName();
        static void CleanupFiles(const char* server_name);
        static int GetTmpdir();
        static void RewriteName(const char* name, char* new_name);
        static void ThrowJackNetException();

        // For OSX only
        static int ComputationMicroSec(int buffer_size)
        {
            if (buffer_size < 128) {
                return 500;
            } else if (buffer_size < 256) {
                return 300;
            } else {
                return 100;
            }
        }
    };

    void BuildClientPath(char* path_to_so, int path_len, const char* so_name);
    void PrintLoadError(const char* so_name);

}

#endif
