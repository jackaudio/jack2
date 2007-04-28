/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2006 Grame

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

#include "JackPort.h"
#include "JackError.h"
#include <stdio.h>

namespace Jack
{

JackPort::JackPort()
        : fFlags(JackPortIsInput), fRefNum( -1), fLatency(0), fMonitorRequests(0), fInUse(false), fLocked(false), fTied(NO_PORT)
{}

JackPort::~JackPort()
{}

void JackPort::Allocate(int refnum, const char* port_name, JackPortFlags flags)
{
    fFlags = flags;
    fRefNum = refnum;
    strcpy(fName, port_name);
    memset(fBuffer, 0, BUFFER_SIZE_MAX * sizeof(float));
    fInUse = true;
    fLocked = false;
    fLatency = 0;
    fTied = NO_PORT;
}

void JackPort::Release()
{
    fFlags = JackPortIsInput;
    fRefNum = -1;
    fInUse = false;
    fLocked = false;
    fLatency = 0;
    fTied = NO_PORT;
}

bool JackPort::IsUsed() const
{
    return fInUse;
}

float* JackPort::GetBuffer()
{
    return fBuffer;
}

int JackPort::GetRefNum() const
{
    return fRefNum;
}

int JackPort::Lock()
{
    fLocked = true;
    return 0;
}

int JackPort::Unlock()
{
    fLocked = false;
    return 0;
}

jack_nframes_t JackPort::GetLatency() const
{
    return fLatency;
}

void JackPort::SetLatency(jack_nframes_t nframes)
{
    fLatency = nframes;
}

int JackPort::Tie(jack_port_id_t port_index)
{
    fTied = port_index;
    return 0;
}

int JackPort::UnTie()
{
    fTied = NO_PORT;
    return 0;
}

int JackPort::RequestMonitor(bool onoff)
{
    /**
    jackd.h 
     * If @ref JackPortCanMonitor is set for this @a port, turn input
     * monitoring on or off.  Otherwise, do nothing.
     
     if (!(fFlags & JackPortCanMonitor))
    	return -1;
     */

    if (onoff) {
        fMonitorRequests++;
    } else if (fMonitorRequests) {
        fMonitorRequests--;
    }

    return 0;
}

int JackPort::EnsureMonitor(bool onoff)
{
    /**
    jackd.h 
        * If @ref JackPortCanMonitor is set for this @a port, turn input
        * monitoring on or off.  Otherwise, do nothing.
     
     if (!(fFlags & JackPortCanMonitor))
    	return -1;
     */

    if (onoff) {
        if (fMonitorRequests == 0) {
            fMonitorRequests++;
        }
    } else {
        if (fMonitorRequests > 0) {
            fMonitorRequests = 0;
        }
    }

    return 0;
}

bool JackPort::MonitoringInput()
{
    return (fMonitorRequests > 0);
}

const char* JackPort::GetName() const
{
    return fName;
}

const char* JackPort::GetShortName() const
{
    /* we know there is always a colon, because we put
       it there ...
    */ 
    return strchr(fName, ':') + 1;
}

int JackPort::Flags() const
{
    return fFlags;
}

const char* JackPort::Type() const
{
    // TO IMPROVE
    return "Audio";
}

int JackPort::SetName(const char* new_name)
{
    char* colon = strchr(fName, ':');
    int len = sizeof(fName) - ((int) (colon - fName)) - 2;
    snprintf(colon + 1, len, "%s", new_name);
    return 0;
}

bool JackPort::NameEquals(const char* target)
{
	return (strcmp(fName, target) == 0 
		|| strcmp(fAlias1, target) == 0 
		|| strcmp(fAlias2, target) == 0);
}

int JackPort::GetAliases(char* const aliases[2])
{
	int cnt = 0;
	
	if (fAlias1[0] != '\0') {
		snprintf(aliases[0], JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE, "%s", fAlias1);
		cnt++;
	}

	if (fAlias2[0] != '\0') {
		snprintf(aliases[1], JACK_CLIENT_NAME_SIZE + JACK_PORT_NAME_SIZE, "%s", fAlias2);
		cnt++;
	}

	return cnt;
}

int JackPort::SetAlias(const char* alias)
{
	if (fAlias1[0] == '\0') {
		snprintf(fAlias1, sizeof(fAlias1), "%s", alias);
	} else if (fAlias2[0] == '\0') {
		snprintf(fAlias2, sizeof(fAlias2), "%s", alias);
	} else {
		return -1;
	}

	return 0;
}

int JackPort::UnsetAlias(const char* alias)
{
	if (strcmp(fAlias1, alias) == 0) {
		fAlias1[0] = '\0';
	} else if (strcmp(fAlias2, alias) == 0) {
		fAlias2[0] = '\0';
	} else {
		return -1;
	}

	return 0;
}

void JackPort::MixBuffer(float* mixbuffer, float* buffer, jack_nframes_t frames)
{
    jack_nframes_t frames_group = frames / 4;
    frames = frames % 4;

    while (frames_group > 0) {
        register float mixFloat1 = *mixbuffer;
        register float sourceFloat1 = *buffer;
        register float mixFloat2 = *(mixbuffer + 1);
        register float sourceFloat2 = *(buffer + 1);
        register float mixFloat3 = *(mixbuffer + 2);
        register float sourceFloat3 = *(buffer + 2);
        register float mixFloat4 = *(mixbuffer + 3);
        register float sourceFloat4 = *(buffer + 3);

        buffer += 4;
        frames_group--;

        mixFloat1 += sourceFloat1;
        mixFloat2 += sourceFloat2;
        mixFloat3 += sourceFloat3;
        mixFloat4 += sourceFloat4;

        *mixbuffer = mixFloat1;
        *(mixbuffer + 1) = mixFloat2;
        *(mixbuffer + 2) = mixFloat3;
        *(mixbuffer + 3) = mixFloat4;

        mixbuffer += 4;
    }

    while (frames > 0) {
        register float mixFloat1 = *mixbuffer;
        register float sourceFloat1 = *buffer;
        buffer++;
        frames--;
        mixFloat1 += sourceFloat1;
        *mixbuffer = mixFloat1;
        mixbuffer++;
    }
}

} // end of namespace
