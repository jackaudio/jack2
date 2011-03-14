/*
Copyright (C) 2009 Grame

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

#include "JackWinMMEDriver.h"
#include "JackGraphManager.h"
#include "JackEngineControl.h"
#include "JackDriverLoader.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

namespace Jack
{

static bool InitHeaders(MidiSlot* slot)
{
    slot->fHeader = (LPMIDIHDR)GlobalAllocPtr(GMEM_MOVEABLE|GMEM_SHARE|GMEM_ZEROINIT, sizeof(MIDIHDR) + kBuffSize);
 	if (!slot->fHeader)
        return false;

	slot->fHeader->lpData = (LPSTR)((LPBYTE)slot->fHeader + sizeof(MIDIHDR));
	slot->fHeader->dwBufferLength = kBuffSize;
	slot->fHeader->dwFlags = 0;
	slot->fHeader->dwUser = 0;
	slot->fHeader->lpNext = 0;
	slot->fHeader->dwBytesRecorded = 0;
	return true;
}

void CALLBACK JackWinMMEDriver::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD userData, DWORD dwParam1, DWORD dwParam2)
{
    jack_ringbuffer_t* ringbuffer = (jack_ringbuffer_t*)userData;
    //jack_info("JackWinMMEDriver::MidiInProc 0\n");

	switch (wMsg) {
		case MIM_OPEN:
            break;

		case MIM_ERROR:
		case MIM_DATA: {

            //jack_info("JackWinMMEDriver::MidiInProc");

            // One event
            unsigned int num_packet = 1;
            jack_ringbuffer_write(ringbuffer, (char*)&num_packet, sizeof(unsigned int));

            // Write event actual data
            jack_ringbuffer_write(ringbuffer, (char*)&dwParam1, 3);
       		break;
		}

		case MIM_LONGERROR:
		case MIM_LONGDATA:
            /*
			Nothing for now
            */
			break;
	}
}

JackWinMMEDriver::JackWinMMEDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table)
    : JackMidiDriver(name, alias, engine, table),
    fRealCaptureChannels(0),
    fRealPlaybackChannels(0),
    fMidiSource(NULL),
    fMidiDestination(NULL)
{}

JackWinMMEDriver::~JackWinMMEDriver()
{}

