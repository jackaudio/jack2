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

#ifndef __JackLoopbackDriver__
#define __JackLoopbackDriver__

#include "JackAudioDriver.h"

namespace Jack
{

/*!
\brief The loopback driver : to be used to "pipeline" applications connected in sequence.
*/

class JackLoopbackDriver : public JackAudioDriver
{

    private:

        virtual int ProcessReadSync();
        virtual int ProcessWriteSync();

        virtual int ProcessReadAsync();
        virtual int ProcessWriteAsync();

    public:

        JackLoopbackDriver(JackLockedEngine* engine, JackSynchro* table)
                : JackAudioDriver("loopback", "loopback", engine, table)
        {}
        virtual ~JackLoopbackDriver()
        {}
};

} // end of namespace

#endif
