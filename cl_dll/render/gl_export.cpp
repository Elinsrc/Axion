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
#include "imgui_manager.h"

extern render_api_t gRenderAPI;

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

/*
 * ==================
 * GL_LoadFunctions
 * ==================
 */
static void GL_LoadFunctions(const dllfunc_t *funcs)
{
    for (const dllfunc_t *f = funcs; f->name; f++)
    {
        *f->func = (void *)gRenderAPI.GL_GetProcAddress(f->name);
        if (*f->func)
            gEngfuncs.Con_Printf("Loaded %s\n", f->name);
        else
            gEngfuncs.Con_Printf("Not load %s\n", f->name);
    }
}

/*
 * ==================
 * GL_Init
 * ==================
 */
bool GL_Init(void)
{
    gEngfuncs.Con_Printf("Loading OpenGL 1.1 functions...\n");
    GL_LoadFunctions(opengl_110funcs);

    gEngfuncs.Con_Printf("Loading OpenGL 2.0 functions...\n");
    GL_LoadFunctions(opengl_200funcs);

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
    gEngfuncs.Con_Printf("OpenGL functions unloaded.\n");
}
