/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "ui_local.h"

void SystemSettings_MenuInit( void );

/*
=======================================================================

SYSTEM SETTINGS MENU

=======================================================================
*/

#define SYSTEMSETTINGS_FRAMEL	"interface/art/frame2_l"
#define SYSTEMSETTINGS_FRAMER	"interface/art/frame1_r"
#define SYSTEMSETTINGS_BACK0	"interface/art/back_0"
#define SYSTEMSETTINGS_BACK1	"interface/art/back_1"
#define SYSTEMSETTINGS_ACCEPT0	"interface/art/accept_0"
#define SYSTEMSETTINGS_ACCEPT1	"interface/art/accept_1"

#define SYSTEM_X_POS		320
#define ID_CONTROLS			100
#define ID_SYSTEM			101
#define ID_GENERAL			102
#define ID_FULLSCREEN		103
#define ID_LIST				104
#define ID_MODE				105
#define ID_RATIO			106
#define ID_EFFECTSVOLUME	107
#define ID_MUSICVOLUME		108
#define ID_QUALITY			109
#define ID_BACK2			110

typedef struct {
	menuframework_s	menu;

	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		controls;
	menutext_s		system;
	menutext_s		general;

	menulist_s		list;
	menulist_s		ratio;
	menulist_s		mode;
	menulist_s		driver;
	menuslider_s	tq;
	menulist_s  	fs;
	menulist_s  	lighting;
	menulist_s  	allow_extensions;
	menulist_s  	texturebits;
	menulist_s  	colordepth;
	menulist_s  	geometry;
	menulist_s  	filter;
	menulist_s		multisample;
	menulist_s		anisotropy;
	menutext_s		driverinfo;

	menuslider_s	sfxvolume;
	menuslider_s	musicvolume;
	menulist_s		quality;	
	
	menubitmap_s	apply;
	menutext_s	back;
} systemsettings_t;

typedef struct
{
	int mode;
	qboolean fullscreen;
	int tq;
	int lighting;
	int colordepth;
	int texturebits;
	int geometry;
	int filter;
	int multisample;
	int anisotropy;
	int driver;
	qboolean extensions;
} InitialVideoOptions_s;

static InitialVideoOptions_s	s_ivo;
static systemsettings_t		s_systemsettings;

static InitialVideoOptions_s s_ivo_templates[] =
{
	{
		6, qtrue, 3, 0, 2, 2, 2, 1, 0, qtrue
	},
	{
		4, qtrue, 2, 0, 2, 2, 1, 1, 0, qtrue	// JDC: this was tq 3
	},
	{
		3, qtrue, 2, 0, 0, 0, 1, 0, 0, qtrue
	},
	{
		2, qtrue, 1, 0, 1, 0, 0, 0, 0, qtrue
	},
	{
		2, qtrue, 1, 1, 1, 0, 0, 0, 0, qtrue
	},
	{
		3, qtrue, 1, 0, 0, 0, 1, 0, 0, qtrue
	}
};

static const char *quality_items[] = {
	"Low", "High", NULL
};

#define NUM_IVO_TEMPLATES ( sizeof( s_ivo_templates ) / sizeof( s_ivo_templates[0] ) )

static const char *builtinResolutions[ ] =
{
	"320x240",
	"400x300",
	"512x384",
	"640x480",
	"800x600",
	"960x720",
	"1024x768",
	"1152x864",
	"1280x1024",
	"1600x1200",
	"2048x1536",
	"856x480",
	NULL
};

static const char *knownRatios[ ][2] =
{
	{ "1.25:1", "5:4"   },
	{ "1.33:1", "4:3"   },
	{ "1.50:1", "3:2"   },
	{ "1.56:1", "14:9"  },
	{ "1.60:1", "16:10" },
	{ "1.67:1", "5:3"   },
	{ "1.78:1", "16:9"  },
	{ NULL    , NULL    }
};

#define MAX_RESOLUTIONS	32

static const char* ratios[ MAX_RESOLUTIONS ];
static char ratioBuf[ MAX_RESOLUTIONS ][ 8 ];
static int ratioToRes[ MAX_RESOLUTIONS ];
static int resToRatio[ MAX_RESOLUTIONS ];

static char resbuf[ MAX_STRING_CHARS ];
static const char* detectedResolutions[ MAX_RESOLUTIONS ];

static const char** resolutions = builtinResolutions;
static qboolean resolutionsDetected = qfalse;

/*
=================
SystemSettings_FindBuiltinResolution
=================
*/
static int SystemSettings_FindBuiltinResolution( int mode )
{
	int i;

	if( !resolutionsDetected )
		return mode;

	if( mode < 0 )
		return -1;

	for( i = 0; builtinResolutions[ i ]; i++ )
	{
		if( !Q_stricmp( builtinResolutions[ i ], detectedResolutions[ mode ] ) )
			return i;
	}

	return -1;
}