int JackWinMMEDriver::Open(bool capturing,
         bool playing,
         int inchannels,
         int outchannels,
         bool monitor,
         const char* capture_driver_name,
         const char* playback_driver_name,
         jack_nframes_t capture_latency,
         jack_nframes_t playback_latency)
{

    jack_log("JackWinMMEDriver::Open");

    fRealCaptureChannels = midiInGetNumDevs();
	fRealPlaybackChannels = midiOutGetNumDevs();

    // Generic JackMidiDriver Open
    if (JackMidiDriver::Open(capturing, playing, fRealCaptureChannels, fRealPlaybackChannels, monitor, capture_driver_name, playback_driver_name, capture_latency, playback_latency) != 0)
        return -1;

    fMidiDestination = new MidiSlot[fRealCaptureChannels];
    assert(fMidiDestination);

    // Real input
    int devindex = 0;
    for (int i = 0; i < fRealCaptureChannels; i++)  {

        HMIDIIN handle;
        fMidiDestination[devindex].fIndex = i;
        MMRESULT ret = midiInOpen(&handle, fMidiDestination[devindex].fIndex, (DWORD)MidiInProc, (DWORD)fRingBuffer[devindex], CALLBACK_FUNCTION);

        if (ret == MMSYSERR_NOERROR) {
            fMidiDestination[devindex].fHandle = handle;
            if (!InitHeaders(&fMidiDestination[devindex])) {
                jack_error("memory allocation failed");
                midiInClose(handle);
                continue;
            }
            ret = midiInPrepareHeader(handle, fMidiDestination[devindex].fHeader, sizeof(MIDIHDR));

            if (ret == MMSYSERR_NOERROR) {
                fMidiDestination[devindex].fHeader->dwUser = 1;
                ret = midiInAddBuffer(handle, fMidiDestination[devindex].fHeader, sizeof(MIDIHDR));
                if (ret == MMSYSERR_NOERROR) {
                    ret = midiInStart(handle);
                    if (ret != MMSYSERR_NOERROR) {
                        jack_error("midiInStart error");
                        CloseInput(&fMidiDestination[devindex]);
                        continue;
                    }
                } else {
                    jack_error ("midiInAddBuffer error");
                    CloseInput(&fMidiDestination[devindex]);
                    continue;
                }
            } else {
                jack_error("midiInPrepareHeader error");
                midiInClose(handle);
                continue;
            }
        } else {
            jack_error ("midiInOpen error");
            continue;
        }
        devindex += 1;
    }
    fRealCaptureChannels = devindex;
    fCaptureChannels = devindex;

    fMidiSource = new MidiSlot[fRealPlaybackChannels];
    assert(fMidiSource);

    // Real output
    devindex = 0;
    for (int i = 0; i < fRealPlaybackChannels; i++)  {
        MMRESULT res;
        HMIDIOUT handle;
        fMidiSource[devindex].fIndex = i;
        UINT ret = midiOutOpen(&handle, fMidiSource[devindex].fIndex, 0L, 0L, CALLBACK_NULL);
        if (ret == MMSYSERR_NOERROR) {
            fMidiSource[devindex].fHandle = handle;
            if (!InitHeaders(&fMidiSource[devindex])) {
                jack_error("memory allocation failed");
                midiOutClose(handle);
                continue;
            }
            res = midiOutPrepareHeader(handle, fMidiSource[devindex].fHeader, sizeof(MIDIHDR));
            if (res != MMSYSERR_NOERROR) {
                jack_error("midiOutPrepareHeader error %d %d %d", i, handle, res);
                continue;
            } else {
                fMidiSource[devindex].fHeader->dwUser = 1;
            }
        } else {
            jack_error("midiOutOpen error");
            continue;
        }
        devindex += 1;
    }
    fRealPlaybackChannels = devindex;
    fPlaybackChannels = devindex;
    return 0;
}

void JackWinMMEDriver::CloseInput(MidiSlot* slot)
{
    MMRESULT res;
    int retry = 0;

    if (slot->fHandle == 0)
        return;

    HMIDIIN handle = (HMIDIIN)slot->fHandle;
    slot->fHeader->dwUser = 0;
    res = midiInStop(handle);
    if (res != MMSYSERR_NOERROR) {
        jack_error("midiInStop error");
    }
    res = midiInReset(handle);
    if (res != MMSYSERR_NOERROR) {
        jack_error("midiInReset error");
    }
    res = midiInUnprepareHeader(handle, slot->fHeader, sizeof(MIDIHDR));
    if (res != MMSYSERR_NOERROR) {
        jack_error("midiInUnprepareHeader error");
    }
    do {
        res = midiInClose(handle);
        if (res != MMSYSERR_NOERROR) {
            jack_error("midiInClose error");
        }
        if (res == MIDIERR_STILLPLAYING)
            midiInReset(handle);
        Sleep (10);
        retry++;
    } while ((res == MIDIERR_STILLPLAYING) && (retry < 10));

    if (slot->fHeader) {
        GlobalFreePtr(slot->fHeader);
    }
}

void JackWinMMEDriver::CloseOutput(MidiSlot* slot)
{
    MMRESULT res;
    int retry = 0;

    if (slot->fHandle == 0)
        return;

    HMIDIOUT handle = (HMIDIOUT)slot->fHandle;
    res = midiOutReset(handle);
    if (res != MMSYSERR_NOERROR)
        jack_error("midiOutReset error");
    midiOutUnprepareHeader(handle, slot->fHeader, sizeof(MIDIHDR));
    do {
        res = midiOutClose(handle);
        if (res != MMSYSERR_NOERROR)
            jack_error("midiOutClose error");
        Sleep(10);
        retry++;
    } while ((res == MIDIERR_STILLPLAYING) && (retry < 10));

    if (slot->fHeader) {
        GlobalFreePtr(slot->fHeader);
    }
}

