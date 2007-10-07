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

#ifdef WIN32 
#pragma warning (disable : 4786)
#endif

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
#include "JackWinNamedPipeServerNotifyChannel.h"
#include "JackWinNamedPipeNotifyChannel.h"
#include "JackWinNamedPipeServerChannel.h"
#include "JackWinNamedPipeClientChannel.h"
#include "JackWinEvent.h"
#include "JackWinThread.h"
#endif

// LINUX
#ifdef __linux__
#include "JackAlsaDriver.h"
#include "JackProcessSync.h"
#include "JackSocketServerNotifyChannel.h"
#include "JackSocketNotifyChannel.h"
#include "JackSocketServerChannel.h"
#include "JackSocketClientChannel.h"
#include "JackPosixThread.h"
#include "JackPosixSemaphore.h"
#include "JackFifo.h"
#endif

// COMMON
#include "JackDummyDriver.h"
#include "JackAudioDriver.h"


using namespace std;

namespace Jack
{

#ifdef WIN32
JackSynchro* JackFactoryWindowsServer::MakeSynchro()
{
    return new JackWinEvent();
}

JackServerNotifyChannelInterface* JackFactoryWindowsServer::MakeServerNotifyChannel()
{
    return new JackWinNamedPipeServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryWindowsServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryWindowsServer::MakeNotifyChannel()
{
    return new JackWinNamedPipeNotifyChannel();
}

JackServerChannelInterface* JackFactoryWindowsServer::MakeServerChannel()
{
    return new JackWinNamedPipeServerChannel();
}

JackSyncInterface* JackFactoryWindowsServer::MakeInterProcessSync()
{
    return new JackWinProcessSync();
}

JackThread* JackFactoryWindowsServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackWinThread(runnable);
}
#endif

#ifdef __linux__

#if defined(SOCKET_RPC_POSIX_SEMA)
JackSynchro* JackFactoryLinuxServer::MakeSynchro()
{
    return new JackPosixSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryLinuxServer::MakeServerNotifyChannel()
{
    return new JackSocketServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryLinuxServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryLinuxServer::MakeNotifyChannel()
{
    return new JackSocketNotifyChannel();
}

JackServerChannelInterface* JackFactoryLinuxServer::MakeServerChannel()
{
    return new JackSocketServerChannel();
}

JackSyncInterface* JackFactoryLinuxServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryLinuxServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackPosixThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_FIFO_SEMA)
JackSynchro* JackFactoryLinuxServer::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryLinuxServer::MakeServerNotifyChannel()
{
    return new JackSocketServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryLinuxServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryLinuxServer::MakeNotifyChannel()
{
    return new JackSocketNotifyChannel();
}

JackServerChannelInterface* JackFactoryLinuxServer::MakeServerChannel()
{
    return new JackSocketServerChannel();
}

JackSyncInterface* JackFactoryLinuxServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryLinuxServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackPosixThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_FIFO_SEMA_DUMMY)
JackSynchro* JackFactoryLinuxServer::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryLinuxServer::MakeServerNotifyChannel()
{
    return new JackSocketServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryLinuxServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryLinuxServer::MakeNotifyChannel()
{
    return new JackSocketNotifyChannel();
}

JackServerChannelInterface* JackFactoryLinuxServer::MakeServerChannel()
{
    return new JackSocketServerChannel();
}

JackSyncInterface* JackFactoryLinuxServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryLinuxServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackPosixThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#endif

#if defined(__APPLE__)

#if defined(MACH_RPC_MACH_SEMA) 
// Mach RPC + Mach Semaphore
JackSynchro* JackFactoryOSXServer::MakeSynchro()
{
    return new JackMachSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXServer::MakeServerNotifyChannel()
{
    return new JackMachServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryOSXServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryOSXServer::MakeNotifyChannel()
{
    return new JackMachNotifyChannel();
}

JackServerChannelInterface* JackFactoryOSXServer::MakeServerChannel()
{
    return new JackMachServerChannel();
}

JackSyncInterface* JackFactoryOSXServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_POSIX_SEMA) 
// Socket RPC + Posix Semaphore
JackSynchro* JackFactoryOSXServer::MakeSynchro()
{
    return new JackPosixSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXServer::MakeServerNotifyChannel()
{
    return new JackSocketServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryOSXServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryOSXServer::MakeNotifyChannel()
{
    return new JackSocketNotifyChannel();
}

JackServerChannelInterface* JackFactoryOSXServer::MakeServerChannel()
{
    return new JackSocketServerChannel();
}

JackSyncInterface* JackFactoryOSXServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_FIFO_SEMA) 
// Socket RPC + Fifo Semaphore
JackSynchro* JackFactoryOSXServer::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryOSXServer::MakeServerNotifyChannel()
{
    return new JackSocketServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryOSXServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryOSXServer::MakeNotifyChannel()
{
    return new JackSocketNotifyChannel();
}

JackServerChannelInterface* JackFactoryOSXServer::MakeServerChannel()
{
    return new JackSocketServerChannel();
}

JackSyncInterface* JackFactoryOSXServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(MACH_RPC_FIFO_SEMA) 
// Mach RPC + Fifo Semaphore
JackSynchro* JackFactoryOSXServer::MakeSynchro()
{
    return new JackFifo();
}

JackServerNotifyChannelInterface* JackFactoryOSXServer::MakeServerNotifyChannel()
{
    return new JackMachServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryOSXServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryOSXServer::MakeNotifyChannel()
{
    return new JackMachNotifyChannel();
}

JackServerChannelInterface* JackFactoryOSXServer::MakeServerChannel()
{
    return new JackMachServerChannel();
}

JackSyncInterface* JackFactoryOSXServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(MACH_RPC_POSIX_SEMA) 
// Mach RPC + Posix Semaphore
JackSynchro* JackFactoryOSXServer::MakeSynchro()
{
    return new JackPosixSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXServer::MakeServerNotifyChannel()
{
    return new JackMachServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryOSXServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryOSXServer::MakeNotifyChannel()
{
    return new JackMachNotifyChannel();
}

JackServerChannelInterface* JackFactoryOSXServer::MakeServerChannel()
{
    return new JackMachServerChannel();
}

JackSyncInterface* JackFactoryOSXServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#if defined(SOCKET_RPC_MACH_SEMA) 
// Socket RPC + Mac Semaphore
JackSynchro* JackFactoryOSXServer::MakeSynchro()
{
    return new JackMachSemaphore();
}

JackServerNotifyChannelInterface* JackFactoryOSXServer::MakeServerNotifyChannel()
{
    return new JackSocketServerNotifyChannel();
}

JackClientChannelInterface* JackFactoryOSXServer::MakeClientChannel()
{
    return NULL;
} // Not used

JackNotifyChannelInterface* JackFactoryOSXServer::MakeNotifyChannel()
{
    return new JackSocketNotifyChannel();
}

JackServerChannelInterface* JackFactoryOSXServer::MakeServerChannel()
{
    return new JackSocketServerChannel();
}

JackSyncInterface* JackFactoryOSXServer::MakeInterProcessSync()
{
    return new JackProcessSync();
}

JackThread* JackFactoryOSXServer::MakeThread(JackRunnableInterface* runnable)
{
    return new JackMachThread(runnable, PTHREAD_CANCEL_ASYNCHRONOUS);
}
#endif

#endif

} // end of namespace
