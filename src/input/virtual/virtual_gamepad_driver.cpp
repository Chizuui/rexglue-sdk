/**
 * @file        input/virtual/virtual_gamepad_driver.cpp
 *
 * @brief       Input driver for the on-screen touch gamepad (Android).
 */
#include <cstring>

#include <rex/input/flags.h>
#include <rex/input/virtual/virtual_gamepad_driver.h>
#include <rex/input/virtual_gamepad_state.h>
#include <rex/logging.h>

namespace rex::input::vgamepad {

namespace {

// A single device, so its handle is a constant.
constexpr rex::input::DeviceId kVirtualGamepadDevice =
    static_cast<rex::input::DeviceId>(0x56474144);  // "VGAD"

}  // namespace

VirtualGamepadDriver::VirtualGamepadDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {}

VirtualGamepadDriver::~VirtualGamepadDriver() = default;

X_STATUS VirtualGamepadDriver::Setup() {
  return X_STATUS_SUCCESS;
}

void VirtualGamepadDriver::EnumerateDevices(std::vector<DeviceInfo>& out) {
  DeviceInfo info;
  info.id = kVirtualGamepadDevice;
  info.name = "Virtual Gamepad";
  info.synthetic = true;
  out.push_back(info);
}

X_RESULT VirtualGamepadDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) {
  if (id != kVirtualGamepadDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (!out_state) {
    return X_ERROR_BAD_ARGUMENTS;
  }
  const auto& state = GetVirtualGamepadState();
  std::memset(out_state, 0, sizeof(*out_state));
  out_state->packet_number = state.packet_number.load();
  out_state->gamepad.buttons = state.buttons.load();
  out_state->gamepad.left_trigger = state.left_trigger.load();
  out_state->gamepad.right_trigger = state.right_trigger.load();
  out_state->gamepad.thumb_lx = state.thumb_lx.load();
  out_state->gamepad.thumb_ly = state.thumb_ly.load();
  out_state->gamepad.thumb_rx = state.thumb_rx.load();
  out_state->gamepad.thumb_ry = state.thumb_ry.load();
  return X_ERROR_SUCCESS;
}

X_RESULT VirtualGamepadDriver::GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                                     X_INPUT_CAPABILITIES* out_caps) {
  if (id != kVirtualGamepadDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  if (out_caps) {
    std::memset(out_caps, 0, sizeof(*out_caps));
    out_caps->type = 0x01;      // XINPUT_DEVTYPE_GAMEPAD
    out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD
    out_caps->flags = 0;
    out_caps->gamepad.buttons = 0xFFFF;  // All buttons supported
    out_caps->gamepad.left_trigger = 0xFF;
    out_caps->gamepad.right_trigger = 0xFF;
    out_caps->gamepad.thumb_lx = static_cast<int16_t>(0x7FFF);
    out_caps->gamepad.thumb_ly = static_cast<int16_t>(0x7FFF);
    out_caps->gamepad.thumb_rx = static_cast<int16_t>(0x7FFF);
    out_caps->gamepad.thumb_ry = static_cast<int16_t>(0x7FFF);
    out_caps->vibration.left_motor_speed = 0;
    out_caps->vibration.right_motor_speed = 0;
  }
  return X_ERROR_SUCCESS;
}

X_RESULT VirtualGamepadDriver::SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) {
  if (id != kVirtualGamepadDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  // No motors on a touch screen.
  return X_ERROR_SUCCESS;
}

X_RESULT VirtualGamepadDriver::GetDeviceKeystroke(DeviceId id, uint32_t flags,
                                                  X_INPUT_KEYSTROKE* out_keystroke) {
  if (id != kVirtualGamepadDevice) {
    return X_ERROR_DEVICE_NOT_CONNECTED;
  }
  return X_ERROR_EMPTY;
}

}  // namespace rex::input::vgamepad
