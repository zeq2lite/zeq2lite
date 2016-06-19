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
// cg_consolecmds.c -- text commands typed in at the local console, or
// executed by a key binding

#include "cg_local.h"
#include "../UI/ui_shared.h"


void CG_TargetCommand_f( void ) {
	int		targetNum;
	char	test[4];

	targetNum = CG_CrosshairPlayer();
	if ( targetNum == -1 ) {
		return;
	}

	trap_Argv( 1, test, 4 );
	trap_SendClientCommand( va( "gc %i %i", targetNum, atoi( test ) ) );
}

/*
=================
CG_SizeUp_f

Keybinding command
=================
*/
static void CG_SizeUp_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer+10)));
}


/*
=================
CG_SizeDown_f

Keybinding command
=================
*/
static void CG_SizeDown_f (void) {
	trap_Cvar_Set("cg_viewsize", va("%i",(int)(cg_viewsize.integer-10)));
}


/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f (void) {
	CG_Printf ("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
		(int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2], 
		(int)cg.refdefViewAngles[YAW]);
}

/*
=================
JUHOX: CG_SaveLensFlareEntities_f
=================
*/
#if MAPLENSFLARES
static void CG_SaveLensFlareEntities_f(void) {
	char mapname[256];
	char name[256];
	fileHandle_t f;
	int i;
	int n;

	if (cgs.editMode != EM_mlf) return;

	trap_Cvar_VariableStringBuffer("mapname", mapname, sizeof(mapname));
	Com_sprintf(name, sizeof(name), "maps/%s.lfe", mapname);

	trap_FS_FOpenFile(name, &f, FS_WRITE);
	if (!f) {
		CG_Printf("Could not create '%s'\n", name);
		return;
	}
	CG_Printf("writing '%s'...\n", name);

	n = 0;
	for (i = 0; i < cgs.numLensFlareEntities; i++) {
		const lensFlareEntity_t* lfent;
		char buf[512];
		char lockOption[16];
		char lightRadius[16];

		lfent = &cgs.lensFlareEntities[i];
		if (!lfent->lfeff) continue;

		if (lfent->lock) {
			Com_sprintf(lockOption, sizeof(lockOption), "mv %d ", lfent->lock->currentState.number);
		}
		else {
			lockOption[0] = 0;
		}

		if (lfent->lightRadius > 1) {
			Com_sprintf(lightRadius, sizeof(lightRadius), "lr %f ", lfent->lightRadius);
		}
		else {
			lightRadius[0] = 0;
		}

		Com_sprintf(
			buf, sizeof(buf), "{ %s %f %f %f %f %f %f %f %f %s%s}\n",
			lfent->lfeff->name,
			lfent->origin[0], lfent->origin[1], lfent->origin[2],
			lfent->radius,
			lfent->dir[0], lfent->dir[1], lfent->dir[2],
			lfent->angle,
			lockOption, lightRadius
		);
		trap_FS_Write(buf, strlen(buf), f);

		n++;
	}
	trap_FS_FCloseFile(f);

	CG_Printf("%d lens flare entities saved\n", n);
}
#endif

/*
=================
JUHOX: CG_RevertLensFlareEntities_f
=================
*/
#if MAPLENSFLARES
static void CG_RevertLensFlareEntities_f(void) {
	if (cgs.editMode != EM_mlf) return;

	CG_LoadLensFlareEntities();
}
#endif

/*
=================
JUHOX: CG_UpdateLensFlares_f
=================
*/
#if MAPLENSFLARES
static void CG_UpdateLensFlares_f(void) {
	if (cgs.editMode != EM_mlf) return;

	CG_SaveLensFlareEntities_f();
	CG_LoadLensFlares();
	CG_LoadLensFlareEntities();
}
#endif



