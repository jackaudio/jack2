/*
Copyright (C) 2001-2003 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include "JackPort.h"
#include "JackError.h"
#include "JackPortType.h"
#include <stdio.h>
#include <assert.h>

namespace Jack
{

JackPort::JackPort()
{
    Release();
}

bool JackPort::Allocate(int refnum, const char* port_name, const char* port_type, JackPortFlags flags)
{
    jack_port_type_id_t id = GetPortTypeId(port_type);
    assert(id >= 0 && id <= PORT_TYPES_MAX);
    if (id == PORT_TYPES_MAX) {
        return false;
    }
    fTypeId = id;
    fFlags = flags;
    fRefNum = refnum;
    strcpy(fName, port_name);
    fInUse = true;
    fLatency = 0;
    fTotalLatency = 0;
    fMonitorRequests = 0;
    fPlaybackLatency.min = fPlaybackLatency.max = 0;
    fCaptureLatency.min = fCaptureLatency.max = 0;
    fTied = NO_PORT;
    fAlias1[0] = '\0';
    fAlias2[0] = '\0';
    // DB: At this point we do not know current buffer size in frames,
    // but every time buffer will be returned to any user,
    // it will be called with either ClearBuffer or MixBuffers
    // with correct current buffer size.
    // So it is safe to init with 0 here.
    ClearBuffer(0);
    return true;
}

void JackPort::Release()
{
    fTypeId = 0;
    fFlags = JackPortIsInput;
    fRefNum = -1;
    fInUse = false;
    fLatency = 0;
    fTotalLatency = 0;
    fMonitorRequests = 0;
    fPlaybackLatency.min = fPlaybackLatency.max = 0;
    fCaptureLatency.min = fCaptureLatency.max = 0;
    fTied = NO_PORT;
    fAlias1[0] = '\0';
    fAlias2[0] = '\0';
}

int JackPort::GetRefNum() const
{
    return fRefNum;
}

jack_nframes_t JackPort::GetLatency() const
{
    return fLatency;
}

jack_nframes_t JackPort::GetTotalLatency() const
{
    return fTotalLatency;
}

void JackPort::SetLatency(jack_nframes_t nframes)
{
    fLatency = nframes;

    /* setup the new latency values here,
	 * so we don't need to change the backend codes.
	 */
	if (fFlags & JackPortIsOutput) {
		fCaptureLatency.min = nframes;
		fCaptureLatency.max = nframes;
	}
	if (fFlags & JackPortIsInput) {
		fPlaybackLatency.min = nframes;
		fPlaybackLatency.max = nframes;
	}
}

void JackPort::SetLatencyRange(jack_latency_callback_mode_t mode, jack_latency_range_t* range)
{
    if (mode == JackCaptureLatency) {
		fCaptureLatency = *range;

		/* hack to set latency up for
		 * backend ports
		 */
		if ((fFlags & JackPortIsOutput) && (fFlags & JackPortIsPhysical)) {
			fLatency = (range->min + range->max) / 2;
        }
	} else {
        fPlaybackLatency = *range;

		/* hack to set latency up for
		 * backend ports
		 */
		if ((fFlags & JackPortIsInput) && (fFlags & JackPortIsPhysical)) {
			fLatency = (range->min + range->max) / 2;
        }
	}
}

void JackPort::GetLatencyRange(jack_latency_callback_mode_t mode, jack_latency_range_t* range) const
{
    if (mode == JackCaptureLatency) {
		*range = fCaptureLatency;
	} else {
		*range = fPlaybackLatency;
    }
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
    * monitoring on or off. Otherwise, do nothing.

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
    * monitoring on or off. Otherwise, do nothing.

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

int JackPort::GetFlags() const
{
    return fFlags;
}

const char* JackPort::GetType() const
{
    const JackPortType* type = GetPortType(fTypeId);
    return type->fName;
}

void JackPort::SetName(const char* new_name)
{
    char* colon = strchr(fName, ':');
    int len = sizeof(fName) - ((int) (colon - fName)) - 2;
    snprintf(colon + 1, len, "%s", new_name);
}

bool JackPort::NameEquals(const char* target)
{
    char buf[REAL_JACK_PORT_NAME_SIZE+1];

    /* this nasty, nasty kludge is here because between 0.109.0 and 0.109.1,
       the ALSA audio backend had the name "ALSA", whereas as before and
       after it, it was called "alsa_pcm". this stops breakage for
       any setups that have saved "alsa_pcm" or "ALSA" in their connection
       state.
    */

    if (strncmp(target, "ALSA:capture", 12) == 0 || strncmp(target, "ALSA:playback", 13) == 0) {
        snprintf(buf, sizeof(buf), "alsa_pcm%s", target + 4);
        target = buf;
    }

    return (strcmp(fName, target) == 0
            || strcmp(fAlias1, target) == 0
            || strcmp(fAlias2, target) == 0);
}

int JackPort::GetAliases(char* const aliases[2])
{
    int cnt = 0;

    if (fAlias1[0] != '\0') {
        snprintf(aliases[0], REAL_JACK_PORT_NAME_SIZE, "%s", fAlias1);
        cnt++;
    }

    if (fAlias2[0] != '\0') {
        snprintf(aliases[1], REAL_JACK_PORT_NAME_SIZE, "%s", fAlias2);
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

void JackPort::ClearBuffer(jack_nframes_t frames)
{
    const JackPortType* type = GetPortType(fTypeId);
    (type->init)(GetBuffer(), frames * sizeof(jack_default_audio_sample_t), frames);
}

void JackPort::MixBuffers(void** src_buffers, int src_count, jack_nframes_t buffer_size)
{
    const JackPortType* type = GetPortType(fTypeId);
    (type->mixdown)(GetBuffer(), src_buffers, src_count, buffer_size);
}

} // end of namespace
