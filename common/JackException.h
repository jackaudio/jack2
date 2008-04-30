/*
Copyright (C) 2008 Grame

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

#ifndef __JackException__
#define __JackException__

#include <stdexcept>
#include <iostream>
#include <string>
#include "JackError.h"

namespace Jack 
{

    class JackException : public std::runtime_error {
    
        public:
        
            JackException(const std::string& msg) : runtime_error(msg) 
            {}
            JackException(const char* msg) : runtime_error(msg) 
            {}
            
            std::string Message() 
            {
                return what();
            }

            void PrintMessage() 
            {
                std::string str = what();
                jack_error(str.c_str());
            }
    };
    
    class JackDriverException : public JackException {
    
        public:
        
           JackDriverException(const std::string& msg) : JackException(msg) 
           {}
           JackDriverException(const char* msg) : JackException(msg) 
           {}
    };

} 

#endif 
