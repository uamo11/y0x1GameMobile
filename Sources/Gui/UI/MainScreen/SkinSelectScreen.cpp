/*
 Copyright (c) 2026 Fran6nd, ZeroSpades developers.

 This file is part of ZeroSpades, a fork of OpenSpades.

 ZeroSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ZeroSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with ZeroSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "SkinSelectScreen.h"
#include "MainScreenUI.h"

#include <algorithm>
#include <string>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <Client/Fonts.h>
#include <Client/IFont.h>
#include <Client/IRenderer.h>
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <Core/ShellApi.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Widgets/DrawUtils.h>

DEFINE_SPADES_SETTING(cg_playerSkin, "default");
DEFINE_SPADES_SETTING(cg_playerCape, "none");
DEFINE_SPADES_SETTING(cg_playerAccessory, "none");

namespace spades {
	namespace gui {
		using ui::Button;
		using ui::SetColorNP;
		using ui::UIElement;
		using ui::UIManager;

		/** Lightweight native tab button matching the Preferences sidebar style */
		class CosmeticTabButton : public ui::Button {
		public:
			bool isSelected = false;

			CosmeticTabButton(UIManager* manager) : ui::Button(manager) {
				alignment = MakeVector2(0.0F, 0.5F);
			}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bg = MakeVector4(0.08F, 0.12F, 0.18F, 0.65F);
				Vector4 textCol = MakeVector4(0.85F, 0.90F, 0.95F, 0.90F);

				if (isSelected) {
					bg = MakeVector4(0.16F, 0.26F, 0.42F, 0.95F);
					textCol = MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
				} else if (hover) {
					bg = MakeVector4(0.12F, 0.18F, 0.28F, 0.85F);
					textCol = MakeVector4(0.95F, 0.95F, 1.0F, 1.0F);
				}

				SetColorNP(r, bg);
				r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				if (isSelected) {
					SetColorNP(r, MakeVector4(0.40F, 0.85F, 1.0F, 1.0F));
					r.DrawFilledRect(pos.x, pos.y, pos.x + 3.5F, pos.y + sz.y);
				} else {
					SetColorNP(r, MakeVector4(0.15F, 0.20F, 0.30F, 0.50F));
					r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);
				}

				client::IFont* font = GetFont();
				if (font) {
					Vector4 shadow = MakeVector4(0.0F, 0.0F, 0.0F, 0.6F);
					font->DrawShadow(caption, MakeVector2(pos.x + 14.0F, pos.y + (sz.y - font->Measure(caption).y) * 0.5F),
					                1.0F, textCol, shadow);
				}
			}
		};

		/** High performance cosmetic option row with radio checkmark and subtitle */
		class CosmeticOptionRow : public ui::Button {
		public:
			std::string valueId;
			std::string itemTitle;
			std::string subtitle;
			bool selected = false;
			Vector4 colorTag = MakeVector4(0.0F, 0.0F, 0.0F, 0.0F);

			CosmeticOptionRow(UIManager* manager, const std::string& valId,
			                  const std::string& title, const std::string& sub,
			                  Vector4 tag = MakeVector4(0.0F, 0.0F, 0.0F, 0.0F))
			    : ui::Button(manager), valueId(valId), itemTitle(title), subtitle(sub), colorTag(tag) {}

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				Vector4 bg = MakeVector4(0.06F, 0.08F, 0.12F, 0.65F);
				Vector4 border = MakeVector4(0.18F, 0.24F, 0.35F, 0.50F);

				if (selected) {
					bg = MakeVector4(0.11F, 0.22F, 0.36F, 0.90F);
					border = MakeVector4(0.40F, 0.80F, 1.0F, 0.95F);
				} else if (hover) {
					bg = MakeVector4(0.09F, 0.14F, 0.22F, 0.80F);
					border = MakeVector4(0.30F, 0.50F, 0.75F, 0.80F);
				}

				SetColorNP(r, bg);
				r.DrawFilledRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				SetColorNP(r, border);
				r.DrawOutlinedRect(pos.x, pos.y, pos.x + sz.x, pos.y + sz.y);

				// Selection radio/check box
				float chkX = pos.x + 14.0F;
				float chkY = pos.y + (sz.y - 14.0F) * 0.5F;
				SetColorNP(r, MakeVector4(0.20F, 0.30F, 0.45F, 0.80F));
				r.DrawOutlinedRect(chkX, chkY, chkX + 14.0F, chkY + 14.0F);

				if (selected) {
					SetColorNP(r, MakeVector4(0.40F, 0.85F, 1.0F, 1.0F));
					r.DrawFilledRect(chkX + 3.0F, chkY + 3.0F, chkX + 11.0F, chkY + 11.0F);
				}

				// Color tag badge (preview swatch)
				if (valueId == "default" || valueId == "off") {
					float tagW = 14.0F;
					float tagH = 12.0F;
					float tagX = chkX + 22.0F;
					float tagY = pos.y + 11.0F;
					// Dual Blue / Green swatch representing server team colors
					SetColorNP(r, MakeVector4(0.18F, 0.45F, 0.85F, 1.0F));
					r.DrawFilledRect(tagX, tagY, tagX + tagW * 0.5F, tagY + tagH);
					SetColorNP(r, MakeVector4(0.20F, 0.60F, 0.25F, 1.0F));
					r.DrawFilledRect(tagX + tagW * 0.5F, tagY, tagX + tagW, tagY + tagH);
					SetColorNP(r, MakeVector4(1.0F, 1.0F, 1.0F, 0.70F));
					r.DrawOutlinedRect(tagX, tagY, tagX + tagW, tagY + tagH);
				} else if (colorTag.w > 0.0F) {
					float tagW = 12.0F;
					float tagH = 12.0F;
					float tagX = chkX + 22.0F;
					float tagY = pos.y + 11.0F;
					SetColorNP(r, colorTag);
					r.DrawFilledRect(tagX, tagY, tagX + tagW, tagY + tagH);
					SetColorNP(r, MakeVector4(1.0F, 1.0F, 1.0F, 0.60F));
					r.DrawOutlinedRect(tagX, tagY, tagX + tagW, tagY + tagH);
				}

				client::IFont* font = GetFont();
				if (font) {
					float textOffsetX = (colorTag.w > 0.0F || valueId == "default" || valueId == "off") ? 58.0F : 38.0F;
					Vector4 titleCol = selected ? MakeVector4(1.0F, 1.0F, 1.0F, 1.0F)
					                            : MakeVector4(0.90F, 0.92F, 0.96F, 0.90F);
					Vector4 shadowCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.70F);

					// Title
					font->DrawShadow(itemTitle, MakeVector2(pos.x + textOffsetX, pos.y + 7.0F), 0.95F,
					                titleCol, shadowCol);

					// Subtitle
					if (!subtitle.empty()) {
						Vector4 subCol = selected ? MakeVector4(0.65F, 0.85F, 1.0F, 0.85F)
						                          : MakeVector4(0.55F, 0.65F, 0.75F, 0.75F);
						font->DrawShadow(subtitle, MakeVector2(pos.x + textOffsetX, pos.y + 24.0F), 0.75F,
						                subCol, shadowCol);
					}
				}
			}
		};

		SkinSelectScreen::SkinSelectScreen(ui::UIElement* owner, client::FontManager* fontManager,
		                                   MainScreenUI* mainScreenUI)
		    : UIElement(&owner->GetManager()), owner(owner), fontManager(fontManager),
		      mainScreenUI(mainScreenUI) {
			UIManager* manager = &GetManager();
			SetFont(&fontManager->GetGuiFont());
			SetBounds(manager->GetRootElement().GetBounds());

			float sw = manager->screenWidth;
			float sh = manager->screenHeight;

#if defined(__ANDROID__) || defined(ANDROID) || defined(ZEROSPADES_MOBILE)
			float dlgW = std::min(580.0F, sw * 0.60F);
			float dlgH = std::min(580.0F, sh - 20.0F);
			float dlgX = 24.0F;
			float dlgY = (sh - dlgH) * 0.5F;

			float sidebarW = 160.0F;
			float tabH = 42.0F;
			float tabGap = 4.0F;
			float btnH = 38.0F;
			float btnGap = 6.0F;
			float rowH = 46.0F;
#else
			float dlgW = std::min(480.0F, sw * 0.48F);
			float dlgH = std::min(550.0F, sh - 32.0F);
			float dlgX = 24.0F;
			float dlgY = (sh - dlgH) * 0.5F;

			float sidebarW = 135.0F;
			float tabH = 34.0F;
			float tabGap = 3.0F;
			float btnH = 30.0F;
			float btnGap = 4.0F;
			float rowH = 40.0F;
#endif

			// --- 1. Sidebar Tabs ---
			{
				Handle<CosmeticTabButton> btn = Handle<CosmeticTabButton>::New(manager);
				tabSkins = btn.GetPointerOrNull();
				tabSkins->caption = _Tr("MainScreen", "Trajes / Skins");
				tabSkins->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F, sidebarW, tabH));
				tabSkins->isSelected = true;
				tabSkins->activated = [this](UIElement&) { SwitchCategory(0); };
				AddChild(tabSkins);
			}
			{
				Handle<CosmeticTabButton> btn = Handle<CosmeticTabButton>::New(manager);
				tabCapes = btn.GetPointerOrNull();
				tabCapes->caption = _Tr("MainScreen", "Capas 3D");
				tabCapes->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F + (tabH + tabGap), sidebarW, tabH));
				tabCapes->activated = [this](UIElement&) { SwitchCategory(1); };
				AddChild(tabCapes);
			}
			{
				Handle<CosmeticTabButton> btn = Handle<CosmeticTabButton>::New(manager);
				tabAccessories = btn.GetPointerOrNull();
				tabAccessories->caption = _Tr("MainScreen", "Accesorios");
				tabAccessories->SetBounds(AABB2(dlgX + 8.0F, dlgY + 45.0F + 2.0F * (tabH + tabGap), sidebarW, tabH));
				tabAccessories->activated = [this](UIElement&) { SwitchCategory(2); };
				AddChild(tabAccessories);
			}

			// --- 2. Sidebar Action Buttons (Bottom) ---
			float bottomY = dlgY + dlgH - 12.0F;

			{
				Handle<Button> btn = Handle<Button>::New(manager);
				cancelButton = btn.GetPointerOrNull();
				cancelButton->caption = _Tr("MainScreen", "Atrás [Esc]");
				cancelButton->SetBounds(AABB2(dlgX + 8.0F, bottomY - btnH, sidebarW, btnH));
				cancelButton->activated = [this](UIElement& s) { OnCancel(s); };
				AddChild(cancelButton);
			}
			{
				Handle<Button> btn = Handle<Button>::New(manager);
				saveButton = btn.GetPointerOrNull();
				saveButton->caption = _Tr("MainScreen", "Guardar");
				saveButton->SetBounds(AABB2(dlgX + 8.0F, bottomY - 2.0F * btnH - btnGap, sidebarW, btnH));
				saveButton->activated = [this](UIElement& s) { OnSave(s); };
				AddChild(saveButton);
			}
			{
				Handle<Button> btn = Handle<Button>::New(manager);
				openFolderButton = btn.GetPointerOrNull();
				openFolderButton->caption = _Tr("MainScreen", "Abrir Carpeta");
				openFolderButton->SetBounds(AABB2(dlgX + 8.0F, bottomY - 3.0F * btnH - 2.0F * btnGap, sidebarW, btnH));
				openFolderButton->activated = [this](UIElement& s) { OnOpenFolder(s); };
				AddChild(openFolderButton);
			}

			// --- 3. Content Panels ---
			float contentX = dlgX + sidebarW + 12.0F;
			float contentY = dlgY + 45.0F;
			float contentW = dlgW - sidebarW - 20.0F;
			float contentH = dlgH - 58.0F;

			float rowGap = 4.0F;

			// A. Skins Panel
			{
				Handle<UIElement> pnl = Handle<UIElement>::New(manager);
				skinsPanel = pnl.GetPointerOrNull();
				skinsPanel->SetBounds(AABB2(contentX, contentY, contentW, contentH));
				AddChild(skinsPanel);

				std::string curSkin = cg_playerSkin;

				struct SkinDef {
					std::string id;
					std::string title;
					std::string desc;
					Vector4 swatch;
				};
				std::vector<SkinDef> defs = {
					{"default", "OFF - Servidor / Equipo Real", "Desactiva skins personalizadas y usa el color/uniforme oficial del equipo en el servidor", MakeVector4(0.0F, 0.0F, 0.0F, 0.0F)},
					{"blue", "Azul Clásico", "Uniforme azul fijo para el jugador", MakeVector4(0.18F, 0.45F, 0.85F, 1.0F)},
					{"green", "Verde Militar", "Camuflaje táctico para combate en jungla", MakeVector4(0.20F, 0.60F, 0.25F, 1.0F)},
					{"red", "Rojo Asalto", "Uniforme ofensivo escarlata de vanguardia", MakeVector4(0.85F, 0.22F, 0.22F, 1.0F)},
					{"gold", "Traje Dorado VIP", "Traje ceremonial dorado para campeones", MakeVector4(1.0F, 0.84F, 0.15F, 1.0F)},
					{"black", "Sombra Táctica", "Uniforme negro para operaciones nocturnas", MakeVector4(0.12F, 0.12F, 0.15F, 1.0F)},
					{"white", "Nieve Ártica", "Camuflaje blanco de operaciones invernales", MakeVector4(0.95F, 0.95F, 0.95F, 1.0F)},
					{"custom", "Skin PNG Personalizada", "Carga skin.png (formato 64x64 Minecraft) desde la carpeta Skins", MakeVector4(0.40F, 0.85F, 1.0F, 1.0F)}
				};

				float y = 0.0F;
				for (const auto& d : defs) {
					Handle<CosmeticOptionRow> row =
					    Handle<CosmeticOptionRow>::New(manager, d.id, d.title, d.desc, d.swatch);
					row->SetBounds(AABB2(0.0F, y, contentW, rowH));
					if (curSkin == d.id)
						row->selected = true;
					else if ((curSkin.empty() || curSkin == "default" || curSkin == "off") && d.id == "default")
						row->selected = true;

					row->activated = [this, r = row.GetPointerOrNull()](UIElement&) {
						for (auto* sr : skinRows)
							sr->selected = (sr == r);
						cg_playerSkin = r->valueId;
						if (this->mainScreenUI)
							this->mainScreenUI->UpdateLobbyCosmetics();
					};
					skinRows.push_back(row.GetPointerOrNull());
					skinsPanel->AddChild(row.GetPointerOrNull());
					y += rowH + rowGap;
				}
			}

			// B. Capes Panel
			{
				Handle<UIElement> pnl = Handle<UIElement>::New(manager);
				capesPanel = pnl.GetPointerOrNull();
				capesPanel->SetBounds(AABB2(contentX, contentY, contentW, contentH));
				capesPanel->visible = false;
				AddChild(capesPanel);

				std::string curCape = cg_playerCape;

				struct CapeDef {
					std::string id;
					std::string title;
					std::string desc;
					Vector4 swatch;
				};
				std::vector<CapeDef> defs = {
					{"none", "Sin Capa", "Espalda despejada sin capa", MakeVector4(0.3F, 0.3F, 0.3F, 0.5F)},
					{"gold", "Capa Dorada VIP", "Capa dorada brillante con física 3D en la espalda", MakeVector4(1.0F, 0.84F, 0.15F, 1.0F)},
					{"classic", "Capa Clásica Azul", "Capa azul con insignia de combate", MakeVector4(0.18F, 0.45F, 0.85F, 1.0F)},
					{"black", "Capa Negra Sombra", "Capa oscura de tela pesada", MakeVector4(0.12F, 0.12F, 0.15F, 1.0F)},
					{"red", "Capa Roja Asalto", "Capa carmesí de honor militar", MakeVector4(0.85F, 0.22F, 0.22F, 1.0F)}
				};

				float y = 0.0F;
				for (const auto& d : defs) {
					Handle<CosmeticOptionRow> row =
					    Handle<CosmeticOptionRow>::New(manager, d.id, d.title, d.desc, d.swatch);
					row->SetBounds(AABB2(0.0F, y, contentW, rowH));
					if (curCape == d.id)
						row->selected = true;
					else if (curCape.empty() && d.id == "none")
						row->selected = true;

					row->activated = [this, r = row.GetPointerOrNull()](UIElement&) {
						for (auto* cr : capeRows)
							cr->selected = (cr == r);
						cg_playerCape = r->valueId;
						if (this->mainScreenUI)
							this->mainScreenUI->UpdateLobbyCosmetics();
					};
					capeRows.push_back(row.GetPointerOrNull());
					capesPanel->AddChild(row.GetPointerOrNull());
					y += rowH + rowGap;
				}
			}

			// C. Accessories Panel
			{
				Handle<UIElement> pnl = Handle<UIElement>::New(manager);
				accPanel = pnl.GetPointerOrNull();
				accPanel->SetBounds(AABB2(contentX, contentY, contentW, contentH));
				accPanel->visible = false;
				AddChild(accPanel);

				std::string curAcc = cg_playerAccessory;

				struct AccDef {
					std::string id;
					std::string title;
					std::string desc;
					Vector4 swatch;
				};
				std::vector<AccDef> defs = {
					{"none", "Ninguno", "Sin accesorios en la cabeza", MakeVector4(0.3F, 0.3F, 0.3F, 0.5F)},
					{"crown", "Corona Real de Oro", "Corona de oro 3D con joyas de rubí para campeones", MakeVector4(1.0F, 0.84F, 0.15F, 1.0F)},
					{"helmet", "Casco Militar Táctico", "Casco de combate balístico de camuflaje", MakeVector4(0.25F, 0.45F, 0.30F, 1.0F)}
				};

				float y = 0.0F;
				for (const auto& d : defs) {
					Handle<CosmeticOptionRow> row =
					    Handle<CosmeticOptionRow>::New(manager, d.id, d.title, d.desc, d.swatch);
					row->SetBounds(AABB2(0.0F, y, contentW, rowH));
					if (curAcc == d.id)
						row->selected = true;
					else if (curAcc.empty() && d.id == "none")
						row->selected = true;

					row->activated = [this, r = row.GetPointerOrNull()](UIElement&) {
						for (auto* ar : accRows)
							ar->selected = (ar == r);
						cg_playerAccessory = r->valueId;
						if (this->mainScreenUI)
							this->mainScreenUI->UpdateLobbyCosmetics();
					};
					accRows.push_back(row.GetPointerOrNull());
					accPanel->AddChild(row.GetPointerOrNull());
					y += rowH + rowGap;
				}
			}
		}

		void SkinSelectScreen::SwitchCategory(int index) {
			activeCategory = index;
			if (tabSkins)
				tabSkins->isSelected = (index == 0);
			if (tabCapes)
				tabCapes->isSelected = (index == 1);
			if (tabAccessories)
				tabAccessories->isSelected = (index == 2);

			if (skinsPanel)
				skinsPanel->visible = (index == 0);
			if (capesPanel)
				capesPanel->visible = (index == 1);
			if (accPanel)
				accPanel->visible = (index == 2);
		}

		void SkinSelectScreen::OnOpenFolder(UIElement&) {
#ifdef _WIN32
			_mkdir("Resources/Skins");
			_mkdir("Skins");
#else
			mkdir("Resources/Skins", 0755);
			mkdir("Skins", 0755);
#endif
			if (FileManager::FileExists("Resources/Skins"))
				ShowDirectoryInShell("Resources/Skins");
			else
				ShowDirectoryInShell("Skins");
		}

		void SkinSelectScreen::OnSave(UIElement&) {
			Settings::GetInstance()->Flush();
			if (mainScreenUI)
				mainScreenUI->UpdateLobbyCosmetics();
			Close();
		}

		void SkinSelectScreen::OnCancel(UIElement&) {
			Close();
		}

		void SkinSelectScreen::Run() {
			owner->enable = false;
			owner->GetParent()->AddChild(this);
			if (mainScreenUI)
				mainScreenUI->SetActiveSkinScreen(this);
		}

		void SkinSelectScreen::Close() {
			if (mainScreenUI)
				mainScreenUI->SetActiveSkinScreen(nullptr);
			owner->enable = true;
			if (GetParent())
				GetParent()->RemoveChild(this);
		}

		void SkinSelectScreen::SelectCustomSkin() {
			SwitchCategory(0);
			for (auto* sr : skinRows) {
				if (sr && sr->valueId == "custom")
					sr->selected = true;
				else if (sr)
					sr->selected = false;
			}
		}

		void SkinSelectScreen::HotKey(const std::string& key) {
			if (IsEnabled() && key == "Escape") {
				OnCancel(*this);
				return;
			} else if (IsEnabled() && key == "Enter") {
				OnSave(*this);
				return;
			}
			UIElement::HotKey(key);
		}

		void SkinSelectScreen::Render() {
			client::IRenderer& r = GetManager().GetRenderer();
			float sw = GetManager().screenWidth;
			float sh = GetManager().screenHeight;

			float dlgW = std::min(480.0F, sw * 0.48F);
			float dlgH = std::min(550.0F, sh - 32.0F);
			float dlgX = 24.0F;
			float dlgY = (sh - dlgH) * 0.5F;

			// Dimmed background overlay ONLY behind the left panel
			// The right side where the 3D player stands remains clear, bright, and interactive!
			SetColorNP(r, MakeVector4(0.0F, 0.0F, 0.0F, 0.40F));
			r.DrawFilledRect(0.0F, 0.0F, dlgX + dlgW + 24.0F, sh);

			// Main dialog body
			SetColorNP(r, MakeVector4(0.04F, 0.06F, 0.09F, 0.96F));
			r.DrawFilledRect(dlgX, dlgY, dlgX + dlgW, dlgY + dlgH);

			// Dialog border
			SetColorNP(r, MakeVector4(0.20F, 0.35F, 0.55F, 0.70F));
			r.DrawOutlinedRect(dlgX, dlgY, dlgX + dlgW, dlgY + dlgH);

			// Glowing top header strip
			SetColorNP(r, MakeVector4(0.25F, 0.65F, 1.0F, 0.90F));
			r.DrawFilledRect(dlgX, dlgY, dlgX + dlgW, dlgY + 3.0F);

			// Vertical separator between sidebar and content
			float sidebarW = 135.0F;
			SetColorNP(r, MakeVector4(0.15F, 0.22F, 0.32F, 0.60F));
			r.DrawFilledRect(dlgX + sidebarW + 6.0F, dlgY + 40.0F, dlgX + sidebarW + 7.5F, dlgY + dlgH - 12.0F);

			if (fontManager) {
				client::IFont& font = fontManager->GetGuiFont();
				Vector4 shadowCol = MakeVector4(0.0F, 0.0F, 0.0F, 0.7F);

				// Dialog title
				font.DrawShadow(_Tr("MainScreen", "PERSONALIZACION DE SKINS"),
				                MakeVector2(dlgX + 16.0F, dlgY + 14.0F), 1.05F,
				                MakeVector4(0.95F, 0.98F, 1.0F, 1.0F), shadowCol);
			}

			UIElement::Render();
		}
	} // namespace gui
} // namespace spades
