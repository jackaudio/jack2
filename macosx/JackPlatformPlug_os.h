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

#ifndef __JackPlatformPlug_APPLE__
#define __JackPlatformPlug_APPLE__

#include <TargetConditionals.h>

#define jack_server_dir "/tmp"
#define jack_client_dir "/tmp"
#define JACK_DEFAULT_DRIVER "coreaudio"

namespace Jack
{
    struct JackRequest;
    struct JackResult;

    class JackPosixMutex;
    class JackMachThread;
    class JackMachFutex;

    class JackSocketServerChannel;
    class JackSocketClientChannel;
    class JackSocketServerNotifyChannel;
    class JackSocketNotifyChannel;

	class JackNetUnixSocket;
}

/* __JackPlatformMutex__ */
#include "JackPosixMutex.h"
namespace Jack { typedef JackPosixMutex JackMutex; }

/* __JackPlatformThread__ */
#include "JackMachThread.h"
namespace Jack { typedef JackMachThread JackThread; }

/* __JackPlatformSynchro__  client activation */
#include "JackMachFutex.h"
namespace Jack { typedef JackMachFutex JackSynchro; }

/* __JackPlatformProcessSync__ */
#include "JackPosixProcessSync.h"
namespace Jack { typedef JackPosixProcessSync JackProcessSync; }

/* __JackPlatformServerChannel__ */
#include "JackSocketServerChannel.h"
namespace Jack { typedef JackSocketServerChannel JackServerChannel; }

/* __JackPlatformClientChannel__ */
#include "JackSocketClientChannel.h"
namespace Jack { typedef JackSocketClientChannel JackClientChannel; }

/* __JackPlatformServerNotifyChannel__ */
#include "JackSocketServerNotifyChannel.h"
namespace Jack { typedef JackSocketServerNotifyChannel JackServerNotifyChannel; }

/* __JackPlatformNotifyChannel__ */
#include "JackSocketNotifyChannel.h"
namespace Jack { typedef JackSocketNotifyChannel JackNotifyChannel; }

/* __JackPlatformNetSocket__ */
#include "JackNetUnixSocket.h"
namespace Jack { typedef JackNetUnixSocket JackNetSocket; }

#endif
