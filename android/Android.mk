#
# jack-1.9.10
#

LOCAL_PATH := $(call my-dir)
JACK_ROOT := $(call my-dir)/..
SUPPORT_ALSA_IN_JACK := true
SUPPORT_ANDROID_REALTIME_SCHED := false
ifeq ($(TARGET_BOARD_PLATFORM),mrvl)
ALSA_INCLUDES := vendor/marvell/external/alsa-lib/include
else
ALSA_INCLUDES := vendor/samsung/common/external/alsa-lib/include
endif
JACK_STL_LDFLAGS := -Lprebuilts/ndk/current/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_CPU_ABI) -lgnustl_static
JACK_STL_INCLUDES := $(JACK_ROOT)/android/cxx-stl/gnu-libstdc++/libs/$(TARGET_CPU_ABI)/include \
                     prebuilts/ndk/current/sources/cxx-stl/gnu-libstdc++/libs/$(TARGET_CPU_ABI)/include \
                     prebuilts/ndk/current/sources/cxx-stl/gnu-libstdc++/include

##########################################################
# common
##########################################################

common_cflags := -O0 -g -Wall -fexceptions -fvisibility=hidden -DHAVE_CONFIG_H
common_cflags += -Wno-unused -Wno-sign-compare -Wno-deprecated-declarations -Wno-cpp
common_cppflags := -frtti -Wno-sign-promo -fcheck-new
common_shm_cflags := -O0 -g -Wall -fexceptions -DHAVE_CONFIG_H -Wno-unused
ifeq ($(TARGET_BOARD_PLATFORM),clovertrail)
common_ldflags := -ldl
else
common_ldflags :=
endif
common_c_includes := \
    $(JACK_ROOT) \
    $(JACK_ROOT)/common \
    $(JACK_ROOT)/common/jack \
    $(JACK_ROOT)/android \
    $(JACK_ROOT)/linux \
    $(JACK_ROOT)/linux/alsa \
    $(JACK_ROOT)/posix \
    $(JACK_STL_INCLUDES)

# copy common source file
common_libsource_server_dir = .server
common_libsource_client_dir = .client

$(shell rm -rf $(LOCAL_PATH)/$(common_libsource_server_dir))
$(shell rm -rf $(LOCAL_PATH)/$(common_libsource_client_dir))
$(shell mkdir $(LOCAL_PATH)/$(common_libsource_server_dir))
$(shell mkdir $(LOCAL_PATH)/$(common_libsource_client_dir))

