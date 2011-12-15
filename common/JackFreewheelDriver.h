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

#ifndef __JackFreewheelDriver__
#define __JackFreewheelDriver__

#include "JackDriver.h"

namespace Jack
{

/*!
\brief The FreeWheel driver : run Jack engine at full speed.
*/

class JackFreewheelDriver : public JackDriver
{
    protected:

        int SuspendRefNum();

    public:

        JackFreewheelDriver(JackLockedEngine* engine, JackSynchro* table): JackDriver("freewheel", "", engine, table)
        {}
        virtual ~JackFreewheelDriver()
        {}

        bool IsRealTime() const
        {
            return false;
        }

        int Process();

        int ProcessReadSync();
        int ProcessWriteSync();

        int ProcessReadAsync();
        int ProcessWriteAsync();

};

} // end of namespace


#endif
