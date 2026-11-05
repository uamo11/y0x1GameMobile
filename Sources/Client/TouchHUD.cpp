/*
 Copyright (c) 2026 ySpades developers.

 This file is part of ySpades Mobile.
 */

#include "TouchHUD.h"
#include <cmath>
#include <algorithm>

#include <Client/Client.h>
#include <Client/Fonts.h>
#include <Client/IRenderer.h>
#include <Core/Settings.h>

#if defined(__ANDROID__) || defined(ANDROID) || defined(ZEROSPADES_MOBILE)
DEFINE_SPADES_SETTING(cg_touchHUD, "1");
#else
DEFINE_SPADES_SETTING(cg_touchHUD, "0");
#endif

DEFINE_SPADES_SETTING(cg_touchSensitivity, "1.2");
DEFINE_SPADES_SETTING(cg_touchOpacity, "0.55");

namespace spades {
	namespace client {

		TouchHUD::TouchHUD() {
			UpdateLayout(1280.0F, 720.0F);
		}

		bool TouchHUD::IsEnabled() {
			return cg_touchHUD;
		}

		void TouchHUD::UpdateLayout(float screenWidth, float screenHeight) {
			sw = screenWidth;
			sh = screenHeight;

			// Left joystick centered at bottom-left
			joystickCenter = MakeVector2(sw * 0.13F, sh * 0.75F);
			joystickBaseRadius = std::min(sw, sh) * 0.14F;
			joystickKnobRadius = joystickBaseRadius * 0.45F;
			if (!joystickActive) {
				joystickCurrent = joystickCenter;
			}

			buttons.clear();
			toolSlots.clear();

			float rAction = std::min(sw, sh) * 0.075F;
			float rSmall = std::min(sw, sh) * 0.055F;

			// Right Hand Action Buttons
			// 1. Fire / Mine (LButton)
			TouchButton btnFire;
			btnFire.key = "LButton";
			btnFire.label = "FIRE";
			btnFire.pos = MakeVector2(sw - rAction * 1.5F, sh - rAction * 2.2F);
			btnFire.radius = rAction;
			buttons.push_back(btnFire);

			// 2. Build / Dig 3x (RButton)
			TouchButton btnBuild;
			btnBuild.key = "RButton";
			btnBuild.label = "BUILD";
			btnBuild.pos = MakeVector2(sw - rAction * 3.4F, sh - rAction * 1.7F);
			btnBuild.radius = rAction * 0.9F;
			buttons.push_back(btnBuild);

			// 3. Jump (Space)
			TouchButton btnJump;
			btnJump.key = "Space";
			btnJump.label = "JUMP";
			btnJump.pos = MakeVector2(sw - rAction * 1.5F, sh - rAction * 4.2F);
			btnJump.radius = rAction * 0.85F;
			buttons.push_back(btnJump);

			// 4. Crouch / Sneak (C)
			TouchButton btnCrouch;
			btnCrouch.key = "C";
			btnCrouch.label = "CROUCH";
			btnCrouch.pos = MakeVector2(sw - rAction * 3.3F, sh - rAction * 3.5F);
			btnCrouch.radius = rSmall;
			buttons.push_back(btnCrouch);

			// 5. Reload (R)
			TouchButton btnReload;
			btnReload.key = "R";
			btnReload.label = "RELOAD";
			btnReload.pos = MakeVector2(sw - rAction * 2.3F, sh - rAction * 5.2F);
			btnReload.radius = rSmall;
			buttons.push_back(btnReload);

			// Quick Weapon & Tool Slots (Bottom center)
			const char* toolLabels[4] = {"1:SPADE", "2:BLOCK", "3:GUN", "4:GRENADE"};
			const char* toolKeys[4] = {"1", "2", "3", "4"};
			float slotW = std::min(sw, sh) * 0.12F;
			float startX = (sw * 0.5F) - (slotW * 2.0F) + (slotW * 0.5F);
			float slotY = sh - rSmall * 1.2F;

			for (int i = 0; i < 4; i++) {
				TouchButton slot;
				slot.key = toolKeys[i];
				slot.label = toolLabels[i];
				slot.pos = MakeVector2(startX + static_cast<float>(i) * (slotW + 6.0F), slotY);
				slot.radius = rSmall * 0.9F;
				toolSlots.push_back(slot);
			}

			// Top Buttons: Menu (ESC) & HKS Menu (Insert)
			escButton.key = "Escape";
			escButton.label = "MENU";
			escButton.pos = MakeVector2(48.0F, 36.0F);
			escButton.radius = 28.0F;

			hksButton.key = "Insert";
			hksButton.label = "HKS";
			hksButton.pos = MakeVector2(sw * 0.5F, 36.0F);
			hksButton.radius = 28.0F;
		}

