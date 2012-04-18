/*
Copyright (C) 2003-2007 Jussi Laako <jussi@sonarnerd.net>
Copyright (C) 2008 Grame & RTL 2008

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

#include "driver_interface.h"
#include "JackThreadedDriver.h"
#include "JackDriverLoader.h"
#include "JackOSSDriver.h"
#include "JackEngineControl.h"
#include "JackGraphManager.h"
#include "JackError.h"
#include "JackTime.h"
#include "JackShmMem.h"
#include "memops.h"

#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>

using namespace std;

namespace Jack
{

#ifdef JACK_MONITOR

#define CYCLE_POINTS 500000

struct OSSCycle {
    jack_time_t fBeforeRead;
    jack_time_t fAfterRead;
    jack_time_t fAfterReadConvert;
    jack_time_t fBeforeWrite;
    jack_time_t fAfterWrite;
    jack_time_t fBeforeWriteConvert;
};

struct OSSCycleTable {
    jack_time_t fBeforeFirstWrite;
    jack_time_t fAfterFirstWrite;
    OSSCycle fTable[CYCLE_POINTS];
};

OSSCycleTable gCycleTable;
int gCycleCount = 0;

#endif

inline int int2pow2(int x)	{ int r = 0; while ((1 << r) < x) r++; return r; }

static inline void CopyAndConvertIn(jack_sample_t *dst, void *src, size_t nframes, int channel, int chcount, int bits)
{
    switch (bits) {

		case 16: {
		    signed short *s16src = (signed short*)src;
            s16src += channel;
            sample_move_dS_s16(dst, (char*)s16src, nframes, chcount<<1);
			break;
        }
		case 24: {
			signed int *s32src = (signed int*)src;
            s32src += channel;
            sample_move_dS_s24(dst, (char*)s32src, nframes, chcount<<2);
			break;
        }
		case 32: {
			signed int *s32src = (signed int*)src;
            s32src += channel;
            sample_move_dS_s32u24(dst, (char*)s32src, nframes, chcount<<2);
			break;
        }
	}
}

static inline void CopyAndConvertOut(void *dst, jack_sample_t *src, size_t nframes, int channel, int chcount, int bits)
{
	switch (bits) {

		case 16: {
			signed short *s16dst = (signed short*)dst;
            s16dst += channel;
            sample_move_d16_sS((char*)s16dst, src, nframes, chcount<<1, NULL); // No dithering for now...
			break;
        }
		case 24: {
			signed int *s32dst = (signed int*)dst;
            s32dst += channel;
            sample_move_d24_sS((char*)s32dst, src, nframes, chcount<<2, NULL); // No dithering for now...
			break;
        }
		case 32: {
            signed int *s32dst = (signed int*)dst;
            s32dst += channel;
            sample_move_d32u24_sS((char*)s32dst, src, nframes, chcount<<2, NULL);
			break;
        }
	}
}

void JackOSSDriver::SetSampleFormat()
{
    switch (fBits) {

	    case 24:	/* native-endian LSB aligned 24-bits in 32-bits integer */
            fSampleFormat = AFMT_S24_NE;
            fSampleSize = sizeof(int);
			break;
		case 32:	/* native-endian 32-bit integer */
            fSampleFormat = AFMT_S32_NE;
            fSampleSize = sizeof(int);
			break;
		case 16:	/* native-endian 16-bit integer */
		default:
            fSampleFormat = AFMT_S16_NE;
            fSampleSize = sizeof(short);
			break;
    }
}