/*
==================
CG_Draw2D_f
Example : draw2D <shader> <x> <y> <width> <height> <duration>
==================
*/
static void CG_Draw2D_f(void){
	char shaderName[256];
	overlay2D *current;
	int index;
	if(trap_Argc()<2){
		CG_Printf("^3Usage : ^7draw2D <^5shaderName^7> <^5x^7> <^5y^7> <^5width^7> <^5height^7> <^5duration^7>\n");
		return;
	}
	Q_strncpyz(shaderName,CG_Argv(1),256);
	for(index=0;index<16;++index){
		if(!cg.scripted2D[index].active){break;}
		if(index == 15){
			CG_Printf("^3Warning : ^7Cannot add overlay.  Maximum overlay surfaces of [^3%i^7] active.\n",16);
			return;
		}
	}
	current = &cg.scripted2D[index];
	current->shader = trap_R_RegisterShader(shaderName);
	if(!current->shader){
		CG_Printf("^1Error : ^7Shader by the name [^1%s^7] could not be found.\n",shaderName);
		return;
	}
	current->active = qtrue;	
	current->x = atoi(CG_Argv(2));
	current->y = atoi(CG_Argv(3));
	current->width = atoi(CG_Argv(4)) ? atoi(CG_Argv(4)) : 640;
	current->height = atoi(CG_Argv(5)) ? atoi(CG_Argv(5)) : 480;
	current->endTime = atoi(CG_Argv(6)) ? atoi(CG_Argv(6)) : -1;
	if(current->endTime != -1){
		current->endTime += cg.time;
	}
}

/*
==================
CG_StartOrbit_f
==================
*/

static void CG_StartOrbit_f( void ) {
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "developer", var, sizeof( var ) );
	if ( !atoi(var) ) {
		return;
	}
	if (cg_cameraOrbit.value != 0) {
		trap_Cvar_Set ("cg_cameraOrbit", "0");
		trap_Cvar_Set("cg_thirdPerson", "0");
	} else {
		trap_Cvar_Set("cg_cameraOrbit", "5");
		trap_Cvar_Set("cg_thirdPerson", "1");
		trap_Cvar_Set("cg_thirdPersonAngle", "0");
		trap_Cvar_Set("cg_thirdPersonRange", "100");
	}
}

/*
static void CG_Camera_f( void ) {
	char name[1024];
	trap_Argv( 1, name, sizeof(name));
	if (trap_loadCamera(name)) {
		cg.cameraMode = qtrue;
		trap_startCamera(cg.time);
	} else {
		CG_Printf ("Unable to load camera %s\n",name);
	}
}
*/
typedef struct {
	char	*cmd;
	void	(*function)(void);
} consoleCommand_t;

static consoleCommand_t	commands[] = {
	{ "viewpos", CG_Viewpos_f },
	{ "+zoom", CG_ZoomDown_f },
	{ "-zoom", CG_ZoomUp_f },
	{ "sizeup", CG_SizeUp_f },
	{ "sizedown", CG_SizeDown_f },
	{ "weapnext", CG_NextWeapon_f },
	{ "weapprev", CG_PrevWeapon_f },
	{ "weapon", CG_Weapon_f },
	{ "tiernext", CG_NextTier_f },
	{ "tierprev", CG_PrevTier_f },
	{ "tier", CG_Tier_f },
	{ "tcmd", CG_TargetCommand_f },
	{ "startOrbit", CG_StartOrbit_f },
	{ "draw2D", CG_Draw2D_f },
	{ "draw2d", CG_Draw2D_f },
	/*{ "draw2DTween", CG_Draw2DTween_f },
	{ "draw2dTween", CG_Draw2DTween_f },
	{ "cameraTween", CG_Camera_f },
	{ "queue", CG_Queue_f },*/
	{ "loaddeferred", CG_LoadDeferredPlayers }
};


/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand( void ) {
	const char	*cmd;
	int		i;

	cmd = CG_Argv(0);

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		if ( !Q_stricmp( cmd, commands[i].cmd ) ) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}


/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands( void ) {
	int		i;

	for ( i = 0 ; i < ARRAY_LEN( commands ) ; i++ ) {
		trap_AddCommand( commands[i].cmd );
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand ("say");
	trap_AddCommand ("tell");
	trap_AddCommand ("votell");
	trap_AddCommand ("follow");
	trap_AddCommand ("levelshot");
	trap_AddCommand ("loaddefered");	// spelled wrong, but not changing for demo
}
