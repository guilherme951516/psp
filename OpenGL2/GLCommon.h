#pragma once

#include "ppsspp_config.h"

#ifdef __EMSCRIPTEN__
// WebGL 2 via Emscripten
#include <GLES3/gl3.h>
#define USING_GLES2
#define GL_MIN_EXT GL_MIN
#define GL_MAX_EXT GL_MAX
#elif PPSSPP_PLATFORM(IOS)
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#elif defined(USING_GLES2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define GL_BGRA_EXT 0x80E1
#else
#include "GL/glew.h"
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif

#if defined(USING_GLES2) && !defined(__EMSCRIPTEN__)
#include "Common/GPU/OpenGL/gl3stub.h"
#endif

#ifdef USING_GLES2

#ifndef GL_MIN_EXT
#define GL_MIN_EXT 0x8007
#endif

#ifndef GL_MAX_EXT
#define GL_MAX_EXT 0x8008
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER GL_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER GL_FRAMEBUFFER
#endif

#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 GL_DEPTH_COMPONENT24_OES
#endif

#ifndef GL_RGBA8
#define GL_RGBA8 GL_RGBA
#endif

#endif

#ifndef GL_DEPTH24_STENCIL8_OES
#define GL_DEPTH24_STENCIL8_OES 0x88F0
#endif