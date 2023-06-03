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

#ifndef SOSSO_WRITECHANNEL_HPP
#define SOSSO_WRITECHANNEL_HPP

#include "sosso/Buffer.hpp"
#include "sosso/Channel.hpp"
#include "sosso/Logging.hpp"
#include <fcntl.h>

namespace sosso {

/*!
 * \brief Playback Channel
 *
 * Specializes the generic Channel class into a playback channel. It keeps track
 * of the OSS playback progress, and writes audio data from an external buffer
 * to the available OSS buffer. If the OSS buffer is memory mapped, the audio
 * data is copied there. Otherwise I/O write() system calls are used.
 */
class WriteChannel : public Channel {
public:
  /*!
   * \brief Open a device for playback.
   * \param device Path to the device, e.g. "/dev/dsp1".
   * \param exclusive Try to get exclusive access to the device.
   * \return True if the device was opened successfully.
   */
  bool open(const char *device, bool exclusive = true) {
    int mode = O_WRONLY | O_NONBLOCK;
    if (exclusive) {
      mode |= O_EXCL;
    }
    return Channel::open(device, mode);
  }

  //! Available OSS buffer space for writing, in frames.
  std::int64_t oss_available() const {
    std::int64_t result = last_progress() + buffer_frames() - _write_position;
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
   * \brief Check OSS progress and write playback audio to the OSS buffer.
   * \param buffer Buffer of playback audio data, untouched if invalid.
   * \param end Buffer end position, matching channel progress.
   * \param now Current time in frame time, see FrameClock.
   * \return True if successful, false means there was an error.
   */
  bool process(Buffer &buffer, std::int64_t end, std::int64_t now) {
    if (map()) {
      return (progress_done(now) || check_map_progress(now)) &&
             (buffer_done(buffer, end) || process_mapped(buffer, end, now));
    } else {
      return (progress_done(now) || check_write_progress(now)) &&
             (buffer_done(buffer, end) || process_write(buffer, end, now));
    }
  }

protected:
  // Indicate that OSS progress has already been checked.
  bool progress_done(std::int64_t now) { return (last_processing() == now); }

  // Check OSS progress in case of memory mapped buffer.
  bool check_map_progress(std::int64_t now) {
    // Get OSS progress through map pointer.
    if (get_play_pointer()) {
      std::int64_t progress = map_progress() - _oss_progress;
      if (progress > 0) {
        // Sometimes OSS playback starts with a bogus extra buffer cycle.
        if (progress > buffer_frames() &&
            now - last_processing() < buffer_frames() / 2) {
          Log::warn(SOSSO_LOC,
                    "OSS playback bogus buffer cycle, %lld frames in %lld.",
                    progress, now - last_processing());
          progress = progress % buffer_frames();
        }
        // Clear obsolete audio data in the buffer.
        write_map(nullptr, (_oss_progress % buffer_frames()) * frame_size(),
                  progress * frame_size());
        _oss_progress = map_progress();
      }
      std::int64_t loss =
          mark_loss(last_progress() + progress - _write_position);
      mark_progress(progress, now);
      if (loss > 0) {
        Log::warn(SOSSO_LOC, "OSS playback buffer underrun, %lld lost.", loss);
        _write_position = last_progress();
      }
    }
    return progress_done(now);
  }

  // Write playback audio data to a memory mapped OSS buffer.
  bool process_mapped(Buffer &buffer, std::int64_t end, std::int64_t now) {
    // Buffer position should be between OSS progress and last write position.
    std::int64_t position = buffer_position(buffer.remaining(), end);
    if (std::int64_t skip =
            buffer_advance(buffer, last_progress() - position)) {
      // First part of the buffer already played, skip it.
      Log::info(SOSSO_LOC, "@%lld - %lld Write %lld already played, skip %lld.",
                now, end, last_progress() - position, skip);
      position += skip;
    } else if (position != _write_position) {
      // Position mismatch, rewrite as much as possible.
      if (std::int64_t rewind =
              buffer_rewind(buffer, position - last_progress())) {
        Log::info(SOSSO_LOC,
                  "@%lld - %lld Write position mismatch, rewrite %lld.", now,
                  end, rewind);
        position -= rewind;
      }
    }
    // The writable window is the whole buffer, starting from OSS progress.
    if (!buffer.done() && position >= last_progress() &&
        position < last_progress() + buffer_frames()) {
      if (_write_position < position && _write_position + 8 >= position) {
        // Small remaining gap between writes, fill in a replay patch.
        std::int64_t offset = _write_position - last_progress();
        unsigned pointer = (_oss_progress + offset) % buffer_frames();
        std::size_t length = (position - _write_position) * frame_size();
        length = buffer.remaining(length);
        std::size_t written =
            write_map(buffer.position(), pointer * frame_size(), length);
        Log::info(SOSSO_LOC, "@%lld - %lld Write small gap %lld, replay %lld.",
                  now, end, position - _write_position, written / frame_size());
      }
      // Write from buffer offset up to either OSS or write buffer end.
      std::int64_t offset = position - last_progress();
      unsigned pointer = (_oss_progress + offset) % buffer_frames();
      std::size_t length = (buffer_frames() - offset) * frame_size();
      length = buffer.remaining(length);
      std::size_t written =
          write_map(buffer.position(), pointer * frame_size(), length);
      buffer.advance(written);
      _write_position = buffer_position(buffer.remaining(), end);
    }
    _write_position += freewheel_finish(buffer, end, now);
    return true;
  }

  // Check progress when using I/O write() system call.
  bool check_write_progress(std::int64_t now) {
    // Check for OSS buffer underruns.
    std::int64_t overdue = now - estimated_dropout(oss_available());
    if ((overdue > 0 && get_play_underruns() > 0) || overdue > max_progress()) {
      // OSS buffer underrun, estimate loss and progress from time.
      std::int64_t progress = _write_position - last_progress();
      std::int64_t loss = mark_loss(progress, now);
      Log::warn(SOSSO_LOC, "OSS playback buffer underrun, %lld lost.", loss);
      mark_progress(progress + loss, now);
      _write_position = last_progress();
    } else {
      // Infer progress from OSS queue changes.
      std::int64_t queued = queued_samples();
      std::int64_t progress = (_write_position - last_progress()) - queued;
      mark_progress(progress, now);
      _write_position = last_progress() + queued;
    }
    return progress_done(now);
  }

  // Write playback audio data to OSS buffer using I/O write() system call.
  bool process_write(Buffer &buffer, std::int64_t end, std::int64_t now) {
    bool ok = true;
    // Adjust buffer position to OSS write position, if possible.
    std::int64_t position = buffer_position(buffer.remaining(), end);
    if (std::int64_t rewind =
            buffer_rewind(buffer, position - _write_position)) {
      // Gap between buffers, replay parts to fill it up.
      Log::info(SOSSO_LOC, "@%lld - %lld Write buffer gap %lld, replay %lld.",
                now, end, position - _write_position, rewind);
      position -= rewind;
    } else if (std::int64_t skip =
                   buffer_advance(buffer, _write_position - position)) {
      // Overlapping buffers, skip the overlapping part.
      Log::info(SOSSO_LOC, "@%lld - %lld Write buffer overlap %lld, skip %lld.",
                now, end, _write_position - position, skip);
      position += skip;
    }
    if (oss_available() == 0) {
      // OSS buffer is full, nothing to do.
    } else if (position > _write_position) {
      // Replay to fill remaining gap, limit the write to just fill the gap.
      std::int64_t gap = position - _write_position;
      std::size_t write_limit = buffer.remaining(gap * frame_size());
      std::size_t bytes_written = 0;
      ok = write_io(buffer.position(), write_limit, bytes_written);
      Log::info(SOSSO_LOC, "@%lld - %lld Write buffer gap %lld, fill %lld.",
                now, end, gap, bytes_written / frame_size());
      _write_position += bytes_written / frame_size();
    } else if (position == _write_position) {
      // Write as much as currently possible.
      std::size_t write_limit = buffer.remaining();
      std::size_t bytes_written = 0;
      ok = write_io(buffer.position(), write_limit, bytes_written);
      _write_position += bytes_written / frame_size();
      buffer.advance(bytes_written);
    }
    // Make sure buffers finish in time, despite irregular progress (freewheel).
    freewheel_finish(buffer, end, now);
    return ok;
  }

private:
  // Calculate write position of the remaining buffer.
  std::int64_t buffer_position(std::size_t remaining, std::int64_t end) const {
    return end - (remaining / frame_size());
  }

  // Indicate that a buffer doesn't need further processing.
  bool buffer_done(const Buffer &buffer, std::int64_t end) const {
    return buffer.done() && end <= _write_position;
  }

  // Avoid stalled buffers with irregular OSS progress in freewheel mode.
  std::int64_t freewheel_finish(Buffer &buffer, std::int64_t end,
                                std::int64_t now) {
    std::int64_t advance = 0;
    // Make sure buffers finish in time, despite irregular progress (freewheel).
    if (freewheel() && now >= end + balance() && !buffer.done()) {
      advance = buffer.advance(buffer.remaining()) / frame_size();
      Log::info(SOSSO_LOC,
                "@%lld - %lld Write freewheel finish remaining buffer %lld.",
                now, end, advance);
    }
    return advance;
  }

  // Skip writing part of the buffer to match OSS write position.
  std::int64_t buffer_advance(Buffer &buffer, std::int64_t frames) {
    if (frames > 0) {
      return buffer.advance(frames * frame_size()) / frame_size();
    }
    return 0;
  }

  // Rewind part of the buffer to match OSS write postion.
  std::int64_t buffer_rewind(Buffer &buffer, std::int64_t frames) {
    if (frames > 0) {
      return buffer.rewind(frames * frame_size()) / frame_size();
    }
    return 0;
  }

  std::int64_t _oss_progress = 0;   // Last memory mapped OSS progress.
  std::int64_t _write_position = 0; // Current write position of the channel.
};

} // namespace sosso

#endif // SOSSO_WRITECHANNEL_HPP
