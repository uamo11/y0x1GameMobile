//
//  OpenGL.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

// Supported platforms: macOS, Android (GLES3), and Desktop (GLEW)
#if __APPLE__
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#elif defined(__ANDROID__) || defined(ANDROID)
#define GL_GLEXT_PROTOTYPES 1
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>

#ifndef GL_ANY_SAMPLES_PASSED
#  define GL_ANY_SAMPLES_PASSED 0x8C2F
#endif

#ifndef GL_CLAMP
#  define GL_CLAMP GL_CLAMP_TO_EDGE
#endif
#ifndef GL_POLYGON
#  define GL_POLYGON GL_TRIANGLE_FAN
#endif
#define glClearDepth glClearDepthf
#define glDepthRange glDepthRangef
#else
#include <GL/glew.h>

// v3.3 / GL_ARB_occlusion_query2
#ifndef GL_ANY_SAMPLES_PASSED
#  define GL_ANY_SAMPLES_PASSED 0x8C2F
#endif

#endif
