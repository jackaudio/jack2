#include "../../IAndroidShm.h" 
#include <binder/MemoryHeapBase.h>
#include <binder/IServiceManager.h>

namespace android {

static sp<IMemoryHeap> receiverMemBase;
#define MAX_SHARED_MEMORY_COUNT 257

sp<IAndroidShm> getAndroidShmService() {
    sp<IAndroidShm> shm = 0;

    /* Get the buffer service */
    if (shm == NULL) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        binder = sm->getService(String16("com.samsung.android.jam.IAndroidShm"));
        if (binder != 0) {
            shm = IAndroidShm::asInterface(binder);
            //shm = interface_cast<IAndroidShm>(binder);
        }
    }
    return shm;
}

unsigned int * getBufferMemPointer(int index) {
    sp<IAndroidShm> shm = getAndroidShmService();
    if (shm == NULL) {
        ALOGE("The EneaBufferServer is not published");
        return (unsigned int *)-1; /* return an errorcode... */
    } else {
        receiverMemBase = shm->getBuffer(index);
        if(receiverMemBase != NULL)
            return (unsigned int *) receiverMemBase->getBase();
        else
            return (unsigned int*)-1;
    }
}

}

using namespace android;

void setup_test() {
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL) return;
    for(int i = 0; i < MAX_SHARED_MEMORY_COUNT; i++) {
        shm->removeShm(i);
    }
}

void teardown_test() {
}

void increase_value_once() {
    ALOGD("*****test: increase_value_once*****\n");
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL) return;

    int slot = shm->allocShm(10000);
    unsigned int *base = getBufferMemPointer(slot);
    if(base != (unsigned int *)-1) {
        ALOGD("ShmServiceTest base=%p Data=0x%x\n",base, *base);
        *base = (*base)+1;
        ALOGD("ShmServiceTest base=%p Data=0x%x CHANGED\n",base, *base);
        //receiverMemBase = 0;
    } else {
        ALOGE("Error shared memory not available\n");
    }
}

void increase_value_10times() {
    ALOGD("*****test: increase_value_10times*****\n");
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL) return;

    int slot = shm->allocShm(10000);

    for(int i = 0; i < 10; i++) {
        unsigned int *base = getBufferMemPointer(slot);
        if(base != (unsigned int *)-1) {
            ALOGD("ShmServiceTest base=%p Data=0x%x\n",base, *base);
            *base = (*base)+1;
            ALOGD("ShmServiceTest base=%p Data=0x%x CHANGED\n",base, *base);
            //receiverMemBase = 0;
        } else {
            ALOGE("Error shared memory not available\n");
        }
    }
}

void check_allocated() {
    ALOGD("*****test: check_allocated*****\n");
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL) return;

    int slot = shm->allocShm(10000);
    int i = 0;
    for(; i < MAX_SHARED_MEMORY_COUNT; i++) {
        if(slot == i) {
            if(shm->isAllocated(i) == 1) {
            //ALOGD("pass\n");
            } else {
            ALOGD("failed\n");
            }
        } else {
            if(shm->isAllocated(i) == 0) {
                //ALOGD("pass\n");
            } else {
                ALOGD("failed\n");
            }
        }
    }

    if(i == MAX_SHARED_MEMORY_COUNT) {
        ALOGD("pass\n");
    }
}

void test_set_get_registry_index() {
    ALOGD("*****test: test_set_get_registry_index*****\n");
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL) return;

    int registry = 1;
    shm->setRegistryIndex(registry);
    if(registry == shm->getRegistryIndex()) {
        ALOGD("pass\n");
    } else {
        ALOGD("fail\n");
    }

    registry = 0;
    shm->setRegistryIndex(registry);
    if(registry == shm->getRegistryIndex()) {
        ALOGD("pass\n");
    } else {
        ALOGD("fail\n");
    }
}

void test_memset() {
    ALOGD("*****test: test_memset*****\n");
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL)  return;

    int slot = shm->allocShm(10000);

    unsigned int * pnt = getBufferMemPointer(slot);

    memset (pnt, 0, 10000);

    ALOGD("result : 0 0 0 0\n");
    ALOGD("memory dump : %d %d %d %d\n", pnt[0], pnt[1], pnt[2], pnt[3]);
    
    memset (pnt, 0xffffffff, 10000);

    ALOGD("result : -1 -1 -1 -1\n");
    ALOGD("memory dump : %d %d %d %d", pnt[0], pnt[1], pnt[2], pnt[3]);
}

int main(int argc, char** argv) {
    // base could be on same address as Servers base but this
    // is purely by luck do NEVER rely on this. Linux memory
    // management may put it wherever it likes.

    setup_test();
    increase_value_once();
    teardown_test();

    setup_test();
    increase_value_10times();
    teardown_test();

    setup_test();
    check_allocated();
    teardown_test();

    setup_test();
    test_set_get_registry_index();
    teardown_test();

    setup_test();
    test_memset();
    teardown_test();

    return 0;
}

