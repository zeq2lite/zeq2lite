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
// cg_info.c -- display information while data is being loading

#include "cg_local.h"

#define MAX_LOADING_PLAYER_ICONS	16

static int			loadingPlayerIconCount;
static char	 		loadingPlayerNames[MAX_LOADING_PLAYER_ICONS][32];
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];

/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int		n;
	int		x, y;
	cgs.media.chatBackgroundShader = trap_R_RegisterShaderNoMip("chatBox");
	for( n = 0; n < loadingPlayerIconCount; n++ ) {
		y = 80 + (40 * n);
		CG_DrawPic(qfalse,-25,y-22,369,75,cgs.media.chatBackgroundShader );
		CG_DrawPic(qfalse,5,y,32,32,loadingPlayerIcons[n] );
		CG_DrawSmallStringCustom(42,y+10,11,11,loadingPlayerNames[n],1.0,6);
		//cgs.clientinfo[clientNum].name
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );
	trap_UpdateScreen();
}


/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			name[32];
	char			personality[MAX_QPATH];
	char			model[MAX_QPATH];
	char			iconName[MAX_QPATH];

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		Q_strncpyz( name, Info_ValueForKey( info, "n" ), sizeof(name) );
		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, MAX_QPATH, "players//%s/icon_%s.tga", model, skin );
	
		strcpy(loadingPlayerNames[loadingPlayerIconCount],name);
		loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "players//characters/%s/icon_%s.tga", model, skin );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !loadingPlayerIcons[loadingPlayerIconCount] ) {
			Com_sprintf( iconName, MAX_QPATH, "players//%s/icon_%s.tga", DEFAULT_MODEL, "default" );
			loadingPlayerIcons[loadingPlayerIconCount] = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( loadingPlayerIcons[loadingPlayerIconCount] ) {
			loadingPlayerIconCount++;
		}
	}
	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof(personality) );
	Q_CleanStr( personality );
	CG_LoadingString( personality );
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*s;
	const char	*info;
	const char	*sysInfo;
	int			y;
	int			value;
	qhandle_t	levelshot,text,dots;
	char		buf[1024];

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );
	s = Info_ValueForKey(info,"mapname");
	levelshot = trap_R_RegisterShaderNoMip(va("maps/%s.jpg",s));
	dots = trap_R_RegisterShaderNoMip("dots");
	if(!levelshot){levelshot = trap_R_RegisterShaderNoMip("menuback");}
	trap_R_SetColor(NULL);
	CG_DrawPic(qfalse,0,0,SCREEN_WIDTH,SCREEN_HEIGHT,levelshot);
	CG_DrawLoadingIcons();
	if(cg.infoScreenText[0]){
		text = trap_R_RegisterShaderNoMip("loading");
		CG_DrawPic(qfalse,0,18,127,64,text);
		CG_DrawPic(qfalse,64,52,8,4,dots);
	}
	else{
		text = trap_R_RegisterShaderNoMip("ready");
		CG_DrawPic(qfalse,0,18,127,64,text);
	}
	y = 180-32;
	// Remote Server only
}

