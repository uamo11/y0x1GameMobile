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

#include "LocalWorldNetClient.h"
#include "Client.h"
#include "ChatWindow.h"
#include <algorithm>
#include <cmath>
#include <Core/Debug.h>
#include <Core/Settings.h>

SPADES_SETTING(cg_playerName);

namespace spades {
	namespace client {
		LocalWorldNetClient::LocalWorldNetClient(Client* c, const std::string& path)
		    : client(c),
		      mapPath(path),
		      status(NetClientStatusConnecting),
		      statusString("Cargando mundo local..."),
		      localBlockColor(MakeIntVector3(128, 128, 128)),
		      initialized(false) {
			properties = std::make_shared<GameProperties>(ProtocolVersion::v075);
			properties->isStaff = false;
		}

		bool LocalWorldNetClient::OpenWorld() {
			std::string path = mapPath;
			std::unique_ptr<IStream> stream;

			// 1. Try exact path if it exists
			if (FileManager::FileExists(path.c_str())) {
				try {
					stream = FileManager::OpenForReading(path.c_str());
				} catch (...) {
					stream.reset();
				}
			}

			// 2. Try prefixing Maps/ if not already present
			if (!stream && path.find("Maps/") != 0 && path.find("Maps\\") != 0) {
				std::string candidate = "Maps/" + path;
				if (FileManager::FileExists(candidate.c_str())) {
					try {
						stream = FileManager::OpenForReading(candidate.c_str());
						path = candidate;
					} catch (...) {
						stream.reset();
					}
				}
			}

			// 3. Fallback: try stripping Maps/ if it failed with prefix
			if (!stream && (path.find("Maps/") == 0 || path.find("Maps\\") == 0)) {
				std::string candidate = path.substr(5);
				if (FileManager::FileExists(candidate.c_str())) {
					try {
						stream = FileManager::OpenForReading(candidate.c_str());
						path = candidate;
					} catch (...) {
						stream.reset();
					}
				}
			}

			// 4. Last resort: try OpenForReading directly inside try/catch
			if (!stream) {
				try {
					stream = FileManager::OpenForReading(path.c_str());
				} catch (...) {
					try {
						stream = FileManager::OpenForReading(("Maps/" + path).c_str());
					} catch (...) {
						stream.reset();
					}
				}
			}

			if (!stream) {
				statusString = "No se pudo abrir el mapa: " + mapPath;
				return false;
			}

			GameMap* map = GameMap::Load(stream.get());
			if (!map) {
				statusString = "Error al decodificar mapa .vxl";
				return false;
			}

			World* w = new World(properties);
			w->SetMap(map);
			map->Release();

			client->SetWorld(w);

			int pId = 0;
			w->SetLocalPlayerIndex(pId);

			// Search for suitable ground spawn position near map center
			int spawnX = 256;
			int spawnY = 256;
			int spawnZ = 32;
			bool foundGround = false;

			// Spiral search outward from center to find solid ground above water
			for (int r = 0; r <= 240 && !foundGround; r += 16) {
				for (int dx = -r; dx <= r && !foundGround; dx += 16) {
					for (int dy = -r; dy <= r && !foundGround; dy += 16) {
						int cx = std::clamp(256 + dx, 16, 496);
						int cy = std::clamp(256 + dy, 16, 496);
						int topZ = map->GetTop(cx, cy);
						if (topZ > 2 && topZ < 62) {
							spawnX = cx;
							spawnY = cy;
							spawnZ = topZ - 2;
							foundGround = true;
							break;
						}
					}
				}
			}

			if (!foundGround) {
				spawnX = 100;
				spawnY = 256;
				int topZ = map->GetTop(spawnX, spawnY);
				spawnZ = (topZ > 2 && topZ < 62) ? topZ - 2 : 30;
			}

			Vector3 pos = MakeVector3(static_cast<float>(spawnX) + 0.5F,
			                          static_cast<float>(spawnY) + 0.5F,
			                          static_cast<float>(spawnZ) - 0.4F);

			auto p = stmp::make_unique<Player>(*w, pId, RIFLE_WEAPON, 0);
			p->SetPosition(pos);

			// Face towards the center of the map horizontally
			Vector3 lookDir = MakeVector3(256.0F - static_cast<float>(spawnX),
			                              256.0F - static_cast<float>(spawnY),
			                              0.0F);
			if (lookDir.GetSquaredLength2D() < 1.0F)
				lookDir = MakeVector3(1.0F, 0.0F, 0.0F);
			else
				lookDir = lookDir.Normalize();
			p->SetOrientation(lookDir);

			// Default block color for local building
			p->SetHeldBlockColor(MakeIntVector3(255, 255, 255));

			// Atmospheric sky and fog color (bright daytime tropical blue)
			w->SetFogColor(MakeIntVector3(140, 195, 245));

			Player& pRef = *p;
			w->SetPlayer(pId, std::move(p));

			std::string pName = StripNewlines(cg_playerName.operator std::string());
			if (pName.empty())
				pName = "Deuce";
			w->GetPlayerPersistent(pId).name = pName;

			client->LocalPlayerCreated();
			client->PlayerJoinedTeam(pRef);
			client->PlayerSpawned(pRef);

			status = NetClientStatusConnected;
			statusString = "Conectado al mundo local";
			initialized = true;
			return true;
		}

		void LocalWorldNetClient::DoEvents(float dt) {
			(void)dt;
		}

		void LocalWorldNetClient::SendBlockAction(IntVector3 v, BlockActionType type) {
			auto wOpt = client->GetWorld();
			if (!wOpt)
				return;
			World& w = *wOpt;

			if (type == BlockActionDig || type == BlockActionTool) {
				std::vector<IntVector3> cells;
				cells.push_back(v);
				w.DestroyBlock(cells);
				client->PlayerDestroyedBlockWithWeaponOrTool(v);
			} else if (type == BlockActionCreate) {
				w.CreateBlock(v, localBlockColor);
				auto pOpt = w.GetPlayer(0);
				if (pOpt) {
					client->PlayerCreatedBlock(pOpt.value());
				}
			}
		}

		void LocalWorldNetClient::SendBlockLine(IntVector3 v1, IntVector3 v2) {
			auto wOpt = client->GetWorld();
			if (!wOpt)
				return;
			World& w = *wOpt;

			auto cells = w.CubeLine(v1, v2, 50);
			for (const auto& c : cells) {
				w.CreateBlock(c, localBlockColor);
			}
		}

		void LocalWorldNetClient::SendChat(std::string text, bool global) {
			(void)global;
			if (client->chatWindow)
				client->chatWindow->AddMessage("[Mundo Local] " + text);
		}
	} // namespace client
} // namespace spades
