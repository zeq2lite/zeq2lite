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
//
// gameinfo.c
//

#include "ui_local.h"


//
// arena info
//

#define POOLSIZE	128 * 1024

static int		ui_numArenas;
static char		*ui_arenaInfos[MAX_ARENAS];

static int		ui_numSinglePlayerArenas;
static int		ui_numSpecialSinglePlayerArenas;

static char		memoryPool[POOLSIZE];
static int		allocPoint, outOfMemory;


/*
===============
UI_Alloc
===============
*/
void *UI_Alloc( int size ) {
	char	*p;

	if ( allocPoint + size > POOLSIZE ) {
		outOfMemory = qtrue;
		return NULL;
	}

	p = &memoryPool[allocPoint];

	allocPoint += ( size + 31 ) & ~31;

	return p;
}

/*
===============
UI_InitMemory
===============
*/
void UI_InitMemory( void ) {
	allocPoint = 0;
	outOfMemory = qfalse;
}

/*
===============
UI_ParseInfos
===============
*/
int UI_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( &buf );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( &buf, qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = UI_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ui_numArenas += UI_ParseInfos( buf, MAX_ARENAS - ui_numArenas, &ui_arenaInfos[ui_numArenas] );
}

/*
===============
UI_LoadArenas
===============
*/
static void UI_LoadArenas( void ) {
	int			numdirs;
	vmCvar_t	arenasFile;
	char		filename[128];
	char		dirlist[2048];
	char*		dirptr;
	int			i, n;
	int			dirlen;
	char		*type;
	char		*tag;
	int			singlePlayerNum, specialNum, otherNum;

	ui_numArenas = 0;
	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("maps", ".arena", dirlist, 2048 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "maps/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}
	trap_Print( va( "%i arenas parsed\n", ui_numArenas ) );
	if (outOfMemory) trap_Print(S_COLOR_YELLOW"WARNING: not anough memory in pool to load all arenas\n");

	// set initial numbers
	for( n = 0; n < ui_numArenas; n++ ) {
		Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", n ) );
	}

	// go through and count single players levels
	ui_numSinglePlayerArenas = 0;
	ui_numSpecialSinglePlayerArenas = 0;
	for( n = 0; n < ui_numArenas; n++ ) {
		// determine type
		type = Info_ValueForKey( ui_arenaInfos[n], "type" );

		// if no type specified, it will be treated as "ffa"
		if( !*type ) {
			continue;
		}

		if( strstr( type, "single" ) ) {
			// check for special single player arenas (training, final)
			tag = Info_ValueForKey( ui_arenaInfos[n], "special" );
			if( *tag ) {
				ui_numSpecialSinglePlayerArenas++;
				continue;
			}

			ui_numSinglePlayerArenas++;
		}
	}

	n = ui_numSinglePlayerArenas % ARENAS_PER_TIER;
	if( n != 0 ) {
		ui_numSinglePlayerArenas -= n;
		trap_Print( va( "%i arenas ignored to make count divisible by %i\n", n, ARENAS_PER_TIER ) );
	}

	// go through once more and assign number to the levels
	singlePlayerNum = 0;
	specialNum = singlePlayerNum + ui_numSinglePlayerArenas;
	otherNum = specialNum + ui_numSpecialSinglePlayerArenas;
	for( n = 0; n < ui_numArenas; n++ ) {
		// determine type
		type = Info_ValueForKey( ui_arenaInfos[n], "type" );

		// if no type specified, it will be treated as "ffa"
		if( *type ) {
			if( strstr( type, "single" ) ) {
				// check for special single player arenas (training, final)
				tag = Info_ValueForKey( ui_arenaInfos[n], "special" );
				if( *tag ) {
					Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", specialNum++ ) );
					continue;
				}

				Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", singlePlayerNum++ ) );
				continue;
			}
		}

		Info_SetValueForKey( ui_arenaInfos[n], "num", va( "%i", otherNum++ ) );
	}
}

/*
===============
UI_GetArenaInfoByNumber
===============
*/
const char *UI_GetArenaInfoByNumber( int num ) {
	int		n;
	char	*value;

	if( num < 0 || num >= ui_numArenas ) {
		//trap_Print( va( S_COLOR_RED "Invalid arena number: %i\n", num ) );
		return NULL;
	}

	for( n = 0; n < ui_numArenas; n++ ) {
		value = Info_ValueForKey( ui_arenaInfos[n], "num" );
		if( *value && atoi(value) == num ) {
			return ui_arenaInfos[n];
		}
	}

	return NULL;
}


/*
===============
UI_GetArenaInfoByNumber
===============
*/
const char *UI_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < ui_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( ui_arenaInfos[n], "map" ), map ) == 0 ) {
			return ui_arenaInfos[n];
		}
	}

	return NULL;
}


/*
===============
UI_GetSpecialArenaInfo
===============
*/
const char *UI_GetSpecialArenaInfo( const char *tag ) {
	int			n;

	for( n = 0; n < ui_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( ui_arenaInfos[n], "special" ), tag ) == 0 ) {
			return ui_arenaInfos[n];
		}
	}

	return NULL;
}

/*
===============
UI_GetNumArenas
===============
*/
int UI_GetNumArenas( void ) {
	return ui_numArenas;
}

/*
===============
UI_InitGameinfo
===============
*/
void UI_InitGameinfo( void ) {

	UI_InitMemory();
	UI_LoadArenas();
	uis.menuamount = 0;
	uis.demoversion = qfalse;
}