void JackOSSDriver::DisplayDeviceInfo()
{
    audio_buf_info info;
    oss_audioinfo ai_in, ai_out;
    memset(&info, 0, sizeof(audio_buf_info));
    int cap = 0;

    // Duplex cards : http://manuals.opensound.com/developer/full_duplex.html
    jack_info("Audio Interface Description :");
    jack_info("Sampling Frequency : %d, Sample Format : %d, Mode : %d", fEngineControl->fSampleRate, fSampleFormat, fRWMode);

    if (fRWMode & kWrite) {

        oss_sysinfo si;
        if (ioctl(fOutFD, OSS_SYSINFO, &si) == -1) {
            jack_error("JackOSSDriver::DisplayDeviceInfo OSS_SYSINFO failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("OSS product %s", si.product);
            jack_info("OSS version %s", si.version);
            jack_info("OSS version num %d", si.versionnum);
            jack_info("OSS numaudios %d", si.numaudios);
            jack_info("OSS numaudioengines %d", si.numaudioengines);
            jack_info("OSS numcards %d", si.numcards);
        }

        jack_info("Output capabilities - %d channels : ", fPlaybackChannels);
        jack_info("Output block size = %d", fOutputBufferSize);

        if (ioctl(fOutFD, SNDCTL_DSP_GETOSPACE, &info) == -1)  {
            jack_error("JackOSSDriver::DisplayDeviceInfo SNDCTL_DSP_GETOSPACE failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("output space info: fragments = %d, fragstotal = %d, fragsize = %d, bytes = %d",
                info.fragments, info.fragstotal, info.fragsize, info.bytes);
        }

        if (ioctl(fOutFD, SNDCTL_DSP_GETCAPS, &cap) == -1)  {
            jack_error("JackOSSDriver::DisplayDeviceInfo SNDCTL_DSP_GETCAPS failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            if (cap & DSP_CAP_DUPLEX) 	jack_info(" DSP_CAP_DUPLEX");
            if (cap & DSP_CAP_REALTIME) jack_info(" DSP_CAP_REALTIME");
            if (cap & DSP_CAP_BATCH) 	jack_info(" DSP_CAP_BATCH");
            if (cap & DSP_CAP_COPROC) 	jack_info(" DSP_CAP_COPROC");
            if (cap & DSP_CAP_TRIGGER)  jack_info(" DSP_CAP_TRIGGER");
            if (cap & DSP_CAP_MMAP) 	jack_info(" DSP_CAP_MMAP");
            if (cap & DSP_CAP_MULTI) 	jack_info(" DSP_CAP_MULTI");
            if (cap & DSP_CAP_BIND) 	jack_info(" DSP_CAP_BIND");
        }
    }

    if (fRWMode & kRead) {

      	oss_sysinfo si;
        if (ioctl(fInFD, OSS_SYSINFO, &si) == -1) {
            jack_error("JackOSSDriver::DisplayDeviceInfo OSS_SYSINFO failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("OSS product %s", si.product);
            jack_info("OSS version %s", si.version);
            jack_info("OSS version num %d", si.versionnum);
            jack_info("OSS numaudios %d", si.numaudios);
            jack_info("OSS numaudioengines %d", si.numaudioengines);
            jack_info("OSS numcards %d", si.numcards);
        }

        jack_info("Input capabilities - %d channels : ", fCaptureChannels);
        jack_info("Input block size = %d", fInputBufferSize);

        if (ioctl(fInFD, SNDCTL_DSP_GETISPACE, &info) == -1) {
            jack_error("JackOSSDriver::DisplayDeviceInfo SNDCTL_DSP_GETOSPACE failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            jack_info("input space info: fragments = %d, fragstotal = %d, fragsize = %d, bytes = %d",
                info.fragments, info.fragstotal, info.fragsize, info.bytes);
        }

        if (ioctl(fInFD, SNDCTL_DSP_GETCAPS, &cap) == -1) {
            jack_error("JackOSSDriver::DisplayDeviceInfo SNDCTL_DSP_GETCAPS failed : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        } else {
            if (cap & DSP_CAP_DUPLEX) 	jack_info(" DSP_CAP_DUPLEX");
            if (cap & DSP_CAP_REALTIME) jack_info(" DSP_CAP_REALTIME");
            if (cap & DSP_CAP_BATCH) 	jack_info(" DSP_CAP_BATCH");
            if (cap & DSP_CAP_COPROC) 	jack_info(" DSP_CAP_COPROC");
            if (cap & DSP_CAP_TRIGGER)  jack_info(" DSP_CAP_TRIGGER");
            if (cap & DSP_CAP_MMAP) 	jack_info(" DSP_CAP_MMAP");
            if (cap & DSP_CAP_MULTI) 	jack_info(" DSP_CAP_MULTI");
            if (cap & DSP_CAP_BIND) 	jack_info(" DSP_CAP_BIND");
        }
    }

    if (ai_in.rate_source != ai_out.rate_source) {
        jack_info("Warning : input and output are not necessarily driven by the same clock!");
    }
}

int JackOSSDriver::OpenInput()
{
    int flags = 0;
    int gFragFormat;
    int cur_capture_channels;
    int cur_sample_format;
    jack_nframes_t cur_sample_rate;

    if (fCaptureChannels == 0) fCaptureChannels = 2;

    if ((fInFD = open(fCaptureDriverName, O_RDONLY | ((fExcl) ? O_EXCL : 0))) < 0) {
        jack_error("JackOSSDriver::OpenInput failed to open device : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        return -1;
    }

    jack_log("JackOSSDriver::OpenInput input fInFD = %d", fInFD);

    if (fExcl) {
        if (ioctl(fInFD, SNDCTL_DSP_COOKEDMODE, &flags) == -1) {
            jack_error("JackOSSDriver::OpenInput failed to set cooked mode : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
    }

    gFragFormat = (2 << 16) + int2pow2(fEngineControl->fBufferSize * fSampleSize * fCaptureChannels);
    if (ioctl(fInFD, SNDCTL_DSP_SETFRAGMENT, &gFragFormat) == -1) {
        jack_error("JackOSSDriver::OpenInput failed to set fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    cur_sample_format = fSampleFormat;
    if (ioctl(fInFD, SNDCTL_DSP_SETFMT, &fSampleFormat) == -1) {
        jack_error("JackOSSDriver::OpenInput failed to set format : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_format != fSampleFormat) {
        jack_info("JackOSSDriver::OpenInput driver forced the sample format %ld", fSampleFormat);
    }

    cur_capture_channels = fCaptureChannels;
    if (ioctl(fInFD, SNDCTL_DSP_CHANNELS, &fCaptureChannels) == -1) {
        jack_error("JackOSSDriver::OpenInput failed to set channels : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_capture_channels != fCaptureChannels) {
        jack_info("JackOSSDriver::OpenInput driver forced the number of capture channels %ld", fCaptureChannels);
    }

    cur_sample_rate = fEngineControl->fSampleRate;
    if (ioctl(fInFD, SNDCTL_DSP_SPEED, &fEngineControl->fSampleRate) == -1) {
        jack_error("JackOSSDriver::OpenInput failed to set sample rate : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_rate != fEngineControl->fSampleRate) {
        jack_info("JackOSSDriver::OpenInput driver forced the sample rate %ld", fEngineControl->fSampleRate);
    }

    fInputBufferSize = 0;
    if (ioctl(fInFD, SNDCTL_DSP_GETBLKSIZE, &fInputBufferSize) == -1) {
        jack_error("JackOSSDriver::OpenInput failed to get fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    if (fInputBufferSize != fEngineControl->fBufferSize * fSampleSize * fCaptureChannels) {
       if (fIgnoreHW) {
           int new_buffer_size = fInputBufferSize / (fSampleSize * fCaptureChannels);
           jack_info("JackOSSDriver::OpenInput driver forced buffer size %ld", new_buffer_size);
           JackAudioDriver::SetBufferSize(new_buffer_size); // never fails
       } else {
           jack_error("JackOSSDriver::OpenInput wanted buffer size cannot be obtained");
           goto error;
       }
    }

    fInputBuffer = (void*)calloc(fInputBufferSize, 1);
    assert(fInputBuffer);
    return 0;

error:
    ::close(fInFD);
    return -1;
}

int JackOSSDriver::OpenOutput()
{
    int flags = 0;
    int gFragFormat;
    int cur_sample_format;
    int cur_playback_channels;
    jack_nframes_t cur_sample_rate;

    if (fPlaybackChannels == 0) fPlaybackChannels = 2;

    if ((fOutFD = open(fPlaybackDriverName, O_WRONLY | ((fExcl) ? O_EXCL : 0))) < 0) {
       jack_error("JackOSSDriver::OpenOutput failed to open device : %s@%i, errno = %d", __FILE__, __LINE__, errno);
       return -1;
    }

    jack_log("JackOSSDriver::OpenOutput output fOutFD = %d", fOutFD);

    if (fExcl) {
        if (ioctl(fOutFD, SNDCTL_DSP_COOKEDMODE, &flags) == -1) {
            jack_error("JackOSSDriver::OpenOutput failed to set cooked mode : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
    }

    gFragFormat = (2 << 16) + int2pow2(fEngineControl->fBufferSize * fSampleSize * fPlaybackChannels);
    if (ioctl(fOutFD, SNDCTL_DSP_SETFRAGMENT, &gFragFormat) == -1) {
        jack_error("JackOSSDriver::OpenOutput failed to set fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    cur_sample_format = fSampleFormat;
    if (ioctl(fOutFD, SNDCTL_DSP_SETFMT, &fSampleFormat) == -1) {
        jack_error("JackOSSDriver::OpenOutput failed to set format : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_format != fSampleFormat) {
        jack_info("JackOSSDriver::OpenOutput driver forced the sample format %ld", fSampleFormat);
    }

    cur_playback_channels = fPlaybackChannels;
    if (ioctl(fOutFD, SNDCTL_DSP_CHANNELS, &fPlaybackChannels) == -1) {
        jack_error("JackOSSDriver::OpenOutput failed to set channels : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_playback_channels != fPlaybackChannels) {
        jack_info("JackOSSDriver::OpenOutput driver forced the number of playback channels %ld", fPlaybackChannels);
    }

    cur_sample_rate = fEngineControl->fSampleRate;
    if (ioctl(fOutFD, SNDCTL_DSP_SPEED, &fEngineControl->fSampleRate) == -1) {
        jack_error("JackOSSDriver::OpenOutput failed to set sample rate : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    if (cur_sample_rate != fEngineControl->fSampleRate) {
        jack_info("JackOSSDriver::OpenInput driver forced the sample rate %ld", fEngineControl->fSampleRate);
    }

    fOutputBufferSize = 0;
    if (ioctl(fOutFD, SNDCTL_DSP_GETBLKSIZE, &fOutputBufferSize) == -1) {
        jack_error("JackOSSDriver::OpenOutput failed to get fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }

    if (fOutputBufferSize != fEngineControl->fBufferSize * fSampleSize * fPlaybackChannels) {
       if (fIgnoreHW) {
           int new_buffer_size = fOutputBufferSize / (fSampleSize * fPlaybackChannels);
           jack_info("JackOSSDriver::OpenOutput driver forced buffer size %ld", new_buffer_size);
           JackAudioDriver::SetBufferSize(new_buffer_size); // never fails
       } else {
           jack_error("JackOSSDriver::OpenInput wanted buffer size cannot be obtained");
           goto error;
       }
    }

    fOutputBuffer = (void*)calloc(fOutputBufferSize, 1);
    fFirstCycle = true;
    assert(fOutputBuffer);
    return 0;

error:
    ::close(fOutFD);
    return -1;
}

int JackOSSDriver::Open(jack_nframes_t nframes,
                      int user_nperiods,
                      jack_nframes_t samplerate,
                      bool capturing,
                      bool playing,
                      int inchannels,
                      int outchannels,
                      bool excl,
                      bool monitor,
                      const char* capture_driver_uid,
                      const char* playback_driver_uid,
                      jack_nframes_t capture_latency,
                      jack_nframes_t playback_latency,
                      int bits,
                      bool ignorehwbuf)
{
    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(nframes, samplerate, capturing, playing, inchannels, outchannels, monitor,
        capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    } else {

        if (!fEngineControl->fSyncMode) {
            jack_error("Cannot run in asynchronous mode, use the -S parameter for jackd");
            return -1;
        }

        fRWMode |= ((capturing) ? kRead : 0);
        fRWMode |= ((playing) ? kWrite : 0);
        fBits = bits;
        fIgnoreHW = ignorehwbuf;
        fNperiods = user_nperiods;
        fExcl = excl;

    #ifdef JACK_MONITOR
        // Force memory page in
        memset(&gCycleTable, 0, sizeof(gCycleTable));
    #endif

        if (OpenAux() < 0) {
            Close();
            return -1;
        } else {
            return 0;
        }
    }
}

int JackOSSDriver::Close()
{
 #ifdef JACK_MONITOR
    FILE* file = fopen("OSSProfiling.log", "w");

    if (file) {
        jack_info("Writing OSS driver timing data....");
        for (int i = 1; i < gCycleCount; i++) {
            int d1 = gCycleTable.fTable[i].fAfterRead - gCycleTable.fTable[i].fBeforeRead;
            int d2 = gCycleTable.fTable[i].fAfterReadConvert - gCycleTable.fTable[i].fAfterRead;
            int d3 = gCycleTable.fTable[i].fAfterWrite - gCycleTable.fTable[i].fBeforeWrite;
            int d4 = gCycleTable.fTable[i].fBeforeWrite - gCycleTable.fTable[i].fBeforeWriteConvert;
            fprintf(file, "%d \t %d \t %d \t %d \t \n", d1, d2, d3, d4);
        }
        fclose(file);
    } else {
        jack_error("JackOSSDriver::Close : cannot open OSSProfiling.log file");
    }

    file = fopen("TimingOSS.plot", "w");

    if (file == NULL) {
        jack_error("JackOSSDriver::Close cannot open TimingOSS.plot file");
    } else {

        fprintf(file, "set grid\n");
        fprintf(file, "set title \"OSS audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"OSSProfiling.log\" using 1 title \"Driver read wait\" with lines, \
                            \"OSSProfiling.log\" using 2 title \"Driver read convert duration\" with lines, \
                            \"OSSProfiling.log\" using 3 title \"Driver write wait\" with lines, \
                            \"OSSProfiling.log\" using 4 title \"Driver write convert duration\" with lines\n");

        fprintf(file, "set output 'TimingOSS.pdf\n");
        fprintf(file, "set terminal pdf\n");

        fprintf(file, "set grid\n");
        fprintf(file, "set title \"OSS audio driver timing\"\n");
        fprintf(file, "set xlabel \"audio cycles\"\n");
        fprintf(file, "set ylabel \"usec\"\n");
        fprintf(file, "plot \"OSSProfiling.log\" using 1 title \"Driver read wait\" with lines, \
                            \"OSSProfiling.log\" using 2 title \"Driver read convert duration\" with lines, \
                            \"OSSProfiling.log\" using 3 title \"Driver write wait\" with lines, \
                            \"OSSProfiling.log\" using 4 title \"Driver write convert duration\" with lines\n");

        fclose(file);
    }
 #endif
    int res = JackAudioDriver::Close();
    CloseAux();
    return res;
}


int JackOSSDriver::OpenAux()
{
    SetSampleFormat();

    if ((fRWMode & kRead) && (OpenInput() < 0)) {
        return -1;
    }

    if ((fRWMode & kWrite) && (OpenOutput() < 0)) {
        return -1;
    }

    // In duplex mode, check that input and output use the same buffer size
    /*

    10/02/09 : desactivated for now, needs more check (only needed when *same* device is used for input and output ??)

    if ((fRWMode & kRead) && (fRWMode & kWrite) && (fInputBufferSize != fOutputBufferSize)) {
       jack_error("JackOSSDriver::OpenAux input and output buffer size are not the same!!");
       return -1;
    }
    */

    DisplayDeviceInfo();
    return 0;
}

void JackOSSDriver::CloseAux()
{
    if (fRWMode & kRead && fInFD > 0) {
        close(fInFD);
        fInFD = -1;
    }

    if (fRWMode & kWrite && fOutFD > 0) {
        close(fOutFD);
        fOutFD = -1;
    }

    if (fInputBuffer)
        free(fInputBuffer);
    fInputBuffer = NULL;

    if (fOutputBuffer)
        free(fOutputBuffer);
    fOutputBuffer = NULL;
}

int JackOSSDriver::Read()
{
    if (fInFD < 0) {
        // Keep begin cycle time
        JackDriver::CycleTakeBeginTime();
        return 0;
    }

    ssize_t count;

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeRead = GetMicroSeconds();
#endif

    audio_errinfo ei_in;
    count = ::read(fInFD, fInputBuffer, fInputBufferSize);

#ifdef JACK_MONITOR
    if (count > 0 && count != (int)fInputBufferSize)
        jack_log("JackOSSDriver::Read count = %ld", count / (fSampleSize * fCaptureChannels));
    gCycleTable.fTable[gCycleCount].fAfterRead = GetMicroSeconds();
#endif

    // XRun detection
    if (ioctl(fInFD, SNDCTL_DSP_GETERROR, &ei_in) == 0) {

        if (ei_in.rec_overruns > 0 ) {
            jack_error("JackOSSDriver::Read overruns");
            jack_time_t cur_time = GetMicroSeconds();
            NotifyXRun(cur_time, float(cur_time - fBeginDateUst));   // Better this value than nothing...
        }

        if (ei_in.rec_errorcount > 0 && ei_in.rec_lasterror != 0) {
            jack_error("%d OSS rec event(s), last=%05d:%d", ei_in.rec_errorcount, ei_in.rec_lasterror, ei_in.rec_errorparm);
        }
    }

    if (count < 0) {
        jack_log("JackOSSDriver::Read error = %s", strerror(errno));
        return -1;
    } else if (count < (int)fInputBufferSize) {
        jack_error("JackOSSDriver::Read error bytes read = %ld", count);
        return -1;
    } else {

        // Keep begin cycle time
        JackDriver::CycleTakeBeginTime();
        for (int i = 0; i < fCaptureChannels; i++) {
            if (fGraphManager->GetConnectionsNum(fCapturePortList[i]) > 0) {
                CopyAndConvertIn(GetInputBuffer(i), fInputBuffer, fEngineControl->fBufferSize, i, fCaptureChannels, fBits);
            }
        }

    #ifdef JACK_MONITOR
        gCycleTable.fTable[gCycleCount].fAfterReadConvert = GetMicroSeconds();
    #endif

        return 0;
    }
}

int JackOSSDriver::Write()
{
    if (fOutFD < 0) {
        // Keep end cycle time
        JackDriver::CycleTakeEndTime();
        return 0;
    }

    ssize_t count;
    audio_errinfo ei_out;

    // Maybe necessary to write an empty output buffer first time : see http://manuals.opensound.com/developer/fulldup.c.html
    if (fFirstCycle) {

        fFirstCycle = false;
        memset(fOutputBuffer, 0, fOutputBufferSize);

        // Prefill ouput buffer
        for (int i = 0; i < fNperiods; i++) {
           count = ::write(fOutFD, fOutputBuffer, fOutputBufferSize);
           if (count < (int)fOutputBufferSize) {
                jack_error("JackOSSDriver::Write error bytes written = %ld", count);
                return -1;
            }
        }

        int delay;
        if (ioctl(fOutFD, SNDCTL_DSP_GETODELAY, &delay) == -1) {
            jack_error("JackOSSDriver::Write error get out delay : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            return -1;
        }

        delay /= fSampleSize * fPlaybackChannels;
        jack_info("JackOSSDriver::Write output latency frames = %ld", delay);
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeWriteConvert = GetMicroSeconds();
#endif

    memset(fOutputBuffer, 0, fOutputBufferSize);
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
            CopyAndConvertOut(fOutputBuffer, GetOutputBuffer(i), fEngineControl->fBufferSize, i, fPlaybackChannels, fBits);
        }
    }

  #ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeWrite = GetMicroSeconds();
  #endif

    // Keep end cycle time
    JackDriver::CycleTakeEndTime();
    count = ::write(fOutFD, fOutputBuffer, fOutputBufferSize);

  #ifdef JACK_MONITOR
    if (count > 0 && count != (int)fOutputBufferSize)
        jack_log("JackOSSDriver::Write count = %ld", count / (fSampleSize * fPlaybackChannels));
    gCycleTable.fTable[gCycleCount].fAfterWrite = GetMicroSeconds();
    gCycleCount = (gCycleCount == CYCLE_POINTS - 1) ? gCycleCount: gCycleCount + 1;
  #endif

    // XRun detection
    if (ioctl(fOutFD, SNDCTL_DSP_GETERROR, &ei_out) == 0) {

        if (ei_out.play_underruns > 0) {
            jack_error("JackOSSDriver::Write underruns");
            jack_time_t cur_time = GetMicroSeconds();
            NotifyXRun(cur_time, float(cur_time - fBeginDateUst));   // Better this value than nothing...
        }

        if (ei_out.play_errorcount > 0 && ei_out.play_lasterror != 0) {
            jack_error("%d OSS play event(s), last=%05d:%d",ei_out.play_errorcount, ei_out.play_lasterror, ei_out.play_errorparm);
        }
    }

    if (count < 0) {
        jack_log("JackOSSDriver::Write error = %s", strerror(errno));
        return -1;
    } else if (count < (int)fOutputBufferSize) {
        jack_error("JackOSSDriver::Write error bytes written = %ld", count);
        return -1;
    } else {
        return 0;
    }
}

int JackOSSDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    CloseAux();
    JackAudioDriver::SetBufferSize(buffer_size); // Generic change, never fails
    return OpenAux();
}

int JackOSSDriver::ProcessSync()
{
    // Read input buffers for the current cycle
    if (Read() < 0) {
        jack_error("ProcessSync: read error, skip cycle");
        return 0;   // Non fatal error here, skip cycle, but continue processing...
    }

    if (fIsMaster) {
        ProcessGraphSync();
    } else {
        ResumeRefNum();
    }

    // Write output buffers for the current cycle
    if (Write() < 0) {
        jack_error("JackAudioDriver::ProcessSync: write error, skip cycle");
        return 0;   // Non fatal error here, skip cycle, but continue processing...
    }

    return 0;
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

SERVER_EXPORT jack_driver_desc_t* driver_get_descriptor()
{
    jack_driver_desc_t * desc;
    jack_driver_desc_filler_t filler;
    jack_driver_param_value_t value;

    desc = jack_driver_descriptor_construct("oss", JackDriverMaster, "OSS API based audio backend", &filler);

    value.ui = OSS_DRIVER_DEF_FS;
    jack_driver_descriptor_add_parameter(desc, &filler, "rate", 'r', JackDriverParamUInt, &value, NULL, "Sample rate", NULL);

    value.ui = OSS_DRIVER_DEF_BLKSIZE;
    jack_driver_descriptor_add_parameter(desc, &filler, "period", 'p', JackDriverParamUInt, &value, NULL, "Frames per period", NULL);

    value.ui = OSS_DRIVER_DEF_NPERIODS;
    jack_driver_descriptor_add_parameter(desc, &filler, "nperiods", 'n', JackDriverParamUInt, &value, NULL, "Number of periods to prefill output buffer", NULL);

    value.i = OSS_DRIVER_DEF_BITS;
    jack_driver_descriptor_add_parameter(desc, &filler, "wordlength", 'w', JackDriverParamInt, &value, NULL, "Word length", NULL);

    value.ui = OSS_DRIVER_DEF_INS;
    jack_driver_descriptor_add_parameter(desc, &filler, "inchannels", 'i', JackDriverParamUInt, &value, NULL, "Capture channels", NULL);

    value.ui = OSS_DRIVER_DEF_OUTS;
    jack_driver_descriptor_add_parameter(desc, &filler, "outchannels", 'o', JackDriverParamUInt, &value, NULL, "Playback channels", NULL);

    value.i = false;
    jack_driver_descriptor_add_parameter(desc, &filler, "excl", 'e', JackDriverParamBool, &value, NULL, "Exclusif (O_EXCL) access mode", NULL);

    strcpy(value.str, OSS_DRIVER_DEF_DEV);
    jack_driver_descriptor_add_parameter(desc, &filler, "capture", 'C', JackDriverParamString, &value, NULL, "Input device", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "playback", 'P', JackDriverParamString, &value, NULL, "Output device", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "device", 'd', JackDriverParamString, &value, NULL, "OSS device name", NULL);

    value.i = false;
    jack_driver_descriptor_add_parameter(desc, &filler, "ignorehwbuf", 'b', JackDriverParamBool, &value, NULL, "Ignore hardware period size", NULL);

    value.ui = 0;
    jack_driver_descriptor_add_parameter(desc, &filler, "input-latency", 'I', JackDriverParamUInt, &value, NULL, "Extra input latency", NULL);
    jack_driver_descriptor_add_parameter(desc, &filler, "output-latency", 'O', JackDriverParamUInt, &value, NULL, "Extra output latency", NULL);

    return desc;
}

SERVER_EXPORT Jack::JackDriverClientInterface* driver_initialize(Jack::JackLockedEngine* engine, Jack::JackSynchro* table, const JSList* params)
{
    int bits = OSS_DRIVER_DEF_BITS;
    jack_nframes_t srate = OSS_DRIVER_DEF_FS;
    jack_nframes_t frames_per_interrupt = OSS_DRIVER_DEF_BLKSIZE;
    const char* capture_pcm_name = OSS_DRIVER_DEF_DEV;
    const char* playback_pcm_name = OSS_DRIVER_DEF_DEV;
    bool capture = false;
    bool playback = false;
    int chan_in = 0;
    int chan_out = 0;
    bool monitor = false;
    bool excl = false;
    unsigned int nperiods = OSS_DRIVER_DEF_NPERIODS;
    const JSList *node;
    const jack_driver_param_t *param;
    bool ignorehwbuf = false;
    jack_nframes_t systemic_input_latency = 0;
    jack_nframes_t systemic_output_latency = 0;

    for (node = params; node; node = jack_slist_next(node)) {

        param = (const jack_driver_param_t *)node->data;

        switch (param->character) {

        case 'r':
            srate = param->value.ui;
            break;

        case 'p':
            frames_per_interrupt = (unsigned int)param->value.ui;
            break;

        case 'n':
            nperiods = (unsigned int)param->value.ui;
            break;

        case 'w':
            bits = param->value.i;
            break;

        case 'i':
            chan_in = (int)param->value.ui;
            break;

        case 'o':
            chan_out = (int)param->value.ui;
            break;

        case 'C':
            capture = true;
            if (strcmp(param->value.str, "none") != 0) {
                capture_pcm_name = param->value.str;
            }
            break;

        case 'P':
            playback = true;
            if (strcmp(param->value.str, "none") != 0) {
                playback_pcm_name = param->value.str;
            }
            break;

        case 'd':
            playback_pcm_name = param->value.str;
            capture_pcm_name = param->value.str;
            break;

        case 'b':
            ignorehwbuf = true;
            break;

        case 'e':
            excl = true;
            break;

        case 'I':
            systemic_input_latency = param->value.ui;
            break;

        case 'O':
            systemic_output_latency = param->value.ui;
            break;
        }
    }

    // duplex is the default
    if (!capture && !playback) {
        capture = true;
        playback = true;
    }

    Jack::JackOSSDriver* oss_driver = new Jack::JackOSSDriver("system", "oss", engine, table);
    Jack::JackDriverClientInterface* threaded_driver = new Jack::JackThreadedDriver(oss_driver);

    // Special open for OSS driver...
    if (oss_driver->Open(frames_per_interrupt, nperiods, srate, capture, playback, chan_in, chan_out,
        excl, monitor, capture_pcm_name, playback_pcm_name, systemic_input_latency, systemic_output_latency, bits, ignorehwbuf) == 0) {
        return threaded_driver;
    } else {
        delete threaded_driver; // Delete the decorated driver
        return NULL;
    }
}

#ifdef __cplusplus
}
#endif
