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
#include <unistd.h>

#ifdef __APPLE__
#include "JackMachThread.h"
#endif

#include "JackPosixThread.h"
#include "JackMutex.h"
#include "thread.h"

using namespace Jack;

static void CleanupHandler(void * arg)
{
    JackLockAble* locked = (JackLockAble*)arg;
    printf("CleanupHandler locked %px \n", locked);
    locked->Unlock();
}

struct LockedObject : public JackLockAble {

    int fCount;
 	
	LockedObject():fCount(0)
	{}
    
    virtual ~LockedObject()
	{}
    
    /*
    void LockedMethod1()
    {   
        JackLock lock(this);
        fCount++;
        printf("LockedMethod1 self %x fCount %d\n", pthread_self(), fCount);
        if (fCount >= 1000) {
            printf("Terminate self %x\n", pthread_self());
            pthread_exit(NULL);
        }
   }
    
    void LockedMethod2()
    {   
        JackLock lock(this);
        fCount++;
        printf("LockedMethod2 self %x fCount %d\n", pthread_self(), fCount);
        if (fCount >= 1500) {
            printf("Terminate self %x\n", pthread_self());
            pthread_exit(NULL);
        }
    }
    
    void LockedMethod3()
    {   
        JackLock lock(this);
        fCount++;
        printf("LockedMethod3 self %x fCount %d\n", pthread_self(), fCount);
        if (fCount >= 3000) {
            printf("Terminate self %x\n", pthread_self());
            pthread_exit(NULL);
        }
    }
    */
    
    void LockedMethod1()
    {   
        pthread_cleanup_push(CleanupHandler, this);
        Lock();
        fCount++;
        //printf("LockedMethod1 self %x fCount %d\n", pthread_self(), fCount);
        if (fCount >= 1000) {
            printf("Terminate self = %px  count = %d\n", pthread_self(), fCount);
            pthread_exit(NULL);
        }
        Unlock();
        pthread_cleanup_pop(0);
    }
    
    void LockedMethod2()
    {   
        pthread_cleanup_push(CleanupHandler, this);
        Lock();

        fCount++;
        //printf("LockedMethod2 self %x fCount %d\n", pthread_self(), fCount);
        if (fCount >= 1500) {
            printf("Terminate self = %px  count = %d\n", pthread_self(), fCount);
            pthread_exit(NULL);
        }
        Unlock();
        pthread_cleanup_pop(0);
    }
    
    void LockedMethod3()
    {   
        pthread_cleanup_push(CleanupHandler, this);
        Lock();

        fCount++;
        //printf("LockedMethod3 self %x fCount %d\n", pthread_self(), fCount);
        if (fCount >= 3000) {
            printf("Terminate self = %px  count = %d\n", pthread_self(), fCount);
            pthread_exit(NULL);
        }
        Unlock();
        pthread_cleanup_pop(0);
    }


};

struct TestThread : public JackRunnableInterface {

	JackMachThread* fThread;
    LockedObject* fObject;
    int fNum;
 	
	TestThread(LockedObject* obj, int num)
	{
        printf("TestThread\n");
        fThread = new JackMachThread(this);
        fObject = obj;
        fNum = num;
	    fThread->StartSync();
   }

	virtual ~TestThread()
	{
        printf("DELETE %px\n", fThread);
        fThread->Kill();
		delete fThread;
	}
     
    bool Execute()
    {
		//printf("TestThread Execute\n");
        switch (fNum) {
        
            case 1:
                fObject->LockedMethod1();
                /*
                if (fObject->fCount >= 500) {
                    printf("Terminate self %x\n", pthread_self());
                    fThread->Terminate();
                }
                */
                break;
                
            case 2:
                fObject->LockedMethod2();
                /*
                if (fObject->fCount >= 1500) {
                    printf("Terminate self %x\n", pthread_self());
                    fThread->Terminate();
                }
                */
                break;
        
            case 3:
                fObject->LockedMethod3();
                /*
                if (fObject->fCount >= 2000) {
                    printf("Terminate self %x\n", pthread_self());
                    fThread->Terminate();
                }
                */
                break;
        };
   		
        //usleep(fNum * 1000);
		return true;
    }
	
};

static void* TestThread1_Execute(void* arg);

struct TestThread1 : public JackRunnableInterface {

	pthread_t fThread;
    LockedObject* fObject;
    int fNum;
 	
	TestThread1(LockedObject* obj, int num)
	{
        if (jack_client_create_thread(NULL, &fThread, 0, 0, TestThread1_Execute, this))
			jack_error( "Can't create the network manager control thread." );
        fObject = obj;
        fNum = num;
 	}

	virtual ~TestThread1()
	{}
     
    bool Execute()
    {
		printf("TestThread Execute\n");
        switch (fNum) {
        
            case 1:
                fObject->LockedMethod1();
                break;
                
            case 2:
                fObject->LockedMethod2();
                break;
        
            case 3:
                fObject->LockedMethod3();
                break;
        };
   		
        //usleep(fNum * 1000);
		return true;
    }
	
};

static void* TestThread1_Execute(void* arg)
{
    TestThread1* obj = (TestThread1*)arg;
    
    while (true) {
        //printf("TestThread Execute\n");
        switch (obj->fNum) {
            
                case 1:
                    obj->fObject->LockedMethod1();
                    break;
                    
                case 2:
                    obj->fObject->LockedMethod2();
                    break;
            
                case 3:
                    obj->fObject->LockedMethod3();
                    break;
        };
            
        //usleep(obj->fNum * 1000);
    }
    
    return 0;
}

int main (int argc, char * const argv[])
{
    char c;
      
    LockedObject obj;
   
    TestThread th1(&obj, 1);
    TestThread th2(&obj, 2);
    TestThread th3(&obj, 3);
     
    /*
    LockedObject obj;
    TestThread1 th1(&obj, 1);
    TestThread th2(&obj, 2);
    TestThread th3(&obj, 3);
    */
      
    /*  
    while ((c = getchar()) != 'q') {
    
    }
    */
    
    while (true) {
        usleep(1000);
        th1.fThread->Kill();
    }
   
    /*
    th1.fThread->Kill();
    th2.fThread->Kill();
    th3.fThread->Kill();
    
     while (true) {
        //usleep(100000);
        th1.fThread->Kill();
    }
    */
    
}
