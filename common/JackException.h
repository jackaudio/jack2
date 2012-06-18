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

#include "JackCompilerDeps.h"

#include <stdexcept>
#include <iostream>
#include <string>

namespace Jack
{

#define	ThrowIf(inCondition, inException)                                               \
			if(inCondition)																\
			{																			\
				throw(inException);														\
			}


/*!
\brief Exception base class.
*/

class SERVER_EXPORT JackException : public std::runtime_error {

    public:

        JackException(const std::string& msg) : std::runtime_error(msg)
        {}
        JackException(char* msg) : std::runtime_error(msg)
        {}
        JackException(const char* msg) : std::runtime_error(msg)
        {}

        std::string Message()
        {
            return what();
        }

        void PrintMessage();

 };

/*!
 \brief Exception thrown by JackEngine in temporary mode.
 */

class SERVER_EXPORT JackTemporaryException : public JackException {

    public:

        JackTemporaryException(const std::string& msg) : JackException(msg)
        {}
        JackTemporaryException(char* msg) : JackException(msg)
        {}
        JackTemporaryException(const char* msg) : JackException(msg)
        {}
        JackTemporaryException() : JackException("")
        {}
};

/*!
 \brief
 */

class SERVER_EXPORT JackQuitException : public JackException {

    public:

        JackQuitException(const std::string& msg) : JackException(msg)
        {}
        JackQuitException(char* msg) : JackException(msg)
        {}
        JackQuitException(const char* msg) : JackException(msg)
        {}
        JackQuitException() : JackException("")
        {}
};

/*!
\brief Exception possibly thrown by Net slaves.
*/

class SERVER_EXPORT JackNetException : public JackException {

    public:

        JackNetException(const std::string& msg) : JackException(msg)
        {}
        JackNetException(char* msg) : JackException(msg)
        {}
        JackNetException(const char* msg) : JackException(msg)
        {}
        JackNetException() : JackException("")
        {}
};

}

#endif
