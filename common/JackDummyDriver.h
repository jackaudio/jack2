/*
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

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

#ifndef __JackDummyDriver__
#define __JackDummyDriver__

#include "JackTimedDriver.h"

namespace Jack
{

/*!
\brief The dummy driver.
*/

class JackDummyDriver : public JackTimedDriver
{

    public:

        JackDummyDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
                : JackTimedDriver(name, alias, engine, table)
        {}
        virtual ~JackDummyDriver()
        {}

        virtual int Process()
        {
            JackDriver::CycleTakeBeginTime();

            if (JackAudioDriver::Process() < 0) {
                return -1;
            } else {
                ProcessWait();
                return 0;
            }
        }

};

} // end of namespace

#endif
