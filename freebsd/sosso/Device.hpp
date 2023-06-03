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

#ifndef SOSSO_DEVICE_HPP
#define SOSSO_DEVICE_HPP

#include "sosso/Logging.hpp"
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/soundcard.h>
#include <unistd.h>

namespace sosso {

/*!
 * \brief Manage OSS devices.
 *
 * Encapsulates all the low-level handling of a FreeBSD OSS pcm device. Due to
 * restrictions of the OSS API, the device can be opened for either playback or
 * recording, not both. For duplex operation, separate instances of Device have
 * to be opened.
 * By default a Device opens 2 channels of 32 bit samples at 48 kHz, but the
 * OSS API will force that to be whatever is supported by the hardware.
 * Different default parameters can be set via set_parameters() prior to opening
 * the Device. Always check the effective parameters before any use.
 */
class Device {
public:
  /*!
   * \brief Translate OSS sample formats to sample size.
   * \param format OSS sample format, see sys/soundcard.h header.
   * \return Sample size in bytes, 0 if unsupported.
   */
  static std::size_t bytes_per_sample(int format) {
    switch (format) {
    case AFMT_S16_LE:
    case AFMT_S16_BE:
      return 2;
    case AFMT_S24_LE:
    case AFMT_S24_BE:
      return 3;
    case AFMT_S32_LE:
    case AFMT_S32_BE:
      return 4;
    default:
      return 0;
    }
  }

  //! Always close device before destruction.
  ~Device() { close(); }

  //! Effective OSS sample format, see sys/soundcard.h header.
  int sample_format() const { return _sample_format; }

  //! Effective sample size in bytes.
  std::size_t bytes_per_sample() const {
    return bytes_per_sample(_sample_format);
  }

  //! Indicate that the device is open.
  bool is_open() const { return _fd >= 0; }

  //! Indicate that the device is opened in playback mode.
  bool playback() const { return _fd >= 0 && (_file_mode & O_WRONLY); }

  //! Indicate that the device is opened in recording mode.
  bool recording() const { return _fd >= 0 && !playback(); }

  //! Get the file descriptor of the device, -1 if not open.
  int file_descriptor() const { return _fd; }

  //! Effective number of audio channels.
  unsigned channels() const { return _channels; }

  //! Effective frame size, one sample for each channel.
  std::size_t frame_size() const { return _channels * bytes_per_sample(); }

  //! Effective OSS buffer size in bytes.
  std::size_t buffer_size() const { return _fragments * _fragment_size; }

  //! Effective OSS buffer size in frames, samples per channel.
  unsigned buffer_frames() const { return buffer_size() / frame_size(); }

  //! Effective sample rate in Hz.
  unsigned sample_rate() const { return _sample_rate; }

  //! Suggested minimal polling step, in frames.
  unsigned stepping() const { return 16U * (1U + (_sample_rate / 50000)); }

  //! Indicate that the OSS buffer can be memory mapped.
  bool can_memory_map() const { return has_capability(PCM_CAP_MMAP); }

  //! A pointer to the memory mapped OSS buffer, null if not mapped.
  char *map() const { return static_cast<char *>(_map); }

  //! Current read / write position in the mapped OSS buffer.
  unsigned map_pointer() const { return _map_progress % buffer_size(); }

  //! Total progress of the mapped OSS buffer, in frames.
  std::int64_t map_progress() const { return _map_progress / frame_size(); }

  /*!
   * \brief Set preferred audio parameters before opening device.
   * \param format OSS sample formet, see sys/soundcard.h header.
   * \param rate Sample rate in Hz.
   * \param channels Number of recording / playback channels.
   * \return True if successful, false means unsupported parameters.
   */
  bool set_parameters(int format, int rate, int channels) {
    if (bytes_per_sample(format) && channels > 0) {
      _sample_format = format;
      _sample_rate = rate;
      _channels = channels;
      return true;
    }
    return false;
  }

