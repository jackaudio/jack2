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

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include "JackMachSemaphore.h"
#endif

#include "JackPosixSemaphore.h"
#include "JackFifo.h"

#include "JackPlatformPlug.h"

#define ITER 500000

using namespace Jack;

struct ServerThread : public JackRunnableInterface {

	JackThread* fThread;
    detail::JackSynchro* fServerSem;
    detail::JackSynchro* fClientSem;
	
	ServerThread()
	{
        fServerSem->Allocate("JackSemServer", "default", 0);
        fClientSem->Allocate("JackSemClient", "default", 0);
		//fThread = new JackMachThread(this);
		fThread->SetParams(0, 500*1000, 500*1000);
        fThread->Start();
		//fThread->AcquireRealTime();
	}

	virtual ~ServerThread()
	{
        fThread->Kill();
		delete fThread;
	}
     
    bool Execute()
    {
		printf("Execute Server\n");
   		for (int i = 0; i < ITER; i++) {
			fClientSem->Signal();
            fServerSem->Wait();
		}
		return true;
    }
	
};

struct ClientThread : public JackRunnableInterface {

	JackThread* fThread;
    detail::JackSynchro* fServerSem;
    detail::JackSynchro* fClientSem;
	
	ClientThread()
	{
        fServerSem->Connect("JackSemServer", "default");
        fClientSem->Connect("JackSemClient", "default");
		//fThread = new JackMachThread(this);
		fThread->SetParams(0, 500*1000, 500*1000);
		fThread->Start();
		//fThread->AcquireRealTime();
	}

	virtual ~ClientThread()
	{
		fThread->Kill();
		delete fThread;
	}
    
    bool Execute()
    {
		struct timeval T0, T1;
        printf("Execute Client\n");
        fClientSem->Wait();
    	gettimeofday(&T0, 0); 
		 
		for (int i = 0; i < ITER; i++) {
			fServerSem->Signal();
            fClientSem->Wait();
		}
		
		gettimeofday(&T1, 0); 
		printf("%5.1lf usec\n", (1e6 * T1.tv_sec - 1e6 * T0.tv_sec + T1.tv_usec - T0.tv_usec) / (2.0 * ITER));
		return true;
    }
	
};

void server(detail::JackSynchro* sem)
{
	char c;
	printf("server\n");
	
	sem->Allocate("JackSem", "default", 0);
	
	while (((c = getchar()) != 'q')) {
	
		switch(c) {
		
			case 's':
				printf("SynchroSignal....\n");
				//sem->Signal();
				sem->SignalAll();
				printf("SynchroSignal OK\n");
				break;
				
			case 'w':
				printf("SemaphoreWait....\n");
				sem->Wait();
				printf("SemaphoreWait OK\n");
				break;
		}
	}
}

void client(detail::JackSynchro* sem)
{
	char c;
	printf("client\n");
	
	sem->Connect("JackSem", "default");
	
	while (((c = getchar()) != 'q')) {
	
		switch(c) {
		
			case 's':
				printf("SemaphoreSignal....\n");
				sem->Signal();
				printf("SemaphoreSignal OK\n");
				break;
				
			case 'w':
				printf("SemaphoreWait....\n");
				sem->Wait();
				printf("SemaphoreWait OK\n");
				break;
		}
	}
}

int main (int argc, char * const argv[])
{
    char c;
    ServerThread* serverthread = NULL;
    ClientThread* clientthread = NULL;
    detail::JackSynchro* sem1 = NULL;
	
	if (strcmp(argv[1],"-s") == 0) {
		printf("Posix semaphore\n");
		sem1 = new JackPosixSemaphore();
	}
			
	if (strcmp(argv[1],"-f") == 0) {
		printf("Fifo\n");
		sem1 = new JackFifo();
	}

#ifdef __APPLE__			
	if (strcmp(argv[1],"-m") == 0) {
		printf("Mach semaphore\n");
		sem1 = new JackMachSemaphore();
	}
#endif 
	
	/*  
    if (strcmp(argv[2], "server") == 0) {
		serverthread = new ServerThread();
    } else {
		clientthread = new ClientThread();
    }
	*/
	
	if (strcmp(argv[2], "server") == 0) {
		server(sem1);
    } else {
		client(sem1);
    }
    
    while (((c = getchar()) != 'q')) {}
    
    delete serverthread;
    delete clientthread;
}
