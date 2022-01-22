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

namespace
{

inline jack_nframes_t TimeToFrames(jack_time_t time, jack_nframes_t sample_rate) {
    return ((time * sample_rate) + 500000ULL) / 1000000ULL;
}

inline long long TimeToOffset(jack_time_t time1, jack_time_t time2, jack_nframes_t sample_rate)
{
    if (time2 > time1) {
        return TimeToFrames(time2 - time1, sample_rate);
    } else {
        return 0LL - TimeToFrames(time1 - time2, sample_rate);
    }
}

inline jack_time_t FramesToTime(jack_nframes_t frames, jack_nframes_t sample_rate) {
    return ((frames * 1000000ULL) + (sample_rate / 2ULL)) / sample_rate;
}

inline jack_nframes_t RoundUp(jack_nframes_t frames, jack_nframes_t block) {
    if (block > 0) {
        frames += (block - 1);
        frames -= (frames % block);
    }
    return frames;
}

inline jack_time_t RoundDown(jack_time_t time, jack_time_t interval) {
    if (interval > 0) {
        time -= (time % interval);
    }
    return time;
}

int GetSampleFormat(int bits)
{
    switch(bits) {
        // Native-endian signed 32 bit samples.
        case 32:
            return AFMT_S32_NE;
        // Native-endian signed 24 bit (packed) samples.
        case 24:
            return AFMT_S24_NE;
        // Native-endian signed 16 bit samples, used by default.
        case 16:
        default:
            return AFMT_S16_NE;
    }
}

unsigned int GetSampleSize(int format)
{
    switch(format) {
        // Native-endian signed 32 bit samples.
        case AFMT_S32_NE:
            return 4;
        // Native-endian signed 24 bit (packed) samples.
        case AFMT_S24_NE:
            return 3;
        // Native-endian signed 16 bit samples.
        case AFMT_S16_NE:
            return 2;
        // Unsupported sample format.
        default:
            return 0;
    }
}

inline int UpToPower2(int x)
{
    int r = 0;
    while ((1 << r) < x)
        r++;
    return r;
}

}

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
            char *s24src = (char*)src;
            s24src += channel * 3;
            sample_move_dS_s24(dst, s24src, nframes, chcount*3);
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
            char *s24dst = (char*)dst;
            s24dst += channel * 3;
            sample_move_d24_sS(s24dst, src, nframes, chcount*3, NULL);
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

void JackOSSDriver::DisplayDeviceInfo()
{
    audio_buf_info info;
    memset(&info, 0, sizeof(audio_buf_info));
    int cap = 0;

    // Duplex cards : http://manuals.opensound.com/developer/full_duplex.html
    jack_info("Audio Interface Description :");
    jack_info("Sampling Frequency : %d, Sample Size : %d", fEngineControl->fSampleRate, fInSampleSize * 8);

    if (fPlayback) {

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
            if (cap & DSP_CAP_DUPLEX)   jack_info(" DSP_CAP_DUPLEX");
            if (cap & DSP_CAP_REALTIME) jack_info(" DSP_CAP_REALTIME");
            if (cap & DSP_CAP_BATCH)    jack_info(" DSP_CAP_BATCH");
            if (cap & DSP_CAP_COPROC)   jack_info(" DSP_CAP_COPROC");
            if (cap & DSP_CAP_TRIGGER)  jack_info(" DSP_CAP_TRIGGER");
            if (cap & DSP_CAP_MMAP)     jack_info(" DSP_CAP_MMAP");
            if (cap & DSP_CAP_MULTI)    jack_info(" DSP_CAP_MULTI");
            if (cap & DSP_CAP_BIND)     jack_info(" DSP_CAP_BIND");
        }
    }

    if (fCapture) {

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
            if (cap & DSP_CAP_DUPLEX)   jack_info(" DSP_CAP_DUPLEX");
            if (cap & DSP_CAP_REALTIME) jack_info(" DSP_CAP_REALTIME");
            if (cap & DSP_CAP_BATCH)    jack_info(" DSP_CAP_BATCH");
            if (cap & DSP_CAP_COPROC)   jack_info(" DSP_CAP_COPROC");
            if (cap & DSP_CAP_TRIGGER)  jack_info(" DSP_CAP_TRIGGER");
            if (cap & DSP_CAP_MMAP)     jack_info(" DSP_CAP_MMAP");
            if (cap & DSP_CAP_MULTI)    jack_info(" DSP_CAP_MULTI");
            if (cap & DSP_CAP_BIND)     jack_info(" DSP_CAP_BIND");
        }
    }
}

