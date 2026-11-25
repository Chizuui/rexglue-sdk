/**
 * @file        rex/input/virtual/virtual_gamepad_driver.h
 *
 * @brief       Input driver for the on-screen touch gamepad (Android).
 *
 * Presents a single synthetic XInput gamepad whose state is written by
 * the touch overlay (see rex/input/virtual_gamepad_state.h). The input
 * system merges it with the other drivers, so it works alongside a
 * physical Bluetooth/USB pad.
 */
#pragma once

#include <rex/input/input_driver.h>

namespace rex::input::vgamepad {

class VirtualGamepadDriver : public InputDriver {
 public:
  VirtualGamepadDriver(rex::ui::Window* window, size_t window_z_order);
  ~VirtualGamepadDriver() override;

  X_STATUS Setup() override;
  void EnumerateDevices(std::vector<DeviceInfo>& out) override;
  X_RESULT GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) override;
  X_RESULT GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                 X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetDeviceKeystroke(DeviceId id, uint32_t flags,
                              X_INPUT_KEYSTROKE* out_keystroke) override;
};

}  // namespace rex::input::vgamepad
