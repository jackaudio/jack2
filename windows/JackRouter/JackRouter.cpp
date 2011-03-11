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

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <process.h>
#include "JackRouter.h"
#include "profport.h"

/*

	08/07/2007 SL : USe jack_client_open instead of jack_client_new (automatic client renaming).
	09/08/2007 SL : Add JackRouter.ini parameter file.
	09/20/2007 SL : Better error report in DllRegisterServer (for Vista).
	09/27/2007 SL : Add AUDO_CONNECT property in JackRouter.ini file.
	10/10/2007 SL : Use ASIOSTInt32LSB instead of ASIOSTInt16LSB.

 */

//------------------------------------------------------------------------------------------
// extern
void getNanoSeconds(ASIOTimeStamp *time);

// local
double AsioSamples2double (ASIOSamples* samples);

static const double twoRaisedTo32 = 4294967296.;
static const double twoRaisedTo32Reciprocal = 1. / twoRaisedTo32;

//------------------------------------------------------------------------------------------
// on windows, we do the COM stuff.

#if WINDOWS
#include "windows.h"
#include "mmsystem.h"
#include "psapi.h"

using namespace std;

// class id.
// {838FE50A-C1AB-4b77-B9B6-0A40788B53F3}
CLSID IID_ASIO_DRIVER = { 0x838fe50a, 0xc1ab, 0x4b77, { 0xb9, 0xb6, 0xa, 0x40, 0x78, 0x8b, 0x53, 0xf3 } };


CFactoryTemplate g_Templates[1] = {
    {L"ASIOJACK", &IID_ASIO_DRIVER, JackRouter::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

CUnknown* JackRouter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	return (CUnknown*)new JackRouter(pUnk,phr);
};

STDMETHODIMP JackRouter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	if (riid == IID_ASIO_DRIVER) {
		return GetInterface(this, ppv);
	}
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//		Register ASIO Driver
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
extern LONG RegisterAsioDriver(CLSID,char *,char *,char *,char *);
extern LONG UnregisterAsioDriver(CLSID,char *,char *);

//
// Server registration, called on REGSVR32.EXE "the dllname.dll"
//
HRESULT _stdcall DllRegisterServer()
{
	LONG	rc;
	char	errstr[128];

	rc = RegisterAsioDriver (IID_ASIO_DRIVER,"JackRouter.dll","JackRouter","JackRouter","Apartment");

	if (rc) {
		memset(errstr,0,128);
		sprintf(errstr,"Register Server failed ! (%d)",rc);
		MessageBox(0,(LPCTSTR)errstr,(LPCTSTR)"JackRouter",MB_OK);
		return -1;
	}

	return S_OK;
}

//
// Server unregistration
//
HRESULT _stdcall DllUnregisterServer()
{
	LONG	rc;
	char	errstr[128];

	rc = UnregisterAsioDriver (IID_ASIO_DRIVER,"JackRouter.dll","JackRouter");

	if (rc) {
		memset(errstr,0,128);
		sprintf(errstr,"Unregister Server failed ! (%d)",rc);
		MessageBox(0,(LPCTSTR)errstr,(LPCTSTR)"JackRouter",MB_OK);
		return -1;
	}

	return S_OK;
}

// Globals

list<pair<string, string> > JackRouter::fConnections;
bool JackRouter::fFirstActivate = true;

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
JackRouter::JackRouter (LPUNKNOWN pUnk, HRESULT *phr)
	: CUnknown("ASIOJACK", pUnk, phr)

//------------------------------------------------------------------------------------------

#else

// when not on windows, we derive from AsioDriver
JackRouter::JackRouter() : AsioDriver()

#endif
{
	long i;

	fSamplePosition = 0;
	fActive = false;
	fStarted = false;
	fTimeInfoMode = false;
	fTcRead = false;
	fClient = NULL;
	fAutoConnectIn = true;
	fAutoConnectOut = true;

	for (i = 0; i < kNumInputs; i++) {
		fInputBuffers[i] = 0;
		fInputPorts[i] = 0;
		fInMap[i] = 0;
	}
	for (i = 0; i < kNumOutputs; i++) {
		fOutputBuffers[i] = 0;
		fOutputPorts[i] = 0;
		fOutMap[i] = 0;
	}
	fCallbacks = 0;
	fActiveInputs = fActiveOutputs = 0;
	fToggle = 0;
	fBufferSize = 512;
	fSampleRate = 44100;
	printf("Constructor\n");

	// Use "jackrouter.ini" parameters if available
	HMODULE handle = LoadLibrary("JackRouter.dll");

	if (handle) {

		// Get JackRouter.dll path
		char dllName[512];
		string confPath;
		DWORD res = GetModuleFileName(handle, dllName, 512);

		// Compute .ini file path
		string fullPath = dllName;
		int lastPos = fullPath.find_last_of(PATH_SEP);
		string  dllFolder =  fullPath.substr(0, lastPos);
		confPath = dllFolder + PATH_SEP + "JackRouter.ini";

		// Get parameters
		kNumInputs = get_private_profile_int("IO", "input", 2, confPath.c_str());
		kNumOutputs = get_private_profile_int("IO", "output", 2, confPath.c_str());

		fAutoConnectIn = get_private_profile_int("AUTO_CONNECT", "input", 1, confPath.c_str());
		fAutoConnectOut = get_private_profile_int("AUTO_CONNECT", "output", 1, confPath.c_str());

		FreeLibrary(handle);

	} else {
		printf("LoadLibrary error\n");
	}
}

//------------------------------------------------------------------------------------------
JackRouter::~JackRouter()
{
	stop ();
	disposeBuffers ();
	printf("Destructor\n");
	jack_client_close(fClient);
}

//------------------------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "psapi.h"

static bool GetEXEName(DWORD dwProcessID, char* name)
{
    DWORD aProcesses [1024], cbNeeded, cProcesses;
    unsigned int i;

    // Enumerate all processes
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return false;

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    TCHAR szEXEName[MAX_PATH];
    // Loop through all process to find the one that matches
    // the one we are looking for

    for (i = 0; i < cProcesses; i++) {
        if (aProcesses [i] == dwProcessID) {
            // Get a handle to the process
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                              PROCESS_VM_READ, FALSE, dwProcessID);

            // Get the process name
            if (NULL != hProcess) {
                HMODULE hMod;
                DWORD cbNeeded;

                if(EnumProcessModules(hProcess, &hMod,
                                      sizeof(hMod), &cbNeeded)) {
                    //Get the name of the exe file
                    GetModuleBaseName(hProcess, hMod, szEXEName,
                        sizeof(szEXEName)/sizeof(TCHAR));
					int len = strlen((char*)szEXEName) - 4; // remove ".exe"
					strncpy(name, (char*)szEXEName, len);
					name[len] = '\0';
					return true;
                 }
            }
        }
    }

    return false;
}

 //------------------------------------------------------------------------------------------