int JackOSSDriver::ProbeInBlockSize()
{
    jack_nframes_t blocks[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int probes = 0;
    int ret = 0;
    // Default values in case of an error.
    fInMeanStep = fEngineControl->fBufferSize;
    fInBlockSize = 1;

    if (fInFD > 0) {
        // Read one frame into a new hardware block so we can check its size.
        // Repeat that for multiple probes, sometimes the first reads differ.
        jack_nframes_t frames = 1;
        for (int p = 0; p < 8 && frames > 0; ++p) {
            ret = Discard(frames);
            frames = 0;
            if (ret == 0) {
                oss_count_t ptr;
                if (ioctl(fInFD, SNDCTL_DSP_CURRENT_IPTR, &ptr) == 0 && ptr.fifo_samples > 0) {
                    // Success, store probed hardware block size for later.
                    blocks[p] = 1U + ptr.fifo_samples;
                    ++probes;
                    // Proceed by reading one frame into the next hardware block.
                    frames = blocks[p];
                }
            } else {
                // Read error - abort.
                jack_error("JackOSSDriver::ProbeInBlockSize read failed with %d", ret);
            }
        }

        // Stop recording.
        ioctl(fInFD, SNDCTL_DSP_HALT_INPUT, NULL);
    }

    if (probes == 8) {
        // Compute mean block size of the last six probes.
        jack_nframes_t sum = 0;
        for (int p = 2; p < 8; ++p) {
            jack_log("JackOSSDriver::ProbeInBlockSize read block of %d frames", blocks[p]);
            sum += blocks[p];
        }
        fInMeanStep = sum / 6;

        // Check that none of the probed block sizes deviates too much.
        jack_nframes_t slack = fInMeanStep / 16;
        bool strict = true;
        for (int p = 2; p < 8; ++p) {
            strict = strict && (blocks[p] > fInMeanStep - slack) && (blocks[p] < fInMeanStep + slack);
        }

        if (strict && fInMeanStep <= fEngineControl->fBufferSize) {
            // Regular hardware block size, use it for rounding.
            jack_info("JackOSSDriver::ProbeInBlockSize read blocks are %d frames", fInMeanStep);
            fInBlockSize = fInMeanStep;
        } else {
            jack_info("JackOSSDriver::ProbeInBlockSize irregular read block sizes");
            jack_info("JackOSSDriver::ProbeInBlockSize mean read block was %d frames", fInMeanStep);
        }

        if (fInBlockSize > fEngineControl->fBufferSize / 2) {
            jack_info("JackOSSDriver::ProbeInBlockSize less than two read blocks per cycle");
            jack_info("JackOSSDriver::ProbeInBlockSize for best results make period a multiple of %d", fInBlockSize);
        }

        if (fInMeanStep > fEngineControl->fBufferSize) {
            jack_error("JackOSSDriver::ProbeInBlockSize period is too small, minimum is %d frames", fInMeanStep);
            return -1;
        }
    }

    return ret;
}

int JackOSSDriver::ProbeOutBlockSize()
{
    jack_nframes_t blocks[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int probes = 0;
    int ret = 0;
    // Default values in case of an error.
    fOutMeanStep = fEngineControl->fBufferSize;
    fOutBlockSize = 1;

    if (fOutFD) {
        // Write one frame over the low water mark, then check the consumed block size.
        // Repeat that for multiple probes, sometimes the initial ones differ.
        jack_nframes_t mark = fNperiods * fEngineControl->fBufferSize;
        WriteSilence(mark + 1);
        for (int p = 0; p < 8 && ret >= 0; ++p) {
            pollfd poll_fd;
            poll_fd.fd = fOutFD;
            poll_fd.events = POLLOUT;
            ret = poll(&poll_fd, 1, 500);
            if (ret < 0) {
                jack_error("JackOSSDriver::ProbeOutBlockSize poll failed with %d", ret);
                break;
            }
            if (poll_fd.revents & POLLOUT) {
                oss_count_t ptr;
                if (ioctl(fOutFD, SNDCTL_DSP_CURRENT_OPTR, &ptr) != -1 && ptr.fifo_samples >= 0) {
                    // Success, store probed hardware block size for later.
                    blocks[p] = mark + 1 - ptr.fifo_samples;
                    ++probes;
                    // Proceed by writing one frame over the low water mark.
                    WriteSilence(blocks[p]);
                }
                poll_fd.revents = 0;
            }
        }

        // Stop playback.
        ioctl(fOutFD, SNDCTL_DSP_HALT_INPUT, NULL);
    }

    if (probes == 8) {
        // Compute mean and maximum block size of the last six probes.
        jack_nframes_t sum = 0;
        for (int p = 2; p < 8; ++p) {
            jack_log("JackOSSDriver::ProbeOutBlockSize write block of %d frames", blocks[p]);
            sum += blocks[p];
        }
        fOutMeanStep = sum / 6;

        // Check that none of the probed block sizes deviates too much.
        jack_nframes_t slack = fOutMeanStep / 16;
        bool strict = true;
        for (int p = 2; p < 8; ++p) {
            strict = strict && (blocks[p] > fOutMeanStep - slack) && (blocks[p] < fOutMeanStep + slack);
        }

        if (strict && fOutMeanStep <= fEngineControl->fBufferSize) {
            // Regular hardware block size, use it for rounding.
            jack_info("JackOSSDriver::ProbeOutBlockSize write blocks are %d frames", fOutMeanStep);
            fOutBlockSize = fOutMeanStep;
        } else {
            jack_info("JackOSSDriver::ProbeOutBlockSize irregular write block sizes");
            jack_info("JackOSSDriver::ProbeOutBlockSize mean write block was %d frames", fOutMeanStep);
        }

        if (fOutBlockSize > fEngineControl->fBufferSize / 2) {
            jack_info("JackOSSDriver::ProbeOutBlockSize less than two write blocks per cycle");
            jack_info("JackOSSDriver::ProbeOutBlockSize for best results make period a multiple of %d", fOutBlockSize);
        }

        if (fOutMeanStep > fEngineControl->fBufferSize) {
            jack_error("JackOSSDriver::ProbeOutBlockSize period is too small, minimum is %d frames", fOutMeanStep);
            return -1;
        }
    }

    return ret;
}

int JackOSSDriver::Discard(jack_nframes_t frames)
{
    if (fInFD < 0) {
        return -1;
    }

    // Read frames from OSS capture buffer to be discarded.
    ssize_t size = frames * fInSampleSize * fCaptureChannels;
    while (size > 0) {
        ssize_t chunk = (size > fInputBufferSize) ? fInputBufferSize : size;
        ssize_t count = ::read(fInFD, fInputBuffer, chunk);
        if (count <= 0) {
            jack_error("JackOSSDriver::Discard error bytes read = %ld", count);
            return -1;
        }
        fOSSReadOffset += count / (fInSampleSize * fCaptureChannels);
        size -= count;
    }
    return 0;
}

int JackOSSDriver::WriteSilence(jack_nframes_t frames)
{
    if (fOutFD < 0) {
        return -1;
    }

    // Fill OSS playback buffer, write some periods of silence.
    memset(fOutputBuffer, 0, fOutputBufferSize);
    ssize_t size = frames * fOutSampleSize * fPlaybackChannels;
    while (size > 0) {
        ssize_t chunk = (size > fOutputBufferSize) ? fOutputBufferSize : size;
        ssize_t count = ::write(fOutFD, fOutputBuffer, chunk);
        if (count <= 0) {
            jack_error("JackOSSDriver::WriteSilence error bytes written = %ld", count);
            return -1;
        }
        fOSSWriteOffset += (count / (fOutSampleSize * fPlaybackChannels));
        size -= count;
    }
    return 0;
}

int JackOSSDriver::WaitAndSync()
{
    oss_count_t ptr = {0, 0, {0}};
    if (fInFD > 0 && fOSSReadSync != 0) {
        // Predict time of next capture sync (poll() return).
        if (fOSSReadOffset + fEngineControl->fBufferSize > 0) {
            jack_nframes_t frames = fOSSReadOffset + fEngineControl->fBufferSize;
            jack_nframes_t rounded = RoundUp(frames, fInBlockSize);
            fOSSReadSync += FramesToTime(rounded, fEngineControl->fSampleRate);
            fOSSReadOffset -= rounded;
        }
    }
    if (fOutFD > 0 && fOSSWriteSync != 0) {
        // Predict time of next playback sync (poll() return).
        if (fOSSWriteOffset > fNperiods * fEngineControl->fBufferSize) {
            jack_nframes_t frames = fOSSWriteOffset - fNperiods * fEngineControl->fBufferSize;
            jack_nframes_t rounded = RoundUp(frames, fOutBlockSize);
            fOSSWriteSync += FramesToTime(rounded, fEngineControl->fSampleRate);
            fOSSWriteOffset -= rounded;
        }
    }
    jack_time_t poll_start = GetMicroSeconds();
    // Poll until recording and playback buffer are ready for this cycle.
    pollfd poll_fd[2];
    poll_fd[0].fd = fInFD;
    if (fInFD > 0 && (fForceSync || poll_start < fOSSReadSync)) {
        poll_fd[0].events = POLLIN;
    } else {
        poll_fd[0].events = 0;
    }
    poll_fd[1].fd = fOutFD;
    if (fOutFD > 0 && (fForceSync || poll_start < fOSSWriteSync)) {
        poll_fd[1].events = POLLOUT;
    } else {
        poll_fd[1].events = 0;
    }
    while (poll_fd[0].events != 0 || poll_fd[1].events != 0) {
        poll_fd[0].revents = 0;
        poll_fd[1].revents = 0;
        int ret = poll(poll_fd, 2, 500);
        jack_time_t now = GetMicroSeconds();
        if (ret <= 0) {
            jack_error("JackOSSDriver::WaitAndSync poll failed with %d after %ld us", ret, now - poll_start);
            return ret;
        }
        if (poll_fd[0].revents & POLLIN) {
            // Check the excess recording frames.
            if (ioctl(fInFD, SNDCTL_DSP_CURRENT_IPTR, &ptr) != -1 && ptr.fifo_samples >= 0) {
                if (fInBlockSize <= 1) {
                    // Irregular block size, let sync time converge slowly when late.
                    fOSSReadSync = min(fOSSReadSync, now) / 2 + now / 2;
                    fOSSReadOffset = -ptr.fifo_samples;
                } else if (ptr.fifo_samples - fEngineControl->fBufferSize >= fInBlockSize) {
                    // Too late for a reliable sync, make sure sync time is not in the future.
                    if (now < fOSSReadSync) {
                        fOSSReadOffset = -ptr.fifo_samples;
                        jack_info("JackOSSDriver::WaitAndSync capture sync %ld us early, %ld frames", fOSSReadSync - now, fOSSReadOffset);
                        fOSSReadSync = now;
                    }
                } else if (fForceSync) {
                    // Uncertain previous sync, just use sync time directly.
                    fOSSReadSync = now;
                    fOSSReadOffset = -ptr.fifo_samples;
                } else {
                    // Adapt expected sync time when early or late - in whole block intervals.
                    // Account for some speed drift, but otherwise round down to earlier interval.
                    jack_time_t interval = FramesToTime(fInBlockSize, fEngineControl->fSampleRate);
                    jack_time_t remainder = fOSSReadSync % interval;
                    jack_time_t max_drift = interval / 4;
                    jack_time_t rounded = RoundDown((now - remainder) + max_drift, interval) + remainder;
                    // Let sync time converge slowly when late, prefer earlier sync times.
                    fOSSReadSync = min(rounded, now) / 2 + now / 2;
                    fOSSReadOffset = -ptr.fifo_samples;
                }
            }
            poll_fd[0].events = 0;
        }
        if (poll_fd[1].revents & POLLOUT) {
            // Check the remaining playback frames.
            if (ioctl(fOutFD, SNDCTL_DSP_CURRENT_OPTR, &ptr) != -1 && ptr.fifo_samples >= 0) {
                if (fOutBlockSize <= 1) {
                    // Irregular block size, let sync time converge slowly when late.
                    fOSSWriteSync = min(fOSSWriteSync, now) / 2 + now / 2;
                    fOSSWriteOffset = ptr.fifo_samples;
                } else if (ptr.fifo_samples + fOutBlockSize <= fNperiods * fEngineControl->fBufferSize) {
                    // Too late for a reliable sync, make sure sync time is not in the future.
                    if (now < fOSSWriteSync) {
                        fOSSWriteOffset = ptr.fifo_samples;
                        jack_info("JackOSSDriver::WaitAndSync playback sync %ld us early, %ld frames", fOSSWriteSync - now, fOSSWriteOffset);
                        fOSSWriteSync = now;
                    }
                } else if (fForceSync) {
                    // Uncertain previous sync, just use sync time directly.
                    fOSSWriteSync = now;
                    fOSSWriteOffset = ptr.fifo_samples;
                } else {
                    // Adapt expected sync time when early or late - in whole block intervals.
                    // Account for some speed drift, but otherwise round down to earlier interval.
                    jack_time_t interval = FramesToTime(fOutBlockSize, fEngineControl->fSampleRate);
                    jack_time_t remainder = fOSSWriteSync % interval;
                    jack_time_t max_drift = interval / 4;
                    jack_time_t rounded = RoundDown((now - remainder) + max_drift, interval) + remainder;
                    // Let sync time converge slowly when late, prefer earlier sync times.
                    fOSSWriteSync = min(rounded, now) / 2 + now / 2;
                    fOSSWriteOffset = ptr.fifo_samples;
                }
            }
            poll_fd[1].events = 0;
        }
    }

    fForceSync = false;

    // Compute balance of read and write buffers combined.
    fBufferBalance = 0;
    if (fInFD > 0 && fOutFD > 0) {
        // Compare actual buffer content with target of (1 + n) * period.
        fBufferBalance += ((1 + fNperiods) * fEngineControl->fBufferSize);
        fBufferBalance -= (fOSSWriteOffset - fOSSReadOffset);
        fBufferBalance += TimeToOffset(fOSSWriteSync, fOSSReadSync, fEngineControl->fSampleRate);

        // Force balancing if sync times deviate too much.
        jack_time_t slack = FramesToTime((fEngineControl->fBufferSize * 2) / 3, fEngineControl->fSampleRate);
        fForceBalancing = fForceBalancing || (fOSSReadSync > fOSSWriteSync + slack);
        fForceBalancing = fForceBalancing || (fOSSWriteSync > fOSSReadSync + slack);
        // Force balancing if buffer is badly balanced.
        fForceBalancing = fForceBalancing || (abs(fBufferBalance) > max(fInMeanStep, fOutMeanStep));
    }

    // Print debug info every 10 seconds.
    if (ptr.samples > 0 && (ptr.samples % (10 * fEngineControl->fSampleRate)) < fEngineControl->fBufferSize) {
        jack_log("JackOSSDriver::Read buffer balance is %ld frames", fBufferBalance);
        jack_time_t now = GetMicroSeconds();
        jack_log("JackOSSDriver::Read recording sync %ld frames %ld us ago", fOSSReadOffset, now - fOSSReadSync);
        jack_log("JackOSSDriver::Read playback sync %ld frames %ld us ago", fOSSWriteOffset, now - fOSSWriteSync);
    }

    return 0;
}

int JackOSSDriver::OpenInput()
{
    int flags = 0;
    int gFragFormat;
    int cur_capture_channels;
    int cur_sample_format;
    jack_nframes_t cur_sample_rate;
    audio_buf_info info;

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

    cur_sample_format = GetSampleFormat(fBits);
    if (ioctl(fInFD, SNDCTL_DSP_SETFMT, &cur_sample_format) == -1) {
        jack_error("JackOSSDriver::OpenInput failed to set format : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    fInSampleSize = GetSampleSize(cur_sample_format);
    if (cur_sample_format != GetSampleFormat(fBits)) {
        if (fInSampleSize > 0) {
            jack_info("JackOSSDriver::OpenInput driver forced %d bit sample format", fInSampleSize * 8);
        } else {
            jack_error("JackOSSDriver::OpenInput unsupported sample format %#x", cur_sample_format);
            goto error;
        }
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

    // Internal buffer size required for one period.
    fInputBufferSize = fEngineControl->fBufferSize * fInSampleSize * fCaptureChannels;

    // Get the total size of the OSS recording buffer, in sample frames.
    info = {0, 0, 0, 0};
    if (ioctl(fInFD, SNDCTL_DSP_GETISPACE, &info) == -1 || info.fragsize <= 0 || info.fragstotal <= 0) {
        jack_error("JackOSSDriver::OpenInput failed to get buffer info : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    fOSSInBuffer = info.fragstotal * info.fragsize / (fInSampleSize * fCaptureChannels);

    if (fOSSInBuffer < fEngineControl->fBufferSize * (1 + fNperiods)) {
        // Total size of the OSS recording buffer is too small, resize it.
        unsigned int buf_size = fInputBufferSize * (1 + fNperiods);
        // Keep current fragment size if possible - respect OSS latency settings.
        gFragFormat = UpToPower2(info.fragsize);
        unsigned int frag_size = 1U << gFragFormat;
        gFragFormat |= ((buf_size + frag_size - 1) / frag_size) << 16;
        jack_info("JackOSSDriver::OpenInput request %d fragments of %d", (gFragFormat >> 16), frag_size);
        if (ioctl(fInFD, SNDCTL_DSP_SETFRAGMENT, &gFragFormat) == -1) {
            jack_error("JackOSSDriver::OpenInput failed to set fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
        // Check the new OSS recording buffer size.
        info = {0, 0, 0, 0};
        if (ioctl(fInFD, SNDCTL_DSP_GETISPACE, &info) == -1 || info.fragsize <= 0 || info.fragstotal <= 0) {
            jack_error("JackOSSDriver::OpenInput failed to get buffer info : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
        fOSSInBuffer = info.fragstotal * info.fragsize / (fInSampleSize * fCaptureChannels);
    }

    if (fOSSInBuffer > fEngineControl->fBufferSize) {
        int mark = fInputBufferSize;
        if (ioctl(fInFD, SNDCTL_DSP_LOW_WATER, &mark) != 0) {
            jack_error("JackOSSDriver::OpenInput failed to set low water mark : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
        jack_info("JackOSSDriver::OpenInput set low water mark to %d", mark);
    }

    fInputBuffer = (void*)calloc(fInputBufferSize, 1);
    assert(fInputBuffer);

    if (ProbeInBlockSize() < 0) {
      goto error;
    }

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
    audio_buf_info info;

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

    cur_sample_format = GetSampleFormat(fBits);
    if (ioctl(fOutFD, SNDCTL_DSP_SETFMT, &cur_sample_format) == -1) {
        jack_error("JackOSSDriver::OpenOutput failed to set format : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    fOutSampleSize = GetSampleSize(cur_sample_format);
    if (cur_sample_format != GetSampleFormat(fBits)) {
        if (fOutSampleSize > 0) {
            jack_info("JackOSSDriver::OpenOutput driver forced %d bit sample format", fOutSampleSize * 8);
        } else {
            jack_error("JackOSSDriver::OpenOutput unsupported sample format %#x", cur_sample_format);
            goto error;
        }
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

    // Internal buffer size required for one period.
    fOutputBufferSize = fEngineControl->fBufferSize * fOutSampleSize * fPlaybackChannels;

    // Get the total size of the OSS playback buffer, in sample frames.
    info = {0, 0, 0, 0};
    if (ioctl(fOutFD, SNDCTL_DSP_GETOSPACE, &info) == -1 || info.fragsize <= 0 || info.fragstotal <= 0) {
        jack_error("JackOSSDriver::OpenOutput failed to get buffer info : %s@%i, errno = %d", __FILE__, __LINE__, errno);
        goto error;
    }
    fOSSOutBuffer = info.fragstotal * info.fragsize / (fOutSampleSize * fPlaybackChannels);

    if (fOSSOutBuffer < fEngineControl->fBufferSize * (1 + fNperiods)) {
        // Total size of the OSS playback buffer is too small, resize it.
        unsigned int buf_size = fOutputBufferSize * (1 + fNperiods);
        // Keep current fragment size if possible - respect OSS latency settings.
        // Some sound cards like Intel HDA may stutter when changing the fragment size.
        gFragFormat = UpToPower2(info.fragsize);
        unsigned int frag_size = 1U << gFragFormat;
        gFragFormat |= ((buf_size + frag_size - 1) / frag_size) << 16;
        jack_info("JackOSSDriver::OpenOutput request %d fragments of %d", (gFragFormat >> 16), frag_size);
        if (ioctl(fOutFD, SNDCTL_DSP_SETFRAGMENT, &gFragFormat) == -1) {
            jack_error("JackOSSDriver::OpenOutput failed to set fragments : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
        // Check the new OSS playback buffer size.
        info = {0, 0, 0, 0};
        if (ioctl(fOutFD, SNDCTL_DSP_GETOSPACE, &info) == -1 || info.fragsize <= 0 || info.fragstotal <= 0) {
            jack_error("JackOSSDriver::OpenOutput failed to get buffer info : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
        fOSSOutBuffer = info.fragstotal * info.fragsize / (fOutSampleSize * fPlaybackChannels);
    }

    if (fOSSOutBuffer > fEngineControl->fBufferSize * fNperiods) {
        jack_nframes_t low = fOSSOutBuffer - (fNperiods * fEngineControl->fBufferSize);
        int mark = low * fOutSampleSize * fPlaybackChannels;
        if (ioctl(fOutFD, SNDCTL_DSP_LOW_WATER, &mark) != 0) {
            jack_error("JackOSSDriver::OpenOutput failed to set low water mark : %s@%i, errno = %d", __FILE__, __LINE__, errno);
            goto error;
        }
        jack_info("JackOSSDriver::OpenOutput set low water mark to %d", mark);
    }

    fOutputBuffer = (void*)calloc(fOutputBufferSize, 1);
    assert(fOutputBuffer);

    if (ProbeOutBlockSize() < 0) {
      goto error;
    }

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
    // Store local settings first.
    fCapture = capturing;
    fPlayback = playing;
    fBits = bits;
    fIgnoreHW = ignorehwbuf;
    fNperiods = user_nperiods;
    fExcl = excl;
    fExtraCaptureLatency = capture_latency;
    fExtraPlaybackLatency = playback_latency;

    // Additional playback latency introduced by the OSS buffer. The extra hardware
    // latency given by the user should then be symmetric as reported by jack_iodelay.
    playback_latency += user_nperiods * nframes;
    // Generic JackAudioDriver Open
    if (JackAudioDriver::Open(nframes, samplerate, capturing, playing, inchannels, outchannels, monitor,
        capture_driver_uid, playback_driver_uid, capture_latency, playback_latency) != 0) {
        return -1;
    } else {

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
    // (Re-)Initialize runtime variables.
    fInSampleSize = fOutSampleSize = 0;
    fInputBufferSize = fOutputBufferSize = 0;
    fInBlockSize = fOutBlockSize = 1;
    fInMeanStep = fOutMeanStep = 0;
    fOSSInBuffer = fOSSOutBuffer = 0;
    fOSSReadSync = fOSSWriteSync = 0;
    fOSSReadOffset = fOSSWriteOffset = 0;
    fBufferBalance = 0;
    fForceBalancing = false;
    fForceSync = false;

    if (fCapture && (OpenInput() < 0)) {
        return -1;
    }

    if (fPlayback && (OpenOutput() < 0)) {
        return -1;
    }

    DisplayDeviceInfo();
    return 0;
}

void JackOSSDriver::CloseAux()
{
    if (fCapture && fInFD > 0) {
        close(fInFD);
        fInFD = -1;
    }

    if (fPlayback && fOutFD > 0) {
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
    if (fInFD > 0 && fOSSReadSync == 0) {
        // First cycle, account for leftover samples from previous reads.
        fOSSReadOffset = 0;
        oss_count_t ptr;
        if (ioctl(fInFD, SNDCTL_DSP_CURRENT_IPTR, &ptr) == 0 && ptr.fifo_samples > 0) {
            jack_log("JackOSSDriver::Read pre recording samples = %ld, fifo_samples = %d", ptr.samples, ptr.fifo_samples);
            fOSSReadOffset = -ptr.fifo_samples;
        }

        // Start capture by reading a new hardware block.,
        jack_nframes_t discard =  fInMeanStep - fOSSReadOffset;
        // Let half a block or at most 1ms remain in buffer, avoid drift issues at start.
        discard -= min(TimeToFrames(1000, fEngineControl->fSampleRate), (fInMeanStep / 2));
        jack_log("JackOSSDriver::Read start recording discard %ld frames", discard);
        fOSSReadSync = GetMicroSeconds();
        Discard(discard);

        fForceSync = true;
        fForceBalancing = true;
    }

    if (fOutFD > 0 && fOSSWriteSync == 0) {
        // First cycle, account for leftover samples from previous writes.
        fOSSWriteOffset = 0;
        oss_count_t ptr;
        if (ioctl(fOutFD, SNDCTL_DSP_CURRENT_OPTR, &ptr) == 0 && ptr.fifo_samples > 0) {
            jack_log("JackOSSDriver::Read pre playback samples = %ld, fifo_samples = %d", ptr.samples, ptr.fifo_samples);
            fOSSWriteOffset = ptr.fifo_samples;
        }

        // Start playback with silence, target latency as given by the user.
        jack_nframes_t silence = (fNperiods + 1) * fEngineControl->fBufferSize;
        // Minus half a block or at most 1ms of frames, avoid drift issues at start.
        silence -= min(TimeToFrames(1000, fEngineControl->fSampleRate), (fOutMeanStep / 2));
        silence = max(silence - fOSSWriteOffset, 1LL);
        jack_log("JackOSSDriver::Read start playback with %ld frames of silence", silence);
        fOSSWriteSync = GetMicroSeconds();
        WriteSilence(silence);

        fForceSync = true;
        fForceBalancing = true;
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeRead = GetMicroSeconds();
#endif

    if (WaitAndSync() < 0) {
        return -1;
    }

    // Keep begin cycle time
    JackDriver::CycleTakeBeginTime();

    if (fInFD < 0) {
        return 0;
    }

    // Try to read multiple times in case of short reads.
    size_t count = 0;
    for (int i = 0; i < 3 && count < fInputBufferSize; ++i) {
        ssize_t ret = ::read(fInFD, ((char*)fInputBuffer) + count, fInputBufferSize - count);
        if (ret < 0) {
            jack_error("JackOSSDriver::Read error = %s", strerror(errno));
            return -1;
        }
        count += ret;
    }

    // Read offset accounting and overrun detection.
    if (count > 0) {
        jack_time_t now = GetMicroSeconds();
        jack_time_t sync = max(fOSSReadSync, fOSSWriteSync);
        if (now - sync > 1000) {
            // Blocking read() may indicate sample loss in OSS - force resync.
            jack_log("JackOSSDriver::Read long read duration of %ld us", now - sync);
            fForceSync = true;
        }
        long long passed = TimeToFrames(now - fOSSReadSync, fEngineControl->fSampleRate);
        passed -= (passed % fInBlockSize);
        if (passed > fOSSReadOffset + fOSSInBuffer) {
            // Overrun, adjust read and write position.
            long long missed = passed - (fOSSReadOffset + fOSSInBuffer);
            jack_error("JackOSSDriver::Read missed %ld frames by overrun, passed=%ld, sync=%ld, now=%ld", missed, passed, fOSSReadSync, now);
            fOSSReadOffset += missed;
            fOSSWriteOffset += missed;
            NotifyXRun(now, float(FramesToTime(missed, fEngineControl->fSampleRate)));
        }
        fOSSReadOffset += count / (fInSampleSize * fCaptureChannels);
    }

#ifdef JACK_MONITOR
    if (count > 0 && count != (int)fInputBufferSize)
        jack_log("JackOSSDriver::Read count = %ld", count / (fInSampleSize * fCaptureChannels));
    gCycleTable.fTable[gCycleCount].fAfterRead = GetMicroSeconds();
#endif

    // Check and clear OSS errors.
    audio_errinfo ei_in;
    if (ioctl(fInFD, SNDCTL_DSP_GETERROR, &ei_in) == 0) {

        // Not reliable for overrun detection, virtual_oss doesn't implement it.
        if (ei_in.rec_overruns > 0 ) {
            jack_error("JackOSSDriver::Read %d overrun events", ei_in.rec_overruns);
        }

        if (ei_in.rec_errorcount > 0 && ei_in.rec_lasterror != 0) {
            jack_error("%d OSS rec event(s), last=%05d:%d", ei_in.rec_errorcount, ei_in.rec_lasterror, ei_in.rec_errorparm);
        }
    }

    if (count < fInputBufferSize) {
        jack_error("JackOSSDriver::Read incomplete read of %ld bytes", count);
        return -1;
    }

    for (int i = 0; i < fCaptureChannels; i++) {
        if (fGraphManager->GetConnectionsNum(fCapturePortList[i]) > 0) {
            CopyAndConvertIn(GetInputBuffer(i), fInputBuffer, fEngineControl->fBufferSize, i, fCaptureChannels, fInSampleSize * 8);
        }
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fAfterReadConvert = GetMicroSeconds();
#endif

    return 0;
}

int JackOSSDriver::Write()
{
    if (fOutFD < 0) {
        return 0;
    }

    unsigned int skip = 0;
    jack_time_t start = GetMicroSeconds();

    if (fOSSWriteSync > 0) {
        // Check for underruns, rounded to hardware block size if available.
        long long passed = TimeToFrames(start - fOSSWriteSync, fEngineControl->fSampleRate);
        long long consumed = passed - (passed % fOutBlockSize);
        long long tolerance = (fOutBlockSize > 1) ? 0 : fOutMeanStep;
        long long overdue = 0;
        if (consumed > fOSSWriteOffset + tolerance) {
            // Skip playback data that already passed.
            overdue = consumed - fOSSWriteOffset - tolerance;
            jack_error("JackOSSDriver::Write underrun, late by %ld, skip %ld frames", passed - fOSSWriteOffset, overdue);
            jack_log("JackOSSDriver::Write playback offset %ld frames synced %ld us ago", fOSSWriteOffset, start - fOSSWriteSync);
            // Also consider buffer balance, there was a gap in playback anyway.
            fForceBalancing = true;
        }
        // Account for buffer balance if needed.
        long long progress = fEngineControl->fBufferSize;
        if (fForceBalancing) {
            fForceBalancing = false;
            progress = max(progress + fBufferBalance, 0LL);
            jack_info("JackOSSDriver::Write buffer balancing %ld frames", fBufferBalance);
            jack_log("JackOSSDriver::Write recording sync %ld frames %ld us ago", fOSSReadOffset, start - fOSSReadSync);
            jack_log("JackOSSDriver::Write playback sync %ld frames %ld us ago", fOSSWriteOffset, start - fOSSWriteSync);
        }
        // How many samples to skip or prepend due to underrun and balancing.
        long long write_length = progress - overdue;
        if (write_length <= 0) {
            skip += fOutputBufferSize;
            fOSSWriteOffset += progress;
        } else if (write_length < fEngineControl->fBufferSize) {
            skip += (fEngineControl->fBufferSize - write_length) * fOutSampleSize * fPlaybackChannels;
            fOSSWriteOffset += overdue;
        } else if (write_length > fEngineControl->fBufferSize) {
            jack_nframes_t fill = write_length - fEngineControl->fBufferSize;
            WriteSilence(fill);
        }
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeWriteConvert = GetMicroSeconds();
#endif

    memset(fOutputBuffer, 0, fOutputBufferSize);
    for (int i = 0; i < fPlaybackChannels; i++) {
        if (fGraphManager->GetConnectionsNum(fPlaybackPortList[i]) > 0) {
            CopyAndConvertOut(fOutputBuffer, GetOutputBuffer(i), fEngineControl->fBufferSize, i, fPlaybackChannels, fOutSampleSize * 8);
        }
    }

#ifdef JACK_MONITOR
    gCycleTable.fTable[gCycleCount].fBeforeWrite = GetMicroSeconds();
#endif

    // Try multiple times in case of short writes.
    ssize_t count = skip;
    for (int i = 0; i < 3 && count < fOutputBufferSize; ++i) {
        ssize_t ret = ::write(fOutFD, ((char*)fOutputBuffer) + count, fOutputBufferSize - count);
        if (ret < 0) {
            jack_error("JackOSSDriver::Write error = %s", strerror(errno));
            return -1;
        }
        count += ret;
    }

    fOSSWriteOffset += ((count - skip) / (fOutSampleSize * fPlaybackChannels));

    jack_time_t duration = GetMicroSeconds() - start;
    if (duration > 1000) {
        // Blocking write() may indicate sample loss in OSS - force resync.
        jack_log("JackOSSDriver::Write long write duration of %ld us", duration);
        fForceSync = true;
    }

#ifdef JACK_MONITOR
    if (count > 0 && count != (int)fOutputBufferSize)
        jack_log("JackOSSDriver::Write count = %ld", (count - skip) / (fOutSampleSize * fPlaybackChannels));
    gCycleTable.fTable[gCycleCount].fAfterWrite = GetMicroSeconds();
    gCycleCount = (gCycleCount == CYCLE_POINTS - 1) ? gCycleCount: gCycleCount + 1;
#endif

    // Check and clear OSS errors.
    audio_errinfo ei_out;
    if (ioctl(fOutFD, SNDCTL_DSP_GETERROR, &ei_out) == 0) {

        // Not reliable for underrun detection, virtual_oss does not implement it.
        if (ei_out.play_underruns > 0) {
            jack_error("JackOSSDriver::Write %d underrun events", ei_out.play_underruns);
        }

        if (ei_out.play_errorcount > 0 && ei_out.play_lasterror != 0) {
            jack_error("%d OSS play event(s), last=%05d:%d",ei_out.play_errorcount, ei_out.play_lasterror, ei_out.play_errorparm);
        }
    }

    if (count < (int)fOutputBufferSize) {
        jack_error("JackOSSDriver::Write incomplete write of %ld bytes", count - skip);
        return -1;
    }

    return 0;
}

int JackOSSDriver::SetBufferSize(jack_nframes_t buffer_size)
{
    CloseAux();

    // Additional latency introduced by the OSS buffer, depends on buffer size.
    fCaptureLatency = fExtraCaptureLatency;
    fPlaybackLatency = fExtraPlaybackLatency + fNperiods * buffer_size;

    JackAudioDriver::SetBufferSize(buffer_size); // Generic change, never fails
    return OpenAux();
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
    jack_driver_descriptor_add_parameter(desc, &filler, "excl", 'e', JackDriverParamBool, &value, NULL, "Exclusive and direct device access", NULL);

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
