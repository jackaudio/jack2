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

#ifndef SOSSO_LOGGING_HPP
#define SOSSO_LOGGING_HPP

#include <cstdint>
#include <cstdio>

namespace sosso {

/*!
 * \brief Store the source location for logging.
 *
 * Keep its implementation close to C++20 std::source_location.
 * It will be replaced by that when C++20 is widely available.
 */
struct SourceLocation {
  //! Get the line number in the source file.
  std::uint_least32_t line() const { return _line; }
  //! Get the column in the source file, not implemented.
  std::uint_least32_t column() const { return _column; }
  //! Get the file name of the source file.
  const char *file_name() const { return _file_name; }
  //! Get the function context in the source file.
  const char *function_name() const { return _function_name; }

  std::uint_least32_t _line;
  std::uint_least32_t _column;
  const char *_file_name;
  const char *_function_name;
};

/// Capture source location in place of this macro.
#define SOSSO_LOC                                                              \
  SourceLocation { __LINE__, 0, __FILE__, __func__ }

/*!
 * \brief Static logging functions.
 *
 * There are three log levels:
 *  - warn() indicates warnings and errors.
 *  - info() provides general information to the user.
 *  - log() is for low-level information and debugging purposes.
 *
 * The single message static logging functions have to be implemented in the
 * application, so they output to the appropriate places. Otherwise there will
 * be a linking error at build time. To give some context for debugging, the
 * source location is given.
 *
 * For printf-style message composition use the corresponding variable argument
 * function templates, limited to 255 character length.
 */
class Log {
public:
  //! Single message low-level log, implement this in the application.
  static void log(SourceLocation location, const char *message);

  //! Compose printf-style low-level log messages.
  template <typename... Args>
  static void log(SourceLocation location, const char *message, Args... args) {
    char formatted[256];
    std::snprintf(formatted, 256, message, args...);
    log(location, formatted);
  }

  //! Single message user information, implement this in the application.
  static void info(SourceLocation location, const char *message);

  //! Compose printf-style user information messages.
  template <typename... Args>
  static void info(SourceLocation location, const char *message, Args... args) {
    char formatted[256];
    std::snprintf(formatted, 256, message, args...);
    info(location, formatted);
  }

  //! Single message warning, implement this in the application.
  static void warn(SourceLocation location, const char *message);

  //! Compose printf-style warning messages.
  template <typename... Args>
  static void warn(SourceLocation location, const char *message, Args... args) {
    char formatted[256];
    std::snprintf(formatted, 256, message, args...);
    warn(location, formatted);
  }
};

} // namespace sosso

#endif // SOSSO_LOGGING_HPP