/*
=================
SystemSettings_FindDetectedResolution
=================
*/
static int SystemSettings_FindDetectedResolution( int mode )
{
	int i;

	if( !resolutionsDetected )
		return mode;

	if( mode < 0 )
		return -1;

	for( i = 0; detectedResolutions[ i ]; i++ )
	{
		if( !Q_stricmp( builtinResolutions[ mode ], detectedResolutions[ i ] ) )
			return i;
	}

	return -1;
}

/*
=================
SystemSettings_GetAspectRatios
=================
*/
static void SystemSettings_GetAspectRatios( void )
{
	int i, r;
	
	// build ratio list from resolutions
	for( r = 0; resolutions[r]; r++ )
	{
		int w, h;
		char *x;
		char str[ sizeof(ratioBuf[0]) ];
		
		// calculate resolution's aspect ratio
		x = strchr( resolutions[r], 'x' ) + 1;
		Q_strncpyz( str, resolutions[r], x-resolutions[r] );
		w = atoi( str );
		h = atoi( x );
		Com_sprintf( str, sizeof(str), "%.2f:1", (float)w / (float)h );
		
		// add ratio to list if it is new
		// establish res/ratio relationship
		for( i = 0; ratioBuf[i][0]; i++ )
		{
			if( !Q_stricmp( str, ratioBuf[i] ) )
				break;
		}
		if( !ratioBuf[i][0] )
		{
			Q_strncpyz( ratioBuf[i], str, sizeof(ratioBuf[i]) );
			ratioToRes[i] = r;
		}
		resToRatio[r] = i;
	}
	
	// prepare itemlist pointer array
	// rename common ratios ("1.33:1" -> "4:3")
	for( r = 0; ratioBuf[r][0]; r++ )
	{
		for( i = 0; knownRatios[i][0]; i++ )
		{
			if( !Q_stricmp( ratioBuf[r], knownRatios[i][0] ) )
			{
				Q_strncpyz( ratioBuf[r], knownRatios[i][1], sizeof(ratioBuf[r]) );
				break;
			}
		}
		ratios[r] = ratioBuf[r];
	}
	ratios[r] = NULL;
}

/*
=================
SystemSettings_GetInitialVideo
=================
*/
static void SystemSettings_GetInitialVideo( void )
{
	s_ivo.colordepth  = s_systemsettings.colordepth.curvalue;
	s_ivo.driver      = s_systemsettings.driver.curvalue;
	s_ivo.mode        = s_systemsettings.mode.curvalue;
	s_ivo.fullscreen  = s_systemsettings.fs.curvalue;
	s_ivo.extensions  = s_systemsettings.allow_extensions.curvalue;
	s_ivo.tq          = s_systemsettings.tq.curvalue;
	s_ivo.lighting    = s_systemsettings.lighting.curvalue;
	s_ivo.geometry    = s_systemsettings.geometry.curvalue;
	s_ivo.filter      = s_systemsettings.filter.curvalue;
	s_ivo.texturebits = s_systemsettings.texturebits.curvalue;
	s_ivo.multisample  = s_systemsettings.multisample.curvalue;
	s_ivo.anisotropy  = s_systemsettings.anisotropy.curvalue;
}

/*
=================
SystemSettings_GetResolutions
=================
*/
static void SystemSettings_GetResolutions( void )
{
	Q_strncpyz(resbuf, UI_Cvar_VariableString("r_availableModes"), sizeof(resbuf));
	if(*resbuf)
	{
		char* s = resbuf;
		unsigned int i = 0;
		while( s && i < sizeof(detectedResolutions)/sizeof(detectedResolutions[0])-1)
		{
			detectedResolutions[i++] = s;
			s = strchr(s, ' ');
			if( s )
				*s++ = '\0';
		}
		detectedResolutions[ i ] = NULL;

		if( i > 0 )
		{
			resolutions = detectedResolutions;
			resolutionsDetected = qtrue;
		}
	}
}

/*
=================
SystemSettings_CheckConfig
=================
*/
static void SystemSettings_CheckConfig( void )
{
	int i;

	for ( i = 0; i < NUM_IVO_TEMPLATES-1; i++ )
	{
		if ( s_ivo_templates[i].colordepth != s_systemsettings.colordepth.curvalue )
			continue;
		if ( s_ivo_templates[i].driver != s_systemsettings.driver.curvalue )
			continue;
		if ( SystemSettings_FindDetectedResolution(s_ivo_templates[i].mode) != s_systemsettings.mode.curvalue )
			continue;
		if ( s_ivo_templates[i].fullscreen != s_systemsettings.fs.curvalue )
			continue;
		if ( s_ivo_templates[i].tq != s_systemsettings.tq.curvalue )
			continue;
		if ( s_ivo_templates[i].lighting != s_systemsettings.lighting.curvalue )
			continue;
		if ( s_ivo_templates[i].geometry != s_systemsettings.geometry.curvalue )
			continue;
		if ( s_ivo_templates[i].filter != s_systemsettings.filter.curvalue )
			continue;
		if ( s_ivo_templates[i].multisample != s_systemsettings.multisample.curvalue )
			continue;
		if ( s_ivo_templates[i].anisotropy != s_systemsettings.anisotropy.curvalue )
			continue;
//		if ( s_ivo_templates[i].texturebits != s_systemsettings.texturebits.curvalue )
//			continue;
		s_systemsettings.list.curvalue = i;
		return;
	}

	// return 'Custom' ivo template
	s_systemsettings.list.curvalue = NUM_IVO_TEMPLATES - 1;
}

