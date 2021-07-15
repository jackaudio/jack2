ChangeLog
#########

* 1.9.19 (2021-07-15)

  * WIP (note to write asking CI help)
  * Add jack_position_t::tick_double, and flags around it
  * Add zalsa "-w" argument to wait for soundcard to be available
  * Bump internal protocol version to 9 (due to struct alignment)
  * Fix alignment of fields for atomic accesses
  * Fix build for platforms needing __STDC_FORMAT_MACROS
  * Fix compilation of documentation

* 1.9.18 (2021-04-15)

  * Add zalsa_in/out as internal client (based on zita-a2j/j2a and jack1 code)
  * Fix jack_midi_dump deadlock on close after the jack server is restarted
  * Fix interrupt signal for linux futex waits
  * Fix usage of meta-data in official macOS builds (private DB errors)
  * Log error message when cleaning previous DB (macOS and Windows)

* 1.9.17 (2021-01-15)

  * Fix jack_control stopping after first command iteration
  * Fix library compat/current version under macOS
  * Fix return codes of jackd on success
  * Ignore DB_VERSION_MISMATCH error on windows, it is expected
  * Remove old workaround for asio4all, as it breaks with jack-router

External changes, related to macOS/Windows installer:
  * Add jack-router to Windows installer, opt-in
  * Fix registry keys for Windows, add 32bit compat ones on 64bit
  * Support for arm64 macOS builds
  * Show welcome and license pages on windows installer
  * Update QjackCtl used in macOS/Windows installers to v0.9.0, with some commits cherry-picked from develop branch

* 1.9.16 (2020-10-16)

  * Fix/revert a change in how meta-data definitions were exposed (regression in 1.9.15)
  * Remove jack-router Windows code from the repository

* 1.9.15 (2020-10-15)

  * Automated builds for macOS and Windows (see jackaudio/jack2-releases repository)
  * Adapt wscript Windows build configuration to match old v1.9.11 installer
  * Bump maximum default number of clients and ports (now 256 clients and 2048 ports)
  * Delete various macOS and Windows-related files from the source code (no longer relevant)
  * Mark JACK-Session as deprecated, please use NSM instead
  * Remove unnecessary GPL include from LGPL code
  * Split example-clients and tools, as done in JACK1 many years ago (WIP)
  * Write Windows registry key during installation, so 3rd parties can find jackd.exe (as HKLM\\Software\\JACK\\Location)
  * jack_control: Fix handling of dbus bytes
  * jack_control: Return a proper exit status on DBus exception
  * jack_property: Fix possible crash with "-l" argument usage
  * jack_wait: Add client name option -n/--name
  * Fix compilation of documentation
  * Fix compilation of mixed mode with meta-data enabled
  * Fix compilation with mingw
  * Fix client-side crash if initial meta-data DB setup fails
  * Fix macOS semaphore usage, so it works again
  * Several fixes for Windows (with contributions from Kjetil S. Matheussen)
  * Several minor fixes and grammar corrections (with contributions from Adam Miartus and Timo Wischer)

* 1.9.14 (2019-10-28)

  * Fix ARM build
  * Fix mixed mode build when meta-data is enabled
  * Fix blocking DBus device reservation, so it plays nice with others (like PipeWire)
  * Use python3 for the waf build scripts

* 1.9.13 (2019-10-06)

  * Meta-data API implementation. (and a few tools updated with support for it)
  * Correct GPL licence to LGPL for files needed to build libjack.
  * Remove FreeBoB backend (superseded by FFADO).
  * define JACK_LIB_EXPORT, useful for internal clients.
  * Mark jack_midi_reset_buffer as deprecated.
  * Add example systemd unit file
  * Signal to systemd when jackd is ready.
  * Set "seq" alsa midi driver to maximum resolution possible.
  * Fix loading internal clients from another internal client.
  * Code cleanup and various fixes. (too many to mention here, see git log for details)

* 1.9.12 (2017-12-13)

  * Fix Windows build issues.
  * Fix build with gcc-7.
  * Show hint when DBus device reservation fails.
  * Add support for internal session files.

* 1.9.11-RC1 (2017-06-13)

  * Various corrections in NetJack2 code. Partial buffers can now be
    transmitted with libjacknet API.
  * Including S24_LE/BE formats to linux ALSA driver.
  * More robust shared memory allocator.
  * Allow autostart of jackd on OSX where device-names can contain spaces.
  * Correct CoreAudio devices aggregation code.
  * Waf and wscripts improvement and update.
  * More flexible RT priority setup on Windows.
  * New JackProxyDriver.
  * Various fixes in JACK MIDI code.
  * Fix return value of SetTimebaseCallback().
  * Correct netmanager latency reporting.
  * Implement new jack_port_rename and JackPortRenameCallback API.
  * For OSX El Capitan support, use of Posix semaphore and move of Frameworks
    in /Library folder.
  * Fix CPU hogging of the midi_thread().
  * Release audio devices when alsa_driver_new fails.
  * String management fix.
  * Correct JackDriver::Open: call to fGraphManager->SetBufferSize has to use
    current fEngineControl->fBufferSize value.
  * Use ARM neon intrinsics for AudioBufferMixdown.
  * Fix Netjack alignment.
  * Various wscript improvements and cleanup.
  * Fix initialization of several class variables.
  * Heap-allocate client matrix in topo sort.
  * Add a toggle command to transport utility, to allow toggling between play
    and stop state.
  * Avoid side effects from parsing of "version" option in jackd.
  * Allow firewire device be selected via -d.
  * Add ARM-NEON acceleration for all non-dithering sample conversion
    functions.
  * Add jack_simdtest utility.
  * Use Linux futex as JackSynchro.
  * Add autoclose option to jack_load.

