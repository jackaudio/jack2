/*
 Copyright (C) 2006-2011 Grame

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _asiosmpl_
#define _asiosmpl_

#include "asiosys.h"

// Globals
static int	kBlockFrames = 256;
static int	kNumInputs = 4;
static int	kNumOutputs = 4;

#if WINDOWS

#include "jack.h"
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include <windows.h>
#include "ole2.h"

#endif

#include "combase.h"
#include "iasiodrv.h"

#define PATH_SEP "\\"

#include <list>
#include <string>

class JackRouter : public IASIO, public CUnknown
{
public:
	JackRouter(LPUNKNOWN pUnk, HRESULT *phr);
	~JackRouter();

	DECLARE_IUNKNOWN
    //STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {      \
    //    return GetOwner()->QueryInterface(riid,ppv);            \
    //};                                                          \
    //STDMETHODIMP_(ULONG) AddRef() {                             \
    //    return GetOwner()->AddRef();                            \
    //};                                                          \
    //STDMETHODIMP_(ULONG) Release() {                            \
    //    return GetOwner()->Release();                           \
    //};

	// Factory method
	static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid,void **ppvObject);
#else

#include "asiodrvr.h"

//---------------------------------------------------------------------------------------------
class JackRouter : public AsioDriver
{
public:
	JackRouter();
	~JackRouter();
#endif

	static int processCallback(jack_nframes_t nframes, void* arg);
    static void connectCallback(jack_port_id_t a, jack_port_id_t b, int connect, void* arg);
	static void shutdownCallback(void* arg);

	ASIOBool init(void* sysRef);
	void getDriverName(char *name);		// max 32 bytes incl. terminating zero
	long getDriverVersion();
	void getErrorMessage(char *string);	// max 128 bytes incl.

	ASIOError start();
	ASIOError stop();

	ASIOError getChannels(long *numInputChannels, long *numOutputChannels);
	ASIOError getLatencies(long *inputLatency, long *outputLatency);
	ASIOError getBufferSize(long *minSize, long *maxSize,
		long *preferredSize, long *granularity);

	ASIOError canSampleRate(ASIOSampleRate sampleRate);
	ASIOError getSampleRate(ASIOSampleRate *sampleRate);
	ASIOError setSampleRate(ASIOSampleRate sampleRate);
	ASIOError getClockSources(ASIOClockSource *clocks, long *numSources);
	ASIOError setClockSource(long index);

	ASIOError getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp);
	ASIOError getChannelInfo(ASIOChannelInfo *info);

	ASIOError createBuffers(ASIOBufferInfo *bufferInfos, long numChannels,
		long bufferSize, ASIOCallbacks *callbacks);
	ASIOError disposeBuffers();

	ASIOError controlPanel();
	ASIOError future(long selector, void *opt);
	ASIOError outputReady();

	void bufferSwitch();
	long getMilliSeconds() {return fMilliSeconds;}

	static std::list<std::pair<std::string, std::string> > fConnections;  // Connections list

private:

	void bufferSwitchX();

	double fSamplePosition;
	ASIOCallbacks* fCallbacks;
	ASIOTime fAsioTime;
	ASIOTimeStamp fTheSystemTime;

	void** fInputBuffers;
	void** fOutputBuffers;

	long* fInMap;
	long* fOutMap;

	long fInputLatency;
	long fOutputLatency;
	long fActiveInputs;
	long fActiveOutputs;
	long fToggle;
	long fMilliSeconds;
	bool fRunning;
	bool fFirstActivate;
    bool fFloatSample;
    bool fAliasSystem;
	bool fTimeInfoMode, fTcRead;
	char fErrorMessage[128];

	bool fAutoConnectIn;
	bool fAutoConnectOut;

	// Jack part
	jack_client_t* fClient;
	jack_port_t** fInputPorts;
	jack_port_t** fOutputPorts;
	long fBufferSize;
	ASIOSampleRate fSampleRate;

	void autoConnect();
	void saveConnections();
    void restoreConnections();
    
    void processInputs();
    void processOutputs();

};

#endif

