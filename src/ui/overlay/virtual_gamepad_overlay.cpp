/**
 * @file        ui/overlay/virtual_gamepad_overlay.cpp
 *
 * @brief       On-screen touch gamepad overlay (Android).
 *
 * Full Xbox layout: D-pad + left stick on the left, face buttons + right
 * stick on the right, LT/RT + LB/RB shoulders, View/Guide/Menu in the
 * middle, and a small menu button for the FPS overlay. All controls are
 * drawn neutral (white/gray) so they stay readable over any scene.
 */
#include <rex/ui/overlay/virtual_gamepad_overlay.h>

#include <algorithm>
#include <cmath>

#include <imgui.h>

#include <rex/input/input.h>
#include <rex/input/virtual_gamepad_state.h>

namespace rex::ui {

VirtualGamepadOverlayDialog::VirtualGamepadOverlayDialog(ImGuiDrawer* imgui_drawer,
                                                         MenuCallback on_menu)
    : ImGuiDialog(imgui_drawer), on_menu_(std::move(on_menu)) {}

VirtualGamepadOverlayDialog::~VirtualGamepadOverlayDialog() {}

VirtualGamepadOverlayDialog::Layout VirtualGamepadOverlayDialog::ComputeLayout(float width,
                                                                              float height) const {
  Layout l;
  l.width = width;
  l.height = height;
  l.r = std::min(width, height) * 0.055f;
  const float r = l.r;
  // Left cluster: D-pad above, left stick below.
  l.dpad_cx = width * 0.12f;
  l.dpad_cy = height * 0.50f;
  l.dpad_half = 2.2f * r;
  l.lstick_cx = width * 0.12f;
  l.lstick_cy = height * 0.85f;
  l.lstick_r = 1.9f * r;
  // Right cluster: face buttons above, right stick below.
  l.face_cx = width * 0.88f;
  l.face_cy = height * 0.50f;
  l.rstick_cx = width * 0.88f;
  l.rstick_cy = height * 0.85f;
  l.rstick_r = 1.9f * r;
  // Shoulders: triggers on the top row, bumpers below them.
  l.lt_cx = width * 0.10f;
  l.lt_cy = height * 0.05f;
  l.rt_cx = width * 0.90f;
  l.rt_cy = height * 0.05f;
  l.lb_cx = width * 0.10f;
  l.lb_cy = height * 0.14f;
  l.rb_cx = width * 0.90f;
  l.rb_cy = height * 0.14f;
  // Middle: View (back), Guide, Menu (start).
  l.back_cx = width * 0.44f;
  l.back_cy = height * 0.07f;
  l.guide_cx = width * 0.50f;
  l.guide_cy = height * 0.07f;
  l.start_cx = width * 0.56f;
  l.start_cy = height * 0.07f;
  // FPS menu button (top-right corner).
  l.menu_cx = width * 0.97f;
  l.menu_cy = height * 0.05f;
  return l;
}

VirtualGamepadOverlayDialog::Control VirtualGamepadOverlayDialog::HitTest(const Layout& l,
                                                                          float x, float y) const {
  auto in_circle = [](float cx, float cy, float radius, float x, float y) {
    float dx = x - cx;
    float dy = y - cy;
    return dx * dx + dy * dy <= radius * radius;
  };
  auto in_rect = [](float cx, float cy, float hw, float hh, float x, float y) {
    return std::abs(x - cx) <= hw && std::abs(y - cy) <= hh;
  };

  if (in_circle(l.menu_cx, l.menu_cy, l.r * 0.9f, x, y)) {
    return Control::kMenu;
  }
  if (in_circle(l.guide_cx, l.guide_cy, l.r * 1.0f, x, y)) {
    return Control::kGuide;
  }
  if (in_rect(l.start_cx, l.start_cy, 1.3f * l.r, 0.7f * l.r, x, y)) {
    return Control::kStart;
  }
  if (in_rect(l.back_cx, l.back_cy, 1.3f * l.r, 0.7f * l.r, x, y)) {
    return Control::kBack;
  }
  if (in_rect(l.lt_cx, l.lt_cy, 1.4f * l.r, 0.7f * l.r, x, y)) {
    return Control::kLeftTrigger;
  }
  if (in_rect(l.rt_cx, l.rt_cy, 1.4f * l.r, 0.7f * l.r, x, y)) {
    return Control::kRightTrigger;
  }
  if (in_rect(l.lb_cx, l.lb_cy, 1.4f * l.r, 0.7f * l.r, x, y)) {
    return Control::kLeftShoulder;
  }
  if (in_rect(l.rb_cx, l.rb_cy, 1.4f * l.r, 0.7f * l.r, x, y)) {
    return Control::kRightShoulder;
  }
  // Face buttons (diamond around the face center).
  if (in_circle(l.face_cx, l.face_cy - 2 * l.r, l.r * 1.15f, x, y)) {
    return Control::kY;
  }
  if (in_circle(l.face_cx, l.face_cy + 2 * l.r, l.r * 1.15f, x, y)) {
    return Control::kA;
  }
  if (in_circle(l.face_cx - 2 * l.r, l.face_cy, l.r * 1.15f, x, y)) {
    return Control::kX;
  }
  if (in_circle(l.face_cx + 2 * l.r, l.face_cy, l.r * 1.15f, x, y)) {
    return Control::kB;
  }
  // Analog sticks.
  if (in_circle(l.lstick_cx, l.lstick_cy, l.lstick_r * 1.15f, x, y)) {
    return Control::kLeftStick;
  }
  if (in_circle(l.rstick_cx, l.rstick_cy, l.rstick_r * 1.15f, x, y)) {
    return Control::kRightStick;
  }
  // D-pad zone (square).
  if (in_rect(l.dpad_cx, l.dpad_cy, l.dpad_half, l.dpad_half, x, y)) {
    return Control::kDPad;
  }
  return Control::kNone;
}

void VirtualGamepadOverlayDialog::UpdateDPad(const Layout& l, float x, float y) {
  float dx = (x - l.dpad_cx) / l.dpad_half;
  float dy = (y - l.dpad_cy) / l.dpad_half;
  bool up = false, down = false, left = false, right = false;
  if (std::abs(dx) <= 1.1f && std::abs(dy) <= 1.1f) {
    up = dy < -0.25f;
    down = dy > 0.25f;
    left = dx < -0.25f;
    right = dx > 0.25f;
  }
  uint16_t bits = 0;
  if (up) bits |= input::X_INPUT_GAMEPAD_DPAD_UP;
  if (down) bits |= input::X_INPUT_GAMEPAD_DPAD_DOWN;
  if (left) bits |= input::X_INPUT_GAMEPAD_DPAD_LEFT;
  if (right) bits |= input::X_INPUT_GAMEPAD_DPAD_RIGHT;
  if (bits != dpad_bits_) {
    dpad_bits_ = bits;
    input::GetVirtualGamepadState().SetDPad(up, down, left, right);
  }
}

void VirtualGamepadOverlayDialog::UpdateStick(Control stick, const Layout& l, float x, float y) {
  float cx = stick == Control::kLeftStick ? l.lstick_cx : l.rstick_cx;
  float cy = stick == Control::kLeftStick ? l.lstick_cy : l.rstick_cy;
  float radius = stick == Control::kLeftStick ? l.lstick_r : l.rstick_r;
  float nx = (x - cx) / radius;
  float ny = (y - cy) / radius;
  // Clamp to the stick's travel (unit circle).
  float len = std::sqrt(nx * nx + ny * ny);
  if (len > 1.0f) {
    nx /= len;
    ny /= len;
  }
  // Dead zone near the center.
  if (len < 0.12f) {
    nx = 0;
    ny = 0;
  }
  if (stick == Control::kLeftStick) {
    lstick_nx_ = nx;
    lstick_ny_ = ny;
    // XInput: up is +Y, screen down is +y, so negate.
    input::GetVirtualGamepadState().SetLeftStick(static_cast<int16_t>(nx * 32767),
                                                 static_cast<int16_t>(-ny * 32767));
  } else {
    rstick_nx_ = nx;
    rstick_ny_ = ny;
    input::GetVirtualGamepadState().SetRightStick(static_cast<int16_t>(nx * 32767),
                                                  static_cast<int16_t>(-ny * 32767));
  }
}

void VirtualGamepadOverlayDialog::PressControl(Control control, const Layout& l, float x, float y) {
  pressed_controls_ |= (1u << static_cast<uint32_t>(control));
  switch (control) {
    case Control::kDPad:
      UpdateDPad(l, x, y);
      break;
    case Control::kLeftStick:
    case Control::kRightStick:
      UpdateStick(control, l, x, y);
      break;
    case Control::kMenu:
      if (on_menu_) {
        on_menu_();
      }
      break;
    case Control::kA:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_A);
      break;
    case Control::kB:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_B);
      break;
    case Control::kX:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_X);
      break;
    case Control::kY:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_Y);
      break;
    case Control::kLeftShoulder:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_LEFT_SHOULDER);
      break;
    case Control::kRightShoulder:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_RIGHT_SHOULDER);
      break;
    case Control::kLeftTrigger:
      input::GetVirtualGamepadState().SetLeftTrigger(255);
      break;
    case Control::kRightTrigger:
      input::GetVirtualGamepadState().SetRightTrigger(255);
      break;
    case Control::kStart:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_START);
      break;
    case Control::kBack:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_BACK);
      break;
    case Control::kGuide:
      input::GetVirtualGamepadState().Press(input::X_INPUT_GAMEPAD_GUIDE);
      break;
    case Control::kNone:
      break;
  }
}

