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

#ifndef SOSSO_FRAMECLOCK_HPP
#define SOSSO_FRAMECLOCK_HPP

#include "sosso/Logging.hpp"
#include <sys/errno.h>
#include <time.h>

namespace sosso {

/*!
 * \brief Clock using audio frames as time unit.
 *
 * Provides time as an offset from an initial time zero, usually when the audio
 * device was started. Instead of nanoseconds it measures time in frames
 * (samples per channel), and thus needs to know the sample rate.
 * It also lets a thread sleep until a specified wakeup time, again in frames.
 */
class FrameClock {
public:
  /*!
   * \brief Initialize the clock, set time zero.
   * \param sample_rate Sample rate in Hz, for time to frame conversion.
   * \return True if successful, false means an error occurred.
   */
  bool init_clock(unsigned sample_rate) {
    return set_sample_rate(sample_rate) && init_zero_time();
  }

  /*!
   * \brief Get current frame time.
   * \param result Set to current frame time, as offset from time zero.
   * \return True if successful, false means an error occurred.
   */
  bool now(std::int64_t &result) const {
    std::int64_t time_ns = 0;
    if (get_time_offset(time_ns)) {
      result = time_to_frames(time_ns);
      return true;
    }
    return false;
  }

  /*!
   * \brief Let the thread sleep until wakeup time.
   * \param wakeup_frame Wakeup time in frames since time zero.
   * \return True if successful, false means an error occurred.
   */
  bool sleep(std::int64_t wakeup_frame) const {
    std::int64_t time_ns = frames_to_time(wakeup_frame);
    return sleep_until(time_ns);
  }

  //! Convert frames to time in nanoseconds.
  std::int64_t frames_to_time(std::int64_t frames) const {
    return (frames * 1000000000) / _sample_rate;
  }

  //! Convert time in nanoseconds to frames.
  std::int64_t time_to_frames(std::int64_t time_ns) const {
    return (time_ns * _sample_rate) / 1000000000;
  }

  //! Convert frames to system clock time in microseconds.
  std::int64_t frames_to_absolute_us(std::int64_t frames) const {
    return _zero.tv_sec * 1000000ULL + _zero.tv_nsec / 1000 +
           frames_to_time(frames) / 1000;
  }

  //! Currently used sample rate in Hz.
  unsigned sample_rate() const { return _sample_rate; }

  //! Set the sample rate in Hz, used for time to frame conversion.
  bool set_sample_rate(unsigned sample_rate) {
    if (sample_rate > 0) {
      _sample_rate = sample_rate;
      return true;
    }
    return false;
  }

  //! Suggested minimal wakeup step in frames.
  unsigned stepping() const { return 16U * (1U + (_sample_rate / 50000)); }

private:
  // Initialize time zero now.
  bool init_zero_time() { return gettime(_zero); }

  // Get current time in nanoseconds, as offset from time zero.
  bool get_time_offset(std::int64_t &result) const {
    timespec now;
    if (gettime(now)) {
      result = ((now.tv_sec - _zero.tv_sec) * 1000000000) + now.tv_nsec -
               _zero.tv_nsec;
      return true;
    }
    return false;
  }

  // Let thread sleep until wakeup time, in nanoseconds since time zero.
  bool sleep_until(std::int64_t offset_ns) const {
    timespec wakeup = {_zero.tv_sec + (_zero.tv_nsec + offset_ns) / 1000000000,
                       (_zero.tv_nsec + offset_ns) % 1000000000};
    if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup, NULL) != 0) {
      Log::warn(SOSSO_LOC, "Sleep failed with error %d.", errno);
      return false;
    }
    return true;
  }

  // Get current time in nanosecons, as a timespec struct.
  bool gettime(timespec &result) const {
    if (clock_gettime(CLOCK_MONOTONIC, &result) != 0) {
      Log::warn(SOSSO_LOC, "Get time failed with error %d.", errno);
      return false;
    }
    return true;
  }

  timespec _zero = {0, 0};       // Time zero as a timespec struct.
  unsigned _sample_rate = 48000; // Sample rate used for frame conversion.
};

} // namespace sosso

#endif // SOSSO_FRAMECLOCK_HPP
