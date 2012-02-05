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
=============================================================================

START SERVER MENU *****

=============================================================================
*/


#include "ui_local.h"


#define GAMESERVER_BACK0		"interface/art/back_0"
#define GAMESERVER_BACK1		"interface/art/back_1"
#define GAMESERVER_NEXT0		"interface/art/next_0"
#define GAMESERVER_NEXT1		"interface/art/next_1"
#define GAMESERVER_FRAMEL		"interface/art/frame2_l"
#define GAMESERVER_FRAMER		"interface/art/frame1_r"
#define GAMESERVER_SELECT		"interface/art/maps_select"
#define GAMESERVER_SELECTED		"interface/art/maps_selected"
#define GAMESERVER_FIGHT0		"interface/art/fight_0"
#define GAMESERVER_FIGHT1		"interface/art/fight_1"
#define GAMESERVER_UNKNOWNMAP	"interface/art/unknownmap"
#define GAMESERVER_ARROWSL		"interface/art/gs_arrows_l"
#define GAMESERVER_ARROWSR		"interface/art/gs_arrows_r"

#define MAX_MAPCOLS		2
#define MAX_MAPSPERPAGE	2

#define MAX_NAMELENGTH	16
#define ID_GAMETYPE				10
#define ID_PICTURES				11	// and 12
#define ID_PREVPAGE				13
#define ID_NEXTPAGE				14
#define ID_STARTSERVERBACK		15
#define ID_STARTSERVERNEXT		16
#define ID_DEDICATED			17


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		gametype;
	menubitmap_s	mappics[MAX_MAPSPERPAGE];
	menubitmap_s	mapbuttons[MAX_MAPSPERPAGE];
	menubitmap_s	prevpage;
	menubitmap_s	nextpage;
	menutext_s		back;
	menutext_s		next;

	menutext_s		mapname;
	menubitmap_s	item_null;

	menulist_s		dedicated;
	menufield_s		powerlevel;
	menufield_s		powerlevelMaximum;
	menufield_s		breakLimitRate;
	menufield_s		hostname;	

	int				currentmap;
	int				nummaps;
	int				page;
	int				maxpages;
	int 			maplist[MAX_ARENAS];
} startserver_t;

static startserver_t s_startserver;

static const char *dedicated_list[] = {
	"No",
	"LAN",
	"Internet",
	NULL
};

static const char *gametype_items[] = {
	"Struggle",
	NULL
};

static int gametype_remap[] = {GT_FFA, GT_TEAM, GT_STRUGGLE,GT_TOURNAMENT, GT_CTF};
static int gametype_remap2[] = {0, 2, 0, 1, 3};


/*
=================
GametypeBits
=================
*/
static int GametypeBits( char *string ) {
	int		bits;
	char	*p;
	char	*token;

	bits = 0;
	p = string;
	while( 1 ) {
		token = COM_ParseExt( &p, qfalse );
		if( token[0] == 0 ) {
			break;
		}

		if( Q_stricmp( token, "ffa" ) == 0 ) {
			bits |= 1 << GT_FFA;
			continue;
		}

		if( Q_stricmp( token, "tourney" ) == 0 ) {
			bits |= 1 << GT_TOURNAMENT;
			continue;
		}
		if( Q_stricmp( token, "struggle" ) == 0 ) {
			bits |= 1 << GT_STRUGGLE;
			continue;
		}
		if( Q_stricmp( token, "single" ) == 0 ) {
			bits |= 1 << GT_SINGLE_PLAYER;
			continue;
		}

		if( Q_stricmp( token, "team" ) == 0 ) {
			bits |= 1 << GT_TEAM;
			continue;
		}

		if( Q_stricmp( token, "ctf" ) == 0 ) {
			bits |= 1 << GT_CTF;
			continue;
		}
	}

	return bits;
}

