/*
 Copyright (c) 2026 ySpades developers.

 This file is part of ySpades Mobile.
 */

#pragma once

#include <string>
#include <vector>
#include <Core/Math.h>
#include <Core/Settings.h>

namespace spades {
	namespace client {
		class Client;
		class IRenderer;

		struct TouchButton {
			std::string key;
			std::string label;
			Vector2 pos{0.0F, 0.0F};
			float radius = 30.0F;
			bool active = false;
			int64_t fingerId = -1;
		};

		/**
		 * On-screen mobile touch HUD for Android:
		 * - Virtual joystick (movement + auto-sprint)
		 * - Touchpad camera rotation and aiming
		 * - Fire, Build, Jump, Crouch, Reload buttons
		 * - Quick tool switcher (1-4)
		 * - Menu and HKS quick toggles
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

			// Action Buttons
			std::vector<TouchButton> buttons;
			std::vector<TouchButton> toolSlots;
			TouchButton escButton;
			TouchButton hksButton;

			void UpdateJoystickKeys(Client& client);
			void ReleaseAllKeys(Client& client);
		};
	} // namespace client
} // namespace spades
