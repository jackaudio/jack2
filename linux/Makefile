
# Choose the installation location
prefix := /usr/local
prefix := ${DESTDIR}${prefix}
libdir := lib

subprojects := ../common ../common/jack ../tests ../example-clients alsa freebob

sources := $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard ../common/*.cpp) $(wildcard ../tests/*.cpp)

TARGET_LINUX_SERVER := jackdmp

TARGET_LINUX_SERVER_LIB := libjackdmp.so

TARGET_LINUX_CLIENT_LIB := libjackmp.so

TARGET_LINUX_WRAPPER_LIB := libjackwrapper.so

TARGET_LINUX_ALSA := jack_alsa.so

TARGET_LINUX_FREEBOB := jack_freebob.so

TARGET_LINUX_DUMMY := jack_dummy.so

VPATH := $(subprojects)

LIB_LINUX := -lpthread -lrt -lasound


objects_common_server_lib := JackActivationCount.o JackAPI.o JackAudioDriver.o JackClient.o JackConnectionManager.o \
	JackDriver.o JackEngine.o JackEngineTiming.o JackError.o JackExternalClient.o JackFrameTimer.o \
	JackFreewheelDriver.o JackGlobalsServer.o JackGraphManager.o JackInternalClient.o JackPort.o JackPosixSemaphore.o JackPosixThread.o JackFifo.o  JackLoopbackDriver.o\
	JackServer.o JackShmMem.o JackThreadedDriver.o shm.o JackSocket.o JackSocketServerChannel.o  JackSocketNotifyChannel.o \
	JackSocketServerNotifyChannel.o JackTime.o JackServerAPI.o JackGlobals.o JackDriverLoader.o internal_metro.o JackDebugClient.o  \
	JackTransportEngine.o JackServerGlobals.o JackServerLaunch.o timestamps.o

objects_common_client_lib := JackActivationCount.o JackAPI.o JackClient.o JackConnectionManager.o ringbuffer.o  JackServerLaunch.o\
	JackError.o JackFrameTimer.o JackGlobalsClient.o JackGraphManager.o JackLibClient.o JackLibAPI.o JackPort.o JackPosixSemaphore.o JackFifo.o \
	JackPosixThread.o JackShmMem.o shm.o JackSocket.o JackSocketClientChannel.o JackTime.o JackGlobals.o JackDebugClient.o JackTransportEngine.o timestamps.o
	

objects_linux_server := Jackdmp.o 

objects_linux_alsa := JackAlsaDriver.o memops.o generic_hw.o hdsp.o hammerfall.o ice1712.o

objects_linux_freebob := JackFreebobDriver.o

objects_linux_dummy := JackDummyDriver.o  

CFLAGS := -g -fPIC -DUSE_POSIX_SHM $(addprefix -I, $(subprojects)) $(CFLAGS)
CXXFLAGS := -g  -fPIC -DSOCKET_RPC_FIFO_SEMA -D__SMP__ -DADDON_DIR=\"$(prefix)\"  $(addprefix -I, $(subprojects)) $(CXXFLAGS) 

#CFLAGS := -g -fPIC -DUSE_POSIX_SHM $(addprefix -I, $(subprojects)) $(CFLAGS)
#CXXFLAGS := -g -fPIC -DSOCKET_RPC_FIFO_SEMA -D__SMP__ -DADDON_DIR=\"$(prefix)\" $(addprefix -I, $(subprojects)) $(CXXFLAGS) 

# Use this line to compile with POSIX names semaphore
#CXXFLAGS := -g -O3 -fPIC -DSOCKET_RPC_POSIX_SEMA -D__SMP__ -DADDON_DIR=\"$(prefix)\"  $(addprefix -I, $(subprojects)) $(CXXFLAGS)

# Add the -D__CLIENTDEBUG__  flag to activate client validation

all : $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) \
        $(TARGET_LINUX_SERVER) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_DUMMY) \
        synchroClient synchroServer synchroServerClient testSem jack_test inprocess jack_load jack_unload

freebob : $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) \
        $(TARGET_LINUX_SERVER) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_FREEBOB) $(TARGET_LINUX_DUMMY) \
        synchroClient synchroServer synchroServerClient testSem jack_test

synchroClient: JackPosixSemaphore.o testSynchroClient.o JackPosixThread.o JackError.o JackFifo.o 
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSynchroClient.o JackPosixThread.o JackError.o JackFifo.o $(LIB_LINUX) -o synchroClient

synchroServer: JackPosixSemaphore.o testSynchroServer.o JackPosixThread.o JackError.o JackFifo.o 
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSynchroServer.o JackPosixThread.o JackError.o JackFifo.o $(LIB_LINUX) -o synchroServer

synchroServerClient: JackPosixSemaphore.o testSynchroServerClient.o JackPosixThread.o JackError.o JackFifo.o JackPthreadCond.o JackShmMem.o shm.o
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSynchroServerClient.o JackPosixThread.o JackError.o JackFifo.o JackShmMem.o shm.o JackPthreadCond.o $(LIB_LINUX) -o synchroServerClient
		
testSem: JackPosixSemaphore.o testSem.o JackPosixThread.o JackError.o JackFifo.o 
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSem.o JackPosixThread.o JackError.o JackFifo.o $(LIB_LINUX) -o testSem

jack_test: jack_test.o
		$(CXX) $(CXXFLAGS) jack_test.o -L. -ljackmp -o jack_test

inprocess: inprocess.o
		$(CXX) $(CXXFLAGS) -shared inprocess.o -L. -ljackmp -o inprocess.so

jack_load: ipload.o
		$(CXX) $(CXXFLAGS) ipload.o -L. -ljackmp -o jack_load

jack_unload: ipunload.o
		$(CXX) $(CXXFLAGS) ipunload.o -L. -ljackmp -o jack_unload
		
$(TARGET_LINUX_SERVER_LIB) : $(objects_common_server_lib)
	$(CXX) $(CXXFLAGS) -shared $(objects_common_server_lib) $(LIB_LINUX) -o $(TARGET_LINUX_SERVER_LIB)

$(TARGET_LINUX_CLIENT_LIB) : $(objects_common_client_lib) $(objects_linux_lib)
	$(CXX) $(CXXFLAGS) -shared $(objects_common_client_lib) $(objects_linux_lib) $(LIB_LINUX)  -o $(TARGET_LINUX_CLIENT_LIB)

$(TARGET_LINUX_WRAPPER_LIB) : JackAPIWrapper.o ringbuffer.o
	$(CXX) $(CXXFLAGS) -shared JackAPIWrapper.o ringbuffer.o -o $(TARGET_LINUX_WRAPPER_LIB)

$(TARGET_LINUX_SERVER) : $(objects_linux_server)
	$(CXX) $(CXXFLAGS)  $(objects_linux_server) $(LIB_LINUX) libjackdmp.so -o $(TARGET_LINUX_SERVER)

$(TARGET_LINUX_ALSA) : $(objects_linux_alsa) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_alsa) $(LIB_LINUX) libjackdmp.so  -o $(TARGET_LINUX_ALSA)

$(TARGET_LINUX_FREEBOB) : $(objects_linux_freebob) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_freebob) $(LIB_LINUX) -lfreebob libjackdmp.so  -o $(TARGET_LINUX_FREEBOB)

$(TARGET_LINUX_DUMMY) : $(objects_linux_dummy) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_dummy) $(LIB_LINUX) libjackdmp.so  -o $(TARGET_LINUX_DUMMY)


# Install jackdmp and overwrite jack installation
install:
	cp jackdmp $(prefix)/bin
	cp libjackmp.so $(prefix)/$(libdir)
	cp libjackdmp.so $(prefix)/$(libdir)
	install -d $(prefix)/$(libdir)/jackmp/
	cp jack_alsa.so $(prefix)/$(libdir)/jackmp
	cp jack_dummy.so $(prefix)/$(libdir)/jackmp
	[ -f jack_freebob.so ]  && cp jack_freebob.so $(prefix)/$(libdir)/jackmp || echo "freebob driver not installed"
	cd $(prefix)/$(libdir) && [ -f libjack.so.0.0.23 ] && mv -f libjack.so.0.0.23 tmp_libjack.so.0.0.23 || echo "Jack not found, continue..."
	cd $(prefix)/bin && [ -f jackd ]  && mv -f jackd tmp_jackd || echo "jackd server not found" 
	cd $(prefix)/$(libdir) && rm -f libjack.so*
	cd $(prefix)/$(libdir) && ln -s libjackmp.so  libjack.so
	cd $(prefix)/$(libdir) && ln -s libjackmp.so  libjack.so.0
	cd $(prefix)/$(libdir) && ln -s jackdmp jackd
	/sbin/ldconfig

# Remove jackdmp and tries to restore jack
uninstall: remove

remove:
	rm $(prefix)/bin/jackdmp
	rm $(prefix)/$(libdir)/libjackmp.so
	rm $(prefix)/$(libdir)/libjackdmp.so
	rm -r $(prefix)/$(libdir)/jackmp
	cd $(prefix)/$(libdir) && rm -f libjack.so*
	cd $(prefix)/$(libdir) && [ -f tmp_libjack.so.0.0.23 ] && mv -f tmp_libjack.so.0.0.23 libjack.so.0.0.23 \
	&& ln -s libjack.so.0.0.23  libjack.so && ln -s libjack.so.0.0.23  libjack.so.0 || echo "libjack not restored"
	cd $(prefix)/bin && [ -f tmp_jackd ] && mv -f tmp_jackd jackd || echo "jackd server not restored"
	/sbin/ldconfig

clean :
	rm -f *.o
	rm -f $(TARGET_LINUX_SERVER)  $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_FREEBOB) $(TARGET_LINUX_DUMMY) \
	synchroClient synchroServer synchroServerClient testSem jack_test jack_load jack_unload inprocess.so	
depend :
	#makedepend -w120 -Y -- $(CXXFLAGS) -- $(sources) 

dox:
	doxygen	
	
# DO NOT DELETE
