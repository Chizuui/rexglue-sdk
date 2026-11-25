/**
 * @file        rex/ui/overlay/virtual_gamepad_overlay.h
 *
 * @brief       On-screen touch gamepad overlay (Android).
 *
 * Draws the gamepad (D-pad, face buttons, shoulders, Start/Back, menu)
 * with ImGui and consumes multi-touch events before ImGui's mouse
 * emulation sees them, writing button state into the shared virtual
 * gamepad state (rex/input/virtual_gamepad_state.h).
 */
#pragma once

#include <cstdint>
#include <functional>
#include <map>

#include <rex/ui/imgui_dialog.h>
#include <rex/ui/window_listener.h>

namespace rex::ui {

class VirtualGamepadOverlayDialog : public ImGuiDialog, public WindowInputListener {
 public:
  // z_order 128: above the ImGuiDrawer (64) so touches on the pad are
  // consumed before ImGui turns them into mouse clicks.
  static constexpr size_t kInputZOrder = 128;

  using MenuCallback = std::function<void()>;

  explicit VirtualGamepadOverlayDialog(ImGuiDrawer* imgui_drawer,
                                       MenuCallback on_menu = {});
  ~VirtualGamepadOverlayDialog() override;

  void OnTouchEvent(TouchEvent& e) override;

 protected:
  void OnDraw(ImGuiIO& io) override;

 private:
  enum class Control : uint8_t {
    kNone,
    kDPad,
    kLeftStick,
    kRightStick,
    kA,
    kB,
    kX,
    kY,
    kLeftShoulder,
    kRightShoulder,
    kLeftTrigger,
    kRightTrigger,
    kStart,
    kBack,
    kGuide,
    kMenu,
  };

  struct Layout {
    float width = 0;
    float height = 0;
    float r = 0;  // button radius
    // D-pad zone (square) and the two analog sticks (circles).
    float dpad_cx = 0, dpad_cy = 0, dpad_half = 0;
    float lstick_cx = 0, lstick_cy = 0, lstick_r = 0;
    float rstick_cx = 0, rstick_cy = 0, rstick_r = 0;
    // Face buttons (diamond around a center).
    float face_cx = 0, face_cy = 0;
    // Shoulders / triggers.
    float lb_cx = 0, lb_cy = 0, rb_cx = 0, rb_cy = 0;
    float lt_cx = 0, lt_cy = 0, rt_cx = 0, rt_cy = 0;
    // Start / back / guide / menu.
    float start_cx = 0, start_cy = 0, back_cx = 0, back_cy = 0;
    float guide_cx = 0, guide_cy = 0;
    float menu_cx = 0, menu_cy = 0;
  };

  Layout ComputeLayout(float width, float height) const;
  Control HitTest(const Layout& layout, float x, float y) const;
  void UpdateDPad(const Layout& layout, float x, float y);
  void UpdateStick(Control stick, const Layout& layout, float x, float y);
  void PressControl(Control control, const Layout& layout, float x, float y);
  void ReleaseControl(Control control);

  MenuCallback on_menu_;
  // Active touches: pointer id -> control held.
  std::map<uint32_t, Control> active_touches_;
  // Current D-pad direction bits, so a slide updates only what changed.
  uint16_t dpad_bits_ = 0;
  // Pressed state per control (for drawing): bit (1 << (int)control).
  uint32_t pressed_controls_ = 0;
  // Analog stick nub offsets (normalized -1..1) for drawing.
  float lstick_nx_ = 0, lstick_ny_ = 0;
  float rstick_nx_ = 0, rstick_ny_ = 0;
  // Last display size seen in OnDraw (touch events need the layout).
  float last_width_ = 0;
  float last_height_ = 0;
};

}  // namespace rex::ui
