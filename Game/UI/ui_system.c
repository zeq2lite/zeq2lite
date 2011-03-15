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
#define ID_SYSTEM			100
#define ID_CONTROLS			101
#define ID_GENERAL			102
#define ID_FULLSCREEN		103
#define ID_LIST				104
#define ID_MODE				105
#define ID_EFFECTSVOLUME	106
#define ID_MUSICVOLUME		107
#define ID_QUALITY			108
#define ID_BACK2			109




typedef struct {
	menuframework_s	menu;

	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		system;
	menutext_s		controls;
	menutext_s		general;

	menulist_s		list;
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
	menubitmap_s	back;
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
		4, qtrue, 2, 0, 2, 2, 1, 1, 0, 0, qtrue	// JDC: this was tq 3
	},
	{
		3, qtrue, 2, 0, 0, 0, 1, 0, 0, 0, qtrue
	},
	{
		2, qtrue, 1, 0, 1, 0, 0, 0, 0, 0, qtrue
	},
	{
		2, qtrue, 1, 1, 1, 0, 0, 0, 0, 0, qtrue
	},
	{
		3, qtrue, 1, 0, 0, 0, 1, 0, 0, 0, qtrue
	}
};

static const char *quality_items[] = {
	"Low", "High", NULL
};

#define NUM_IVO_TEMPLATES ( sizeof( s_ivo_templates ) / sizeof( s_ivo_templates[0] ) )

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
SystemSettings_CheckConfig
=================
*/
static void SystemSettings_CheckConfig( void )
{
	int i;

	for ( i = 0; i < NUM_IVO_TEMPLATES; i++ )
	{
		if ( s_ivo_templates[i].colordepth != s_systemsettings.colordepth.curvalue )
			continue;
		if ( s_ivo_templates[i].driver != s_systemsettings.driver.curvalue )
			continue;
		if ( s_ivo_templates[i].mode != s_systemsettings.mode.curvalue )
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
	s_systemsettings.list.curvalue = 4;
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
	trap_Cvar_SetValue( "r_mode", s_systemsettings.mode.curvalue );
	trap_Cvar_SetValue( "r_fullscreen", s_systemsettings.fs.curvalue );
	switch ( s_systemsettings.colordepth.curvalue )
	{
	case 0:
		trap_Cvar_SetValue( "r_colorbits", 0 );
		trap_Cvar_SetValue( "r_depthbits", 0 );
		trap_Cvar_SetValue( "r_stencilbits", 0 );
		break;
	case 1:
		trap_Cvar_SetValue( "r_colorbits", 16 );
		trap_Cvar_SetValue( "r_depthbits", 16 );
		trap_Cvar_SetValue( "r_stencilbits", 0 );
		break;
	case 2:
		trap_Cvar_SetValue( "r_colorbits", 32 );
		trap_Cvar_SetValue( "r_depthbits", 24 );
		break;
	}
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
	case ID_MODE:
		// clamp 3dfx video modes
		if ( s_systemsettings.driver.curvalue == 1 )
		{
			if ( s_systemsettings.mode.curvalue < 2 )
				s_systemsettings.mode.curvalue = 2;
			else if ( s_systemsettings.mode.curvalue > 6 )
				s_systemsettings.mode.curvalue = 6;
		}
		break;

	case ID_LIST:
		ivo = &s_ivo_templates[s_systemsettings.list.curvalue];

		s_systemsettings.mode.curvalue        = ivo->mode;
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
		
	case ID_SYSTEM:
		UI_SystemSettingsMenu();
		break;

	case ID_CONTROLS:
		UI_ControlsMenu();
		break;
		
	case ID_GENERAL:
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
	s_systemsettings.mode.curvalue = trap_Cvar_VariableValue( "r_mode" );
	if ( s_systemsettings.mode.curvalue < 0 )
	{
		s_systemsettings.mode.curvalue = 3;
	}
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

	static const char *resolutions[] = 
	{
		"320 x 240",
		"400 x 300",
		"512 x 384",
		"640 x 480",
		"800 x 600",
		"960 x 720",
		"1024 x 768",
		"1152 x 864",
		"1280 x 1024",
		"1600 x 1200",
		"2048 x 1536",
		"856 x 480 16:9",
		"1280 x 720 16:9",
		"1365 x 768 16:9",
		"1600 x 900 16:9",
		"1920 x 1080 16:9",
		"1440 x 900 16:10",
		"1680 x 1050 16:10",
		"1920 x 1200 16:10",
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
	
	SystemSettings_Cache();

	s_systemsettings.menu.wrapAround = qtrue;
	s_systemsettings.menu.fullscreen = qtrue;
	s_systemsettings.menu.draw       = SystemSettings_MenuDraw;

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

/*
	s_systemsettings.list.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.list.generic.name     = "Graphics Settings:";
	s_systemsettings.list.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.list.generic.x        = SYSTEM_X_POS;
	s_systemsettings.list.generic.y        = y;
	s_systemsettings.list.generic.callback = SystemSettings_Event;
	s_systemsettings.list.generic.id       = ID_LIST;
	s_systemsettings.list.itemnames        = s_graphics_options_names;
	y += 2 * ( BIGCHAR_HEIGHT + 2 );

	s_systemsettings.driver.generic.type  = MTYPE_SPINCONTROL;
	s_systemsettings.driver.generic.name  = "GL Driver:";
	s_systemsettings.driver.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.driver.generic.x     = SYSTEM_X_POS;
	s_systemsettings.driver.generic.y     = y;
	s_systemsettings.driver.itemnames     = s_driver_names;
	s_systemsettings.driver.curvalue      = (uis.glconfig.driverType == GLDRV_VOODOO);
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_allowExtensions"
	s_systemsettings.allow_extensions.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.allow_extensions.generic.name	    = "GL Extensions:";
	s_systemsettings.allow_extensions.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.allow_extensions.generic.x	    = SYSTEM_X_POS;
	s_systemsettings.allow_extensions.generic.y	    = y;
	s_systemsettings.allow_extensions.itemnames        = enabled_names;
	y += BIGCHAR_HEIGHT+2;
*/
	// references/modifies "r_mode"
	y = 240 - 6 * (BIGCHAR_HEIGHT + 2);
	s_systemsettings.mode.generic.type     = MTYPE_SPINCONTROL;
	s_systemsettings.mode.generic.name     = "Video Mode:";
	s_systemsettings.mode.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.mode.generic.x        = SYSTEM_X_POS;
	s_systemsettings.mode.generic.y        = y;
	s_systemsettings.mode.itemnames        = resolutions;
	s_systemsettings.mode.generic.callback = SystemSettings_Event;
	s_systemsettings.mode.generic.id       = ID_MODE;
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

/*	// references/modifies "r_vertexLight"
	s_systemsettings.lighting.generic.type  = MTYPE_SPINCONTROL;
	s_systemsettings.lighting.generic.name	 = "Lighting:";
	s_systemsettings.lighting.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.lighting.generic.x	 = SYSTEM_X_POS;
	s_systemsettings.lighting.generic.y	 = y;
	s_systemsettings.lighting.itemnames     = lighting_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_lodBias" & "subdivisions"
	s_systemsettings.geometry.generic.type  = MTYPE_SPINCONTROL;
	s_systemsettings.geometry.generic.name	 = "Geometric Detail:";
	s_systemsettings.geometry.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_systemsettings.geometry.generic.x	 = SYSTEM_X_POS;
	s_systemsettings.geometry.generic.y	 = y;
	s_systemsettings.geometry.itemnames     = quality_names;
	y += BIGCHAR_HEIGHT+2;
*/

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
	
	s_systemsettings.back.generic.type			= MTYPE_BITMAP;
	s_systemsettings.back.generic.name			= SYSTEMSETTINGS_BACK0;
	s_systemsettings.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_systemsettings.back.generic.callback		= SystemSettings_Event;
	s_systemsettings.back.generic.id			= ID_BACK2;
	s_systemsettings.back.generic.x				= 0;
	s_systemsettings.back.generic.y				= 480-64;
	s_systemsettings.back.width					= 128;
	s_systemsettings.back.height				= 64;
	s_systemsettings.back.focuspic				= SYSTEMSETTINGS_BACK1;

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
	
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.system );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.controls );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.general );

//	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.list );
//	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.driver );
//	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.allow_extensions );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.mode );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.colordepth );
	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.fs );
//	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.lighting );
//	Menu_AddItem( &s_systemsettings.menu, ( void * ) &s_systemsettings.geometry );
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
	uis.menuamount = 3;
	uis.hideEarth = qtrue;
	uis.showFrame = qtrue;
	SystemSettings_MenuInit();
	UI_PushMenu( &s_systemsettings.menu );
	Menu_SetCursorToItem(&s_systemsettings.menu,&s_systemsettings.system);	
}

