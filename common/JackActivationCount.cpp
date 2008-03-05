/*
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

#include "JackAtomic.h"
#include "JackActivationCount.h"
#include "JackConstants.h"
#include "JackClientControl.h"
#include "JackError.h"

namespace Jack
{

bool JackActivationCount::Signal(JackSynchro* synchro, JackClientControl* control)
{
    if (fValue == 0) {
        // Transfer activation to next clients
        jack_error("JackActivationCount::Signal value = 0 ref = %ld", control->fRefNum);
        return synchro->Signal();
    } else if (DEC_ATOMIC(&fValue) == 1) {
        return synchro->Signal();
    } else {
        return true;
    }
}

} // end of namespace