  /*!
   * \brief Open the device for either recording or playback.
   * \param device Path to the OSS device (e.g. "/dev/dsp1").
   * \param mode Open mode read or write, optional exclusive and non-blocking.
   * \return True if successful.
   */
  bool open(const char *device, int mode) {
    if (mode & O_RDWR) {
      Log::warn(SOSSO_LOC, "Only one direction allowed, open %s in read mode.",
                device);
      mode = O_RDONLY | (mode & O_EXCL) | (mode & O_NONBLOCK);
    }
    _fd = ::open(device, mode);
    if (_fd >= 0) {
      _file_mode = mode;
      if (bitperfect_mode(_fd) && set_sample_format(_fd) && set_channels(_fd) &&
          set_sample_rate(_fd) && get_buffer_info() && get_capabilities()) {
        return true;
      }
    }
    Log::warn(SOSSO_LOC, "Unable to open device %s, errno %d.", device, errno);
    close();
    return false;
  }

  //! Close the device.
  void close() {
    if (map()) {
      memory_unmap();
    }
    if (_fd >= 0) {
      ::close(_fd);
      _fd = -1;
    }
  }

  /*!
   * \brief Request a specific OSS buffer size.
   * \param fragments Number of fragments.
   * \param fragment_size Size of the fragments in bytes.
   * \return True if successful.
   * \warning Due to OSS API limitations, resulting buffer sizes are not really
   *          predictable and may cause problems with some soundcards.
   */
  bool set_buffer_size(unsigned fragments, unsigned fragment_size) {
    int frg = 0;
    while ((1U << frg) < fragment_size) {
      ++frg;
    }
    frg |= (fragments << 16);
    Log::info(SOSSO_LOC, "Request %d fragments of %u bytes.", (frg >> 16),
              (1U << (frg & 0xffff)));
    if (ioctl(_fd, SNDCTL_DSP_SETFRAGMENT, &frg) != 0) {
      Log::warn(SOSSO_LOC, "Set fragments failed with %d.", errno);
      return false;
    }
    return get_buffer_info();
  }

  /*!
   * \brief Request a specific OSS buffer size.
   * \param total_size Total size of all buffer fragments.
   * \return True if successful.
   * \warning Due to OSS API limitations, resulting buffer sizes are not really
   *          predictable and may cause problems with some soundcards.
   */
  bool set_buffer_size(unsigned total_size) {
    if (_fragment_size > 0) {
      unsigned fragments = (total_size + _fragment_size - 1) / _fragment_size;
      return set_buffer_size(fragments, _fragment_size);
    }
    return false;
  }

  /*!
   * \brief Read recorded audio data from OSS buffer.
   * \param buffer Pointer to destination buffer.
   * \param length Maximum read length in bytes.
   * \param count Byte counter, increased by effective read length.
   * \return True if successful or if nothing to do.
   */
  bool read_io(char *buffer, std::size_t length, std::size_t &count) {
    if (buffer && length > 0 && recording()) {
      ssize_t result = ::read(_fd, buffer, length);
      if (result >= 0) {
        count += result;
      } else if (errno == EAGAIN) {
        count += 0;
      } else {
        Log::warn(SOSSO_LOC, "Data read failed with %d.", errno);
        return false;
      }
    }
    return true;
  }

  /*!
   * \brief Read recorded audio data from memory mapped OSS buffer.
   * \param buffer Pointer to destination buffer.
   * \param offset Read offset into the OSS buffer, in bytes.
   * \param length Maximum read length in bytes.
   * \return The number of bytes read.
   */
  std::size_t read_map(char *buffer, std::size_t offset, std::size_t length) {
    std::size_t bytes_read = 0;
    if (length > 0 && map()) {
      // Sanitize offset and length parameters.
      offset = offset % buffer_size();
      if (length > buffer_size()) {
        length = buffer_size();
      }
      // Check if the read length spans across an OSS buffer cycle.
      if (offset + length > buffer_size()) {
        // Read until buffer end first.
        bytes_read = read_map(buffer, offset, buffer_size() - offset);
        length -= bytes_read;
        buffer += bytes_read;
        offset = 0;
      }
      // Read remaining data.
      std::memcpy(buffer, map() + offset, length);
      bytes_read += length;
    }
    return bytes_read;
  }