* 1.9.10 (2014-07-19)

  * More robust code in JackPortAudioDriver to handle buffer size change and
    backend switching.
  * Fix bus error on ARM platforms.
  * Dynamically scan and print backend and internal names in jackd.
  * CoreMIDI driver fixes.
  * Rework NetJack2 code (OPUS codec on OSX, latency management, libjacknet
    code).
  * Correct auto-connect for audioadapter.
  * Add IIO driver.
  * Merge of Nedko no-self-connect branch.
  * Fix freewheel mode.
  * JackServer::SwitchMaster now correctly notify buffer_size and sample_rate
    changes, cleanup/improvements in JackNetDriver.
  * Tim Mayberry : Add support for building with mingw compiler.
  * Merge of Kim Jeong Yeon Android branch.
  * Partial port of metadata API.

* 1.9.9.5 (2012-11-26)

  * Adrian Knoth fix in midiseq.c.
  * Fix library symbols export issue.
  * Cleanup drivers and internals loading code.
  * jackctl_driver_params_parse API moved in public control.h.
  * More general drivers/internals loading model on Windows.
  * Factorize code the server/client request in JackRequestDecoder class.
  * More robust server/client protocol.
  * Implement shutdown for in server clients.
  * Better time-out management in NetJack2.
  * Experimental system port alias use in Windows JackRouter.
  * Improve ShutDown in NetManager.
  * Correct ShutDown in JackInternalClient and JackLibClient.
  * Fix NetJack2 initialisation bug.
  * Add EndTime function (especially for Windows).
  * Rename JackProcessSync in JackPosixProcessSync.
  * A bit more robust JackMessageBuffer implementation (in progress).
  * Check server API callback from notification thread.
  * Use a time-out in notification channel write function.
  * Fix lock management in JackEngine.
  * In control API, UNIX like sigset_t replaced by more abstract
    jackctl_sigmask_t * opaque struct.
  * Improve libjacknet master mode.
  * Remove JACK_32_64 flag, so POST_PACKED_STRUCTURE now always used.
    POST_PACKED_STRUCTURE used for jack_latency_range_t type.
  * Rework JackMessageBuffer. [firewire]

  * Introduce UpdateLatencies() in FFADO backend. [firewire]

  * Allow FFADO backend to change the buffer size.
  * Update waf.
  * New jack_get_cycle_times() implementation from Fons Adriennsen.
  * Align buffers to 32 byte boundaries to allow AVX processing.
  * Extend jack_control to have parameter reset commands.
  * Fix alsa driver parameter order.
  * Control API: Enforce driver/internal parameter order.
  * Fix in ALSA adapter.
  * Devin Anderson patch for Jack/CoreMIDI duplicated messages.
  * Change framework installation hierarchy for OSX Mountain Lion.
  * Update JackCoreAudioDriver and JackCoreAudioAdapter with more recent API.
  * jack_control: fix epr command.
  * Add opus support to NetJack2.
  * More robust channel mapping handling in JackCoreAudioDriver.
  * netjack1/netone opus support.
  * controlapi: fix double free on master switch.
  * Use string ids in the alsa device list.
  * netjack/opus: don't re-init en/decoders.
  * Correct JackPortAudioDriver::Open: special case for ASIO drivers.

* 1.9.8 (2011-12-19)

  * Merge newer-midi branch (Devin Anderson redesign of the MIDI drivers:
    alsarawmidi, ffado, coremidi and winmme).
  * Correction in jackdmp.cpp: notify_server_stop should be done after server
    destruction.
  * Correct driver lifetime management.
  * Add XRun detection in PortAudio driver.
  * CELT code for NetJack2.
  * Merge branch switch-master-port-registration-notifications: correct driver
    port registration.
  * Libjacknet in progress.
  * Correct MIDI in NetJack2.
  * Correct OSX real-time thread setup.
  * Correct rd_acquire in dbus code.
  * Correct NetJack2 connection handling.
  * SaveConnections/RestoreConnections in NetDriver and JackAudioDriver.
  * Special version of jack_attach_shm/jack_release_shm on client side for
    POSIX shared memory, to solve a memory leak issue.
  * Another round of code improvements to handle completely buggy Digidesign
    CoreAudio user-land driver.
  * Special CATCH_CLOSE_EXCEPTION_RETURN to handle Close API calls.
  * Add JACK_NETJACK_PORT and JACK_NETJACK_MULTICAST environment variables for
    NetJack2. NetJack2 now only send data on network only is ports are
    connected both sides.
  * Fix for "starting two instances of same app in parallel does not work"
    bug.
  * Enable explicit channel mapping in CoreAudio driver.
  * New JackTimedDriver class to be used by JackDummyDriver, JackNetDriver and
    JackNetOneDriver classes.
  * More robust code in synchronization primitives and in JackMessageBuffer.
  * More robust Control API implementation. Add jackctl_driver_get_type in
    Control API.
  * Singleton behaviour for JackCoreMidiDriver and JackWinMMEDriver.
  * John Emmas patch for DSP CPU computation.
  * John Emmas Windows server launching patch.
  * Fix jack_set_port_name API.
  * Enable local access in NetJack2 code.
  * Dynamic port management in JACK/CoreMidi bridge.

