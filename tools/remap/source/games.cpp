/* -------------------------------------------------------------------------------

   Copyright (C) 1999-2007 id Software, Inc. and contributors.
   For a list of contributors, see the accompanying CONTRIBUTORS file.

   This file is part of GtkRadiant.

   GtkRadiant is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   GtkRadiant is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GtkRadiant; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

   ----------------------------------------------------------------------------------

   This code has been altered significantly from its original form, to support
   several games based on the Quake III Arena engine, in the form of "Q3Map2."

   ------------------------------------------------------------------------------- */

#include "games.h"
#include "bspfile_titanfall.h"
#include "bspfile_titanfall2.h"
#include "bspfile_apex_legends.h"
#include "qstringops.h"
#include "inout.h"


struct game_titanfallonline : game_t
{
	/* most of this is copied over, just want to et vis tree injected first */
	game_titanfallonline() : game_t
	{
		"titanfallonline",          /* -game x */
		"assets",               /* default base game data dir */
		".assets",              /* unix home sub-dir */
		"soldier",              /* magic path word */
		"shaders",              /* shader directory */
		64,                     /* max lightmapped surface verts */
		999,                    /* max surface verts */
		6000,                   /* max surface indexes */
		true,                   /* flares */
		"gfx/misc/lens_flare",  /* default flare shader */
		false,                  /* wolf lighting model? */
		128,                    /* lightmap width/height */
		1.0f,                   /* lightmap gamma */
		false,                  /* lightmap sRGB */
		false,                  /* texture sRGB */
		false,                  /* color sRGB */
		0.0f,                   /* lightmap exposure */
		1.0f,                   /* lightmap compensate */
		1.0f,                   /* lightgrid scale */
		1.0f,                   /* lightgrid ambient scale */
		false,                  /* light angle attenuation uses half-lambert curve */
		false,                  /* disable shader lightstyles hack */
		false,                  /* keep light entities on bsp */
		8,                      /* default patchMeta subdivisions tolerance */
		false,                  /* patch casting enabled */
		false,                  /* compile deluxemaps */
		0,                      /* deluxemaps default mode */
		512,                    /* minimap size */
		1.0f,                   /* minimap sharpener */
		0.0f,                   /* minimap border */
		true,                   /* minimap keep aspect */
		EMiniMapMode::Gray,     /* minimap mode */
		"%s.tga",               /* minimap name format */
		"rBSP",                 /* bsp file prefix */
		29,                      /* bsp file version */
		false,                  /* cod-style lump len/ofs order */
		LoadR1BSPFile,           /* bsp load function */
		WriteR1BSPFile,          /* bsp write function */
		CompileR1BSPFile,
		{
			/* name             contentFlags                contentFlagsClear           surfaceFlags                surfaceFlagsClear           compileFlags                compileFlagsClear */

			/* default */
			{ "default",        0,                         -1,                          0x100000,                  -1,                          C_SOLID,                    -1 },

			/* compiler */
			{ "nodraw",         0,                          0,                          0,                          0,                          C_NODRAW,                   0 },

			// remap
			{ "sky2d",          0,                          0,                          S_SKY_2D,                   0,                          0,                          0 },
			{ "sky",            0,                          0,                          S_SKY,                      0,                          0,                          0 },
			{ "trans",          0,                          0,                          S_TRANSLUCENT,              0,                          0,                          0 },
			{ "unlit",          0,                          0,                          S_VERTEX_UNLIT,             0,                          0,                          0 },
			{ "litflat",        0,                          0,                          S_VERTEX_LIT_FLAT,          0,                          0,                          0 },
			{ "litbump",        0,                          0,                          S_VERTEX_LIT_BUMP,          0,                          0,                          0 },
			{ "unlitts",        0,                          0,                          S_VERTEX_UNLIT_TS,          0,                          0,                          0 },
		},
		0
	}{}
};

