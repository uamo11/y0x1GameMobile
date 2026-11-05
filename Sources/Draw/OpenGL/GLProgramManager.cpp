/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

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

#include "GLProgramManager.h"
#include "GLDynamicLightShader.h"
#include "GLProgram.h"
#include "GLSettings.h"
#include "GLShader.h"
#include "GLShadowMapShader.h"
#include "GLShadowShader.h"
#include "IGLShadowMapRenderer.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/FileManager.h>
#include <Core/Math.h>
#include <Core/Stopwatch.h>
#include <Core/TMPUtils.h>
#include <sstream>
#include <unordered_set>

namespace spades {
	namespace draw {
		GLProgramManager::GLProgramManager(IGLDevice& d, GLSettings& settings)
		    : device(d), settings(settings) {
			SPADES_MARK_FUNCTION();
		}

		GLProgramManager::~GLProgramManager() { SPADES_MARK_FUNCTION(); }

		GLProgram* GLProgramManager::RegisterProgram(const std::string& name) {
			SPADES_MARK_FUNCTION();

			auto it = programs.find(name);
			if (it == programs.end()) {
				auto program = CreateProgram(name);
				GLProgram* programPtr = program.get();
				programs[name] = std::move(program);
				return programPtr;
			} else {
				return it->second.get();
			}
		}

		GLShader* GLProgramManager::RegisterShader(const std::string& name) {
			SPADES_MARK_FUNCTION();

			auto it = shaders.find(name);
			if (it == shaders.end()) {
				auto shader = CreateShader(name);
				GLShader* shaderPtr = shader.get();
				shaders[name] = std::move(shader);
				return shaderPtr;
			} else {
				return it->second.get();
			}
		}

		std::unique_ptr<GLProgram> GLProgramManager::CreateProgram(const std::string& name) {
			SPADES_MARK_FUNCTION();

			SPLog("Loading GLSL program '%s'", name.c_str());
			std::string text = FileManager::ReadAllBytes(name.c_str());
			std::vector<std::string> lines = SplitIntoLines(text);

			std::vector<std::string> vsFiles;
			std::vector<std::string> fsFiles;
			std::vector<std::string> gsFiles;

			auto addFile = [&](const std::string& file) {
				if (file.find(".vs") != std::string::npos) {
					vsFiles.push_back(file);
				} else if (file.find(".fs") != std::string::npos) {
					fsFiles.push_back(file);
				} else if (file.find(".gs") != std::string::npos) {
					gsFiles.push_back(file);
				} else {
					SPRaise("Failed to determine the type of a shader: %s", file.c_str());
				}
			};

			for (const auto& line : lines) {
				std::string t = TrimSpaces(line);
				if (t.empty())
					continue;
				if (t[0] == '#')
					continue;

				if (t == "*shadow*") {
					for (const auto& f : GLShadowShader::GetShaderNames(settings, false))
						addFile(f);
				} else if (t == "*shadow-lite*") {
					for (const auto& f : GLShadowShader::GetShaderNames(settings, false, true))
						addFile(f);
				} else if (t == "*shadow-variance*") {
					for (const auto& f : GLShadowShader::GetShaderNames(settings, true))
						addFile(f);
				} else if (t == "*dlight*") {
					for (const auto& f : GLDynamicLightShader::GetShaderNames())
						addFile(f);
				} else if (t == "*shadowmap*") {
					for (const auto& f : GLShadowMapShader::GetShaderNames())
						addFile(f);
				} else if (t[0] == '*') {
					SPRaise("Unknown special shader: %s", t.c_str());
				} else {
					addFile(t);
				}
			}

			auto p = stmp::make_unique<GLProgram>(&device, name);

			if (!vsFiles.empty()) {
				auto vs = CreateCombinedShader(GLShader::VertexShader, vsFiles);
				p->Attach(*vs);
				allocatedShaders.push_back(std::move(vs));
			}

			if (!fsFiles.empty()) {
				auto fs = CreateCombinedShader(GLShader::FragmentShader, fsFiles);
				p->Attach(*fs);
				allocatedShaders.push_back(std::move(fs));
			}

			if (!gsFiles.empty()) {
				auto gs = CreateCombinedShader(GLShader::GeometryShader, gsFiles);
				p->Attach(*gs);
				allocatedShaders.push_back(std::move(gs));
			}

			Stopwatch sw;
			p->Link();
			SPLog("Successfully linked GLSL program '%s' in %.3fms",
				name.c_str(), sw.GetTime() * 1000.0);
			return p;
		}