void VirtualGamepadOverlayDialog::ReleaseControl(Control control) {
  pressed_controls_ &= ~(1u << static_cast<uint32_t>(control));
  switch (control) {
    case Control::kDPad:
      dpad_bits_ = 0;
      input::GetVirtualGamepadState().SetDPad(false, false, false, false);
      break;
    case Control::kLeftStick:
      lstick_nx_ = 0;
      lstick_ny_ = 0;
      input::GetVirtualGamepadState().SetLeftStick(0, 0);
      break;
    case Control::kRightStick:
      rstick_nx_ = 0;
      rstick_ny_ = 0;
      input::GetVirtualGamepadState().SetRightStick(0, 0);
      break;
    case Control::kA:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_A);
      break;
    case Control::kB:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_B);
      break;
    case Control::kX:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_X);
      break;
    case Control::kY:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_Y);
      break;
    case Control::kLeftShoulder:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_LEFT_SHOULDER);
      break;
    case Control::kRightShoulder:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_RIGHT_SHOULDER);
      break;
    case Control::kLeftTrigger:
      input::GetVirtualGamepadState().SetLeftTrigger(0);
      break;
    case Control::kRightTrigger:
      input::GetVirtualGamepadState().SetRightTrigger(0);
      break;
    case Control::kStart:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_START);
      break;
    case Control::kBack:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_BACK);
      break;
    case Control::kGuide:
      input::GetVirtualGamepadState().Release(input::X_INPUT_GAMEPAD_GUIDE);
      break;
    case Control::kMenu:
    case Control::kNone:
      break;
  }
}