* 1.9.7 (2011-03-30)

  * Sync JackAlsaDriver::alsa_driver_check_card_type with JACK1 backend.
  * Correct JackServer::Open to avoid a race when control API is used on OSX.
  * Improve backend error handling: fatal error returned by Read/Write now
    cause a Process failure (so a thread exit for blocking backends).
    Recoverable ones (XRuns..) are now treated internally in ALSA, FreeBob and
    FFADO backends.
  * In jackdmp.cpp, jackctl_setup_signals moved before jackctl_server_start.
  * Correct symbols export in backends on OSX. ALSA backend: suspend/resume
    handling.
  * Correct dummy driver.
  * Adrian Knoth jack_lsp patch.
  * Remove JackPortIsActive flag.
  * New latency API implementation.
  * ComputeTotalLatencies now a client/server call.
  * Add latent test client for latency API.
  * Also print playback and capture latency in jack_lsp.
    jack_client_has_session_callback implementation.
  * Check requested buffer size and limit to 1..8192 - avoids weird behaviour
    caused by jack_bufsize foobar.
  * jack_port_type_get_buffer_size implementation.
  * Stop using alloca and allocate buffer on the heap for alsa_io.
  * Rename jdelay to jack_iodelay as per Fons' request.
  * Call buffer size callback in activate (actually this is done on client side
    in the RT thread Init method).
  * Add jack_midi_dump client.
  * Synchronize net JACK1 with JACK1 version.
  * Synchronize jack_connect/jack_disconnect with JACK1 version.
  * Correct JackNetMaster::SetBufferSize.
  * Use jack_default_audio_sample_t instead of float consistently, fix ticket
    #201.
  * -X now allows to add several slave backends, add -I to load several
    internal clients.
  * Rework internal slave driver management, JackServerGlobals now handle same
    parameters as jackdmp.
  * Correct JackEngine::NotifyGraphReorder, update JackDebugClient with latest
    API.
  * Devin Anderson server-ctl-proposal branch merged on trunk: improved control
    API, slave backend reworked. Implement renaming in JackDriver::Open to
    avoid name collision (thanks Devin Anderson).
  * Correct alsa_driver_restart (thanks Devin Anderson). Correction of
    jack_connect/jack_disconnect: use of jack_activate and volatile keyword for
    thread shared variable.
  * Correction of JackNetOneDriver for latest CELT API.
  * Synchronize JackWeakAPI.cpp with new APIs.

* 1.9.6 (2010-08-30)

  * Improve JackCoreAudioDriver and JackCoreAudioAdapter : when no devices are
    described, takes default input and output and aggregate them.
  * Correct JackGraphManager::DeactivatePort.
  * Correct JackMachServerChannel::Execute : keep running even in error cases.
    Raise JACK_PROTOCOL_VERSION number.
  * Arnold Krille firewire patch.
  * Raise JACK_DRIVER_PARAM_STRING_MAX and JACK_PARAM_STRING_MAX to 127
    otherwise some audio drivers cannot be loaded on OSX.
  * Fix some file header to have library side code use LGPL.
  * On Windows, now use TRE library for regexp (BSD license instead of GPL
    license).
  * ffado-portname-sync.patch from ticket #163 applied.
  * Remove call to exit in library code.
  * Make jack_connect/jack_disconnect wait for effective port
    connection/disconnection.
  * Add tests to validate intclient.h API.
  * On Linux, inter-process synchronization primitive switched to POSIX
    semaphore.
  * In JackCoreAudioDriver, move code called in MeasureCallback to be called
    once in IO thread.
  * David Garcia Garzon netone patch.
  * Fix from Fernando Lopez-Lezcano for compilation on fc13.
  * Fix JackPosixSemaphore::TimedWait : same behavior as
    JackPosixSemaphore::Wait regarding EINTR.
  * David Garcia Garzon unused_pkt_buf_field_jack2 netone patch.
  * Arnold Krille firewire snooping patch.
  * Jan Engelhardt patch for get_cycles on SPARC.
  * Adrian Knoth hurd.patch, kfreebsd-fix.patch and alpha_ia64-sigsegv.patch
    from ticket 177.
  * Adrian Knoth fix for linux cycle.h (ticket 188).
  * In JackCoreAudioDriver, fix an issue when no value is given for input.

