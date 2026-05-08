/*
r_cvars.h - renderer console variables list
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

#ifndef R_CVARS_H
#define R_CVARS_H
#include "cvardef.h"

void R_InitializeConVars();

extern cvar_t *r_test;	// just cvar for testify new effects
extern cvar_t *r_drawentities;
extern cvar_t *gl_extensions;
extern cvar_t *cv_crosshair;
extern cvar_t *r_detailtextures;
extern cvar_t *r_lighting_ambient;
extern cvar_t *r_lightstyle_lerping;
extern cvar_t *r_lighting_extended;
extern cvar_t *r_draw_beams;
extern cvar_t *r_overview;
extern cvar_t *r_novis;
extern cvar_t *r_nocull;
extern cvar_t *r_lockpvs;
extern cvar_t *r_lockfrustum;
extern cvar_t *r_pvs_radius;
extern cvar_t *cv_nosort;
extern cvar_t *r_lightmap;
extern cvar_t *r_speeds;
extern cvar_t *r_decals;
extern cvar_t *r_wireframe;
extern cvar_t *r_polyoffset;
extern cvar_t *r_dynamic;
extern cvar_t *r_finish;
extern cvar_t *r_clear;
extern cvar_t *r_fullbright;
extern cvar_t *cv_gamma;
extern cvar_t *cv_brightness;

#endif//R_CVARS_H