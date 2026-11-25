/**
 * @file        rex/input/virtual_gamepad_state.h
 *
 * @brief       Shared state for the on-screen touch gamepad.
 *
 * The overlay (UI thread) writes button/stick state; the virtual input
 * driver (input thread) reads it. Plain atomics: a frame of skew between
 * the button bits and the packet number is harmless for a gamepad.
 */
#pragma once

#include <atomic>
#include <cstdint>

#include <rex/input/input.h>

namespace rex::input {

struct VirtualGamepadState {
  std::atomic<uint16_t> buttons{0};
  std::atomic<int16_t> thumb_lx{0};
  std::atomic<int16_t> thumb_ly{0};
  std::atomic<int16_t> thumb_rx{0};
  std::atomic<int16_t> thumb_ry{0};
  std::atomic<uint8_t> left_trigger{0};
  std::atomic<uint8_t> right_trigger{0};
  std::atomic<uint32_t> packet_number{0};

  // ORs the given button bits in (press).
  void Press(uint16_t mask) {
    buttons.fetch_or(mask);
    packet_number.fetch_add(1);
  }

  // Clears the given button bits (release).
  void Release(uint16_t mask) {
    buttons.fetch_and(static_cast<uint16_t>(~mask));
    packet_number.fetch_add(1);
  }

  // Sets the four D-pad bits to exactly the given values (used by the
  // D-pad zone, which recomputes the direction from the touch position).
  void SetDPad(bool up, bool down, bool left, bool right) {
    uint16_t dpad = 0;
    if (up) dpad |= X_INPUT_GAMEPAD_DPAD_UP;
    if (down) dpad |= X_INPUT_GAMEPAD_DPAD_DOWN;
    if (left) dpad |= X_INPUT_GAMEPAD_DPAD_LEFT;
    if (right) dpad |= X_INPUT_GAMEPAD_DPAD_RIGHT;
    uint16_t old = buttons.load();
    uint16_t updated = (old & ~static_cast<uint16_t>(X_INPUT_GAMEPAD_DPAD_UP |
                                                      X_INPUT_GAMEPAD_DPAD_DOWN |
                                                      X_INPUT_GAMEPAD_DPAD_LEFT |
                                                      X_INPUT_GAMEPAD_DPAD_RIGHT)) |
                       dpad;
    buttons.store(updated);
    packet_number.fetch_add(1);
  }

  void SetLeftStick(int16_t x, int16_t y) {
    thumb_lx.store(x);
    thumb_ly.store(y);
    packet_number.fetch_add(1);
  }

  void SetRightStick(int16_t x, int16_t y) {
    thumb_rx.store(x);
    thumb_ry.store(y);
    packet_number.fetch_add(1);
  }

  void SetLeftTrigger(uint8_t value) {
    left_trigger.store(value);
    packet_number.fetch_add(1);
  }

  void SetRightTrigger(uint8_t value) {
    right_trigger.store(value);
    packet_number.fetch_add(1);
  }

  void Clear() {
    buttons.store(0);
    thumb_lx.store(0);
    thumb_ly.store(0);
    thumb_rx.store(0);
    thumb_ry.store(0);
    left_trigger.store(0);
    right_trigger.store(0);
    packet_number.fetch_add(1);
  }
};

// Process-wide instance: the overlay writes it, the driver reads it.
inline VirtualGamepadState& GetVirtualGamepadState() {
  static VirtualGamepadState state;
  return state;
}

}  // namespace rex::input
