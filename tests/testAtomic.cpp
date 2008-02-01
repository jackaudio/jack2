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

#include <iostream>
#include <unistd.h>

#include "JackAtomicState.h"
#include "JackPosixThread.h"

using namespace Jack;

#define SIZE 1024

struct TestState {

	long fTable[SIZE];
	long fReadCounter;
	long fWriteCounter;
	
	TestState()
	{
		for (int i = 0; i < SIZE; i++) {
			fTable[i] = 0;
		}
		fReadCounter = 0;
		fWriteCounter = 0;
	}
	virtual ~TestState()
	{}

	void Write() 
	{
        fWriteCounter++;
		for (int i = 0; i < SIZE; i++) {
			fTable[i] = fTable[i] + 10;
		}
	}
	
	bool Read() 
	{
		int val = fTable[0];
		fReadCounter++;
		for (int i = 0; i < SIZE; i++) {
			if (fTable[i] != val) {
				printf("Read error fReadCounter %ld, i %ld, curVal %ld, oldVal %ld\n", fReadCounter, i, fTable[i], val);
				return false;
			}
		}
		return true;
	}
	
	bool ReadCopy(long* result) 
	{
		int val = fTable[0];
		fReadCounter++;
		for (int i = 0; i < SIZE; i++) {
			result[i] = fTable[i];
			if (fTable[i] != val) {
				//printf("ReadCopy error fReadCounter %ld, i %ld, curVal %ld, oldVal %ld\n", fReadCounter, i, fTable[i], val);
				return false;
			}
		}
		return true;
	}
	
	bool Check(long* result) 
	{
		int val = result[0];
		for (int i = 0; i < SIZE; i++) {
			if (result[i] != val) {
				printf("Check error fReadCounter %ld, i %ld, curVal %ld, oldVal %ld\n", fReadCounter, i, fTable[i], val);
				return false;
			}
		}
		return true;
	}
	
	int GetVal() 
	{
		return fTable[10];
	}
	
}; 

/*!
\brief The state wrapped with the 2 state atomic class.
*/

class TestStateUser : public JackAtomicState<TestState> {

	public:
	
		TestStateUser(){}
		virtual ~TestStateUser(){}
		
		void TestWriteMethod() 
		{
			TestState* state = WriteNextStateStart();
			state->Write();
			state->Write();
			state->Write();
			WriteNextStateStop(); 
		}
			
		void TestReadMethod() 
		{
			TestState* state;
			int fCount = 0;
			long result[SIZE];
            UInt16 cur_index;
            UInt16 next_index;
			do {
                cur_index = GetCurrentIndex();
                fCount++;
				state = ReadCurrentState();
				bool res = state->ReadCopy(result);
			    next_index = GetCurrentIndex();
                if (!res) 
                    printf("TestReadMethod fCount %ld cur %ld next %ld\n", fCount, cur_index, next_index);
        	}while (cur_index != next_index);
			state->Check(result);
		}
		
		void TestReadRTMethod1() 
		{
			TestState* state = TrySwitchState();
			bool res = state->Read();
		}
		
		void TestReadRTMethod2() 
		{
			TestState* state = ReadCurrentState();
			state->Read();
		}
};

/*!
\brief Base class for reader/writer threads.
*/

struct TestThread : public JackRunnableInterface {

	JackPosixThread* fThread;
	TestStateUser* fObject;
	
	TestThread(TestStateUser* state):fObject(state)
	{
		fThread = new JackPosixThread(this);
		int res = fThread->Start();
	}

	virtual ~TestThread()
	{
		fThread->Kill();
		delete fThread;
	}
	
};

/*!
\brief "Real-time" reader thread.
*/

struct RTReaderThread : public TestThread {
	
	RTReaderThread(TestStateUser* state):TestThread(state) 
	{}
	
	bool Execute()
	{
		fObject->TestReadRTMethod1();
		
		for (int i = 0; i < 5; i++) {
			fObject->TestReadRTMethod2();
		}
	
		usleep(50);
		return true;
	}
};

/*!
\brief Non "Real-time" reader thread.
*/

struct ReaderThread : public TestThread {

	ReaderThread(TestStateUser* state):TestThread(state)
	{}
	
	bool Execute()
	{
		 fObject->TestReadMethod();
         usleep(56);
		 return true;
	}
};

/*!
\brief Writer thread.
*/

struct WriterThread : public TestThread {

	WriterThread(TestStateUser* state):TestThread(state)
	{}
	
	bool Execute()
	{
		fObject->TestWriteMethod();
		usleep(75);
		return true;
	}
};


int main(int argc, char * const argv[])
{
	char c;
	
	printf("Test concurrent access to a TestState data structure protected with the 2 state JackAtomicState class\n"); 
	
	TestStateUser fObject;
	WriterThread writer(&fObject);
	RTReaderThread readerRT1(&fObject);
  	ReaderThread reader1(&fObject);
	
	/*
	ReaderThread reader2(&fObject);
	ReaderThread reader3(&fObject);
	ReaderThread reader4(&fObject);
	ReaderThread reader5(&fObject);
	ReaderThread reader6(&fObject);
	*/

	while ((c = getchar()) != 'q') {}
	return 1;
}


