/*
gl_export.cpp - OpenGL dynamically linkage
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#define EXTERN
#include "gl_export.h"
#include "cl_util.h"
#include "gl_cvars.h"
#include "gl_local.h"
#include "imgui_manager.h"

#include "build.h"

extern render_api_t gRenderfuncs;
extern bool g_fRenderInitialized;

glConfig_t	glConfig;

#ifdef XASH_WIN32
#define strnicmp _strnicmp
#else
#define strnicmp strncasecmp
#endif


static dllfunc_t opengl_110funcs[] =
{
    { "glClearColor"         	, (void **)&glClearColor },
    { "glClear"              	, (void **)&glClear },
    { "glAlphaFunc"          	, (void **)&glAlphaFunc },
    { "glBlendFunc"          	, (void **)&glBlendFunc },
    { "glCullFace"           	, (void **)&glCullFace },
    { "glDrawBuffer"         	, (void **)&glDrawBuffer },
    { "glReadBuffer"         	, (void **)&glReadBuffer },
    { "glAccum"         		, (void **)&glAccum },
    { "glEnable"             	, (void **)&glEnable },
    { "glDisable"            	, (void **)&glDisable },
    { "glEnableClientState"  	, (void **)&glEnableClientState },
    { "glDisableClientState" 	, (void **)&glDisableClientState },
    { "glGetBooleanv"        	, (void **)&glGetBooleanv },
    { "glGetDoublev"         	, (void **)&glGetDoublev },
    { "glGetFloatv"          	, (void **)&glGetFloatv },
    { "glGetIntegerv"        	, (void **)&glGetIntegerv },
    { "glGetError"           	, (void **)&glGetError },
    { "glGetString"          	, (void **)&glGetString },
    { "glFinish"             	, (void **)&glFinish },
    { "glFlush"              	, (void **)&glFlush },
    { "glClearDepth"         	, (void **)&glClearDepth },
    { "glDepthFunc"          	, (void **)&glDepthFunc },
    { "glDepthMask"          	, (void **)&glDepthMask },
    { "glDepthRange"         	, (void **)&glDepthRange },
    { "glFrontFace"          	, (void **)&glFrontFace },
    { "glDrawElements"       	, (void **)&glDrawElements },
    { "glDrawArrays"       		, (void **)&glDrawArrays },
    { "glColorMask"          	, (void **)&glColorMask },
    { "glIndexPointer"       	, (void **)&glIndexPointer },
    { "glVertexPointer"      	, (void **)&glVertexPointer },
    { "glNormalPointer"      	, (void **)&glNormalPointer },
    { "glColorPointer"       	, (void **)&glColorPointer },
    { "glTexCoordPointer"    	, (void **)&glTexCoordPointer },
    { "glArrayElement"       	, (void **)&glArrayElement },
    { "glColor3f"            	, (void **)&glColor3f },
    { "glColor3fv"           	, (void **)&glColor3fv },
    { "glColor4f"            	, (void **)&glColor4f },
    { "glColor4fv"           	, (void **)&glColor4fv },
    { "glColor3ub"           	, (void **)&glColor3ub },
    { "glColor4ub"           	, (void **)&glColor4ub },
    { "glColor4ubv"          	, (void **)&glColor4ubv },
    { "glTexCoord1f"         	, (void **)&glTexCoord1f },
    { "glTexCoord2f"         	, (void **)&glTexCoord2f },
    { "glTexCoord3f"         	, (void **)&glTexCoord3f },
    { "glTexCoord4f"         	, (void **)&glTexCoord4f },
    { "glTexCoord1fv"        	, (void **)&glTexCoord1fv },
    { "glTexCoord2fv"        	, (void **)&glTexCoord2fv },
    { "glTexCoord3fv"        	, (void **)&glTexCoord3fv },
    { "glTexCoord4fv"        	, (void **)&glTexCoord4fv },
    { "glTexGenf"            	, (void **)&glTexGenf },
    { "glTexGenfv"           	, (void **)&glTexGenfv },
    { "glTexGeni"            	, (void **)&glTexGeni },
    { "glVertex2f"           	, (void **)&glVertex2f },
    { "glVertex3f"           	, (void **)&glVertex3f },
    { "glVertex3fv"          	, (void **)&glVertex3fv },
    { "glNormal3f"           	, (void **)&glNormal3f },
    { "glNormal3fv"          	, (void **)&glNormal3fv },
    { "glBegin"              	, (void **)&glBegin },
    { "glEnd"                	, (void **)&glEnd },
    { "glLineWidth"          	, (void**)&glLineWidth },
    { "glPointSize"          	, (void**)&glPointSize },
    { "glMatrixMode"         	, (void **)&glMatrixMode },
    { "glOrtho"              	, (void **)&glOrtho },
    { "glRasterPos2f"        	, (void **) &glRasterPos2f },
    { "glFrustum"            	, (void **)&glFrustum },
    { "glViewport"           	, (void **)&glViewport },
    { "glPushMatrix"         	, (void **)&glPushMatrix },
    { "glPopMatrix"          	, (void **)&glPopMatrix },
    { "glPushAttrib"         	, (void **)&glPushAttrib },
    { "glPopAttrib"          	, (void **)&glPopAttrib },
    { "glLoadIdentity"       	, (void **)&glLoadIdentity },
    { "glLoadMatrixd"        	, (void **)&glLoadMatrixd },
    { "glLoadMatrixf"        	, (void **)&glLoadMatrixf },
    { "glMultMatrixd"        	, (void **)&glMultMatrixd },
    { "glMultMatrixf"        	, (void **)&glMultMatrixf },
    { "glRotated"            	, (void **)&glRotated },
    { "glRotatef"            	, (void **)&glRotatef },
    { "glScaled"             	, (void **)&glScaled },
    { "glScalef"             	, (void **)&glScalef },
    { "glTranslated"         	, (void **)&glTranslated },
    { "glTranslatef"         	, (void **)&glTranslatef },
    { "glReadPixels"         	, (void **)&glReadPixels },
    { "glDrawPixels"         	, (void **)&glDrawPixels },
    { "glStencilFunc"        	, (void **)&glStencilFunc },
    { "glStencilMask"        	, (void **)&glStencilMask },
    { "glStencilOp"          	, (void **)&glStencilOp },
    { "glClearStencil"       	, (void **)&glClearStencil },
    { "glIsEnabled"          	, (void **)&glIsEnabled },
    { "glIsList"             	, (void **)&glIsList },
    { "glIsTexture"          	, (void **)&glIsTexture },
    { "glTexEnvf"            	, (void **)&glTexEnvf },
    { "glTexEnvfv"           	, (void **)&glTexEnvfv },
    { "glTexEnvi"            	, (void **)&glTexEnvi },
    { "glTexParameterf"      	, (void **)&glTexParameterf },
    { "glTexParameterfv"     	, (void **)&glTexParameterfv },
    { "glTexParameteri"      	, (void **)&glTexParameteri },
    { "glHint"               	, (void **)&glHint },
    { "glPixelStoref"        	, (void **)&glPixelStoref },
    { "glPixelStorei"        	, (void **)&glPixelStorei },
    { "glGenTextures"        	, (void **)&glGenTextures },
    { "glDeleteTextures"     	, (void **)&glDeleteTextures },
    { "glBindTexture"        	, (void **)&glBindTexture },
    { "glTexImage1D"         	, (void **)&glTexImage1D },
    { "glTexImage2D"         	, (void **)&glTexImage2D },
    { "glTexSubImage1D"      	, (void **)&glTexSubImage1D },
    { "glTexSubImage2D"      	, (void **)&glTexSubImage2D },
    { "glCopyTexImage1D"     	, (void **)&glCopyTexImage1D },
    { "glCopyTexImage2D"     	, (void **)&glCopyTexImage2D },
    { "glCopyTexSubImage1D"  	, (void **)&glCopyTexSubImage1D },
    { "glCopyTexSubImage2D"  	, (void **)&glCopyTexSubImage2D },
    { "glScissor"            	, (void **)&glScissor },
    { "glGetTexImage"			, (void **)&glGetTexImage },
    { "glGetTexEnviv"        	, (void **)&glGetTexEnviv },
    { "glPolygonOffset"      	, (void **)&glPolygonOffset },
    { "glPolygonMode"        	, (void **)&glPolygonMode },
    { "glPolygonStipple"     	, (void **)&glPolygonStipple },
    { "glClipPlane"          	, (void **)&glClipPlane },
    { "glGetClipPlane"       	, (void **)&glGetClipPlane },
    { "glShadeModel"         	, (void **)&glShadeModel },
    { "glGetTexLevelParameteriv"	, (void **)&glGetTexLevelParameteriv },
    { "glGetTexLevelParameterfv"	, (void **)&glGetTexLevelParameterfv },
    { "glFogfv"              	, (void **)&glFogfv },
    { "glFogf"               	, (void **)&glFogf },
    { "glFogi"               	, (void **)&glFogi },
    { NULL, NULL }
};

static dllfunc_t opengl_200funcs[] =
{
    { "glCreateShader"				, (void **)&glCreateShader },
    { "glAttachShader"				, (void **)&glAttachShader },
    { "glDetachShader"				, (void **)&glDetachShader },
    { "glCompileShader"				, (void **)&glCompileShader },
    { "glShaderSource"				, (void **)&glShaderSource },
    { "glGetShaderSource"			, (void **)&glGetShaderSource },
    { "glGetShaderiv"				, (void **)&glGetShaderiv },
    { "glDeleteShader"		        , (void **)&glDeleteShader },
    { "glUseProgram"				, (void **)&glUseProgram },
    { "glLinkProgram"				, (void **)&glLinkProgram },
    { "glValidateProgram"			, (void **)&glValidateProgram },
    { "glCreateProgram"				, (void **)&glCreateProgram },
    { "glDeleteProgram"				, (void **)&glDeleteProgram },
    { "glGetShaderInfoLog"			, (void **)&glGetShaderInfoLog },
    { "glGetProgramInfoLog"			, (void **)&glGetProgramInfoLog },
    { "glGetActiveUniform"			, (void **)&glGetActiveUniform },
    { "glGetUniformLocation"		, (void **)&glGetUniformLocation },
    { "glGetProgramiv"              , (void **)&glGetProgramiv },
    { "glBlendEquation"             , (void **)&glBlendEquation },
    { "glVertexAttribPointer"		, (void **)&glVertexAttribPointerARB },
    { "glEnableVertexAttribArray"	, (void **)&glEnableVertexAttribArrayARB },
    { "glDisableVertexAttribArray"	, (void **)&glDisableVertexAttribArrayARB },
    { "glBindAttribLocation"		, (void **)&glBindAttribLocationARB },
    { "glGetActiveAttrib"			, (void **)&glGetActiveAttribARB },
    { "glGetAttribLocation"			, (void **)&glGetAttribLocationARB },
    { "glTexImage3D"				, (void **)&glTexImage3D },
    { "glTexSubImage3D"				, (void **)&glTexSubImage3D },
    { "glCopyTexSubImage3D"			, (void **)&glCopyTexSubImage3D },
    { "glDrawRangeElements"			, (void **)&glDrawRangeElements },
    { "glDrawBuffers"				, (void **)&glDrawBuffersARB },
    { "glMultiTexCoord1f"			, (void **)&glMultiTexCoord1f },
    { "glMultiTexCoord2f"			, (void **)&glMultiTexCoord2f },
    { "glMultiTexCoord3f"			, (void **)&glMultiTexCoord3f },
    { "glMultiTexCoord4f"			, (void **)&glMultiTexCoord4f },
    { "glActiveTexture"				, (void **)&glActiveTexture },
    { "glUniformMatrix2fv"			, (void **)&glUniformMatrix2fvARB },
    { "glUniformMatrix3fv"			, (void **)&glUniformMatrix3fvARB },
    { "glUniformMatrix4fv"			, (void **)&glUniformMatrix4fvARB },
    { "glUniform1f"					, (void **)&glUniform1fARB },
    { "glUniform2f"					, (void **)&glUniform2fARB },
    { "glUniform3f"					, (void **)&glUniform3fARB },
    { "glUniform4f"					, (void **)&glUniform4fARB },
    { "glUniform1i"					, (void **)&glUniform1iARB },
    { "glUniform2i"					, (void **)&glUniform2iARB },
    { "glUniform3i"					, (void **)&glUniform3iARB },
    { "glUniform4i"					, (void **)&glUniform4iARB },
    { "glUniform1fv"				, (void **)&glUniform1fvARB },
    { "glUniform2fv"				, (void **)&glUniform2fvARB },
    { "glUniform3fv"				, (void **)&glUniform3fvARB },
    { "glUniform4fv"				, (void **)&glUniform4fvARB },
    { "glUniform1iv"				, (void **)&glUniform1ivARB },
    { "glUniform2iv"				, (void **)&glUniform2ivARB },
    { "glUniform3iv"				, (void **)&glUniform3ivARB },
    { "glUniform4iv"				, (void **)&glUniform4ivARB },
    { "glVertexAttrib2f"            , (void **)&glVertexAttrib2fARB },
    { "glVertexAttrib2fv"           , (void **)&glVertexAttrib2fvARB },
    { "glVertexAttrib3fv"           , (void **)&glVertexAttrib3fvARB },
    { "glVertexAttrib4fv"           , (void **)&glVertexAttrib4fvARB },
    { "glVertexAttrib4ubv"			, (void **)&glVertexAttrib4ubvARB },
    { NULL, NULL }
};

static dllfunc_t drawrangeelementsextfuncs[] =
{
    { "glDrawRangeElementsEXT" , (void **)&glDrawRangeElements },
    { NULL, NULL }
};

static dllfunc_t debugoutputfuncs[] =
{
    { "glDebugMessageControlARB" , (void **)&glDebugMessageControlARB },
    { "glDebugMessageInsertARB" , (void **)&glDebugMessageInsertARB },
    { "glDebugMessageCallbackARB" , (void **)&glDebugMessageCallbackARB },
    { "glGetDebugMessageLogARB" , (void **)&glGetDebugMessageLogARB },
    { NULL, NULL }
};

static dllfunc_t khr_debug_funcs[] =
{
    { "glGetObjectLabel"	, (void **)&glGetObjectLabel },
    { "glGetObjectPtrLabel"	, (void **)&glGetObjectPtrLabel },
    { "glObjectLabel"		, (void **)&glObjectLabel },
    { "glObjectPtrLabel"	, (void **)&glObjectPtrLabel },
    { "glPopDebugGroup"		, (void **)&glPopDebugGroup },
    { "glPushDebugGroup"	, (void **)&glPushDebugGroup },
    { NULL, NULL }
};

static dllfunc_t blendseparatefunc[] =
{
    { "glBlendFuncSeparateEXT", (void **)&glBlendFuncSeparate },
    { NULL, NULL }
};

static dllfunc_t shaderobjectsfuncs[] =
{
    { "glGetInfoLogARB"               , (void **)&glGetInfoLogARB },
    { "glGetAttachedObjectsARB"       , (void **)&glGetAttachedObjectsARB },
    { "glGetUniformfvARB"             , (void **)&glGetUniformfvARB },
    { "glGetUniformivARB"             , (void **)&glGetUniformivARB },
    { NULL, NULL }
};

static dllfunc_t binaryshaderfuncs[] =
{
    { "glProgramBinary"              , (void **)&glProgramBinary },
    { "glGetProgramBinary"           , (void **)&glGetProgramBinary },
    { "glProgramParameteri"          , (void **)&glProgramParameteri },
    { NULL, NULL }
};

static dllfunc_t vbofuncs[] =
{
    { "glBindBufferARB"    , (void **)&glBindBufferARB },
    { "glDeleteBuffersARB" , (void **)&glDeleteBuffersARB },
    { "glGenBuffersARB"    , (void **)&glGenBuffersARB },
    { "glIsBufferARB"      , (void **)&glIsBufferARB },
    { "glMapBufferARB"     , (void **)&glMapBufferARB },
    { "glUnmapBufferARB"   , (void **)&glUnmapBufferARB },
    { "glBufferDataARB"    , (void **)&glBufferDataARB },
    { "glBufferSubDataARB" , (void **)&glBufferSubDataARB },
    { NULL, NULL }
};

static dllfunc_t vaofuncs[] =
{
    { "glBindVertexArray"    , (void **)&glBindVertexArray },
    { "glDeleteVertexArrays" , (void **)&glDeleteVertexArrays },
    { "glGenVertexArrays"    , (void **)&glGenVertexArrays },
    { "glIsVertexArray"      , (void **)&glIsVertexArray },
    { NULL, NULL }
};

static dllfunc_t fbofuncs[] =
{
    { "glIsRenderbuffer"                      , (void **)&glIsRenderbuffer },
    { "glBindRenderbuffer"                    , (void **)&glBindRenderbuffer },
    { "glDeleteRenderbuffers"                 , (void **)&glDeleteRenderbuffers },
    { "glGenRenderbuffers"                    , (void **)&glGenRenderbuffers },
    { "glRenderbufferStorage"                 , (void **)&glRenderbufferStorage },
    { "glGetRenderbufferParameteriv"          , (void **)&glGetRenderbufferParameteriv },
    { "glIsFramebuffer"                       , (void **)&glIsFramebuffer },
    { "glBindFramebuffer"                     , (void **)&glBindFramebuffer },
    { "glBlitFramebuffer"                     , (void **)&glBlitFramebuffer },
    { "glDeleteFramebuffers"                  , (void **)&glDeleteFramebuffers },
    { "glGenFramebuffers"                     , (void **)&glGenFramebuffers },
    { "glCheckFramebufferStatus"              , (void **)&glCheckFramebufferStatus },
    { "glFramebufferTexture1D"                , (void **)&glFramebufferTexture1D },
    { "glFramebufferTexture2D"                , (void **)&glFramebufferTexture2D },
    { "glFramebufferTexture3D"                , (void **)&glFramebufferTexture3D },
    { "glFramebufferRenderbuffer"             , (void **)&glFramebufferRenderbuffer },
    { "glGetFramebufferAttachmentParameteriv" , (void **)&glGetFramebufferAttachmentParameteriv },
    { "glGenerateMipmap"                      , (void **)&glGenerateMipmap },
    { "glColorMaski"                          , (void **)&glColorMaski },
    { NULL, NULL }
};

static dllfunc_t occlusionfunc[] =
{
    { "glGenQueriesARB"        , (void **)&glGenQueriesARB },
    { "glDeleteQueriesARB"     , (void **)&glDeleteQueriesARB },
    { "glIsQueryARB"           , (void **)&glIsQueryARB },
    { "glBeginQueryARB"        , (void **)&glBeginQueryARB },
    { "glEndQueryARB"          , (void **)&glEndQueryARB },
    { "glGetQueryivARB"        , (void **)&glGetQueryivARB },
    { "glGetQueryObjectivARB"  , (void **)&glGetQueryObjectivARB },
    { "glGetQueryObjectuivARB" , (void **)&glGetQueryObjectuivARB },
    { NULL, NULL }
};

static dllfunc_t nv_dither_control_func[] =
{
    { "glAlphaToCoverageDitherControlNV", (void **)&glAlphaToCoverageDitherControlNV },
    { NULL, NULL }
};

/*
=================
GL_SetExtension
=================
*/
void GL_SetExtension(int r_ext, int enable)
{
	if(r_ext >= 0 && r_ext < R_EXTCOUNT)
		glConfig.extension[r_ext] = enable ? GL_TRUE : GL_FALSE;
    else gEngfuncs.Con_Printf("GL_SetExtension: invalid extension %d\n", r_ext);
}

