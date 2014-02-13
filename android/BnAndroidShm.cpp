#include "BnAndroidShm.h"
#include <binder/Parcel.h>

namespace android {
    status_t BnAndroidShm::onTransact( uint32_t code,
            const Parcel &data,
            Parcel *reply,
            uint32_t flags)
    {
        switch(code) {
            case HW_SENDCOMMAND:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                const char *str;
                str = data.readCString();
                reply->writeInt32(sendCommand(str));
                return NO_ERROR;
            }break;

            case HW_GETBUFFER:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                int32_t index;
                data.readInt32(&index);
                sp<IMemoryHeap> Data = getBuffer(index);
                if(Data != NULL){
                    reply->writeStrongBinder(Data->asBinder());
                }
                return NO_ERROR;
            }break;

            case HW_ALLOC_SHM:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                int32_t size;
                data.readInt32(&size);
                reply->writeInt32(allocShm(size));
                return NO_ERROR;
            }break;

            case HW_REMOVE_SHM:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                int32_t index;
                data.readInt32(&index);
                reply->writeInt32(removeShm(index));
                return NO_ERROR;
            }break;
            
            case HW_IS_ALLOCATED:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                int32_t index;
                data.readInt32(&index);
                reply->writeInt32(isAllocated(index));
                return NO_ERROR;
            }break;

            case HW_SET_REGISTRY_INDEX:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                int32_t index;
                data.readInt32(&index);
                reply->writeInt32(setRegistryIndex(index));
                return NO_ERROR;
            }break;

            case HW_GET_REGISTRY_INDEX:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                reply->writeInt32(getRegistryIndex());
                return NO_ERROR;
            }break;

            case HW_INIT_SEMAPHORE:{
                CHECK_INTERFACE(IAndroidShm, data, reply);
                const char *name;
                name = data.readCString();
                sp<IMemoryHeap> Data = InitSemaphore(name);
                if(Data != NULL){
                    reply->writeStrongBinder(Data->asBinder());
                }
                return NO_ERROR;
            }break;
                            
            default:
                return BBinder::onTransact(code, data, reply, flags);
        }
    }
};