static inline jack_default_audio_sample_t ClipFloat(jack_default_audio_sample_t sample)
{
     return (sample < jack_default_audio_sample_t(-1.0)) ? jack_default_audio_sample_t(-1.0) : (sample > jack_default_audio_sample_t(1.0)) ? jack_default_audio_sample_t(1.0) : sample;
}

//------------------------------------------------------------------------------------------
void JackRouter::shutdown(void* arg)
{
	JackRouter* driver = (JackRouter*)arg;
	/*
	//exit(1);
	char errstr[128];

	memset(errstr,0,128);
	sprintf(errstr,"JACK server has quitted");
	MessageBox(0,(LPCTSTR)errstr,(LPCTSTR)"JackRouter",MB_OK);
	*/
}

//------------------------------------------------------------------------------------------
int JackRouter::process(jack_nframes_t nframes, void* arg)
{
	JackRouter* driver = (JackRouter*)arg;
	int i,j;
	int pos = (driver->fToggle) ? 0 : driver->fBufferSize ;

	for (i = 0; i < driver->fActiveInputs; i++) {

#ifdef LONG_SAMPLE
		jack_default_audio_sample_t* buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(driver->fInputPorts[i], nframes);
		long* in = driver->fInputBuffers[i] + pos;
		for (j = 0; j < nframes; j++) {
			in[j] = buffer[j] * jack_default_audio_sample_t(0x7fffffff);
		}
#else
		memcpy(driver->fInputBuffers[i] + pos,
				jack_port_get_buffer(driver->fInputPorts[i], nframes),
				nframes * sizeof(jack_default_audio_sample_t));
#endif

	}

	driver->bufferSwitch();

	for (i = 0; i < driver->fActiveOutputs; i++) {

#ifdef LONG_SAMPLE
		jack_default_audio_sample_t* buffer = (jack_default_audio_sample_t*)jack_port_get_buffer(driver->fOutputPorts[i], nframes);
		long* out = driver->fOutputBuffers[i] + pos;
		jack_default_audio_sample_t gain = jack_default_audio_sample_t(1)/jack_default_audio_sample_t(0x7fffffff);
		for (j = 0; j < nframes; j++) {
			buffer[j] = out[j] * gain;
		}
#else
		memcpy(jack_port_get_buffer(driver->fOutputPorts[i], nframes),
				driver->fOutputBuffers[i] + pos,
				nframes * sizeof(jack_default_audio_sample_t));
#endif
	}

	return 0;
}

