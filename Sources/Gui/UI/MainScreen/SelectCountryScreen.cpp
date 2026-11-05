/*
 Copyright (c) 2026 Fran6nd, ZeroSpades developers.

 This file is part of ZeroSpades, a fork of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "SelectCountryScreen.h"

#include <algorithm>
#include <cctype>

#include <Client/Fonts.h>
#include <Client/IFont.h>
#include <Client/IRenderer.h>
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Gui/UI/Framework/UIManager.h>
#include <Gui/UI/Widgets/DrawUtils.h>
#include <Gui/UI/Widgets/Label.h>

SPADES_SETTING(cg_country);
SPADES_SETTING(cg_countryIsSet);

namespace spades {
	namespace gui {
		using ui::Field;
		using ui::Label;
		using ui::ListView;
		using ui::ListViewModel;
		using ui::SetColorNP;
		using ui::UIElement;
		using ui::UIManager;

		class SelectFlagCard : public ui::ButtonBase {
			SelectCountryScreen* screen;
			CountryInfo info;
			FlagIconRenderer* flagRenderer;

		public:
			SelectFlagCard(UIManager* manager, SelectCountryScreen* screen,
			               CountryInfo info, FlagIconRenderer* flagRenderer)
			    : ui::ButtonBase(manager),
			      screen(screen),
			      info(std::move(info)),
			      flagRenderer(flagRenderer) {}

			const std::string& GetCode() const { return info.code; }

			void Render() override {
				client::IRenderer& r = GetManager().GetRenderer();
				Vector2 pos = GetScreenPosition();
				Vector2 sz = size;

				bool isSelected = (screen != nullptr && info.code == cg_country);

				// Card background
				Vector4 bg = MakeVector4(0.07F, 0.09F, 0.13F, 0.90F);
				if (isSelected) {
					bg = MakeVector4(0.16F, 0.36F, 0.68F, 0.95F);
				} else if (pressed && hover) {
					bg = MakeVector4(0.22F, 0.30F, 0.44F, 0.95F);
				} else if (hover) {
					bg = MakeVector4(0.13F, 0.19F, 0.29F, 0.90F);
				}

				SetColorNP(r, bg);
				r.DrawImage(nullptr, AABB2(pos.x, pos.y, sz.x, sz.y));

				// Border line
				Vector4 borderColor = MakeVector4(0.22F, 0.28F, 0.38F, 0.45F);
				float borderThick = 1.0F;
				if (isSelected) {
					borderColor = MakeVector4(0.38F, 0.75F, 1.0F, 1.0F);
					borderThick = 2.0F;
				} else if (hover) {
					borderColor = MakeVector4(0.55F, 0.82F, 1.0F, 0.9F);
				}

				SetColorNP(r, borderColor);
				r.DrawImage(nullptr, AABB2(pos.x, pos.y, sz.x, borderThick));
				r.DrawImage(nullptr, AABB2(pos.x, pos.y + sz.y - borderThick, sz.x, borderThick));
				r.DrawImage(nullptr, AABB2(pos.x, pos.y, borderThick, sz.y));
				r.DrawImage(nullptr, AABB2(pos.x + sz.x - borderThick, pos.y, borderThick, sz.y));

				// Large Flag (56x38 px)
				float flagW = 56.0F;
				float flagH = 38.0F;
				float flagX = pos.x + (sz.x - flagW) * 0.5F;
				float flagY = pos.y + (sz.y - flagH) * 0.5F;
				AABB2 flagRect(flagX, flagY, flagW, flagH);

				if (info.code == "NONE" || info.code.empty()) {
					// Draw Empty Flag (crisp white banner with flagpole)
					// White fabric body
					SetColorNP(r, MakeVector4(0.92F, 0.93F, 0.95F, 1.0F));
					r.DrawImage(nullptr, flagRect);

					// Fabric border outline
					SetColorNP(r, MakeVector4(0.40F, 0.45F, 0.55F, 0.65F));
					r.DrawImage(nullptr, AABB2(flagRect.min.x, flagRect.min.y, flagW, 1.0F));
					r.DrawImage(nullptr, AABB2(flagRect.min.x, flagRect.max.y - 1.0F, flagW, 1.0F));
					r.DrawImage(nullptr, AABB2(flagRect.min.x, flagRect.min.y, 1.0F, flagH));
					r.DrawImage(nullptr, AABB2(flagRect.max.x - 1.0F, flagRect.min.y, 1.0F, flagH));

					// Flagpole on left edge
					SetColorNP(r, MakeVector4(0.70F, 0.72F, 0.78F, 0.9F));
					r.DrawImage(nullptr, AABB2(flagRect.min.x - 3.0F, flagRect.min.y - 2.0F, 2.0F, flagH + 6.0F));

					// Golden finial on top of pole
					SetColorNP(r, MakeVector4(0.95F, 0.80F, 0.25F, 1.0F));
					r.DrawImage(nullptr, AABB2(flagRect.min.x - 4.0F, flagRect.min.y - 4.0F, 4.0F, 3.0F));
				} else if (flagRenderer) {
					flagRenderer->DrawIcon(info.code, flagRect);
				}
			}
		};

		class SelectCountryRow : public ui::UIElement {
		public:
			SelectCountryRow(UIManager* manager) : ui::UIElement(manager) {}
		};

		class SelectCountryListModel : public ui::ListViewModel {
			UIManager* manager;
			SelectCountryScreen* screen;
			const std::vector<CountryInfo>& items;
			FlagIconRenderer* flagRenderer;
			int numCols;
			float cardW;
			float cardH;
			float cardGap;

		public:
			SelectCountryListModel(UIManager* manager, SelectCountryScreen* screen,
			                       const std::vector<CountryInfo>& items,
			                       FlagIconRenderer* flagRenderer,
			                       int numCols, float cardW, float cardH, float cardGap)
			    : manager(manager),
			      screen(screen),
			      items(items),
			      flagRenderer(flagRenderer),
			      numCols(numCols),
			      cardW(cardW),
			      cardH(cardH),
			      cardGap(cardGap) {}

			int GetNumRows() override {
				if (numCols <= 0 || items.empty())
					return 0;
				return (static_cast<int>(items.size()) + numCols - 1) / numCols;
			}

			Handle<UIElement> CreateElement(int row) override {
				if (row < 0 || row >= GetNumRows())
					return Handle<UIElement>();

				Handle<SelectCountryRow> rowElem = Handle<SelectCountryRow>::New(manager);
				rowElem->SetBounds(AABB2(0.0F, 0.0F,
				                         static_cast<float>(numCols) * cardW +
				                             static_cast<float>(numCols - 1) * cardGap,
				                         cardH + cardGap));

				for (int c = 0; c < numCols; ++c) {
					int idx = row * numCols + c;
					if (idx >= static_cast<int>(items.size()))
						break;

					Handle<SelectFlagCard> card =
					    Handle<SelectFlagCard>::New(manager, screen, items[idx], flagRenderer);
					card->SetBounds(AABB2(static_cast<float>(c) * (cardW + cardGap), 0.0F, cardW, cardH));

					std::string code = items[idx].code;
					card->activated = [this, code](UIElement&) {
						screen->ConfirmCode(code);
					};

					rowElem->AddChild(card.GetPointerOrNull());
				}

				return rowElem.Cast<UIElement>();
			}
		};

		static std::string ToLowerString(const std::string& str) {
			std::string out = str;
			for (char& c : out)
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			return out;
		}

		SelectCountryScreen::SelectCountryScreen(UIElement* owner,
		                                         client::FontManager* fontManager,
		                                         std::function<void(const std::string&)> onSelected,
		                                         bool canCancel)
		    : UIElement(&owner->GetManager()),
		      owner(owner),
		      fontManager(fontManager),
		      onSelected(std::move(onSelected)),
		      canCancel(canCancel) {
			UIManager* manager = &GetManager();
			flagIconRenderer = Handle<FlagIconRenderer>::New(&manager->GetRenderer());

			selectedCode = static_cast<std::string>(cg_country);
			if (selectedCode.empty())
				selectedCode = "US";

			// First option: Empty Flag (Bandera Vacía)
			CountryInfo emptyFlag;
			emptyFlag.code = "NONE";
			emptyFlag.name = "Sin bandera none empty vacia blanca neutral";
			allCountries.push_back(emptyFlag);

			// Supported country list
			const auto& supported = FlagIconRenderer::GetSupportedCountries();
			for (const auto& c : supported) {
				allCountries.push_back(c);
			}

			filteredCountries = allCountries;

			float sw = manager->screenWidth;
			float sh = manager->screenHeight;
			SetBounds(AABB2(0.0F, 0.0F, sw, sh));

			// Solid black background overlay covering entire screen
			{
				Handle<Label> overlay = Handle<Label>::New(manager);
				overlay->backgroundColor = MakeVector4(0.0F, 0.0F, 0.0F, 1.0F);
				overlay->SetBounds(AABB2(0.0F, 0.0F, sw, sh));
				AddChild(overlay.GetPointerOrNull());
			}

			// Minimalist search field at top
			float fieldW = 360.0F;
			float fieldH = 32.0F;
			float fieldX = (sw - fieldW) * 0.5F;
			float fieldY = 24.0F;
			{
				Handle<Field> field = Handle<Field>::New(manager);
				field->SetBounds(AABB2(fieldX, fieldY, fieldW, fieldH));
				field->placeholder = _Tr("SelectCountryScreen", "Buscar...");
				field->changed = [this](UIElement& s) { OnFilterChanged(s); };
				filterField = field.GetPointerOrNull();
				AddChild(filterField);
			}

			// Grid parameters
			float cardW = 74.0F;
			float cardH = 52.0F;
			float cardGap = 8.0F;
			int numCols = std::max(4, static_cast<int>((sw - 80.0F) / (cardW + cardGap)));
			if (numCols > 8)
				numCols = 8;

			float gridW = static_cast<float>(numCols) * cardW + static_cast<float>(numCols - 1) * cardGap;
			float listW = gridW + 18.0F;
			float listX = (sw - listW) * 0.5F;
			float listY = fieldY + fieldH + 16.0F;
			float listH = sh - listY - 24.0F;

			// Country list (grid of large flags)
			{
				Handle<ListView> lv = Handle<ListView>::New(manager);
				lv->rowHeight = cardH + cardGap;
				lv->SetBounds(AABB2(listX, listY, listW, listH));
				countryList = lv.GetPointerOrNull();
				AddChild(countryList);
			}

			RebuildModel();
		}

		void SelectCountryScreen::RebuildModel() {
			if (!countryList)
				return;

			float sw = GetManager().screenWidth;
			float cardW = 74.0F;
			float cardH = 52.0F;
			float cardGap = 8.0F;
			int numCols = std::max(4, static_cast<int>((sw - 80.0F) / (cardW + cardGap)));
			if (numCols > 8)
				numCols = 8;

			listModel = Handle<SelectCountryListModel>::New(&GetManager(), this,
			                                                filteredCountries,
			                                                flagIconRenderer.GetPointerOrNull(),
			                                                numCols, cardW, cardH, cardGap);
			countryList->SetModel(listModel.Cast<ListViewModel>().GetPointerOrNull());
		}

		void SelectCountryScreen::ApplyFilter(const std::string& query) {
			std::string q = ToLowerString(query);
			filteredCountries.clear();

			for (const auto& c : allCountries) {
				if (q.empty() || ToLowerString(c.name).find(q) != std::string::npos ||
				    ToLowerString(c.code).find(q) != std::string::npos) {
					filteredCountries.push_back(c);
				}
			}

			RebuildModel();
		}

		void SelectCountryScreen::OnFilterChanged(UIElement&) {
			if (filterField)
				ApplyFilter(filterField->GetText());
		}

		void SelectCountryScreen::ConfirmCode(const std::string& code) {
			selectedCode = code;
			cg_country = code;
			cg_countryIsSet = 1;
			Settings::GetInstance()->Flush();
			if (onSelected)
				onSelected(selectedCode);
			Close();
		}

		void SelectCountryScreen::Run() {
			owner->enable = false;
			owner->GetParent()->AddChild(this);
			if (filterField)
				GetManager().SetActiveElement(filterField);
		}

		void SelectCountryScreen::Close() {
			owner->enable = true;
			if (GetParent())
				GetParent()->RemoveChild(this);
			if (closed)
				closed(*this);
		}

		void SelectCountryScreen::HotKey(const std::string& key) {
			if (IsEnabled() && key == "Escape") {
				if (!canCancel || !static_cast<bool>(cg_countryIsSet)) {
					ConfirmCode("NONE");
				} else {
					Close();
				}
				return;
			}
			UIElement::HotKey(key);
		}

		void SelectCountryScreen::Render() {
			client::IRenderer& r = GetManager().GetRenderer();
			float sw = GetManager().screenWidth;
			float sh = GetManager().screenHeight;

			// Ensure solid black background under everything
			SetColorNP(r, MakeVector4(0.0F, 0.0F, 0.0F, 1.0F));
			r.DrawImage(nullptr, AABB2(0.0F, 0.0F, sw, sh));

			UIElement::Render();
		}
	} // namespace gui
} // namespace spades