/*
=================
StartServer_Start
=================
*/
static void StartServer_Start( void ) {
	int		powerlevel;
	int 	powerlevelMaximum;
	int		breakLimitRate;
	int		dedicated;
	int		n;
	char	buf[64];
	const char *info;

	powerlevel	 = atoi( s_startserver.powerlevel.field.buffer );
	powerlevelMaximum = atoi( s_startserver.powerlevelMaximum.field.buffer );
	breakLimitRate	 = atoi( s_startserver.breakLimitRate.field.buffer );
	dedicated	 = s_startserver.dedicated.curvalue;

	trap_Cvar_SetValue( "ui_ffa_powerlevel", powerlevel );
	trap_Cvar_SetValue( "ui_ffa_powerlevelMaximum", powerlevelMaximum );
	trap_Cvar_SetValue( "ui_ffa_breakLimitRate", breakLimitRate );
	
	
#if MAPLENSFLARES	// JUHOX: reset edit mode
	trap_Cvar_SetValue("g_editmode", 0);
#endif
	trap_Cvar_SetValue( "sv_maxclients", 64);
	trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, dedicated ) );
	trap_Cvar_SetValue ("g_powerlevel", Com_Clamp( 1, 32767, powerlevel ) );
	trap_Cvar_SetValue ("g_powerlevelMaximum", Com_Clamp( 1, 32767, powerlevelMaximum ) );
	trap_Cvar_SetValue ("g_breakLimitRate", Com_Clamp( 1, 100, breakLimitRate ) );
	trap_Cvar_Set("sv_hostname", s_startserver.hostname.field.buffer );

	// the wait commands will allow the dedicated to take effect
	info = UI_GetArenaInfoByNumber( s_startserver.maplist[ s_startserver.currentmap ]);
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", Info_ValueForKey( info, "map" )));
}

/*
=================
StartServer_Update
=================
*/
static void StartServer_Update( void ) {
	int				i;
	int				top;
	static	char	picname[MAX_MAPSPERPAGE][64];
	const char		*info;
	char			mapname[MAX_NAMELENGTH];

	top = s_startserver.page*MAX_MAPSPERPAGE;

	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		if (top+i >= s_startserver.nummaps)
			break;
		
		info = UI_GetArenaInfoByNumber( s_startserver.maplist[ top + i ]);
		Q_strncpyz( mapname, Info_ValueForKey( info, "map"), MAX_NAMELENGTH );
		Q_strupr( mapname );

		Com_sprintf( picname[i], sizeof(picname[i]), "maps/%s", mapname );

		s_startserver.mappics[i].generic.flags &= ~QMF_HIGHLIGHT;
		s_startserver.mappics[i].generic.name   = picname[i];
		s_startserver.mappics[i].shader         = 0;

		// reset
		s_startserver.mapbuttons[i].generic.flags |= QMF_PULSEIFFOCUS;
		s_startserver.mapbuttons[i].generic.flags &= ~QMF_INACTIVE;
	}

	for (; i<MAX_MAPSPERPAGE; i++)
	{
		s_startserver.mappics[i].generic.flags &= ~QMF_HIGHLIGHT;
		s_startserver.mappics[i].generic.name   = NULL;
		s_startserver.mappics[i].shader         = 0;

		// disable
		s_startserver.mapbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		s_startserver.mapbuttons[i].generic.flags |= QMF_INACTIVE;
	}


	// no servers to start
	if( !s_startserver.nummaps ) {
		s_startserver.next.generic.flags |= QMF_INACTIVE;

		// set the map name
		strcpy( s_startserver.mapname.string, "NO MAPS FOUND" );
	}
	else {
		// set the highlight
		s_startserver.next.generic.flags &= ~QMF_INACTIVE;
		i = s_startserver.currentmap - top;
		if ( i >=0 && i < MAX_MAPSPERPAGE ) 
		{
			s_startserver.mappics[i].generic.flags    |= QMF_HIGHLIGHT;
			s_startserver.mapbuttons[i].generic.flags &= ~QMF_PULSEIFFOCUS;
		}

		// set the map name
		info = UI_GetArenaInfoByNumber( s_startserver.maplist[ s_startserver.currentmap ]);
		Q_strncpyz( s_startserver.mapname.string, Info_ValueForKey( info, "map" ), MAX_NAMELENGTH);
	}
	
	Q_strupr( s_startserver.mapname.string );
}


/*
=================
StartServer_MapEvent
=================
*/
static void StartServer_MapEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED) {
		return;
	}

	s_startserver.currentmap = (s_startserver.page*MAX_MAPSPERPAGE) + (((menucommon_s*)ptr)->id - ID_PICTURES);
	StartServer_Update();
}