  /*!
   * \brief Write audio data to OSS buffer.
   * \param buffer Pointer to source buffer.
   * \param length Maximum write length in bytes.
   * \param count Byte counter, increased by effective write length.
   * \return True if successful or if nothing to do.
   */
  bool write_io(char *buffer, std::size_t length, std::size_t &count) {
    if (buffer && length > 0 && playback()) {
      ssize_t result = ::write(file_descriptor(), buffer, length);
      if (result >= 0) {
        count += result;
      } else if (errno == EAGAIN) {
        count += 0;
      } else {
        Log::warn(SOSSO_LOC, "Data write failed with %d.", errno);
        return false;
      }
    }
    return true;
  }

  /*!
   * \brief Write audio data to a memory mapped OSS buffer.
   * \param buffer Pointer to source buffer, null writes zeros to OSS buffer.
   * \param offset Write offset into the OSS buffer, in bytes.
   * \param length Maximum write length in bytes.
   * \return The number of bytes written.
   */
  std::size_t write_map(const char *buffer, std::size_t offset,
                        std::size_t length) {
    std::size_t bytes_written = 0;
    if (length > 0 && map()) {
      // Sanitize pointer and length parameters.
      offset = offset % buffer_size();
      if (length > buffer_size()) {
        length = buffer_size();
      }
      // Check if the write length spans across an OSS buffer cycle.
      if (offset + length > buffer_size()) {
        // Write until buffer end first.
        bytes_written += write_map(buffer, offset, buffer_size() - offset);
        length -= bytes_written;
        if (buffer) {
          buffer += bytes_written;
        }
        offset = 0;
      }
      // Write source if available, otherwise clear the buffer.
      if (buffer) {
        std::memcpy(map() + offset, buffer, length);
      } else {
        std::memset(map() + offset, 0, length);
      }
      bytes_written += length;
    }
    return bytes_written;
  }

  /*!
   * \brief Query number of frames in the OSS buffer (non-mapped).
   * \return Number of frames, 0 if not successful.
   */
  int queued_samples() {
    unsigned long request =
        playback() ? SNDCTL_DSP_CURRENT_OPTR : SNDCTL_DSP_CURRENT_IPTR;
    oss_count_t ptr;
    if (ioctl(_fd, request, &ptr) == 0) {
      return ptr.fifo_samples;
    }
    return 0;
  }

  //! Indicate that the device can be triggered to start.
  bool can_trigger() const { return has_capability(PCM_CAP_TRIGGER); }

  //! Trigger the device to start recording / playback.
  bool start() const {
    if (!can_trigger()) {
      Log::warn(SOSSO_LOC, "Trigger start not supported by device.");
      return false;
    }
    int trigger = recording() ? PCM_ENABLE_INPUT : PCM_ENABLE_OUTPUT;
    if (ioctl(file_descriptor(), SNDCTL_DSP_SETTRIGGER, &trigger) != 0) {
      const char *direction = recording() ? "recording" : "playback";
      Log::warn(SOSSO_LOC, "Starting %s channel failed with error %d.",
                direction, errno);
      return false;
    }
    return true;
  }

  /*!
   * \brief Add device to a sync group for synchronized start.
   * \param id Id of the sync group, 0 will initialize a new group.
   * \return True if successful.
   */
  bool add_to_sync_group(int &id) {
    oss_syncgroup sync_group = {0, 0, {0}};
    sync_group.id = id;
    sync_group.mode |= (recording() ? PCM_ENABLE_INPUT : PCM_ENABLE_OUTPUT);
    if (ioctl(file_descriptor(), SNDCTL_DSP_SYNCGROUP, &sync_group) == 0 &&
        (id == 0 || sync_group.id == id)) {
      id = sync_group.id;
      return true;
    }
    Log::warn(SOSSO_LOC, "Sync grouping channel failed with error %d.", errno);
    return false;
  }

  /*!
   * \brief Synchronized start of all devices in the sync group.
   * \param id Id of the sync group.
   * \return True if successful.
   */
  bool start_sync_group(int id) {
    if (ioctl(file_descriptor(), SNDCTL_DSP_SYNCSTART, &id) == 0) {
      return true;
    }
    Log::warn(SOSSO_LOC, "Start of sync group failed with error %d.", errno);
    return false;
  }

