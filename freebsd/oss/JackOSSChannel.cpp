/*
Copyright (C) 2023 Florian Walpen <dev@submerge.ch>

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

#include "JackOSSChannel.h"
#include "JackError.h"
#include "JackThread.h"
#include "memops.h"

#include <cstdint>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <iostream>
#include <assert.h>
#include <stdio.h>

typedef jack_default_audio_sample_t jack_sample_t;

namespace
{

int SuggestSampleFormat(int bits)
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

bool SupportedSampleFormat(int format)
{
    switch(format) {
        // Only signed sample formats are supported by the conversion functions.
        case AFMT_S16_NE:
        case AFMT_S16_OE:
        case AFMT_S24_NE:
        case AFMT_S24_OE:
        case AFMT_S32_NE:
        case AFMT_S32_OE:
            return true;
    }
    return false;
}

void CopyAndConvertIn(jack_sample_t *dst, char *src, size_t nframes, int channel, int chcount, int format)
{
    switch (format) {

        case AFMT_S16_NE:
            src += channel * 2;
            sample_move_dS_s16(dst, src, nframes, chcount * 2);
            break;
        case AFMT_S16_OE:
            src += channel * 2;
            sample_move_dS_s16s(dst, src, nframes, chcount * 2);
            break;
        case AFMT_S24_NE:
            src += channel * 3;
            sample_move_dS_s24(dst, src, nframes, chcount * 3);
            break;
        case AFMT_S24_OE:
            src += channel * 3;
            sample_move_dS_s24s(dst, src, nframes, chcount * 3);
            break;
        case AFMT_S32_NE:
            src += channel * 4;
            sample_move_dS_s32(dst, src, nframes, chcount * 4);
            break;
        case AFMT_S32_OE:
            src += channel * 4;
            sample_move_dS_s32s(dst, src, nframes, chcount * 4);
            break;
    }
}

void CopyAndConvertOut(char *dst, jack_sample_t *src, size_t nframes, int channel, int chcount, int format)
{
    switch (format) {

        case AFMT_S16_NE:
            dst += channel * 2;
            sample_move_d16_sS(dst, src, nframes, chcount * 2, NULL);
            break;
        case AFMT_S16_OE:
            dst += channel * 2;
            sample_move_d16_sSs(dst, src, nframes, chcount * 2, NULL);
            break;
        case AFMT_S24_NE:
            dst += channel * 3;
            sample_move_d24_sS(dst, src, nframes, chcount * 3, NULL);
            break;
        case AFMT_S24_OE:
            dst += channel * 3;
            sample_move_d24_sSs(dst, src, nframes, chcount * 3, NULL);
            break;
        case AFMT_S32_NE:
            dst += channel * 4;
            sample_move_d32_sS(dst, src, nframes, chcount * 4, NULL);
            break;
        case AFMT_S32_OE:
            dst += channel * 4;
            sample_move_d32_sSs(dst, src, nframes, chcount * 4, NULL);
            break;
    }
}

}

void sosso::Log::log(sosso::SourceLocation location, const char* message) {
    jack_log(message);
}

void sosso::Log::info(sosso::SourceLocation location, const char* message) {
    jack_info(message);
}

void sosso::Log::warn(sosso::SourceLocation location, const char* message) {
    jack_error(message);
}

namespace Jack
{

bool JackOSSChannel::InitialSetup(unsigned int sample_rate)
{
    fFrameStamp = 0;
    fCorrection.clear();
    return fFrameClock.set_sample_rate(sample_rate);
}

bool JackOSSChannel::OpenCapture(const char *device, bool exclusive, int bits, int &channels)
{
    if (channels == 0) channels = 2;

    int sample_format = SuggestSampleFormat(bits);

    if (!fReadChannel.set_parameters(sample_format, fFrameClock.sample_rate(), channels)) {
        jack_error("JackOSSChannel::OpenCapture unsupported sample format %#x", sample_format);
        return false;
    }

    if (!fReadChannel.open(device, exclusive)) {
        return false;
    }

    if (fReadChannel.sample_rate() != fFrameClock.sample_rate()) {
        jack_error("JackOSSChannel::OpenCapture driver forced sample rate %ld", fReadChannel.sample_rate());
        fReadChannel.close();
        return false;
    }

    if (!SupportedSampleFormat(fReadChannel.sample_format())) {
        jack_error("JackOSSChannel::OpenCapture unsupported sample format %#x", fReadChannel.sample_format());
        fReadChannel.close();
        return false;
    }

    jack_log("JackOSSChannel::OpenCapture capture file descriptor = %d", fReadChannel.file_descriptor());

    if (fReadChannel.channels() != channels) {
        channels = fReadChannel.channels();
        jack_info("JackOSSChannel::OpenCapture driver forced the number of capture channels %ld", channels);
    }

    fReadChannel.memory_map();

    return true;
}

bool JackOSSChannel::OpenPlayback(const char *device, bool exclusive, int bits, int &channels)
{
    if (channels == 0) channels = 2;

    int sample_format = SuggestSampleFormat(bits);

    if (!fWriteChannel.set_parameters(sample_format, fFrameClock.sample_rate(), channels)) {
        jack_error("JackOSSChannel::OpenPlayback unsupported sample format %#x", sample_format);
        return false;
    }

    if (!fWriteChannel.open(device, exclusive)) {
        return false;
    }

    if (fWriteChannel.sample_rate() != fFrameClock.sample_rate()) {
        jack_error("JackOSSChannel::OpenPlayback driver forced sample rate %ld", fWriteChannel.sample_rate());
        fWriteChannel.close();
        return false;
    }

    if (!SupportedSampleFormat(fWriteChannel.sample_format())) {
        jack_error("JackOSSChannel::OpenPlayback unsupported sample format %#x", fWriteChannel.sample_format());
        fWriteChannel.close();
        return false;
    }

    jack_log("JackOSSChannel::OpenPlayback playback file descriptor = %d", fWriteChannel.file_descriptor());

    if (fWriteChannel.channels() != channels) {
        channels = fWriteChannel.channels();
        jack_info("JackOSSChannel::OpenPlayback driver forced the number of playback channels %ld", channels);
    }

    fWriteChannel.memory_map();

    return true;
}

bool JackOSSChannel::Read(jack_sample_t **sample_buffers, jack_nframes_t length, std::int64_t end)
{
    if (fReadChannel.recording()) {
        // Get buffer from read channel.
        sosso::Buffer buffer = fReadChannel.take_buffer();

        // Get recording audio data and then clear buffer.
        for (unsigned i = 0; i < fReadChannel.channels(); i++) {
            if (sample_buffers[i]) {
                CopyAndConvertIn(sample_buffers[i], buffer.data(), length, i, fReadChannel.channels(), fReadChannel.sample_format());
            }
        }
        buffer.reset();

        // Put buffer back to capture at requested end position.
        fReadChannel.set_buffer(std::move(buffer), end);
        SignalWork();
        return true;
    }
    return false;
}

bool JackOSSChannel::Write(jack_sample_t **sample_buffers, jack_nframes_t length, std::int64_t end)
{
    if (fWriteChannel.playback()) {
        // Get buffer from write channel.
        sosso::Buffer buffer = fWriteChannel.take_buffer();

        // Clear buffer and write new playback audio data.
        memset(buffer.data(), 0, buffer.length());
        buffer.reset();
        for (unsigned i = 0; i < fWriteChannel.channels(); i++) {
            if (sample_buffers[i]) {
                CopyAndConvertOut(buffer.data(), sample_buffers[i], length, i, fWriteChannel.channels(), fWriteChannel.sample_format());
            }
        }

        // Put buffer back to playback at requested end position.
        end += PlaybackCorrection();
        fWriteChannel.set_buffer(std::move(buffer), end);
        SignalWork();
        return true;
    }
    return false;
}

bool JackOSSChannel::StartChannels(unsigned int buffer_frames)
{
    int group_id = 0;

    if (fReadChannel.recording()) {
        // Allocate two recording buffers for double buffering.
        size_t buffer_size = buffer_frames * fReadChannel.frame_size();
        sosso::Buffer buffer((char*) calloc(buffer_size, 1), buffer_size);
        assert(buffer.data());
        fReadChannel.set_buffer(std::move(buffer), 0);
        buffer = sosso::Buffer((char*) calloc(buffer_size, 1), buffer_size);
        assert(buffer.data());
        fReadChannel.set_buffer(std::move(buffer), buffer_frames);
        // Add recording channel to synced start group.
        fReadChannel.add_to_sync_group(group_id);
    }

    if (fWriteChannel.playback()) {
        // Allocate two playback buffers for double buffering.
        size_t buffer_size = buffer_frames * fWriteChannel.frame_size();
        sosso::Buffer buffer((char*) calloc(buffer_size, 1), buffer_size);
        assert(buffer.data());
        fWriteChannel.set_buffer(std::move(buffer), 0);
        buffer = sosso::Buffer((char*) calloc(buffer_size, 1), buffer_size);
        assert(buffer.data());
        fWriteChannel.set_buffer(std::move(buffer), buffer_frames);
        // Add playback channel to synced start group.
        fWriteChannel.add_to_sync_group(group_id);
    }

    // Start both channels in sync if supported.
    if (fReadChannel.recording()) {
        fReadChannel.start_sync_group(group_id);
    } else {
        fWriteChannel.start_sync_group(group_id);
    }

    // Init frame clock here to mark start time.
    if (!fFrameClock.init_clock(fFrameClock.sample_rate())) {
        return false;
    }

    // Small drift corrections to keep latency whithin +/- 1ms.
    std::int64_t limit = fFrameClock.sample_rate() / 1000;
    fCorrection.set_drift_limits(-limit, limit);
    // Drastic corrections when drift exceeds half a period.
    limit = std::max<std::int64_t>(limit, buffer_frames / 2);
    fCorrection.set_loss_limits(-limit, limit);

    SignalWork();

    return true;
}

bool JackOSSChannel::StopChannels()
{
    if (fReadChannel.recording()) {
        free(fReadChannel.take_buffer().data());
        free(fReadChannel.take_buffer().data());
        fReadChannel.memory_unmap();
        fReadChannel.close();
    }

    if (fWriteChannel.playback()) {
        free(fWriteChannel.take_buffer().data());
        free(fWriteChannel.take_buffer().data());
        fWriteChannel.memory_unmap();
        fWriteChannel.close();
    }

    return true;
}

bool JackOSSChannel::StartAssistThread(bool realtime, int priority)
{
    if (fAssistThread.Start() >= 0) {
        if (realtime && fAssistThread.AcquireRealTime(priority) != 0) {
            jack_error("JackOSSChannel::StartAssistThread realtime priority failed.");
        }
        return true;
    }
    return false;
}

bool JackOSSChannel::StopAssistThread()
{
    if (fAssistThread.GetStatus() != JackThread::kIdle) {
        fAssistThread.SetStatus(JackThread::kIdle);
        SignalWork();
        fAssistThread.Kill();
    }
    return true;
}

bool JackOSSChannel::CheckTimeAndRun()
{
    // Check current frame time.
    if (!fFrameClock.now(fFrameStamp)) {
        jack_error("JackOSSChannel::CheckTimeAndRun(): Frame clock failed.");
        return false;
    }
    std::int64_t now = fFrameStamp;

    // Process read channel if wakeup time passed, or OSS buffer data available.
    if (fReadChannel.recording() && !fReadChannel.total_finished(now)) {
        if (now >= fReadChannel.wakeup_time(now)) {
            if (!fReadChannel.process(now)) {
                jack_error("JackOSSChannel::CheckTimeAndRun(): Read process failed.");
                return false;
            }
        }
    }
    // Process write channel if wakeup time passed, or OSS buffer space available.
    if (fWriteChannel.playback() && !fWriteChannel.total_finished(now)) {
        if (now >= fWriteChannel.wakeup_time(now)) {
            if (!fWriteChannel.process(now)) {
                jack_error("JackOSSChannel::CheckTimeAndRun(): Write process failed.");
                return false;
            }
        }
    }

    return true;
}

bool JackOSSChannel::Sleep() const
{
    std::int64_t wakeup = NextWakeup();
    if (wakeup > fFrameStamp) {
        return fFrameClock.sleep(wakeup);
    }
    return true;
}

bool JackOSSChannel::CaptureFinished() const
{
    return fReadChannel.finished(fFrameStamp);
}

bool JackOSSChannel::PlaybackFinished() const
{
    return fWriteChannel.finished(fFrameStamp);
}

std::int64_t JackOSSChannel::PlaybackCorrection()
{
    std::int64_t correction = 0;
    // If both channels are used, correct drift relative to recording balance.
    if (fReadChannel.recording() && fWriteChannel.playback()) {
        std::int64_t previous = fCorrection.correction();
        correction = fCorrection.correct(fWriteChannel.balance(), fReadChannel.balance());
        if (correction != previous) {
            jack_info("Playback correction changed from %lld to %lld.", previous, correction);
            jack_info("Read balance %lld vs write balance %lld.", fReadChannel.balance(), fWriteChannel.balance());
        }
    }
    return correction;
}

bool JackOSSChannel::Init()
{
    return true;
}

bool JackOSSChannel::Execute()
{
    if (Lock()) {
        if (fAssistThread.GetStatus() != JackThread::kIdle) {
            if (!CheckTimeAndRun()) {
                return Unlock() && false;
            }
            std::int64_t wakeup = NextWakeup();
            if (fReadChannel.total_finished(fFrameStamp) && fWriteChannel.total_finished(fFrameStamp)) {
                // Nothing to do, wait on the mutex for work.
                jack_info("JackOSSChannel::Execute waiting for work.");
                fMutex.TimedWait(1000000);
                jack_info("JackOSSChannel::Execute resuming work.");
            } else if (fFrameStamp < wakeup) {
                // Unlock mutex before going to sleep, let others process.
                return Unlock() && fFrameClock.sleep(wakeup);
            }
        }
        return Unlock();
    }
    return false;
}

std::int64_t JackOSSChannel::XRunGap() const
{
    // Compute processing gap in case we are late.
    std::int64_t max_end = std::max(fReadChannel.total_end(), fWriteChannel.total_end());
    if (max_end < fFrameStamp) {
        return fFrameStamp - max_end;
    }
    return 0;
}

void JackOSSChannel::ResetBuffers(std::int64_t offset)
{
    // Clear buffers and offset their positions, after processing gaps.
    if (fReadChannel.recording()) {
        fReadChannel.reset_buffers(fReadChannel.end_frames() + offset);
    }
    if (fWriteChannel.playback()) {
        fWriteChannel.reset_buffers(fWriteChannel.end_frames() + offset);
    }
}

std::int64_t JackOSSChannel::NextWakeup() const
{
    return std::min(fReadChannel.wakeup_time(fFrameStamp), fWriteChannel.wakeup_time(fFrameStamp));
}

} // end of namespace
