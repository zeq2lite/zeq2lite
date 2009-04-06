// Copyright (C) 1999-2000 Id Software, Inc.
//

/*
=======================================================================

MUSIC MENU

=======================================================================
*/

#include "ui_local.h"

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"	
#define ART_GO0				"menu/art/play_0"
#define ART_GO1				"menu/art/play_1"
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_ARROWS			"menu/art/arrows_horz_0"
#define ART_ARROWLEFT		"menu/art/arrows_horz_left"
#define ART_ARROWRIGHT		"menu/art/arrows_horz_right"

#define MAX_MUSIC			128
#define NAMEBUFSIZE			( MAX_MUSIC * 16 )

#define ID_BACK				29
#define ID_GO				30
#define ID_LIST				31
#define ID_RIGHT			32
#define ID_LEFT				33

#define ARROWS_WIDTH		128
#define ARROWS_HEIGHT		48


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		list;

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menubitmap_s	back;
	menubitmap_s	go;

	int				numMusic;
	char			names[NAMEBUFSIZE];
	char			*musiclist[MAX_MUSIC];
} music_t;

static music_t	s_music;


/*
===============
Music_MenuEvent
===============
*/
static void Music_MenuEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
//		UI_ForceMenuOff ();
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "music music/%s\n",
								s_music.list.itemnames[s_music.list.curvalue]) );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_LEFT:
		ScrollList_Key( &s_music.list, K_LEFTARROW );
		break;

	case ID_RIGHT:
		ScrollList_Key( &s_music.list, K_RIGHTARROW );
		break;
	}
}


/*
=================
UI_MusicMenu_Key
=================
*/
static sfxHandle_t UI_MusicMenu_Key( int key ) {
	menucommon_s	*item;

	item = Menu_ItemAtCursor( &s_music.menu );

	return Menu_DefaultKey( &s_music.menu, key );
}


