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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../../Shared/q_shared.h"
#include "bg_public.h"

//======================================================================

#define	QUADB1(t)		(t*t)
#define	QUADB2(t)		(2*t*(1-t))
#define	QUADB3(t)		((1-t)*(1-t))

#define	QUADB1dt(t)		(2*t)
#define	QUADB2dt(t)		(2-4*t)
#define	QUADB3dt(t)		(2*t-2)

void BG_LerpQuadraticSpline( const vec3_t cp1, const vec3_t cp2, const vec3_t cp3, float t, vec3_t lerpBase )
{
	// CP1: Start point
	// CP2: Mid point
	// CP3: End point
	lerpBase[0] = cp1[0]*QUADB1(t) + cp2[0]*QUADB2(t) + cp3[0]*QUADB3(t);
	lerpBase[1] = cp1[1]*QUADB1(t) + cp2[1]*QUADB2(t) + cp3[1]*QUADB3(t);
	lerpBase[2] = cp1[2]*QUADB1(t) + cp2[2]*QUADB2(t) + cp3[2]*QUADB3(t);
}

void BG_LerpQuadraticSplineDelta( const vec3_t cp1, const vec3_t cp2, const vec3_t cp3, float t, vec3_t lerpDelta )
{
	// CP1: Start point
	// CP2: Mid point
	// CP3: End point
	lerpDelta[0] = cp1[0]*QUADB1(t) + cp2[0]*QUADB2(t) + cp3[0]*QUADB3(t);
	lerpDelta[1] = cp1[1]*QUADB1(t) + cp2[1]*QUADB2(t) + cp3[1]*QUADB3(t);
	lerpDelta[2] = cp1[2]*QUADB1(t) + cp2[2]*QUADB2(t) + cp3[2]*QUADB3(t);
	VectorInverse( lerpDelta );
}


