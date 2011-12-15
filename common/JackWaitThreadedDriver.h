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

#ifndef __JackWaitThreadedDriver__
#define __JackWaitThreadedDriver__

#include "JackThreadedDriver.h"
#include "JackTimedDriver.h"

namespace Jack
{

/*!
\brief To be used as a wrapper of JackNetDriver.

The idea is to behave as the "dummy" driver, until the network connection is really started and processing starts.
The Execute method will call the Process method from the base JackTimedDriver, until the decorated driver Init method returns.
A helper JackDriverStarter thread is used for that purpose.
*/

class SERVER_EXPORT JackWaitThreadedDriver : public JackThreadedDriver
{
    private:

        struct JackDriverStarter : public JackRunnableInterface
        {

                JackDriver* fDriver;
                JackThread fThread;
                volatile bool fRunning;

                JackDriverStarter(JackDriver* driver)
                    :fDriver(driver), fThread(this), fRunning(false)
                {}

                ~JackDriverStarter()
                {
                     fThread.Kill();
                }

                int Start()
                {
                    fRunning = false;
                    return fThread.Start();
                }

                // JackRunnableInterface interface
                bool Execute()
                {
                    // Blocks until decorated driver is started (that is when it's Initialize method returns).
                    if (fDriver->Initialize()) {
                        fRunning = true;
                    } else {
                        jack_error("Initing net driver fails...");
                    }

                    return false;
                }

        };

        JackDriverStarter fStarter;

    public:

        JackWaitThreadedDriver(JackDriver* net_driver)
            : JackThreadedDriver(net_driver), fStarter(net_driver)
        {}
        virtual ~JackWaitThreadedDriver()
        {}

        // JackRunnableInterface interface
        bool Init();
        bool Execute();
};


} // end of namespace


#endif