* 1.9.5 (2010-02-12)

  * Dynamic choice of maximum port number.
  * More robust sample rate change handling code in JackCoreAudioDriver.
  * Devin Anderson patch for Jack FFADO driver issues with lost MIDI bytes
    between periods (and more).
  * Fix port_rename callback: now both old name and new name are given as
    parameters.
  * Special code in JackCoreAudio driver to handle completely buggy Digidesign
    CoreAudio user-land driver.
  * Ensure that client-side message buffer thread calls thread_init callback
    if/when it is set by the client (backport of JACK1 rev 3838).
  * Check dynamic port-max value.
  * Fix JackCoreMidiDriver::ReadProcAux when ring buffer is full (thanks Devin
    Anderson).
  * Josh Green ALSA driver capture only patch.
  * When threads are cancelled, the exception has to be rethrown.
  * Use a QUIT notification to properly quit the server channel, the server
    channel thread can then be 'stopped' instead of 'canceled'.
  * Mario Lang alsa_io time calculation overflow patch. Shared memory manager
    was calling abort in case of fatal error, now return an error in caller.
  * Change JackEngineProfiling and JackAudioAdapterInterface gnuplot scripts
    to output SVG instead of PDF.

* 1.9.4 (2009-11-19)

  * Solaris boomer backend now working in capture or playback only mode.
  * Add a -G parameter in CoreAudio backend (the computation value in RT
    thread expressed as percent of period).
  * Use SNDCTL_DSP_SYNCGROUP/SNDCTL_DSP_SYNCSTART API to synchronize input and
    output in Solaris boomer backend.
  * Big endian bug fix in memops.c.
  * Fix issues in JackNetDriver::DecodeTransportData and
    JackNetDriver::Initialize.
  * Correct CPU timing in JackNetDriver, now take cycle begin time after Read.
  * Simplify transport in NetJack2: master only can control transport.
  * Change CoreAudio notification thread setup for OSX Snow Leopard.
  * Correct server temporary mode: now set a global and quit after
    server/client message handling is finished.
  * Add a string parameter to server ==> client notification, add a new
    JackInfoShutdownCallback type.
  * CoreAudio backend now issue a JackInfoShutdownCallback when an
    unrecoverable error is detected (sampling rate change, stream
    configuration change).
  * Correct jackdmp.cpp (failures case were not correct..).
  * Improve JackCoreAudioDriver code.
  * Raise default port number to 2048.
  * Correct JackProcessSync::LockedTimedWait.
  * Correct JACK_MESSAGE_SIZE value, particularly in OSX RPC code.
  * Now start server channel thread only when backend has been started (so in
    JackServer::Start).
  * Should solve race conditions at start time.
  * jack_verbose moved to JackGlobals class.
  * Improve aggregate device management in JackCoreAudioDriver: now a
    "private" device only and cleanup properly.
  * Aggregate device code added to JackCoreAudioAdapter.
  * Implement "hog mode" (exclusive access of the audio device) in
    JackCoreAudioDriver.
  * Fix jack_set_sample_rate_callback to have he same behavior as in JACK1.
  * Dynamic system version detection in JackCoreAudioDriver to either create
    public or private aggregate device.
  * In JackCoreAudioDriver, force the SR value to the wanted one *before*
    creating aggregate device (otherwise creation will fail).
  * In JackCoreAudioDriver, better cleanup of AD when intermediate open
    failure.
  * In JackCoreAudioDriver::Start, wait for the audio driver to effectively
    start (use the MeasureCallback).
  * In JackCoreAudioDriver, improve management of input/output channels: -1 is
    now used internally to indicate a wanted max value.
  * In JackCoreAudioDriver::OpenAUHAL, correct stream format setup and
    cleanup.
  * Correct crash bug in JackAudioAdapterInterface when not input is used in
    adapter (temporary fix).
  * Sync JackCoreAudioAdapter code on JackCoreAudioDriver one.
  * JACK_SCHED_POLICY switched to SCHED_FIFO.
  * Now can aggregate device that are themselves AD.
  * No reason to make jack_on_shutdown deprecated, so revert the incorrect
    change.
  * Thread AcquireRealTime and DropRealTime were (incorrectly) using fThread
    field.
  * Use pthread_self()) (or GetCurrentThread() on Windows) to get the calling
    thread.
  * Correctly save and restore RT mode state in freewheel mode.
  * Correct freewheel code on client side.
  * Fix AcquireRealTime and DropRealTime: now distinguish when called from
    another thread (AcquireRealTime/DropRealTime) and from the thread itself
    (AcquireSelfRealTime/DropSelfRealTime).
  * Correct JackPosixThread::StartImp: thread priority setting now done in the
    RT case only.
  * Correct JackGraphManager::GetBuffer for the "client loop with one
    connection" case: buffer must be copied.
  * Correct JackInfoShutdownCallback prototype, two new
    JackClientProcessFailure and JackClientZombie JackStatus code.
  * Correct JackCoreAudio driver when empty strings are given as -C, -P or -d
    parameter.
  * Better memory allocation error checking on client (library) side.
  * Better memory allocation error checking in ringbuffer.c, weak import
    improvements.
  * Memory allocation error checking for jack_client_new and jack_client_open
    (server and client side).
  * Memory allocation error checking in server for RPC.
  * Simplify server temporary mode: now use a JackTemporaryException.
  * Lock/Unlock shared memory segments (to test...).
  * Sync with JACK1 : -r parameter now used for no-realtime, realtime (-R) is
    now default, usable backend given vie platform.
  * In JackCoreAudio driver, (possibly) clock drift compensation when needed
    in aggregated devices.
  * In JackCoreAudio driver, clock drift compensation in aggregated devices
    working.
  * In JackCoreAudio driver, clock drift compensation semantic changed a bit:
    when on, does not activate if not needed (same clock domain).
  * Sync JackCoreAudioAdapter code with JackCoreAudioDriver.

