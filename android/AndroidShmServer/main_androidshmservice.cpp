#define LOG_TAG "main_androidshmservice"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

#include "../../common/shm.h"
#include "../Shm.h"  //android extension of shm.h

using namespace android;

int main(int argc, char *argv[]) {
	jack_instantiate();
	ProcessState::self()->startThreadPool();
	ALOGI("AndroidShmService is starting now");
	IPCThreadState::self()->joinThreadPool();
	return 0;
}
