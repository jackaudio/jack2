/*
 Copyright (C) 2014 Cédric Schieli

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __JackWaitCallbackDriver__
#define __JackWaitCallbackDriver__

#include "JackWaitThreadedDriver.h"

namespace Jack
{

/*!
\brief Wrapper for a restartable non-threaded driver (e.g. JackProxyDriver).

Simply ends its thread when the decorated driver Initialize method returns.
Self register with the supplied JackRestarterDriver so it can restart the thread.
*/

class SERVER_EXPORT JackWaitCallbackDriver : public JackWaitThreadedDriver
{
    public:

        JackWaitCallbackDriver(JackRestarterDriver* driver);

    protected:

        bool ExecuteReal();
};


} // end of namespace


#endif
