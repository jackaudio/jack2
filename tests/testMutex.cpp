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
#include "JackMachSemaphore.h"
#include "JackMachThread.h"
#endif

#include "JackPosixThread.h"
#include "JackMutex.h"


using namespace Jack;

struct LockedObject : public JackLockAble {

	JackThread* fThread;
    int fCount;
 	
	LockedObject():fCount(0)
	{}
    
    virtual ~LockedObject()
	{
        fThread->Kill();
		delete fThread;
	}
    
    void LockedMethod1()
    {   
        JackLock lock(this);
        fCount++;
        //printf("LockedMethod1 self %x fCount %ld\n", pthread_self(), fCount);
    }
    
    void LockedMethod2()
    {   
        JackLock lock(this);
        fCount--;
        //printf("LockedMethod2 self %x fCount %ld\n", pthread_self(), fCount);
    }
    
    void LockedMethod3()
    {   
        JackLock lock(this);
        fCount--;
        //printf("LockedMethod3 self %x fCount %ld\n", pthread_self(), fCount);
    }

};

struct TestThread : public JackRunnableInterface {

	JackThread* fThread;
    LockedObject* fObject;
    int fNum;
 	
	TestThread(LockedObject* obj, int num)
	{
        fThread = new JackMachThread(this);
        fObject = obj;
        fNum = num;
	    fThread->StartSync();
 	}

	virtual ~TestThread()
	{
        fThread->Kill();
		delete fThread;
	}
     
    bool Execute()
    {
		//printf("TestThread Execute\n");
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

int main (int argc, char * const argv[])
{
    char c;
    
    LockedObject* obj = new LockedObject();
    TestThread* th1 = new TestThread(obj, 1);
    TestThread* th2 = new TestThread(obj,3);
    TestThread* th3 = new TestThread(obj, 2);
      
    while ((c = getchar()) != 'q')) {}
    
}
