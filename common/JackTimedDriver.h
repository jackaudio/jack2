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

#ifndef __JackTimedDriver__
#define __JackTimedDriver__

#include "JackAudioDriver.h"

namespace Jack
{

/*!
\brief The timed driver.
*/

class SERVER_EXPORT JackTimedDriver : public JackAudioDriver
{
    protected:

        int fCycleCount;
        jack_time_t fAnchorTimeUsec;

        int FirstCycle(jack_time_t cur_time);
        int CurrentCycle(jack_time_t cur_time);

        void ProcessWait();

    public:

        JackTimedDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
                : JackAudioDriver(name, alias, engine, table), fCycleCount(0), fAnchorTimeUsec(0)
        {}
        virtual ~JackTimedDriver()
        {}

        // BufferSize can be changed
        bool IsFixedBufferSize()
        {
            return false;
        }

        int Start();

};

class SERVER_EXPORT JackWaiterDriver : public JackTimedDriver
{

    public:

        JackWaiterDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
            : JackTimedDriver(name, alias, engine, table)
        {}
        virtual ~JackWaiterDriver()
        {}

        virtual int ProcessNull();

};

} // end of namespace

#endif