/*
===============
Music_MenuInit
===============
*/
static void Music_MenuInit( void ) {
	int		i;
	int		len;
	char	*musicname, extension[32];

	memset( &s_music, 0 ,sizeof(music_t) );
	s_music.menu.key = UI_MusicMenu_Key;

	Music_Cache();

	s_music.menu.fullscreen = qfalse;
	s_music.menu.wrapAround = qtrue;

	s_music.banner.generic.type		= MTYPE_BTEXT;
	s_music.banner.generic.x		= 320;
	s_music.banner.generic.y		= 16;
	s_music.banner.string			= "MUSIC";
	s_music.banner.color			= color_white;
	s_music.banner.style			= UI_CENTER|UI_DROPSHADOW;

	s_music.framel.generic.type		= MTYPE_BITMAP;
	s_music.framel.generic.name		= ART_FRAMEL;
	s_music.framel.generic.flags	= QMF_INACTIVE;
	s_music.framel.generic.x		= 0;  
	s_music.framel.generic.y		= 78;
	s_music.framel.width			= 256;
	s_music.framel.height			= 329;

	s_music.framer.generic.type		= MTYPE_BITMAP;
	s_music.framer.generic.name		= ART_FRAMER;
	s_music.framer.generic.flags	= QMF_INACTIVE;
	s_music.framer.generic.x		= 376;
	s_music.framer.generic.y		= 46;
	s_music.framer.width			= 256;
	s_music.framer.height			= 334;

	s_music.arrows.generic.type		= MTYPE_BITMAP;
	s_music.arrows.generic.name		= ART_ARROWS;
	s_music.arrows.generic.flags	= QMF_INACTIVE;
	s_music.arrows.generic.x		= 315-ARROWS_WIDTH/2;
	s_music.arrows.generic.y		= 343;
	s_music.arrows.width			= ARROWS_WIDTH;
	s_music.arrows.height			= ARROWS_HEIGHT;

	s_music.left.generic.type		= MTYPE_BITMAP;
	s_music.left.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_music.left.generic.x			= 315-ARROWS_WIDTH/2;
	s_music.left.generic.y			= 343;
	s_music.left.generic.id			= ID_LEFT;
	s_music.left.generic.callback	= Music_MenuEvent;
	s_music.left.width				= ARROWS_WIDTH/2;
	s_music.left.height				= ARROWS_HEIGHT;
	s_music.left.focuspic			= ART_ARROWLEFT;

	s_music.right.generic.type		= MTYPE_BITMAP;
	s_music.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_music.right.generic.x			= 315;
	s_music.right.generic.y			= 343;
	s_music.right.generic.id		= ID_RIGHT;
	s_music.right.generic.callback	= Music_MenuEvent;
	s_music.right.width				= ARROWS_WIDTH/2;
	s_music.right.height			= ARROWS_HEIGHT;
	s_music.right.focuspic			= ART_ARROWRIGHT;

	s_music.back.generic.type		= MTYPE_BITMAP;
	s_music.back.generic.name		= ART_BACK0;
	s_music.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_music.back.generic.id			= ID_BACK;
	s_music.back.generic.callback	= Music_MenuEvent;
	s_music.back.generic.x			= 103;
	s_music.back.generic.y			= 330;
	s_music.back.width				= 128;
	s_music.back.height				= 64;
	s_music.back.focuspic			= ART_BACK1;

	s_music.go.generic.type			= MTYPE_BITMAP;
	s_music.go.generic.name			= ART_GO0;
	s_music.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_music.go.generic.id			= ID_GO;
	s_music.go.generic.callback		= Music_MenuEvent;
	s_music.go.generic.x			= 541;
	s_music.go.generic.y			= 330;
	s_music.go.width				= 128;
	s_music.go.height				= 64;
	s_music.go.focuspic				= ART_GO1;

	s_music.list.generic.type		= MTYPE_SCROLLLIST;
	s_music.list.generic.flags		= QMF_PULSEIFFOCUS;
	s_music.list.generic.callback	= Music_MenuEvent;
	s_music.list.generic.id			= ID_LIST;
	s_music.list.generic.x			= 10;//118;
	s_music.list.generic.y			= 100;
	s_music.list.width				= 16;
	s_music.list.height				= 14;
	s_music.list.numitems			= trap_FS_GetFileList( "music", "ogg", s_music.names, NAMEBUFSIZE );
	s_music.list.itemnames			= (const char **)s_music.musiclist;
	s_music.list.columns			= 4;//3;

	if (!s_music.list.numitems) {
		strcpy( s_music.names, "No Music Found." );
		s_music.list.numitems = 1;

		//degenerate case, not selectable
		s_music.go.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else if (s_music.list.numitems > MAX_MUSIC)
		s_music.list.numitems = MAX_MUSIC;

	musicname = s_music.names;
	for ( i = 0; i < s_music.list.numitems; i++ ) {
		s_music.list.itemnames[i] = musicname;
		
		// strip extension
		len = strlen( musicname );
		if (!Q_stricmp(musicname +  len - 4,".ogg"))
			musicname[len-4] = '\0';

		Q_strupr(musicname);

		musicname += len + 1;
	}

	Menu_AddItem( &s_music.menu, &s_music.banner );
	Menu_AddItem( &s_music.menu, &s_music.framel );
	Menu_AddItem( &s_music.menu, &s_music.framer );
	Menu_AddItem( &s_music.menu, &s_music.list );
	Menu_AddItem( &s_music.menu, &s_music.arrows );
	Menu_AddItem( &s_music.menu, &s_music.left );
	Menu_AddItem( &s_music.menu, &s_music.right );
	Menu_AddItem( &s_music.menu, &s_music.back );
	Menu_AddItem( &s_music.menu, &s_music.go );
}

/*
=================
Music_Cache
=================
*/
void Music_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_GO0 );
	trap_R_RegisterShaderNoMip( ART_GO1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWLEFT );
	trap_R_RegisterShaderNoMip( ART_ARROWRIGHT );
}

/*
===============
UI_MusicMenu
===============
*/
void UI_MusicMenu( void ) {
	Music_MenuInit();
	UI_PushMenu( &s_music.menu );
}

/*
=======================================================================

INGAME MENU

=======================================================================
*/


#define INGAME_FRAME					"menu/art/addbotframe"
//#define INGAME_FRAME					"menu/art/cut_frame"
#define INGAME_MENU_VERTICAL_SPACING	28

#define ID_TEAM					10
#define ID_ADDBOTS				11
#define ID_REMOVEBOTS			12
#define ID_SETUP				13
#define ID_SERVERINFO			14
#define ID_LEAVEARENA			15
#define ID_RESTART				16
#define ID_QUIT					17
#define ID_RESUME				18
#define ID_TEAMORDERS			19
#define ID_MUSIC				20


typedef struct {
	menuframework_s	menu;

	menubitmap_s	frame;
	menutext_s		team;
	menutext_s		setup;
	menutext_s		server;
	menutext_s		leave;
	menutext_s		restart;
	menutext_s		addbots;
	menutext_s		removebots;
	menutext_s		teamorders;
	menutext_s		music;
	menutext_s		quit;
	menutext_s		resume;
} ingamemenu_t;

static ingamemenu_t	s_ingame;


