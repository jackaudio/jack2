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

#include "JackGlobals.h"

// OSX
#if defined(__APPLE__)
#include "JackCoreAudioDriver.h"
#include "JackMachServerNotifyChannel.h"
#include "JackMachNotifyChannel.h"
#include "JackMachServerChannel.h"
#include "JackMachClientChannel.h"
#include "JackMachThread.h"
#include "JackMachSemaphore.h"
#include "JackProcessSync.h"
#include "JackSocketServerNotifyChannel.h"
#include "JackSocketNotifyChannel.h"
#include "JackSocketServerChannel.h"
#include "JackSocketClientChannel.h"
#include "JackPosixThread.h"
#include "JackPosixSemaphore.h"
#include "JackFifo.h"
#endif

// WINDOWS
#ifdef WIN32
#include "JackWinProcessSync.h"
#include "JackWinNamedPipeClientChannel.h"
#include "JackWinEvent.h"
#include "JackWinSemaphore.h"
#include "JackWinThread.h"
#endif

// LINUX
#ifdef __linux__
#include "linux/alsa/JackAlsaDriver.h"
#include "JackProcessSync.h"
#include "JackSocketServerNotifyChannel.h"
#include "JackSocketNotifyChannel.h"
#include "JackSocketServerChannel.h"
#include "JackSocketClientChannel.h"
#include "JackPosixThread.h"
#include "JackPosixSemaphore.h"
#include "JackFifo.h"
#endif


using namespace std;

namespace Jack
{

#ifdef WIN32

JackSynchro* JackFactoryWindowsClient::MakeSynchro()
{
    return new JackWinSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryWindowsClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryWindowsClient::MakeClientChannel()
{
    return new JackWinNamedPipeClientChannel();
}

JackNotifyChannelInterface* JackFactoryWindowsClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryWindowsClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryWindowsClient::MakeInterProcessSync()
{
    return new JackWinProcessSync();
}

JackThread* JackFactoryWindowsClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackWinThread(runnable);
}
#endif

#ifdef __linux__

#if defined(SOCKET_RPC_POSIX_SEMA)
JackSynchro* JackFactoryLinuxClient::MakeSynchro()
{
    return new JackPosixSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryLinuxClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryLinuxClient::MakeClientChannel()
{
    return new JackSocketClientChannel();
}

JackNotifyChannelInterface* JackFactoryLinuxClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryLinuxClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryLinuxClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryLinuxClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackPosixThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_FIFO_SEMA)
JackSynchro* JackFactoryLinuxClient::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryLinuxClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryLinuxClient::MakeClientChannel()
{
    return new JackSocketClientChannel();
}

JackNotifyChannelInterface* JackFactoryLinuxClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryLinuxClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryLinuxClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryLinuxClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackPosixThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_FIFO_SEMA_DUMMY)
JackSynchro* JackFactoryLinuxClient::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryLinuxClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryLinuxClient::MakeClientChannel()
{
    return new JackSocketClientChannel();
}

JackNotifyChannelInterface* JackFactoryLinuxClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryLinuxClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryLinuxClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryLinuxClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackPosixThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#endif

#if defined(__APPLE__)

#if defined(MACH_RPC_MACH_SEMA)
// Mach RPC + Mach Semaphore
JackSynchro* JackFactoryOSXClient::MakeSynchro()
{
    return new JackMachSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryOSXClient::MakeClientChannel()
{
    return new JackMachClientChannel();
}

JackNotifyChannelInterface* JackFactoryOSXClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryOSXClient::MakeServerChannel()
{
    return NULL;
} // Not used

JackSyncInterface* JackFactoryOSXClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_POSIX_SEMA)
// Socket RPC + Posix Semaphore
JackSynchro* JackFactoryOSXClient::MakeSynchro()
{
    return new JackPosixSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryOSXClient::MakeClientChannel()
{
    return new JackSocketClientChannel();
}

JackNotifyChannelInterface* JackFactoryOSXClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryOSXClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryOSXClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_FIFO_SEMA)
// Socket RPC + Fifo Semaphore
JackSynchro* JackFactoryOSXClient::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryOSXClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryOSXClient::MakeClientChannel()
{
    return new JackSocketClientChannel();
}

JackNotifyChannelInterface* JackFactoryOSXClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryOSXClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryOSXClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}

#endif

#if defined(MACH_RPC_FIFO_SEMA)
// Mach RPC + Fifo Semaphore
JackSynchro* JackFactoryOSXClient::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryOSXClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryOSXClient::MakeClientChannel()
{
    return new JackMachClientChannel();
}

JackNotifyChannelInterface* JackFactoryOSXClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryOSXClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryOSXClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(MACH_RPC_POSIX_SEMA)
// Mach RPC + Posix Semaphore
JackSynchro* JackFactoryOSXClient::MakeSynchro()
{
    return new JackPosixSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryOSXClient::MakeClientChannel()
{
    return new JackMachClientChannel();
}

JackNotifyChannelInterface* JackFactoryOSXClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryOSXClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryOSXClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined SOCKET_RPC_MACH_SEMA
// Socket RPC + Mach Semaphore
JackSynchro* JackFactoryOSXClient::MakeSynchro()
{
    return new JackMachSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXClient::MakeServerNotifyChannel()
{
    return NULL;
}
// Not used
JackClientChannelInterface* JackFactoryOSXClient::MakeClientChannel()
{
    return new JackSocketClientChannel();
}

JackNotifyChannelInterface* JackFactoryOSXClient::MakeNotifyChannel()
{
    return NULL;
}
// Not used
JackServerChannelInterface* JackFactoryOSXClient::MakeServerChannel()
{
    return NULL;
}
// Not used
JackSyncInterface* JackFactoryOSXClient::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXClient::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#endif

} // end of namespace


