/*
 Copyright (c) 2026 ySpades developers.

 This file is part of ySpades Mobile.
 */

#include "TouchHUD.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

#include <Client/Client.h>
#include <Client/Fonts.h>
#include <Client/IRenderer.h>
#include <Client/Player.h>
#include <Client/World.h>
#include <Core/Settings.h>

#if defined(__ANDROID__) || defined(ANDROID) || defined(ZEROSPADES_MOBILE)
DEFINE_SPADES_SETTING(cg_touchHUD, "1");
#else
DEFINE_SPADES_SETTING(cg_touchHUD, "0");
#endif

DEFINE_SPADES_SETTING(cg_touchSensitivity, "1.2");
DEFINE_SPADES_SETTING(cg_touchOpacity, "0.65");

namespace spades {
	namespace client {

		static void DrawGlassCircle(IRenderer& r, const Vector2& center, float radius,
		                            bool active, const Vector4& glowColor, float opacity) {
			// 1. Dark translucent glass background
			Vector4 bg = active
			    ? MakeVector4(glowColor.x * 0.40F, glowColor.y * 0.40F, glowColor.z * 0.40F, opacity * 0.90F)
			    : MakeVector4(0.08F, 0.10F, 0.16F, opacity * 0.65F);
			r.SetColorAlphaPremultiplied(bg);
			r.DrawFilledCircle(center, radius);

			// 2. Outer glowing ring
			Vector4 ring = active
			    ? MakeVector4(glowColor.x, glowColor.y, glowColor.z, opacity * 0.98F)
			    : MakeVector4(0.70F, 0.80F, 0.95F, opacity * 0.65F);
			r.SetColorAlphaPremultiplied(ring);
			r.DrawOutlinedCircle(center, radius, active ? 2.5F : 1.8F);

			// 3. Inner highlight rim
			if (!active) {
				Vector4 innerRim = MakeVector4(1.0F, 1.0F, 1.0F, opacity * 0.18F);
				r.SetColorAlphaPremultiplied(innerRim);
				r.DrawOutlinedCircle(center, radius * 0.88F, 1.0F);
			}
		}

		static void DrawGlassCard(IRenderer& r, float x, float y, float w, float h,
		                          bool active, bool isCurrentWeapon, const Vector4& glowColor, float opacity) {
			float x0 = x - w * 0.5F;
			float y0 = y - h * 0.5F;
			float x1 = x + w * 0.5F;
			float y1 = y + h * 0.5F;

			// Dark glass body
			Vector4 bg = (active || isCurrentWeapon)
			    ? MakeVector4(glowColor.x * 0.30F, glowColor.y * 0.30F, glowColor.z * 0.30F, opacity * 0.90F)
			    : MakeVector4(0.08F, 0.10F, 0.16F, opacity * 0.60F);
			r.SetColorAlphaPremultiplied(bg);
			r.DrawFilledRect(x0, y0, x1, y1);

			// Glowing border
			Vector4 border = (active || isCurrentWeapon)
			    ? MakeVector4(glowColor.x, glowColor.y, glowColor.z, opacity * 0.98F)
			    : MakeVector4(0.55F, 0.65F, 0.80F, opacity * 0.45F);
			r.SetColorAlphaPremultiplied(border);
			r.DrawOutlinedRect(x0, y0, x1, y1, (active || isCurrentWeapon) ? 2 : 1);
		}

		TouchHUD::TouchHUD() {
			UpdateLayout(1280.0F, 720.0F);
		}

		bool TouchHUD::IsEnabled() {
			return cg_touchHUD;
		}