$(shell cp -f $(LOCAL_PATH)/../common/JackActivationCount.cpp       $(LOCAL_PATH)/$(common_libsource_server_dir)/JackActivationCount.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackAPI.cpp                   $(LOCAL_PATH)/$(common_libsource_server_dir)/JackAPI.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackClient.cpp                $(LOCAL_PATH)/$(common_libsource_server_dir)/JackClient.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackConnectionManager.cpp     $(LOCAL_PATH)/$(common_libsource_server_dir)/JackConnectionManager.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/ringbuffer.c                  $(LOCAL_PATH)/$(common_libsource_server_dir)/ringbuffer.c)
$(shell cp -f $(LOCAL_PATH)/JackError.cpp                           $(LOCAL_PATH)/$(common_libsource_server_dir)/JackError.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackException.cpp             $(LOCAL_PATH)/$(common_libsource_server_dir)/JackException.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackFrameTimer.cpp            $(LOCAL_PATH)/$(common_libsource_server_dir)/JackFrameTimer.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackGraphManager.cpp          $(LOCAL_PATH)/$(common_libsource_server_dir)/JackGraphManager.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackPort.cpp                  $(LOCAL_PATH)/$(common_libsource_server_dir)/JackPort.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackPortType.cpp              $(LOCAL_PATH)/$(common_libsource_server_dir)/JackPortType.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackAudioPort.cpp             $(LOCAL_PATH)/$(common_libsource_server_dir)/JackAudioPort.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackMidiPort.cpp              $(LOCAL_PATH)/$(common_libsource_server_dir)/JackMidiPort.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackMidiAPI.cpp               $(LOCAL_PATH)/$(common_libsource_server_dir)/JackMidiAPI.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackEngineControl.cpp         $(LOCAL_PATH)/$(common_libsource_server_dir)/JackEngineControl.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackShmMem.cpp                $(LOCAL_PATH)/$(common_libsource_server_dir)/JackShmMem.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackGenericClientChannel.cpp  $(LOCAL_PATH)/$(common_libsource_server_dir)/JackGenericClientChannel.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackGlobals.cpp               $(LOCAL_PATH)/$(common_libsource_server_dir)/JackGlobals.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackDebugClient.cpp           $(LOCAL_PATH)/$(common_libsource_server_dir)/JackDebugClient.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackTransportEngine.cpp       $(LOCAL_PATH)/$(common_libsource_server_dir)/JackTransportEngine.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/timestamps.c                  $(LOCAL_PATH)/$(common_libsource_server_dir)/timestamps.c)
$(shell cp -f $(LOCAL_PATH)/../common/JackTools.cpp                 $(LOCAL_PATH)/$(common_libsource_server_dir)/JackTools.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackMessageBuffer.cpp         $(LOCAL_PATH)/$(common_libsource_server_dir)/JackMessageBuffer.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackEngineProfiling.cpp       $(LOCAL_PATH)/$(common_libsource_server_dir)/JackEngineProfiling.cpp)
$(shell cp -f $(LOCAL_PATH)/JackAndroidThread.cpp                   $(LOCAL_PATH)/$(common_libsource_server_dir)/JackAndroidThread.cpp)
$(shell cp -f $(LOCAL_PATH)/JackAndroidSemaphore.cpp                $(LOCAL_PATH)/$(common_libsource_server_dir)/JackAndroidSemaphore.cpp)
$(shell cp -f $(LOCAL_PATH)/../posix/JackPosixProcessSync.cpp       $(LOCAL_PATH)/$(common_libsource_server_dir)/JackPosixProcessSync.cpp)
$(shell cp -f $(LOCAL_PATH)/../posix/JackPosixMutex.cpp             $(LOCAL_PATH)/$(common_libsource_server_dir)/JackPosixMutex.cpp)
$(shell cp -f $(LOCAL_PATH)/../posix/JackSocket.cpp                 $(LOCAL_PATH)/$(common_libsource_server_dir)/JackSocket.cpp)
$(shell cp -f $(LOCAL_PATH)/../linux/JackLinuxTime.c                $(LOCAL_PATH)/$(common_libsource_server_dir)/JackLinuxTime.c)

