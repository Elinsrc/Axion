/*
gl_local.h - renderer local definitions
this code written for Paranoia 2: Savior modification
Copyright (C) 2013 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef GL_LOCAL_H
#define GL_LOCAL_H

#include "gl_export.h"

/*
=======================================================================

 GL STATE MACHINE

=======================================================================
*/
enum
{
	R_OPENGL_110 = 0,		// base
	R_OPENGL_200,
	R_WGL_PROCADDRESS,
	R_ARB_VERTEX_BUFFER_OBJECT_EXT,
	R_ARB_VERTEX_ARRAY_OBJECT_EXT,
	R_DRAW_BUFFERS_EXT,
	R_ARB_MULTITEXTURE,
	R_TEXTURECUBEMAP_EXT,
	R_SHADER_GLSL100_EXT,
	R_DRAW_RANGEELEMENTS_EXT,
	R_TEXTURE_3D_EXT,
	R_SHADER_OBJECTS_EXT,
	R_VERTEX_SHADER_EXT,	// glsl vertex program
	R_FRAGMENT_SHADER_EXT,	// glsl fragment program	
	R_ARB_TEXTURE_NPOT_EXT,
	R_TEXTURE_2D_RECT_EXT,
	R_DEPTH_TEXTURE,
	R_SHADOW_EXT,
	R_FRAMEBUFFER_OBJECT,
	R_SEPARATE_BLENDFUNC_EXT,
	R_OCCLUSION_QUERIES_EXT,
	R_SEAMLESS_CUBEMAP,
	R_BINARY_SHADER_EXT,
	R_PARANOIA_EXT,		// custom OpenGL32.dll with hacked function glDepthRange
	R_DEBUG_OUTPUT,
	R_KHR_DEBUG,
	R_ARB_PIXEL_BUFFER_OBJECT,
	R_A2C_DITHER_CONTROL,
	R_EXTCOUNT,		// must be last
};

typedef enum
{
	GLHW_GENERIC,		// where everthing works the way it should
	GLHW_RADEON,		// where you don't have proper GLSL support
	GLHW_NVIDIA		    // Geforce 8/9 class DX10 hardware
} glHWType_t;

typedef struct
{
	const char	*renderer_string;		// ptrs to OpenGL32.dll, use with caution
	const char	*version_string;
	const char	*vendor_string;

	glHWType_t	hardware_type;
	float		version;

	// list of supported extensions
	const char	*extensions_string;
	bool		extension[R_EXTCOUNT];
	bool		debug_context;
} glConfig_t;

extern glConfig_t glConfig;

typedef struct {
    const char *name;
    void **func;
} dllfunc_t;


//
// gl_export.cpp
//
bool GL_Init(void);
void GL_Shutdown(void);
bool GL_Support(int r_ext);
bool GL_SupportExtension(const char *name);

#endif//GL_LOCAL_H