		void TouchHUD::Draw(IRenderer& r, Client& client) {
			if (!cg_touchHUD || !client.fontManager)
				return;

			float op = Clamp(static_cast<float>(cg_touchOpacity), 0.1F, 1.0F);
			IFont& font = client.fontManager->GetGuiFont();
			IFont& smallFont = client.fontManager->GetSmallFont();

			Vector4 baseCol = MakeVector4(0.12F, 0.15F, 0.22F, op * 0.6F);
			Vector4 knobCol = MakeVector4(0.35F, 0.55F, 0.85F, op * 0.9F);
			Vector4 ringCol = MakeVector4(0.80F, 0.85F, 0.95F, op * 0.8F);

			// 1. Draw Virtual Joystick
			r.SetColorAlphaPremultiplied(baseCol);
			r.DrawFilledCircle(joystickCenter, joystickBaseRadius);
			r.SetColorAlphaPremultiplied(ringCol);
			r.DrawOutlinedCircle(joystickCenter, joystickBaseRadius, 2.0F);

			// Direction indicators
			r.DrawOutlinedCircle(joystickCenter, joystickBaseRadius * 0.5F, 1.0F);

			r.SetColorAlphaPremultiplied(joystickActive ? knobCol : MakeVector4(0.3F, 0.4F, 0.5F, op * 0.7F));
			r.DrawFilledCircle(joystickCurrent, joystickKnobRadius);
			r.SetColorAlphaPremultiplied(MakeVector4(1.0F, 1.0F, 1.0F, op));
			r.DrawOutlinedCircle(joystickCurrent, joystickKnobRadius, 2.0F);

			// 2. Draw Action Buttons
			for (const auto& btn : buttons) {
				Vector4 fillCol = btn.active ? MakeVector4(0.95F, 0.35F, 0.25F, op * 0.9F)
				                             : MakeVector4(0.10F, 0.12F, 0.18F, op * 0.65F);
				if (btn.key == "RButton") {
					fillCol = btn.active ? MakeVector4(0.20F, 0.75F, 0.45F, op * 0.9F)
					                     : MakeVector4(0.10F, 0.18F, 0.14F, op * 0.65F);
				} else if (btn.key == "Space") {
					fillCol = btn.active ? MakeVector4(0.30F, 0.60F, 1.0F, op * 0.9F)
					                     : MakeVector4(0.12F, 0.16F, 0.24F, op * 0.65F);
				}

				r.SetColorAlphaPremultiplied(fillCol);
				r.DrawFilledCircle(btn.pos, btn.radius);
				r.SetColorAlphaPremultiplied(ringCol);
				r.DrawOutlinedCircle(btn.pos, btn.radius, 2.0F);

				Vector2 sz = smallFont.Measure(btn.label);
				smallFont.DrawShadow(btn.label, btn.pos - sz * 0.5F, 1.0F,
				                     MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.7F));
			}

			// 3. Draw Quick Weapon & Tool Slots
			for (const auto& slot : toolSlots) {
				Vector4 fillCol = slot.active ? MakeVector4(0.85F, 0.75F, 0.20F, op * 0.9F)
				                              : MakeVector4(0.12F, 0.14F, 0.20F, op * 0.6F);
				r.SetColorAlphaPremultiplied(fillCol);
				r.DrawFilledCircle(slot.pos, slot.radius);
				r.SetColorAlphaPremultiplied(ringCol);
				r.DrawOutlinedCircle(slot.pos, slot.radius, 1.5F);

				Vector2 sz = smallFont.Measure(slot.label);
				smallFont.DrawShadow(slot.label, slot.pos - sz * 0.5F, 0.85F,
				                     MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.7F));
			}