* 1.9.3 (2009-07-21)

  * New JackBoomerDriver class for Boomer driver on Solaris.
  * Add mixed 32/64 bits mode (off by default).
  * Native MIDI backend (JackCoreMidiDriver, JackWinMMEDriver).
  * In ALSA audio card reservation code, tries to open the card even if
    reservation fails.
  * Clock source setting on Linux.
  * Add jackctl_server_switch_master API.
  * Fix transport callback (timebase master, sync) issue when used after
    jack_activate (RT thread was not running).
  * D-Bus access for jackctl_server_add_slave/jackctl_server_remove_slave API.
  * Cleanup "loopback" stuff in server.
  * Torben Hohn fix for InitTime and GetMicroSeconds in JackWinTime.c.
  * New jack_free function added in jack.h.
  * Reworked Torben Hohn fix for server restart issue on Windows.
  * Correct jack_set_error_function, jack_set_info_function and
    jack_set_thread_creator functions.
  * Correct JackFifo::TimedWait for EINTR handling.
  * Move DBus based audio device reservation code in ALSA backend compilation.
  * Correct JackTransportEngine::MakeAllLocating, sync callback has to be
    called in this case also.
  * NetJack2 code: better error checkout, method renaming.
  * Tim Bechmann patch: hammerfall, only release monitor thread, if it has
    been created.
  * Tim Bechmann memops.c optimization patches.
  * In combined --dbus and --classic compilation code, use PulseAudio
    acquire/release code.
  * Big rewrite of Solaris boomer driver, seems to work in duplex mode at
    least.
  * Loopback backend reborn as a dynamically loadable separated backend.

* 1.9.2 (2009-02-11)

  * Solaris version.
  * New "profiling" tools.
  * Rework the mutex/signal classes.
  * Support for BIG_ENDIAN machines in NetJack2.
  * D-BUS based device reservation to better coexist with PulseAudio on Linux.
  * Add auto_connect parameter in netmanager and netadapter.
  * Use Torben Hohn PI controler code for adapters.
  * Client incorrect re-naming fixed : now done at socket and fifo level.
  * Virtualize and allow overriding of thread creation function, to allow Wine
    support (from JACK1).

* 1.9.1 (2008-11-14)

  * Fix jackctl_server_unload_internal.
  * Filter SIGPIPE to avoid having client get a SIGPIPE when trying to access
    a died server.
  * Libjack shutdown handler does not "deactivate" (fActive = false) the
    client anymore, so that jack_deactivate correctly does the job later on.
  * Better isolation of server and clients system resources to allow starting
    the server in several user account at the same time.
  * Report ringbuffer.c fixes from JACK1.
  * Client and library global context cleanup in case of incorrect shutdown
    handling (that is applications not correctly closing client after server
    has shutdown).
  * Use JACK_DRIVER_DIR variable in internal clients loader.
  * For ALSA driver, synchronize with latest JACK1 memops functions.
  * Synchronize JACK2 public headers with JACK1 ones.
  * Implement jack_client_real_time_priority and
    jack_client_max_real_time_priority API.
  * Use up to BUFFER_SIZE_MAX frames in midi ports, fix for ticket #117.
  * Cleanup server starting code for clients directly linked with
    libjackserver.so.
  * JackMessageBuffer was using thread "Stop" scheme in destructor, now use
    the safer thread "Kill" way.
  * Synchronize ALSA backend code with JACK1 one.
  * Set default mode to 'slow' in JackNetDriver and JackNetAdapter.
  * Simplify audio packet order verification.
  * Fix JackNetInterface::SetNetBufferSize for socket buffer size computation
    and JackNetMasterInterface::DataRecv if synch packet is received, various
    cleanup.
  * Better recovery of network overload situations, now "resynchronize" by
    skipping cycles.".
  * Support for BIG_ENDIAN machines in NetJack2.
  * Support for BIG_ENDIAN machines in NetJack2 for MIDI ports.
  * Support for "-h" option in internal clients to print the parameters.
  * In NetJack2, fix a bug when capture or playback only channels are used.
  * Add a JACK_INTERNAL_DIR environment variable to be used for internal
    clients.
  * Add a resample quality parameter in audioadapter.
  * Now correctly return an error if JackServer::SetBufferSize could not
    change the buffer size (and was just restoring the current one).
  * Use PRIu32 kind of macro in JackAlsaDriver again.
  * Add a resample quality parameter in netadapter.

* 1.9.0 (2008-03-18)

  * Waf based build system: Nedko Arnaudov, Grame for preliminary OSX support.
  * Control API, dbus based server control access: Nedko Arnaudov, Grame.
  * NetJack2 components (in progress): jack_net backend, netmanager,
    audioadapter, netadapter : Romain Moret, Grame.
  * Code restructuring to help port on other architectures: Michael Voigt.
  * Code cleanup/optimization: Tim Blechmann.
  * Improve handling of server internal clients that can now be
    loaded/unloaded using the new server control API: Grame.
  * A lot of bug fix and improvements.