/*
=================
SystemSettings_UpdateMenuItems
=================
*/
static void SystemSettings_UpdateMenuItems( void )
{
	if ( s_systemsettings.driver.curvalue == 1 )
	{
		s_systemsettings.fs.curvalue = 1;
		s_systemsettings.fs.generic.flags |= QMF_GRAYED;
		s_systemsettings.colordepth.curvalue = 1;
	}
	else
	{
		s_systemsettings.fs.generic.flags &= ~QMF_GRAYED;
	}

	if ( s_systemsettings.fs.curvalue == 0 || s_systemsettings.driver.curvalue == 1 )
	{
		s_systemsettings.colordepth.curvalue = 0;
		s_systemsettings.colordepth.generic.flags |= QMF_GRAYED;
	}
	else
	{
		s_systemsettings.colordepth.generic.flags &= ~QMF_GRAYED;
	}

	if ( s_systemsettings.allow_extensions.curvalue == 0 )
	{
		if ( s_systemsettings.texturebits.curvalue == 0 )
		{
			s_systemsettings.texturebits.curvalue = 1;
		}
	}

	s_systemsettings.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if ( s_ivo.mode != s_systemsettings.mode.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.fullscreen != s_systemsettings.fs.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.extensions != s_systemsettings.allow_extensions.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.tq != s_systemsettings.tq.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.lighting != s_systemsettings.lighting.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.colordepth != s_systemsettings.colordepth.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.driver != s_systemsettings.driver.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.texturebits != s_systemsettings.texturebits.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.geometry != s_systemsettings.geometry.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.filter != s_systemsettings.filter.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.multisample != s_systemsettings.multisample.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.anisotropy != s_systemsettings.anisotropy.curvalue )
	{
		s_systemsettings.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	SystemSettings_CheckConfig();
}

/*
=================
SystemSettings_ApplyChanges
=================
*/
static void SystemSettings_ApplyChanges( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	switch ( s_systemsettings.texturebits.curvalue  )
	{
	case 0:
		trap_Cvar_SetValue( "r_texturebits", 0 );
		break;
	case 1:
		trap_Cvar_SetValue( "r_texturebits", 16 );
		break;
	case 2:
		trap_Cvar_SetValue( "r_texturebits", 32 );
		break;
	}
	trap_Cvar_SetValue( "r_picmip", 3 - s_systemsettings.tq.curvalue );
	trap_Cvar_SetValue( "r_allowExtensions", s_systemsettings.allow_extensions.curvalue );

	if( resolutionsDetected )
	{
		// search for builtin mode that matches the detected mode
		int mode;
		if ( s_systemsettings.mode.curvalue == -1
			|| s_systemsettings.mode.curvalue >= sizeof(detectedResolutions)/sizeof(detectedResolutions[0]) )
			s_systemsettings.mode.curvalue = 0;

		mode = SystemSettings_FindBuiltinResolution( s_systemsettings.mode.curvalue );
		if( mode == -1 )
		{
			char w[ 16 ], h[ 16 ];
			Q_strncpyz( w, detectedResolutions[ s_systemsettings.mode.curvalue ], sizeof( w ) );
			*strchr( w, 'x' ) = 0;
			Q_strncpyz( h,
					strchr( detectedResolutions[ s_systemsettings.mode.curvalue ], 'x' ) + 1, sizeof( h ) );
			trap_Cvar_Set( "r_customwidth", w );
			trap_Cvar_Set( "r_customheight", h );
		}

		trap_Cvar_SetValue( "r_mode", mode );
	}
	else
		trap_Cvar_SetValue( "r_mode", s_systemsettings.mode.curvalue );

	trap_Cvar_SetValue( "r_fullscreen", s_systemsettings.fs.curvalue );
	trap_Cvar_SetValue( "r_colorbits", 0 );
	trap_Cvar_SetValue( "r_depthbits", 0 );
	trap_Cvar_SetValue( "r_stencilbits", 0 );
	trap_Cvar_SetValue( "r_vertexLight", s_systemsettings.lighting.curvalue );

	if ( s_systemsettings.geometry.curvalue == 2 )
	{
		trap_Cvar_SetValue( "r_lodBias", 0 );
		trap_Cvar_SetValue( "r_subdivisions", 4 );
	}
	else if ( s_systemsettings.geometry.curvalue == 1 )
	{
		trap_Cvar_SetValue( "r_lodBias", 1 );
		trap_Cvar_SetValue( "r_subdivisions", 12 );
	}
	else
	{
		trap_Cvar_SetValue( "r_lodBias", 1 );
		trap_Cvar_SetValue( "r_subdivisions", 20 );
	}

	if ( s_systemsettings.multisample.curvalue == 4 )
	{
		trap_Cvar_Set( "r_ext_multisample", "4" );
	}
	else if ( s_systemsettings.multisample.curvalue == 3 )
	{
		trap_Cvar_Set( "r_ext_multisample", "3" );
	}
	else if ( s_systemsettings.multisample.curvalue == 2 )
	{
		trap_Cvar_Set( "r_ext_multisample", "2" );
	}
	else if ( s_systemsettings.multisample.curvalue == 1 )
	{
		trap_Cvar_Set( "r_ext_multisample", "1" );
	}
	else
	{
		trap_Cvar_Set( "r_ext_multisample", "0" );
	}

	if ( s_systemsettings.anisotropy.curvalue == 4 )
	{
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "16" );
		trap_Cvar_Set( "r_ext_max_anisotropy", "16" );
	}
	else if ( s_systemsettings.anisotropy.curvalue == 3 )
	{
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "8" );
		trap_Cvar_Set( "r_ext_max_anisotropy", "8" );
	}
	else if ( s_systemsettings.anisotropy.curvalue == 2 )
	{
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "4" );
		trap_Cvar_Set( "r_ext_max_anisotropy", "4" );
	}
	else if ( s_systemsettings.anisotropy.curvalue == 1 )
	{
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "2" );
		trap_Cvar_Set( "r_ext_max_anisotropy", "2" );
	}
	else
	{
		trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
		trap_Cvar_Set( "r_ext_max_anisotropy", "0" );
	}

	if ( s_systemsettings.filter.curvalue )
	{
		trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR" );
	}
	else
	{
		trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

/*
=================
SystemSettings_Event
=================
*/
static void SystemSettings_Event( void* ptr, int event ) {
	InitialVideoOptions_s *ivo;

	if( event != QM_ACTIVATED ) {
	 	return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_RATIO:
		s_systemsettings.mode.curvalue =
			ratioToRes[ s_systemsettings.ratio.curvalue ];
		// fall through to apply mode constraints

	case ID_MODE:
		// clamp 3dfx video modes
		if ( s_systemsettings.driver.curvalue == 1 )
		{
			if ( s_systemsettings.mode.curvalue < 2 )
				s_systemsettings.mode.curvalue = 2;
			else if ( s_systemsettings.mode.curvalue > 6 )
				s_systemsettings.mode.curvalue = 6;
		}
		s_systemsettings.ratio.curvalue =
			resToRatio[ s_systemsettings.mode.curvalue ];
		break;

	case ID_LIST:
		ivo = &s_ivo_templates[s_systemsettings.list.curvalue];

		s_systemsettings.mode.curvalue        = SystemSettings_FindDetectedResolution(ivo->mode);
		s_systemsettings.ratio.curvalue =
			resToRatio[ s_systemsettings.mode.curvalue ];
		s_systemsettings.tq.curvalue          = ivo->tq;
		s_systemsettings.lighting.curvalue    = ivo->lighting;
		s_systemsettings.colordepth.curvalue  = ivo->colordepth;
		s_systemsettings.texturebits.curvalue = ivo->texturebits;
		s_systemsettings.geometry.curvalue    = ivo->geometry;
		s_systemsettings.filter.curvalue      = ivo->filter;
		s_systemsettings.multisample.curvalue  = ivo->multisample;
		s_systemsettings.anisotropy.curvalue  = ivo->anisotropy;
		s_systemsettings.fs.curvalue          = ivo->fullscreen;
		break;

	case ID_EFFECTSVOLUME:
		trap_Cvar_SetValue( "s_volume", s_systemsettings.sfxvolume.curvalue / 10 );
		break;

	case ID_MUSICVOLUME:
		trap_Cvar_SetValue( "cg_music", s_systemsettings.musicvolume.curvalue / 10 );
		trap_Cvar_SetValue( "s_musicvolume", s_systemsettings.musicvolume.curvalue / 10 );
		break;

	case ID_QUALITY:
		if( s_systemsettings.quality.curvalue ) {
			trap_Cvar_SetValue( "s_khz", 22 );
			trap_Cvar_SetValue( "s_compression", 0 );
		}
		else {
			trap_Cvar_SetValue( "s_khz", 11 );
			trap_Cvar_SetValue( "s_compression", 1 );
		}
		UI_ForceMenuOff();
		trap_Cmd_ExecuteText( EXEC_APPEND, "snd_restart\n" );
		break;

	case ID_CONTROLS:
		UI_PopMenu();
		UI_ControlsMenu();
		break;
		
	case ID_SYSTEM:
		break;
		
	case ID_GENERAL:
		UI_PopMenu();
		UI_PreferencesMenu();
		break;

	case ID_BACK2:
		UI_PopMenu();
		break;
	}
}


/*
================
SystemSettings_TQEvent
================
*/
static void SystemSettings_TQEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
	 	return;
	}
	s_systemsettings.tq.curvalue = (int)(s_systemsettings.tq.curvalue + 0.5);
}