			// 4. Menu & HKS Buttons
			{
				Vector4 menuFill = escButton.active ? MakeVector4(0.7F, 0.2F, 0.2F, op)
				                                    : MakeVector4(0.12F, 0.12F, 0.16F, op * 0.7F);
				r.SetColorAlphaPremultiplied(menuFill);
				r.DrawFilledCircle(escButton.pos, escButton.radius);
				r.SetColorAlphaPremultiplied(ringCol);
				r.DrawOutlinedCircle(escButton.pos, escButton.radius, 1.5F);
				Vector2 sz = smallFont.Measure(escButton.label);
				smallFont.DrawShadow(escButton.label, escButton.pos - sz * 0.5F, 0.9F,
				                     MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.7F));

				Vector4 hksFill = hksButton.active ? MakeVector4(0.65F, 0.25F, 0.9F, op)
				                                   : MakeVector4(0.20F, 0.12F, 0.28F, op * 0.7F);
				r.SetColorAlphaPremultiplied(hksFill);
				r.DrawFilledCircle(hksButton.pos, hksButton.radius);
				r.SetColorAlphaPremultiplied(MakeVector4(0.85F, 0.45F, 1.0F, op * 0.85F));
				r.DrawOutlinedCircle(hksButton.pos, hksButton.radius, 1.5F);
				Vector2 hksSz = smallFont.Measure(hksButton.label);
				smallFont.DrawShadow(hksButton.label, hksButton.pos - hksSz * 0.5F, 0.9F,
				                     MakeVector4(0.95F, 0.7F, 1.0F, 1.0F), MakeVector4(0, 0, 0, 0.7F));
			}
		}

		void TouchHUD::UpdateJoystickKeys(Client& client) {
			Vector2 diff = joystickCurrent - joystickCenter;
			float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
			float deadzone = joystickBaseRadius * 0.20F;
			float sprintDist = joystickBaseRadius * 0.85F;

			bool wantW = false;
			bool wantS = false;
			bool wantA = false;
			bool wantD = false;
			bool wantShift = false;

			if (len > deadzone) {
				float nx = diff.x / len;
				float ny = diff.y / len;

				if (ny < -0.38F) wantW = true;
				if (ny > 0.38F) wantS = true;
				if (nx < -0.38F) wantA = true;
				if (nx > 0.38F) wantD = true;
				if (len >= sprintDist) wantShift = true;
			}

			if (wantW != keyW) { keyW = wantW; client.KeyEvent("Up", keyW); }
			if (wantS != keyS) { keyS = wantS; client.KeyEvent("Down", keyS); }
			if (wantA != keyA) { keyA = wantA; client.KeyEvent("Left", keyA); }
			if (wantD != keyD) { keyD = wantD; client.KeyEvent("Right", keyD); }
			if (wantShift != keyShift) { keyShift = wantShift; client.KeyEvent("LeftShift", keyShift); }
		}

		void TouchHUD::ReleaseAllKeys(Client& client) {
			if (keyW) { keyW = false; client.KeyEvent("Up", false); }
			if (keyS) { keyS = false; client.KeyEvent("Down", false); }
			if (keyA) { keyA = false; client.KeyEvent("Left", false); }
			if (keyD) { keyD = false; client.KeyEvent("Right", false); }
			if (keyShift) { keyShift = false; client.KeyEvent("LeftShift", false); }
		}

		bool TouchHUD::HandleFingerDown(float x, float y, int64_t fingerId, Client& client) {
			if (!cg_touchHUD)
				return false;

			Vector2 touchPos{x, y};

			// Check Menu / HKS buttons
			auto checkCircle = [&](TouchButton& btn) -> bool {
				Vector2 d = touchPos - btn.pos;
				if (d.x * d.x + d.y * d.y <= btn.radius * btn.radius * 1.3F) {
					btn.active = true;
					btn.fingerId = fingerId;
					client.KeyEvent(btn.key, true);
					return true;
				}
				return false;
			};

			if (checkCircle(escButton)) return true;
			if (checkCircle(hksButton)) return true;

			// Check Action Buttons
			for (auto& btn : buttons) {
				if (checkCircle(btn)) return true;
			}

			// Check Tool Slots
			for (auto& slot : toolSlots) {
				if (checkCircle(slot)) return true;
			}

			// Left screen side: Joystick
			if (x < sw * 0.42F) {
				if (!joystickActive) {
					joystickActive = true;
					joystickFingerId = fingerId;
					joystickCenter = touchPos;
					joystickCurrent = touchPos;
					UpdateJoystickKeys(client);
					return true;
				}
			}

			// Right screen side: Look Touchpad
			if (x >= sw * 0.42F) {
				if (!lookActive) {
					lookActive = true;
					lookFingerId = fingerId;
					lookLastPos = touchPos;
					return true;
				}
			}

			return false;
		}

		bool TouchHUD::HandleFingerMotion(float x, float y, float dx, float dy, int64_t fingerId, Client& client) {
			if (!cg_touchHUD)
				return false;

			Vector2 touchPos{x, y};

			// Update joystick motion
			if (joystickActive && joystickFingerId == fingerId) {
				Vector2 diff = touchPos - joystickCenter;
				float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
				if (len > joystickBaseRadius) {
					diff = diff * (joystickBaseRadius / len);
				}
				joystickCurrent = joystickCenter + diff;
				UpdateJoystickKeys(client);
				return true;
			}

			// Update look touchpad motion
			if (lookActive && lookFingerId == fingerId) {
				float sens = static_cast<float>(cg_touchSensitivity);
				float deltaX = (touchPos.x - lookLastPos.x) * sens;
				float deltaY = (touchPos.y - lookLastPos.y) * sens;
				lookLastPos = touchPos;

				if (std::abs(deltaX) > 0.001F || std::abs(deltaY) > 0.001F) {
					client.MouseEvent(deltaX, deltaY);
				}
				return true;
			}

			return false;
		}

		bool TouchHUD::HandleFingerUp(float x, float y, int64_t fingerId, Client& client) {
			if (!cg_touchHUD)
				return false;

			// Release joystick
			if (joystickActive && joystickFingerId == fingerId) {
				joystickActive = false;
				joystickFingerId = -1;
				joystickCurrent = joystickCenter;
				ReleaseAllKeys(client);
				return true;
			}

			// Release look touchpad
			if (lookActive && lookFingerId == fingerId) {
				lookActive = false;
				lookFingerId = -1;
				return true;
			}

			// Release buttons
			auto releaseBtn = [&](TouchButton& btn) -> bool {
				if (btn.active && btn.fingerId == fingerId) {
					btn.active = false;
					btn.fingerId = -1;
					client.KeyEvent(btn.key, false);
					return true;
				}
				return false;
			};

			if (releaseBtn(escButton)) return true;
			if (releaseBtn(hksButton)) return true;

			for (auto& btn : buttons) {
				if (releaseBtn(btn)) return true;
			}

			for (auto& slot : toolSlots) {
				if (releaseBtn(slot)) return true;
			}

			return false;
		}

		void TouchHUD::Reset(Client& client) {
			joystickActive = false;
			joystickFingerId = -1;
			joystickCurrent = joystickCenter;
			lookActive = false;
			lookFingerId = -1;
			ReleaseAllKeys(client);

			for (auto& btn : buttons) {
				if (btn.active) {
					btn.active = false;
					btn.fingerId = -1;
					client.KeyEvent(btn.key, false);
				}
			}
			for (auto& slot : toolSlots) {
				if (slot.active) {
					slot.active = false;
					slot.fingerId = -1;
					client.KeyEvent(slot.key, false);
				}
			}
			escButton.active = false;
			escButton.fingerId = -1;
			hksButton.active = false;
			hksButton.fingerId = -1;
		}

	} // namespace client
} // namespace spades
