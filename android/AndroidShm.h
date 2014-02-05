#ifndef ANDROIDSHM
#define ANDROIDSHM

#include <binder/Parcel.h>
#include "BnAndroidShm.h"
#include <utils/Log.h>
#include <binder/MemoryHeapBase.h>
#include "shm.h"
#include "android/Shm.h"  //android extension of shm.h

namespace android {

    class AndroidShm : public BnAndroidShm
    {
#define MAX_SHARED_MEMORY_COUNT 257
        private:
            int MemAlloc(unsigned int size);
        
        public:
            virtual ~AndroidShm();
            static int instantiate();
            virtual int sendCommand(const char* command);
            virtual int allocShm(const int size); // if negative return value is error
            virtual int removeShm(const unsigned int index); // shared memory 제거 
            virtual int isAllocated(const unsigned int index); // allocated 여부 확인
            virtual int setRegistryIndex(const unsigned int index);
            virtual int getRegistryIndex();
            virtual sp<IMemoryHeap> InitSemaphore(const char* name);
            virtual sp<IMemoryHeap> getBuffer(int index);
            //virtual status_t onTransact(
            //        uint32_t code,
            //        const Parcel& data,
            //        Parcel* reply,
            //        uint32_t flags);
        private:
            int testGetBuffer();
            int testGetBufferByNewProcess();
            AndroidShm();

            sp<MemoryHeapBase> mMemHeap[MAX_SHARED_MEMORY_COUNT];
            unsigned int mRegistryIndex;

            // for named semaphore simulation
            #define MAX_SEMAPHORE_MEMORY_COUNT 300
            #define MAX_SEMAPHORE_NAME_LENGTH 300
            sp<MemoryHeapBase> mSemaphore[MAX_SEMAPHORE_MEMORY_COUNT];
            char mSemaphoreName[MAX_SEMAPHORE_MEMORY_COUNT][MAX_SEMAPHORE_NAME_LENGTH];
    };
};

#endif