/*
================
SystemSettings_MenuDraw
================
*/
void SystemSettings_MenuDraw (void)
{
//APSFIX - rework this
	SystemSettings_UpdateMenuItems();

	Menu_Draw( &s_systemsettings.menu );
}

/*
=================
SystemSettings_SetMenuItems
=================
*/
static void SystemSettings_SetMenuItems( void )
{
	s_systemsettings.mode.curvalue =
		SystemSettings_FindDetectedResolution( trap_Cvar_VariableValue( "r_mode" ) );

	if ( s_systemsettings.mode.curvalue < 0 )
	{
		if( resolutionsDetected )
		{
			int i;
			char buf[MAX_STRING_CHARS];
			trap_Cvar_VariableStringBuffer("r_customwidth", buf, sizeof(buf)-2);
			buf[strlen(buf)+1] = 0;
			buf[strlen(buf)] = 'x';
			trap_Cvar_VariableStringBuffer("r_customheight", buf+strlen(buf), sizeof(buf)-strlen(buf));

			for(i = 0; detectedResolutions[i]; ++i)
			{
				if(!Q_stricmp(buf, detectedResolutions[i]))
				{
					s_systemsettings.mode.curvalue = i;
					break;
				}
			}
			if ( s_systemsettings.mode.curvalue < 0 )
				s_systemsettings.mode.curvalue = 0;
		}
		else
		{
			s_systemsettings.mode.curvalue = 3;
		}
	}
	s_systemsettings.ratio.curvalue =
		resToRatio[ s_systemsettings.mode.curvalue ];
	s_systemsettings.fs.curvalue = trap_Cvar_VariableValue("r_fullscreen");
	s_systemsettings.allow_extensions.curvalue = trap_Cvar_VariableValue("r_allowExtensions");
	s_systemsettings.tq.curvalue = 3-trap_Cvar_VariableValue( "r_picmip");
	if ( s_systemsettings.tq.curvalue < 0 )
	{
		s_systemsettings.tq.curvalue = 0;
	}
	else if ( s_systemsettings.tq.curvalue > 3 )
	{
		s_systemsettings.tq.curvalue = 3;
	}

	s_systemsettings.lighting.curvalue = trap_Cvar_VariableValue( "r_vertexLight" ) != 0;
	switch ( ( int ) trap_Cvar_VariableValue( "r_texturebits" ) )
	{
	default:
	case 0:
		s_systemsettings.texturebits.curvalue = 0;
		break;
	case 16:
		s_systemsettings.texturebits.curvalue = 1;
		break;
	case 32:
		s_systemsettings.texturebits.curvalue = 2;
		break;
	}

	if ( !Q_stricmp( UI_Cvar_VariableString( "r_textureMode" ), "GL_LINEAR_MIPMAP_NEAREST" ) )
	{
		s_systemsettings.filter.curvalue = 0;
	}
	else
	{
		s_systemsettings.filter.curvalue = 1;
	}

	switch ( ( int ) trap_Cvar_VariableValue( "r_ext_multisample" ) )
	{
	default:
	case 0:
		s_systemsettings.multisample.curvalue = 0;
		break;
	case 1:
		s_systemsettings.multisample.curvalue = 1;
		break;
	case 2:
		s_systemsettings.multisample.curvalue = 2;
		break;
	case 3:
		s_systemsettings.multisample.curvalue = 3;
		break;
	case 4:
		s_systemsettings.multisample.curvalue = 4;
		break;
	}

	switch ( ( int ) trap_Cvar_VariableValue( "r_ext_max_anisotropy" ) )
	{
	default:
	case 0:
		s_systemsettings.anisotropy.curvalue = 0;
		break;
	case 2:
		s_systemsettings.anisotropy.curvalue = 1;
		break;
	case 4:
		s_systemsettings.anisotropy.curvalue = 2;
		break;
	case 8:
		s_systemsettings.anisotropy.curvalue = 3;
		break;
	case 16:
		s_systemsettings.anisotropy.curvalue = 4;
		break;
	}

	if ( trap_Cvar_VariableValue( "r_lodBias" ) > 0 )
	{
		if ( trap_Cvar_VariableValue( "r_subdivisions" ) >= 20 )
		{
			s_systemsettings.geometry.curvalue = 0;
		}
		else
		{
			s_systemsettings.geometry.curvalue = 1;
		}
	}
	else
	{
		s_systemsettings.geometry.curvalue = 2;
	}

	switch ( ( int ) trap_Cvar_VariableValue( "r_colorbits" ) )
	{
	default:
	case 0:
		s_systemsettings.colordepth.curvalue = 0;
		break;
	case 16:
		s_systemsettings.colordepth.curvalue = 1;
		break;
	case 32:
		s_systemsettings.colordepth.curvalue = 2;
		break;
	}

	if ( s_systemsettings.fs.curvalue == 0 )
	{
		s_systemsettings.colordepth.curvalue = 0;
	}
	if ( s_systemsettings.driver.curvalue == 1 )
	{
		s_systemsettings.colordepth.curvalue = 1;
	}
}

