// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int			i;
	centity_t	*cent;
	snapshot_t	*snap;
	entityState_t	*ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr ) {
	int			i, x, zd, zu;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, cg.physicsTime, origin );
		} else {
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		trap_CM_TransformedBoxTrace ( &trace, start, end,
			mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask ) {
	trace_t	t;

	trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
JUHOX: CG_SmoothTrace
================
*/
void CG_SmoothTrace(
	trace_t *result,
	const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
	int skipNumber, int mask
) {
	int physicsTime;

	physicsTime = cg.physicsTime;
	cg.physicsTime = cg.time;
	CG_Trace(result, start, mins, maxs, end, skipNumber, mask);
	cg.physicsTime = cg.time;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, ent->origin, ent->angles );
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState(qboolean grabAngles) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;
	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;
	*out = cg.snap->ps;
	// if we are still allowing local input, short circuit the view angles
	if(grabAngles){
		usercmd_t	cmd;
		int	cmdNum;
		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd(cmdNum,&cmd);
		PM_UpdateViewAngles(out,&cmd);
	}
	if(cg.nextFrameTeleport){return;}
	if(!next || next->serverTime <= prev->serverTime){return;}
	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );
	i = next->ps.bobCycle;
	if(i<prev->ps.bobCycle){i += 256;}
	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );
	for(i=0;i<3;i++){
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i]);
		if(!grabAngles){
			out->viewangles[i] = LerpAngle(prev->ps.viewangles[i],next->ps.viewangles[i],f);
		}
		out->velocity[i] = prev->ps.velocity[i] + f * (next->ps.velocity[i] - prev->ps.velocity[i]);
	}
}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	return;
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[powerLevelTotal] <= 0 ) {
		return;
	}

	// JUHOX: don't touch triggers in lens flare editor
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) return;
#endif

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, 
			cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
			BG_TouchJumpPad( &cg.predictedPlayerState, ent );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount ) {
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}



