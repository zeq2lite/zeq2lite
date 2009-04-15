// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER			10
#define ID_MULTIPLAYER			11
#define ID_SETUP				12
#define ID_DEMOS				13
#define ID_CINEMATICS			14
#define ID_TEAMARENA			15
#define ID_MODS					16
#define ID_EXIT					17

#define	ID_MODEL				18
#define MAX_NAMELENGTH			20

//#define MAIN_BANNER_MODEL				"models/mapobjects/banner/banner5.md3"
#define MAIN_MENU_VERTICAL_SPACING		34

typedef struct {
	menuframework_s	menu;

	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		cinematics;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		exit;
	qhandle_t		bannerModel;

	menubitmap_s	player1;
	playerInfo_t	playerinfo1;
	char			playerModel1[MAX_QPATH];

	menubitmap_s	player2;
	playerInfo_t	playerinfo2;
	char			playerModel2[MAX_QPATH];
} mainmenu_t;

static mainmenu_t s_main;

/*
=================
MainMenu_DrawPlayer1
=================
*/
static void MainMenu_DrawPlayer1( void *self ) {
	menubitmap_s	*b;
	vec3_t			viewangles;
	int				r;

	if ( strcmp( "vegeta", s_main.playerModel1 ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_main.playerinfo1, "vegeta");
		strcpy( s_main.playerModel1, "vegeta");

		r = random() * 4;

		viewangles[YAW]   = 90;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		UI_PlayerInfo_SetInfo( &s_main.playerinfo1, LEGS_SPEED_MELEE_ATTACK + r, TORSO_SPEED_MELEE_ATTACK + r, viewangles, vec3_origin, WP_NONE, qfalse );
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_main.playerinfo1, uis.realtime / PLAYER_MODEL_SPEED );
}

/*
=================
MainMenu_DrawPlayer2
=================
*/
static void MainMenu_DrawPlayer2( void *self ) {
	menubitmap_s	*b;
	vec3_t			viewangles;
	int				r;

	if ( strcmp( "goku", s_main.playerModel2 ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_main.playerinfo2, "goku");
		strcpy( s_main.playerModel2, "goku");

		r = random() * 4;

		viewangles[YAW]   = -90;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		UI_PlayerInfo_SetInfo( &s_main.playerinfo2, LEGS_SPEED_MELEE_ATTACK + r, TORSO_SPEED_MELEE_ATTACK + r, viewangles, vec3_origin, WP_NONE, qfalse );
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_main.playerinfo2, uis.realtime / PLAYER_MODEL_SPEED );
}

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SINGLEPLAYER:
		UI_SPLevelMenu();
		break;

	case ID_MULTIPLAYER:
		UI_ArenaServersMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_CINEMATICS:
		UI_CinematicsMenu();
		break;

	case ID_MODS:
		UI_ModsMenu();
		break;
/*
	case ID_TEAMARENA:
		trap_Cvar_Set( "fs_game", "missionpack");
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;
*/
	case ID_EXIT:
		UI_ConfirmMenu( "Quit already?", 0, MainMenu_ExitAction );
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
//	s_main.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
}


/*
===============
Main_MenuDraw
===============
*/
static void Main_MenuDraw( void ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			adjust;
	float			x, y, w, h;
//	vec4_t			color = {0.5, 0, 0, 1};

	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 0;
	y = 0;
	w = 640;
	h = 480;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	adjust = 0; // JDC: Kenneth asked me to stop this 1.0 * sin( (float)uis.realtime / 1000 );
	refdef.fov_x = 20 + adjust;
	refdef.fov_y = 15 + adjust;

	refdef.time = uis.realtime;

	origin[0] = 640; //z depth
	origin[1] = 0; //x depth
	origin[2] = 0; //y depth

	trap_R_ClearScene();

	// add the model
/*
	memset( &ent, 0, sizeof(ent) );

	adjust = 0.0 * sin( (float)uis.realtime / 5000 );
	VectorSet( angles, 0, 180 + adjust, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = s_main.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );

	// standard menu drawing
*/
	Menu_Draw( &s_main.menu );

	if (uis.demoversion) {
//		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "ZEQ II (C) 2002-2004, www.zeq2.com  All Rights Reserved. Teaser Build", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
	} else {
		UI_DrawString( 320, 450, "ZEQ II (C) 2002-2004, www.zeq2.com  All Rights Reserved. Pre-Alpha Build", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
	}
}


/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0) {
			return qtrue;
		}
    dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style = UI_LEFT | UI_DROPSHADOW | UI_SMALLFONT;
	
	trap_S_StartBackgroundTrack("music/general01.ogg", "music/general01.ogg");
	trap_Cvar_Set( "sv_killserver", "1" );

	// set menu cursor to a nice location
	uis.cursorx = 320;
	uis.cursory = 240;
