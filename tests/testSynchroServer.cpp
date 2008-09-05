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

#ifdef WIN32
LARGE_INTEGER gFreq;
long gQueryOverhead;

static long elapsed (LARGE_INTEGER* t1, LARGE_INTEGER* t2)
{
    long high = t1->HighPart - t2->HighPart;
    double low = t1->LowPart - t2->LowPart;
    // ignore values when high part changes
    return high ? 0 : (long)((low * 1000000) / gFreq.LowPart);
}

static BOOL overhead (long * overhead)
{
    LARGE_INTEGER t1, t2;
    BOOL r1, r2;
    int i = 50;
    r1 = QueryPerformanceCounter (&t1);
    while (i--)
        QueryPerformanceCounter (&t2);
    r2 = QueryPerformanceCounter (&t2);
    if (!r1 || !r2)
        return FALSE;
    *overhead = elapsed(&t2, &t1) / 50;
    return TRUE;
}

#endif

class Test1 : public JackRunnableInterface
{

    private:

        detail::JackSynchro* fSynchro1;
        detail::JackSynchro* fSynchro2;

    public:

        Test1(detail::JackSynchro* synchro1, detail::JackSynchro* synchro2)
                : fSynchro1(synchro1), fSynchro2(synchro2)
        {}

        bool Execute()
        {

	#ifdef WIN32
            LARGE_INTEGER t1, t2;
            BOOL r1, r2;
            r1 = QueryPerformanceCounter(&t1);
	#else
           struct timeval T0, T1;
            clock_t time1, time2;
            // Get total time for 2 * ITER process swaps
            time1 = clock();
            gettimeofday(&T0, 0);
	#endif
            printf("Execute loop\n");
            for (int i = 0; i < ITER; i++) {
                fSynchro2->Signal();
                fSynchro1->Wait();
            }

	#ifdef WIN32
            r2 = QueryPerformanceCounter (&t2);
            elapsed(&t2, &t1);
            printf ("%5.1lf usec for inter process swap\n", elapsed(&t2, &t1) / (2.0 * ITER));
	#else
            time2 = clock();
            gettimeofday(&T1, 0);
            printf ("%5.1lf usec for inter process swap\n", (1e6 * T1.tv_sec - 1e6 * T0.tv_sec + T1.tv_usec - T0.tv_usec) / (2.0 * ITER));
            printf ("%f usec for inter process swap \n", (1e6 * ((time2 - time1) / (double(CLOCKS_PER_SEC)))) / (2.0 * ITER));
	#endif
           return true;
        }

};

int main(int ac, char *av [])
{
    Test1* obj;
    detail::JackSynchro* sem1 = NULL;
    detail::JackSynchro* sem2 = NULL;
    JackThread* thread;

#ifdef WIN32
    if (!QueryPerformanceFrequency (&gFreq) ||
            !overhead (&gQueryOverhead)) {
        printf ("cannot query performance counter\n");
    }
#endif

    printf("Test of synchronization primitives : server side\n");
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

    if (!sem1->Allocate(SERVER, "default", 0))
        return -1;
    if (!sem2->Allocate(CLIENT, "default", 0))
        return -1;

    // run test in RT thread
    obj = new Test1(sem1, sem2);

#ifdef __APPLE__
    thread = new JackMachThread(obj, 10000 * 1000, 500 * 1000, 10000 * 1000);
#endif

#ifdef  WIN32
    thread = new JackWinThread(obj);
#endif

#ifdef linux
    thread = new JackPosixThread(obj, false, 50, PTHREAD_CANCEL_DEFERRED);
#endif

    thread->Start();
    thread->AcquireRealTime();
#ifdef  WIN32
	Sleep(90 * 1000);
#else
    sleep(30);
#endif

    thread->Stop();
    sem1->Destroy();
    sem2->Destroy();
    delete obj;
    delete thread;
    delete sem1;
    delete sem2;
    return 0;
}