/*
=================
StartServer_GametypeEvent
=================
*/
static void StartServer_GametypeEvent( void* ptr, int event ) {
	int			i;
	int			count;
	int			gamebits;
	int			matchbits;
	const char	*info;

	if( event != QM_ACTIVATED) {
		return;
	}

	count = UI_GetNumArenas();
	s_startserver.nummaps = 0;
	matchbits = 1 << gametype_remap[s_startserver.gametype.curvalue];
	if( gametype_remap[s_startserver.gametype.curvalue] == GT_FFA ) {
		matchbits |= ( 1 << GT_SINGLE_PLAYER );
	}
	for( i = 0; i < count; i++ ) {
		info = UI_GetArenaInfoByNumber( i );
	
		gamebits = GametypeBits( Info_ValueForKey( info, "type") );
		if( !( gamebits & matchbits ) ) {
			continue;
		}

		s_startserver.maplist[ s_startserver.nummaps ] = i;
		s_startserver.nummaps++;
	}
	s_startserver.maxpages = (s_startserver.nummaps + MAX_MAPSPERPAGE-1)/MAX_MAPSPERPAGE;
	s_startserver.page = 0;
	s_startserver.currentmap = 0;

	StartServer_Update();
}


/*
=================
StartServer_MenuEvent
=================
*/
static void StartServer_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_PREVPAGE:
		if( s_startserver.page > 0 ) {
			s_startserver.page--;
			StartServer_Update();
		}
		break;

	case ID_NEXTPAGE:
		if( s_startserver.page < s_startserver.maxpages - 1 ) {
			s_startserver.page++;
			StartServer_Update();
		}
		break;

	case ID_STARTSERVERNEXT:
		trap_Cvar_SetValue( "g_gameType", gametype_remap[s_startserver.gametype.curvalue] );
		StartServer_Start();
		break;

	case ID_STARTSERVERBACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
StartServer_LevelshotDraw
===============
*/
static void StartServer_LevelshotDraw( void *self ) {
	menubitmap_s	*b;
	int				x;
	int				y;
	int				w;
	int				h;
	int				n;
	const char		*info;

	b = (menubitmap_s *)self;

	if( !b->generic.name ) {
		return;
	}

	if( b->generic.name && !b->shader ) {
		b->shader = trap_R_RegisterShaderNoMip( b->generic.name );
		if( !b->shader && b->errorpic ) {
			b->shader = trap_R_RegisterShaderNoMip( b->errorpic );
		}
	}

	if( b->focuspic && !b->focusshader ) {
		b->focusshader = trap_R_RegisterShaderNoMip( b->focuspic );
	}

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height;
	if( b->shader ) {
		UI_DrawHandlePic( x, y, w, h, b->shader );
	}

	x = b->generic.x;
	y = b->generic.y + b->height;
	UI_FillRect( x, y, b->width, 28, colorBlack );

	x += b->width / 2;
	y += 4;
	n = s_startserver.page * MAX_MAPSPERPAGE + b->generic.id - ID_PICTURES;

	info = UI_GetArenaInfoByNumber( s_startserver.maplist[ n ]);
	UI_DrawString( x, y, Info_ValueForKey( info, "map" ), UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_orange );

	x = b->generic.x;
	y = b->generic.y;
	w = b->width;
	h =	b->height + 28;
	if( b->generic.flags & QMF_HIGHLIGHT ) {	
		UI_DrawHandlePic( x, y, w, h, b->focusshader );
	}
}

/*
=================
StartServer_SetMenuItems
=================
*/
static void StartServer_SetMenuItems( void ) {
	const char	*info;

	Com_sprintf( s_startserver.powerlevel.field.buffer, 6, "%i", (int)Com_Clamp( 1, 32767, trap_Cvar_VariableValue( "ui_ffa_powerlevel" ) ) );
	Com_sprintf( s_startserver.powerlevelMaximum.field.buffer, 6, "%i", (int)Com_Clamp( 1, 32767, trap_Cvar_VariableValue( "ui_ffa_powerlevelMaximum" ) ) );
	Com_sprintf( s_startserver.breakLimitRate.field.buffer, 4, "%i", (int)Com_Clamp( 1, 100, trap_Cvar_VariableValue( "ui_ffa_breakLimitRate" ) ) );

	Q_strncpyz( s_startserver.hostname.field.buffer, UI_Cvar_VariableString( "sv_hostname" ), sizeof( s_startserver.hostname.field.buffer ) );
}


/*
=================
StartServer_MenuInit
=================
*/

