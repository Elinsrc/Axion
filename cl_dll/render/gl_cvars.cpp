/*
r_cvars.cpp - renderer console variables list
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "gl_cvars.h"
#include "cl_dll.h"

cvar_t *r_test;
cvar_t *r_drawentities;
cvar_t *gl_extensions;
cvar_t *cv_crosshair;
cvar_t *r_detailtextures;
cvar_t *r_lighting_ambient;
cvar_t *r_lightstyle_lerping;
cvar_t *r_lighting_extended;
cvar_t *r_draw_beams;
cvar_t *r_overview;
cvar_t *r_novis;
cvar_t *r_nocull;
cvar_t *r_lockpvs;
cvar_t *r_lockfrustum;
cvar_t *r_pvs_radius;
cvar_t *cv_nosort;
cvar_t *r_lightmap;
cvar_t *r_speeds;
cvar_t *r_decals;
cvar_t *r_wireframe;
cvar_t *r_polyoffset;
cvar_t *r_dynamic;
cvar_t *r_finish;
cvar_t *r_clear;
cvar_t *r_fullbright;
cvar_t *cv_gamma;
cvar_t *cv_brightness;

void R_InitializeConVars()
{
	// setup some engine cvars for custom rendering
	r_test = gEngfuncs.pfnGetCvarPointer("gl_test");
	cv_nosort = gEngfuncs.pfnGetCvarPointer("gl_nosort");
	r_overview = gEngfuncs.pfnGetCvarPointer("dev_overview");
	r_fullbright = gEngfuncs.pfnGetCvarPointer("r_fullbright");
	r_drawentities = gEngfuncs.pfnGetCvarPointer("r_drawentities");
	gl_extensions = gEngfuncs.pfnGetCvarPointer("gl_allow_extensions");
	r_finish = gEngfuncs.pfnGetCvarPointer("gl_finish");
	cv_crosshair = gEngfuncs.pfnGetCvarPointer("crosshair");
	r_lighting_ambient = gEngfuncs.pfnGetCvarPointer("r_lighting_ambient");
	r_lightstyle_lerping = gEngfuncs.pfnGetCvarPointer("cl_lightstyle_lerping");
	r_lighting_extended = gEngfuncs.pfnGetCvarPointer("r_lighting_extended");
	r_draw_beams = gEngfuncs.pfnGetCvarPointer("cl_draw_beams");
	r_detailtextures = gEngfuncs.pfnGetCvarPointer("r_detailtextures");
	r_speeds = gEngfuncs.pfnGetCvarPointer("r_speeds");
	r_novis = gEngfuncs.pfnGetCvarPointer("r_novis");
	r_nocull = gEngfuncs.pfnGetCvarPointer("r_nocull");
	r_lockpvs = gEngfuncs.pfnGetCvarPointer("r_lockpvs");
	r_lockfrustum = gEngfuncs.pfnGetCvarPointer("r_lockfrustum");
	r_pvs_radius = gEngfuncs.pfnGetCvarPointer("r_pvs_radius");
	r_wireframe = gEngfuncs.pfnGetCvarPointer("gl_wireframe");
	r_lightmap = gEngfuncs.pfnGetCvarPointer("r_lightmap");
	r_decals = gEngfuncs.pfnGetCvarPointer("r_decals");
	r_clear = gEngfuncs.pfnGetCvarPointer("gl_clear");
	r_dynamic = gEngfuncs.pfnGetCvarPointer("r_dynamic");
	cv_gamma = gEngfuncs.pfnGetCvarPointer("gamma");
	cv_brightness = gEngfuncs.pfnGetCvarPointer("brightness");
	r_polyoffset = gEngfuncs.pfnGetCvarPointer("gl_polyoffset");
}