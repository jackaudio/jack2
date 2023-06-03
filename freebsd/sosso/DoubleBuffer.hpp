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

#ifndef SOSSO_DOUBLEBUFFER_HPP
#define SOSSO_DOUBLEBUFFER_HPP

#include "sosso/Buffer.hpp"
#include "sosso/Logging.hpp"
#include <algorithm>
#include <limits>

namespace sosso {

/*!
 * \brief Double Buffering for Channel
 *
 * Manages double buffering on top of a ReadChannel or WriteChannel. It takes
 * two buffers with corresponding end positions. One of these is selected
 * for processing, depending on the buffer and channel positions. The buffers
 * can be overlapping or have gaps in between.
 * A buffer is marked as finished when all buffer data was processed and the
 * channel progress has reached the buffer end. This provides steady buffer
 * replacement times, synchronized with channel progress.
 * The wakeup times for processing are adapted to available channel data and
 * work pending (unprocessed buffer data).
 */
template <class Channel> class DoubleBuffer : public Channel {
  /*!
   * \brief Store a buffer and its end position.
   *
   * The end position of the buffer corresponds to channel progress in frames.
   * Marking the end position allows to map the buffer content to the matching
   * channel data, independent of read and write positions.
   */
  struct BufferRecord {
    Buffer buffer;               // External buffer, may be empty.
    std::int64_t end_frames = 0; // Buffer end position in frames.
  };

public:
  //! Indicate that buffer is ready for processing.
  bool ready() const { return _buffer_a.buffer.valid(); }

  /*!
   * \brief Set the next consecutive buffer to be processed.
   * \param buffer External buffer ready for processing.
   * \param end_frames End position of the buffer in frames.
   * \return True if successful, false means there are already two buffers.
   */
  bool set_buffer(Buffer &&buffer, std::int64_t end_frames) {
    // Set secondary buffer if available.
    if (!_buffer_b.buffer.valid()) {
      _buffer_b.buffer = std::move(buffer);
      _buffer_b.end_frames = end_frames;
      // Promote secondary buffer to primary if primary is not set.
      if (!_buffer_a.buffer.valid()) {
        std::swap(_buffer_b, _buffer_a);
      }
      return ready();
    }
    return false;
  }

  /*!
   * \brief Reset the buffer end positions in case of over- and underruns.
   * \param end_frames New end position of the primary buffer.
   * \return True if ready to proceed.
   */
  bool reset_buffers(std::int64_t end_frames) {
    // Reset primary buffer.
    if (_buffer_a.buffer.valid()) {
      std::memset(_buffer_a.buffer.data(), 0, _buffer_a.buffer.length());
      _buffer_a.buffer.reset();
      Log::info(SOSSO_LOC, "Primary buffer reset from %lld to %lld.",
                _buffer_a.end_frames, end_frames);
      _buffer_a.end_frames = end_frames;
    }
    // Reset secondary buffer.
    if (_buffer_b.buffer.valid()) {
      std::memset(_buffer_b.buffer.data(), 0, _buffer_b.buffer.length());
      _buffer_b.buffer.reset();
      end_frames += _buffer_b.buffer.length() / Channel::frame_size();
      Log::info(SOSSO_LOC, "Secondary buffer reset from %lld to %lld.",
                _buffer_b.end_frames, end_frames);
      _buffer_b.end_frames = end_frames;
    }
    return ready();
  }

  //! Retrieve the primary buffer, may be empty.
  Buffer &&take_buffer() {
    std::swap(_buffer_a, _buffer_b);
    return std::move(_buffer_b.buffer);
  }

  /*!
   * \brief Process channel with given buffers to read or write.
   * \param now Time offset from channel start in frames, see FrameClock.
   * \return True if there were no processing errors.
   */
  bool process(std::int64_t now) {
    // Round frame time down to steppings, ignore timing jitter.
    now = now - now % Channel::stepping();
    // Always process primary buffer, No-Op if already done.
    bool ok = Channel::process(_buffer_a.buffer, _buffer_a.end_frames, now);
    // Process secondary buffer when primary is done.
    if (ok && _buffer_a.buffer.done() && _buffer_b.buffer.valid()) {
      ok = Channel::process(_buffer_b.buffer, _buffer_b.end_frames, now);
    }
    return ok;
  }

  //! End position of the primary buffer.
  std::int64_t end_frames() const {
    if (ready()) {
      return _buffer_a.end_frames;
    }
    return 0;
  }

  //! Expected frame time when primary buffer is finished.
  std::int64_t period_end() const {
    if (ready()) {
      return end_frames() + Channel::balance();
    }
    return 0;
  }

  //! Expected frame time when both buffers are finished.
  std::int64_t total_end() const {
    if (ready()) {
      if (_buffer_b.buffer.valid()) {
        return _buffer_b.end_frames + Channel::balance();
      }
      return end_frames() + Channel::balance();
    }
    return 0;
  }

  /*!
   * \brief Calculate next wakeup time for processing.
   * \param now Current frame time as offset from channel start, see FrameClock.
   * \return Next suggested wakeup in frame time.
   */
  std::int64_t wakeup_time(std::int64_t now) const {
    // No need to wake up if channel is not running.
    if (!Channel::is_open()) {
      return std::numeric_limits<std::int64_t>::max();
    }
    // Wakeup immediately if there's more work to do now.
    if (Channel::oss_available() > 0 &&
        (!_buffer_a.buffer.done() || !_buffer_b.buffer.done())) {
      Log::log(SOSSO_LOC, "Immediate wakeup at %lld for more work.", now);
      return now;
    }
    // Get upcoming buffer end and compute next channel wakeup time.
    std::int64_t sync_frames = now;
    if (_buffer_a.buffer.valid() && !finished(now)) {
      sync_frames = period_end();
    } else if (_buffer_b.buffer.valid() && !total_finished(now)) {
      sync_frames = _buffer_b.end_frames + Channel::balance();
    } else {
      sync_frames = std::numeric_limits<std::int64_t>::max();
    }
    return Channel::wakeup_time(sync_frames);
  }

  //! Indicate progress on processing the primary buffer, in frames.
  std::int64_t buffer_progress() const {
    return _buffer_a.buffer.progress() / Channel::frame_size();
  }

  //! Indicate that primary buffer is finished at current frame time.
  bool finished(std::int64_t now) const {
    return period_end() <= now && _buffer_a.buffer.done();
  }

  //! Indicate that both buffers are finished at current frame time.
  bool total_finished(std::int64_t now) const {
    return total_end() <= now && _buffer_a.buffer.done() &&
           _buffer_b.buffer.done();
  }

  //! Print channel state as user information, at current frame time.
  void log_state(std::int64_t now) const {
    const char *direction = Channel::playback() ? "Out" : "In";
    const char *sync = (Channel::last_sync() == now) ? "sync" : "frame";
    std::int64_t buf_a = _buffer_a.buffer.progress() / Channel::frame_size();
    std::int64_t buf_b = _buffer_b.buffer.progress() / Channel::frame_size();
    Log::log(SOSSO_LOC,
             "%s %s, %lld bal %lld, buf A %lld B %lld OSS %lld, %lld left, "
             "req %u min %lld",
             direction, sync, now, Channel::balance(), buf_a, buf_b,
             Channel::oss_available(), period_end() - now,
             Channel::sync_level(), Channel::min_progress());
  }

private:
  BufferRecord _buffer_a; // Primary buffer, may be empty.
  BufferRecord _buffer_b; // Secondary buffer, may be empty.
};

} // namespace sosso

#endif // SOSSO_DOUBLEBUFFER_HPP