/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState( void ) {
	int			cmdNum, current;
	playerState_t	oldPlayerState;
	qboolean	moved;
	usercmd_t	oldestCmd;
	usercmd_t	latestCmd;

	cg.hyperspace = qfalse;
	if(!cg.validPPS){
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
	}
	if(cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState(qfalse);
		return;
	}
	if(cg.snap->ps.lockedTarget > 0){
		CG_InterpolatePlayerState(qtrue);
		return;
	}
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	cg_pmove.tracemask = MASK_PLAYERSOLID;
	if(cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;
	}
#if MAPLENSFLARES	// JUHOX: set player tracemask for lens flare editor
	if (cgs.editMode == EM_mlf) {
		cg_pmove.tracemask = 0;
	}
#endif
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;
	oldPlayerState = cg.predictedPlayerState;
	current = trap_GetCurrentCmdNumber();
	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd );
	if (oldestCmd.serverTime > cg.snap->ps.commandTime && oldestCmd.serverTime < cg.time){
		if(cg_showmiss.integer){
			CG_Printf ("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}
	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd );
	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to 
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		cg.predictedPlayerState = cg.nextSnap->ps;
		cg.physicsTime = cg.nextSnap->serverTime;
	} else {
		cg.predictedPlayerState = cg.snap->ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer;// | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

	// run cmds
	moved = qfalse;
	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd );

		if ( cg_pmove.pmove_fixed ) {
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime ) {
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showmiss.integer ) {
					CG_Printf( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t	adjusted;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
					cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted );

				if ( cg_showmiss.integer ) {
					if (!VectorCompare( oldPlayerState.origin, adjusted )) {
						CG_Printf("prediction error\n");
					}
				}
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showmiss.integer ) {
						CG_Printf("Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showmiss.integer ) {
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		if (
			cg.predictedPlayerState.stats[target] >= 0 &&
			cg.predictedPlayerState.stats[target] < ENTITYNUM_MAX_NORMAL
		) {
			centity_t* theTarget;
			float pos;

			theTarget = &cg_entities[cg.predictedPlayerState.stats[target]];
			BG_EvaluateTrajectory(&theTarget->currentState,&theTarget->currentState.pos, cg.time, cg_pmove.target);
			//cg_pmove.target[2] += BG_PlayerTargetOffset(&theTarget->currentState, pos);
		}

#if MAPLENSFLARES	// JUHOX: lens flare editor movement
		if (
			cgs.editMode == EM_mlf &&
			cg.lfEditor.cmdMode == LFECM_main
		) {
			if (
				!cg.lfEditor.selectedLFEnt &&
				cg.lfEditor.markedLFEnt >= 0 &&
				cg.lfEditor.lastClick < cg.time - 100 &&
				(cg.lfEditor.oldButtons & BUTTON_ATTACK) == 0 &&
				(cg_pmove.cmd.buttons & BUTTON_ATTACK)
			) {
				CG_SelectLFEnt(cg.lfEditor.markedLFEnt);
			}
			else if (
				cg.lfEditor.selectedLFEnt &&
				cg.lfEditor.editMode == LFEEM_none &&
				cg.lfEditor.lastClick < cg.time - 100 &&
				(cg.lfEditor.oldButtons & BUTTON_ATTACK) == 0 &&
				(cg_pmove.cmd.buttons & BUTTON_ATTACK)
			) {
				cg.lfEditor.selectedLFEnt = NULL;
				CG_SetLFEdMoveMode(LFEMM_coarse);
			}

			if (
				cg.lfEditor.editMode == LFEEM_pos &&
				cg.lfEditor.moveMode == LFEMM_coarse &&
				(cg_pmove.cmd.buttons & BUTTON_ATTACK)
			) {
				CG_SetLFEdMoveMode(LFEMM_fine);
			}

			if (
				cg.lfEditor.editMode == LFEEM_pos &&
				cg.lfEditor.moveMode == LFEMM_fine &&
				cg.lfEditor.selectedLFEnt &&
				(cg_pmove.cmd.buttons & BUTTON_ATTACK)
			) {
				float move;

				move = cg_pmove.cmd.forwardmove * (cg_pmove.cmd.serverTime - cg_pmove.ps->commandTime) / 2000.0;
				cg.lfEditor.fmm_distance -= move;
				if (cg.lfEditor.fmm_distance < 20) cg.lfEditor.fmm_distance = 20;
				if (cg.lfEditor.fmm_distance > 300) cg.lfEditor.fmm_distance = 300;

				move = cg_pmove.cmd.rightmove * (cg_pmove.cmd.serverTime - cg_pmove.ps->commandTime) / 8000.0;
				cg.lfEditor.selectedLFEnt->radius += move;
				if (cg.lfEditor.selectedLFEnt->radius < 2.5) cg.lfEditor.selectedLFEnt->radius = 2.5;
				if (cg.lfEditor.selectedLFEnt->radius > 100) cg.lfEditor.selectedLFEnt->radius = 100;

				cg_pmove.cmd.forwardmove = 0;
				cg_pmove.cmd.rightmove = 0;
				cg_pmove.cmd.upmove = 0;
			}

			if (
				cg.lfEditor.editMode == LFEEM_target &&
				cg.lfEditor.moveMode == LFEMM_fine
			) {
				float move;

				move = cg_pmove.cmd.forwardmove * (cg_pmove.cmd.serverTime - cg_pmove.ps->commandTime) / 2000.0;
				cg.lfEditor.fmm_distance -= move;
				if (cg.lfEditor.fmm_distance < 20) cg.lfEditor.fmm_distance = 20;
				if (cg.lfEditor.fmm_distance > 300) cg.lfEditor.fmm_distance = 300;

				cg_pmove.cmd.forwardmove = 0;
				cg_pmove.cmd.rightmove = 0;
				cg_pmove.cmd.upmove = 0;
			}

			if (
				cg.lfEditor.selectedLFEnt &&
				cg.lfEditor.editMode == LFEEM_target &&
				cg.lfEditor.editTarget &&
				cg.lfEditor.lastClick < cg.time - 100 &&
				(cg.lfEditor.oldButtons & BUTTON_ATTACK) == 0 &&
				(cg_pmove.cmd.buttons & BUTTON_ATTACK)
			) {
				vec3_t dir;

				CG_LFEntOrigin(cg.lfEditor.selectedLFEnt, dir);
				VectorSubtract(cg.refdef.vieworg, dir, dir);
				if (VectorNormalize(dir) > 0.1) {
					VectorCopy(dir, cg.lfEditor.selectedLFEnt->dir);
					CG_ComputeMaxVisAngle(cg.lfEditor.selectedLFEnt);
				}
				cg.lfEditor.editTarget = qfalse;
				VectorCopy(cg.refdef.vieworg, cg.lfEditor.targetPosition);
			}
			else if(
				cg.lfEditor.selectedLFEnt &&
				cg.lfEditor.editMode == LFEEM_target &&
				!cg.lfEditor.editTarget &&
				cg.lfEditor.lastClick < cg.time - 100 &&
				(cg.lfEditor.oldButtons & BUTTON_ATTACK) == 0 &&
				(cg_pmove.cmd.buttons & BUTTON_ATTACK)
			){
				if(Distance(cg.refdef.vieworg, cg.lfEditor.targetPosition) >= 1){
					vec3_t origin;
					vec3_t dir;
					CG_LFEntOrigin(cg.lfEditor.selectedLFEnt, origin);
					VectorSubtract(cg.refdef.vieworg, origin, dir);
					if (VectorNormalize(dir) > 0.1){
						cg.lfEditor.selectedLFEnt->angle = acos(DotProduct(dir, cg.lfEditor.selectedLFEnt->dir)) * (180.0 / M_PI);
					}
				}
				else{
					cg.lfEditor.selectedLFEnt->angle = -1;
				}
				CG_ComputeMaxVisAngle(cg.lfEditor.selectedLFEnt);
				cg.lfEditor.editMode = LFEEM_none;
				CG_SetLFEdMoveMode(LFEMM_coarse);
			}
			if(cg.lfEditor.selectedLFEnt && cg.lfEditor.editMode == LFEEM_radius){
				float move;
				if (cg_pmove.cmd.buttons & BUTTON_ATTACK) {
					move = cg_pmove.cmd.forwardmove * (cg_pmove.cmd.serverTime - cg_pmove.ps->commandTime) / 2000.0;
					cg.lfEditor.fmm_distance -= move;
					if (cg.lfEditor.fmm_distance < 20) cg.lfEditor.fmm_distance = 20;
					if (cg.lfEditor.fmm_distance > 300) cg.lfEditor.fmm_distance = 300;					
				}
				else {
					move = cg_pmove.cmd.forwardmove * (cg_pmove.cmd.serverTime - cg_pmove.ps->commandTime) / 8000.0;
					cg.lfEditor.selectedLFEnt->lightRadius += move;
					if (cg.lfEditor.selectedLFEnt->lightRadius < 2) {
						if (move > 0) {
							cg.lfEditor.selectedLFEnt->lightRadius = 2;
						}
						else {
							cg.lfEditor.selectedLFEnt->lightRadius = 0.5;
						}
					}
					if (cg.lfEditor.selectedLFEnt->lightRadius > cg.lfEditor.selectedLFEnt->radius) {
						cg.lfEditor.selectedLFEnt->lightRadius = cg.lfEditor.selectedLFEnt->radius;
					}
					
					move = cg_pmove.cmd.rightmove * (cg_pmove.cmd.serverTime - cg_pmove.ps->commandTime) / 8000.0;
					cg.lfEditor.selectedLFEnt->radius += move;
					if (cg.lfEditor.selectedLFEnt->radius < 2.5) cg.lfEditor.selectedLFEnt->radius = 2.5;
					if (cg.lfEditor.selectedLFEnt->radius > 100) cg.lfEditor.selectedLFEnt->radius = 100;
				}
				cg_pmove.cmd.forwardmove = 0;
				cg_pmove.cmd.rightmove = 0;
				cg_pmove.cmd.upmove = 0;
			}
			if(cg.lfEditor.oldButtons != cg_pmove.cmd.buttons){
				cg.lfEditor.lastClick = cg.time;
			}
			cg.lfEditor.oldButtons = cg_pmove.cmd.buttons;
		}
#endif

		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		}

		Pmove (&cg_pmove);

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);
	}

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		if ( cg_showmiss.integer ) {
			CG_Printf( "not moved\n" );
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover(cg.predictedPlayerState.origin,cg.predictedPlayerState.groundEntityNum,cg.physicsTime, cg.time, cg.predictedPlayerState.origin);
	if(cg_showmiss.integer){
		if(cg.predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS){
			CG_Printf("WARNING: dropped event\n");
		}
	}
	// fire events and other transition triggered things
	CG_TransitionPlayerState(&cg.predictedPlayerState,&oldPlayerState);
	if ( cg_showmiss.integer ) {
		if (cg.eventSequence > cg.predictedPlayerState.eventSequence) {
			CG_Printf("WARNING: double event\n");
			cg.eventSequence = cg.predictedPlayerState.eventSequence;
		}
	}
}


