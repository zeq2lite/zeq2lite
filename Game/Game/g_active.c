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

#include "g_local.h"
#include "bg_local.h"
/*===============
P_DamageFeedback
===============*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
	vec3_t	angles;

	client = player->client;
	// total points of damage shot at the player this frame
	count = client->damage_blood; // + client->damage_armor;
	if ( count == 0 ) { // didn't take any damage
		// ADDING FOR ZEQ2
		// but we DO need to reset this to 0 for the deductHP to work!
		client->ps.damageCount = 0;
		return;		
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->powerLevelTotal );
		client->ps.damageEvent++;
	}


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	//client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	ent->client->ps.loopSound = 0;
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

#if MAPLENSFLARES	// JUHOX: never touch triggers in lens flare editor
	if (g_editmode.integer == EM_mlf) return;
#endif
	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
		}
		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.stats[stSpeed] = 400;	// faster than normal

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;
		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );
		// END ADDING

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & BUTTON_ATTACK) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int			i, j;
	int			event;
	int 		tier;
	int			distance;
	int 		enemyMelee,playerMelee;
	gclient_t	*client;
	gentity_t	*enemy;
	int			damage;
	int			amount;
	vec3_t		dir;
	vec3_t		origin, angles;
//	qboolean	fired;
	gentity_t	*drop;
	playerState_t *ps;
	playerState_t *enemyPS;
	// ADDING FOR ZEQ2
	gentity_t	*missile;
	// END ADDING
	client = ent->client;
	ps = &client->ps;
	if(ps->lockedTarget>0){
		enemyPS = &g_entities[ps->lockedTarget-1].client->ps;
	}
	tier = ps->powerLevel[plTierCurrent];
	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];
		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			break;
		case EV_FIRE_WEAPON:
			FireWeapon(ent,qfalse);
			break;
		case EV_CHANGE_WEAPON:
			break;
		case EV_ZANZOKEN_START:
			if(!ps->bitFlags & usingMelee){}
			break;
		case EV_ALTFIRE_WEAPON:
			FireWeapon(ent,qtrue);
			break;
		case EV_DETONATE_WEAPON:
			missile = client->guidetarget;
			G_DetachUserWeapon(missile);
			break;
		case EV_BALLFLIP:
			break;
		case EV_MELEE_CHECK:
			if(ps->lockedTarget>0){
				if(!&g_entities[ps->lockedTarget-1].client || &g_entities[ps->lockedTarget-1].client->pers.connected == CON_DISCONNECTED){
					ps->lockedPosition = 0;
					ps->lockedPlayer = 0;
					ps->lockedTarget = 0;
					break;
				}
				if(ps->lockedPlayer->bitFlags & isStruggling || ps->lockedPlayer->bitFlags & isDead || ps->lockedPlayer->bitFlags & isUnconcious ||
				   ps->lockedPlayer->bitFlags & isTransforming || ps->lockedPlayer->bitFlags & isCrashed){
					ps->lockedPosition = 0;
					ps->lockedPlayer = 0;
					ps->lockedTarget = 0;
					break;
				}
				ps->lockedPosition = &g_entities[ps->lockedTarget-1].r.currentOrigin;
				ps->lockedPlayer = &g_entities[ps->lockedTarget-1].client->ps;
			}
			break;
		case EV_MELEE_SPEED:
			break;
		case EV_MELEE_MISS:
			break;
		case EV_MELEE_KNOCKBACK:
			break;
		case EV_MELEE_STUN:
			break;
		case EV_MELEE_KNOCKOUT:
			enemy = &g_entities[ps->lockedTarget-1];
			enemy->enemy = ent;
			break;
		case EV_TIERCHECK:
			checkTier(client);
			break;
		case EV_TIERUP_FIRST:
		case EV_TIERUP:
		case EV_TIERDOWN:
			break;
		case EV_SYNCTIER:
			syncTier(client);
			break;
		case EV_ALTERUP_START:
			break;
		case EV_ALTERDOWN_START:
			break;
		case EV_POWERINGUP_START:
			break;
		case EV_BOOST_START:
			break;
		case EV_LOCKON_START:
			break;
		case EV_LOCKON_END:
			break;
		case EV_DEATH:
			ps->powerLevel[plTierCurrent] = 0;
			ent->r.contents &= ~CONTENTS_BODY;
			syncTier(client);
			client->respawnTime = level.time + 10000;
			break;
		case EV_UNCONCIOUS:
			ps->powerLevel[plTierCurrent] = 0;
			syncTier(client);
			break;
		default:
			break;
		}
	}

}
void BotTestSolid(vec3_t origin);

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s,qtrue );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

void G_CheckSafety(vec3_t origin,playerState_t *ps){
	/*int radius,distance;
	int	entityList[MAX_GENTITIES];
	int areaEntities;
	int i,e;
	gentity_t *ent;
	vec3_t mins,maxs,deltaRadius;
	if(ps->bitFlags & isTargeted || ps->lockedTarget > 0){
		ps->bitFlags &= ~isSafe;
		ps->timers[tmSafe] = level.time;
		return;
	}
	for(i=0;i<3;i++){
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}
	radius = 5000;
	areaEntities = trap_EntitiesInBox(mins,maxs,entityList,MAX_GENTITIES);
	for(e=0;e<areaEntities;e++){
		ent = &g_entities[entityList[e]];
		for(i=0;i<3;i++){
			if(origin[i]<ent->r.absmin[i]){deltaRadius[i] = ent->r.absmin[i] - origin[i];}
			else if(origin[i]>ent->r.absmax[i]){deltaRadius[i] = origin[i] - ent->r.absmax[i];}
			else{deltaRadius[i] = 0;}
		}
		distance = VectorLength(deltaRadius);
		if(distance >= radius){continue;}
		if(ent->s.eType == ET_PLAYER && ent->r.currentOrigin != origin){
			ps->bitFlags &= ~isSafe;
			ps->timers[tmSafe] = level.time;
			return;
		}
	}*/
}
/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;
	client = ent->client;
	if (client->pers.connected != CON_CONNECTED){return;}
	ucmd = &ent->client->pers.cmd;
	if ( ucmd->serverTime > level.time + 200 ) {ucmd->serverTime = level.time + 200;}
	if ( ucmd->serverTime < level.time - 1000 ) {ucmd->serverTime = level.time - 1000;} 
	msec = ucmd->serverTime - client->ps.commandTime;
	if(msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW){return;}
	if(msec > 200){msec = 200;}
	if(pmove_msec.integer < 8 ){trap_Cvar_Set("pmove_msec", "8");}
	else if(pmove_msec.integer > 33){trap_Cvar_Set("pmove_msec", "33");}
	if(pmove_fixed.integer || client->pers.pmoveFixed){
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
	}
	if(level.intermissiontime){
		ClientIntermissionThink( client );
		return;
	}
	if(client->sess.sessionTeam == TEAM_SPECTATOR){
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}
	if(!ClientInactivityTimer(client)){return;}
	if(client->noclip){
		client->ps.pm_type = PM_NOCLIP;
	}
	else{
		client->ps.pm_type = PM_NORMAL;
	}
	ent->s.playerBitFlags = client->ps.bitFlags;
	ent->s.attackPowerCurrent = client->ps.powerLevel[plHealth];
	ent->s.attackPowerTotal = client->ps.powerLevel[plMaximum];
	G_LinkUserWeaponData( &(client->ps) );
	G_CheckSkills(&(client->ps));
	G_CheckSafety(ent->r.currentOrigin,&(client->ps));
	if ( client->ps.weapon == WP_GRAPPLING_HOOK &&
		client->hook && !( ucmd->buttons & BUTTON_ATTACK ) ) {
		Weapon_HookFree(client->hook);
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));
	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}
	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	VectorCopy( client->ps.origin, client->oldOrigin );
	Pmove(&pm);
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	SendPendingPredictableEvents(&ent->client->ps);
	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;
	}
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);
	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;
	ClientEvents(ent,oldEventSequence);
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
	ClientImpacts(ent,&pm);
	if(ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;
	if (client->ps.bitFlags & isDead){
		if(level.time > client->respawnTime){respawn(ent);}
		return;
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer && !ent->client->ps.lockedTarget) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer && !ent->client->ps.lockedTarget) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*==================
SpectatorClientEndFrame
==================*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum, flags;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
//	int			i;
	clientPersistant_t	*pers;
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	pers = &ent->client->pers;
	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects( ent );

	// apply all the damage taken this frame
	P_DamageFeedback( ent );

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}
	G_SetClientSound (ent);
	// set the latest infor
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	SendPendingPredictableEvents( &ent->client->ps );

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}


