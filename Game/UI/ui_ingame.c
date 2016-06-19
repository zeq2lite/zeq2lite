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

INWORLD MENU

=======================================================================
*/

#include "ui_local.h"

#define MENU_FRAME					"interface/art/addbotframe"
#define MENU_VERTICAL_SPACING	28

#define ID_SETTINGS				13
#define ID_CHARACTER			14
#define ID_CAMERA				15
#define ID_LEAVE				16
#define ID_QUIT					18
#define ID_RESUME				19

typedef struct {
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		resume;
	menutext_s		settings;
	menutext_s		character;
	menutext_s		camera;
	menutext_s		leave;
	menutext_s		quit;
} inworldmenu_t;

static inworldmenu_t	s_inworld;

/*
=================
QuitAction
=================
*/
static void QuitAction( qboolean result ) {
	if(!result) {return;}
	UI_PopMenu();
	UI_CreditMenu();
}

/*
=================
LeaveAction
=================
*/
static void LeaveAction( qboolean result ) {
	if(!result) {return;}
	// JUHOX: reset edit mode
#if MAPLENSFLARES
	trap_Cvar_Set("g_editmode", "0");
#endif
	trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
}

/*
=================
InWorld_Event
=================
*/
void InWorld_Event( void *ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {return;}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_RESUME:
		UI_PopMenu();
		break;
	case ID_SETTINGS:
		UI_SystemSettingsMenu();
		break;
	case ID_CHARACTER:
		UI_PlayerModelMenu();
		break;	
	case ID_CAMERA:
		UI_CameraMenu();
		break;		
	case ID_LEAVE:
		UI_ConfirmMenu( "LEAVE SAGA?",  0, LeaveAction );
		break;
	case ID_QUIT:
		UI_ConfirmMenu( "QUIT ZEQII-Lite?",  0, QuitAction );
		break;
	}
}

/*
=================
InWorld_Cache
=================
*/
void InWorld_Cache( void ) {
	trap_R_RegisterShaderNoMip( MENU_FRAME );
}

/*
=================
InWorld_MenuInit
=================
*/
void InWorld_MenuInit( void ) {
	int		y;
	uiClientState_t	cs;
	char	info[MAX_INFO_STRING];

	memset( &s_inworld, 0 ,sizeof(inworldmenu_t) );

	InWorld_Cache();

	s_inworld.menu.wrapAround = qtrue;
	s_inworld.menu.fullscreen = qfalse;

	s_inworld.frame.generic.type			= MTYPE_BITMAP;
	s_inworld.frame.generic.flags		= QMF_INACTIVE;
	s_inworld.frame.generic.name			= MENU_FRAME;
	s_inworld.frame.generic.x			= 320-233;//142;
	s_inworld.frame.generic.y			= 240-166;//118;
	s_inworld.frame.width				= 470;//359;
	s_inworld.frame.height				= 350;//256;

	//y = 96;
	y = 125;
	
	y += MENU_VERTICAL_SPACING;
	s_inworld.resume.generic.type			= MTYPE_PTEXT;
	s_inworld.resume.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inworld.resume.generic.x			= 320;
	s_inworld.resume.generic.y			= y;
	s_inworld.resume.generic.id			= ID_RESUME;
	s_inworld.resume.generic.callback		= InWorld_Event; 
	s_inworld.resume.string				= "RESUME";
	s_inworld.resume.color				= color_white;
	s_inworld.resume.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	
	y += MENU_VERTICAL_SPACING;
	s_inworld.settings.generic.type			= MTYPE_PTEXT;
	s_inworld.settings.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inworld.settings.generic.x			= 320;
	s_inworld.settings.generic.y			= y;
	s_inworld.settings.generic.id			= ID_SETTINGS;
	s_inworld.settings.generic.callback		= InWorld_Event; 
	s_inworld.settings.string				= "SETTINGS";
	s_inworld.settings.color				= color_white;
	s_inworld.settings.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += MENU_VERTICAL_SPACING;
	s_inworld.character.generic.type		= MTYPE_PTEXT;
	s_inworld.character.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inworld.character.generic.x			= 320;
	s_inworld.character.generic.y			= y;
	s_inworld.character.generic.id			= ID_CHARACTER;
	s_inworld.character.generic.callback	= InWorld_Event; 
	s_inworld.character.string				= "CHARACTER";
	s_inworld.character.color				= color_white;
	s_inworld.character.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	
	y += MENU_VERTICAL_SPACING;
	s_inworld.camera.generic.type		= MTYPE_PTEXT;
	s_inworld.camera.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inworld.camera.generic.x			= 320;
	s_inworld.camera.generic.y			= y;
	s_inworld.camera.generic.id			= ID_CAMERA;
	s_inworld.camera.generic.callback	= InWorld_Event; 
	s_inworld.camera.string				= "CAMERA";
	s_inworld.camera.color				= color_white;
	s_inworld.camera.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += MENU_VERTICAL_SPACING;
	s_inworld.leave.generic.type			= MTYPE_PTEXT;
	s_inworld.leave.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inworld.leave.generic.x			= 320;
	s_inworld.leave.generic.y			= y;
	s_inworld.leave.generic.id			= ID_LEAVE;
	s_inworld.leave.generic.callback		= InWorld_Event; 
	s_inworld.leave.string				= "LEAVE";
	s_inworld.leave.color				= color_white;
	s_inworld.leave.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	
	y += MENU_VERTICAL_SPACING;
	s_inworld.quit.generic.type			= MTYPE_PTEXT;
	s_inworld.quit.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_inworld.quit.generic.x			= 320;
	s_inworld.quit.generic.y			= y;
	s_inworld.quit.generic.id			= ID_QUIT;
	s_inworld.quit.generic.callback		= InWorld_Event; 
	s_inworld.quit.string				= "QUIT";
	s_inworld.quit.color				= color_white;
	s_inworld.quit.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	Menu_AddItem( &s_inworld.menu, &s_inworld.frame );
	Menu_AddItem( &s_inworld.menu, &s_inworld.resume );
	Menu_AddItem( &s_inworld.menu, &s_inworld.settings );
	Menu_AddItem( &s_inworld.menu, &s_inworld.character );
	Menu_AddItem( &s_inworld.menu, &s_inworld.camera );
	Menu_AddItem( &s_inworld.menu, &s_inworld.leave );
	Menu_AddItem( &s_inworld.menu, &s_inworld.quit );
}

/*
=================
UI_InWorldMenu
=================
*/
void UI_InWorldMenu( void ) {
	// force as top level menu
	uis.menusp = 0;  

	// set menu cursor to a nice location
	uis.cursorx = 320;
	uis.cursory = 240;

	InWorld_MenuInit();
	UI_PushMenu( &s_inworld.menu );
}