#define OPTIONS_X	230
static void StartServer_MenuInit( void ) {
	int	i;
	int	x;
	int	y;
	int n;
	static char mapnamebuffer[64];

	// zero set all our globals
	memset( &s_startserver, 0 ,sizeof(startserver_t) );

	StartServer_Cache();

	s_startserver.menu.wrapAround = qtrue;
	s_startserver.menu.fullscreen = qtrue;

	s_startserver.banner.generic.type  = MTYPE_BTEXT;
	s_startserver.banner.generic.x	   = 320;
	s_startserver.banner.generic.y	   = 16;
	s_startserver.banner.string        = "GAME SERVER";
	s_startserver.banner.color         = color_white;
	s_startserver.banner.style         = UI_CENTER|UI_DROPSHADOW;

	s_startserver.framel.generic.type  = MTYPE_BITMAP;
	s_startserver.framel.generic.name  = GAMESERVER_FRAMEL;
	s_startserver.framel.generic.flags = QMF_INACTIVE;
	s_startserver.framel.generic.x	   = 0;  
	s_startserver.framel.generic.y	   = 78;
	s_startserver.framel.width  	   = 256;
	s_startserver.framel.height  	   = 329;

	s_startserver.framer.generic.type  = MTYPE_BITMAP;
	s_startserver.framer.generic.name  = GAMESERVER_FRAMER;
	s_startserver.framer.generic.flags = QMF_INACTIVE;
	s_startserver.framer.generic.x	   = 376;
	s_startserver.framer.generic.y	   = 76;
	s_startserver.framer.width  	   = 256;
	s_startserver.framer.height  	   = 334;

	s_startserver.gametype.generic.type		= MTYPE_SPINCONTROL;
	s_startserver.gametype.generic.name		= "Game Type:";
	s_startserver.gametype.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_startserver.gametype.generic.callback	= StartServer_GametypeEvent;
	s_startserver.gametype.generic.id		= ID_GAMETYPE;
	s_startserver.gametype.generic.x		= 320 - 24;
	s_startserver.gametype.generic.y		= 368;
	s_startserver.gametype.itemnames		= gametype_items;

	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		x =	(i % MAX_MAPCOLS) * (140+8) + 188;
		y = i + 90;

		s_startserver.mappics[i].generic.type   = MTYPE_BITMAP;
		s_startserver.mappics[i].generic.flags  = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
		s_startserver.mappics[i].generic.x	    = x;
		s_startserver.mappics[i].generic.y	    = y + 40;
		s_startserver.mappics[i].generic.id		= ID_PICTURES+i;
		s_startserver.mappics[i].width  		= 128;
		s_startserver.mappics[i].height  	    = 96;
		s_startserver.mappics[i].focuspic       = GAMESERVER_SELECTED;
		s_startserver.mappics[i].errorpic       = GAMESERVER_UNKNOWNMAP;
		s_startserver.mappics[i].generic.ownerdraw = StartServer_LevelshotDraw;

		s_startserver.mapbuttons[i].generic.type     = MTYPE_BITMAP;
		s_startserver.mapbuttons[i].generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_NODEFAULTINIT;
		s_startserver.mapbuttons[i].generic.id       = ID_PICTURES+i;
		s_startserver.mapbuttons[i].generic.callback = StartServer_MapEvent;
		s_startserver.mapbuttons[i].generic.x	     = x - 30;
		s_startserver.mapbuttons[i].generic.y	     = y + 40 - 32;
		s_startserver.mapbuttons[i].width  		     = 256;
		s_startserver.mapbuttons[i].height  	     = 248;
		s_startserver.mapbuttons[i].generic.left     = x;
		s_startserver.mapbuttons[i].generic.top  	 = y;
		s_startserver.mapbuttons[i].generic.right    = x + 128;
		s_startserver.mapbuttons[i].generic.bottom   = y + 128;
		s_startserver.mapbuttons[i].focuspic         = GAMESERVER_SELECT;
	}

	s_startserver.prevpage.generic.type	    = MTYPE_BITMAP;
	s_startserver.prevpage.generic.name     = GAMESERVER_ARROWSL;
	s_startserver.prevpage.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.prevpage.generic.callback = StartServer_MenuEvent;
	s_startserver.prevpage.generic.id	    = ID_PREVPAGE;
	s_startserver.prevpage.generic.x		= 128;
	s_startserver.prevpage.generic.y		= 440;
	s_startserver.prevpage.width  		    = 64;
	s_startserver.prevpage.height  		    = 32;
	s_startserver.prevpage.focuspic         = GAMESERVER_ARROWSL;

	s_startserver.nextpage.generic.type	    = MTYPE_BITMAP;
	s_startserver.nextpage.generic.name     = GAMESERVER_ARROWSR;
	s_startserver.nextpage.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.nextpage.generic.callback = StartServer_MenuEvent;
	s_startserver.nextpage.generic.id	    = ID_NEXTPAGE;
	s_startserver.nextpage.generic.x		= 465;
	s_startserver.nextpage.generic.y		= 440;
	s_startserver.nextpage.width  		    = 64;
	s_startserver.nextpage.height  		    = 32;
	s_startserver.nextpage.focuspic         = GAMESERVER_ARROWSR;

	s_startserver.mapname.generic.type  = MTYPE_PTEXT;
	s_startserver.mapname.generic.flags = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_startserver.mapname.generic.x	    = 320;
	s_startserver.mapname.generic.y	    = 440;
	s_startserver.mapname.string        = mapnamebuffer;
	s_startserver.mapname.style         = UI_CENTER|UI_BIGFONT|UI_DROPSHADOW;
	s_startserver.mapname.color         = text_color_normal;

	s_startserver.hostname.generic.type       = MTYPE_FIELD;
	s_startserver.hostname.generic.name       = "Hostname     :";
	s_startserver.hostname.generic.flags      = QMF_LEFT_JUSTIFY | QMF_SMALLFONT;
	s_startserver.hostname.generic.x          = OPTIONS_X;
	s_startserver.hostname.generic.y	        = 270;
	s_startserver.hostname.field.widthInChars = 24;
	s_startserver.hostname.field.maxchars     = 64;
	
	y += BIGCHAR_HEIGHT+2;
	s_startserver.powerlevel.generic.type       = MTYPE_FIELD;
	s_startserver.powerlevel.generic.name       = "Power Level  :";
	s_startserver.powerlevel.generic.flags      = QMF_LEFT_JUSTIFY |QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_startserver.powerlevel.generic.x	         = OPTIONS_X;
	s_startserver.powerlevel.generic.y	         = 286;
	s_startserver.powerlevel.field.widthInChars = 5;
	s_startserver.powerlevel.field.maxchars     = 5;

	y += BIGCHAR_HEIGHT+2;
	s_startserver.powerlevelMaximum.generic.type       = MTYPE_FIELD;
	s_startserver.powerlevelMaximum.generic.name       = "Maximum PL   :";
	s_startserver.powerlevelMaximum.generic.flags      = QMF_LEFT_JUSTIFY | QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_startserver.powerlevelMaximum.generic.x	         = OPTIONS_X;
	s_startserver.powerlevelMaximum.generic.y	         = 302;
	s_startserver.powerlevelMaximum.field.widthInChars = 5;
	s_startserver.powerlevelMaximum.field.maxchars     = 5;

	y += BIGCHAR_HEIGHT+2;
	s_startserver.breakLimitRate.generic.type       = MTYPE_FIELD;
	s_startserver.breakLimitRate.generic.name       = "Charge Speed :";
	s_startserver.breakLimitRate.generic.flags      = QMF_LEFT_JUSTIFY | QMF_NUMBERSONLY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_startserver.breakLimitRate.generic.x	         = OPTIONS_X;
	s_startserver.breakLimitRate.generic.y	         = 318;
	s_startserver.breakLimitRate.field.widthInChars = 3;
	s_startserver.breakLimitRate.field.maxchars     = 3;

	y += BIGCHAR_HEIGHT+2;
	s_startserver.dedicated.generic.type		= MTYPE_SPINCONTROL;
	s_startserver.dedicated.generic.id		= ID_DEDICATED;
	s_startserver.dedicated.generic.flags		= QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_startserver.dedicated.generic.callback	= StartServer_MenuEvent;
	s_startserver.dedicated.generic.x			= OPTIONS_X+2;
	s_startserver.dedicated.generic.y			= 334;
	s_startserver.dedicated.generic.name		= "Dedicated    :";
	s_startserver.dedicated.itemnames			= dedicated_list;
	
	s_startserver.next.generic.type			= MTYPE_PTEXT;
	s_startserver.next.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.next.generic.id			= ID_STARTSERVERNEXT;
	s_startserver.next.generic.callback		= StartServer_MenuEvent;
	s_startserver.next.generic.x			= 28;
	s_startserver.next.generic.y			= 119;
	s_startserver.next.string				= "FIGHT!";
	s_startserver.next.style				= UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	s_startserver.next.color				= color_white;
	y+=38;			
	s_startserver.back.generic.type			= MTYPE_PTEXT;
	s_startserver.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_startserver.back.generic.id			= ID_STARTSERVERBACK;
	s_startserver.back.generic.callback		= StartServer_MenuEvent;
	s_startserver.back.generic.x			= 28;
	s_startserver.back.generic.y			= 157;
	s_startserver.back.string				= "BACK";
	s_startserver.back.style				= UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	s_startserver.back.color				= color_white;
	y+=38;	
		

	s_startserver.item_null.generic.type	= MTYPE_BITMAP;
	s_startserver.item_null.generic.flags	= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	s_startserver.item_null.generic.x		= 0;
	s_startserver.item_null.generic.y		= 0;
	s_startserver.item_null.width			= 640;
	s_startserver.item_null.height			= 480;

	for (i=0; i<MAX_MAPSPERPAGE; i++)
	{
		Menu_AddItem( &s_startserver.menu, &s_startserver.mappics[i] );
		Menu_AddItem( &s_startserver.menu, &s_startserver.mapbuttons[i] );
	}

	Menu_AddItem( &s_startserver.menu, &s_startserver.powerlevel );
	Menu_AddItem( &s_startserver.menu, &s_startserver.powerlevelMaximum );
	Menu_AddItem( &s_startserver.menu, &s_startserver.breakLimitRate );
	Menu_AddItem( &s_startserver.menu, &s_startserver.dedicated );
	Menu_AddItem( &s_startserver.menu, &s_startserver.hostname );	
	
	Menu_AddItem( &s_startserver.menu, &s_startserver.prevpage );
	Menu_AddItem( &s_startserver.menu, &s_startserver.nextpage );
	Menu_AddItem( &s_startserver.menu, &s_startserver.next );
	Menu_AddItem( &s_startserver.menu, &s_startserver.back );
	Menu_AddItem( &s_startserver.menu, &s_startserver.mapname );
	Menu_AddItem( &s_startserver.menu, &s_startserver.item_null );
	
	StartServer_GametypeEvent( NULL, QM_ACTIVATED );
	StartServer_SetMenuItems();
}


