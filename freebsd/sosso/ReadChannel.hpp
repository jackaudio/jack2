/*
 * Copyright (c) 2023 Florian Walpen <dev@submerge.ch>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SOSSO_READCHANNEL_HPP
#define SOSSO_READCHANNEL_HPP

#include "sosso/Buffer.hpp"
#include "sosso/Channel.hpp"
#include "sosso/Logging.hpp"
#include <fcntl.h>

namespace sosso {

/*!
 * \brief Recording Channel
 *
 * Specializes the generic Channel class into a recording channel. It keeps
 * track of the OSS recording progress, and reads the available audio data to an
 * external buffer. If the OSS buffer is memory mapped, the audio data is copied
 * from there. Otherwise I/O read() system calls are used.
 */
class ReadChannel : public Channel {
public:
  /*!
   * \brief Open a device for recording.
   * \param device Path to the device, e.g. "/dev/dsp1".
   * \param exclusive Try to get exclusive access to the device.
   * \return True if the device was opened successfully.
   */
  bool open(const char *device, bool exclusive = true) {
    int mode = O_RDONLY | O_NONBLOCK;
    if (exclusive) {
      mode |= O_EXCL;
    }
    return Channel::open(device, mode);
  }

  //! Available audio data to be read, in frames.
  std::int64_t oss_available() const {
    std::int64_t result = last_progress() - _read_position;
    if (result < 0) {
      result = 0;
    } else if (result > buffer_frames()) {
      result = buffer_frames();
    }
    return result;
  }

  /*!
   * \brief Calculate next wakeup time.
   * \param sync_frames Required sync event (e.g. buffer end), in frame time.
   * \return Suggested and safe wakeup time for next process(), in frame time.
   */
  std::int64_t wakeup_time(std::int64_t sync_frames) const {
    return Channel::wakeup_time(sync_frames, oss_available());
  }

  /*!
   * \brief Check OSS progress and read recorded audio to the buffer.
   * \param buffer Buffer to write to, untouched if invalid.
   * \param end Buffer end position, matching channel progress.
   * \param now Current time in frame time, see FrameClock.
   * \return True if successful, false means there was an error.
   */
  bool process(Buffer &buffer, std::int64_t end, std::int64_t now) {
    if (map()) {
      return (progress_done(now) || check_map_progress(now)) &&
             (buffer_done(buffer, end) || process_mapped(buffer, end, now));
    } else {
      return (progress_done(now) || check_read_progress(now)) &&
             (buffer_done(buffer, end) || process_read(buffer, end, now));
    }
  }

protected:
  // Indicate that OSS progress has already been checked.
  bool progress_done(std::int64_t now) { return (last_processing() == now); }

  // Check OSS progress in case of memory mapped buffer.
  bool check_map_progress(std::int64_t now) {
    // Get OSS progress through map pointer.
    if (get_rec_pointer()) {
      std::int64_t progress = map_progress() - _oss_progress;
      _oss_progress += progress;
      std::int64_t available = last_progress() + progress - _read_position;
      std::int64_t loss = mark_loss(available - buffer_frames());
      mark_progress(progress, now);
      if (loss > 0) {
        Log::warn(SOSSO_LOC, "OSS recording buffer overrun, %lld lost.", loss);
        _read_position = last_progress() - buffer_frames();
      }
    }
    return progress_done(now);
  }

  // Read recorded audio data to buffer, in case of memory mapped OSS buffer.
  bool process_mapped(Buffer &buffer, std::int64_t end, std::int64_t now) {
    // Calculate current read buffer position.
    std::int64_t position = buffer_position(buffer, end);
    // Only read what is available until OSS captured its complete buffer.
    std::int64_t oldest = last_progress() - buffer_frames();
    if (_oss_progress < buffer_frames()) {
      oldest = last_progress() - _oss_progress;
    }
    if (std::int64_t skip = buffer_advance(buffer, oldest - position)) {
      // First part of the read buffer already passed, fill it up.
      Log::info(SOSSO_LOC, "@%lld - %lld Read buffer late by %lld, skip %lld.",
                now, end, oldest - position, skip);
      position += skip;
    } else if (position != _read_position) {
      // Position mismatch, reread what is available.
      if (std::int64_t rewind = buffer_rewind(buffer, position - oldest)) {
        Log::info(SOSSO_LOC,
                  "@%lld - %lld Read position mismatch, reread %lld.", now, end,
                  rewind);
        position -= rewind;
      }
    }
    if (position >= oldest && position < last_progress() && !buffer.done()) {
      // Read from offset up to current position, if read buffer can hold it.
      std::int64_t offset = last_progress() - position;
      std::size_t length = buffer.remaining(offset * frame_size());
      unsigned pointer = (_oss_progress - offset) % buffer_frames();
      length = read_map(buffer.position(), pointer * frame_size(), length);
      buffer.advance(length);
      _read_position = buffer_position(buffer, end);
    }
    _read_position += freewheel_finish(buffer, end, now);
    return true;
  }