		void TouchHUD::LoadAssets(IRenderer& r) {
			if (assetsLoaded)
				return;

			imgJoystick = r.RegisterImage("Gfx/Touch/joystick.png");
			imgKnob = r.RegisterImage("Gfx/Touch/knob.png");
			imgReload = r.RegisterImage("Gfx/Touch/reload.png");
			imgArrow = r.RegisterImage("Gfx/Touch/arrow.png");
			imgSpade = r.RegisterImage("Gfx/Hotbar/Spade.png");
			imgBlock = r.RegisterImage("Gfx/Hotbar/Block.png");
			imgRifle = r.RegisterImage("Gfx/Hotbar/Rifle.png");
			imgSMG = r.RegisterImage("Gfx/Hotbar/SMG.png");
			imgShotgun = r.RegisterImage("Gfx/Hotbar/Shotgun.png");
			imgGrenade = r.RegisterImage("Gfx/Hotbar/Grenade.png");

			assetsLoaded = true;
		}

		void TouchHUD::UpdateLayout(float screenWidth, float screenHeight) {
			sw = screenWidth;
			sh = screenHeight;

			// Left joystick centered at bottom-left
			joystickCenter = MakeVector2(sw * 0.13F, sh * 0.72F);
			joystickBaseRadius = std::min(sw, sh) * 0.16F;
			joystickKnobRadius = joystickBaseRadius * 0.44F;
			if (!joystickActive) {
				joystickCurrent = joystickCenter;
			}

			// Right Hand Action Buttons (Mobile Competitive FPS standard)
			float rFire = std::min(sw, sh) * 0.088F;    // Primary large button (~58px)
			float rAction = std::min(sw, sh) * 0.072F;  // Secondary medium button (~48px)
			float rSmall = std::min(sw, sh) * 0.060F;   // Utility small button (~40px)
			float rChat = std::min(sw, sh) * 0.052F;    // Quick chat button (~35px)

			// 1. Primary Trigger: FIRE (LeftMouseButton)
			btnFire.key = "LeftMouseButton";
			btnFire.label = "FIRE";
			btnFire.pos = MakeVector2(sw - 135.0F, sh - 150.0F);
			btnFire.radius = rFire;
			btnFire.isRect = false;

			// 2. Secondary Action: AIM / SCOPE / BUILD (RightMouseButton)
			btnAim.key = "RightMouseButton";
			btnAim.label = "AIM";
			btnAim.pos = MakeVector2(sw - 255.0F, sh - 155.0F);
			btnAim.radius = rAction;
			btnAim.isRect = false;

			// 3. Mobility: JUMP (Space)
			btnJump.key = "Space";
			btnJump.label = "JUMP";
			btnJump.pos = MakeVector2(sw - 135.0F, sh - 275.0F);
			btnJump.radius = rAction;
			btnJump.isRect = false;

			// 4. Stance: CROUCH (Control)
			btnCrouch.key = "Control";
			btnCrouch.label = "CROUCH";
			btnCrouch.pos = MakeVector2(sw - 250.0F, sh - 275.0F);
			btnCrouch.radius = rSmall;
			btnCrouch.isRect = false;

			// 5. Utility: RELOAD (r)
			btnReload.key = "r";
			btnReload.label = "RELOAD";
			btnReload.pos = MakeVector2(sw - 135.0F, sh - 390.0F);
			btnReload.radius = rSmall;
			btnReload.isRect = false;

			// 6. Quick Chat: CHAT (t)
			btnChat.key = "t";
			btnChat.label = "CHAT";
			btnChat.pos = MakeVector2(sw - 245.0F, sh - 390.0F);
			btnChat.radius = rChat;
			btnChat.isRect = false;

			// Bottom Weapon / Tool Slots
			toolSlots.clear();
			const char* toolKeys[4] = {"1", "2", "3", "4"};
			const char* toolNames[4] = {"1:PALA", "2:BLOQUE", "3:ARMA", "4:GRANADA"};
			float cardW = 86.0F;
			float cardH = 54.0F;
			float spacing = 10.0F;
			float totalW = 4.0F * cardW + 3.0F * spacing;
			float startX = (sw - totalW) * 0.5F + cardW * 0.5F;
			float slotY = sh - cardH * 0.55F - 6.0F;

			for (int i = 0; i < 4; i++) {
				TouchButton slot;
				slot.key = toolKeys[i];
				slot.label = toolNames[i];
				slot.pos = MakeVector2(startX + static_cast<float>(i) * (cardW + spacing), slotY);
				slot.width = cardW;
				slot.height = cardH;
				slot.radius = cardW * 0.5F;
				slot.isRect = true;
				toolSlots.push_back(slot);
			}

			// Top Buttons: Menu (ESC), HKS Menu (Insert), Scoreboard (Tab)
			escButton.key = "Escape";
			escButton.label = "PAUSA";
			escButton.pos = MakeVector2(75.0F, 85.0F);
			escButton.width = 95.0F;
			escButton.height = 38.0F;
			escButton.radius = 28.0F;
			escButton.isRect = true;

			hksButton.key = "Insert";
			hksButton.label = "MODS (Insert)";
			hksButton.pos = MakeVector2(225.0F, 85.0F);
			hksButton.width = 160.0F;
			hksButton.height = 38.0F;
			hksButton.radius = 24.0F;
			hksButton.isRect = true;

			btnScore.key = "Tab";
			btnScore.label = "TAB";
			btnScore.pos = MakeVector2(sw - 60.0F, 85.0F);
			btnScore.width = 80.0F;
			btnScore.height = 38.0F;
			btnScore.radius = 24.0F;
			btnScore.isRect = true;
		}

