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

#define MAIN_MENU_VERTICAL_SPACING		34

int	singleXpos = 0;
int singleYpos = 0;

int	multiXpos = 64;
int multiYpos = 24;

int setupXpos = 230;
int setupYpos = 24;

int demosXpos = 0;
int demosYpos = 0;

int cinXpos = 0;
int cinYpos = 0;

int modsXpos = 390;
int modsYpos = 24;

int exitXpos = 550;
int exitYpos = 24;

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

	menubitmap_s	player;
	playerInfo_t	playerinfo;
	char			playerModel[MAX_QPATH];

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
MainMenu_DrawPlayer
=================
*/
static void MainMenu_DrawPlayer( void *self ) {
	menubitmap_s	*b;
	vec3_t			viewangles;
	char			buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );
	if ( strcmp( buf, s_main.playerModel ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_main.playerinfo, buf );
		strcpy( s_main.playerModel, buf );

		viewangles[YAW]   = 180 - 30;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		UI_PlayerInfo_SetInfo( &s_main.playerinfo, LEGS_FLY_IDLE, TORSO_FLY_IDLE, viewangles, vec3_origin, WP_NONE, qfalse );
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_main.playerinfo, uis.realtime / PLAYER_MODEL_SPEED );
}

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

	case ID_EXIT:
		UI_ConfirmMenu( "Quit ZEQ2Lite?", 0, MainMenu_ExitAction );
		break;

	case ID_MODEL:
		UI_PlayerModelMenu();
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {

}

/*
===============
Main_MenuDraw
===============
*/
static void Main_MenuDraw( void ) {
	int DBSize = 32;
	int DBXOffset = DBSize + 4;
	int DBYOffset = 3;

	// draw logo
	//UI_SetColor( NULL );
	//UI_DrawHandlePic( 0, 0, 640, 480, uis.logoShader);
	UI_MenuLogo();

	// draw dragonballs
	UI_DrawHandlePic( multiXpos - DBXOffset, multiYpos - DBYOffset, DBSize, DBSize, uis.DragonBall1Star);
	UI_DrawHandlePic( setupXpos - DBXOffset, setupYpos - DBYOffset, DBSize, DBSize, uis.DragonBall2Star);
	UI_DrawHandlePic( modsXpos - DBXOffset, modsYpos - DBYOffset, DBSize, DBSize, uis.DragonBall3Star);
	UI_DrawHandlePic( exitXpos - DBXOffset, exitYpos - DBYOffset, DBSize, DBSize, uis.DragonBall4Star);

	// standard menu drawing
	Menu_Draw( &s_main.menu );

	if (uis.demoversion) {
		UI_DrawString( 320, 400, "ZEQ2Lite (C) 2002-2009, www.zeq2.com/lite  All Rights Reserved. Teaser Build", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
	} else {
		UI_DrawString( 320, 450, "ZEQ2Lite (C) 2002-2009,  www.zeq2.com/lite  All Rights Reserved.  Beta Build", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
	}
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
	qboolean teamArena = qfalse;
	int		style = UI_LEFT | UI_DROPSHADOW | UI_SMALLFONT;

	trap_S_StopBackgroundTrack();
	trap_S_StartBackgroundTrack("music/yamamoto/menu01.ogg", "music/yamamoto/menu01.ogg");
	trap_Cvar_Set( "sv_killserver", "1" );

	// set menu cursor to a nice location
	uis.cursorx = 320;
	uis.cursory = 240;

#if MAPLENSFLARES	// JUHOX: reset g_editmode
	trap_Cvar_Set("g_editmode", "0");
#endif

	memset( &s_main, 0 ,sizeof(mainmenu_t) );

	MainMenu_Cache();

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= singleXpos;
	s_main.singleplayer.generic.y			= singleYpos;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
	s_main.singleplayer.string				= "Story";
	s_main.singleplayer.color				= color_white;
	s_main.singleplayer.style				= style;

	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= multiXpos;
	s_main.multiplayer.generic.y			= multiYpos;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent; 
	s_main.multiplayer.string				= "PLAY";
	s_main.multiplayer.color				= color_white;
	s_main.multiplayer.style				= style;

	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= setupXpos;
	s_main.setup.generic.y					= setupYpos;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_white;
	s_main.setup.style						= style;

	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= demosXpos;
	s_main.demos.generic.y					= demosYpos;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= color_white;
	s_main.demos.style						= style;

	s_main.cinematics.generic.type			= MTYPE_PTEXT;
	s_main.cinematics.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x				= cinXpos;
	s_main.cinematics.generic.y				= cinYpos;
	s_main.cinematics.generic.id			= ID_CINEMATICS;
	s_main.cinematics.generic.callback		= Main_MenuEvent; 
	s_main.cinematics.string				= "CINEMATICS";
	s_main.cinematics.color					= color_white;
	s_main.cinematics.style					= style;

	s_main.mods.generic.type			= MTYPE_PTEXT;
	s_main.mods.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x				= modsXpos;
	s_main.mods.generic.y				= modsYpos;
	s_main.mods.generic.id				= ID_MODS;
	s_main.mods.generic.callback		= Main_MenuEvent; 
	s_main.mods.string					= "MODS";
	s_main.mods.color					= color_white;
	s_main.mods.style					= style;

	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= exitXpos;
	s_main.exit.generic.y					= exitYpos;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_white;
	s_main.exit.style						= style;

	s_main.player.generic.type				= MTYPE_BITMAP;
	s_main.player.generic.flags				= QMF_SILENT;
	s_main.player.generic.ownerdraw			= MainMenu_DrawPlayer;
	s_main.player.generic.id				= ID_MODEL;
	s_main.player.generic.callback			= Main_MenuEvent;
	s_main.player.generic.x					= 400;
	s_main.player.generic.y					= 0;
	s_main.player.width						= 32*10;
	s_main.player.height					= 56*10;

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

//	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );

//	Menu_AddItem( &s_main.menu,	&s_main.demos );
//	Menu_AddItem( &s_main.menu,	&s_main.cinematics );

	Menu_AddItem( &s_main.menu,	&s_main.mods );
	Menu_AddItem( &s_main.menu,	&s_main.exit );

//	Menu_AddItem( &s_main.menu, &s_main.player );
//	Menu_AddItem( &s_main.menu, &s_main.player1 );
//	Menu_AddItem( &s_main.menu, &s_main.player2 );

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
}
