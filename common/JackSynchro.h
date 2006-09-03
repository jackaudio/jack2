/*
Copyright (C) 2004-2006 Grame  

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

#ifndef __JackSynchro__
#define __JackSynchro__

#include "JackError.h"

#define SYNC_MAX_NAME_SIZE 256

namespace Jack
{

/*!
\brief An inter process synchronization primitive.
*/

class JackSynchro
{

    protected:

        char fName[SYNC_MAX_NAME_SIZE];
        bool fFlush; // If true, signal are "flushed" : used for drivers that do no consume the signal

        virtual void BuildName(const char* name, char* res)
        {}

    public:

        JackSynchro(): fFlush(false)
        {}
        virtual ~JackSynchro()
        {}

        virtual bool Signal()
        {
            return true;
        }
        virtual bool SignalAll()
        {
            return true;
        }
        virtual bool Wait()
        {
            return true;
        }
        virtual bool TimedWait(long usec)
        {
            return true;
        }
        virtual bool Allocate(const char* name, int value)
        {
            return true;
        }
        virtual bool Connect(const char* name)
        {
            return true;
        }
        virtual bool ConnectInput(const char* name)
        {
            return true;
        }
        virtual bool ConnectOutput(const char* name)
        {
            return true;
        }
        virtual bool Disconnect()
        {
            return true;
        }
        virtual void Destroy()
        {}

        void SetFlush(bool mode)
        {
            fFlush = mode;
        }

};


} // end of namespace

#endif