$(shell cp -f $(LOCAL_PATH)/../common/JackActivationCount.cpp       $(LOCAL_PATH)/$(common_libsource_client_dir)/JackActivationCount.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackAPI.cpp                   $(LOCAL_PATH)/$(common_libsource_client_dir)/JackAPI.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackClient.cpp                $(LOCAL_PATH)/$(common_libsource_client_dir)/JackClient.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackConnectionManager.cpp     $(LOCAL_PATH)/$(common_libsource_client_dir)/JackConnectionManager.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/ringbuffer.c                  $(LOCAL_PATH)/$(common_libsource_client_dir)/ringbuffer.c)
$(shell cp -f $(LOCAL_PATH)/JackError.cpp                           $(LOCAL_PATH)/$(common_libsource_client_dir)/JackError.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackException.cpp             $(LOCAL_PATH)/$(common_libsource_client_dir)/JackException.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackFrameTimer.cpp            $(LOCAL_PATH)/$(common_libsource_client_dir)/JackFrameTimer.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackGraphManager.cpp          $(LOCAL_PATH)/$(common_libsource_client_dir)/JackGraphManager.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackPort.cpp                  $(LOCAL_PATH)/$(common_libsource_client_dir)/JackPort.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackPortType.cpp              $(LOCAL_PATH)/$(common_libsource_client_dir)/JackPortType.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackAudioPort.cpp             $(LOCAL_PATH)/$(common_libsource_client_dir)/JackAudioPort.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackMidiPort.cpp              $(LOCAL_PATH)/$(common_libsource_client_dir)/JackMidiPort.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackMidiAPI.cpp               $(LOCAL_PATH)/$(common_libsource_client_dir)/JackMidiAPI.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackEngineControl.cpp         $(LOCAL_PATH)/$(common_libsource_client_dir)/JackEngineControl.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackShmMem.cpp                $(LOCAL_PATH)/$(common_libsource_client_dir)/JackShmMem.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackGenericClientChannel.cpp  $(LOCAL_PATH)/$(common_libsource_client_dir)/JackGenericClientChannel.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackGlobals.cpp               $(LOCAL_PATH)/$(common_libsource_client_dir)/JackGlobals.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackDebugClient.cpp           $(LOCAL_PATH)/$(common_libsource_client_dir)/JackDebugClient.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackTransportEngine.cpp       $(LOCAL_PATH)/$(common_libsource_client_dir)/JackTransportEngine.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/timestamps.c                  $(LOCAL_PATH)/$(common_libsource_client_dir)/timestamps.c)
$(shell cp -f $(LOCAL_PATH)/../common/JackTools.cpp                 $(LOCAL_PATH)/$(common_libsource_client_dir)/JackTools.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackMessageBuffer.cpp         $(LOCAL_PATH)/$(common_libsource_client_dir)/JackMessageBuffer.cpp)
$(shell cp -f $(LOCAL_PATH)/../common/JackEngineProfiling.cpp       $(LOCAL_PATH)/$(common_libsource_client_dir)/JackEngineProfiling.cpp)
$(shell cp -f $(LOCAL_PATH)/JackAndroidThread.cpp                   $(LOCAL_PATH)/$(common_libsource_client_dir)/JackAndroidThread.cpp)
$(shell cp -f $(LOCAL_PATH)/JackAndroidSemaphore.cpp                $(LOCAL_PATH)/$(common_libsource_client_dir)/JackAndroidSemaphore.cpp)
$(shell cp -f $(LOCAL_PATH)/../posix/JackPosixProcessSync.cpp       $(LOCAL_PATH)/$(common_libsource_client_dir)/JackPosixProcessSync.cpp)
$(shell cp -f $(LOCAL_PATH)/../posix/JackPosixMutex.cpp             $(LOCAL_PATH)/$(common_libsource_client_dir)/JackPosixMutex.cpp)
$(shell cp -f $(LOCAL_PATH)/../posix/JackSocket.cpp                 $(LOCAL_PATH)/$(common_libsource_client_dir)/JackSocket.cpp)
$(shell cp -f $(LOCAL_PATH)/../linux/JackLinuxTime.c                $(LOCAL_PATH)/$(common_libsource_client_dir)/JackLinuxTime.c)

common_libsource_server := \
    $(common_libsource_server_dir)/JackActivationCount.cpp \
    $(common_libsource_server_dir)/JackAPI.cpp \
    $(common_libsource_server_dir)/JackClient.cpp \
    $(common_libsource_server_dir)/JackConnectionManager.cpp \
    $(common_libsource_server_dir)/ringbuffer.c \
    $(common_libsource_server_dir)/JackError.cpp \
    $(common_libsource_server_dir)/JackException.cpp \
    $(common_libsource_server_dir)/JackFrameTimer.cpp \
    $(common_libsource_server_dir)/JackGraphManager.cpp \
    $(common_libsource_server_dir)/JackPort.cpp \
    $(common_libsource_server_dir)/JackPortType.cpp \
    $(common_libsource_server_dir)/JackAudioPort.cpp \
    $(common_libsource_server_dir)/JackMidiPort.cpp \
    $(common_libsource_server_dir)/JackMidiAPI.cpp \
    $(common_libsource_server_dir)/JackEngineControl.cpp \
    $(common_libsource_server_dir)/JackShmMem.cpp \
    $(common_libsource_server_dir)/JackGenericClientChannel.cpp \
    $(common_libsource_server_dir)/JackGlobals.cpp \
    $(common_libsource_server_dir)/JackDebugClient.cpp \
    $(common_libsource_server_dir)/JackTransportEngine.cpp \
    $(common_libsource_server_dir)/timestamps.c \
    $(common_libsource_server_dir)/JackTools.cpp \
    $(common_libsource_server_dir)/JackMessageBuffer.cpp \
    $(common_libsource_server_dir)/JackEngineProfiling.cpp \
    $(common_libsource_server_dir)/JackAndroidThread.cpp \
    $(common_libsource_server_dir)/JackAndroidSemaphore.cpp \
    $(common_libsource_server_dir)/JackPosixProcessSync.cpp \
    $(common_libsource_server_dir)/JackPosixMutex.cpp \
    $(common_libsource_server_dir)/JackSocket.cpp \
    $(common_libsource_server_dir)/JackLinuxTime.c

