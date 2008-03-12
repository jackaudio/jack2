# Type "make" or "make all" to compile ALSA driver, "make freebob" to compile FREEBOB driver and "make firewire" to compile FFADO driver.

# Choose the installation location
prefix := /usr/local
prefix := ${DESTDIR}${prefix}
libdir := lib

subprojects :=  ../common ../common/jack ../tests ../example-clients alsa freebob firewire ../linux ..

sources := $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard ../common/*.cpp) $(wildcard ../tests/*.cpp)

TARGET_LINUX_SERVER := jackdmp

TARGET_LINUX_SERVER_LIB := libjackservermp.so

TARGET_LINUX_CLIENT_LIB := libjackmp.so

TARGET_LINUX_WRAPPER_LIB := libjackwrapper.so

TARGET_LINUX_ALSA := jack_alsa.so

TARGET_LINUX_FREEBOB := jack_freebob.so

TARGET_LINUX_FIREWIRE := jack_firewire.so

TARGET_LINUX_DUMMY := jack_dummy.so

VPATH := $(subprojects)

LIB_LINUX := -lpthread -lrt -lasound

objects_common_server_lib := JackActivationCount.o JackAPI.o JackAudioDriver.o JackClient.o JackConnectionManager.o \
	                     JackDriver.o JackEngine.o JackEngineControl.o JackError.o JackExternalClient.o JackFrameTimer.o \
	                     JackFreewheelDriver.o JackGlobalsServer.o JackGraphManager.o JackInternalClient.o JackPort.o JackPosixSemaphore.o \
                             JackPosixThread.o JackFifo.o  JackLoopbackDriver.o JackPortType.o JackAudioPort.o JackMidiPort.o JackMidiAPI.o \
	                     JackServer.o JackShmMem.o JackThreadedDriver.o shm.o JackSocket.o JackSocketServerChannel.o  JackSocketNotifyChannel.o \
	                     JackSocketServerNotifyChannel.o JackTime.o JackServerAPI.o JackGlobals.o JackDriverLoader.o internal_metro.o JackDebugClient.o  \
	                     JackTransportEngine.o JackServerGlobals.o JackServerLaunch.o timestamps.o JackTools.o ringbuffer.o

objects_common_client_lib := JackActivationCount.o JackAPI.o JackClient.o JackConnectionManager.o ringbuffer.o  JackServerLaunch.o\
	                     JackError.o JackFrameTimer.o JackGlobalsClient.o JackGraphManager.o JackLibClient.o JackLibAPI.o JackPort.o JackPosixSemaphore.o  \
	                     JackFifo.o JackPortType.o JackAudioPort.o JackMidiPort.o JackMidiAPI.o JackEngineControl.o JackPosixThread.o JackShmMem.o \
	                     shm.o JackSocket.o JackSocketClientChannel.o JackTime.o JackGlobals.o JackDebugClient.o JackTransportEngine.o timestamps.o JackTools.o
	
objects_linux_server := Jackdmp.o 

objects_linux_alsamidi := alsa_rawmidi.o alsa_seqmidi.o alsa_midi_jackmp.o

objects_linux_alsa := JackAlsaDriver.o memops.o generic_hw.o hdsp.o hammerfall.o ice1712.o $(objects_linux_alsamidi)

objects_linux_freebob := JackFreebobDriver.o

objects_linux_firewire := JackFFADODriver.o

objects_linux_dummy := JackDummyDriver.o  

CFLAGS := -g -O3 -fPIC -DJACKMP -DUSE_POSIX_SHM $(addprefix -I, $(subprojects)) $(CFLAGS)
CXXFLAGS := -g -O3 -fPIC -DJACKMP -DSOCKET_RPC_FIFO_SEMA -D__SMP__ -DADDON_DIR=\"$(prefix)/$(libdir)/jackmp\" -DLIB_DIR=\"$(libdir)\" -DJACK_LOCATION=\"$(prefix)/bin\" $(addprefix -I, $(subprojects)) $(CXXFLAGS) 

#CFLAGS := -g -fPIC -DUSE_POSIX_SHM $(addprefix -I, $(subprojects)) $(CFLAGS)
#CXXFLAGS := -g -fPIC -DSOCKET_RPC_FIFO_SEMA -D__SMP__ -DADDON_DIR=\"$(prefix)/$(libdir)/jackmp\" -DLIB_DIR=\"$(libdir)\" -DJACK_LOCATION=\"$(prefix)/bin\" $(addprefix -I, $(subprojects)) $(CXXFLAGS) 

# Use this line to compile with POSIX names semaphore
#CXXFLAGS := -g -O3 -fPIC -DSOCKET_RPC_POSIX_SEMA -D__SMP__ -DADDON_DIR=\"$(prefix)/$(libdir)/jackmp\" -DLIB_DIR=\"$(libdir)\" -DJACK_LOCATION=\"$(prefix)/bin\" $(addprefix -I, $(subprojects)) $(CXXFLAGS)

all : $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) \
        $(TARGET_LINUX_SERVER) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_DUMMY) \
        synchroClient synchroServer synchroServerClient testSem jack_test inprocess jack_load jack_unload

freebob : $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) \
        $(TARGET_LINUX_SERVER) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_FREEBOB) $(TARGET_LINUX_DUMMY) \
        synchroClient synchroServer synchroServerClient testSem jack_test inprocess jack_load jack_unload

firewire : $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) \
        $(TARGET_LINUX_SERVER) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_FIREWIRE) $(TARGET_LINUX_DUMMY) \
        synchroClient synchroServer synchroServerClient testSem jack_test

synchroClient: JackPosixSemaphore.o testSynchroClient.o JackPosixThread.o JackError.o JackFifo.o 
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSynchroClient.o JackPosixThread.o JackError.o JackFifo.o $(LIB_LINUX) -o synchroClient

synchroServer: JackPosixSemaphore.o testSynchroServer.o JackPosixThread.o JackError.o JackFifo.o 
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSynchroServer.o JackPosixThread.o JackError.o JackFifo.o $(LIB_LINUX) -o synchroServer

synchroServerClient: JackPosixSemaphore.o testSynchroServerClient.o JackPosixThread.o JackError.o JackFifo.o JackShmMem.o shm.o
		$(CXX) $(CXXFLAGS) JackPosixSemaphore.o testSynchroServerClient.o JackPosixThread.o JackError.o JackFifo.o JackShmMem.o shm.o $(LIB_LINUX) -o synchroServerClient
		
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
	$(CXX) $(CXXFLAGS)  $(objects_linux_server) $(LIB_LINUX) libjackservermp.so -o $(TARGET_LINUX_SERVER)

$(TARGET_LINUX_ALSA) : $(objects_linux_alsa) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_alsa) $(LIB_LINUX) libjackmp.so libjackservermp.so  -o $(TARGET_LINUX_ALSA)

$(TARGET_LINUX_FREEBOB) : $(objects_linux_freebob) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_freebob) $(LIB_LINUX) -lfreebob libjackservermp.so  -o $(TARGET_LINUX_FREEBOB)

$(TARGET_LINUX_FIREWIRE) : $(objects_linux_firewire) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_firewire) $(LIB_LINUX) -lffado libjackservermp.so  -o $(TARGET_LINUX_FIREWIRE)

$(TARGET_LINUX_DUMMY) : $(objects_linux_dummy) 
	$(CXX) $(CXXFLAGS) -shared $(objects_linux_dummy) $(LIB_LINUX) libjackservermp.so  -o $(TARGET_LINUX_DUMMY)

# Install jackdmp and overwrite jack installation
install:
	cp jackdmp $(prefix)/bin
	cp libjackmp.so $(prefix)/$(libdir)
	cp libjackservermp.so $(prefix)/$(libdir)
	install -d $(prefix)/$(libdir)/jackmp/
	cp jack_alsa.so $(prefix)/$(libdir)/jackmp
	cp jack_dummy.so $(prefix)/$(libdir)/jackmp
	[ -f jack_freebob.so ]  && cp jack_freebob.so $(prefix)/$(libdir)/jackmp || echo "freebob driver not installed"
	[ -f jack_firewire.so ]  && cp jack_firewire.so $(prefix)/$(libdir)/jackmp || echo "firewire driver not installed"
	cd $(prefix)/bin && [ -f jackd ]  && mv -f jackd tmp_jackd || echo "jackd server not found, continue..."
	cd $(prefix)/$(libdir) && [ -f libjack.so.0.0.28 ] && mv -f libjack.so.0.0.28 tmp_libjack.so.0.0.28 || echo "libjack not found, continue..."
	cd $(prefix)/include &&  [ -d jack ] && mv -f jack tmp_jack || echo "jack headers not found, continue..."
	install -d $(prefix)/include/jack/
	cp ../common/jack/*.h $(prefix)/include/jack/
	cd $(prefix)/$(libdir) && rm -f libjack.so*
	cd $(prefix)/$(libdir) && ln -s libjackmp.so  libjack.so
	cd $(prefix)/$(libdir) && ln -s libjackmp.so  libjack.so.0
	cd $(prefix)/bin && ln -s jackdmp jackd
	/sbin/ldconfig

installwrapper:
	cp jackdmp $(prefix)/bin
	cp libjackmp.so $(prefix)/$(libdir)
	cp libjackservermp.so $(prefix)/$(libdir)
	cp libjackwrapper.so $(prefix)/$(libdir)
	install -d $(prefix)/$(libdir)/jackmp/
	cp jack_alsa.so $(prefix)/$(libdir)/jackmp
	cp jack_dummy.so $(prefix)/$(libdir)/jackmp
	[ -f jack_freebob.so ]  && cp jack_freebob.so $(prefix)/$(libdir)/jackmp || echo "freebob driver not installed"
	[ -f jack_firewire.so ]  && cp jack_firewire.so $(prefix)/$(libdir)/jackmp || echo "firewire driver not installed"
	cd $(prefix)/include &&  [ -d jack ] && mv -f jack tmp_jack || echo "jack headers not found, continue..."
	install -d $(prefix)/include/jack/
	cp ../common/jack/*.h $(prefix)/include/jack/
	cd $(prefix)/$(libdir) && rm -f libjack.so
	cd $(prefix)/$(libdir) && rm -f libjack.so.0
	cd $(prefix)/$(libdir) && ln -s libjackwrapper.so  libjack.so
	cd $(prefix)/$(libdir) && ln -s libjackwrapper.so  libjack.so.0
	#/sbin/ldconfig

# Remove jackdmp and tries to restore jack
uninstall: remove

remove:
	rm $(prefix)/bin/jackdmp
	rm $(prefix)/$(libdir)/libjackmp.so
	rm $(prefix)/$(libdir)/libjackservermp.so
	rm $(prefix)/$(libdir)/libjackwrapper.so
	rm -r $(prefix)/$(libdir)/jackmp
	rm -r $(prefix)/include/jack
	cd $(prefix)/$(libdir) && rm -f libjack.so*
	cd $(prefix)/bin && [ -f tmp_jackd ] && mv -f tmp_jackd jackd || echo "jackd server not restored, continue.."
	cd $(prefix)/$(libdir) && [ -f tmp_libjack.so.0.0.28 ] && mv -f tmp_libjack.so.0.0.28 libjack.so.0.0.28 \
	&& ln -s libjack.so.0.0.28  libjack.so && ln -s libjack.so.0.0.28  libjack.so.0 || echo "libjack not restored, continue.."
	cd $(prefix)/include && [ -d tmp_jack ] && mv -f tmp_jack jack || echo "jack headers not restored, continue..."
	/sbin/ldconfig

clean :
	rm -f *.o
	rm -f $(TARGET_LINUX_SERVER)  $(TARGET_LINUX_SERVER_LIB) $(TARGET_LINUX_CLIENT_LIB) $(TARGET_LINUX_WRAPPER_LIB) $(TARGET_LINUX_ALSA) $(TARGET_LINUX_FREEBOB) $(TARGET_LINUX_FIREWIRE) $(TARGET_LINUX_DUMMY) \
	synchroClient synchroServer synchroServerClient testSem jack_test jack_load jack_unload inprocess.so	
depend :
	#makedepend -w120 -Y -- $(CXXFLAGS) -- $(sources) 

dox:
	doxygen	
	
# DO NOT DELETE
