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
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.ogg",
	"*death2.ogg",
	"*death3.ogg",
	"*jump1.ogg",
	"*highjump1.ogg",
	"*pain1.ogg",
	"*pain2.ogg",
	"*pain3.ogg",
	"*pain4.ogg",
	"*pain5.ogg",
	"*pain6.ogg",
	"*pain7.ogg",
	"*pain8.ogg",
	"*pain9.ogg",
	"*falling1.ogg",
	"*gasp.ogg",
	"*drown.ogg",
	"*fall1.ogg",
	"*fallSoft.ogg",
	"*fallHard1.ogg",
	"*fallHard2.ogg",
	"*fallHard3.ogg",
	"*taunt.ogg",
};

// HACK: We have to copy the entire playerEntity_t information
//       because otherwise Q3 finds that all previously set information
//       regarding frames and refEntities are nulled out on certain occassions.
//       WTF is up with this stupid bug anyway?! Same thing happened when adding
//       certain fields to centity_t...
static playerEntity_t	playerInfoDuplicate[MAX_GENTITIES];




/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
		}
	}

	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
players//visor/animation.cfg, etc
======================
*/
// static qboolean CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
// FIXME: Needs to lose static to use it in cg_tiers.c
qboolean CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[32000];
	fileHandle_t	f;
	animation_t *animations;
	qboolean doneMeshType;
	animations = ci->animations;
	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the text
	text_p = text;
	skip = 0;	// quiet the compiler warning
	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;
	ci->overrideHead = qfalse;
	// Alex Adding For MD4 Handling
	ci->usingMD4 = qfalse;
	doneMeshType = qfalse;
	//Alex End Adding 
	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		//Alex Adding For MD4 Handling...
		if(!Q_stricmp( token, "mesh_type") && doneMeshType != qtrue){
			token = COM_Parse( &text_p );
			Com_Printf("Parsing MeshType: %s\n", token);
			if( !Q_stricmp( token, "md4" ) || !Q_stricmp( token, "skeletalanimation")){
				doneMeshType = qtrue;
				ci->usingMD4 = qtrue;
				//Com_Printf("Setting ci->usingMD4 = %i\n", ci->usingMD4);
				continue;
			}
			else if( !Q_stricmp( token, "md3") || !Q_stricmp( token, "vertexanimation"))
			{
				doneMeshType = qtrue;
				ci->usingMD4 = qfalse;
				//Com_Printf("Setting ci->usingMD4 = %i\n", ci->usingMD4);
				continue;
			}
			else{
				doneMeshType = qtrue;
				ci->usingMD4 = qfalse;
				Com_Printf( "Bad MESH_TYPE param in %s: %s\n", filename, token );
				continue;
			}
		}
		//Alex end adding
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {
				ci->footsteps = FOOTSTEP_FLESH;
			} else if ( !Q_stricmp( token, "mech" ) ) {
				ci->footsteps = FOOTSTEP_MECH;
			} else if ( !Q_stricmp( token, "energy" ) ) {
				ci->footsteps = FOOTSTEP_ENERGY;
			} else {
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !*token ) {
					break;
				}
				ci->headOffset[i] = atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			if ( token[0] == 'f' || token[0] == 'F' ) {
				ci->gender = GENDER_FEMALE;
			} else if ( token[0] == 'n' || token[0] == 'N' ) {
				ci->gender = GENDER_NEUTER;
			} else {
				ci->gender = GENDER_MALE;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			ci->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			ci->fixedtorso = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "overrideHead" ) ) {
			ci->overrideHead = qtrue;
			continue;
		}
		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf( "unknown token '%s' in %s\n", token, filename );
	}
	if(doneMeshType!=qtrue) ci->usingMD4 = qfalse;
	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {
		token = COM_Parse( &text_p );
		if(!*token){break;}
		animations[i].firstFrame = atoi( token );
		token = COM_Parse( &text_p );
		if(!*token){break;}
		animations[i].numFrames = atoi( token );
		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0) {
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}
		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		animations[i].loopFrames = atoi( token );
		token = COM_Parse( &text_p );
		if ( !*token ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
		// ADDING FOR ZEQ2
		// Read the continuous flag for ki attack animations
		if ( i >= ANIM_SKILL1_FIRE &&
			 i < MAX_ANIMATIONS &&
			 (i - ANIM_SKILL1_FIRE) % 2 == 0 ) {
			token = COM_Parse( &text_p );
			if(!*token){break;}
			animations[i].continuous = atoi(token);
		}
	}
	memcpy(&animations[ANIM_BACKWALK], &animations[ANIM_WALK], sizeof(animation_t));
	animations[ANIM_BACKWALK].reversed = qtrue;
	memcpy(&animations[ANIM_BACKRUN], &animations[ANIM_RUN], sizeof(animation_t));
	animations[ANIM_BACKRUN].reversed = qtrue;
	return qtrue;
}