common_libsource_client := \
    $(common_libsource_client_dir)/JackActivationCount.cpp \
    $(common_libsource_client_dir)/JackAPI.cpp \
    $(common_libsource_client_dir)/JackClient.cpp \
    $(common_libsource_client_dir)/JackConnectionManager.cpp \
    $(common_libsource_client_dir)/ringbuffer.c \
    $(common_libsource_client_dir)/JackError.cpp \
    $(common_libsource_client_dir)/JackException.cpp \
    $(common_libsource_client_dir)/JackFrameTimer.cpp \
    $(common_libsource_client_dir)/JackGraphManager.cpp \
    $(common_libsource_client_dir)/JackPort.cpp \
    $(common_libsource_client_dir)/JackPortType.cpp \
    $(common_libsource_client_dir)/JackAudioPort.cpp \
    $(common_libsource_client_dir)/JackMidiPort.cpp \
    $(common_libsource_client_dir)/JackMidiAPI.cpp \
    $(common_libsource_client_dir)/JackEngineControl.cpp \
    $(common_libsource_client_dir)/JackShmMem.cpp \
    $(common_libsource_client_dir)/JackGenericClientChannel.cpp \
    $(common_libsource_client_dir)/JackGlobals.cpp \
    $(common_libsource_client_dir)/JackDebugClient.cpp \
    $(common_libsource_client_dir)/JackTransportEngine.cpp \
    $(common_libsource_client_dir)/timestamps.c \
    $(common_libsource_client_dir)/JackTools.cpp \
    $(common_libsource_client_dir)/JackMessageBuffer.cpp \
    $(common_libsource_client_dir)/JackEngineProfiling.cpp \
    $(common_libsource_client_dir)/JackAndroidThread.cpp \
    $(common_libsource_client_dir)/JackAndroidSemaphore.cpp \
    $(common_libsource_client_dir)/JackPosixProcessSync.cpp \
    $(common_libsource_client_dir)/JackPosixMutex.cpp \
    $(common_libsource_client_dir)/JackSocket.cpp \
    $(common_libsource_client_dir)/JackLinuxTime.c

server_libsource := \
    ../common/JackAudioDriver.cpp \
    ../common/JackTimedDriver.cpp \
    ../common/JackMidiDriver.cpp \
    ../common/JackDriver.cpp \
    ../common/JackEngine.cpp \
    ../common/JackExternalClient.cpp \
    ../common/JackFreewheelDriver.cpp \
    ../common/JackInternalClient.cpp \
    ../common/JackServer.cpp \
    ../common/JackThreadedDriver.cpp \
    ../common/JackRestartThreadedDriver.cpp \
    ../common/JackWaitThreadedDriver.cpp \
    ../common/JackServerAPI.cpp \
    ../common/JackDriverLoader.cpp \
    ../common/JackServerGlobals.cpp \
    ../common/JackControlAPI.cpp \
    JackControlAPIAndroid.cpp \
    ../common/JackNetTool.cpp \
    ../common/JackNetInterface.cpp \
    ../common/JackArgParser.cpp \
    ../common/JackRequestDecoder.cpp \
    ../common/JackMidiAsyncQueue.cpp \
    ../common/JackMidiAsyncWaitQueue.cpp \
    ../common/JackMidiBufferReadQueue.cpp \
    ../common/JackMidiBufferWriteQueue.cpp \
    ../common/JackMidiRawInputWriteQueue.cpp \
    ../common/JackMidiRawOutputWriteQueue.cpp \
    ../common/JackMidiReadQueue.cpp \
    ../common/JackMidiReceiveQueue.cpp \
    ../common/JackMidiSendQueue.cpp \
    ../common/JackMidiUtil.cpp \
    ../common/JackMidiWriteQueue.cpp \
    ../posix/JackSocketServerChannel.cpp \
    ../posix/JackSocketNotifyChannel.cpp \
    ../posix/JackSocketServerNotifyChannel.cpp \
    ../posix/JackNetUnixSocket.cpp