		void TouchHUD::Draw(IRenderer& r, Client& client) {
			if (!cg_touchHUD || !client.fontManager)
				return;

			LoadAssets(r);

			float op = Clamp(static_cast<float>(cg_touchOpacity), 0.15F, 1.0F);
			IFont& font = client.fontManager->GetGuiFont();
			IFont& smallFont = client.fontManager->GetSmallFont();

			// Query active player state
			int activeTool = -1;
			WeaponType weaponType = RIFLE_WEAPON;
			IntVector3 heldBlockColor = MakeIntVector3(255, 255, 255);
			bool isHoldingWeapon = false;

			if (client.GetWorld()) {
				auto maybePlayer = client.GetWorld()->GetLocalPlayer();
				if (maybePlayer) {
					Player& p = maybePlayer.value();
					activeTool = static_cast<int>(p.GetTool());
					weaponType = p.GetWeaponType();
					heldBlockColor = p.GetBlockColor();
					isHoldingWeapon = p.IsToolWeapon();
				}
			}

			// Dynamic label for secondary button
			btnAim.label = isHoldingWeapon ? "AIM" : (activeTool == Player::ToolBlock ? "BUILD" : "DIG");

			// 1. Draw Virtual Joystick
			if (imgJoystick) {
				r.SetColorAlphaPremultiplied(MakeVector4(1.0F, 1.0F, 1.0F, op * 0.85F));
				r.DrawImage(*imgJoystick, AABB2(joystickCenter.x - joystickBaseRadius,
				                                joystickCenter.y - joystickBaseRadius,
				                                joystickBaseRadius * 2.0F,
				                                joystickBaseRadius * 2.0F));
			} else {
				r.SetColorAlphaPremultiplied(MakeVector4(0.08F, 0.10F, 0.16F, op * 0.60F));
				r.DrawFilledCircle(joystickCenter, joystickBaseRadius);
				r.SetColorAlphaPremultiplied(MakeVector4(0.70F, 0.85F, 1.0F, op * 0.75F));
				r.DrawOutlinedCircle(joystickCenter, joystickBaseRadius, 2.0F);
			}

			// Center rest dot
			if (imgKnob) {
				float restDotSz = joystickKnobRadius * 0.55F;
				r.SetColorAlphaPremultiplied(MakeVector4(0.6F, 0.75F, 0.9F, op * 0.45F));
				r.DrawImage(*imgKnob, AABB2(joystickCenter.x - restDotSz * 0.5F,
				                            joystickCenter.y - restDotSz * 0.5F,
				                            restDotSz, restDotSz));
			}

			// Dynamic thumbstick knob
			if (joystickActive) {
				r.SetColorAlphaPremultiplied(MakeVector4(0.35F, 0.75F, 1.0F, op * 0.50F));
				r.DrawLine(joystickCenter, joystickCurrent, 2.0F);
			}

			if (imgKnob) {
				float knobSz = joystickKnobRadius * 2.0F;
				Vector4 knobTint = joystickActive
				    ? MakeVector4(0.35F, 0.85F, 1.0F, op * 0.98F)
				    : MakeVector4(0.85F, 0.90F, 0.98F, op * 0.75F);
				r.SetColorAlphaPremultiplied(knobTint);
				r.DrawImage(*imgKnob, AABB2(joystickCurrent.x - knobSz * 0.5F,
				                            joystickCurrent.y - knobSz * 0.5F,
				                            knobSz, knobSz));
			} else {
				Vector4 knobCol = joystickActive ? MakeVector4(0.35F, 0.75F, 1.0F, op * 0.95F)
				                                 : MakeVector4(0.7F, 0.8F, 0.9F, op * 0.70F);
				r.SetColorAlphaPremultiplied(knobCol);
				r.DrawFilledCircle(joystickCurrent, joystickKnobRadius);
				r.SetColorAlphaPremultiplied(MakeVector4(1.0F, 1.0F, 1.0F, op));
				r.DrawOutlinedCircle(joystickCurrent, joystickKnobRadius, 2.0F);
			}

			// 2. Draw FIRE Button (Tactical Reticle)
			{
				Vector4 fireGlow = MakeVector4(1.0F, 0.30F, 0.15F, 1.0F);
				DrawGlassCircle(r, btnFire.pos, btnFire.radius, btnFire.active, fireGlow, op);

				Vector4 reticleCol = btnFire.active ? MakeVector4(1.0F, 1.0F, 1.0F, op)
				                                    : MakeVector4(1.0F, 0.45F, 0.35F, op * 0.85F);
				r.SetColorAlphaPremultiplied(reticleCol);
				float retR = btnFire.radius * 0.40F;
				r.DrawOutlinedCircle(btnFire.pos, retR, 1.8F);

				float pipIn = retR * 0.55F;
				float pipOut = retR * 1.45F;
				r.DrawLine(btnFire.pos - MakeVector2(pipOut, 0), btnFire.pos - MakeVector2(pipIn, 0), 2.0F);
				r.DrawLine(btnFire.pos + MakeVector2(pipIn, 0), btnFire.pos + MakeVector2(pipOut, 0), 2.0F);
				r.DrawLine(btnFire.pos - MakeVector2(0, pipOut), btnFire.pos - MakeVector2(0, pipIn), 2.0F);
				r.DrawLine(btnFire.pos + MakeVector2(0, pipIn), btnFire.pos + MakeVector2(0, pipOut), 2.0F);
				r.DrawFilledCircle(btnFire.pos, 2.0F);

				Vector2 sz = smallFont.Measure("FIRE");
				smallFont.DrawShadow("FIRE", btnFire.pos + MakeVector2(-sz.x * 0.5F, btnFire.radius * 0.45F), 0.75F,
				                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));
			}

