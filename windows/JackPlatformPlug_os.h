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


#ifndef __JackPlatformPlug_WIN32__
#define __JackPlatformPlug_WIN32__

#define jack_server_dir "server"
#define jack_client_dir "client"
#define JACK_DEFAULT_DRIVER "portaudio"
#define JACK_LOCATION "C:/Program Files/Jack"

#ifndef ADDON_DIR
    #define ADDON_DIR "jack"
#endif

namespace Jack
{
    struct JackRequest;
	struct JackResult;

	class JackWinMutex;
	class JackWinThread;
	class JackWinSemaphore;
	class JackWinProcessSync;
	class JackWinNamedPipeServerChannel;
	class JackWinNamedPipeClientChannel;
	class JackWinNamedPipeServerNotifyChannel;
	class JackWinNamedPipeNotifyChannel;
	class JackWinNamedPipe;
	class JackNetWinSocket;
}

/* __JackPlatformMutex__ */
#include "JackWinMutex.h"
namespace Jack {typedef JackWinMutex JackMutex; }

/* __JackPlatformThread__ */
#include "JackWinThread.h"
namespace Jack { typedef JackWinThread JackThread; }

/* __JackPlatformSynchro__  client activation */
#include "JackWinSemaphore.h"
namespace Jack { typedef JackWinSemaphore JackSynchro; }

/* __JackPlatformChannelTransaction__ */
#include "JackWinNamedPipe.h"
namespace Jack { typedef JackWinNamedPipe JackChannelTransaction; }

/* __JackPlatformProcessSync__ */
#include "JackWinProcessSync.h"
namespace Jack { typedef JackWinProcessSync JackProcessSync; }

/* __JackPlatformServerChannel__ */
#include "JackWinNamedPipeServerChannel.h"
namespace Jack { typedef JackWinNamedPipeServerChannel JackServerChannel; }

/* __JackPlatformClientChannel__ */
#include "JackWinNamedPipeClientChannel.h"
namespace Jack { typedef JackWinNamedPipeClientChannel JackClientChannel; }

/* __JackPlatformServerNotifyChannel__ */
#include "JackWinNamedPipeServerNotifyChannel.h"
namespace Jack { typedef JackWinNamedPipeServerNotifyChannel JackServerNotifyChannel; }

/* __JackPlatformNotifyChannel__ */
#include "JackWinNamedPipeNotifyChannel.h"
namespace Jack { typedef JackWinNamedPipeNotifyChannel JackNotifyChannel; }

/* __JackPlatformNetSocket__ */
#include "JackNetWinSocket.h"
namespace Jack { typedef JackNetWinSocket JackNetSocket; }

#endif