net_libsource := \
    ../common/JackNetAPI.cpp \
    ../common/JackNetInterface.cpp \
    ../common/JackNetTool.cpp \
    ../common/JackException.cpp \
    ../common/JackAudioAdapterInterface.cpp \
    ../common/JackLibSampleRateResampler.cpp \
    ../common/JackResampler.cpp \
    ../common/JackGlobals.cpp \
    ../posix/JackPosixMutex.cpp \
    ../common/ringbuffer.c \
    ../posix/JackNetUnixSocket.cpp \
    $(common_libsource_server_dir)/JackAndroidThread.cpp \
    ../linux/JackLinuxTime.c

client_libsource := \
    ../common/JackLibClient.cpp \
    ../common/JackLibAPI.cpp \
    ../posix/JackSocketClientChannel.cpp \
    ../posix/JackPosixServerLaunch.cpp

netadapter_libsource := \
    ../common/JackResampler.cpp \
    ../common/JackLibSampleRateResampler.cpp \
    ../common/JackAudioAdapter.cpp \
    ../common/JackAudioAdapterInterface.cpp \
    ../common/JackNetAdapter.cpp

audioadapter_libsource := \
    ../common/JackResampler.cpp \
    ../common/JackLibSampleRateResampler.cpp \
    ../common/JackAudioAdapter.cpp \
    ../common/JackAudioAdapterInterface.cpp \
    ../common/JackAudioAdapterFactory.cpp \
    ../linux/alsa/JackAlsaAdapter.cpp

ifeq ($(SUPPORT_ANDROID_REALTIME_SCHED), true)
sched_c_include := bionic/libc/bionic \
    frameworks/av/services/audioflinger
endif

# ========================================================
# libjackserver.so
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_libsource_server) $(server_libsource)
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libjackshm
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := libjackserver
ifeq ($(SUPPORT_ANDROID_REALTIME_SCHED), true)
LOCAL_CFLAGS += -DJACK_ANDROID_REALTIME_SCHED
LOCAL_C_INCLUDES += $(sched_c_include)
LOCAL_SHARED_LIBRARIES += libbinder
LOCAL_STATIC_LIBRARIES := libscheduling_policy
endif

include $(BUILD_SHARED_LIBRARY)

## ========================================================
## libjacknet.so
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := $(net_libsource)
#LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libsamplerate
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := libjacknet
#
#include $(BUILD_SHARED_LIBRARY)

# ========================================================
# libjack.so
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(common_libsource_client) $(client_libsource)
LOCAL_CFLAGS := $(common_cflags)
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libjackshm
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := libjack
ifeq ($(SUPPORT_ANDROID_REALTIME_SCHED), true)
LOCAL_CFLAGS += -DJACK_ANDROID_REALTIME_SCHED
LOCAL_C_INCLUDES += $(sched_c_include)
LOCAL_SHARED_LIBRARIES += libbinder
LOCAL_STATIC_LIBRARIES := libscheduling_policy
endif

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# netmanager.so
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../common/JackNetManager.cpp
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := netmanager

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# profiler.so
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../common/JackProfiler.cpp
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := profiler

include $(BUILD_SHARED_LIBRARY)

## ========================================================
## netadapter.so
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := $(netadapter_libsource)
#LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libsamplerate libjackserver
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := netadapter
#
#include $(BUILD_SHARED_LIBRARY)