* 0.72 (2008-04-10)

* 0.71 (2008-02-14)

  * Add port register/unregister notification in JackAlsaDriver.
  * Correct JACK_port_unregister in MIDI backend.
  * Add TimeCallback in JackDebugClient class.
  * Correct jack_get_time propotype.
  * Correct JackSocketClientChannel::ClientClose to use ServerSyncCall instead
    of ServerAsyncCall.
  * Better documentation in jack.h. libjackdmp.so renamed to
    libjackservermp.so and same for OSX framework.
  * Define an internal jack_client_open_aux needed for library wrapper feature.
  * Remove unneeded jack_port_connect API.
  * Correct jack_port_get_connections function (should return NULL when no
    connections).
  * In thread model, execute a dummy cycle to be sure thread has the correct
    properties (ensure thread creation is finished).
  * Fix engine real-time notification (was broken since ??).
  * Implements wrapper layer.
  * Correct jack_port_get_total_latency.
  * Correct all backend playback port latency in case of "asynchronous" mode
    (1 buffer more).
  * Add test for jack_cycle_wait, jack_cycle_wait and jack_set_process_thread
    API.
  * RT scheduling for OSX thread (when used in dummy driver).
  * Add -L (extra output latency in aynchronous mode) in CoreAudio driver.
  * New JackLockedEngine decorator class to serialize access from ALSA Midi
    thread, command thread and in-server clients.
  * Use engine in JackAlsaDriver::port_register and
    JackAlsaDriver::port_unregister.
  * Fix connect notification to deliver *one* notification only.
  * Correct JackClient::Activate so that first kGraphOrderCallback can be
    received by the client notification thread.
  * New jack_server_control client to test notifications when linked to the
    server library.
  * Synchronise transport.h with latest jackd version (Video handling).
  * Transport timebase fix.
  * Dmitry Baikov patch for alsa_rawmidi driver.
  * Pieter Palmers patch for FFADO driver.
  * Add an Init method for blocking drivers to be decorated using
    JackThreadedDriver class.
  * Correct PortRegister, port name checking must be done on server side.
  * Correct a missing parameter in the usage message of jack_midiseq.
  * New SetNonBlocking method for JackSocket.
  * Correct a dirty port array issue in JackGraphManager::GetPortsAux.

* 0.70 (2008-01-24)

  * Updated API to match jack 0.109.0 version.
  * Update in usx2y.c and JackPort.cpp to match jackd 0.109.2.
  * Latest jack_lsp code from jack SVN.
  * Add jack_mp_thread_wait client example.
  * Add jack_thread_wait client example.
  * Remove checking thread in CoreAudio driver, better device state change
    recovery strategy: the driver is stopped and restarted.
  * Move transport related methods from JackEngine to JackServer.


  * Tim Blechmann sse optimization patch for JackaudioPort::MixAudioBuffer,
    use of Apple Accelerate framework on OSX.
  * Remove use of assert in JackFifo, JackMachSemaphore, and
    JackPosixSemaphore: print an error instead.
  * Correct "server_connect": close the communication channel.
  * More robust external API.
  * Use SetAlias for port naming.
  * Use jackd midi port naming scheme.
  * Notify ports unregistration in JackEngine::ClientCloseAux.
  * Fix in JackClient::Error(): when RT thread is failing and calling
    Shutdown, Shutdown was not desactivating the client correctly.

* 0.69

  * On OSX, use CFNotificationCenterPostNotificationWithOptions with
    kCFNotificationDeliverImmediately | kCFNotificationPostToAllSessions for
    server ==> JackRouter plugin notification.
  * On OSX, use jack server name in notification system.
  * Correct fPeriodUsecs computation in JackAudioDriver::SetBufferSize and
    JackAudioDriver::SetSampleRate.
  * Correct JackMachNotifyChannel::ClientNotify.
  * Correct bug in CoreAudio driver sample rate management.
  * Add a sample_rate change listener in CoreAudio driver.
  * Correct sample_rate management in JackCoreAudioDriver::Open.
  * Better handling in sample_rate change listener.
  * Pieter Palmers FFADO driver and scons based build.
  * Pieter Palmers second new build system: scons and Makefile based build.
  * Tim Blechmann scons patch.
  * Change string management for proper compilation with gcc 4.2.2.
  * JackLog cleanup.
  * Cleanup in CoreAudio driver.
  * Tim Blechmann patch for JackGraphManager::GetPortsAux memory leak, Tim
    Blechmann patch for scons install.
  * Dmitry Baikov MIDI patch: alsa_seqmidi and alsa_rammidi drivers.
  * CoreAudio driver improvement: detect and notify abnormal situations
    (stopped driver in case of SR change...).