void VirtualGamepadOverlayDialog::OnTouchEvent(TouchEvent& e) {
  if (last_width_ <= 0 || last_height_ <= 0) {
    return;
  }
  Layout layout = ComputeLayout(last_width_, last_height_);
  switch (e.action()) {
    case TouchEvent::Action::kDown: {
      Control control = HitTest(layout, e.x(), e.y());
      if (control != Control::kNone) {
        active_touches_[e.pointer_id()] = control;
        PressControl(control, layout, e.x(), e.y());
        e.set_handled(true);
      }
      break;
    }
    case TouchEvent::Action::kMove: {
      auto it = active_touches_.find(e.pointer_id());
      if (it != active_touches_.end()) {
        if (it->second == Control::kDPad) {
          UpdateDPad(layout, e.x(), e.y());
        } else if (it->second == Control::kLeftStick || it->second == Control::kRightStick) {
          UpdateStick(it->second, layout, e.x(), e.y());
        }
        e.set_handled(true);
      }
      break;
    }
    case TouchEvent::Action::kUp:
    case TouchEvent::Action::kCancel: {
      auto it = active_touches_.find(e.pointer_id());
      if (it != active_touches_.end()) {
        ReleaseControl(it->second);
        active_touches_.erase(it);
        e.set_handled(true);
      }
      break;
    }
  }
}