/* titanfall2 */
struct game_titanfall2 : game_t
{
	/* most of this is copied over, just want to et vis tree injected first */
	game_titanfall2() : game_t
	{
		"titanfall2",                 /* -game x */
		"assets",                 /* default base game data dir */
		".assets",                /* unix home sub-dir */
		"soldier",              /* magic path word */
		"shaders",              /* shader directory */
		64,                     /* max lightmapped surface verts */
		999,                    /* max surface verts */
		6000,                   /* max surface indexes */
		true,                   /* flares */
		"gfx/misc/lens_flare",  /* default flare shader */
		false,                  /* wolf lighting model? */
		128,                    /* lightmap width/height */
		1.0f,                   /* lightmap gamma */
		false,                  /* lightmap sRGB */
		false,                  /* texture sRGB */
		false,                  /* color sRGB */
		0.0f,                   /* lightmap exposure */
		1.0f,                   /* lightmap compensate */
		1.0f,                   /* lightgrid scale */
		1.0f,                   /* lightgrid ambient scale */
		false,                  /* light angle attenuation uses half-lambert curve */
		false,                  /* disable shader lightstyles hack */
		false,                  /* keep light entities on bsp */
		8,                      /* default patchMeta subdivisions tolerance */
		false,                  /* patch casting enabled */
		false,                  /* compile deluxemaps */
		0,                      /* deluxemaps default mode */
		512,                    /* minimap size */
		1.0f,                   /* minimap sharpener */
		0.0f,                   /* minimap border */
		true,                   /* minimap keep aspect */
		EMiniMapMode::Gray,     /* minimap mode */
		"%s.tga",               /* minimap name format */
		"rBSP",                 /* bsp file prefix */
		37,                      /* bsp file version */
		false,                  /* cod-style lump len/ofs order */
		LoadR2BSPFile,           /* bsp load function */
		WriteR2BSPFile,          /* bsp write function */
		CompileR2BSPFile,
		{
			/* name             contentFlags                contentFlagsClear           surfaceFlags                surfaceFlagsClear           compileFlags                compileFlagsClear */

			/* default */
			{ "default",        0,                         -1,                          0x100000,                  -1,                          C_SOLID,                    -1 },

			/* compiler */
			{ "nodraw",         0,                          0,                          0,                          0,                          C_NODRAW,                   0 },

			// remap
			{ "sky2d",          0,                          0,                          S_SKY_2D,                   0,                          0,                          0 },
			{ "sky",            0,                          0,                          S_SKY,                      0,                          0,                          0 },
			{ "trans",          0,                          0,                          S_TRANSLUCENT,              0,                          0,                          0 },
			{ "unlit",          0,                          0,                          S_VERTEX_UNLIT,             0,                          0,                          0 },
			{ "litflat",        0,                          0,                          S_VERTEX_LIT_FLAT,          0,                          0,                          0 },
			{ "litbump",        0,                          0,                          S_VERTEX_LIT_BUMP,          0,                          0,                          0 },
			{ "unlitts",        0,                          0,                          S_VERTEX_UNLIT_TS,          0,                          0,                          0 },
		},
		0
	}{}
};

/* apex legends */
struct game_apexlegends : game_t
{
	/* most of this is copied over, just want to et vis tree injected first */
	game_apexlegends() : game_t
	{
		"apexlegends",          /* -game x */
		"assets",               /* default base game data dir */
		".assets",              /* unix home sub-dir */
		"soldier",              /* magic path word */
		"shaders",              /* shader directory */
		64,                     /* max lightmapped surface verts */
		999,                    /* max surface verts */
		6000,                   /* max surface indexes */
		true,                   /* flares */
		"gfx/misc/lens_flare",  /* default flare shader */
		false,                  /* wolf lighting model? */
		128,                    /* lightmap width/height */
		1.0f,                   /* lightmap gamma */
		false,                  /* lightmap sRGB */
		false,                  /* texture sRGB */
		false,                  /* color sRGB */
		0.0f,                   /* lightmap exposure */
		1.0f,                   /* lightmap compensate */
		1.0f,                   /* lightgrid scale */
		1.0f,                   /* lightgrid ambient scale */
		false,                  /* light angle attenuation uses half-lambert curve */
		false,                  /* disable shader lightstyles hack */
		false,                  /* keep light entities on bsp */
		8,                      /* default patchMeta subdivisions tolerance */
		false,                  /* patch casting enabled */
		false,                  /* compile deluxemaps */
		0,                      /* deluxemaps default mode */
		512,                    /* minimap size */
		1.0f,                   /* minimap sharpener */
		0.0f,                   /* minimap border */
		true,                   /* minimap keep aspect */
		EMiniMapMode::Gray,     /* minimap mode */
		"%s.tga",               /* minimap name format */
		"rBSP",                 /* bsp file prefix */
		47,                      /* bsp file version */
		false,                  /* cod-style lump len/ofs order */
		LoadR5BSPFile,           /* bsp load function */
		WriteR5BSPFile,          /* bsp write function */
		CompileR5BSPFile,
		{
			/* name             contentFlags                contentFlagsClear           surfaceFlags                surfaceFlagsClear           compileFlags                compileFlagsClear */

			/* default */
			{ "default",        0,                         -1,                          0x100000,                  -1,                          C_SOLID,                    -1 },

			/* compiler */
			{ "nodraw",         0,                          0,                          0,                          0,                          C_NODRAW,                   0 },

			// remap
			{ "sky2d",          0,                          0,                          S_SKY_2D,                   0,                          0,                          0 },
			{ "sky",            0,                          0,                          S_SKY,                      0,                          0,                          0 },
			{ "trans",          0,                          0,                          S_TRANSLUCENT,              0,                          0,                          0 },
			{ "unlit",          0,                          0,                          S_VERTEX_UNLIT,             0,                          0,                          0 },
			{ "litflat",        0,                          0,                          S_VERTEX_LIT_FLAT,          0,                          0,                          0 },
			{ "litbump",        0,                          0,                          S_VERTEX_LIT_BUMP,          0,                          0,                          0 },
			{ "unlitts",        0,                          0,                          S_VERTEX_UNLIT_TS,          0,                          0,                          0 },
		},
		0
	}{}
};


const std::vector<game_t> g_games = { game_titanfallonline(),
                                      game_titanfall2(),
									  game_apexlegends()
									};
const game_t *g_game = &g_games[0];
