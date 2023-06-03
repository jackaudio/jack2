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

#ifndef SOSSO_BUFFER_HPP
#define SOSSO_BUFFER_HPP

#include <cstring>

namespace sosso {

/*!
 * \brief Buffer Management
 *
 * Provides means to access and manipulate externally allocated buffer memory.
 * It stores a memory pointer, length and a read / write position. The buffer
 * memory can be passed from one Buffer instance to another, through move
 * constructor and move assignment. This prevents multiple Buffer instances from
 * referencing the same memory.
 */
class Buffer {
public:
  //! Construct an empty and invalid Buffer.
  Buffer() = default;

  /*!
   * \brief Construct Buffer operating on given memory.
   * \param buffer Pointer to the externally allocated memory.
   * \param length Length of the memory dedicated to this Buffer.
   */
  Buffer(char *buffer, std::size_t length)
      : _data(buffer), _length(length), _position(0) {}

  /*!
   * \brief Move construct a buffer.
   * \param other Adopt memory from this Buffer, leaving it empty.
   */
  Buffer(Buffer &&other) noexcept
      : _data(other._data), _length(other._length), _position(other._position) {
    other._data = nullptr;
    other._position = 0;
    other._length = 0;
  }

  /*!
   * \brief Move assign memory from another Buffer.
   * \param other Adopt memory from this Buffer, leaving it empty.
   * \return This newly assigned Buffer.
   */
  Buffer &operator=(Buffer &&other) {
    _data = other._data;
    _position = other._position;
    _length = other._length;
    other._data = nullptr;
    other._position = 0;
    other._length = 0;
    return *this;
  }

  //! Buffer is valid if the memory is accessable.
  bool valid() const { return (_data != nullptr) && (_length > 0); }

  //! Access the underlying memory, null if invalid.
  char *data() const { return _data; }

  //! Length of the underlying memory in bytes, 0 if invalid.
  std::size_t length() const { return _length; }

  //! Access buffer memory at read / write position.
  char *position() const { return _data + _position; }

  //! Get read / write progress from buffer start, in bytes.
  std::size_t progress() const { return _position; }

  //! Remaining buffer memory in bytes.
  std::size_t remaining() const { return _length - _position; }

  /*!
   * \brief Cap given progress by remaining buffer memory.
   * \param progress Progress in bytes.
   * \return Progress limited by the remaining buffer memory.
   */
  std::size_t remaining(std::size_t progress) const {
    if (progress > remaining()) {
      progress = remaining();
    }
    return progress;
  }

  //! Indicate that the buffer is fully processed.
  bool done() const { return _position == _length; }

  //! Advance the buffer read / write position.
  std::size_t advance(std::size_t progress) {
    progress = remaining(progress);
    _position += progress;
    return progress;
  }

  //! Rewind the buffer read / write position.
  std::size_t rewind(std::size_t progress) {
    if (progress > _position) {
      progress = _position;
    }
    _position -= progress;
    return progress;
  }

  /*!
   * \brief Erase an already processed part, rewind.
   * \param begin Start position of the region to be erased.
   * \param end End position of the region to be erased.
   * \return The number of bytes that were effectively erased.
   */
  std::size_t erase(std::size_t begin, std::size_t end) {
    if (begin < _position && begin < end) {
      if (end > _position) {
        end = _position;
      }
      std::size_t copy = _position - end;
      if (copy > 0) {
        std::memmove(_data + begin, _data + end, copy);
      }
      _position -= (end - begin);
      return (end - begin);
    }
    return 0;
  }

  //! Reset the buffer position to zero.
  void reset() { _position = 0; }

private:
  char *_data = nullptr;     // External buffer memory, null if invalid.
  std::size_t _length = 0;   // Total length of the buffer memory.
  std::size_t _position = 0; // Current read / write position.
};

} // namespace sosso

#endif // SOSSO_BUFFER_HPP
