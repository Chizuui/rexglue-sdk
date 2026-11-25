// Minimal Android entry-point glue (PROBE STUB).
// Xenia's main_android.h was never ported to ReXGlue, but
// src/core/threading_posix.cpp includes <rex/main_android.h> under
// REX_PLATFORM_ANDROID and uses rex::GetAndroidApiLevel().
// This stub provides only that; a real Android shell will need the
// full NativeActivity entry point (android_main) alongside it.
#pragma once

#include <android/api-level.h>

namespace rex {

// Device API level, e.g. 26 for Android 8.0. Available since API 24.
inline int GetAndroidApiLevel() { return android_get_device_api_level(); }

}  // namespace rex
