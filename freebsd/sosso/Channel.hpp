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

#ifndef SOSSO_CHANNEL_HPP
#define SOSSO_CHANNEL_HPP

#include "sosso/Device.hpp"
#include <algorithm>

namespace sosso {

/*!
 * \brief Audio Channel of a Device
 *
 * As a base class for read and write channels, this class provides generic
 * handling of progress, loss and wakeup times. Progress here means the OSS
 * device captures or consumes audio data, in frames. When progress is detected
 * within a short wakeup interval, this counts as a sync where we can exactly
 * match the device progress to current time.
 * The balance indicates the drift between device progress and external time,
 * usually taken from FrameClock.
 * At device start and after loss, device progress can be irregular and is
 * temporarily decoupled from Channel progress (freewheel). Sync events are
 * required to change into normal mode which strictly follows device progress.
 */
class Channel : public Device {
public:
  /*!
   * \brief Open the device, initialize Channel
   * \param device Full device path.
   * \param mode Open mode (read / write).
   * \return True if successful.
   */
  bool open(const char *device, int mode) {
    // Reset all internal statistics from last run.
    _last_processing = 0;
    _last_sync = 0;
    _last_progress = 0;
    _balance = 0;
    _min_progress = 0;
    _max_progress = 0;
    _total_loss = 0;
    _sync_level = 8;
    return Device::open(device, mode);
  }

  //! Total progress of the device since start.
  std::int64_t last_progress() const { return _last_progress; }

  //! Balance (drift) compared to external time.
  std::int64_t balance() const { return _balance; }

  //! Last time there was a successful sync.
  std::int64_t last_sync() const { return _last_sync; }

  //! Last time the Channel was processed (mark_progress()).
  std::int64_t last_processing() const { return _last_processing; }

  //! Maximum progress step encountered.
  std::int64_t max_progress() const { return _max_progress; }

  //! Minimum progress step encountered.
  std::int64_t min_progress() const { return _min_progress; }

  //! Current number of syncs required to change to normal mode.
  unsigned sync_level() const { return _sync_level; }

  //! Indicate Channel progress decoupled from device progress.
  bool freewheel() const { return _sync_level > 4; }

  //! Indicate a full resync with small wakeup steps is required.
  bool full_resync() const { return _sync_level > 2; }

  //! Indicate a resync is required.
  bool resync() const { return _sync_level > 0; }

  //! Total number of frames lost due to over- or underruns.
  std::int64_t total_loss() const { return _total_loss; }

  //! Next time a device progress could be expected.
  std::int64_t next_min_progress() const {
    return _last_progress + _min_progress + _balance;
  }

  //! Calculate safe wakeup time to avoid over- or underruns.
  std::int64_t safe_wakeup(std::int64_t oss_available) const {
    return next_min_progress() + buffer_frames() - oss_available -
           max_progress();
  }

  //! Estimate the time to expect over- or underruns.
  std::int64_t estimated_dropout(std::int64_t oss_available) const {
    return _last_progress + _balance + buffer_frames() - oss_available;
  }

  /*!
   * \brief Calculate next wakeup time.
   * \param sync_target External wakeup target like the next buffer end.
   * \param oss_available Number of frames available in OSS buffer.
   * \return Next wakeup time in external frame time.
   */
  std::int64_t wakeup_time(std::int64_t sync_target,
                           std::int64_t oss_available) const {
    // Use one sync step by default.
    std::int64_t wakeup = _last_processing + Device::stepping();
    if (freewheel() || full_resync()) {
      // Small steps when doing a full resync.
    } else if (resync() || wakeup + max_progress() > sync_target) {
      // Sync required, wake up prior to next progress if possible.
      if (next_min_progress() > wakeup) {
        wakeup = next_min_progress() - Device::stepping();
      } else if (next_min_progress() > _last_processing) {
        wakeup = next_min_progress();
      }
    } else {
      // Sleep until prior to sync target, then sync again.
      wakeup = sync_target - max_progress();
    }
    // Make sure we wake up at sync target.
    if (sync_target > _last_processing && sync_target < wakeup) {
      wakeup = sync_target;
    }
    // Make sure we don't sleep into an OSS under- or overrun.
    if (safe_wakeup(oss_available) < wakeup) {
      wakeup = std::max(safe_wakeup(oss_available),
                        _last_processing + Device::stepping());
    }
    return wakeup;
  }

protected:
  // Account for progress detected, at current time.
  void mark_progress(std::int64_t progress, std::int64_t now) {
    if (progress > 0) {
      if (freewheel()) {
        // Some cards show irregular progress at the beginning, correct that.
        // Also correct loss after under- and overruns, assume same balance.
        _last_progress = now - progress - _balance;
        // Require a sync before transition back to normal processing.
        if (now <= _last_processing + stepping()) {
          _sync_level -= 1;
        }
      } else if (now <= _last_processing + stepping()) {
        // Successful sync on progress within small processing steps.
        _balance = now - (_last_progress + progress);
        _last_sync = now;
        if (_sync_level > 0) {
          _sync_level -= 1;
        }
        if (progress < _min_progress || _min_progress == 0) {
          _min_progress = progress;
        }
        if (progress > _max_progress) {
          _max_progress = progress;
        }
      } else {
        // Big step with progress but no sync, requires a resync.
        _sync_level += 1;
      }
      _last_progress += progress;
    }
    _last_processing = now;
  }

  // Account for loss given progress and current time.
  std::int64_t mark_loss(std::int64_t progress, std::int64_t now) {
    // Estimate frames lost due to over- or underrun.
    std::int64_t loss = (now - _balance) - (_last_progress + progress);
    return mark_loss(loss);
  }

  // Account for loss.
  std::int64_t mark_loss(std::int64_t loss) {
    if (loss > 0) {
      _total_loss += loss;
      // Resync OSS progress to frame time (now) to recover from loss.
      _sync_level = std::max(_sync_level, 6U);
    } else {
      loss = 0;
    }
    return loss;
  }

private:
  std::int64_t _last_processing = 0; // Last processing time.
  std::int64_t _last_sync = 0;       // Last sync time.
  std::int64_t _last_progress = 0;   // Total device progress.
  std::int64_t _balance = 0;         // Channel drift.
  std::int64_t _min_progress = 0;    // Minimum progress step encountered.
  std::int64_t _max_progress = 0;    // Maximum progress step encountered.
  std::int64_t _total_loss = 0;      // Total loss due to over- or underruns.
  unsigned _sync_level = 0;          // Syncs required.
};

} // namespace sosso

#endif // SOSSO_CHANNEL_HPP