/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( entityState_t *es, const trajectory_t *tr, int atTime, vec3_t result ) {
	float			deltaTime;
	float			phase;
	float			v0;
	vec3_t			dir;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;

	case TR_LINEAR:
	case TR_DRUNKEN:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );

		if ( tr->trType == TR_DRUNKEN ) { // trDuration is how much the missile will wobble
			vec3_t	axis[3];
			float	ph1, ph2;

			if ( VectorNormalize2( tr->trDelta, axis[0] ) == 0.0f ) {
				VectorSet( axis[0], 0, 0, 1 );
			}
			MakeNormalVectors( tr->trDelta, axis[1], axis[2] );
			RotateAroundDirection( axis, deltaTime );
			ph1 = sin( deltaTime * M_PI * 2 );
			ph2 = (cos( deltaTime * M_PI * 2 ) - ph1) + cos( deltaTime * M_PI * 4 );
			VectorMA( result, tr->trDuration * ph1, axis[1], result );
			VectorMA( result, tr->trDuration * ph2, axis[2], result );
		}
		break;

	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;

	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;

	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;

	case TR_ACCEL:
		deltaTime = ( atTime - tr->trTime ) * 0.001f;	// milliseconds to seconds

		/*
			NOTE:
			There's a problem with accelerated missiles being able to
			get a negative acceleration and thus 'boomeranging' backwards.
			We want to set a minimum velocity of 0 to prevent this.

			NOTE:
			trDuration = acceleration,
		*/

		VectorCopy( tr->trDelta, dir );
		v0 = VectorNormalize( dir );
		phase = v0 + tr->trDuration * deltaTime;
		if ( phase < 0 ) {

			// We have to find the point in time at which the velocity
			// becomes zero. We solve 0 = v0 + a * t for t.

			deltaTime = -v0 / tr->trDuration;

			// Now we can use this time to get the missile's position
		}

			// the 0.5*a*t^2 part. 
			phase = (tr->trDuration / 2.0f) * (deltaTime * deltaTime);
			VectorMA (tr->trBase, phase, dir, result);

			// The u*t part.
			VectorMA (result, deltaTime, tr->trDelta, result);
		break;

	case TR_ARCH:
		if (!es) {
			Com_Error( ERR_DROP, "BG_EvaluateTrajectory: NULL entityState: %i", tr->trTime );
		} else {
			deltaTime = 1.0f - ( ( atTime - tr->trTime ) / (float) tr->trDuration );
			BG_LerpQuadraticSpline( tr->trBase, es->angles2, tr->trDelta, deltaTime, result );
		}
		break;

	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( entityState_t *es, const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;
	vec3_t	dir;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
	case TR_DRUNKEN:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	case TR_ACCEL:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds

		// Turn magnitude of acceleration into a vector
		VectorCopy(tr->trDelta, dir);
		phase = VectorNormalize (dir);
		phase = phase + tr->trDuration * deltaTime;
		if ( phase < 0 ) { // prevent 'negative' velocity, clamp it at zero.
			VectorSet( dir, 0, 0, 0);
		} else {
			VectorScale (dir, tr->trDuration, dir);
		}

		// u + t * a = v
		VectorMA (tr->trDelta, deltaTime, dir, result);
		break;
	case TR_ARCH:
		if (!es) {
			Com_Error( ERR_DROP, "BG_EvaluateTrajectory: NULL entityState: %i", tr->trTime );
		} else {
			deltaTime = 1.0f - ( ( atTime - tr->trTime ) / (float) tr->trDuration );
			BG_LerpQuadraticSplineDelta( tr->trBase, es->angles2, tr->trDelta, deltaTime, result );
			VectorNormalize( result );

			// Reverse the speed calculation we did in user_missile.c under HOM_ARCH
			VectorScale( result, 1000.0f * Distance( tr->trBase, tr->trDelta ) / (float) tr->trDuration, result );
		}
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",
	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",
	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",
	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_MISSILE_MISS_AIR",
	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player
	"EV_HIGHJUMP",
	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_DETONATE_WEAPON",
	"EV_ALTFIRE_WEAPON",
	"EV_TIERCHECK",
	"EV_TIERUP_FIRST",
	"EV_TIERUP",
	"EV_TIERDOWN",
	"EV_SYNCTIER",
	"EV_ALTERUP_START",
	"EV_ALTERDOWN_START",
	"EV_POWERINGUP_START",
	"EV_BOOST_START",
	"EV_BALLFLIP",
	"EV_MELEE_SPEED",
	"EV_MELEE_KNOCKBACK",
	"EV_MELEE_STUN",
	"EV_MELEE_CHECK",
	"EV_ZANZOKEN_END",
	"EV_ZANZOKEN_START",
	"EV_DRAIN",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_PAIN",
	"EV_DEATH",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_UNCONCIOUS",
	"EV_OBITUARY",
	"EV_SCOREPLUM",			// score plum
	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_LOCKON_START",
	"EV_MELEE_CHECK",
	"EV_LOCKON_END",
	"EV_BEAM_FADE"
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

extern void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
#ifdef GAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s,qboolean snap ) {
	int	i;
	int time;
	time = ps->commandTime;
	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR){s->eType = ET_INVISIBLE;}
	else{s->eType = ET_PLAYER;}
	s->number = ps->clientNum;
	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if(snap){
		SnapVector(s->pos.trBase);
	}
	VectorCopy(ps->velocity,s->pos.trDelta);
	s->pos.trTime = time;
	s->pos.trDuration = 50;
	s->apos.trType = TR_INTERPOLATE;
	if(ps->weaponstate != WEAPON_GUIDING && ps->weaponstate != WEAPON_ALTGUIDING && !(ps->bitFlags & isDead) && !(ps->bitFlags & isUnconcious) && !(ps->bitFlags & isCrashed) && !(ps->bitFlags & usingAlter)){
		VectorCopy(ps->viewangles,s->apos.trBase);
		if(snap){SnapVector(s->apos.trBase);}
	}
	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;
	s->eFlags = ps->eFlags;
	if(ps->externalEvent){
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if(ps->entityEventSequence < ps->eventSequence){
		int	seq;
		if(ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS){
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[seq] | ((ps->entityEventSequence & 3 ) << 8);
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}
	s->weapon = ps->weapon;
	s->weaponstate = ps->weaponstate;
	s->playerBitFlags = ps->bitFlags;
	s->attackPowerCurrent = ps->powerLevel[plHealth];
	s->attackPowerTotal = ps->powerLevel[plMaximum];
	s->charge1.chBase = ps->stats[stChargePercentPrimary];
	s->charge2.chBase = ps->stats[stChargePercentSecondary];
	s->groundEntityNum = ps->groundEntityNum;
	s->tier = ps->powerLevel[plTierCurrent];
	s->powerups = 0;
	for(i = 0;i < MAX_POWERUPS;i++){
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}
	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