			// 3. Draw AIM / BUILD Button (Scope / Cube)
			{
				Vector4 aimGlow = isHoldingWeapon ? MakeVector4(0.20F, 0.85F, 0.55F, 1.0F)
				                                  : MakeVector4(0.25F, 0.65F, 1.0F, 1.0F);
				DrawGlassCircle(r, btnAim.pos, btnAim.radius, btnAim.active, aimGlow, op);

				Vector4 reticleCol = btnAim.active ? MakeVector4(1.0F, 1.0F, 1.0F, op)
				                                   : MakeVector4(aimGlow.x, aimGlow.y, aimGlow.z, op * 0.85F);
				r.SetColorAlphaPremultiplied(reticleCol);

				if (isHoldingWeapon) {
					float sR = btnAim.radius * 0.42F;
					r.DrawOutlinedCircle(btnAim.pos, sR, 1.8F);
					r.DrawOutlinedCircle(btnAim.pos, sR * 0.5F, 1.0F);
					r.DrawLine(btnAim.pos - MakeVector2(sR * 1.2F, 0), btnAim.pos + MakeVector2(sR * 1.2F, 0), 1.2F);
					r.DrawLine(btnAim.pos - MakeVector2(0, sR * 1.2F), btnAim.pos + MakeVector2(0, sR * 1.2F), 1.2F);
				} else {
					float bS = btnAim.radius * 0.32F;
					r.DrawOutlinedRect(btnAim.pos.x - bS, btnAim.pos.y - bS, btnAim.pos.x + bS, btnAim.pos.y + bS, 2);
				}

				Vector2 sz = smallFont.Measure(btnAim.label);
				smallFont.DrawShadow(btnAim.label, btnAim.pos + MakeVector2(-sz.x * 0.5F, btnAim.radius * 0.45F), 0.72F,
				                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));
			}

