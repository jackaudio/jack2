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

#ifdef WIN32

#else

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>

#endif

#ifdef __APPLE__
	#include "JackMachSemaphore.h"
#endif

#ifdef WIN32
	#include "JackWinEvent.h"
#endif

#ifdef linux
	#include "JackPosixSemaphore.h"
	#include "JackFifo.h"
#endif

#include "JackPlatformPlug.h"

#define ITER 1000

#define SERVER "serveur3"
#define CLIENT "client3"

using namespace Jack;

class Test2 : public JackRunnableInterface
{

    private:

        detail::JackSynchro* fSynchro1;
        detail::JackSynchro* fSynchro2;

    public:

        Test2(detail::JackSynchro* synchro1, detail::JackSynchro* synchro2)
                : fSynchro1(synchro1), fSynchro2(synchro2)
        {}

        bool Execute()
        {
			int a = 1;
            for (int i = 0; i < ITER; i++) {
                fSynchro2->Wait();
				for (int j = 0; j < 2000000; j++) {
					 a += j;
				}
                fSynchro1->Signal();
            }
            return true;
        }

};

int main(int ac, char *av [])
{
    Test2* obj;
    detail::JackSynchro* sem1 = NULL;
    detail::JackSynchro* sem2 = NULL;
    JackThread* thread;

    printf("Test of synchronization primitives : client side\n");
    printf("type -s to test Posix semaphore\n");
    printf("type -f to test Fifo\n");
    printf("type -m to test Mach semaphore\n");
    printf("type -e to test Windows event\n");

#ifdef __APPLE__
    if (strcmp(av[1], "-m") == 0) {
        printf("Mach semaphore\n");
        sem1 = new JackMachSemaphore();
        sem2 = new JackMachSemaphore();
    }
#endif

#ifdef WIN32
    if (strcmp(av[1], "-e") == 0) {
        printf("Win event\n");
        sem1 = new JackWinEvent();
        sem2 = new JackWinEvent();
    }
#endif

#ifdef linux
    if (strcmp(av[1], "-s") == 0) {
        printf("Posix semaphore\n");
        sem1 = new JackPosixSemaphore();
        sem2 = new JackPosixSemaphore();
    }

    if (strcmp(av[1], "-f") == 0) {
        printf("Fifo\n");
        sem1 = new JackFifo();
        sem2 = new JackFifo();
    }
#endif

    if (!sem1->ConnectOutput(SERVER, "default"))
        return -1;
    if (!sem2->ConnectInput(CLIENT, "default"))
        return -1;

    obj = new Test2(sem1, sem2);

#ifdef __APPLE__
    thread = new JackMachThread(obj, 10000 * 1000, 500 * 1000, 10000 * 1000);
#endif

#ifdef WIN32
    thread = new JackWinThread(obj);
#endif

#ifdef linux
    thread = new JackPosixThread(obj, false, 50, PTHREAD_CANCEL_DEFERRED);
#endif

    thread->Start();
    thread->AcquireRealTime();
#ifdef WIN32
    Sleep(30 * 1000);
#else
    sleep(30);
#endif
    //thread->Stop();
    thread->Kill();
    sem1->Disconnect();
    sem2->Disconnect();
    delete obj;
    delete thread;
    delete sem1;
    delete sem2;
    return 0;
}