void VirtualGamepadOverlayDialog::OnDraw(ImGuiIO& io) {
  float width = io.DisplaySize.x;
  float height = io.DisplaySize.y;
  if (width <= 0 || height <= 0) {
    return;
  }
  last_width_ = width;
  last_height_ = height;
  Layout l = ComputeLayout(width, height);
  ImDrawList* draw = ImGui::GetBackgroundDrawList();
  if (!draw) {
    return;
  }

  auto is_pressed = [this](Control c) {
    return (pressed_controls_ & (1u << static_cast<uint32_t>(c))) != 0;
  };
  // Neutral white: faint when idle, solid when pressed.
  auto fill = [](bool pressed) {
    return pressed ? IM_COL32(255, 255, 255, 200) : IM_COL32(255, 255, 255, 70);
  };
  const ImU32 kBorder = IM_COL32(255, 255, 255, 140);
  const ImU32 kLabel = IM_COL32(255, 255, 255, 220);
  const ImU32 kLabelDark = IM_COL32(0, 0, 0, 220);

  // D-pad zone.
  draw->AddRectFilled(ImVec2(l.dpad_cx - l.dpad_half, l.dpad_cy - l.dpad_half),
                      ImVec2(l.dpad_cx + l.dpad_half, l.dpad_cy + l.dpad_half),
                      IM_COL32(255, 255, 255, 28), l.r * 0.4f);
  draw->AddRect(ImVec2(l.dpad_cx - l.dpad_half, l.dpad_cy - l.dpad_half),
                ImVec2(l.dpad_cx + l.dpad_half, l.dpad_cy + l.dpad_half), kBorder, l.r * 0.4f);
  const float tri = l.r * 0.55f;
  auto triangle = [&](float cx, float cy, float dx, float dy, bool pressed) {
    ImVec2 tip(cx + dx * tri, cy + dy * tri);
    ImVec2 left(cx - dy * tri * 0.7f - dx * tri * 0.4f, cy - dx * tri * 0.7f - dy * tri * 0.4f);
    ImVec2 right(cx + dy * tri * 0.7f - dx * tri * 0.4f, cy + dx * tri * 0.7f - dy * tri * 0.4f);
    draw->AddTriangleFilled(tip, left, right, fill(pressed));
  };
  triangle(l.dpad_cx, l.dpad_cy - l.r, 0, -1, (dpad_bits_ & input::X_INPUT_GAMEPAD_DPAD_UP) != 0);
  triangle(l.dpad_cx, l.dpad_cy + l.r, 0, 1, (dpad_bits_ & input::X_INPUT_GAMEPAD_DPAD_DOWN) != 0);
  triangle(l.dpad_cx - l.r, l.dpad_cy, -1, 0, (dpad_bits_ & input::X_INPUT_GAMEPAD_DPAD_LEFT) != 0);
  triangle(l.dpad_cx + l.r, l.dpad_cy, 1, 0, (dpad_bits_ & input::X_INPUT_GAMEPAD_DPAD_RIGHT) != 0);

  // Analog sticks: outer ring + nub (follows the finger).
  auto stick = [&](float cx, float cy, float radius, float nx, float ny, Control c) {
    bool pressed = is_pressed(c);
    draw->AddCircleFilled(ImVec2(cx, cy), radius, IM_COL32(255, 255, 255, 28));
    draw->AddCircle(ImVec2(cx, cy), radius, kBorder);
    float nub_r = radius * 0.5f;
    float nub_x = cx + nx * (radius - nub_r);
    float nub_y = cy + ny * (radius - nub_r);
    draw->AddCircleFilled(ImVec2(nub_x, nub_y), nub_r, fill(pressed));
    draw->AddCircle(ImVec2(nub_x, nub_y), nub_r, kBorder);
  };
  stick(l.lstick_cx, l.lstick_cy, l.lstick_r, lstick_nx_, lstick_ny_, Control::kLeftStick);
  stick(l.rstick_cx, l.rstick_cy, l.rstick_r, rstick_nx_, rstick_ny_, Control::kRightStick);

  // Face buttons (neutral white, letters only).
  auto face_button = [&](float cx, float cy, const char* label, Control c) {
    bool pressed = is_pressed(c);
    draw->AddCircleFilled(ImVec2(cx, cy), l.r, fill(pressed));
    draw->AddCircle(ImVec2(cx, cy), l.r, kBorder);
    ImVec2 ts = ImGui::CalcTextSize(label);
    draw->AddText(ImVec2(cx - ts.x * 0.5f, cy - ts.y * 0.5f),
                  pressed ? kLabelDark : kLabel, label);
  };
  face_button(l.face_cx, l.face_cy - 2 * l.r, "Y", Control::kY);
  face_button(l.face_cx, l.face_cy + 2 * l.r, "A", Control::kA);
  face_button(l.face_cx - 2 * l.r, l.face_cy, "X", Control::kX);
  face_button(l.face_cx + 2 * l.r, l.face_cy, "B", Control::kB);

  // Shoulders / triggers / view / menu.
  auto pill_button = [&](float cx, float cy, float hw, float hh, const char* label, Control c) {
    bool pressed = is_pressed(c);
    draw->AddRectFilled(ImVec2(cx - hw, cy - hh), ImVec2(cx + hw, cy + hh), fill(pressed),
                        hh * 0.5f);
    draw->AddRect(ImVec2(cx - hw, cy - hh), ImVec2(cx + hw, cy + hh), kBorder, hh * 0.5f);
    ImVec2 ts = ImGui::CalcTextSize(label);
    draw->AddText(ImVec2(cx - ts.x * 0.5f, cy - ts.y * 0.5f),
                  pressed ? kLabelDark : kLabel, label);
  };
  pill_button(l.lt_cx, l.lt_cy, 1.4f * l.r, 0.7f * l.r, "LT", Control::kLeftTrigger);
  pill_button(l.rt_cx, l.rt_cy, 1.4f * l.r, 0.7f * l.r, "RT", Control::kRightTrigger);
  pill_button(l.lb_cx, l.lb_cy, 1.4f * l.r, 0.7f * l.r, "LB", Control::kLeftShoulder);
  pill_button(l.rb_cx, l.rb_cy, 1.4f * l.r, 0.7f * l.r, "RB", Control::kRightShoulder);
  pill_button(l.back_cx, l.back_cy, 1.3f * l.r, 0.7f * l.r, "VIEW", Control::kBack);
  pill_button(l.start_cx, l.start_cy, 1.3f * l.r, 0.7f * l.r, "MENU", Control::kStart);

  // Guide button (Xbox).
  {
    bool pressed = is_pressed(Control::kGuide);
    draw->AddCircleFilled(ImVec2(l.guide_cx, l.guide_cy), l.r * 0.95f, fill(pressed));
    draw->AddCircle(ImVec2(l.guide_cx, l.guide_cy), l.r * 0.95f, kBorder);
    ImVec2 ts = ImGui::CalcTextSize("X");
    draw->AddText(ImVec2(l.guide_cx - ts.x * 0.5f, l.guide_cy - ts.y * 0.5f),
                  pressed ? kLabelDark : kLabel, "X");
  }

  // FPS menu button (three lines).
  {
    bool pressed = is_pressed(Control::kMenu);
    draw->AddCircleFilled(ImVec2(l.menu_cx, l.menu_cy), l.r * 0.8f, fill(pressed));
    draw->AddCircle(ImVec2(l.menu_cx, l.menu_cy), l.r * 0.8f, kBorder);
    for (int i = -1; i <= 1; ++i) {
      float y = l.menu_cy + i * l.r * 0.3f;
      draw->AddLine(ImVec2(l.menu_cx - l.r * 0.35f, y), ImVec2(l.menu_cx + l.r * 0.35f, y),
                    IM_COL32(0, 0, 0, 220), 2.0f);
    }
  }
}

}  // namespace rex::ui
