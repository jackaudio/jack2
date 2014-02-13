#ifndef IANDROIDSHM
#define IANDROIDSHM

#include <binder/IInterface.h>
#include <binder/IMemory.h>

 namespace android {

	enum {
		HW_GETBUFFER = IBinder::FIRST_CALL_TRANSACTION, 
		HW_MULTIPLY,
		HW_STARTSERVER,
		HW_MAKECLIENT,
		HW_SENDCOMMAND,
		HW_LOADSO,
		HW_ALLOC_SHM,
		HW_REMOVE_SHM,
		HW_IS_ALLOCATED,
		HW_SET_REGISTRY_INDEX,
		HW_GET_REGISTRY_INDEX,
		HW_INIT_SEMAPHORE
	};
	
	class IAndroidShm: public IInterface {
		public:
			DECLARE_META_INTERFACE(AndroidShm);

			virtual sp<IMemoryHeap> getBuffer(int index) = 0;
			virtual int sendCommand(const char *command) = 0;
			virtual int allocShm(const int size) = 0; // if negative return value is error
			virtual int removeShm(const unsigned int index) = 0; // shared memory 제거 
			virtual int isAllocated(const unsigned int index) = 0; // allocated 여부 확인
			virtual int setRegistryIndex(const unsigned int index) = 0;
			virtual int getRegistryIndex() = 0;

			// for named semaphore simulation
			virtual sp<IMemoryHeap> InitSemaphore(const char* name) = 0;
	};
};

#endif