/*
==========================
CG_FileExists
==========================
*/
static qboolean	CG_FileExists(const char *filename) {
	int len;

	len = trap_FS_FOpenFile( filename, NULL, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}

/*
==========================
CG_FindClientModelFile
==========================
*/
static qboolean	CG_FindClientModelFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext ) {
	char *team, *charactersFolder;
	int i;
	if ( cgs.gametype >= GT_TEAM ) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}
	charactersFolder = "";
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"players//characters/james/stroggs/lower_lily_red.skin"
				Com_sprintf( filename, length, "players//%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"players//characters/james/lower_lily_red.skin"
				Com_sprintf( filename, length, "players//%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM ) {
				if ( i == 0 && teamName && *teamName ) {
					//								"players//characters/james/stroggs/lower_red.skin"
					Com_sprintf( filename, length, "players//%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, team, ext );
				}
				else {
					//								"players//characters/james/lower_red.skin"
					Com_sprintf( filename, length, "players//%s%s/%s_%s.%s", charactersFolder, modelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					//								"players//characters/james/stroggs/lower_lily.skin"
					Com_sprintf( filename, length, "players//%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
				}
				else {
					//								"players//characters/james/lower_lily.skin"
					Com_sprintf( filename, length, "players//%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( charactersFolder[0] ) {
			break;
		}
		charactersFolder = "characters/";
	}

	return qfalse;
}

/*
==========================
CG_FindClientHeadFile
==========================
*/
static qboolean	CG_FindClientHeadFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext ) {
	char *team, *headsFolder;
	int i;

	if ( cgs.gametype >= GT_TEAM ) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}

	if ( headModelName[0] == '*' ) {
		headsFolder = "heads/";
		headModelName++;
	}
	else {
		headsFolder = "";
	}
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				Com_sprintf( filename, length, "players//%s%s/%s/%s%s_%s.%s", headsFolder, headModelName, headSkinName, teamName, base, team, ext );
			}
			else {
				Com_sprintf( filename, length, "players//%s%s/%s/%s_%s.%s", headsFolder, headModelName, headSkinName, base, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM ) {
				if ( i == 0 &&  teamName && *teamName ) {
					Com_sprintf( filename, length, "players//%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, team, ext );
				}
				else {
					Com_sprintf( filename, length, "players//%s%s/%s_%s.%s", headsFolder, headModelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					Com_sprintf( filename, length, "players//%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, headSkinName, ext );
				}
				else {
					Com_sprintf( filename, length, "players//%s%s/%s_%s.%s", headsFolder, headModelName, base, headSkinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( headsFolder[0] ) {
			break;
		}
		headsFolder = "heads/";
	}

	return qfalse;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/
static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName ) {
	/*
	char filename[MAX_QPATH];

	
	Com_sprintf( filename, sizeof( filename ), "players//%s/%slower_%s.skin", modelName, teamName, skinName );
	ci->legsSkin = trap_R_RegisterSkin( filename );
	if (!ci->legsSkin) {
		Com_sprintf( filename, sizeof( filename ), "players//characters/%s/%slower_%s.skin", modelName, teamName, skinName );
		ci->legsSkin = trap_R_RegisterSkin( filename );
		if (!ci->legsSkin) {
			Com_Printf( "Leg skin load failure: %s\n", filename );
		}
	}


	Com_sprintf( filename, sizeof( filename ), "players//%s/%supper_%s.skin", modelName, teamName, skinName );
	ci->torsoSkin = trap_R_RegisterSkin( filename );
	if (!ci->torsoSkin) {
		Com_sprintf( filename, sizeof( filename ), "players//characters/%s/%supper_%s.skin", modelName, teamName, skinName );
		ci->torsoSkin = trap_R_RegisterSkin( filename );
		if (!ci->torsoSkin) {
			Com_Printf( "Torso skin load failure: %s\n", filename );
		}
	}
	*/

	/*
	if ( CG_FindClientModelFile( filename, sizeof(filename), ci, teamName, modelName, skinName, "lower", "skin" ) ) {
		ci->legsSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->legsSkin) {
		Com_Printf( "Leg skin load failure: %s\n", filename );
	}

	if ( CG_FindClientModelFile( filename, sizeof(filename), ci, teamName, modelName, skinName, "upper", "skin" ) ) {
		ci->torsoSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->torsoSkin) {
		Com_Printf( "Torso skin load failure: %s\n", filename );
	}

	if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headModelName, headSkinName, "head", "skin" ) ) {
		ci->headSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->headSkin) {
		Com_Printf( "Head skin load failure: %s\n", filename );
	}

	// if any skins failed to load
	if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin ) {
		return qfalse;
	}

	*/
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName ) {

	//Com_Printf("RegisterClientModelname: before tier load ci->usingMD4 = %i\n", ci->usingMD4);
	if (!CG_RegisterClientModelnameWithTiers( ci, modelName, skinName ) ) {
		return qfalse;
	}
	//Com_Printf("RegisterClientModelname: after tier load ci->usingMD4 = %i\n", ci->usingMD4);

/*
	char	filename[MAX_QPATH*2];
	const char		*headName;
	char newTeamName[MAX_QPATH*2];

	if ( headModelName[0] == '\0' ) {
		headName = modelName;
	}
	else {
		headName = headModelName;
	}
	Com_sprintf( filename, sizeof( filename ), "players//%s/lower.md3", modelName );
	ci->legsModel = trap_R_RegisterModel( filename );
	if ( !ci->legsModel ) {
		Com_sprintf( filename, sizeof( filename ), "players//characters/%s/lower.md3", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );
		if ( !ci->legsModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof( filename ), "players//%s/upper.md3", modelName );
	ci->torsoModel = trap_R_RegisterModel( filename );
	if ( !ci->torsoModel ) {
		Com_sprintf( filename, sizeof( filename ), "players//characters/%s/upper.md3", modelName );
		ci->torsoModel = trap_R_RegisterModel( filename );
		if ( !ci->torsoModel ) {
			Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}

	if( headName[0] == '*' ) {
		Com_sprintf( filename, sizeof( filename ), "players//heads/%s/%s.md3", &headModelName[1], &headModelName[1] );
	}
	else {
		Com_sprintf( filename, sizeof( filename ), "players//%s/head.md3", headName );
	}
	ci->headModel = trap_R_RegisterModel( filename );
	// if the head model could not be found and we didn't load from the heads folder try to load from there
	if ( !ci->headModel && headName[0] != '*' ) {
		Com_sprintf( filename, sizeof( filename ), "players//heads/%s/%s.md3", headModelName, headModelName );
		ci->headModel = trap_R_RegisterModel( filename );
	}
	if ( !ci->headModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, teamName, modelName, skinName, headName, headSkinName ) ) {
		if ( teamName && *teamName) {
			Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", teamName, modelName, skinName, headName, headSkinName );
			if( ci->team == TEAM_BLUE ) {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
			}
			else {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
			}
			if ( !CG_RegisterClientSkin( ci, newTeamName, modelName, skinName, headName, headSkinName ) ) {
				Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", newTeamName, modelName, skinName, headName, headSkinName );
				return qfalse;
			}
		} else {
			Com_Printf( "Failed to load skin file: %s : %s, %s : %s\n", modelName, skinName, headName, headSkinName );
			return qfalse;
		}
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "players//%s/animation.cfg", modelName );
	if ( !CG_ParseAnimationFile( filename, ci ) ) {
		Com_sprintf( filename, sizeof( filename ), "players//characters/%s/animation.cfg", modelName );
		if ( !CG_ParseAnimationFile( filename, ci ) ) {
			Com_Printf( "Failed to load animation file %s\n", filename );
			return qfalse;
		}
	}

	if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "skin" ) ) {
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}
	else if ( CG_FindClientHeadFile( filename, sizeof(filename), ci, teamName, headName, headSkinName, "icon", "tga" ) ) {
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}

	if ( !ci->modelIcon ) {
		return qfalse;
	}
*/

	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( int clientNum, clientInfo_t *ci ) {
	const char	*dir, *fallback;
	int			i, modelloaded;
	const char	*s;
	char		teamname[MAX_QPATH];

	teamname[0] = 0;

	modelloaded = qtrue;
	ci->usingMD4 = qfalse;
	Com_Printf("LoadClientInfo pre CG_RegisterClientModelname\n");
	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname ) ) {
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
		}

		// fall back to default team name
		if( cgs.gametype >= GT_TEAM) {
			// keep skin name
			if( ci->team == TEAM_BLUE ) {
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname) );
			} else {
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname) );
			}
			if ( !CG_RegisterClientModelname( ci, DEFAULT_TEAM_MODEL, ci->skinName, DEFAULT_TEAM_HEAD, ci->skinName, teamname ) ) {
				CG_Error( "DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinName );
			}
		} else {
			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", DEFAULT_MODEL, "default", teamname ) ) {
				CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
			}
		}
		modelloaded = qfalse;
	}
	//Com_Printf("LoadClientInfo: ci->usingMD4 = %i \n",ci->usingMD4);
	ci->newAnims = qfalse;
	// sounds
	dir = ci->modelName;
	fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded) {
			ci->sounds[i] = trap_S_RegisterSound( va("players/%s/%s", dir, s + 1), qfalse );
		}
		if ( !ci->sounds[i] ) {
			ci->sounds[i] = trap_S_RegisterSound( va("players/%s/%s", fallback, s + 1), qfalse );
		}
	}
	ci->deferred = qfalse;
	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& (cg_entities[i].currentState.eType == ET_INVISIBLE
			|| cg_entities[i].currentState.eType == ET_PLAYER) ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
	// REFPOINT: Load the additional tiers' clientInfo here
	CG_RegisterClientAura(clientNum,ci);
	{
		char	filename[MAX_QPATH];
		
		Com_sprintf( filename, sizeof( filename ), "players//%s/%s.grfx", ci->modelName, ci->skinName );
		CG_weapGfx_Parse( filename, clientNum );
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	// ADDING FOR ZEQ2
	int fromIndex, toIndex, i;

	fromIndex = from - cgs.clientinfo;
	toIndex = to - cgs.clientinfo;
	// END ADDING

	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;

	for ( i = 0; i < 8; i++ ) {
		to->legsModel[i] = from->legsModel[i];
		to->legsSkin[i] = from->legsSkin[i];
		to->torsoModel[i] = from->torsoModel[i];
		to->torsoSkin[i] = from->torsoSkin[i];
		to->headModel[i] = from->headModel[i];
		to->headSkin[i] = from->headSkin[i];
	}
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	// ADDING FOR ZEQ2
	// Copy over the weapon graphics settings!
	CG_CopyUserWeaponGraphics( fromIndex, toIndex );
	// END ADDING

	memcpy( to->animations, from->animations, sizeof( to->animations ) );
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );

	
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->headModelName, match->headModelName )
			&& !Q_stricmp( ci->headSkinName, match->headSkinName ) 
			&& !Q_stricmp( ci->blueTeam, match->blueTeam ) 
			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (cgs.gametype < GT_TEAM || ci->team == match->team) ) {
			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( int clientNum, clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// JUHOX: don't care about client models in lens flare editor
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) return;
#endif

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			 (cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
			continue;
		}
		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ||
				(cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( clientNum, ci );
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;
	char		*slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );

	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi(v);

	v = Info_ValueForKey( configstring, "g_redteam" );
	Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

	v = Info_ValueForKey( configstring, "g_blueteam" );
	Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if( cgs.gametype >= GT_TEAM ) {
			Q_strncpyz( newInfo.modelName, DEFAULT_TEAM_MODEL, sizeof( newInfo.modelName ) );
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}

			Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );
		}

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	// head model
	v = Info_ValueForKey( configstring, "hmodel" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if( cgs.gametype >= GT_TEAM ) {
			Q_strncpyz( newInfo.headModelName, DEFAULT_TEAM_MODEL, sizeof( newInfo.headModelName ) );
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "headmodel", modelStr, sizeof( modelStr ) );
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}

			Q_strncpyz( newInfo.headSkinName, skin, sizeof( newInfo.headSkinName ) );
			Q_strncpyz( newInfo.headModelName, modelStr, sizeof( newInfo.headModelName ) );
		}

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.headModelName, v, sizeof( newInfo.headModelName ) );

		slash = strchr( newInfo.headModelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	/* REFPOINT:
	FIXME: These deferred loading routines fuck up the client number, so don't use them!
	       Conveniently, it ALSO prevents defered loading, which we didn't want anyway.
		   Hooray! :/
	*/

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean	forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( clientNum, &newInfo );
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo( clientNum, &newInfo );
		}
	}

