#pragma once
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
 * Android presentation surface: wraps the ANativeWindow SDL creates for its
 * Android video driver. The Vulkan presenter turns this into a
 * VK_KHR_android_surface. (ReXGlue never shipped this file; it is the
 * Android counterpart of surface_gnulinux.h / surface_win.h / surface_mac.h.)
 */

#include <rex/ui/surface.h>

#include <android/native_window.h>

struct SDL_Window;

namespace rex {
namespace ui {

class AndroidNativeWindowSurface final : public Surface {
 public:
  explicit AndroidNativeWindowSurface(ANativeWindow* window, SDL_Window* sdl_window)
      : window_(window), sdl_window_(sdl_window) {}
  TypeIndex GetType() const override { return kTypeIndex_AndroidNativeWindow; }
  ANativeWindow* window() const { return window_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  ANativeWindow* window_;
  SDL_Window* sdl_window_;
};

}  // namespace ui
}  // namespace rex
