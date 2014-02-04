#include <binder/Parcel.h>
#include <utils/Log.h>
#include "BpAndroidShm.h"

namespace android{

    int BpAndroidShm::sendCommand(const char*command) {
        Parcel data, reply;
        data.writeInterfaceToken(
                IAndroidShm::getInterfaceDescriptor());
        data.writeCString(command);
        status_t status = remote()->transact(HW_SENDCOMMAND, data, &reply);
        if(status != NO_ERROR) {
            ALOGE("print sendCommand error: %s", strerror(-status));
        } else {
            status= reply.readInt32();
        }
        return status;
    }

    sp<IMemoryHeap> BpAndroidShm::getBuffer(int index) {
        Parcel data, reply;
        sp<IMemoryHeap> memHeap = NULL;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        data.writeInt32(index);
        remote()->transact(HW_GETBUFFER, data, &reply);
        memHeap = interface_cast<IMemoryHeap> (reply.readStrongBinder());
        return memHeap;
    }

    BpAndroidShm::BpAndroidShm( const sp<IBinder>& impl)
        : BpInterface<IAndroidShm>(impl)
    {}

    BpAndroidShm::~BpAndroidShm()
    {}

    int BpAndroidShm::allocShm(const int size) { // if negative return value is error
        Parcel data, reply;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        data.writeInt32(size);
        status_t status = remote()->transact(HW_ALLOC_SHM, data, &reply);
        if(status != NO_ERROR) {
            ALOGE("print allocShm error: %s", strerror(-status));
        } else {
            status= reply.readInt32();
        }
        return status;
    }
    
    int BpAndroidShm::removeShm(const unsigned int index) { // shared memory 제거 
        Parcel data, reply;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        data.writeInt32(index);
        status_t status = remote()->transact(HW_REMOVE_SHM, data, &reply);
        if(status != NO_ERROR) {
            ALOGE("print removeShm error: %s", strerror(-status));
        } else {
            status= reply.readInt32();
        }
        return status;
    }

    int BpAndroidShm::isAllocated(const unsigned int index) { // allocated 여부 확인
        Parcel data, reply;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        data.writeInt32(index);
        status_t status = remote()->transact(HW_IS_ALLOCATED, data, &reply);
        if(status != NO_ERROR) {
            ALOGE("print isAllocated error: %s", strerror(-status));
        } else {
            status= reply.readInt32();
        }
        return status;
    }

    int BpAndroidShm::setRegistryIndex(const unsigned int index) {
        Parcel data, reply;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        data.writeInt32(index);
        status_t status = remote()->transact(HW_SET_REGISTRY_INDEX, data, &reply);
        if(status != NO_ERROR) {
            ALOGE("print setRegistryIndex error: %s", strerror(-status));
        } else {
            status= reply.readInt32();
        }
        return status;
    }

    int BpAndroidShm::getRegistryIndex() {
        Parcel data, reply;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        status_t status = remote()->transact(HW_GET_REGISTRY_INDEX, data, &reply);
        if(status != NO_ERROR) {
            ALOGE("print getRegistryIndex error: %s", strerror(-status));
        } else {
            status= reply.readInt32();
        }
        return status;
    }

    sp<IMemoryHeap> BpAndroidShm::InitSemaphore(const char* name) {
        Parcel data, reply;
        sp<IMemoryHeap> memHeap = NULL;
        data.writeInterfaceToken(IAndroidShm::getInterfaceDescriptor());
        data.writeCString(name);
        status_t status = remote()->transact(HW_INIT_SEMAPHORE, data, &reply);
        memHeap = interface_cast<IMemoryHeap> (reply.readStrongBinder());
        return memHeap;
    }
};