## ========================================================
## audioadapter.so
## ========================================================
#ifeq ($(SUPPORT_ALSA_IN_JACK),true)
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := $(audioadapter_libsource)
#LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE -D_POSIX_SOURCE
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include $(ALSA_INCLUDES)
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libasound libsamplerate libjackserver
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := audioadapter
#
#include $(BUILD_SHARED_LIBRARY)
##endif

# ========================================================
# in.so - sapaproxy internal client
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := JackSapaProxy.cpp JackSapaProxyIn.cpp
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := in

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# out.so - sapaproxy internal client
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := JackSapaProxy.cpp JackSapaProxyOut.cpp
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := out

include $(BUILD_SHARED_LIBRARY)

##########################################################
# linux
##########################################################

# ========================================================
# jackd
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ../common/Jackdmp.cpp
#    ../dbus/reserve.c
#    ../dbus/audio_reserve.c
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(JACK_STL_LDFLAGS) -ldl -Wl,--no-fatal-warnings
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libutils libjackserver
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jackd

include $(BUILD_EXECUTABLE)

# ========================================================
# driver - dummy
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../common/JackDummyDriver.cpp
#'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_dummy

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# driver - alsa
# ========================================================
ifeq ($(SUPPORT_ALSA_IN_JACK),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    ../linux/alsa/JackAlsaDriver.cpp \
    ../linux/alsa/alsa_midi_jackmp.cpp \
    ../common/memops.c \
    ../linux/alsa/generic_hw.c \
    ../linux/alsa/hdsp.c \
    ../linux/alsa/alsa_driver.c \
    ../linux/alsa/hammerfall.c \
    ../linux/alsa/ice1712.c
#    ../linux/alsa/alsa_rawmidi.c
#    ../linux/alsa/alsa_seqmidi.c
#'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE -D_POSIX_SOURCE -D_XOPEN_SOURCE=600
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes) $(ALSA_INCLUDES)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver libasound
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_alsa

include $(BUILD_SHARED_LIBRARY)
endif

## ========================================================
## driver - alsarawmidi
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := \
#    ../linux/alsarawmidi/JackALSARawMidiDriver.cpp \
#    ../linux/alsarawmidi/JackALSARawMidiInputPort.cpp \
#    ../linux/alsarawmidi/JackALSARawMidiOutputPort.cpp \
#    ../linux/alsarawmidi/JackALSARawMidiPort.cpp \
#    ../linux/alsarawmidi/JackALSARawMidiReceiveQueue.cpp \
#    ../linux/alsarawmidi/JackALSARawMidiSendQueue.cpp \
#    ../linux/alsarawmidi/JackALSARawMidiUtil.cpp
##'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
#LOCAL_CFLAGS := $(common_cflags) -D_POSIX_SOURCE -D__ALSA_RAWMIDI_H
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes) $(ALSA_INCLUDES)
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver libasound
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_alsarawmidi
#
#include $(BUILD_SHARED_LIBRARY)

## LIBFREEBOB required
## ========================================================
## driver - freebob
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../linux/freebob/JackFreebobDriver.cpp
##'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
#LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes)
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_freebob
#
#include $(BUILD_SHARED_LIBRARY)

## LIBFFADO required
## ========================================================
## driver - firewire
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := \
#    ../linux/firewire/JackFFADODriver.cpp \
#    ../linux/firewire/JackFFADOMidiInputPort.cpp \
#    ../linux/firewire/JackFFADOMidiOutputPort.cpp \
#    ../linux/firewire/JackFFADOMidiReceiveQueue.cpp \
#    ../linux/firewire/JackFFADOMidiSendQueue.cpp
##'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
#LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes)
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_firewire
#
#include $(BUILD_SHARED_LIBRARY)

# ========================================================
# driver - net
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../common/JackNetDriver.cpp
#'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_net

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# driver - loopback
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../common/JackLoopbackDriver.cpp
#'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_loopback

include $(BUILD_SHARED_LIBRARY)