//	*/

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
	//CG_LoadClientInfo( int clientnum, ci );
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				// ADDING FOR ZEQ2
				// REFPOINT: Add dialogbox to initiate vid_restart for cache purge here!

				// END ADDING
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( i, ci );
//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}	

	// see if the animation sequence is switching
	if ( !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}
	if ( newAnimation != lf->animationNumber ) {
		// If the only difference is the togglebit, and the animation is supposed to be
		// continuous.
		if ( (newAnimation & ~ANIM_TOGGLEBIT) == (lf->animationNumber & ~ANIM_TOGGLEBIT) &&
			lf->animation->continuous ) {
			// do nothing, animation should continue to loop
		} else {
			CG_SetLerpFrameAnimation( ci, lf, newAnimation );
		}
	}

	/*
	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}
	*/

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if ( f >= numFrames ) {
			f -= numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if ( anim->reversed ) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f>=anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n");
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}


/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent,
								int *legsOld, int *legs, float *legsBackLerp,
								int *torsoOld, int *torso, float *torsoBackLerp,
								int *headOld, int *head, float *headBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum, tier;
	float			speedScale;
	qboolean		onBodyQue;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	// <-- MDave:  Check if we're dealing with the actual client entity (and not
	//           a dead body from the body queue)
	onBodyQue = (cent->currentState.number != cent->currentState.clientNum);
	// -->

	if (onBodyQue) {
		tier = 0;
	} else {
		tier = cent->currentState.tier;
	}
	speedScale = 1;

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_IDLE ) {
		CG_RunLerpFrame( ci, &cent->pe.legs, ANIM_TURN, speedScale );
	} else {
		CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;

	// ADDING FOR ZEQ2
	{
		int				torsoAnimNum;
		int				legsAnimNum;

		// NOTE: Torso animations take precedence over leg animations when deciding which
		//       head animation to play.
		torsoAnimNum = cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT;

		if ( cent->currentState.eFlags & EF_AURA && ci->overrideHead) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KI_CHARGE, speedScale );
		} else if ( ci->auraConfig[tier]->auraAlways && ci->overrideHead) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KI_CHARGE, speedScale );
		} else if ( ANIM_FLY_UP == torsoAnimNum && ci->overrideHead ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KI_CHARGE, speedScale );
		} else if ( ANIM_FLY_DOWN == torsoAnimNum && ci->overrideHead ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KI_CHARGE, speedScale );
		} else if ( ANIM_FLOOR_RECOVER == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLOOR_RECOVER, speedScale );
		} else if ( ANIM_WALK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_WALK, speedScale );
		} else if ( ANIM_RUN == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_RUN, speedScale );
		} else if ( ANIM_BACKRUN == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BACKRUN, speedScale );
		} else if ( ANIM_JUMP_UP == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_JUMP_UP, speedScale );
		} else if ( ANIM_LAND_UP == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_LAND_UP, speedScale );
		} else if ( ANIM_JUMP_FORWARD == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_JUMP_FORWARD, speedScale );
		} else if ( ANIM_LAND_FORWARD == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_LAND_FORWARD, speedScale );
		} else if ( ANIM_JUMP_BACK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_JUMP_BACK, speedScale );
		} else if ( ANIM_LAND_BACK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_LAND_BACK, speedScale );
		} else if ( ANIM_SWIM_IDLE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_SWIM_IDLE, speedScale );
		} else if ( ANIM_SWIM == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_SWIM, speedScale );
		} else if ( ANIM_DASH_RIGHT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DASH_RIGHT, speedScale );
		} else if ( ANIM_DASH_LEFT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DASH_LEFT, speedScale );
		} else if ( ANIM_DASH_FORWARD == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DASH_FORWARD, speedScale );
		} else if ( ANIM_DASH_BACKWARD == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DASH_BACKWARD, speedScale );
		} else if ( ANIM_KI_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KI_CHARGE, speedScale );
		} else if ( ANIM_PL_DOWN == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_PL_DOWN, speedScale );
		} else if ( ANIM_PL_UP == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_PL_UP, speedScale );
		} else if ( ANIM_TRANS_UP == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_TRANS_UP, speedScale );
		} else if ( ANIM_TRANS_BACK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_TRANS_BACK, speedScale );
		} else if ( ANIM_FLY_IDLE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLY_IDLE, speedScale );
		} else if ( ANIM_FLY_START == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLY_START, speedScale );
		} else if ( ANIM_FLY_FORWARD == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLY_FORWARD, speedScale );
		} else if ( ANIM_FLY_BACKWARD == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLY_BACKWARD, speedScale );
		} else if ( ANIM_FLY_UP == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLY_UP, speedScale );
		} else if ( ANIM_FLY_DOWN == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_FLY_DOWN, speedScale );
		} else if ( ANIM_STUNNED == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_STUNNED, speedScale );
		} else if ( ANIM_PUSH == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_PUSH, speedScale );
		} else if ( ANIM_DEATH_GROUND == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DEATH_GROUND, speedScale );
		} else if ( ANIM_DEATH_AIR == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DEATH_AIR, speedScale );
		} else if ( ANIM_DEATH_AIR_LAND == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DEATH_AIR_LAND, speedScale );
		} else if ( ANIM_STUN == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_STUN, speedScale );
		} else if ( ANIM_DEFLECT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_DEFLECT, speedScale );
		} else if ( ANIM_BLOCK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BLOCK, speedScale );
		} else if ( ANIM_SPEED_MELEE_ATTACK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_SPEED_MELEE_ATTACK, speedScale );
		} else if ( ANIM_SPEED_MELEE_DODGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_SPEED_MELEE_DODGE, speedScale );
		} else if ( ANIM_SPEED_MELEE_BLOCK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_SPEED_MELEE_BLOCK, speedScale );
		} else if ( ANIM_SPEED_MELEE_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_SPEED_MELEE_HIT, speedScale );
		} else if ( ANIM_BREAKER_MELEE_ATTACK1 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_ATTACK1, speedScale );
		} else if ( ANIM_BREAKER_MELEE_ATTACK2 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_ATTACK2, speedScale );
		} else if ( ANIM_BREAKER_MELEE_ATTACK3 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_ATTACK3, speedScale );
		} else if ( ANIM_BREAKER_MELEE_ATTACK4 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_ATTACK4, speedScale );
		} else if ( ANIM_BREAKER_MELEE_ATTACK5 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_ATTACK5, speedScale );
		} else if ( ANIM_BREAKER_MELEE_ATTACK6 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_ATTACK6, speedScale );
		} else if ( ANIM_BREAKER_MELEE_HIT1 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_HIT1, speedScale );
		} else if ( ANIM_BREAKER_MELEE_HIT2 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_HIT2, speedScale );
		} else if ( ANIM_BREAKER_MELEE_HIT3 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_HIT3, speedScale );
		} else if ( ANIM_BREAKER_MELEE_HIT4 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_HIT4, speedScale );
		} else if ( ANIM_BREAKER_MELEE_HIT5 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_HIT5, speedScale );
		} else if ( ANIM_BREAKER_MELEE_HIT6 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_BREAKER_MELEE_HIT6, speedScale );
		} else if ( ANIM_POWER_MELEE_1_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_1_CHARGE, speedScale );
		} else if ( ANIM_POWER_MELEE_2_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_2_CHARGE, speedScale );
		} else if ( ANIM_POWER_MELEE_3_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_3_CHARGE, speedScale );
		} else if ( ANIM_POWER_MELEE_4_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_4_CHARGE, speedScale );
		} else if ( ANIM_POWER_MELEE_5_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_5_CHARGE, speedScale );
		} else if ( ANIM_POWER_MELEE_6_CHARGE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_6_CHARGE, speedScale );
		} else if ( ANIM_POWER_MELEE_1_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_1_HIT, speedScale );
		} else if ( ANIM_POWER_MELEE_2_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_2_HIT, speedScale );
		} else if ( ANIM_POWER_MELEE_3_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_3_HIT, speedScale );
		} else if ( ANIM_POWER_MELEE_4_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_4_HIT, speedScale );
		} else if ( ANIM_POWER_MELEE_5_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_5_HIT, speedScale );
		} else if ( ANIM_POWER_MELEE_6_HIT == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_POWER_MELEE_6_HIT, speedScale );
		} else if ( ANIM_STUNNED_MELEE == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_STUNNED_MELEE, speedScale );
		} else if ( ANIM_KNOCKBACK == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KNOCKBACK, speedScale );
		} else if ( ANIM_KNOCKBACK_HIT_WALL == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KNOCKBACK_HIT_WALL, speedScale );
		} else if ( ANIM_KNOCKBACK_RECOVER_1 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KNOCKBACK_RECOVER_1, speedScale );
		} else if ( ANIM_KNOCKBACK_RECOVER_2 == torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, ANIM_KNOCKBACK_RECOVER_2, speedScale );
		} else if ( ANIM_SKILL1_CHARGE <= torsoAnimNum && ANIM_SKILL32_ALT_FIRE >= torsoAnimNum ) {
			CG_RunLerpFrame( ci, &cent->pe.head, torsoAnimNum - ANIM_SKILL1_CHARGE + ANIM_SKILL1_CHARGE, speedScale );
		} else {
			legsAnimNum = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
			if (cg.predictedPlayerState.lockedTarget >0) {
				CG_RunLerpFrame( ci, &cent->pe.head, ANIM_IDLE_LOCKED, speedScale );
			} else {
				CG_RunLerpFrame( ci, &cent->pe.head, ANIM_IDLE, speedScale );
			}
		}
	}

	*headOld = cent->pe.head.oldFrame;
	*head = cent->pe.head.frame;
	*headBackLerp = cent->pe.head.backlerp;

	// END ADDING
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection ) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}