int JackWinMMEDriver::Close()
{
    jack_log("JackWinMMEDriver::Close");

    // Generic midi driver close
    int res = JackMidiDriver::Close();

    // Close input
    if (fMidiDestination) {
        for (int i = 0; i < fRealCaptureChannels; i++)  {
            CloseInput(&fMidiDestination[i]);
        }
        delete[] fMidiDestination;
    }

    // Close output
    if (fMidiSource) {
        for (int i = 0; i < fRealPlaybackChannels; i++)  {
            CloseOutput(&fMidiSource[i]);
        }
        delete[] fMidiSource;
    }

    return res;
}

int JackWinMMEDriver::Attach()
{
    JackPort* port;
    jack_port_id_t port_index;
    char name[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    char alias[JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE];
    MMRESULT res;
    int i;

    jack_log("JackMidiDriver::Attach fBufferSize = %ld fSampleRate = %ld", fEngineControl->fBufferSize, fEngineControl->fSampleRate);

    for (i = 0; i < fCaptureChannels; i++) {
        MIDIINCAPS caps;
		res = midiInGetDevCaps(fMidiDestination[i].fIndex, &caps, sizeof(caps));
		if (res == MMSYSERR_NOERROR) {
            snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, caps.szPname, i + 1);
		} else {
		    snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, fCaptureDriverName, i + 1);
		}
        snprintf(name, sizeof(name) - 1, "%s:capture_%d", fClientControl.fName, i + 1);

        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE, CaptureDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fCapturePortList[i] = port_index;
        jack_log("JackMidiDriver::Attach fCapturePortList[i] port_index = %ld", port_index);
    }

    for (i = 0; i < fPlaybackChannels; i++) {
        MIDIOUTCAPS caps;
		res = midiOutGetDevCaps(fMidiSource[i].fIndex, &caps, sizeof(caps));
        if (res == MMSYSERR_NOERROR) {
            snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, caps.szPname, i + 1);
		} else {
		    snprintf(alias, sizeof(alias) - 1, "%s:%s:out%d", fAliasName, fPlaybackDriverName, i + 1);
		}
        snprintf(name, sizeof(name) - 1, "%s:playback_%d", fClientControl.fName, i + 1);

        if ((port_index = fGraphManager->AllocatePort(fClientControl.fRefNum, name, JACK_DEFAULT_MIDI_TYPE, PlaybackDriverFlags, fEngineControl->fBufferSize)) == NO_PORT) {
            jack_error("driver: cannot register port for %s", name);
            return -1;
        }
        port = fGraphManager->GetPort(port_index);
        port->SetAlias(alias);
        fPlaybackPortList[i] = port_index;
        jack_log("JackMidiDriver::Attach fPlaybackPortList[i] port_index = %ld", port_index);
    }

    return 0;
}

int JackWinMMEDriver::Read()
{
    size_t size;

    for (int chan = 0; chan < fCaptureChannels; chan++)  {

        if (fGraphManager->GetConnectionsNum(fCapturePortList[chan]) > 0) {

            JackMidiBuffer* midi_buffer = GetInputBuffer(chan);

            if (jack_ringbuffer_read_space (fRingBuffer[chan]) == 0) {
                // Reset buffer
                midi_buffer->Reset(midi_buffer->nframes);
            } else {

                while ((size = jack_ringbuffer_read_space (fRingBuffer[chan])) > 0) {

                    //jack_info("jack_ringbuffer_read_space %d", size);
                    int ev_count = 0;
                    jack_ringbuffer_read(fRingBuffer[chan], (char*)&ev_count, sizeof(int));

                    if (ev_count > 0) {
                        for (int j = 0; j < ev_count; j++)  {
                            unsigned int event_len = 3;
                            // Read event actual data
                            jack_midi_data_t* dest = midi_buffer->ReserveEvent(0, event_len);
                            jack_ringbuffer_read(fRingBuffer[chan], (char*)dest, event_len);
                        }
                    }
                }
            }
        } else {
            //jack_info("Consume ring buffer");
            jack_ringbuffer_read_advance(fRingBuffer[chan], jack_ringbuffer_read_space(fRingBuffer[chan]));
        }
    }
    return 0;
}

