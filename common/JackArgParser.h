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

#ifndef __JackArgParser__
#define __JackArgParser__

#include "jslist.h"
#include "driver_interface.h"
#include "JackCompilerDeps.h"
#include "JackError.h"

#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>

namespace Jack
{

    class SERVER_EXPORT JackArgParser
    {
        private:

            std::string fArgString;
            int fArgc;
            std::vector<std::string> fArgv;

        public:

            JackArgParser ( const char* arg );
            ~JackArgParser();
            std::string GetArgString();
            int GetNumArgv();
            int GetArgc();
            int GetArgv ( std::vector<std::string>& argv );
            int GetArgv ( char** argv );
            void DeleteArgv ( const char** argv );
            bool ParseParams ( jack_driver_desc_t* desc, JSList** param_list );
            void FreeParams ( JSList* param_list );
    };

}

#endif