##HAVE_SAMPLERATE, HAVE_CELT required
## ========================================================
## driver - netone
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := \
#    ../common/JackNetOneDriver.cpp \
#    ../common/netjack.c \
#    ../common/netjack_packet.c
##'HAVE_CONFIG_H','SERVER_SIDE', 'HAVE_PPOLL', 'HAVE_TIMERFD
#LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
#LOCAL_CPPFLAGS := $(common_cppflags)
#LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include
#LOCAL_SHARED_LIBRARIES := libc libdl libcutils libsamplerate libjackserver
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_netone
#
#include $(BUILD_SHARED_LIBRARY)

##########################################################
# android
##########################################################

# ========================================================
# libjackshm.so
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := BnAndroidShm.cpp BpAndroidShm.cpp IAndroidShm.cpp AndroidShm.cpp Shm.cpp
LOCAL_CFLAGS := $(common_shm_cflags) -DSERVER_SIDE
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libutils libbinder
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := libjackshm

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# jack_goldfish.so - Goldfish driver for emulator
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := JackGoldfishDriver.cpp
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_goldfish

include $(BUILD_SHARED_LIBRARY)

# ========================================================
# jack_opensles.so - OpenSL ES driver
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := JackOpenSLESDriver.cpp opensl_io.c
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes) frameworks/wilhelm/include
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver libOpenSLES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_opensles

include $(BUILD_SHARED_LIBRARY)

##########################################################
# android/AndroidShmServer
##########################################################

# ========================================================
# androidshmservice
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ./AndroidShmServer/main_androidshmservice.cpp
LOCAL_CFLAGS := $(common_cflags)
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libcutils libutils libbinder libjackshm
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE:= androidshmservice

include $(BUILD_EXECUTABLE)

# ========================================================
# shmservicetest
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ./AndroidShmServer/test/shmservicetest.cpp
LOCAL_CFLAGS := $(common_cflags) -DLOG_TAG=\"ShmServiceTest\"
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libcutils libutils libjackshm libbinder
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := shmservicetest

include $(BUILD_EXECUTABLE)