/*
=================
InGame_RestartAction
=================
*/
static void InGame_RestartAction( qboolean result ) {
	if( !result ) {
		return;
	}

	UI_PopMenu();
	trap_Cmd_ExecuteText( EXEC_APPEND, "map_restart 0\n" );
}


/*
=================
InGame_QuitAction
=================
*/
static void InGame_QuitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}


/*
=================
InGame_Event
=================
*/
// bk001205 - for LCC
typedef void (*voidfunc_f)(void);

void InGame_Event( void *ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_TEAM:
		UI_TeamMainMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_LEAVEARENA:
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		break;

	case ID_RESTART:
		UI_ConfirmMenu( "RESTART ARENA?", 0, InGame_RestartAction );
		break;

	case ID_QUIT:
		UI_ConfirmMenu( "EXIT GAME?",  0, InGame_QuitAction );
		break;

	case ID_SERVERINFO:
		UI_ServerInfoMenu();
		break;

	case ID_ADDBOTS:
		UI_AddBotsMenu();
		break;

	case ID_REMOVEBOTS:
		UI_RemoveBotsMenu();
		break;

	case ID_TEAMORDERS:
		UI_TeamOrdersMenu();
		break;

	case ID_MUSIC:
		UI_MusicMenu();
		break;

	case ID_RESUME:
		UI_PopMenu();
		break;
	}
}


