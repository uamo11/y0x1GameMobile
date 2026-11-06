/*
 Copyright (c) 2026 ySpades developers.

 This file is part of ySpades Mobile.
 */

#pragma once

#include <string>
#include <vector>
#include <Core/Math.h>
#include <Core/Settings.h>
#include <Client/IImage.h>

namespace spades {
	namespace client {
		class Client;
		class IRenderer;

		struct TouchButton {
			std::string key;
			std::string label;
			Vector2 pos{0.0F, 0.0F};
			float radius = 30.0F;
			float width = 0.0F;
			float height = 0.0F;
			bool isRect = false;
			bool active = false;
			int64_t fingerId = -1;
		};

		/**
		 * Tactical mobile touch HUD for Android:
		 * - Kyro textured analog joystick with dynamic thumb knob
		 * - Tactical combat buttons (Fire with crosshairs, Aim/ADS/Build, Jump, Crouch, Reload)
		 * - Glassmorphic weapon & tool selector cards with active highlight
		 * - Menu & HKS quick toggles
		 */
		class TouchHUD {
		public:
			TouchHUD();

			void UpdateLayout(float screenWidth, float screenHeight);
			void Draw(IRenderer& r, Client& client);

			bool HandleFingerDown(float x, float y, int64_t fingerId, Client& client);
			bool HandleFingerUp(float x, float y, int64_t fingerId, Client& client);
			bool HandleFingerMotion(float x, float y, float dx, float dy, int64_t fingerId, Client& client);

			void Reset(Client& client);

			static bool IsEnabled();

		private:
			float sw = 1280.0F;
			float sh = 720.0F;

			// Textured Assets
			bool assetsLoaded = false;
			Handle<IImage> imgJoystick;
			Handle<IImage> imgKnob;
			Handle<IImage> imgReload;
			Handle<IImage> imgArrow;
			Handle<IImage> imgSpade;
			Handle<IImage> imgBlock;
			Handle<IImage> imgRifle;
			Handle<IImage> imgSMG;
			Handle<IImage> imgShotgun;
			Handle<IImage> imgGrenade;

			void LoadAssets(IRenderer& r);

			// Virtual Joystick (Left Hand)
			bool joystickActive = false;
			int64_t joystickFingerId = -1;
			Vector2 joystickCenter{140.0F, 560.0F};
			Vector2 joystickCurrent{140.0F, 560.0F};
			float joystickBaseRadius = 80.0F;
			float joystickKnobRadius = 36.0F;

			bool keyW = false;
			bool keyS = false;
			bool keyA = false;
			bool keyD = false;
			bool keyShift = false;

			// Look Touchpad (Right Hand)
			bool lookActive = false;
			int64_t lookFingerId = -1;
			Vector2 lookLastPos{0.0F, 0.0F};

			// Combat Buttons (Right Hand)
			TouchButton btnFire;
			TouchButton btnAim;
			TouchButton btnJump;
			TouchButton btnCrouch;
			TouchButton btnReload;
			TouchButton btnChat;

			// Bottom Weapon / Tool Slots
			std::vector<TouchButton> toolSlots;

			// Top quick buttons
			TouchButton escButton;
			TouchButton hksButton;
			TouchButton btnScore;

			void UpdateJoystickKeys(Client& client);
			void ReleaseAllKeys(Client& client);
		};
	} // namespace client
} // namespace spades