/*
================
SystemSettings_MenuInit
================
*/
void SystemSettings_MenuInit( void )
{
	static const char *s_driver_names[] =
	{
		"Default",
		"Voodoo",
		NULL
	};

	static const char *tq_names[] =
	{
		"Default",
		"16 bit",
		"32 bit",
		NULL
	};

	static const char *s_graphics_options_names[] =
	{
		"Very High Quality",
		"High Quality",
		"Normal",
		"Fast",
		"Fastest",
		"Custom",
		NULL
	};

	static const char *lighting_names[] =
	{
		"Lightmap",
		"Vertex",
		NULL
	};

	static const char *colordepth_names[] =
	{
		"Default",
		"16 bit",
		"32 bit",
		NULL
	};

	static const char *filter_names[] =
	{
		"Bilinear",
		"Trilinear",
		NULL
	};

	static const char *multisample_names[] =
	{
		"Off",
		"2x",
		"4x",
		"8x",
		"16x",
		NULL
	};

	static const char *anisotropic_names[] =
	{
		"Off",
		"2x",
		"4x",
		"8x",
		"16x",
		NULL
	};

	static const char *vertexshader_names[] =
	{
		"Disabled",
		"Enabled",
		NULL
	};
	static const char *quality_names[] =
	{
		"Low",
		"Medium",
		"High",
		NULL
	};
	static const char *enabled_names[] =
	{
		"Off",
		"On",
		NULL
	};

	int x = 28;
	int	y = 119;
	int	offset = 38;
	int	style = UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;

	// zero set all our globals
	memset( &s_systemsettings, 0 ,sizeof(systemsettings_t) );

	SystemSettings_GetResolutions();
	SystemSettings_GetAspectRatios();
	
	SystemSettings_Cache();

	s_systemsettings.menu.wrapAround = qtrue;
	s_systemsettings.menu.fullscreen = qtrue;
	s_systemsettings.menu.draw       = SystemSettings_MenuDraw;

	s_systemsettings.controls.generic.type			= MTYPE_PTEXT;
	s_systemsettings.controls.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_systemsettings.controls.generic.id			= ID_CONTROLS;
	s_systemsettings.controls.generic.callback		= SystemSettings_Event;
	s_systemsettings.controls.generic.x				= x;
	s_systemsettings.controls.generic.y				= y;
	s_systemsettings.controls.string				= "CONTROLS";
	s_systemsettings.controls.style					= style;
	s_systemsettings.controls.color					= color_white;
	y+=offset;	
	s_systemsettings.system.generic.type		= MTYPE_PTEXT;
	s_systemsettings.system.generic.flags		= QMF_LEFT_JUSTIFY;
	s_systemsettings.system.generic.id			= ID_SYSTEM;
	s_systemsettings.system.generic.callback	= SystemSettings_Event;
	s_systemsettings.system.generic.x			= x;
	s_systemsettings.system.generic.y			= y;
	s_systemsettings.system.string				= "SYSTEM";
	s_systemsettings.system.style				= style;
	s_systemsettings.system.color				= color_white;
	y+=offset;	
	s_systemsettings.general.generic.type		= MTYPE_PTEXT;
	s_systemsettings.general.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_systemsettings.general.generic.id			= ID_GENERAL;
	s_systemsettings.general.generic.callback	= SystemSettings_Event;
	s_systemsettings.general.generic.x			= x;
	s_systemsettings.general.generic.y			= y;
	s_systemsettings.general.string				= "GENERAL";
	s_systemsettings.general.style				= style;
	s_systemsettings.general.color				= color_white;
	y+=offset;
	s_systemsettings.back.generic.type		= MTYPE_PTEXT;
	s_systemsettings.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_systemsettings.back.generic.id		= ID_BACK2;
	s_systemsettings.back.generic.callback	= SystemSettings_Event;
	s_systemsettings.back.generic.x			= x;
	s_systemsettings.back.generic.y			= y;
	s_systemsettings.back.string			= "BACK";
	s_systemsettings.back.style				= style;
	s_systemsettings.back.color				= color_white;
	y+=offset;	

	s_systemsettings.framel.generic.type  	= MTYPE_BITMAP;
	s_systemsettings.framel.generic.name  	= SYSTEMSETTINGS_FRAMEL;
	s_systemsettings.framel.generic.flags 	= QMF_INACTIVE;
	s_systemsettings.framel.generic.x		= 0;
	s_systemsettings.framel.generic.y		= 78;
	s_systemsettings.framel.width			= 256;
	s_systemsettings.framel.height			= 329;

	s_systemsettings.framer.generic.type	= MTYPE_BITMAP;
	s_systemsettings.framer.generic.name	= SYSTEMSETTINGS_FRAMER;
	s_systemsettings.framer.generic.flags	= QMF_INACTIVE;
	s_systemsettings.framer.generic.x		= 376;
	s_systemsettings.framer.generic.y		= 76;
	s_systemsettings.framer.width			= 256;
	s_systemsettings.framer.height			= 334;

	// references/modifies "r_mode"
	y = 240 - 6 * (BIGCHAR_HEIGHT + 2);
	s_systemsettings.mode.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.mode.generic.name     = "Resolution:";
	s_systemsettings.mode.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.mode.generic.x        = SYSTEM_X_POS;
	s_systemsettings.mode.generic.y        = y;
	s_systemsettings.mode.itemnames        = resolutions;
	s_systemsettings.mode.generic.callback = SystemSettings_Event;
	s_systemsettings.mode.generic.id       = ID_MODE;
	y += BIGCHAR_HEIGHT+2;	
	
	s_systemsettings.ratio.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.ratio.generic.name     = "Aspect Ratio:";
	s_systemsettings.ratio.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.ratio.generic.x        = SYSTEM_X_POS;
	s_systemsettings.ratio.generic.y        = y;
	s_systemsettings.ratio.itemnames        = ratios;
	s_systemsettings.ratio.generic.callback = SystemSettings_Event;
	s_systemsettings.ratio.generic.id       = ID_RATIO;
	y += BIGCHAR_HEIGHT+2;

	// references "r_colorbits"
	s_systemsettings.colordepth.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.colordepth.generic.name     = "Color Depth:";
	s_systemsettings.colordepth.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.colordepth.generic.x        = SYSTEM_X_POS;
	s_systemsettings.colordepth.generic.y        = y;
	s_systemsettings.colordepth.itemnames        = colordepth_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_fullscreen"
	s_systemsettings.fs.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.fs.generic.name	  = "Fullscreen:";
	s_systemsettings.fs.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.fs.generic.x	      = SYSTEM_X_POS;
	s_systemsettings.fs.generic.y	      = y;
	s_systemsettings.fs.itemnames	      = enabled_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_picmip"
	s_systemsettings.tq.generic.type	= MTYPE_SLIDER;
	s_systemsettings.tq.generic.name	= "Texture Detail:";
	s_systemsettings.tq.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.tq.generic.x		= SYSTEM_X_POS;
	s_systemsettings.tq.generic.y		= y;
	s_systemsettings.tq.minvalue       = 0;
	s_systemsettings.tq.maxvalue       = 3;
	s_systemsettings.tq.generic.callback = SystemSettings_TQEvent;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_textureBits"
	s_systemsettings.texturebits.generic.type  = MTYPE_SPINCONTROL;
	s_systemsettings.texturebits.generic.name	= "Texture Quality:";
	s_systemsettings.texturebits.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.texturebits.generic.x	    = SYSTEM_X_POS;
	s_systemsettings.texturebits.generic.y	    = y;
	s_systemsettings.texturebits.itemnames     = tq_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_textureMode"
	s_systemsettings.filter.generic.type   = MTYPE_SPINCONTROL;
	s_systemsettings.filter.generic.name	= "Texture Filter:";
	s_systemsettings.filter.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.filter.generic.x	    = SYSTEM_X_POS;
	s_systemsettings.filter.generic.y	    = y;
	s_systemsettings.filter.itemnames      = filter_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_ext_multisample"
	s_systemsettings.multisample.generic.type   = MTYPE_SPINCONTROL;
	s_systemsettings.multisample.generic.name	= "Multisample:";
	s_systemsettings.multisample.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.multisample.generic.x	    = SYSTEM_X_POS;
	s_systemsettings.multisample.generic.y	    = y;
	s_systemsettings.multisample.itemnames      = multisample_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_textureAnisotropy"
	s_systemsettings.anisotropy.generic.type   = MTYPE_SPINCONTROL;
	s_systemsettings.anisotropy.generic.name	= "Texture Anisotropy:";
	s_systemsettings.anisotropy.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.anisotropy.generic.x	    = SYSTEM_X_POS;
	s_systemsettings.anisotropy.generic.y	    = y;
	s_systemsettings.anisotropy.itemnames      = anisotropic_names;
	y += BIGCHAR_HEIGHT+15;


	s_systemsettings.sfxvolume.generic.type		= MTYPE_SLIDER;
	s_systemsettings.sfxvolume.generic.name		= "Effects Volume:";
	s_systemsettings.sfxvolume.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.sfxvolume.generic.callback	= SystemSettings_Event;
	s_systemsettings.sfxvolume.generic.id		= ID_EFFECTSVOLUME;
	s_systemsettings.sfxvolume.generic.x		= SYSTEM_X_POS;
	s_systemsettings.sfxvolume.generic.y		= y;
	s_systemsettings.sfxvolume.minvalue			= 0;
	s_systemsettings.sfxvolume.maxvalue			= 10;
	y += BIGCHAR_HEIGHT+5;

	s_systemsettings.musicvolume.generic.type		= MTYPE_SLIDER;
	s_systemsettings.musicvolume.generic.name		= "Music Volume:";
	s_systemsettings.musicvolume.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.musicvolume.generic.callback	= SystemSettings_Event;
	s_systemsettings.musicvolume.generic.id			= ID_MUSICVOLUME;
	s_systemsettings.musicvolume.generic.x			= SYSTEM_X_POS;
	s_systemsettings.musicvolume.generic.y			= y;
	s_systemsettings.musicvolume.minvalue			= 0;
	s_systemsettings.musicvolume.maxvalue			= 10;
	y += BIGCHAR_HEIGHT+2;

	s_systemsettings.quality.generic.type		= MTYPE_SPINCONTROL;
	s_systemsettings.quality.generic.name		= "Sound Quality:";
	s_systemsettings.quality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.quality.generic.callback	= SystemSettings_Event;
	s_systemsettings.quality.generic.id			= ID_QUALITY;
	s_systemsettings.quality.generic.x			= SYSTEM_X_POS;
	s_systemsettings.quality.generic.y			= y;
	s_systemsettings.quality.itemnames			= quality_items;	
	y += BIGCHAR_HEIGHT+2;

	s_systemsettings.apply.generic.type     = MTYPE_BITMAP;
	s_systemsettings.apply.generic.name     = SYSTEMSETTINGS_ACCEPT0;
	s_systemsettings.apply.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_systemsettings.apply.generic.callback = SystemSettings_ApplyChanges;
	s_systemsettings.apply.generic.x        = 640;
	s_systemsettings.apply.generic.y        = 480-64;
	s_systemsettings.apply.width			= 128;
	s_systemsettings.apply.height  		 	= 64;
	s_systemsettings.apply.focuspic         = SYSTEMSETTINGS_ACCEPT1;

	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.framel );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.framer );
	
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.controls );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.system );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.general );

	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.mode );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.ratio );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.colordepth );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.fs );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.tq );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.texturebits );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.filter );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.multisample );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.anisotropy );
	
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.sfxvolume );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.musicvolume );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.quality );
	
	s_systemsettings.sfxvolume.curvalue = trap_Cvar_VariableValue( "s_volume" ) * 10;
	s_systemsettings.musicvolume.curvalue = trap_Cvar_VariableValue( "s_musicvolume" ) * 10;
	s_systemsettings.quality.curvalue = !trap_Cvar_VariableValue( "s_compression" );

	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.back );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.apply );

	SystemSettings_SetMenuItems();
	SystemSettings_GetInitialVideo();

	if ( uis.glconfig.driverType == GLDRV_ICD &&
		 uis.glconfig.hardwareType == GLHW_3DFX_2D3D )
	{
		s_systemsettings.driver.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	}
}


/*
=================
SystemSettings_Cache
=================
*/
void SystemSettings_Cache( void ) {
	trap_R_RegisterShaderNoMip( SYSTEMSETTINGS_FRAMEL );
	trap_R_RegisterShaderNoMip( SYSTEMSETTINGS_FRAMER );
	trap_R_RegisterShaderNoMip( SYSTEMSETTINGS_BACK0 );
	trap_R_RegisterShaderNoMip( SYSTEMSETTINGS_BACK1 );
	trap_R_RegisterShaderNoMip( SYSTEMSETTINGS_ACCEPT0 );
	trap_R_RegisterShaderNoMip( SYSTEMSETTINGS_ACCEPT1 );
}


/*
=================
UI_SystemSettingsMenu
=================
*/
void UI_SystemSettingsMenu( void ) {
	uis.menuamount = 4;
	uis.hideEarth = qtrue;
	uis.showFrame = qtrue;
	SystemSettings_MenuInit();
	UI_PushMenu( &s_systemsettings.menu );
	Menu_SetCursorToItem(&s_systemsettings.menu,&s_systemsettings.system);	
}