  //! Query the number of playback underruns since last called.
  int get_play_underruns() {
    int play_underruns = 0;
    int rec_overruns = 0;
    get_errors(play_underruns, rec_overruns);
    return play_underruns;
  }

  //! Query the number of recording overruns since last called.
  int get_rec_overruns() {
    int play_underruns = 0;
    int rec_overruns = 0;
    get_errors(play_underruns, rec_overruns);
    return rec_overruns;
  }

  //! Update current playback position for memory mapped OSS buffer.
  bool get_play_pointer() {
    count_info info = {};
    if (ioctl(file_descriptor(), SNDCTL_DSP_GETOPTR, &info) == 0) {
      if (info.ptr >= 0 && static_cast<unsigned>(info.ptr) < buffer_size() &&
          (info.ptr % frame_size()) == 0 && info.blocks >= 0) {
        // Calculate pointer delta without complete buffer cycles.
        unsigned delta =
            (info.ptr + buffer_size() - map_pointer()) % buffer_size();
        // Get upper bound on progress from blocks info.
        unsigned max_bytes = (info.blocks + 1) * _fragment_size - 1;
        if (max_bytes >= delta) {
          // Estimate cycle part and round it down to buffer cycles.
          unsigned cycles = max_bytes - delta;
          cycles -= (cycles % buffer_size());
          delta += cycles;
        }
        int fragments = delta / _fragment_size;
        if (info.blocks < fragments || info.blocks > fragments + 1) {
          Log::warn(SOSSO_LOC, "Play pointer blocks: %u - %d, %d, %d.",
                    map_pointer(), info.ptr, info.blocks, info.bytes);
        }
        _map_progress += delta;
        return true;
      }
      Log::warn(SOSSO_LOC, "Play pointer out of bounds: %d, %d blocks.",
                info.ptr, info.blocks);
    } else {
      Log::warn(SOSSO_LOC, "Play pointer failed with error: %d.", errno);
    }
    return false;
  }

  //! Update current recording position for memory mapped OSS buffer.
  bool get_rec_pointer() {
    count_info info = {};
    if (ioctl(file_descriptor(), SNDCTL_DSP_GETIPTR, &info) == 0) {
      if (info.ptr >= 0 && static_cast<unsigned>(info.ptr) < buffer_size() &&
          (info.ptr % frame_size()) == 0 && info.blocks >= 0) {
        // Calculate pointer delta without complete buffer cycles.
        unsigned delta =
            (info.ptr + buffer_size() - map_pointer()) % buffer_size();
        // Get upper bound on progress from blocks info.
        unsigned max_bytes = (info.blocks + 1) * _fragment_size - 1;
        if (max_bytes >= delta) {
          // Estimate cycle part and round it down to buffer cycles.
          unsigned cycles = max_bytes - delta;
          cycles -= (cycles % buffer_size());
          delta += cycles;
        }
        int fragments = delta / _fragment_size;
        if (info.blocks < fragments || info.blocks > fragments + 1) {
          Log::warn(SOSSO_LOC, "Rec pointer blocks: %u - %d, %d, %d.",
                    map_pointer(), info.ptr, info.blocks, info.bytes);
        }
        _map_progress += delta;
        return true;
      }
      Log::warn(SOSSO_LOC, "Rec pointer out of bounds: %d, %d blocks.",
                info.ptr, info.blocks);
    } else {
      Log::warn(SOSSO_LOC, "Rec pointer failed with error: %d.", errno);
    }
    return false;
  }

  //! Memory map the OSS buffer.
  bool memory_map() {
    if (!can_memory_map()) {
      Log::warn(SOSSO_LOC, "Memory map not supported by device.");
      return false;
    }
    int protection = PROT_NONE;
    if (playback()) {
      protection = PROT_WRITE;
    }
    if (recording()) {
      protection = PROT_READ;
    }
    if (_map == nullptr && protection != PROT_NONE) {
      _map = mmap(NULL, buffer_size(), protection, MAP_SHARED,
                  file_descriptor(), 0);
      if (_map == MAP_FAILED) {
        Log::warn(SOSSO_LOC, "Memory map failed with error %d.", errno);
        _map = nullptr;
      }
    }
    return (_map != nullptr);
  }