/*
=================
GL_Support
=================
*/
bool GL_Support(int r_ext)
{
	if(r_ext >= 0 && r_ext < R_EXTCOUNT)
		return glConfig.extension[r_ext] ? true : false;
	gEngfuncs.Con_Printf("GL_Support: invalid extension %d\n", r_ext);
	return false;		
}

bool GL_SupportExtension(const char *name)
{
	if (name && name[0] && !strstr(glConfig.extensions_string, name))
		return true;
	else
		return false;
}

/*
=================
GL_CheckExtension
=================
*/
static void GL_CheckExtension( const char *name, const dllfunc_t *funcs, const char *cvarname, int r_ext, bool cvar_from_engine = false )
{
	const dllfunc_t	*func;
	cvar_t		*parm;

	gEngfuncs.Con_Printf("GL_CheckExtension: %s ", name);

	// ugly hack for p1 opengl32.dll
	if((name[0] == 'P' || name[2] == '_' || name[3] == '_') && !strstr(glConfig.extensions_string, name))
	{
		GL_SetExtension( r_ext, false );	// update render info
		gEngfuncs.Con_Printf( "- ^1failed\n" );
		return;
	}

	if( cvarname )
	{
		// NOTE: engine will be ignore cvar value if variable already exitsts (e.g. created on exec opengl.cfg)
		// so this call just update variable description (because Host_WriteOpenGLConfig won't archive cvars without it)
		if( cvar_from_engine ) parm = CVAR_GET_POINTER( cvarname );
		else parm = gEngfuncs.pfnRegisterVariable( (char *)cvarname, "1", FCVAR_GLCONFIG );

		if( !CVAR_TO_BOOL( parm ) || ( !CVAR_TO_BOOL( gl_extensions ) && r_ext != R_OPENGL_110 ))
		{
			gEngfuncs.Con_Printf( "- ^3disabled\n" );
			GL_SetExtension( r_ext, false );
			return; // nothing to process at
		}
		GL_SetExtension( r_ext, true );
	}

	// clear exports
	for( func = funcs; func && func->name; func++ )
		*func->func = NULL;

	GL_SetExtension( r_ext, true ); // predict extension state
	for( func = funcs; func && func->name != NULL; func++ )
	{
		// functions are cleared before all the extensions are evaluated
		if(!(*func->func = (void *)gRenderfuncs.GL_GetProcAddress(func->name)))
			GL_SetExtension( r_ext, false ); // one or more functions are invalid, extension will be disabled
	}

	if( GL_Support( r_ext ))
		gEngfuncs.Con_Printf( "- ^2enabled\n" );
	else 
		gEngfuncs.Con_Printf( "- ^1failed\n" );
}

static void GL_InitExtensions( void )
{
	// initialize gl extensions
	GL_CheckExtension("OpenGL 1.1.0", opengl_110funcs, NULL, R_OPENGL_110);
	GL_CheckExtension("OpenGL 2.0", opengl_200funcs, NULL, R_OPENGL_200);

	if (!GL_Support(R_OPENGL_110))
	{
		gEngfuncs.Con_Printf( "OpenGL 1.0 can't be installed. Custom renderer disabled\n" );
		// TODO this is used in P2 but not here 
		// g_fRenderInterfaceValid = false;
		g_fRenderInitialized = false;
		return;
	}
}

/*
 * ==================
 * GL_Init
 * ==================
 */
bool GL_Init(void)
{
	GL_InitExtensions();

    if( !g_fRenderInitialized )
	{
		GL_Shutdown();
		return false;
	}

    g_ImGuiManager.Initialize();

    return true;
}

/*
 * ==================
 * GL_Shutdown
 * ==================
 */
void GL_Shutdown(void)
{
    g_ImGuiManager.Terminate();
    
    // now all extensions are disabled
	memset( glConfig.extension, 0, sizeof( glConfig.extension[0] ) * R_EXTCOUNT );
}