/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
//	vec3_t		velocity;
//	float		speed;
	int			dir, clientNum;
	clientInfo_t	*ci;
	
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum > MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
		return;
	}
	ci = &cgs.clientinfo[ clientNum ];
	
	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE
		|| ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE
		|| ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE_LOCKED
		|| ( cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE_LOCKED ) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else if ( (( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) >= ANIM_DASH_RIGHT ) &&
				(( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) <= ANIM_DASH_BACKWARD ) ) {
		// don't let these animations adjust angles.
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if ( dir < 0 || dir > 7 ) {
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
	if ( ci->fixedtorso || cent->currentState.playerSkillState == skillCharging || cent->currentState.playerSkillState == skillAltCharging ||
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_DEATH_GROUND || 
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_DEATH_AIR || 
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_DEATH_AIR_LAND ||
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_KNOCKBACK_HIT_WALL ||
				( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_FLOOR_RECOVER ) {
		headAngles[PITCH] = 0.0f;
		torsoAngles[PITCH] = 0.0f;
	}
	if ( ci->fixedlegs ) {
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
	}
	// ADDING FOR ZEQ2
	// We're flying, so we change the entire body's directions altogether.
	if((&cg.predictedPlayerState)->bitFlags & usingFlight || cent->currentState.playerBitFlags & usingFlight){
		VectorCopy( cent->lerpAngles, headAngles );
		VectorCopy( cent->lerpAngles, torsoAngles );
		VectorCopy( cent->lerpAngles, legsAngles );
	}
	// END ADDING

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail( centity_t *cent ) {}

/*
===============
CG_BreathPuffs
===============
*/
static void CG_BreathPuffs( centity_t *cent, refEntity_t *head) {}

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hSkin, refEntity_t *torso ) {}


/*===============
CG_PlayerPowerups
===============*/
static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;
	clientInfo_t	*ci;
	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
}


/*===============
CG_PlayerFloatSprite
Float a sprite over the player's head
===============*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	int		team;
	if(cent->currentState.eFlags & EF_CONNECTION){
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}
	if ( cent->currentState.eFlags & EF_TALK ) {
		CG_PlayerFloatSprite( cent, cgs.media.chatBubble );
		return;
	}
	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( !(cent->currentState.eFlags & EF_DEAD) && 
		cg.snap->ps.persistant[PERS_TEAM] == team &&
		cgs.gametype >= GT_TEAM &&
		cent->currentState.number != cg.snap->ps.clientNum) {
		if (cg_drawFriend.integer) {
			CG_PlayerFloatSprite( cent, cgs.media.friendShader );
		}
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;
	*shadowPlane = 0;
	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}
	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
void CG_PlayerSplash( centity_t *cent, int scale ) {
	vec3_t			start, end;
	vec3_t			origin;
	trace_t			trace;
	polyVert_t		verts[4];
	entityState_t	*s1;
	clientInfo_t	*ci;
	int				contents;
	int				powerBoost;
	float			xyzspeed;
	s1 = &cent->currentState;

	// Measure the objects velocity
	xyzspeed = sqrt( cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
	cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
	cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);

	// Get the objects content value and position.
	BG_EvaluateTrajectory( s1, &s1->pos, cg.time, origin );
	contents = trap_CM_PointContents( origin, 0 );

	VectorCopy( cent->lerpOrigin, end );
	VectorCopy( cent->lerpOrigin, start );

	if (xyzspeed && cent->currentState.eFlags & EF_AURA) {
		powerBoost = 1;
		start[2] += 128;
		end[2] -= 128;
	} else if (!xyzspeed && cent->currentState.eFlags & EF_AURA) {
		powerBoost = 6;
		start[2] += 512;
		end[2] -= 512;
	} else if (xyzspeed) {
		powerBoost = 1;
		start[2] += 64;
		end[2] -= 48;
	} else {
		powerBoost = 1;
		start[2] += 32;
		end[2] -= 24;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	if ( trace.fraction == 1.0 ) {
		return;
	}

	if (xyzspeed || (cent->currentState.eFlags & EF_AURA)) {

		if (powerBoost == 1) {
			CG_WaterSplash(trace.endpos,powerBoost+(xyzspeed/scale));
		}

		if ( cent->trailTime > cg.time ) {
			return;
		}

		cent->trailTime += 250;

		if ( cent->trailTime < cg.time ) {
			cent->trailTime = cg.time;
		}

		if (!xyzspeed && cent->currentState.eFlags & EF_AURA) {
			CG_WaterRipple(trace.endpos,scale/20,qtrue);
		}else{
			CG_WaterRipple(trace.endpos,powerBoost+(xyzspeed/scale),qfalse);
		}
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}

/*
===============
CG_PlayerDirtPush
===============
*/
void CG_PlayerDirtPush( centity_t *cent, int scale, qboolean once ) {
	vec3_t			start, end;
	trace_t			trace;
	playerState_t	*ps;
	ps = &cg.snap->ps;

	if (!once){
		if ( cent->dustTrailTime > cg.time ) {
			return;
		}

		cent->dustTrailTime += 250;

		if ( cent->dustTrailTime < cg.time ) {
			cent->dustTrailTime = cg.time;
		}
	}

	VectorCopy( cent->lerpOrigin, end );
	VectorCopy( cent->lerpOrigin, start );

	if(once){
		end[2] -= 4096;
	}else{
		end[2] -= 512;
	}

	CG_Trace( &trace, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if (trace.fraction == 1.0f){
		return;
	}

	VectorCopy( trace.endpos, end );
	end[2] += 8;

	CG_DirtPush(end,trace.plane.normal,scale);
}

/*
===============
CG_AddRefExtEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefExtEntityWithPowerups( refExtEntity_t *ent, entityState_t *state, int team, qboolean auraAlways ) {
	trap_R_AddRefExtendedEntityToScene( ent );
	if(!auraAlways){
		ent->customShader = cgs.media.globalCelLighting;
		trap_R_AddRefExtendedEntityToScene( ent );
	}
}

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team, qboolean auraAlways ) {
	trap_R_AddRefEntityToScene( ent );
	if(!auraAlways){
		ent->customShader = cgs.media.globalCelLighting;
		trap_R_AddRefEntityToScene( ent );
	}
}


/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs,torso,head;
	refExtEntity_t	altLegs, altTorso;
	playerState_t	*ps;
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
	int				oframes[3],nframes[3];
	float			lerps[3];
	vec3_t			angles[3];
	qboolean		onBodyQue;
	int				tier;
	int				state,enemyState;
	int				scale;
	float			xyzspeed;
	ps = &cg.snap->ps;
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ){CG_Error( "Bad clientNum on player entity" );}
	ci = &cgs.clientinfo[clientNum];
	if (!ci->infoValid){return;}
	onBodyQue = (cent->currentState.number != cent->currentState.clientNum);
	if(onBodyQue){tier = 0;}
	else{
		tier = cent->currentState.tier;
		if ( ci->tierCurrent != tier ) {
			ci->tierCurrent = tier;
			CG_AddEarthquake(cent->lerpOrigin, 400, 1, 0, 1, 400);
		}
	}
	if(ci->tierCurrent > ci->tierMax){
		ci->tierMax = ci->tierCurrent;
	}
	renderfx = 0;
	// Measure the players velocity
	xyzspeed = sqrt( cent->currentState.pos.trDelta[0] * cent->currentState.pos.trDelta[0] +
	cent->currentState.pos.trDelta[1] * cent->currentState.pos.trDelta[1] + 
	cent->currentState.pos.trDelta[2] * cent->currentState.pos.trDelta[2]);
	if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 1000){scale = 5;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 5462){scale = 6;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 10924){scale = 7;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 16386){scale = 8;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 21848){scale = 9;
	}else if((cent->currentState.number == ps->clientNum) && ps->powerLevel[plCurrent] <= 27310){scale = 10;
	}else{scale = 11;
	}
	CG_PlayerSplash(cent,10*scale);
	//if(!cg.renderingThirdPerson){renderfx |= RF_THIRD_PERSON;}
	//else if(cg_cameraMode.integer){return;}
	if ((cent->currentState.playerBitFlags & usingZanzoken) || ((cent->currentState.number == ps->clientNum) && (ps->bitFlags & usingZanzoken) && !(ps->bitFlags & isUnconcious) && !(ps->bitFlags & isDead) && !(ps->bitFlags & isCrashed))) {
		return;
	}
	CG_PlayerSprites(cent);
	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
							  &torso.oldframe, &torso.frame, &torso.backlerp,
							  &head.oldframe, &head.frame, &head.backlerp );
	shadow = CG_PlayerShadow(cent,&shadowPlane);
	if (cg_shadows.integer == 3 && shadow){
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
	legs.hModel = ci->legsModel[tier];
	legs.customSkin = ci->legsSkin[tier];
	VectorCopy(cent->lerpOrigin,legs.origin);
	VectorCopy(cent->lerpOrigin,legs.lightingOrigin);
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all
	memcpy(&altLegs, &legs, sizeof(legs));
	if (!legs.hModel){return;}
	if(ci->usingMD4==qtrue){
		memcpy(&altTorso, &torso, sizeof(torso));
		oframes[0] = altLegs.oldframe;
		oframes[1] = altTorso.oldframe;
		oframes[2] = oframes[1];
		nframes[0] = altLegs.frame;
		nframes[1] = altTorso.frame;
		nframes[2] = nframes[1];
		lerps[0] = 1.0 - altLegs.backlerp;
		lerps[1] = 1.0 - altTorso.backlerp;
		lerps[2] = lerps[1];
		angles[0][PITCH] = 0;
		angles[0][YAW] = 0;
		angles[0][ROLL] = 0;
		angles[1][PITCH] = cent->pe.torso.pitchAngle - cent->pe.legs.pitchAngle;
		angles[1][YAW] = cent->pe.torso.yawAngle - cent->pe.legs.yawAngle;
		angles[1][ROLL] = 0;
		angles[2][PITCH] = cent->lerpAngles[PITCH];
		angles[2][YAW] = cent->lerpAngles[YAW] - cent->pe.legs.yawAngle;
		angles[2][ROLL] = 0;
		trap_R_SetBlendPose(&altLegs.skel,altLegs.hModel,oframes,nframes,lerps,angles);
		altLegs.renderfx |= RF_SKEL;
		memcpy( &(cent->pe.legsRef ), &legs , sizeof(refEntity_t));
		memcpy( &playerInfoDuplicate[cent->currentState.number], &cent->pe, sizeof(playerEntity_t));
		CG_BreathPuffs(cent,&legs);
		CG_AddRefExtEntityWithPowerups( &altLegs, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
		CG_AddPlayerWeaponMD4(&altLegs,NULL,cent,ci->team);
		CG_PlayerPowerups(cent,&legs);
	}
	else{
		torso.hModel = ci->torsoModel[tier];
		torso.customSkin = ci->torsoSkin[tier];
		if(!torso.hModel){return;}
		VectorCopy( cent->lerpOrigin, torso.lightingOrigin );
		//CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");
		CG_PositionRotatedEntityOnTag( &torso, &legs, legs.hModel, "tag_torso");
		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;
		head.hModel = ci->headModel[tier];
		head.customSkin = ci->headSkin[tier];
		if(!head.hModel){return;}
		VectorCopy( cent->lerpOrigin, head.lightingOrigin );
		CG_PositionRotatedEntityOnTag( &head, &torso, torso.hModel, "tag_head");
		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;
		CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
		CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
		CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team, ci->auraConfig[tier]->auraAlways );
		CG_BreathPuffs(cent,&head);
		memcpy( &(cent->pe.headRef ), &head , sizeof(refEntity_t));
		memcpy( &(cent->pe.torsoRef), &torso, sizeof(refEntity_t));
		memcpy( &(cent->pe.legsRef ), &legs , sizeof(refEntity_t));
		memcpy( &playerInfoDuplicate[cent->currentState.number], &cent->pe, sizeof(playerEntity_t));
		if(onBodyQue){return;}
		CG_AddPlayerWeapon(&torso,NULL,cent,ci->team);
		CG_PlayerPowerups(cent,&torso);
	}
		if((cent->currentState.eFlags & EF_AURA) || ci->auraConfig[tier]->auraAlways){
			CG_AuraStart(cent);
			if(!xyzspeed){CG_PlayerDirtPush(cent,scale*2,qfalse);}
			if(ps->powerLevel[plCurrent] == ps->powerLevel[plMaximum] && ps->bitFlags & usingAlter){
				CG_AddEarthquake(cent->lerpOrigin, 400, 1, 1, 1, 25);
			}
		}
		else{CG_AuraEnd(cent);}
		CG_AddAuraToScene(cent);
		if(ps->timers[tmKnockback]){
			if(ps->timers[tmKnockback] > 0){
				trap_S_StartSound(cent->lerpOrigin,ENTITYNUM_NONE,CHAN_BODY,cgs.media.knockbackSound);
				trap_S_AddLoopingSound(cent->currentState.number,cent->lerpOrigin,vec3_origin,cgs.media.knockbackLoopSound);
			}
			return;
		}
		if(ci->auraConfig[tier]->showLightning){CG_LightningEffect(cent->lerpOrigin, ci, tier);}
		if(ci->auraConfig[tier]->showLightning && ps->bitFlags & usingMelee){CG_BigLightningEffect(cent->lerpOrigin);}
	
}
qboolean CG_GetTagOrientationFromPlayerEntityHeadModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );
	if(cgs.clientinfo[clientNum].usingMD4) {
		Com_Printf("Trying to get an zMesh Tag called %s!\n", tagName);
	}

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->headRef.hModel, pe->head.oldFrame, pe->head.frame, 1.0 - pe->head.backlerp, tagName ) ) {
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}
		Com_Printf("Found tag %s\n", tagName);
		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );

		return qtrue;
	} else if(pe->legsRef.renderfx & RF_SKEL) {
		Com_Printf("Trying to get an zMesh Tag\n");
		/*for(i=0;i<pe->legsRef.skel.numBones;i++)
		{
			if(!strcmp(pe->headRef.skel.bones[i].name,tagName))
			{
				VectorCopy(pe->headRef.skel.bones[i].origin,lerped.origin);
				VectorCopy(pe->headRef.skel.bones[i].axis[0],lerped.axis[0]);
				VectorCopy(pe->headRef.skel.bones[i].axis[1],lerped.axis[1]);
				VectorCopy(pe->headRef.skel.bones[i].axis[2],lerped.axis[2]);
			}
		}
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );
		return qtrue;*/
	}

	// If we didn't find the tag, return false
	return qfalse;
}