		std::unique_ptr<GLShader> GLProgramManager::CreateCombinedShader(
			GLShader::Type type, const std::vector<std::string>& fileNames) {
			SPADES_MARK_FUNCTION();

			if (fileNames.empty())
				return nullptr;

			auto s = stmp::make_unique<GLShader>(device, type);

			std::string extensions;
			std::string combinedBody;
			std::unordered_set<std::string> declaredSymbols;

			auto extractDeclName = [](const std::string& line, std::string& outName) -> bool {
				size_t start = line.find_first_not_of(" \t\r\n");
				if (start == std::string::npos) return false;
				if (line.compare(start, 8, "uniform ") != 0 && line.compare(start, 8, "uniform\t") != 0 &&
				    line.compare(start, 8, "varying ") != 0 && line.compare(start, 8, "varying\t") != 0 &&
				    line.compare(start, 10, "attribute ") != 0 && line.compare(start, 10, "attribute\t") != 0) {
					return false;
				}
				size_t semi = line.find(';', start);
				if (semi == std::string::npos) return false;
				std::string decl = line.substr(start, semi - start);
				size_t bracket = decl.find('[');
				if (bracket != std::string::npos) {
					decl = decl.substr(0, bracket);
				}
				size_t lastChar = decl.find_last_not_of(" \t\r\n");
				if (lastChar == std::string::npos) return false;
				size_t idStart = decl.find_last_of(" \t*&", lastChar);
				if (idStart == std::string::npos) {
					outName = decl.substr(0, lastChar + 1);
				} else {
					outName = decl.substr(idStart + 1, lastChar - idStart);
				}
				return !outName.empty();
			};

			for (const auto& fname : fileNames) {
				SPLog("Loading GLSL shader component '%s'", fname.c_str());
				std::string content = FileManager::ReadAllBytes(fname.c_str());
				std::istringstream stream(content);
				std::string line;
				while (std::getline(stream, line)) {
					size_t first = line.find_first_not_of(" \t\r");
					if (first != std::string::npos && line.compare(first, 10, "#extension") == 0) {
						if (extensions.find(line) == std::string::npos) {
							extensions += line + "\n";
						}
					} else {
						std::string declName;
						if (extractDeclName(line, declName)) {
							if (declaredSymbols.find(declName) != declaredSymbols.end()) {
								continue;
							}
							declaredSymbols.insert(declName);
						}
						combinedBody += line + "\n";
					}
				}
				combinedBody += "\n";
			}

#if defined(__ANDROID__) || defined(ANDROID) || defined(ZEROSPADES_MOBILE)
			if (type == GLShader::FragmentShader) {
				if (extensions.find("GL_EXT_shadow_samplers") == std::string::npos) {
					extensions += "#extension GL_EXT_shadow_samplers : enable\n";
				}
				if (extensions.find("GL_OES_standard_derivatives") == std::string::npos) {
					extensions += "#extension GL_OES_standard_derivatives : enable\n";
				}
			}
#endif

			std::string finalSource = extensions;

			if (settings.r_hdr) {
				finalSource += "#define USE_HDR 1\n";
				finalSource += "#define LINEAR_FRAMEBUFFER 1\n";
			} else {
				finalSource += "#define USE_HDR 0\n";
				finalSource += "#define LINEAR_FRAMEBUFFER 0\n";
			}

			if (settings.r_fogShadow)
				finalSource += "#define USE_VOLUMETRIC_FOG 1\n";
			else
				finalSource += "#define USE_VOLUMETRIC_FOG 0\n";

			if (settings.r_ssao)
				finalSource += "#define USE_SSAO 1\n";
			else
				finalSource += "#define USE_SSAO 0\n";

			if (settings.r_radiosity.operator int() >= 2)
				finalSource += "#define USE_RADIOSITY 2\n";
			else if (settings.r_radiosity.operator int() >= 1)
				finalSource += "#define USE_RADIOSITY 1\n";
			else
				finalSource += "#define USE_RADIOSITY 0\n";

#if defined(__ANDROID__) || defined(ANDROID) || defined(ZEROSPADES_MOBILE)
			if (type == GLShader::FragmentShader) {
				finalSource += "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
				               "precision highp float;\n"
				               "#else\n"
				               "precision mediump float;\n"
				               "#endif\n"
				               "precision mediump int;\n"
				               "precision mediump sampler2D;\n"
				               "#ifdef GL_EXT_shadow_samplers\n"
				               "precision mediump sampler2DShadow;\n"
				               "#define shadow2D shadow2DEXT\n"
				               "#endif\n";
			} else if (type == GLShader::VertexShader) {
				finalSource += "precision highp float;\n"
				               "precision mediump int;\n";
			}
#endif

			finalSource += combinedBody;
			s->AddSource(finalSource);

			Stopwatch sw;
			s->Compile();
			SPLog("Successfully compiled combined GLSL stage (%d components) in %.3fms",
				(int)fileNames.size(), sw.GetTime() * 1000.0);
			return s;
		}

		std::unique_ptr<GLShader> GLProgramManager::CreateShader(const std::string& name) {
			SPADES_MARK_FUNCTION();

			GLShader::Type type;
			if (name.find(".fs") != std::string::npos)
				type = GLShader::FragmentShader;
			else if (name.find(".vs") != std::string::npos)
				type = GLShader::VertexShader;
			else if (name.find(".gs") != std::string::npos)
				type = GLShader::GeometryShader;
			else
				SPRaise("Failed to determine the type of a shader: %s", name.c_str());

			return CreateCombinedShader(type, {name});
		}
	} // namespace draw
} // namespace spades