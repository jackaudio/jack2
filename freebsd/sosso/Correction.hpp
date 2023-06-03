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

#ifndef SOSSO_CORRECTION_HPP
#define SOSSO_CORRECTION_HPP

#include <cstdint>

namespace sosso {

/*!
 * \brief Drift Correction
 *
 * Calculates drift correction for a channel, relative to another channel if
 * required. Usually the playback channel is corrected relative to the recording
 * channel, if in use.
 * It keeps track of the correction parameter (in frames), and also the
 * threshhold values which determine the amount of correction. Above these
 * threshholds, either single frame correction is applied for smaller drift,
 * or rigorous correction in case of large discrepance. The idea is that single
 * frame corrections typically go unnoticed, but it may not be sufficient to
 * correct something more grave like packet loss on a USB audio interface.
 */
class Correction {
public:
  //! Default constructor, threshhold values are set separately.
  Correction() = default;

  /*!
   * \brief Set thresholds for small drift correction.
   * \param drift_min Limit for negative drift balance.
   * \param drift_max Limit for positive drift balance.
   */
  void set_drift_limits(std::int64_t drift_min, std::int64_t drift_max) {
    if (drift_min < drift_max) {
      _drift_min = drift_min;
      _drift_max = drift_max;
    } else {
      _drift_min = drift_max;
      _drift_max = drift_min;
    }
  }

  /*!
   * \brief Set thresholds for rigorous large discrepance correction.
   * \param loss_min Limit for negative discrepance balance.
   * \param loss_max Limit for positive discrepance balance.
   */
  void set_loss_limits(std::int64_t loss_min, std::int64_t loss_max) {
    if (loss_min < loss_max) {
      _loss_min = loss_min;
      _loss_max = loss_max;
    } else {
      _loss_min = loss_max;
      _loss_max = loss_min;
    }
  }

  //! Get current correction parameter.
  std::int64_t correction() const { return _correction; }

  /*!
   * \brief Calculate a new correction parameter.
   * \param balance Balance of the corrected channel, compared to FrameClock.
   * \param target Balance of a master channel which acts as reference.
   * \return Current correction parameter.
   */
  std::int64_t correct(std::int64_t balance, std::int64_t target = 0) {
    std::int64_t corrected_balance = balance - target + _correction;
    if (corrected_balance > _loss_max) {
      // Large positive discrepance, rigorous correction.
      _correction -= corrected_balance - _loss_max;
    } else if (corrected_balance < _loss_min) {
      // Large negative discrepance, rigorous correction.
      _correction += _loss_min - corrected_balance;
    } else if (corrected_balance > _drift_max) {
      // Small positive drift, correct by a single frame.
      _correction -= 1;
    } else if (corrected_balance < _drift_min) {
      // Small negative drift, correct by a single frame.
      _correction += 1;
    }
    return _correction;
  }

  //! Clear the current correction parameter, but not the thresholds.
  void clear() { _correction = 0; }

private:
  std::int64_t _loss_min = -128; // Negative threshold for rigorous correction.
  std::int64_t _loss_max = 128;  // Positive threshold for rigorous correction.
  std::int64_t _drift_min = -64; // Negative threshold for drift correction.
  std::int64_t _drift_max = 64;  // Positive threshold for drift correction.
  std::int64_t _correction = 0;  // Correction parameter.
};

} // namespace sosso

#endif // SOSSO_CORRECTION_HPP