qboolean CG_GetTagOrientationFromPlayerEntityTorsoModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->torsoRef.hModel, pe->torso.oldFrame, pe->torso.frame, 1.0 - pe->torso.backlerp, tagName ) ) {
		VectorCopy( pe->torsoRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->torsoRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->torsoRef.axis, tagOrient->axis );

		return qtrue;
	}/* else if(pe->torsoRef.renderfx & RF_SKEL) {
		for(i=0;i<pe->torsoRef.skel.numBones;i++)
		{
			if(!strcmp(pe->torsoRef.skel.bones[i].name,tagName))
			{
				VectorCopy(pe->torsoRef.skel.bones[i].origin,lerped.origin);
				VectorCopy(pe->torsoRef.skel.bones[i].axis[0],lerped.axis[0]);
				VectorCopy(pe->torsoRef.skel.bones[i].axis[1],lerped.axis[1]);
				VectorCopy(pe->torsoRef.skel.bones[i].axis[2],lerped.axis[2]);
			}
		}
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );
		return qtrue;
	}*/

	// If we didn't find the tag, return false
	return qfalse;
}

qboolean CG_GetTagOrientationFromPlayerEntityLegsModel( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->legsRef.hModel, pe->legs.oldFrame, pe->legs.frame, 1.0 - pe->legs.backlerp, tagName ) ) {
		VectorCopy( pe->legsRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->legsRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->legsRef.axis, tagOrient->axis );

		return qtrue;
	}/* else if(pe->legsRef.renderfx & RF_SKEL) {
		for(i=0;i<pe->legsRef.skel.numBones;i++)
		{
			if(!strcmp(pe->legsRef.skel.bones[i].name,tagName))
			{
				VectorCopy(pe->legsRef.skel.bones[i].origin,lerped.origin);
				VectorCopy(pe->legsRef.skel.bones[i].axis[0],lerped.axis[0]);
				VectorCopy(pe->legsRef.skel.bones[i].axis[1],lerped.axis[1]);
				VectorCopy(pe->legsRef.skel.bones[i].axis[2],lerped.axis[2]);
			}
		}
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );
		return qtrue;
	}*/

	// If we didn't find the tag, return false
	return qfalse;
}