/*
=================
InGame_MenuInit
=================
*/
void InGame_MenuInit( void ) {
	int		y;
	uiClientState_t	cs;
	char	info[MAX_INFO_STRING];
	int		team;

	memset( &s_ingame, 0 ,sizeof(ingamemenu_t) );

	InGame_Cache();

	s_ingame.menu.wrapAround = qtrue;
	s_ingame.menu.fullscreen = qfalse;

	s_ingame.frame.generic.type			= MTYPE_BITMAP;
	s_ingame.frame.generic.flags		= QMF_INACTIVE;
	s_ingame.frame.generic.name			= INGAME_FRAME;
	s_ingame.frame.generic.x			= 320-233;//142;
	s_ingame.frame.generic.y			= 240-166;//118;
	s_ingame.frame.width				= 470;//359;
	s_ingame.frame.height				= 350;//256;

	//y = 96;
	y = 88;
	s_ingame.team.generic.type			= MTYPE_PTEXT;
	s_ingame.team.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.team.generic.x				= 320;
	s_ingame.team.generic.y				= y;
	s_ingame.team.generic.id			= ID_TEAM;
	s_ingame.team.generic.callback		= InGame_Event; 
	s_ingame.team.string				= "START";
	s_ingame.team.color					= color_white;
	s_ingame.team.style					= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.addbots.generic.type		= MTYPE_PTEXT;
	s_ingame.addbots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.addbots.generic.x			= 320;
	s_ingame.addbots.generic.y			= y;
	s_ingame.addbots.generic.id			= ID_ADDBOTS;
	s_ingame.addbots.generic.callback	= InGame_Event; 
	s_ingame.addbots.string				= "ADD BOTS";
	s_ingame.addbots.color				= color_white;
	s_ingame.addbots.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" ) || (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)) {
		s_ingame.addbots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.removebots.generic.type		= MTYPE_PTEXT;
	s_ingame.removebots.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.removebots.generic.x			= 320;
	s_ingame.removebots.generic.y			= y;
	s_ingame.removebots.generic.id			= ID_REMOVEBOTS;
	s_ingame.removebots.generic.callback	= InGame_Event; 
	s_ingame.removebots.string				= "REMOVE BOTS";
	s_ingame.removebots.color				= color_white;
	s_ingame.removebots.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	if( !trap_Cvar_VariableValue( "sv_running" ) || !trap_Cvar_VariableValue( "bot_enable" ) || (trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER)) {
		s_ingame.removebots.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.teamorders.generic.type		= MTYPE_PTEXT;
	s_ingame.teamorders.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.teamorders.generic.x			= 320;
	s_ingame.teamorders.generic.y			= y;
	s_ingame.teamorders.generic.id			= ID_TEAMORDERS;
	s_ingame.teamorders.generic.callback	= InGame_Event; 
	s_ingame.teamorders.string				= "TEAM ORDERS";
	s_ingame.teamorders.color				= color_white;
	s_ingame.teamorders.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	if( !(trap_Cvar_VariableValue( "g_gametype" ) >= GT_TEAM) ) {
		s_ingame.teamorders.generic.flags |= QMF_GRAYED;
	}
	else {
		trap_GetClientState( &cs );
		trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
		team = atoi( Info_ValueForKey( info, "t" ) );
		if( team == TEAM_SPECTATOR ) {
			s_ingame.teamorders.generic.flags |= QMF_GRAYED;
		}
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.setup.generic.type			= MTYPE_PTEXT;
	s_ingame.setup.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.setup.generic.x			= 320;
	s_ingame.setup.generic.y			= y;
	s_ingame.setup.generic.id			= ID_SETUP;
	s_ingame.setup.generic.callback		= InGame_Event; 
	s_ingame.setup.string				= "SETUP";
	s_ingame.setup.color				= color_white;
	s_ingame.setup.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.music.generic.type			= MTYPE_PTEXT;
	s_ingame.music.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.music.generic.x			= 320;
	s_ingame.music.generic.y			= y;
	s_ingame.music.generic.id			= ID_MUSIC;
	s_ingame.music.generic.callback		= InGame_Event; 
	s_ingame.music.string				= "MUSIC";
	s_ingame.music.color				= color_white;
	s_ingame.music.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.server.generic.type		= MTYPE_PTEXT;
	s_ingame.server.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.server.generic.x			= 320;
	s_ingame.server.generic.y			= y;
	s_ingame.server.generic.id			= ID_SERVERINFO;
	s_ingame.server.generic.callback	= InGame_Event; 
	s_ingame.server.string				= "SERVER INFO";
	s_ingame.server.color				= color_white;
	s_ingame.server.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.restart.generic.type		= MTYPE_PTEXT;
	s_ingame.restart.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.restart.generic.x			= 320;
	s_ingame.restart.generic.y			= y;
	s_ingame.restart.generic.id			= ID_RESTART;
	s_ingame.restart.generic.callback	= InGame_Event; 
	s_ingame.restart.string				= "RESTART ARENA";
	s_ingame.restart.color				= color_white;
	s_ingame.restart.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;
	if( !trap_Cvar_VariableValue( "sv_running" ) ) {
		s_ingame.restart.generic.flags |= QMF_GRAYED;
	}

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.resume.generic.type			= MTYPE_PTEXT;
	s_ingame.resume.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.resume.generic.x				= 320;
	s_ingame.resume.generic.y				= y;
	s_ingame.resume.generic.id				= ID_RESUME;
	s_ingame.resume.generic.callback		= InGame_Event; 
	s_ingame.resume.string					= "RESUME GAME";
	s_ingame.resume.color					= color_white;
	s_ingame.resume.style					= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.leave.generic.type			= MTYPE_PTEXT;
	s_ingame.leave.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.leave.generic.x			= 320;
	s_ingame.leave.generic.y			= y;
	s_ingame.leave.generic.id			= ID_LEAVEARENA;
	s_ingame.leave.generic.callback		= InGame_Event; 
	s_ingame.leave.string				= "LEAVE ARENA";
	s_ingame.leave.color				= color_white;
	s_ingame.leave.style				= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	y += INGAME_MENU_VERTICAL_SPACING;
	s_ingame.quit.generic.type			= MTYPE_PTEXT;
	s_ingame.quit.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_ingame.quit.generic.x				= 320;
	s_ingame.quit.generic.y				= y;
	s_ingame.quit.generic.id			= ID_QUIT;
	s_ingame.quit.generic.callback		= InGame_Event; 
	s_ingame.quit.string				= "EXIT GAME";
	s_ingame.quit.color					= color_white;
	s_ingame.quit.style					= UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW;

	Menu_AddItem( &s_ingame.menu, &s_ingame.frame );
	Menu_AddItem( &s_ingame.menu, &s_ingame.team );
	Menu_AddItem( &s_ingame.menu, &s_ingame.addbots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.removebots );
	Menu_AddItem( &s_ingame.menu, &s_ingame.teamorders );
	Menu_AddItem( &s_ingame.menu, &s_ingame.setup );
	Menu_AddItem( &s_ingame.menu, &s_ingame.music );
	Menu_AddItem( &s_ingame.menu, &s_ingame.server );
	Menu_AddItem( &s_ingame.menu, &s_ingame.restart );
	Menu_AddItem( &s_ingame.menu, &s_ingame.resume );
	Menu_AddItem( &s_ingame.menu, &s_ingame.leave );
	Menu_AddItem( &s_ingame.menu, &s_ingame.quit );
}


/*
=================
InGame_Cache
=================
*/
void InGame_Cache( void ) {
	trap_R_RegisterShaderNoMip( INGAME_FRAME );
}


/*
=================
UI_InGameMenu
=================
*/
void UI_InGameMenu( void ) {
	// force as top level menu
	uis.menusp = 0;  

	// set menu cursor to a nice location
	uis.cursorx = 320;
	uis.cursory = 240;

	InGame_MenuInit();
	UI_PushMenu( &s_ingame.menu );
}