# ========================================================
# shmservicedump
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ./AndroidShmServer/test/shmservicedump.cpp
LOCAL_CFLAGS := $(common_cflags) -DLOG_TAG=\"ShmServiceDump\"
LOCAL_CPPFLAGS := $(common_cppflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_SHARED_LIBRARIES := libcutils libutils libjackshm libbinder
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := shmservicedump

include $(BUILD_EXECUTABLE)

##########################################################
# example-clients
##########################################################

# ========================================================
# jack_freewheel
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/freewheel.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_freewheel

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_connect
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/connect.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_connect

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_disconnect
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/connect.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_disconnect

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_lsp
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/lsp.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_lsp

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_metro
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/metro.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_metro

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_midiseq
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/midiseq.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_midiseq

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_midisine
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/midisine.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_midisine

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_showtime
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/showtime.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_showtime

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_simple_client
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/simple_client.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_simple_client

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_zombie
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/zombie.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_zombie

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_load
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/ipload.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_load

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_unload
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/ipunload.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_unload

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_alias
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/alias.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_alias

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_bufsize
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/bufsize.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_bufsize

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_wait
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/wait.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_wait

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_samplerate
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/samplerate.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_samplerate

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_evmon
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/evmon.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_evmon

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_monitor_client
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/monitor_client.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_monitor_client

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_thru
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/thru_client.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_thru

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_cpu_load
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/cpu_load.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_cpu_load

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_simple_session_client
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/simple_session_client.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_simple_session_client

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_session_notify
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/session_notify.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_session_notify

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_server_control
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/server_control.cpp
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_server_control

include $(BUILD_EXECUTABLE)

## ========================================================
## jack_net_slave
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../example-clients/netslave.c
#LOCAL_CFLAGS := $(common_cflags)
#LOCAL_LDFLAGS := $(common_ldflags)
#LOCAL_C_INCLUDES := $(common_c_includes)
#LOCAL_SHARED_LIBRARIES := libjacknet
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_net_slave
#
#include $(BUILD_EXECUTABLE)

## ========================================================
## jack_net_master
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../example-clients/netmaster.c
#LOCAL_CFLAGS := $(common_cflags)
#LOCAL_LDFLAGS := $(common_ldflags)
#LOCAL_C_INCLUDES := $(common_c_includes)
#LOCAL_SHARED_LIBRARIES := libjacknet
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_net_master
#
#include $(BUILD_EXECUTABLE)

# ========================================================
# jack_latent_client
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/latent_client.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_latent_client

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_midi_dump
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/midi_dump.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_midi_dump

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_midi_latency_test
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/midi_latency_test.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_midi_latency_test

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_transport
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/transport.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_transport

include $(BUILD_EXECUTABLE)

## ========================================================
## jack_rec
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../example-clients/capture_client.c
#LOCAL_CFLAGS := $(common_cflags)
#LOCAL_LDFLAGS := $(common_ldflags)
#LOCAL_C_INCLUDES := $(common_c_includes)  $(JACK_ROOT)/../libsndfile/src
#LOCAL_SHARED_LIBRARIES := libjack libsndfile
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_rec
#
#include $(BUILD_EXECUTABLE)

## ========================================================
## jack_netsource
## ========================================================
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../example-clients/netsource.c  ../common/netjack_packet.c
#LOCAL_CFLAGS := $(common_cflags) -DNO_JACK_ERROR
#LOCAL_LDFLAGS := $(common_ldflags)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include
#LOCAL_SHARED_LIBRARIES := libsamplerate libjack
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := jack_netsource
#
#include $(BUILD_EXECUTABLE)

## ========================================================
## alsa_in
## ========================================================
#ifeq ($(SUPPORT_ALSA_IN_JACK),true)
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../example-clients/alsa_in.c  ../common/memops.c
#LOCAL_CFLAGS := $(common_cflags) -DNO_JACK_ERROR -D_POSIX_SOURCE -D_XOPEN_SOURCE=600
#LOCAL_LDFLAGS := $(common_ldflags)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include $(ALSA_INCLUDES)
#LOCAL_SHARED_LIBRARIES := libasound libsamplerate libjack
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := alsa_in
#
#include $(BUILD_EXECUTABLE)
#endif

## ========================================================
## alsa_out
## ========================================================
#ifeq ($(SUPPORT_ALSA_IN_JACK),true)
#include $(CLEAR_VARS)
#
#LOCAL_SRC_FILES := ../example-clients/alsa_out.c  ../common/memops.c
#LOCAL_CFLAGS := $(common_cflags) -DNO_JACK_ERROR -D_POSIX_SOURCE -D_XOPEN_SOURCE=600
#LOCAL_LDFLAGS := $(common_ldflags)
#LOCAL_C_INCLUDES := $(common_c_includes) $(JACK_ROOT)/../libsamplerate/include $(ALSA_INCLUDES)
#LOCAL_SHARED_LIBRARIES := libasound libsamplerate libjack
#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_MODULE := alsa_out
#
#include $(BUILD_EXECUTABLE)
#endif

# ========================================================
# inprocess
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../example-clients/inprocess.c
LOCAL_CFLAGS := $(common_cflags) -DSERVER_SIDE
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libc libdl libcutils libjackserver
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/jack
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := inprocess

include $(BUILD_SHARED_LIBRARY)

##########################################################
# tests
##########################################################

# ========================================================
# jack_test
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../tests/test.cpp
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack libjackshm
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_test

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_cpu
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../tests/cpu.c
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack libjackshm
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_cpu

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_iodelay
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../tests/iodelay.cpp
LOCAL_CFLAGS := $(common_cflags)
LOCAL_CFLAGS += -Wno-narrowing
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack libjackshm
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_iodelay

include $(BUILD_EXECUTABLE)

# ========================================================
# jack_multiple_metro
# ========================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ../tests/external_metro.cpp
LOCAL_CFLAGS := $(common_cflags)
LOCAL_LDFLAGS := $(common_ldflags) $(JACK_STL_LDFLAGS)
LOCAL_C_INCLUDES := $(common_c_includes)
LOCAL_SHARED_LIBRARIES := libjack libjackshm
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE := jack_multiple_metro

include $(BUILD_EXECUTABLE)