			// 4. Draw JUMP Button (Space)
			{
				Vector4 jumpGlow = MakeVector4(0.30F, 0.70F, 1.0F, 1.0F);
				DrawGlassCircle(r, btnJump.pos, btnJump.radius, btnJump.active, jumpGlow, op);

				Vector4 iconCol = btnJump.active ? MakeVector4(1.0F, 1.0F, 1.0F, op)
				                                 : MakeVector4(0.4F, 0.8F, 1.0F, op * 0.9F);
				r.SetColorAlphaPremultiplied(iconCol);
				float aY = btnJump.pos.y - btnJump.radius * 0.20F;
				float aW = btnJump.radius * 0.35F;
				float aH = btnJump.radius * 0.25F;
				r.DrawFilledTriangle(MakeVector2(btnJump.pos.x, aY - aH),
				                     MakeVector2(btnJump.pos.x - aW, aY + aH * 0.5F),
				                     MakeVector2(btnJump.pos.x + aW, aY + aH * 0.5F));

				Vector2 sz = smallFont.Measure("JUMP");
				smallFont.DrawShadow("JUMP", btnJump.pos + MakeVector2(-sz.x * 0.5F, btnJump.radius * 0.35F), 0.75F,
				                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));
			}

			// 5. Draw CROUCH Button (C)
			{
				Vector4 crouchGlow = MakeVector4(0.95F, 0.75F, 0.25F, 1.0F);
				DrawGlassCircle(r, btnCrouch.pos, btnCrouch.radius, btnCrouch.active, crouchGlow, op);

				Vector4 iconCol = btnCrouch.active ? MakeVector4(1.0F, 1.0F, 1.0F, op)
				                                   : MakeVector4(0.95F, 0.80F, 0.35F, op * 0.9F);
				r.SetColorAlphaPremultiplied(iconCol);
				float aY = btnCrouch.pos.y - btnCrouch.radius * 0.15F;
				float aW = btnCrouch.radius * 0.32F;
				float aH = btnCrouch.radius * 0.22F;
				r.DrawFilledTriangle(MakeVector2(btnCrouch.pos.x, aY + aH),
				                     MakeVector2(btnCrouch.pos.x - aW, aY - aH * 0.5F),
				                     MakeVector2(btnCrouch.pos.x + aW, aY - aH * 0.5F));

				Vector2 sz = smallFont.Measure("CROUCH");
				smallFont.DrawShadow("CROUCH", btnCrouch.pos + MakeVector2(-sz.x * 0.5F, btnCrouch.radius * 0.32F), 0.65F,
				                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));
			}

			// 6. Draw RELOAD Button (R)
			{
				Vector4 reloadGlow = MakeVector4(0.85F, 0.45F, 1.0F, 1.0F);
				DrawGlassCircle(r, btnReload.pos, btnReload.radius, btnReload.active, reloadGlow, op);

				if (imgReload) {
					float rSz = btnReload.radius * 1.15F;
					Vector4 rCol = btnReload.active ? MakeVector4(1.0F, 1.0F, 1.0F, op)
					                                : MakeVector4(0.9F, 0.75F, 1.0F, op * 0.9F);
					r.SetColorAlphaPremultiplied(rCol);
					r.DrawImage(*imgReload, AABB2(btnReload.pos.x - rSz * 0.5F,
					                              btnReload.pos.y - rSz * 0.5F,
					                              rSz, rSz));
				} else {
					Vector2 sz = smallFont.Measure("R");
					smallFont.DrawShadow("R", btnReload.pos - sz * 0.5F, 1.0F,
					                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));
				}
			}

			// Draw CHAT Button (t)
			{
				Vector4 chatGlow = MakeVector4(0.25F, 0.75F, 1.0F, 1.0F);
				DrawGlassCircle(r, btnChat.pos, btnChat.radius, btnChat.active, chatGlow, op);
				Vector4 iconCol = btnChat.active ? MakeVector4(1.0F, 1.0F, 1.0F, op)
				                                 : MakeVector4(0.40F, 0.85F, 1.0F, op * 0.9F);
				r.SetColorAlphaPremultiplied(iconCol);

				float cW = btnChat.radius * 0.40F;
				float cH = btnChat.radius * 0.28F;
				r.DrawOutlinedRect(btnChat.pos.x - cW, btnChat.pos.y - cH - 2.0F,
				                   btnChat.pos.x + cW, btnChat.pos.y + cH - 2.0F, 1.5F);
				r.DrawLine(MakeVector2(btnChat.pos.x - cW * 0.5F, btnChat.pos.y + cH - 2.0F),
				           MakeVector2(btnChat.pos.x - cW * 0.2F, btnChat.pos.y + cH + 3.0F), 1.5F);
				r.DrawLine(MakeVector2(btnChat.pos.x - cW * 0.2F, btnChat.pos.y + cH + 3.0F),
				           MakeVector2(btnChat.pos.x, btnChat.pos.y + cH - 2.0F), 1.5F);

				Vector2 sz = smallFont.Measure("CHAT");
				smallFont.DrawShadow("CHAT", btnChat.pos + MakeVector2(-sz.x * 0.5F, btnChat.radius * 0.35F), 0.65F,
				                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));
			}

			// 7. Draw Bottom Weapon / Tool Selector Bar (Cards with real weapon icons)
			for (size_t i = 0; i < toolSlots.size(); i++) {
				const auto& slot = toolSlots[i];
				bool isEquipped = (static_cast<int>(i) == activeTool);
				Vector4 glowCol = isEquipped ? MakeVector4(0.35F, 0.85F, 1.0F, 1.0F)
				                             : MakeVector4(0.70F, 0.75F, 0.85F, 1.0F);

				DrawGlassCard(r, slot.pos.x, slot.pos.y, slot.width, slot.height,
				              slot.active, isEquipped, glowCol, op);

				// Slot number in top-left
				char numBuf[4];
				snprintf(numBuf, sizeof(numBuf), "%d", static_cast<int>(i + 1));
				smallFont.DrawShadow(numBuf, MakeVector2(slot.pos.x - slot.width * 0.42F, slot.pos.y - slot.height * 0.42F),
				                     0.68F, isEquipped ? MakeVector4(0.35F, 0.85F, 1.0F, op) : MakeVector4(0.7F, 0.7F, 0.7F, op * 0.7F),
				                     MakeVector4(0, 0, 0, op * 0.8F));

				// Draw tool texture
				Handle<IImage> toolImg;
				if (i == 0) toolImg = imgSpade;
				else if (i == 1) toolImg = imgBlock;
				else if (i == 2) {
					if (weaponType == SMG_WEAPON) toolImg = imgSMG;
					else if (weaponType == SHOTGUN_WEAPON) toolImg = imgShotgun;
					else toolImg = imgRifle;
				} else if (i == 3) toolImg = imgGrenade;

				if (toolImg) {
					float imgW = toolImg->GetWidth();
					float imgH = toolImg->GetHeight();
					float maxDim = slot.height * 0.65F;
					float scale = std::min(maxDim / imgH, (slot.width * 0.65F) / imgW);
					float dw = imgW * scale;
					float dh = imgH * scale;

					Vector4 iconCol = isEquipped ? MakeVector4(1.0F, 1.0F, 1.0F, op)
					                             : MakeVector4(0.75F, 0.80F, 0.90F, op * 0.75F);
					r.SetColorAlphaPremultiplied(iconCol);
					r.DrawImage(*toolImg, AABB2(slot.pos.x - dw * 0.5F + 4.0F,
					                            slot.pos.y - dh * 0.5F, dw, dh));
				}

				// For Block (slot 1): draw tiny held block color indicator swatch
				if (i == 1) {
					Vector4 colSwatch = ConvertColorRGBA(heldBlockColor);
					colSwatch.w = op;
					r.SetColorAlphaPremultiplied(colSwatch);
					r.DrawFilledRect(slot.pos.x + slot.width * 0.28F, slot.pos.y - 4.0F,
					                 slot.pos.x + slot.width * 0.42F, slot.pos.y + 4.0F);
					r.SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, op * 0.6F));
					r.DrawOutlinedRect(slot.pos.x + slot.width * 0.28F, slot.pos.y - 4.0F,
					                   slot.pos.x + slot.width * 0.42F, slot.pos.y + 4.0F, 1);
				}
			}

			// 8. Draw Menu & HKS Buttons
			{
				// PAUSA
				DrawGlassCard(r, escButton.pos.x, escButton.pos.y, escButton.width, escButton.height,
				              escButton.active, false, MakeVector4(0.95F, 0.35F, 0.35F, 1.0F), op);
				Vector2 mSz = smallFont.Measure(escButton.label);
				smallFont.DrawShadow(escButton.label, escButton.pos - mSz * 0.5F, 0.78F,
				                     MakeVector4(1, 1, 1, op), MakeVector4(0, 0, 0, op * 0.8F));

				// MODS (Insert)
				DrawGlassCard(r, hksButton.pos.x, hksButton.pos.y, hksButton.width, hksButton.height,
				              hksButton.active, false, MakeVector4(1.0F, 0.75F, 0.20F, 1.0F), op);
				Vector2 hSz = smallFont.Measure(hksButton.label);
				smallFont.DrawShadow(hksButton.label, hksButton.pos - hSz * 0.5F, 0.78F,
				                     MakeVector4(1.0F, 0.90F, 0.40F, op), MakeVector4(0, 0, 0, op * 0.8F));

				// TAB (Scoreboard)
				DrawGlassCard(r, btnScore.pos.x, btnScore.pos.y, btnScore.width, btnScore.height,
				              btnScore.active, false, MakeVector4(0.70F, 0.45F, 1.0F, 1.0F), op);
				Vector2 sSz = smallFont.Measure(btnScore.label);
				smallFont.DrawShadow(btnScore.label, btnScore.pos - sSz * 0.5F, 0.78F,
				                     MakeVector4(0.85F, 0.80F, 1.0F, op), MakeVector4(0, 0, 0, op * 0.8F));
			}
		}

		void TouchHUD::UpdateJoystickKeys(Client& client) {
			Vector2 diff = joystickCurrent - joystickCenter;
			float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
			float deadzone = joystickBaseRadius * 0.18F;
			float sprintDist = joystickBaseRadius * 0.82F;

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

			if (wantW != keyW) {
				keyW = wantW;
				client.KeyEvent("w", keyW);
				client.KeyEvent("Up", keyW);
			}
			if (wantS != keyS) {
				keyS = wantS;
				client.KeyEvent("s", keyS);
				client.KeyEvent("Down", keyS);
			}
			if (wantA != keyA) {
				keyA = wantA;
				client.KeyEvent("a", keyA);
				client.KeyEvent("Left", keyA);
			}
			if (wantD != keyD) {
				keyD = wantD;
				client.KeyEvent("d", keyD);
				client.KeyEvent("Right", keyD);
			}
			if (wantShift != keyShift) {
				keyShift = wantShift;
				client.KeyEvent("Shift", keyShift);
				client.KeyEvent("LeftShift", keyShift);
			}
		}

		void TouchHUD::ReleaseAllKeys(Client& client) {
			if (keyW) { keyW = false; client.KeyEvent("w", false); client.KeyEvent("Up", false); }
			if (keyS) { keyS = false; client.KeyEvent("s", false); client.KeyEvent("Down", false); }
			if (keyA) { keyA = false; client.KeyEvent("a", false); client.KeyEvent("Left", false); }
			if (keyD) { keyD = false; client.KeyEvent("d", false); client.KeyEvent("Right", false); }
			if (keyShift) { keyShift = false; client.KeyEvent("Shift", false); client.KeyEvent("LeftShift", false); }
		}

		bool TouchHUD::HandleFingerDown(float x, float y, int64_t fingerId, Client& client) {
			if (!cg_touchHUD)
				return false;

			Vector2 touchPos{x, y};

			// Check button helper
			auto checkBtn = [&](TouchButton& btn) -> bool {
				if (btn.isRect) {
					float hx = btn.width * 0.5F + 8.0F;
					float hy = btn.height * 0.5F + 8.0F;
					if (touchPos.x >= btn.pos.x - hx && touchPos.x <= btn.pos.x + hx &&
					    touchPos.y >= btn.pos.y - hy && touchPos.y <= btn.pos.y + hy) {
						btn.active = true;
						btn.fingerId = fingerId;
						client.KeyEvent(btn.key, true);
						if (btn.key == "LeftMouseButton") client.KeyEvent("LButton", true);
						if (btn.key == "RightMouseButton") client.KeyEvent("RButton", true);
						if (btn.key == "Control") client.KeyEvent("c", true);
						return true;
					}
				} else {
					Vector2 d = touchPos - btn.pos;
					if (d.x * d.x + d.y * d.y <= btn.radius * btn.radius * 1.35F) {
						btn.active = true;
						btn.fingerId = fingerId;
						client.KeyEvent(btn.key, true);
						if (btn.key == "LeftMouseButton") client.KeyEvent("LButton", true);
						if (btn.key == "RightMouseButton") client.KeyEvent("RButton", true);
						if (btn.key == "Control") client.KeyEvent("c", true);
						return true;
					}
				}
				return false;
			};

			if (checkBtn(escButton)) return true;
			if (checkBtn(hksButton)) return true;
			if (checkBtn(btnScore)) return true;
			if (checkBtn(btnFire)) return true;
			if (checkBtn(btnAim)) return true;
			if (checkBtn(btnJump)) return true;
			if (checkBtn(btnCrouch)) return true;
			if (checkBtn(btnReload)) return true;
			if (checkBtn(btnChat)) return true;

			// Check Tool Slots
			for (auto& slot : toolSlots) {
				if (checkBtn(slot)) return true;
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
					if (btn.key == "LeftMouseButton") client.KeyEvent("LButton", false);
					if (btn.key == "RightMouseButton") client.KeyEvent("RButton", false);
					if (btn.key == "Control") client.KeyEvent("c", false);
					return true;
				}
				return false;
			};

			if (releaseBtn(escButton)) return true;
			if (releaseBtn(hksButton)) return true;
			if (releaseBtn(btnScore)) return true;
			if (releaseBtn(btnFire)) return true;
			if (releaseBtn(btnAim)) return true;
			if (releaseBtn(btnJump)) return true;
			if (releaseBtn(btnCrouch)) return true;
			if (releaseBtn(btnReload)) return true;
			if (releaseBtn(btnChat)) return true;

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

			btnFire.active = false; btnFire.fingerId = -1;
			btnAim.active = false; btnAim.fingerId = -1;
			btnJump.active = false; btnJump.fingerId = -1;
			btnCrouch.active = false; btnCrouch.fingerId = -1;
			btnReload.active = false; btnReload.fingerId = -1;
			btnChat.active = false; btnChat.fingerId = -1;
			btnScore.active = false; btnScore.fingerId = -1;
			escButton.active = false; escButton.fingerId = -1;
			hksButton.active = false; hksButton.fingerId = -1;

			for (auto& slot : toolSlots) {
				slot.active = false;
				slot.fingerId = -1;
			}
		}

	} // namespace client
} // namespace spades