/*
===============
CG_GetTagOrientationFromPlayerEntity

If the entity in question is of the type CT_PLAYER, this gets the orientation of a tag on the player's model
and stores it in tagOrient. If the entity is of a different type, the tag is not found, or the model is on
the bodyQue and belongs to a disconnected client with invalid clientInfo, false is returned. In other cases
true is returned.
===============
*/
qboolean CG_GetTagOrientationFromPlayerEntity( centity_t *cent, char *tagName, orientation_t *tagOrient ) {
	if ( CG_GetTagOrientationFromPlayerEntityTorsoModel( cent, tagName, tagOrient ))
		return qtrue;
	if ( CG_GetTagOrientationFromPlayerEntityHeadModel( cent, tagName, tagOrient ))
		return qtrue;
	if ( CG_GetTagOrientationFromPlayerEntityLegsModel( cent, tagName, tagOrient ))
		return qtrue;

	return qfalse;

/*
	int				i, clientNum;
	orientation_t	lerped;
	vec3_t			tempAxis[3];
	playerEntity_t	*pe;

	if ( cent->currentState.eType != ET_PLAYER ) {
		return qfalse;
	}

	if ( !tagName[0] ) {
		return qfalse;
	}

	// The client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	
	// It is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !cgs.clientinfo[clientNum].infoValid ) {
		return qfalse;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	AxisClear( tagOrient->axis );

	// Step through the sections of the model to find which one holds the tag, and calculate the orientation_t
	if ( trap_R_LerpTag( &lerped, pe->torsoRef.hModel, pe->torso.oldFrame, pe->torso.frame, 1.0 - pe->torso.backlerp, tagName ) ) {
		VectorCopy( pe->torsoRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->torsoRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->torsoRef.axis, tagOrient->axis );

		return qtrue;
	}
	if ( trap_R_LerpTag( &lerped, pe->headRef.hModel, pe->head.oldFrame, pe->head.frame, 1.0 - pe->head.backlerp, tagName ) ) {
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );

		return qtrue;
	}
	if ( trap_R_LerpTag( &lerped, pe->legsRef.hModel, pe->legs.oldFrame, pe->legs.frame, 1.0 - pe->legs.backlerp, tagName ) ) {
		VectorCopy( pe->legsRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->legsRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->legsRef.axis, tagOrient->axis );

		return qtrue;
	}

	// If we didn't find the tag in any of the model's parts, return false
	return qfalse;
*/

}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	

	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim );
	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );

	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

void CG_SpawnLightSpeedGhost( centity_t *cent ) {
	if (random() < 0.2){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound1 );
	}else if (random() < 0.4){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound2 );
	}else if (random() < 0.6){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound3 );
	}else if ( random() < 0.8){
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound4 );
	} else {
		trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, cgs.media.lightspeedSound5 );
	}
}