int JackWinMMEDriver::Write()
{
    for (int chan = 0; chan < fPlaybackChannels; chan++)  {

        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[chan]) > 0) {

            JackMidiBuffer* midi_buffer = GetOutputBuffer(chan);

            // TODO : use timestamp

            for (unsigned int j = 0; j < midi_buffer->event_count; j++) {
                JackMidiEvent* ev = &midi_buffer->events[j];
                if (ev->size <= 3) {
                    jack_midi_data_t *d = ev->GetData(midi_buffer);
                    DWORD winev = 0;
                    if (ev->size > 0) winev |= d[0];
                    if (ev->size > 1) winev |= (d[1] << 8);
                    if (ev->size > 2) winev |= (d[2] << 16);
                    MMRESULT res = midiOutShortMsg((HMIDIOUT)fMidiSource[chan].fHandle, winev);
                    if (res != MMSYSERR_NOERROR)
                        jack_error ("midiOutShortMsg error res %d", res);
                } else  {

                }
            }
        }
    }

    return 0;
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

    SERVER_EXPORT jack_driver_desc_t * driver_get_descriptor()
    {
        jack_driver_desc_t * desc;
        //unsigned int i;

        desc = (jack_driver_desc_t*)calloc (1, sizeof (jack_driver_desc_t));
        strcpy(desc->name, "winmme");                             // size MUST be less then JACK_DRIVER_NAME_MAX + 1
        strcpy(desc->desc, "WinMME API based MIDI backend");      // size MUST be less then JACK_DRIVER_PARAM_DESC + 1

        desc->nparams = 0;
        desc->params = (jack_driver_param_desc_t*)calloc (desc->nparams, sizeof (jack_driver_param_desc_t));

        return desc;
    }

    SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
    {
        /*
        unsigned int capture_ports = 2;
        unsigned int playback_ports = 2;
        unsigned long wait_time = 0;
        const JSList * node;
        const jack_driver_param_t * param;
        bool monitor = false;

        for (node = params; node; node = jack_slist_next (node)) {
            param = (const jack_driver_param_t *) node->data;

            switch (param->character) {

                case 'C':
                    capture_ports = param->value.ui;
                    break;

                case 'P':
                    playback_ports = param->value.ui;
                    break;

                case 'r':
                    sample_rate = param->value.ui;
                    break;

                case 'p':
                    period_size = param->value.ui;
                    break;

                case 'w':
                    wait_time = param->value.ui;
                    break;

                case 'm':
                    monitor = param->value.i;
                    break;
            }
        }
        */

        Jack::JackDriverClientInterface* driver = new Jack::JackWinMMEDriver("system_midi", "winmme", engine, table);
        if (driver->Open(1, 1, 0, 0, false, "in", "out", 0, 0) == 0) {
            return driver;
        } else {
            delete driver;
            return NULL;
        }
    }

#ifdef __cplusplus
}
#endif


/*
jack_connect system:midi_capture_1 system_midi:playback_1
jack_connect system:midi_capture_1 system_midi:playback_2

jack_connect system:midi_capture_1 system_midi:playback_1

jack_connect system:midi_capture_1 system_midi:playback_1

jack_connect system:midi_capture_1 system_midi:playback_1

jack_connect system_midi:capture_1 system:midi_playback_1
jack_connect system_midi:capture_2 system:midi_playback_1

jack_connect system_midi:capture_1  system_midi:playback_1

*/