/*
	if( !uis.demoversion && !ui_cdkeychecked.integer ) {
		char	key[17];

		trap_GetCDKey( key, sizeof(key) );
		if( trap_VerifyCDKey( key, NULL ) == qfalse ) {
			UI_CDKeyMenu();
			return;
		}
	}
*/

#if MAPLENSFLARES	// JUHOX: reset g_editmode
	trap_Cvar_Set("g_editmode", "0");
#endif

	memset( &s_main, 0 ,sizeof(mainmenu_t) );

	MainMenu_Cache();

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	y = 24;
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= 24;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
	s_main.singleplayer.string				= "Story";
	s_main.singleplayer.color				= color_white;
	s_main.singleplayer.style				= style;

	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= 120;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent; 
	s_main.multiplayer.string				= "MULTIPLAYER";
	s_main.multiplayer.color				= color_white;
	s_main.multiplayer.style				= style;

	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= 292;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_white;
	s_main.setup.style						= style;

	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= 375;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= color_white;
	s_main.demos.style						= style;

	s_main.cinematics.generic.type			= MTYPE_PTEXT;
	s_main.cinematics.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x				= 470;
	s_main.cinematics.generic.y				= y;
	s_main.cinematics.generic.id			= ID_CINEMATICS;
	s_main.cinematics.generic.callback		= Main_MenuEvent; 
	s_main.cinematics.string				= "CINEMATICS";
	s_main.cinematics.color					= color_white;
	s_main.cinematics.style					= style;

//	if (UI_TeamArenaExists()) {
//		teamArena = qtrue;
//		y += MAIN_MENU_VERTICAL_SPACING;
//		s_main.teamArena.generic.type			= MTYPE_PTEXT;
//		s_main.teamArena.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
//		s_main.teamArena.generic.x				= 20;
//		s_main.teamArena.generic.y				= y;
//		s_main.teamArena.generic.id				= ID_TEAMARENA;
//		s_main.teamArena.generic.callback		= Main_MenuEvent; 
//		s_main.teamArena.string					= "MOD RESTART";
//		s_main.teamArena.color					= color_white;
//		s_main.teamArena.style					= style;
//	}

	s_main.mods.generic.type			= MTYPE_PTEXT;
	s_main.mods.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x				= 24;
	s_main.mods.generic.y				= 420;
	s_main.mods.generic.id				= ID_MODS;
	s_main.mods.generic.callback		= Main_MenuEvent; 
	s_main.mods.string					= "MODS";
	s_main.mods.color					= color_white;
	s_main.mods.style					= style;

	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= 560;
	s_main.exit.generic.y					= 420;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_white;
	s_main.exit.style						= style;

	s_main.player1.generic.type				= MTYPE_BITMAP;
	s_main.player1.generic.flags			= QMF_INACTIVE;
	s_main.player1.generic.ownerdraw		= MainMenu_DrawPlayer1;
	s_main.player1.generic.x				= 225;
	s_main.player1.generic.y				= 0;
	s_main.player1.width					= 32*10;
	s_main.player1.height					= 56*10;

	s_main.player2.generic.type				= MTYPE_BITMAP;
	s_main.player2.generic.flags			= QMF_INACTIVE;
	s_main.player2.generic.ownerdraw		= MainMenu_DrawPlayer2;
	s_main.player2.generic.x				= 75;
	s_main.player2.generic.y				= 0;
	s_main.player2.width					= 32*10;
	s_main.player2.height					= 56*10;

	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.cinematics );
//	if (teamArena) {
//		Menu_AddItem( &s_main.menu,	&s_main.teamArena );
//	}
	Menu_AddItem( &s_main.menu,	&s_main.mods );
	Menu_AddItem( &s_main.menu,	&s_main.exit );

	Menu_AddItem( &s_main.menu, &s_main.player1 );
	Menu_AddItem( &s_main.menu, &s_main.player2 );

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
}