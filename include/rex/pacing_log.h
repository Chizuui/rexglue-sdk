#pragma once

// TEKKEN6_PACING_LOG=<file>: frame pacing events, one per line, for
// measurement only. Times are host nanoseconds on the clock Core Animation
// uses for display timestamps (CLOCK_UPTIME_RAW on macOS).
//   v <t>                              guest vblank
//   s <t> shader                       frame not shown: a shader still compiling
//   f <t> <version>                    guest frame handed to the presenter
//   p <t> <present id> <version>       present carrying a new guest frame
//   d <present id> <actual> <earliest> <margin>
//                                      when that present reached the display
//                                      (VK_GOOGLE_display_timing)
// Each library that logs opens the file itself, in append mode.

#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#if defined(__APPLE__)
#include <time.h>
#endif

namespace rex::pacing {

inline FILE* File() {
  static FILE* file = [] {
    const char* path = std::getenv("TEKKEN6_PACING_LOG");
    FILE* f = (path && *path) ? std::fopen(path, "a") : nullptr;
    if (f) {
      std::setvbuf(f, nullptr, _IOLBF, 1 << 12);
    }
    return f;
  }();
  return file;
}

inline bool Enabled() { return File() != nullptr; }

inline uint64_t Now() {
#if defined(__APPLE__)
  return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#else
  return uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
#endif
}

inline void Log(const char* format, ...) {
  FILE* f = File();
  if (!f) {
    return;
  }
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  va_list args;
  va_start(args, format);
  std::vfprintf(f, format, args);
  va_end(args);
}

}  // namespace rex::pacing
