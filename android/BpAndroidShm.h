#ifndef BPANDROIDSHM
#define BPANDROIDSHM

#include <binder/Parcel.h>
#include "IAndroidShm.h"
#include <binder/IMemory.h>

namespace android {
	class BpAndroidShm: public BpInterface<IAndroidShm> {
		public:
			BpAndroidShm( const sp<IBinder> & impl);
			virtual ~BpAndroidShm();
			virtual sp<IMemoryHeap> getBuffer(int index);
			virtual int sendCommand(const char *command);
			virtual int allocShm(const int size); // if negative return value is error
			virtual int removeShm(const unsigned int index); // shared memory 제거 
			virtual int isAllocated(const unsigned int index); // allocated 여부 확인
			virtual int setRegistryIndex(const unsigned int index);
			virtual int getRegistryIndex();

			virtual sp<IMemoryHeap> InitSemaphore(const char* name);
	};
};

#endif
