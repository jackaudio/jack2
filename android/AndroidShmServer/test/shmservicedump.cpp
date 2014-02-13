#include "../../IAndroidShm.h" 
#include <binder/MemoryHeapBase.h>
#include <binder/IServiceManager.h>
#include "../../../common/shm.h"

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

unsigned int * getBufferMemPointer(int index)
{
  sp<IAndroidShm> shm = getAndroidShmService();
  if (shm == NULL) {
    printf("The EneaBufferServer is not published\n");
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

void showStatus() {
    sp<IAndroidShm> shm = getAndroidShmService();
    if(shm == NULL) {
        printf("shm service is not available\n");
        return;
    }
    
    printf("<<<<<<<<<<< dump memory allocation status >>>>>>>>>>\n");
    for(int i = 256; i >= 0; i--) {
        if(shm->isAllocated(i) == 1) {
            printf("Mem[%3d] == 0x%x\n", i, (unsigned int)getBufferMemPointer(i));
        } else {
            printf("Mem[%3d] == NULL\n", i);
        }
    }
}

void showRegistryIndex() {
    sp<IAndroidShm> shm = getAndroidShmService();

    if(shm == NULL) {
        printf("shm service is not available\n");
        return;
    }

    printf("<<<<<<<<<<< show registry index >>>>>>>>>>\n");

    printf("index [%3d]\n",shm->getRegistryIndex());
}

void showHeader() {
    sp<IAndroidShm> shm = getAndroidShmService();

    if(shm == NULL) {
        printf("shm service is not available\n");
        return;
    }

    if(shm->getRegistryIndex() > 256) {
        printf("don't have a registry header\n");
        return;
    }
    
    unsigned int* buffer = getBufferMemPointer(shm->getRegistryIndex());
    if(buffer) {
        jack_shm_header_t * header = (jack_shm_header_t*)buffer;
        printf("<<<<<<<<<<  register header value  >>>>>>>>>>\n");
        printf("memory address 0x%x 0x%x\n", (unsigned int)(header), (unsigned int)buffer);
        printf("magic = %d\n", header->magic);
        printf("protocol = %d\n", header->protocol);
        printf("type = %d\n", header->type);
        printf("size = %d\n", header->size);
        printf("hdr_len = %d\n", header->hdr_len);
        printf("entry_len = %d\n", header->entry_len);
        for(int j = 0; j < MAX_SERVERS; j++) {
            //char name[256];
            //memset(name, '\0', 256);
            //strncpy(name, header->server[j].name, 10);
            printf("server[%d] pid = %d, name = %s\n", j, header->server[j].pid, header->server[j].name);
        }
    }
}

void showBody() {
    sp<IAndroidShm> shm = getAndroidShmService();

    if(shm == NULL) {
        printf("shm service is not available\n");
        return;
    }
    
    if(shm->getRegistryIndex() > 256) {
        printf("don't have a registry body\n");
        return;
    }
    unsigned int* buffer = getBufferMemPointer(shm->getRegistryIndex());
    if(buffer) {        
        jack_shm_header_t * header = (jack_shm_header_t*)buffer;
        printf("<<<<<<<<<< registry body value >>>>>>>>>>\n");
        jack_shm_registry_t * registry = (jack_shm_registry_t *) (header + 1);
        for(int k = 255; k >= 0; k--) {
            printf("registry[%3d] index[%3d],allocator[%3d],size[%6d],id[%10s],fd[%3d]\n", k, 
                registry[k].index, registry[k].allocator, registry[k].size, 
                registry[k].id, 
                registry[k].fd);
        }
    }
}

void showSemaphore() {
    sp<IAndroidShm> shm = getAndroidShmService();

    if(shm == NULL) {
        printf("shm service is not available\n");
        return;
    }

    shm->sendCommand("semaphore");
    printf("log will be shown in the logcat log\n");
}

int main(int argc, char** argv) {
    // base could be on same address as Servers base but this
    // is purely by luck do NEVER rely on this. Linux memory
    // management may put it wherever it likes.

    if(argc < 2) {
        printf("usage\n shmservicedump [status|header|body|index|semaphore]\n");
        printf(" status: show the shared memory allocation status\n");
        printf(" header: show the registry header infomations if the registry exist\n");
        printf(" body: show the registry body infomations if the registry exist\n");
        printf(" index: show the index of array that is allocated registry shared memory\n");
        printf(" semaphore: show the memory array about semaphore simulation\n");
        return 0;
    }

    if(strcmp(argv[1], "semaphore") == 0) {
        showSemaphore();
    } else if(strcmp(argv[1], "index") == 0) {
        showRegistryIndex();
    } else if(strcmp(argv[1], "status") == 0) {
        showStatus();
    } else if(strcmp(argv[1], "header") == 0) {
        showHeader();
    } else if(strcmp(argv[1], "body") == 0) {
        showBody();
    } else {
        printf("%s is invalid parameter\n", argv[1]);
    }
    
    return 0;
}
