#ifndef BNANDROIDSHM
#define BNANDROIDSHM

#include <binder/Parcel.h>
#include "IAndroidShm.h"

namespace android {
    class BnAndroidShm : public BnInterface<IAndroidShm> {
        public:
        virtual status_t onTransact( uint32_t code,
                const Parcel& data,
                Parcel* reply,
                uint32_t flags = 0);
    };
};
#endif