  // Check progress when using I/O read() system call.
  bool check_read_progress(std::int64_t now) {
    // Check for OSS buffer overruns.
    std::int64_t overdue = now - estimated_dropout(oss_available());
    if ((overdue > 0 && get_rec_overruns() > 0) || overdue > max_progress()) {
      std::int64_t progress = buffer_frames() - oss_available();
      std::int64_t loss = mark_loss(progress, now);
      Log::warn(SOSSO_LOC, "OSS recording buffer overrun, %lld lost.", loss);
      mark_progress(progress + loss, now);
      _read_position = last_progress() - buffer_frames();
    } else {
      // Infer progress from OSS queue changes.
      std::int64_t queued = queued_samples();
      std::int64_t progress = queued - (last_progress() - _read_position);
      mark_progress(progress, now);
      _read_position = last_progress() - queued;
    }
    return progress_done(now);
  }

  // Read recorded audio data to buffer, using I/O read() syscall.
  bool process_read(Buffer &buffer, std::int64_t end, std::int64_t now) {
    bool ok = true;
    std::int64_t position = buffer_position(buffer, end);
    if (std::int64_t skip = buffer_advance(buffer, _read_position - position)) {
      // Overlapping buffers, skip the overlapping part.
      Log::info(SOSSO_LOC, "@%lld - %lld Read buffer overlap %lld, skip %lld.",
                now, end, _read_position - position, skip);
      position += skip;
    } else if (std::int64_t rewind =
                   buffer_rewind(buffer, position - _read_position)) {
      // Gap between reads, try to rewind to last read position.
      Log::info(SOSSO_LOC, "@%lld - %lld Read buffer gap %lld, rewind %lld.",
                now, end, position - _read_position, rewind);
      position -= rewind;
    }
    if (oss_available() == 0) {
      // OSS buffer is empty, nothing to do.
    } else if (position > _read_position) {
      // Read and omit data of remaining gap, drain OSS buffer.
      std::int64_t gap = position - _read_position;
      std::size_t read_limit = buffer.remaining(gap * frame_size());
      std::size_t bytes_read = 0;
      ok = read_io(buffer.position(), read_limit, bytes_read);
      Log::info(SOSSO_LOC, "@%lld - %lld Read buffer gap %lld, drain %lu.", now,
                end, gap, bytes_read / frame_size());
      _read_position += bytes_read / frame_size();
    } else if (position == _read_position) {
      // Read as much as currently available.
      std::size_t bytes_read = 0;
      ok = read_io(buffer.position(), buffer.remaining(), bytes_read);
      _read_position += bytes_read / frame_size();
      buffer.advance(bytes_read);
    }
    freewheel_finish(buffer, end, now);
    return ok;
  }

private:
  // Calculate read position of the remaining buffer.
  std::int64_t buffer_position(const Buffer &buffer, std::int64_t end) const {
    return end - extra_latency() - (buffer.remaining() / frame_size());
  }

  // Indicate that a buffer doesn't need further processing.
  bool buffer_done(const Buffer &buffer, std::int64_t end) const {
    return buffer.done() && buffer_position(buffer, end) <= _read_position;
  }

  // Extra latency to always finish on time, regardless of OSS progress steps.
  std::int64_t extra_latency() const { return max_progress(); }

  // Avoid stalled buffers with irregular OSS progress in freewheel mode.
  std::int64_t freewheel_finish(Buffer &buffer, std::int64_t end,
                                std::int64_t now) {
    std::int64_t advance = 0;
    if (freewheel() && now >= end + balance() && !buffer.done()) {
      // Buffer is overdue in freewheel sync mode, finish immediately.
      std::memset(buffer.position(), 0, buffer.remaining());
      advance = buffer.advance(buffer.remaining()) / frame_size();
      Log::info(SOSSO_LOC, "@%lld - %lld Read buffer overdue, fill by %lu.",
                now, end, advance);
    }
    return advance;
  }

  // Skip reading part of the buffer to match OSS read position.
  std::int64_t buffer_advance(Buffer &buffer, std::int64_t frames) {
    if (frames > 0) {
      std::size_t skip = buffer.remaining(frames * frame_size());
      std::memset(buffer.position(), 0, skip);
      return buffer.advance(skip) / frame_size();
    }
    return 0;
  }

  // Rewind part of the buffer to match OSS read position.
  std::int64_t buffer_rewind(Buffer &buffer, std::int64_t frames) {
    if (frames > 0) {
      return buffer.rewind(frames * frame_size()) / frame_size();
    }
    return 0;
  }

  std::int64_t _oss_progress = 0;  // Last memory mapped OSS progress.
  std::int64_t _read_position = 0; // Current read position of channel.
};

} // namespace sosso

#endif // SOSSO_READCHANNEL_HPP
