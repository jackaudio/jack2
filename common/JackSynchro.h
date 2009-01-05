/*
Copyright (C) 2004-2008 Grame

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

#ifndef __JackSynchro__
#define __JackSynchro__

#define SYNC_MAX_NAME_SIZE 256

namespace Jack
{

namespace detail
{

/*!
\brief An inter process synchronization primitive.
*/

class JackSynchro
{

    protected:

        char fName[SYNC_MAX_NAME_SIZE];
        bool fFlush; // If true, signal are "flushed" : used for drivers that do no consume the signal

        void BuildName(const char* name, const char* server_name, char* res)
        {}

    public:

        JackSynchro(): fFlush(false)
        {}
        ~JackSynchro()
        {}

        bool Signal()
        {
            return true;
        }
        bool SignalAll()
        {
            return true;
        }
        bool Wait()
        {
            return true;
        }
        bool TimedWait(long usec)
        {
            return true;
        }
        bool Allocate(const char* name, const char* server_name, int value)
        {
            return true;
        }
        bool Connect(const char* name, const char* server_name)
        {
            return true;
        }
        bool ConnectInput(const char* name, const char* server_name)
        {
            return true;
        }
        bool ConnectOutput(const char* name, const char* server_name)
        {
            return true;
        }
        bool Disconnect()
        {
            return true;
        }
        void Destroy()
        {}

        void SetFlush(bool mode)
        {
            fFlush = mode;
        }

};

}

} // end of namespace

#endif

