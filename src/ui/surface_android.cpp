/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 *
 * Android presentation surface implementation (see surface_android.h).
 */

#include <android/native_window.h>

#include <SDL3/SDL.h>

#include <rex/logging.h>
#include <rex/ui/surface_android.h>

namespace rex {
namespace ui {

bool AndroidNativeWindowSurface::GetSizeImpl(uint32_t& width_out,
                                             uint32_t& height_out) const {
  if (!window_) {
    return false;
  }
  int32_t w = ANativeWindow_getWidth(window_);
  int32_t h = ANativeWindow_getHeight(window_);
  if (w <= 0 || h <= 0) {
    REXLOG_INFO("AndroidNativeWindowSurface: Drawable size is {}x{}", w, h);
    return false;
  }
  width_out = uint32_t(w);
  height_out = uint32_t(h);
  return true;
}

}  // namespace ui
}  // namespace rex
