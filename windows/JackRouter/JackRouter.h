/*
Copyright (C) 2006 Grame  

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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

#define MAX_PORTS 32

#define LONG_SAMPLE 1

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

	static int process(jack_nframes_t nframes, void* arg);
	static void shutdown(void* arg);

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

	static bool fFirstActivate;
	static std::list<std::pair<std::string, std::string> > fConnections;  // Connections list

private:

	void bufferSwitchX();

	double fSamplePosition;
	ASIOCallbacks* fCallbacks;
	ASIOTime fAsioTime;
	ASIOTimeStamp fTheSystemTime;

#ifdef LONG_SAMPLE
	long* fInputBuffers[MAX_PORTS * 2];
	long* fOutputBuffers[MAX_PORTS * 2];
#else
	float* fInputBuffers[MAX_PORTS * 2];
	float* fOutputBuffers[MAX_PORTS * 2];
#endif
	long fInMap[MAX_PORTS];
	long fOutMap[MAX_PORTS];

	long fInputLatency;
	long fOutputLatency;
	long fActiveInputs;
	long fActiveOutputs;
	long fToggle;
	long fMilliSeconds;
	bool fActive, fStarted;
	bool fTimeInfoMode, fTcRead;
	char fErrorMessage[128];

	bool fAutoConnectIn;
	bool fAutoConnectOut;

	// Jack part
	jack_client_t* fClient;
	jack_port_t* fInputPorts[MAX_PORTS];
	jack_port_t* fOutputPorts[MAX_PORTS];
	long fBufferSize;
	ASIOSampleRate fSampleRate;

	void AutoConnect();
	void SaveConnections();
    void RestoreConnections();

};

#endif