* 0.68 (2007-10-16)

  * Internal loadable client implementation, winpipe version added.
  * Reorganize jack headers.
  * Improve Linux install/remove scripts.
  * Use LIB_DIR variable for 64 bits related compilation (drivers location).
  * More generic Linux script.
  * Correct jack_acquire_real_time_scheduling on OSX.
  * Merge of Dmitry Baikov MIDI branch.
  * Correct JackGraphManager::GetPortsAux to use port type.
  * Remove JackEngineTiming class: code moved in JackEngineControl.
  * Add midiseq and midisine examples.
  * Cleanup old zombification code.
  * Linux Makefile now install jack headers.
  * Use of JACK_CLIENT_DEBUG environment variable to activate debug client
    mode.
  * Definition of JACK_LOCATION variable using -D in the Makefile.
  * Restore jack 0.103.0 MIDI API version.
  * Fix a bug in freewheel management in async mode: drivers now receive the
    kStartFreewheelCallback and kStopFreewheelCallback notifications.
  * Server and user directory related code moved in a JackTools file.
  * Client name rewriting to remove path characters (used in fifo naming).
  * Correct ALSA driver Attach method: internal driver may have changed the
    buffer_size and sample_rate values.
  * Add JackWinSemaphore class.
  * Add an implementation for obsolete jack_internal_client_new and
    jack_internal_client_close.
  * Add missing jack_port_type_size.
  * Use of JackWinSemaphore instead of JackWinEvent for inter-process
    synchronization.
  * Correct types.h for use with MINGW on Windows.
  * Move OSX start/stop notification mechanism in Jackdmp.cpp.
  * Correct CheckPort in JackAPI.cpp.

* 0.67 (2007-09-28)

  * Correct jack_client_open "status" management.
  * Rename server_name from "default" to "jackdmp_default" to avoid conflict
    with regular jackd server.
  * Fix a resource leak issue in JackCoreAudioDriver::Close().
  * Better implement "jack_client_open" when linking a client with the server
    library.
  * Correct "jack_register_server" in shm.c.
  * Add missing timestamps.c and timestamps.h files.
  * Correctly export public headers in OSX frameworks.
  * Suppress JackEngine::ClientInternalCloseIm method.
  * Use .jackdrc file (instead of .jackdmprc).
  * Install script now creates a link "jackd ==> jackdmp" so that automatic
    launch can work correctly.
  * Paul Davis patch for -r (--replace-registry) feature.
  * Internal loadable client implementation.
  * Fix JackEngine::Close() method.
  * Windows JackRouter.dll version 0.17: 32 integer sample format.

* 0.66 (2007-09-06)

  * Internal cleanup.
  * Windows JackRouter.dll version 0.16: use of "jack_client_open" API to
    allow automatic client renaming, better Windows VISTA support, new
    JackRouter.ini file.

* 0.65 (2007-08-30)

  * Fix backend port alias management (renaming in system:xxx).
  * Fix a bug in JackLibClient::Open introduced when adding automatic client
    renaming.
  * Fix a bug in jack_test.
  * Correct JackShmMem destructor.
  * Correct end case in JackClient::Execute.
  * Correct JackMachSemaphore::Disconnect.
  * Implement server temporary (-T) mode.
  * Make "Rename" a method of JackPort class, call it from driver Attach
    method.
  * Server/library protocol checking implementation.

* 0.64 (2007-07-26)

  * Checking in the server to avoid calling the clients if no callback are
    registered.
  * Correct deprecated jack_set_sample_rate_callback to return 0 instead of
    -1.
  * Dmitry Baikov buffer size patch.
  * Correct notification for kActivateClient event. Correct
    JackEngine::ClientCloseAux (when called from
    JackEngine::ClientExternalOpen).
  * Correct JackWinEvent::Allocate.
  * Automatic client renaming.
  * Add "systemic" latencies management in CoreAudio driver.
  * Automatic server launch.
  * Removes unneeded 'volatile' for JackTransportEngine::fWriteCounter.

* 0.63 (2007-04-05)

  * Correct back JackAlsaDriver::Read method.
  * Dmitry Baikov patch for JackGraphManager.cpp. Merge JackGraphManager Remove
    and Release method in a unique Release method.
  * Dmitry Baikov jackmp-time patch : add jack_get_time, jack_time_to_frames,
    jack_frames_to_time. Add missing -D__SMP__in OSX project.  Add new
    jack_port_set_alias, jack_port_unset_alias and jack_port_get_aliases API.
  * Steven Chamberlain patch to fix jack_port_by_id export.
  * Steven Chamberlain patch to fix jack_port_type. Test for jack_port_type
    behaviour in jack_test.cpp tool. Add jack_set_client_registration_callback
    API. Add "callback exiting" and "jack_frame_time" tests in jack_test.

* 0.62 (2007-02-16)

  * More client debug code: check if the client is still valid in every
    JackDebugClient method, check if the library context is still valid in
    every API call.
  * Uses a time out value of 10 sec in freewheel mode (like jack).
  * More robust activation/deactivation code, especially in case of client
    crash.
  * New LockAllMemory and UnlockAllMemory functions.
  * Use pthread_attr_setstacksize in JackPosixThread class.
  * Add Pieter Palmers FreeBob driver.
  * Thibault LeMeur ALSA driver patch.
  * Thom Johansen fix for port buffer alignment issues.
  * Better error checking in PortAudio driver.

