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

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

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

    /*!
    \brief Generic monitoring class. Saves data to GnuPlot files ('.plt' and '.log' datafile)

    This template class allows to manipulate monitoring records, and automatically generate the GnuPlot config and data files.
    Operations are RT safe because it uses fixed size data buffers.
    You can set the number of measure points, and the number of records.

    To use it :
    - create a JackGnuPlotMonitor, you can use the data type you want.
    - create a temporary array for your measure
    - once you have filled this array with 'measure points' value, call write() to add it to the record
    - once you've done with your measurment, just call save() to save your data file

    You can also call SetPlotFile() to automatically generate '.plt' file from an options list.

    */

    template <class T> class JackGnuPlotMonitor
    {
        private:
            uint32_t fMeasureCnt;
            uint32_t fMeasurePoints;
            uint32_t fMeasureId;
            T* fCurrentMeasure;
            T** fMeasureTable;
            uint32_t fTablePos;
            std::string fName;

        public:

            JackGnuPlotMonitor(uint32_t measure_cnt = 512, uint32_t measure_points = 5, std::string name = std::string("default"));

            ~JackGnuPlotMonitor();

            T AddNew(T measure_point);

			uint32_t New();

            T Add(T measure_point);

            uint32_t AddLast(T measure_point);

            uint32_t Write();

            int Save(std::string name = std::string(""));

            int SetPlotFile(std::string* options_list = NULL, uint32_t options_number = 0,
                            std::string* field_names = NULL, uint32_t field_number = 0,
                            std::string name = std::string(""));
    };

    void BuildClientPath(char* path_to_so, int path_len, const char* so_name);
    void PrintLoadError(const char* so_name);

}

#endif
