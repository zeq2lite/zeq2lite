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
/*
=======================================================================

GAME OPTIONS MENU
=======================================================================*/
#include "ui_local.h"
#define ART_FRAMEL				"interface/art/frame2_l"
#define ART_FRAMER				"interface/art/frame1_r"
#define ART_BACK0				"interface/art/back_0"
#define ART_BACK1				"interface/art/back_1"
#define ART_ACCEPT0				"interface/art/accept_0"
#define ART_ACCEPT1				"interface/art/accept_1"
#define PREFERENCES_X_POS		320
#define ID_CONTROLS				100
#define ID_SYSTEM				101
#define ID_GENERAL				102
#define ID_CROSSHAIR			127
#define ID_CAMERASTYLE			128
#define	ID_BEAMCONTROL			129
#define	ID_BEAMDETAIL			130
#define ID_CROSSHAIRSIZE		131
#define ID_PARTICLESOPTIMISE	132
#define ID_PARTICLESQUALITY		133
#define ID_FLIGHT				134
#define ID_BLOOMQUALITY			135
#define ID_BLOOMINTENSITY		136
#define ID_BLOOMDARKEN			137
#define ID_BLOOMALPHA			138
#define ID_OUTLINES				139
#define ID_CROSSHAIRNAMES		140
#define ID_PORTALS				141
#define ID_BACK2				142
#define	NUM_CROSSHAIRS			10
typedef struct{
	menuframework_s		menu;
	menubitmap_s		framel;
	menubitmap_s		framer;
	menutext_s			controls;
	menutext_s			system;
	menutext_s			general;	
	menulist_s			crosshair;
	menulist_s			crosshairSize;
	menulist_s			camerastyle;
	menulist_s			beamdetail;
	menulist_s			particlesOptimise;
	menulist_s			particlesQuality;	
	menulist_s			bloomQuality;
	menuslider_s		bloomIntensity;
	menulist_s			bloomDarken;
	menuslider_s		bloomAlpha;
	menulist_s			beamcontrol;
	menuradiobutton_s	outlines;
	menuradiobutton_s	flight;
	menuradiobutton_s	crosshairNames;
	menuradiobutton_s	portals;
	menubitmap_s		apply;
	menutext_s			back;
	qhandle_t			crosshairShader[NUM_CROSSHAIRS];
} preferences_t;
static int	initialBloomQuality;
static preferences_t s_preferences;
static const char *particlesQuality_names[] = {"Off","Sprites","Models",0};
static const char *particlesOptimise_names[] = {"Remove After Awhile","Remove On Stop",	0};
static const char *beamdetail_names[] = {"Very Low","Low","Medium","High","Very High", 0};
static const char *beamcontrol_names[] = {"Beam Head Focus","Crosshair Focus",0};
static const char *camerastyle_names[] = {"Eyes","Behind Character",0};
static const char *crosshairSize_names[] = {"Tiny!","Very Small","Small","Normal","Large","Extra Large!","Extra Extra Large?!",0};
static const char *bloomQuality_names[] = {"Off","Low","Medium","High","Very High", 0};
static const char *bloomIntensity_names[] = {"Low","Medium","High", 0};
static const char *bloomDarken_names[] = {"Low","Medium","High", 0};
static const char *bloomAlpha_names[] = {"Low","Medium","High", 0};
static void Preferences_SetMenuItems( void ) {
	s_preferences.crosshair.curvalue		= (int)trap_Cvar_VariableValue( "cg_drawCrosshair" ) % NUM_CROSSHAIRS;
	s_preferences.camerastyle.curvalue		= Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_thirdPersonCamera" ) );
	s_preferences.crosshairSize.curvalue	= Com_Clamp( 0, 6, trap_Cvar_VariableValue( "cg_crosshairSize" ) );
	s_preferences.beamdetail.curvalue		= Com_Clamp( 0, 4, trap_Cvar_VariableValue( "r_beamDetail" ) );
	s_preferences.particlesQuality.curvalue	= Com_Clamp( 0, 2, trap_Cvar_VariableValue( "cg_particlesQuality" ) );
	s_preferences.particlesOptimise.curvalue= Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_particlesStop" ) );
	s_preferences.beamcontrol.curvalue		= Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_beamControl" ) );
	s_preferences.outlines.curvalue			= trap_Cvar_VariableValue( "r_outlines" ) != 0;
	s_preferences.flight.curvalue			= trap_Cvar_VariableValue( "cg_advancedFlight" ) != 0;
	s_preferences.crosshairNames.curvalue	= trap_Cvar_VariableValue( "cg_drawCrosshairNames" ) != 0;
	s_preferences.portals.curvalue			= trap_Cvar_VariableValue( "r_portals" ) != 0;
	// Bloom Quality
	switch ( ( int ) trap_Cvar_VariableValue( "r_bloom_sample_size" ) )
	{
	default:
	case 0:
		s_preferences.bloomQuality.curvalue = 0;
		break;
	case 64:
		s_preferences.bloomQuality.curvalue = 1;
		break;
	case 128:
		s_preferences.bloomQuality.curvalue = 2;
		break;
	case 256:
		s_preferences.bloomQuality.curvalue = 3;
		break;
	case 512:
		s_preferences.bloomQuality.curvalue = 4;
		break;
	}
	// Bloom Intensity
	s_preferences.bloomIntensity.curvalue = trap_Cvar_VariableValue( "r_bloom_intensity" );
	// Bloom Darken
	switch ( ( int ) trap_Cvar_VariableValue( "r_bloom_darken" ) )
	{
	default:
	case 16:
		s_preferences.bloomDarken.curvalue = 0;
		break;
	case 32:
		s_preferences.bloomDarken.curvalue = 1;
		break;
	case 64:
		s_preferences.bloomDarken.curvalue = 2;
		break;
	}
	// Bloom Alpha
	s_preferences.bloomAlpha.curvalue = trap_Cvar_VariableValue( "r_bloom_alpha" );
}
static void Preferences_Event( void* ptr, int notification ) {
	if(notification != QM_ACTIVATED){
		return;
	}
	switch(((menucommon_s*)ptr)->id){
	case ID_CROSSHAIR:
		s_preferences.crosshair.curvalue++;
		if( s_preferences.crosshair.curvalue == NUM_CROSSHAIRS ) {
			s_preferences.crosshair.curvalue = 0;
		}
		trap_Cvar_SetValue( "cg_drawCrosshair", s_preferences.crosshair.curvalue );
		break;
	case ID_CROSSHAIRSIZE:
		trap_Cvar_SetValue( "cg_crosshairSize", s_preferences.crosshairSize.curvalue);
		break;
	case ID_CROSSHAIRNAMES:
		trap_Cvar_SetValue( "cg_drawCrosshairNames", s_preferences.crosshairNames.curvalue);
		break;
	case ID_CAMERASTYLE:
		trap_Cvar_SetValue( "cg_thirdPersonCamera", s_preferences.camerastyle.curvalue );
		break;
	case ID_BEAMCONTROL:
		trap_Cvar_SetValue( "cg_beamControl", s_preferences.beamcontrol.curvalue );
		break;
	case ID_BEAMDETAIL:
		trap_Cvar_SetValue( "r_beamDetail", s_preferences.beamdetail.curvalue);
		break;
	case ID_PARTICLESQUALITY:
		trap_Cvar_SetValue( "cg_particlesQuality", s_preferences.particlesQuality.curvalue );
		break;
	case ID_PARTICLESOPTIMISE:
		trap_Cvar_SetValue( "cg_particlesStop", s_preferences.particlesOptimise.curvalue );
		break;
	case ID_PORTALS:
		trap_Cvar_SetValue( "r_portals", s_preferences.portals.curvalue );
		break;
	case ID_OUTLINES:
		trap_Cvar_SetValue( "r_outlines", s_preferences.outlines.curvalue );
		break;
	case ID_FLIGHT:
		trap_Cvar_SetValue( "cg_advancedFlight", s_preferences.flight.curvalue );
		break;		
	case ID_BLOOMQUALITY:
		switch ( s_preferences.bloomQuality.curvalue  )
		{
		case 0:
			trap_Cvar_SetValue( "r_bloom_sample_size", 0 );
			trap_Cvar_SetValue( "r_bloom", 0 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_bloom_sample_size", 64 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_bloom_sample_size", 128 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		case 3:
			trap_Cvar_SetValue( "r_bloom_sample_size", 256 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		case 4:
			trap_Cvar_SetValue( "r_bloom_sample_size", 512 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		}
		break;
	case ID_BLOOMINTENSITY:
		trap_Cvar_SetValue( "r_bloom_intensity", s_preferences.bloomIntensity.curvalue);
		break;
	case ID_BLOOMDARKEN:
		switch ( s_preferences.bloomDarken.curvalue  )
		{
		case 0:
			trap_Cvar_SetValue( "r_bloom_darken", 16 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_bloom_darken", 32 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_bloom_darken", 64 );
			break;
		}
		break;
	case ID_BLOOMALPHA:
		trap_Cvar_SetValue( "r_bloom_alpha", s_preferences.bloomAlpha.curvalue);
		break;
	case ID_CONTROLS:
		UI_PopMenu();
		UI_ControlsMenu();
		break;		
	case ID_SYSTEM:
		UI_PopMenu();
		UI_SystemSettingsMenu();
		break;
	case ID_GENERAL:
		break;		
	case ID_BACK2:
		UI_PopMenu();
		break;
	}
}
/*=================
Crosshair_Draw
=================*/
static void Crosshair_Draw( void *self ) {
	menulist_s	*s;
	float		*color;
	int			x, y;
	int			style;
	int			crosshairSizeImage;
	qboolean	focus;
	crosshairSizeImage = trap_Cvar_VariableValue( "cg_crosshairSize" ) * 8 + 8;
	s = (menulist_s *)self;
	x = s->generic.x;
	y =	s->generic.y;
	style = UI_TINYFONT|UI_DROPSHADOW;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);
	if(s->generic.flags & QMF_GRAYED){
		color = text_color_disabled;
	}
	else if(focus){
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if(s->generic.flags & QMF_BLINK){
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else{
		color = text_color_normal;
	}
	if(focus){
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT|UI_DROPSHADOW, color);
	}
	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	if(!s->curvalue){return;}
	UI_DrawHandlePic( x + SMALLCHAR_WIDTH + 8 - 0.5f * crosshairSizeImage, y + 6 - 0.5f * crosshairSizeImage, crosshairSizeImage, crosshairSizeImage, s_preferences.crosshairShader[s->curvalue] );
}

/*
=================
Bloom_ApplyChanges
=================
*/
static void Bloom_ApplyChanges( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}
/*
================
Preferences_MenuDraw
================
*/
void Preferences_MenuDraw (void)
{
	s_preferences.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if ( initialBloomQuality != s_preferences.bloomQuality.curvalue )
	{
		s_preferences.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	Menu_Draw( &s_preferences.menu );
}

static void Preferences_MenuInit( void ) {
	int x = 28;
	int	y = 119;
	int	offset = 38;
	int	style = UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	memset( &s_preferences, 0 ,sizeof(preferences_t) );
	Preferences_Cache();
	s_preferences.menu.wrapAround = qtrue;
	s_preferences.menu.fullscreen = qtrue;
	s_preferences.menu.draw = Preferences_MenuDraw;
	s_preferences.framel.generic.type  = MTYPE_BITMAP;
	s_preferences.framel.generic.name  = ART_FRAMEL;
	s_preferences.framel.generic.flags = QMF_INACTIVE;
	s_preferences.framel.generic.x	   = 0;
	s_preferences.framel.generic.y	   = 78;
	s_preferences.framel.width  	   = 256;
	s_preferences.framel.height  	   = 329;
	s_preferences.framer.generic.type  = MTYPE_BITMAP;
	s_preferences.framer.generic.name  = ART_FRAMER;
	s_preferences.framer.generic.flags = QMF_INACTIVE;
	s_preferences.framer.generic.x	   = 376;
	s_preferences.framer.generic.y	   = 76;
	s_preferences.framer.width  	   = 256;
	s_preferences.framer.height  	   = 334;

	s_preferences.controls.generic.type			= MTYPE_PTEXT;
	s_preferences.controls.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_preferences.controls.generic.id			= ID_CONTROLS;
	s_preferences.controls.generic.callback		= Preferences_Event;
	s_preferences.controls.generic.x			= x;
	s_preferences.controls.generic.y			= y;
	s_preferences.controls.string				= "CONTROLS";
	s_preferences.controls.style				= style;
	s_preferences.controls.color				= color_silver;
	y+=offset;
	s_preferences.system.generic.type		= MTYPE_PTEXT;
	s_preferences.system.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_preferences.system.generic.id			= ID_SYSTEM;
	s_preferences.system.generic.callback	= Preferences_Event;
	s_preferences.system.generic.x			= x;
	s_preferences.system.generic.y			= y;
	s_preferences.system.string				= "SYSTEM";
	s_preferences.system.style				= style;
	s_preferences.system.color				= color_silver;
	y+=offset;		
	s_preferences.general.generic.type		= MTYPE_PTEXT;
	s_preferences.general.generic.flags		= QMF_RIGHT_JUSTIFY;
	s_preferences.general.generic.id		= ID_GENERAL;
	s_preferences.general.generic.callback	= Preferences_Event;
	s_preferences.general.generic.x			= x;
	s_preferences.general.generic.y			= y;
	s_preferences.general.string			= "GENERAL";
	s_preferences.general.style				= style;
	s_preferences.general.color				= color_silver;
	y+=offset;	
	s_preferences.back.generic.type			= MTYPE_PTEXT;
	s_preferences.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_preferences.back.generic.id			= ID_BACK2;
	s_preferences.back.generic.callback		= Preferences_Event;
	s_preferences.back.generic.x			= x;
	s_preferences.back.generic.y			= y;
	s_preferences.back.string				= "BACK";
	s_preferences.back.style				= style;
	s_preferences.back.color				= color_silver;
	y+=offset;	
	
	y = 132;
	s_preferences.crosshair.generic.type		= MTYPE_TEXT;
	s_preferences.crosshair.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NODEFAULTINIT|QMF_OWNERDRAW;
	s_preferences.crosshair.generic.x			= PREFERENCES_X_POS;
	s_preferences.crosshair.generic.y			= y;
	s_preferences.crosshair.generic.name		= "Crosshair:";
	s_preferences.crosshair.generic.callback	= Preferences_Event;
	s_preferences.crosshair.generic.ownerdraw	= Crosshair_Draw;
	s_preferences.crosshair.generic.id			= ID_CROSSHAIR;
	s_preferences.crosshair.generic.top			= y - 2;
	s_preferences.crosshair.generic.bottom		= y + 16;
	s_preferences.crosshair.generic.left		= PREFERENCES_X_POS - ( ( strlen(s_preferences.crosshair.generic.name) + 1 ) * SMALLCHAR_WIDTH ) - 32;
	s_preferences.crosshair.generic.right		= PREFERENCES_X_POS;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.crosshairSize.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.crosshairSize.generic.name		= "Crosshair Size:";
	s_preferences.crosshairSize.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.crosshairSize.generic.callback	= Preferences_Event;
	s_preferences.crosshairSize.generic.id			= ID_CROSSHAIRSIZE;
	s_preferences.crosshairSize.generic.x			= PREFERENCES_X_POS;
	s_preferences.crosshairSize.generic.y			= y;
	s_preferences.crosshairSize.itemnames			= crosshairSize_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.crosshairNames.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.crosshairNames.generic.name		= "Player Names:";
	s_preferences.crosshairNames.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.crosshairNames.generic.callback	= Preferences_Event;
	s_preferences.crosshairNames.generic.id			= ID_CROSSHAIRNAMES;
	s_preferences.crosshairNames.generic.x			= PREFERENCES_X_POS;
	s_preferences.crosshairNames.generic.y			= y;
	
	y += BIGCHAR_HEIGHT+2;
	s_preferences.camerastyle.generic.type        = MTYPE_SPINCONTROL;
	s_preferences.camerastyle.generic.name	      = "Camera style:";
	s_preferences.camerastyle.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.camerastyle.generic.callback    = Preferences_Event;
	s_preferences.camerastyle.generic.id          = ID_CAMERASTYLE;
	s_preferences.camerastyle.generic.x	          = PREFERENCES_X_POS;
	s_preferences.camerastyle.generic.y	          = y;
	s_preferences.camerastyle.itemnames			  = camerastyle_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.beamcontrol.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.beamcontrol.generic.name		= "Beam Control Style:";
	s_preferences.beamcontrol.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.beamcontrol.generic.callback	= Preferences_Event;
	s_preferences.beamcontrol.generic.id		= ID_BEAMCONTROL;
	s_preferences.beamcontrol.generic.x			= PREFERENCES_X_POS;
	s_preferences.beamcontrol.generic.y			= y;
	s_preferences.beamcontrol.itemnames			= beamcontrol_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.beamdetail.generic.type			  = MTYPE_SPINCONTROL;
	s_preferences.beamdetail.generic.name			  = "Beam Detail:";
	s_preferences.beamdetail.generic.flags	          = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.beamdetail.generic.callback         = Preferences_Event;
	s_preferences.beamdetail.generic.id               = ID_BEAMDETAIL;
	s_preferences.beamdetail.generic.x	              = PREFERENCES_X_POS;
	s_preferences.beamdetail.generic.y	              = y;
	s_preferences.beamdetail.itemnames				  = beamdetail_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.particlesQuality.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.particlesQuality.generic.name			= "Particle Quality:";
	s_preferences.particlesQuality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.particlesQuality.generic.callback		= Preferences_Event;
	s_preferences.particlesQuality.generic.id			= ID_PARTICLESQUALITY;
	s_preferences.particlesQuality.generic.x			= PREFERENCES_X_POS;
	s_preferences.particlesQuality.generic.y			= y;
	s_preferences.particlesQuality.itemnames			= particlesQuality_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.particlesOptimise.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.particlesOptimise.generic.name		= "Particle Physics:";
	s_preferences.particlesOptimise.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.particlesOptimise.generic.callback	= Preferences_Event;
	s_preferences.particlesOptimise.generic.id			= ID_PARTICLESOPTIMISE;
	s_preferences.particlesOptimise.generic.x			= PREFERENCES_X_POS;
	s_preferences.particlesOptimise.generic.y			= y;
	s_preferences.particlesOptimise.itemnames			= particlesOptimise_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.portals.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.portals.generic.name		= "Reflections:";
	s_preferences.portals.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.portals.generic.callback	= Preferences_Event;
	s_preferences.portals.generic.id		= ID_PORTALS;
	s_preferences.portals.generic.x			= PREFERENCES_X_POS;
	s_preferences.portals.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.outlines.generic.type			= MTYPE_RADIOBUTTON;
	s_preferences.outlines.generic.name			= "Character Outlines:";
	s_preferences.outlines.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.outlines.generic.callback		= Preferences_Event;
	s_preferences.outlines.generic.id			= ID_OUTLINES;
	s_preferences.outlines.generic.x			= PREFERENCES_X_POS;
	s_preferences.outlines.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.flight.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.flight.generic.name		= "Advanced Flight:";
	s_preferences.flight.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.flight.generic.callback	= Preferences_Event;
	s_preferences.flight.generic.id			= ID_FLIGHT;
	s_preferences.flight.generic.x			= PREFERENCES_X_POS;
	s_preferences.flight.generic.y			= y;	

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomQuality.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.bloomQuality.generic.name			= "Bloom Quality:";
	s_preferences.bloomQuality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomQuality.generic.callback		= Preferences_Event;
	s_preferences.bloomQuality.generic.id			= ID_BLOOMQUALITY;
	s_preferences.bloomQuality.generic.x			= PREFERENCES_X_POS;
	s_preferences.bloomQuality.generic.y			= y;
	s_preferences.bloomQuality.itemnames			= bloomQuality_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomDarken.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.bloomDarken.generic.name		= "Bloom Darken:";
	s_preferences.bloomDarken.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomDarken.generic.callback	= Preferences_Event;
	s_preferences.bloomDarken.generic.id		= ID_BLOOMDARKEN;
	s_preferences.bloomDarken.generic.x			= PREFERENCES_X_POS;
	s_preferences.bloomDarken.generic.y			= y;
	s_preferences.bloomDarken.itemnames			= bloomDarken_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomIntensity.generic.type		= MTYPE_SLIDER;
	s_preferences.bloomIntensity.generic.name		= "Bloom Intensity:";
	s_preferences.bloomIntensity.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomIntensity.generic.callback	= Preferences_Event;
	s_preferences.bloomIntensity.generic.id			= ID_BLOOMINTENSITY;
	s_preferences.bloomIntensity.generic.x			= PREFERENCES_X_POS - 8;
	s_preferences.bloomIntensity.generic.y			= y;
	s_preferences.bloomIntensity.minvalue			= 0;
	s_preferences.bloomIntensity.maxvalue			= 1;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomAlpha.generic.type		= MTYPE_SLIDER;
	s_preferences.bloomAlpha.generic.name		= "Bloom Alpha:";
	s_preferences.bloomAlpha.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomAlpha.generic.callback	= Preferences_Event;
	s_preferences.bloomAlpha.generic.id			= ID_BLOOMALPHA;
	s_preferences.bloomAlpha.generic.x			= PREFERENCES_X_POS - 8;
	s_preferences.bloomAlpha.generic.y			= y;
	s_preferences.bloomAlpha.minvalue			= 0;
	s_preferences.bloomAlpha.maxvalue			= 1;

	s_preferences.apply.generic.type     = MTYPE_BITMAP;
	s_preferences.apply.generic.name     = ART_ACCEPT0;
	s_preferences.apply.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_preferences.apply.generic.callback = Bloom_ApplyChanges;
	s_preferences.apply.generic.x        = 640;
	s_preferences.apply.generic.y        = 480-64;
	s_preferences.apply.width  		     = 128;
	s_preferences.apply.height  		 = 64;
	s_preferences.apply.focuspic         = ART_ACCEPT1;

	Menu_AddItem( &s_preferences.menu, ( void * ) &s_preferences.controls );
	Menu_AddItem( &s_preferences.menu, ( void * ) &s_preferences.system );
	Menu_AddItem( &s_preferences.menu, ( void * ) &s_preferences.general );
	
	Menu_AddItem( &s_preferences.menu, &s_preferences.framel );
	Menu_AddItem( &s_preferences.menu, &s_preferences.framer );
	Menu_AddItem( &s_preferences.menu, &s_preferences.crosshair );
	Menu_AddItem( &s_preferences.menu, &s_preferences.crosshairSize );
	Menu_AddItem( &s_preferences.menu, &s_preferences.crosshairNames );
	Menu_AddItem( &s_preferences.menu, &s_preferences.camerastyle );
	Menu_AddItem( &s_preferences.menu, &s_preferences.beamcontrol );
	Menu_AddItem( &s_preferences.menu, &s_preferences.beamdetail );
	Menu_AddItem( &s_preferences.menu, &s_preferences.particlesQuality );
	Menu_AddItem( &s_preferences.menu, &s_preferences.particlesOptimise );
	Menu_AddItem( &s_preferences.menu, &s_preferences.portals );
	Menu_AddItem( &s_preferences.menu, &s_preferences.outlines );
	Menu_AddItem( &s_preferences.menu, &s_preferences.flight );		
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomQuality );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomIntensity );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomDarken );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomAlpha );
	Menu_AddItem( &s_preferences.menu, &s_preferences.back );
	Menu_AddItem( &s_preferences.menu, &s_preferences.apply );

	Preferences_SetMenuItems();
	initialBloomQuality = s_preferences.bloomQuality.curvalue;
}
/*===============
Preferences_Cache
===============*/
void Preferences_Cache(void){
	int	n;
	trap_R_RegisterShaderNoMip(ART_FRAMEL);
	trap_R_RegisterShaderNoMip(ART_FRAMER);
	trap_R_RegisterShaderNoMip(ART_BACK0);
	trap_R_RegisterShaderNoMip(ART_BACK1);
	trap_R_RegisterShaderNoMip(ART_ACCEPT0);
	trap_R_RegisterShaderNoMip(ART_ACCEPT1);
	for(n=0;n<NUM_CROSSHAIRS;n++){
		s_preferences.crosshairShader[n] = trap_R_RegisterShaderNoMip( va("crosshair%c", 'a' + n ) );
	}
}
/*===============
UI_PreferencesMenu
===============*/
void UI_PreferencesMenu( void ) {
	uis.menuamount = 4;
	uis.hideEarth = qtrue;
	uis.showFrame = qtrue;
	Preferences_MenuInit();
	UI_PushMenu( &s_preferences.menu );
	Menu_SetCursorToItem(&s_preferences.menu,&s_preferences.general);	
}