/*
=================
StartServer_Cache
=================
*/
void StartServer_Cache( void )
{
	int				i;
	const char		*info;
	qboolean		precache;
	char			picname[64];
	char			mapname[ MAX_NAMELENGTH ];

	trap_R_RegisterShaderNoMip( GAMESERVER_BACK0 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_BACK1 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_NEXT0 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_NEXT1 );	
	trap_R_RegisterShaderNoMip( GAMESERVER_FRAMEL );	
	trap_R_RegisterShaderNoMip( GAMESERVER_FRAMER );	
	trap_R_RegisterShaderNoMip( GAMESERVER_SELECT );
	trap_R_RegisterShaderNoMip( GAMESERVER_FIGHT0 );
	trap_R_RegisterShaderNoMip( GAMESERVER_FIGHT1 );

	trap_R_RegisterShaderNoMip( GAMESERVER_SELECTED );	
	trap_R_RegisterShaderNoMip( GAMESERVER_UNKNOWNMAP );
	trap_R_RegisterShaderNoMip( GAMESERVER_ARROWSL );
	trap_R_RegisterShaderNoMip( GAMESERVER_ARROWSR );

	precache = trap_Cvar_VariableValue("com_buildscript");

	if( precache ) {
		for( i = 0; i < UI_GetNumArenas(); i++ ) {
			info = UI_GetArenaInfoByNumber( i );
			Q_strncpyz( mapname, Info_ValueForKey( info, "map"), MAX_NAMELENGTH );
			Q_strupr( mapname );
	
			Com_sprintf( picname, sizeof(picname), "maps/%s", mapname );
			trap_R_RegisterShaderNoMip(picname);
		}
	}
}


/*
=================
UI_StartServerMenu
=================
*/
void UI_StartServerMenu( void ) {
	uis.menuamount = 2;
	uis.hideEarth = qtrue;
	uis.showFrame = qtrue;	
	StartServer_MenuInit();
	UI_PushMenu( &s_startserver.menu );
}