//------------------------------------------------------------------------------------------
void JackRouter::getDriverName(char *name)
{
	strcpy (name, "JackRouter");
}

//------------------------------------------------------------------------------------------
long JackRouter::getDriverVersion()
{
	return 0x00000001L;
}

//------------------------------------------------------------------------------------------
void JackRouter::getErrorMessage(char *string)
{
	strcpy (string, fErrorMessage);
}

//------------------------------------------------------------------------------------------
ASIOBool JackRouter::init(void* sysRef)
{
	char name[MAX_PATH];
	sysRef = sysRef;

	if (fActive)
		return true;

	HANDLE win = (HANDLE)sysRef;
	int	my_pid = _getpid();

	if (!GetEXEName(my_pid, name)) { // If getting the .exe name fails, takes a generic one.
		_snprintf(name, sizeof(name) - 1, "JackRouter_%d", my_pid);
	}

	if (fClient) {
		printf("Error: jack client still present...\n");
		return true;
	}

	fClient = jack_client_open(name, JackNullOption, NULL);
	if (fClient == NULL) {
		strcpy (fErrorMessage, "Open error: is jack server running?");
		printf("Open error: is jack server running?\n");
		return false;
	}

	fBufferSize = jack_get_buffer_size(fClient);
	fSampleRate = jack_get_sample_rate(fClient);
	jack_set_process_callback(fClient, process, this);
	jack_on_shutdown(fClient, shutdown, this);

	fInputLatency = fBufferSize;		// typically
	fOutputLatency = fBufferSize * 2;
	fMilliSeconds = (long)((double)(fBufferSize * 1000) / fSampleRate);

	// Typically fBufferSize * 2; try to get 1 by offering direct buffer
	// access, and using asioPostOutput for lower latency

	printf("Init ASIO Jack\n");
	fActive = true;
	return true;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::start()
{
	if (fCallbacks) {
		fSamplePosition = 0;
		fTheSystemTime.lo = fTheSystemTime.hi = 0;
		fToggle = 0;
		fStarted = true;
		printf("Start ASIO Jack\n");

		if (jack_activate(fClient) == 0) {

			if (fFirstActivate) {
				AutoConnect();
				fFirstActivate = false;
			} else {
				RestoreConnections();
			}

			return ASE_OK;

		} else {
			return ASE_NotPresent;
		}
	}

	return ASE_NotPresent;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::stop()
{
	fStarted = false;
	printf("Stop ASIO Jack\n");
	SaveConnections();
	jack_deactivate(fClient);
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getChannels(long *numInputChannels, long *numOutputChannels)
{
	*numInputChannels = kNumInputs;
	*numOutputChannels = kNumOutputs;
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getLatencies(long *_inputLatency, long *_outputLatency)
{
	*_inputLatency = fInputLatency;
	*_outputLatency = fOutputLatency;
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getBufferSize(long *minSize, long *maxSize, long *preferredSize, long *granularity)
{
	*minSize = *maxSize = *preferredSize = fBufferSize;		// allow this size only
	*granularity = 0;
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::canSampleRate(ASIOSampleRate sampleRate)
{
	return (sampleRate == fSampleRate) ? ASE_OK : ASE_NoClock;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getSampleRate(ASIOSampleRate *sampleRate)
{
	*sampleRate = fSampleRate;
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::setSampleRate(ASIOSampleRate sampleRate)
{
	return (sampleRate == fSampleRate) ? ASE_OK : ASE_NoClock;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getClockSources(ASIOClockSource *clocks, long *numSources)
{
	// Internal
	if (clocks && numSources) {
		clocks->index = 0;
		clocks->associatedChannel = -1;
		clocks->associatedGroup = -1;
		clocks->isCurrentSource = ASIOTrue;
		strcpy(clocks->name, "Internal");
		*numSources = 1;
		return ASE_OK;
	} else {
		return ASE_InvalidParameter;
	}
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::setClockSource(long index)
{
	if (!index) {
		fAsioTime.timeInfo.flags |= kClockSourceChanged;
		return ASE_OK;
	} else {
		return ASE_NotPresent;
	}
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp)
{
	tStamp->lo = fTheSystemTime.lo;
	tStamp->hi = fTheSystemTime.hi;

	if (fSamplePosition >= twoRaisedTo32) {
		sPos->hi = (unsigned long)(fSamplePosition * twoRaisedTo32Reciprocal);
		sPos->lo = (unsigned long)(fSamplePosition - (sPos->hi * twoRaisedTo32));
	} else {
		sPos->hi = 0;
		sPos->lo = (unsigned long)fSamplePosition;
	}
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::getChannelInfo(ASIOChannelInfo *info)
{
	if (info->channel < 0 || (info->isInput ? info->channel >= kNumInputs : info->channel >= kNumOutputs))
		return ASE_InvalidParameter;
#ifdef LONG_SAMPLE
	info->type = ASIOSTInt32LSB;
#else
	info->type = ASIOSTFloat32LSB;
#endif

	info->channelGroup = 0;
	info->isActive = ASIOFalse;
	long i;
	char buf[32];

	if (info->isInput) {
		for (i = 0; i < fActiveInputs; i++) {
			if (fInMap[i] == info->channel) {
				info->isActive = ASIOTrue;
				//_snprintf(buf, sizeof(buf) - 1, "Jack::In%d ", info->channel);
				//strcpy(info->name, buf);
				//strcpy(info->name, jack_port_name(fInputPorts[i]));
				break;
			}
		}
		_snprintf(buf, sizeof(buf) - 1, "In%d ", info->channel);
		strcpy(info->name, buf);
	} else {
		for (i = 0; i < fActiveOutputs; i++) {
			if (fOutMap[i] == info->channel) {  //NOT USED !!
				info->isActive = ASIOTrue;
				//_snprintf(buf, sizeof(buf) - 1, "Jack::Out%d ", info->channel);
				//strcpy(info->name, buf);
				//strcpy(info->name, jack_port_name(fOutputPorts[i]));
				break;
			}
		}
		_snprintf(buf, sizeof(buf) - 1, "Out%d ", info->channel);
		strcpy(info->name, buf);
	}
	return ASE_OK;
}

//------------------------------------------------------------------------------------------
ASIOError JackRouter::createBuffers(ASIOBufferInfo *bufferInfos, long numChannels,
	long bufferSize, ASIOCallbacks *callbacks)
{
	ASIOBufferInfo *info = bufferInfos;
	long i;
	bool notEnoughMem = false;
	char buf[256];
	fActiveInputs = 0;
	fActiveOutputs = 0;

	for (i = 0; i < numChannels; i++, info++) {
		if (info->isInput) {
			if (info->channelNum < 0 || info->channelNum >= kNumInputs)
				goto error;
			fInMap[fActiveInputs] = info->channelNum;
		#ifdef LONG_SAMPLE
			fInputBuffers[fActiveInputs] = new long[fBufferSize * 2];	// double buffer
		#else
			fInputBuffers[fActiveInputs] = new jack_default_audio_sample_t[fBufferSize * 2];	// double buffer
		#endif
			if (fInputBuffers[fActiveInputs]) {
				info->buffers[0] = fInputBuffers[fActiveInputs];
				info->buffers[1] = fInputBuffers[fActiveInputs] + fBufferSize;
			} else {
				info->buffers[0] = info->buffers[1] = 0;
				notEnoughMem = true;
			}

			_snprintf(buf, sizeof(buf) - 1, "in%d", fActiveInputs + 1);
			fInputPorts[fActiveInputs]
				= jack_port_register(fClient, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,0);
			if (fInputPorts[fActiveInputs] == NULL)
				goto error;

			fActiveInputs++;
			if (fActiveInputs > kNumInputs) {
error:
				disposeBuffers();
				return ASE_InvalidParameter;
			}
		} else {	// output
			if (info->channelNum < 0 || info->channelNum >= kNumOutputs)
				goto error;
			fOutMap[fActiveOutputs] = info->channelNum;

		#ifdef LONG_SAMPLE
			fOutputBuffers[fActiveOutputs] = new long[fBufferSize * 2];	// double buffer
		#else
			fOutputBuffers[fActiveOutputs] = new jack_default_audio_sample_t[fBufferSize * 2];	// double buffer
		#endif

			if (fOutputBuffers[fActiveOutputs]) {
				info->buffers[0] = fOutputBuffers[fActiveOutputs];
				info->buffers[1] = fOutputBuffers[fActiveOutputs] + fBufferSize;
			} else {
				info->buffers[0] = info->buffers[1] = 0;
				notEnoughMem = true;
			}

			_snprintf(buf, sizeof(buf) - 1, "out%d", fActiveOutputs + 1);
			fOutputPorts[fActiveOutputs]
				= jack_port_register(fClient, buf, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput,0);
			if (fOutputPorts[fActiveOutputs] == NULL)
				goto error;

			fActiveOutputs++;
			if (fActiveOutputs > kNumOutputs) {
				fActiveOutputs--;
				disposeBuffers();
				return ASE_InvalidParameter;
			}
		}
	}

	if (notEnoughMem) {
		disposeBuffers();
		return ASE_NoMemory;
	}

	this->fCallbacks = callbacks;
	if (callbacks->asioMessage (kAsioSupportsTimeInfo, 0, 0, 0)) {
		fTimeInfoMode = true;
		fAsioTime.timeInfo.speed = 1.;
		fAsioTime.timeInfo.systemTime.hi = fAsioTime.timeInfo.systemTime.lo = 0;
		fAsioTime.timeInfo.samplePosition.hi = fAsioTime.timeInfo.samplePosition.lo = 0;
		fAsioTime.timeInfo.sampleRate = fSampleRate;
		fAsioTime.timeInfo.flags = kSystemTimeValid | kSamplePositionValid | kSampleRateValid;

		fAsioTime.timeCode.speed = 1.;
		fAsioTime.timeCode.timeCodeSamples.lo = fAsioTime.timeCode.timeCodeSamples.hi = 0;
		fAsioTime.timeCode.flags = kTcValid | kTcRunning ;
	} else {
		fTimeInfoMode = false;
	}

	return ASE_OK;
}

//---------------------------------------------------------------------------------------------
ASIOError JackRouter::disposeBuffers()
{
	long i;

	fCallbacks = 0;
	stop();

	for (i = 0; i < fActiveInputs; i++) {
		delete[] fInputBuffers[i];
		jack_port_unregister(fClient, fInputPorts[i]);
	}
	fActiveInputs = 0;

	for (i = 0; i < fActiveOutputs; i++) {
		delete[] fOutputBuffers[i];
		jack_port_unregister(fClient, fOutputPorts[i]);
	}
	fActiveOutputs = 0;

	return ASE_OK;
}

//---------------------------------------------------------------------------------------------
ASIOError JackRouter::controlPanel()
{
	return ASE_NotPresent;
}

//---------------------------------------------------------------------------------------------
ASIOError JackRouter::future(long selector, void* opt)	// !!! check properties
{
	ASIOTransportParameters* tp = (ASIOTransportParameters*)opt;
	switch (selector)
	{
		case kAsioEnableTimeCodeRead:	fTcRead = true;	return ASE_SUCCESS;
		case kAsioDisableTimeCodeRead:	fTcRead = false; return ASE_SUCCESS;
		case kAsioSetInputMonitor:		return ASE_SUCCESS;	// for testing!!!
		case kAsioCanInputMonitor:		return ASE_SUCCESS;	// for testing!!!
		case kAsioCanTimeInfo:			return ASE_SUCCESS;
		case kAsioCanTimeCode:			return ASE_SUCCESS;
	}
	return ASE_NotPresent;
}

//--------------------------------------------------------------------------------------------------------
// private methods
//--------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------
void JackRouter::bufferSwitch()
{
	if (fStarted && fCallbacks) {
		getNanoSeconds(&fTheSystemTime);			// latch system time
		fSamplePosition += fBufferSize;
		if (fTimeInfoMode) {
			bufferSwitchX ();
		} else {
			fCallbacks->bufferSwitch (fToggle, ASIOFalse);
		}
		fToggle = fToggle ? 0 : 1;
	}
}

//---------------------------------------------------------------------------------------------
// asio2 buffer switch
void JackRouter::bufferSwitchX ()
{
	getSamplePosition (&fAsioTime.timeInfo.samplePosition, &fAsioTime.timeInfo.systemTime);
	long offset = fToggle ? fBufferSize : 0;
	if (fTcRead) {
		// Create a fake time code, which is 10 minutes ahead of the card's sample position
		// Please note that for simplicity here time code will wrap after 32 bit are reached
		fAsioTime.timeCode.timeCodeSamples.lo = fAsioTime.timeInfo.samplePosition.lo + 600.0 * fSampleRate;
		fAsioTime.timeCode.timeCodeSamples.hi = 0;
	}
	fCallbacks->bufferSwitchTimeInfo (&fAsioTime, fToggle, ASIOFalse);
	fAsioTime.timeInfo.flags &= ~(kSampleRateChanged | kClockSourceChanged);
}

//---------------------------------------------------------------------------------------------
ASIOError JackRouter::outputReady()
{
	return ASE_NotPresent;
}

//---------------------------------------------------------------------------------------------
double AsioSamples2double(ASIOSamples* samples)
{
	double a = (double)(samples->lo);
	if (samples->hi)
		a += (double)(samples->hi) * twoRaisedTo32;
	return a;
}

//---------------------------------------------------------------------------------------------
void getNanoSeconds(ASIOTimeStamp* ts)
{
	double nanoSeconds = (double)((unsigned long)timeGetTime ()) * 1000000.;
	ts->hi = (unsigned long)(nanoSeconds / twoRaisedTo32);
	ts->lo = (unsigned long)(nanoSeconds - (ts->hi * twoRaisedTo32));
}

//------------------------------------------------------------------------
void JackRouter::SaveConnections()
{
    const char** connections;
 	int i;

    for (i = 0; i < fActiveInputs; ++i) {
        if (fInputPorts[i] && (connections = jack_port_get_connections(fInputPorts[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(connections[j], jack_port_name(fInputPorts[i])));
            }
            jack_free(connections);
        }
    }

    for (i = 0; i < fActiveOutputs; ++i) {
        if (fOutputPorts[i] && (connections = jack_port_get_connections(fOutputPorts[i])) != 0) {
            for (int j = 0; connections[j]; j++) {
                fConnections.push_back(make_pair(jack_port_name(fOutputPorts[i]), connections[j]));
            }
            jack_free(connections);
        }
    }
}

//------------------------------------------------------------------------
void JackRouter::RestoreConnections()
{
    list<pair<string, string> >::const_iterator it;

    for (it = fConnections.begin(); it != fConnections.end(); it++) {
        pair<string, string> connection = *it;
        jack_connect(fClient, connection.first.c_str(), connection.second.c_str());
    }

    fConnections.clear();
}

//------------------------------------------------------------------------------------------
void JackRouter::AutoConnect()
{
	const char** ports;

	if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsOutput)) == NULL) {
		printf("Cannot find any physical capture ports\n");
	} else {
		if (fAutoConnectIn) {
			for (int i = 0; i < fActiveInputs; i++) {
				if (!ports[i]) {
					printf("source port is null i = %ld\n", i);
					break;
				} else if (jack_connect(fClient, ports[i], jack_port_name(fInputPorts[i])) != 0) {
					printf("Cannot connect input ports\n");
				}
			}
		}
		jack_free(ports);
	}

	if ((ports = jack_get_ports(fClient, NULL, NULL, JackPortIsPhysical | JackPortIsInput)) == NULL) {
		printf("Cannot find any physical playback ports");
	} else {
		if (fAutoConnectOut) {
			for (int i = 0; i < fActiveOutputs; i++) {
				if (!ports[i]){
					printf("destination port is null i = %ld\n", i);
					break;
				} else if (jack_connect(fClient, jack_port_name(fOutputPorts[i]), ports[i]) != 0) {
					printf("Cannot connect output ports\n");
				}
			}
		}
		jack_free(ports);
	}
}