  //! Unmap a previously memory mapped OSS buffer.
  bool memory_unmap() {
    if (_map) {
      if (munmap(_map, buffer_size()) != 0) {
        Log::warn(SOSSO_LOC, "Memory unmap failed with error %d.", errno);
        return false;
      }
      _map = nullptr;
    }
    return true;
  }

  /*!
   * \brief Check device capabilities.
   * \param capabilities Device capabilities, see sys/soundcard.h header.
   * \return True if the device has the capabilities in question.
   */
  bool has_capability(int capabilities) const {
    return (_capabilities & capabilities) == capabilities;
  }

  //! Print device info to user information log.
  void log_device_info() const {
    if (!is_open()) {
      return;
    }
    const char *direction = (recording() ? "Recording" : "Playback");
    Log::info(SOSSO_LOC, "%s device is %u channels at %u Hz, %lu bits.",
              direction, _channels, _sample_rate, bytes_per_sample() * 8);
    Log::info(SOSSO_LOC, "Device buffer is %u fragments of size %u, %u frames.",
              _fragments, _fragment_size, buffer_frames());
    oss_sysinfo sys_info = {};
    if (ioctl(_fd, SNDCTL_SYSINFO, &sys_info) == 0) {
      Log::info(SOSSO_LOC, "OSS version %s number %d on %s.", sys_info.version,
                sys_info.versionnum, sys_info.product);
    }
    Log::info(SOSSO_LOC, "PCM capabilities:");
    if (has_capability(PCM_CAP_TRIGGER))
      Log::info(SOSSO_LOC, "  PCM_CAP_TRIGGER (Trigger start)");
    if (has_capability(PCM_CAP_MMAP))
      Log::info(SOSSO_LOC, "  PCM_CAP_MMAP (Memory map)");
    if (has_capability(PCM_CAP_MULTI))
      Log::info(SOSSO_LOC, "  PCM_CAP_MULTI (Multiple open)");
    if (has_capability(PCM_CAP_INPUT))
      Log::info(SOSSO_LOC, "  PCM_CAP_INPUT (Recording)");
    if (has_capability(PCM_CAP_OUTPUT))
      Log::info(SOSSO_LOC, "  PCM_CAP_OUTPUT (Playback)");
    if (has_capability(PCM_CAP_VIRTUAL))
      Log::info(SOSSO_LOC, "  PCM_CAP_VIRTUAL (Virtual device)");
    if (has_capability(PCM_CAP_ANALOGIN))
      Log::info(SOSSO_LOC, "  PCM_CAP_ANALOGIN (Analog input)");
    if (has_capability(PCM_CAP_ANALOGOUT))
      Log::info(SOSSO_LOC, "  PCM_CAP_ANALOGOUT (Analog output)");
    if (has_capability(PCM_CAP_DIGITALIN))
      Log::info(SOSSO_LOC, "  PCM_CAP_DIGITALIN (Digital input)");
    if (has_capability(PCM_CAP_DIGITALOUT))
      Log::info(SOSSO_LOC, "  PCM_CAP_DIGITALOUT (Digital output)");
  }

private:
  // Disable auto-conversion (bitperfect) when opened in exclusive mode.
  bool bitperfect_mode(int fd) {
    if (_file_mode & O_EXCL) {
      int flags = 0;
      int result = ioctl(fd, SNDCTL_DSP_COOKEDMODE, &flags);
      if (result < 0) {
        Log::warn(SOSSO_LOC, "Unable to set cooked mode.");
      }
      return result >= 0;
    }
    return true;
  }