* 0.61 (2006-12-18)

  * Tom Szilagyi memory leak fix in ringbuffer.c.
  * Move client refnum management in JackEngine.
  * Shared_ports renamed to shared_graph.
  * Add call to the init callback (set up using the
    jack_set_thread_init_callback API) in Real-Time and Notification threads.
  * Define a new 'kActivateClient' notification.
  * New server/client data transfer model to fix a 64 bits system bug.
  * Fix a device name reversal bug in ALSA driver.
  * Implement thread.h API.

* 0.60 (2006-11-23)

  * Improve audio driver synchronous code to better handle possible time-out
    cases.
  * Correct JackWinEnvent::Allocate (handle the ERROR_ALREADY_EXISTS case).
  * Correct JackEngine::ClientExternalNew.

* 0.59 (2006-09-22)

  * Various fixes in Windows version.
  * Signal handling in the Windows server.
  * Improved JackRouter ASIO/Jack bridge on Windows.
  * Rename global "verbose" in "jack_verbose" to avoid symbol clash with
    PureData.
  * Add a new cpu testing/loading client.
  * Correct server SetBufferSize in case of failure.
  * Correct PortAudio driver help.
  * Use -D to setup ADDON_DIR on OSX and Linux.
  * Synchronize ALSA backend with jack one.

* 0.58 (2006-09-06)

  * Correct a bug introduced in 0.55 version that was preventing coreaudio
    audio inputs to work.
  * Restructured code structure after import on svn.

* 0.57

  * Correct bug in Mutex code in JackClientPipeThread::HandleRequest.
  * ASIO JackRouter driver supports more applications.
  * Updated HTML documentation.
  * Windows dll binaries are compiled in "release" mode.

* 0.56

  * Correct SetBufferSize in coreaudio driver, portaudio driver and
    JackServer.
  * Real-time notifications for Windows version.
  * In the PortAudio backend, display more informations for installed WinMME,

  * DirectSound and ASIO drivers.

* 0.55

  * Windows version.
  * Correct management of monitor ports in ALSA driver.
  * Engine code cleanup.
  * Apply Rui patch for more consistent parameter naming in coreaudio driver.
  * Correct JackProcessSync::TimedWait: time-out was not computed correctly.
  * Check the return code of NotifyAddClient in JackEngine. 

* 0.54

  * Use the latest shm implementation that solve the uncleaned shm segment
    problem on OSX.
  * Close still opened file descriptors (report from Giso Grimm). Updated html
    documentation.

* 0.53

  * Correct JackPilotMP tool on OSX.
  * Correct CoreAudio driver for half duplex cases.
  * Fix a bug in transport for "unactivated" clients.
  * Fix a bug when removing "unactivated" clients from the server. Tested on
    Linux/PPC.

* 0.52

  * Universal version for Mac Intel and PPC.
  * Improvement of CoreAudio driver for half duplex cases.

* 0.51

  * Correct bugs in transport API implementation.

* 0.50

  * Transport API implementation.

* 0.49

  * Internal connection manager code cleanup.

* 0.48

  * Finish software monitoring implementation for ALSA and CoreAudio drivers.
  * Simpler shared library management on OSX.

* 0.47

  * More fix for 64 bits compilation.
  * Correct ALSA driver.
  * Create a specific folder for jackdmp drivers.
  * Use /dev/shm as default for fifo and sockets.
  * "Install" and "Remove" script for smoother use with regular jack.

* 0.46

  * Fix a bug in loop management.
  * Fix a bug in driver loading/unloading code.
  * Internal code cleanup for better 64 bits architecture support.
  * Compilation on OSX/Intel.
  * Add the -d option for coreaudio driver (display CoreAudio devices internal
    name).

* 0.45

  * Script to remove the OSX binary stuff.
  * Correct an export symbol issue that was preventing QjackCtl to work on OSX.
  * Fix the consequences of the asynchronous semantic of
    connections/disconnections.

* 0.44

  * Patch from Dmitry Daikov: use clock_gettime by default for timing.
  * Correct dirty buffer issue in CoreAudio driver. Updated doc.

* 0.43

  * Correct freewheel mode.
  * Optimize ALSA and coreaudio drivers.
  * Correct OSX installation script.

* 0.42

  * Patch from Nick Mainsbridge.
  * Correct default mode for ALSA driver.
  * Correct XCode project.

* 0.41

  * Add the ALSA MMAP_COMPLEX support for ALSA driver.
  * Patch from Dmitry Daikov: compilation option to choose between
    "get_cycles" and "gettimeofday" to measure timing.

* 0.4

  * Linux version, code cleanup, new -L parameter to activate the loopback
    driver (see Documentation), a number of loopback ports can be defined.
    Client validation tool.

* 0.31

  * Correct bug in mixing code that caused Ardour + jackdmp to crash...

* 0.3

  * Implement client zombification + correct feedback loop management + code
    cleanup.

* 0.2

  * Implements jack_time_frame, new -S (sync) mode: when "synch" mode is
    activated, the jackdmp server waits for the graph to be finished in the
    current cycle before writing the output buffers. Note: To experiment with
    the -S option, jackdmp must be launched in a console.

* 0.1

  * First published version

