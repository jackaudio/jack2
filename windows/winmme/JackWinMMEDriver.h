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

#ifndef __JackWinMMEDriver__
#define __JackWinMMEDriver__

#include "JackMidiDriver.h"
#include "JackTime.h"

namespace Jack
{

/*!
\brief The WinMME driver.
*/

#define kBuffSize	512

struct MidiSlot {

	LPVOID	    fHandle;    // MMSystem handler
	short		fIndex;     // MMSystem dev index
	LPMIDIHDR	fHeader;    // for long msg output

	MidiSlot():fHandle(0),fIndex(0)
	{}

};

class JackWinMMEDriver : public JackMidiDriver
{

    private:

        int fRealCaptureChannels;
        int fRealPlaybackChannels;

        MidiSlot* fMidiSource;
        MidiSlot* fMidiDestination;

        void CloseInput(MidiSlot* slot);
        void CloseOutput(MidiSlot* slot);

        static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

    public:

        JackWinMMEDriver(const char* name, const char* alias, JackLockedEngine* engine, JackSynchro* table);
        virtual ~JackWinMMEDriver();

        int Open(bool capturing,
                 bool playing,
                 int chan_in,
                 int chan_out,
                 bool monitor,
                 const char* capture_driver_name,
                 const char* playback_driver_name,
                 jack_nframes_t capture_latency,
                 jack_nframes_t playback_latency);
        int Close();

        int Attach();

        int Read();
        int Write();

};

} // end of namespace

#endif
