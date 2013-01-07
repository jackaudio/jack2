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

#ifndef __JackActivationCount__
#define __JackActivationCount__

#include "JackPlatformPlug.h"
#include "JackTime.h"
#include "JackTypes.h"

namespace Jack
{

struct JackClientControl;

/*!
\brief Client activation counter.
*/

/* Note: This class must be kept 32/64 clean! */
class JackActivationCount
{

    private:

        SInt32 fValue;
        SInt32 fCount;

    public:

        JackActivationCount(): fValue(0), fCount(0)
        {}

        bool Signal(JackSynchro* synchro, JackClientControl* control);

        inline void Reset()
        {
            fValue = fCount;
        }

        inline void SetValue(int val)
        {
            fCount = val;
        }

        inline void IncValue()
        {
            fCount++;
        }

        inline void DecValue()
        {
            fCount--;
        }

        inline int GetValue() const
        {
            return fValue;
        }

};

} // end of namespace


#endif