  // Set sample format and the check the result.
  bool set_sample_format(int fd) {
    int format = _sample_format;
    int result = ioctl(fd, SNDCTL_DSP_SETFMT, &format);
    if (result != 0) {
      Log::warn(SOSSO_LOC, "Unable to set sample format, error %d.", errno);
      return false;
    } else if (bytes_per_sample(format) == 0) {
      Log::warn(SOSSO_LOC, "Unsupported sample format %d.", format);
      return false;
    } else if (format != _sample_format) {
      Log::warn(
          SOSSO_LOC, "Driver changed the sample format, %lu bit vs %lu bit.",
          bytes_per_sample(format) * 8, bytes_per_sample(_sample_format) * 8);
    }
    _sample_format = format;
    return true;
  }

  // Set sample rate and then check the result.
  bool set_sample_rate(int fd) {
    int rate = _sample_rate;
    if (ioctl(fd, SNDCTL_DSP_SPEED, &rate) == 0) {
      if (rate != _sample_rate) {
        Log::warn(SOSSO_LOC, "Driver changed the sample rate, %d vs %d.", rate,
                  _sample_rate);
        _sample_rate = rate;
      }
      return true;
    }
    Log::warn(SOSSO_LOC, "Unable to set sample rate, error %d.", errno);
    return false;
  }

  // Set the number of channels and then check the result.
  bool set_channels(int fd) {
    int channels = _channels;
    if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) == 0) {
      if (channels != _channels) {
        Log::warn(SOSSO_LOC, "Driver changed number of channels, %d vs %d.",
                  channels, _channels);
        _channels = channels;
      }
      return true;
    }
    Log::warn(SOSSO_LOC, "Unable to set channels, error %d.", errno);
    return false;
  }

  // Query fragments and size of the OSS buffer.
  bool get_buffer_info() {
    audio_buf_info info = {0, 0, 0, 0};
    unsigned long request =
        playback() ? SNDCTL_DSP_GETOSPACE : SNDCTL_DSP_GETISPACE;
    if (ioctl(_fd, request, &info) >= 0) {
      _fragments = info.fragstotal;
      _fragment_size = info.fragsize;
      return true;
    } else {
      Log::warn(SOSSO_LOC, "Unable to get buffer info.");
      return false;
    }
  }

  // Query capabilities of the device.
  bool get_capabilities() {
    if (ioctl(_fd, SNDCTL_DSP_GETCAPS, &_capabilities) == 0) {
      oss_sysinfo sysinfo = {};
      if (ioctl(_fd, OSS_SYSINFO, &sysinfo) == 0) {
        if (std::strncmp(sysinfo.version, "1302000", 7) < 0) {
          // Memory map on FreeBSD prior to 13.2 may use wrong buffer size.
          Log::warn(SOSSO_LOC,
                    "Disable memory map, workaround OSS bug on FreeBSD < 13.2");
          _capabilities &= ~PCM_CAP_MMAP;
        }
        return true;
      } else {
        Log::warn(SOSSO_LOC, "Unable to get system info, error %d.", errno);
      }
    } else {
      Log::warn(SOSSO_LOC, "Unable to get device capabilities, error %d.",
                errno);
      _capabilities = 0;
    }
    return false;
  }

  // Query error information from the device.
  bool get_errors(int &play_underruns, int &rec_overruns) {
    audio_errinfo error_info = {};
    if (ioctl(file_descriptor(), SNDCTL_DSP_GETERROR, &error_info) == 0) {
      play_underruns = error_info.play_underruns;
      rec_overruns = error_info.rec_overruns;
      return true;
    }
    return false;
  }

private:
  int _fd = -1;                     // File descriptor.
  int _file_mode = O_RDONLY;        // File open mode.
  void *_map = nullptr;             // Memory map pointer.
  std::uint64_t _map_progress = 0;  // Memory map progress.
  int _channels = 2;                // Number of channels.
  int _capabilities = 0;            // Device capabilities.
  int _sample_format = AFMT_S32_NE; // Sample format.
  int _sample_rate = 48000;         // Sample rate.
  unsigned _fragments = 0;          // Number of OSS buffer fragments.
  unsigned _fragment_size = 0;      // OSS buffer fragment size.
};

} // namespace sosso

#endif // SOSSO_DEVICE_HPP
