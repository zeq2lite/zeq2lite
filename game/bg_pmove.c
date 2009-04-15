// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate
#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "bg_userweapons.h"
pmove_t		*pm;
pml_t		pml;
// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_swimScale = 0.80f;
float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 1.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 15.0f;
float	pm_dashaccelerate = 15.0f; // 8.0
float	pm_friction = 6.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 6.0f; // 3.0f
float	pm_spectatorfriction = 7.0f;
int		c_pmove = 0;
//=====================
//  PM_DeductFromHealth
//=====================
qboolean PM_DeductFromHealth( int wanted ) {
	int newBuf;
	newBuf = pml.bufferHealth - wanted;
	if(newBuf >= 0){
		pml.bufferHealth = newBuf;
		return qtrue;
	}
	else{
		if((pm->ps->stats[powerLevelTotal] + newBuf) > 50){
			pml.bufferHealth = 0;
			pm->ps->stats[powerLevelTotal] += newBuf;
			return qtrue;
		}
	}
	return qfalse;
}
//=====================
//  PM_BuildBufferHealth
//=====================
#define MAX_BUFFERHEALTH_PER_MSEC 2
// FIXME: MAX_BUFFERHEALTH_PER_MSEC will go into a per-player configuration statistic called 'regen rate' later on.
void PM_BuildBufferHealth( void ) {
	float capRatio, maxRatio;

	// If current powerLevel exceeds the cap, give no buffer at all.
	if ( pm->ps->stats[powerLevelTotal] > pm->ps->persistant[powerLevelMaximum] ) {
		pml.bufferHealth = 0;
		return;
	}

	// Get the ratios between cap and current, and cap and max.
	//capRatio = (float)pm->ps->persistant[powerLevelMaximum] / (float)pm->ps->stats[powerLevel];
	//maxRatio = (float)pm->ps->persistant[powerLevelMaximum] / (float)pm->ps->stats[powerLevelTotal];

	// Calculate the buffer depending on both ratios, together with pml.frametime to
	// make it framerate independant.
	//pml.bufferHealth = (int)ceil(MAX_BUFFERHEALTH_PER_MSEC * pml.msec);
}


/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int		i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}
static void PM_StartLegsAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}
//	if ( pm->ps->legsTimer > 0 ) {
//		return;		// a high priority animation is running
//	}
	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
		| anim;
}

static void PM_ContinueLegsAnim( int anim ) {
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
//	if ( pm->ps->legsTimer > 0 ) {
//		return;		// a high priority animation is running
//	}
	PM_StartLegsAnim( anim );
}

static void PM_ContinueTorsoAnim( int anim ) {
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
//	if ( pm->ps->torsoTimer > 0 ) {
//		return;		// a high priority animation is running
//	}
	PM_StartTorsoAnim( anim );
}

static void PM_ForceLegsAnim( int anim ) {
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct (in, normal);

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t	vec;
	float	*vel;
	float	speed, newspeed, control;
	float	drop;

	vel = pm->ps->velocity;

	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0;	// ignore slope movement
	}

	speed = VectorLength(vec);
	if (speed < 1) {
		vel[0] = 0;
		vel[1] = 0;		// allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	//if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK) ) {
			// if getting knocked back, no friction
			if ( ! (pm->ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control*pm_friction*pml.frametime;
			}
		}
	//}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}


	// flight friction when flying
	if ( pm->ps->powerups[PW_FLYING] ) {
		drop += speed*pm_flightfriction*pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR) {
		drop += speed*pm_spectatorfriction*pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
#if 1
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	// clamp the speed lower if wading or moving underwater.
	if ( pm->waterlevel ) {
		float waterScale;

		waterScale = 1.0f - ( 1.0f - pm_swimScale ) * (float)pm->waterlevel / 3.0f;
		wishspeed = wishspeed * waterScale;
	}

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*pml.frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}

	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];
	}

#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t		wishVelocity;
	vec3_t		pushDir;
	float		pushLen;
	float		canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel*pml.frametime*wishspeed;
	if (canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int		max;
	float	total;
	float	scale;

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
		+ cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );

	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		}
	}
}

static void PM_StopBoost( void ) {
	pm->ps->stats[bitFlags] &= ~STATBIT_BOOSTING;
	pm->ps->powerups[PW_BOOST] = 0;
}

static void PM_StopDash( void ) {
	VectorClear( pm->ps->dashDir );
//	PM_StopBoost();
}

/*
============
PM_CheckTier
============
HACK: Currently just a quick hack routine that instantly raises tier when exceeding certain powerLevel
*/
static void PM_CheckTier( void ) {
	int tier, lowBreak, highBreak;
	tier = pm->ps->stats[tierCurrent];
	lowBreak = (32000 / 9) * (tier);
	highBreak = (32000 / 9) * (tier + 1);
	if((pm->ps->stats[powerLevel] < lowBreak) && tier > 0){PM_AddEvent(EV_TIERDOWN);}
	else if((pm->ps->stats[powerLevel] > highBreak ) && tier < 7){PM_AddEvent( EV_TIERUP );}
}

/*
==================
PM_CheckPowerLevel
==================
*/
static qboolean PM_CheckPowerLevel( void ) {
	int plSpeed,tier,lowBreak,highBreak,chargeScale,amount;
	tier = pm->ps->stats[tierCurrent];
	lowBreak = 1000;
	highBreak = (32000 / 9) * (tier + 1);
	chargeScale = pm->ps->powerlevelChargeScale * 10;
	// If the player wants to power up
	pm->ps->stats[powerLevelTimer2] += pml.msec;
	while(pm->ps->stats[powerLevelTimer2] >= 50){
		pm->ps->stats[powerLevelTimer2] -= 50;
		if(pm->ps->stats[powerLevel] > pm->ps->stats[powerLevelTotal]){
			pm->ps->stats[powerLevel] -= 6 * (pm->ps->stats[tierCurrent] + 1);
		}
		if(pm->ps->stats[powerLevelTotal] >= 32768) {
			pm->ps->stats[powerLevelTotal] = 32768;
		}
	}
	if(pm->cmd.buttons & BUTTON_POWER_UP){
		// set representations for powering up
		pm->ps->stats[bitFlags] |= STATBIT_ALTER_PL;
		PM_StopDash(); // implicitly stops boost and lightspeed as well
		PM_StopBoost();
		pm->ps->eFlags |= EF_AURA;
		if((pm->ps->stats[powerLevel] > highBreak)){
			PM_ContinueLegsAnim( LEGS_TRANS_UP );
		}
		else{
			PM_ContinueLegsAnim( LEGS_PL_UP );
		}
		// Increment the hold down timer
		if ( pm->ps->stats[STAT_POWERBUTTONS_TIMER] < 0 ) pm->ps->stats[STAT_POWERBUTTONS_TIMER] = 0;
		if ( pm->ps->stats[STAT_POWERBUTTONS_TIMER] < 16000 ) {
			pm->ps->stats[STAT_POWERBUTTONS_TIMER] += pml.msec;
		}
		if ( pm->ps->stats[STAT_POWERBUTTONS_TIMER] > 16000 ) {
			pm->ps->stats[STAT_POWERBUTTONS_TIMER] = 16000;
		}
		// Increment the power level timer scaled by hold down time
		plSpeed = (pm->ps->stats[STAT_POWERBUTTONS_TIMER] / 500.0f) + 1;
		pm->ps->stats[powerLevelCounter] += pml.msec * plSpeed;

		pm->ps->stats[powerLevelTimer] += pml.msec;
		while(pm->ps->stats[powerLevelTimer] >= 12){
			pm->ps->stats[powerLevelTimer] -= 10;
			if(pm->ps->stats[powerLevel] < pm->ps->stats[powerLevelTotal]){
				pm->ps->stats[powerLevel] += chargeScale;
			}
			else{
				if(pm->ps->stats[powerLevel] > pm->ps->stats[powerLevelTotal]){
					pm->ps->stats[powerLevelTotal] += chargeScale * 0.5;
				}
				else if(pm->ps->stats[powerLevelTotal] <= pm->ps->persistant[powerLevelMaximum]){
					pm->ps->stats[powerLevelTotal] += chargeScale * 0.2;
				}
				else{
					pm->ps->stats[powerLevelTotal] += chargeScale * 0.4;
				}
				if(pm->ps->stats[powerLevelTotal] >= pm->ps->persistant[powerLevelMaximum]){
					pm->ps->persistant[powerLevelMaximum] = pm->ps->stats[powerLevelTotal];
				}
			}
		}
		return qtrue;
	}
	// If the player wants to power down
	if ( pm->cmd.buttons & BUTTON_POWER_DOWN ) {
		// set representations for powering down
		pm->ps->stats[bitFlags] |= STATBIT_ALTER_PL;
		PM_StopDash(); // implicitly stops boost and lightspeed as well
		PM_StopBoost();
		PM_ContinueLegsAnim( LEGS_PL_DOWN );
		// Decrement the hold down timer
		if ( pm->ps->stats[STAT_POWERBUTTONS_TIMER] > 0 ) pm->ps->stats[STAT_POWERBUTTONS_TIMER] = 0;
		if ( pm->ps->stats[STAT_POWERBUTTONS_TIMER] > -16000 ) {
			pm->ps->stats[STAT_POWERBUTTONS_TIMER] -= pml.msec;
		}
		if ( pm->ps->stats[STAT_POWERBUTTONS_TIMER] < -16000 ) {
			pm->ps->stats[STAT_POWERBUTTONS_TIMER] = -16000;
		}
		plSpeed = (-pm->ps->stats[STAT_POWERBUTTONS_TIMER] / 500.0f) + 1;
		pm->ps->stats[powerLevelTimer] -= pml.msec * plSpeed;
		while ( pm->ps->stats[powerLevelTimer] <= -50 ) {
			pm->ps->stats[powerLevelTimer] += 50;
			amount = chargeScale * (pm->ps->stats[tierCurrent] + 1);
			if ( pm->ps->stats[powerLevel] - amount > 50 ) {
				pm->ps->stats[powerLevel] -= amount;
			}
			else{
				pm->ps->stats[powerLevel] = 50;
			}
		}

		return qtrue;
	}

	// Reset the hold down timer
	pm->ps->stats[bitFlags] &= ~STATBIT_ALTER_PL;
	pm->ps->stats[STAT_POWERBUTTONS_TIMER] = 0;
	return qfalse;
}


/*
===============
PM_CheckBoost
===============
*/
static qboolean PM_CheckBoost( void ) {

	// don't allow until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		PM_StopBoost();
		return qfalse;
	}

	if ( !(pm->cmd.buttons & BUTTON_BOOST )) {
		PM_StopBoost();
		return qfalse;
	}
/*
	// Still holding down the charge/boost key from a previous boosting round
	if ( !(pm->ps->stats[bitFlags] & STATBIT_BOOSTING) && (pm->ps->pm_flags & PMF_BOOST_HELD)) {
		PM_StopBoost();
		return qfalse;
	}
*/
	if ( !pm->ps->powerups[PW_BOOST] && PM_DeductFromHealth( 1 )) {
		pm->ps->powerups[PW_BOOST] = 100; // gives you a 100 msec boost
		pm->ps->pm_flags |= PMF_BOOST_HELD;
		pm->ps->stats[bitFlags] |= STATBIT_BOOSTING;
	}

	if ( pm->ps->powerups[PW_BOOST] ) {
		return qtrue;
	}

	return qfalse;
}



/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;		// don't allow jump until all buttons are up
	}

	if ( !( pm->cmd.buttons & BUTTON_JUMP )) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;		// jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;


	pm->ps->groundEntityNum = ENTITYNUM_NONE;


	if ( VectorLength( pm->ps->dashDir ) > 0.0f ) {

		float len;
		vec3_t forward, right, xy_vel;


		VectorCopy( pml.forward, forward );
		VectorCopy( pml.right, right );
		forward[2] = 0;
		right[2] = 0;

		VectorScale( forward, pm->cmd.forwardmove, forward );
		VectorScale( right, pm->cmd.rightmove, right );
		VectorAdd( forward, right, xy_vel );
		len = VectorNormalize2( xy_vel, pm->ps->velocity );
		if ( !len ) {
			VectorSet(pm->ps->velocity, 0, 0, 1);
		}
		VectorScale( pm->ps->velocity, JUMP_VELOCITY, pm->ps->velocity );

//		pm->ps->velocity[0] *= 1.8f;
//		pm->ps->velocity[1] *= 1.8f;
		pm->ps->velocity[2] = JUMP_VELOCITY * 1.75f;

		PM_AddEvent( EV_HIGHJUMP );

	} else {
//		pm->ps->velocity[0] *= 2.0f;
//		pm->ps->velocity[1] *= 2.0f;
		pm->ps->velocity[2] = JUMP_VELOCITY * 1.25f;

		PM_AddEvent( EV_JUMP );
	}

	if ( pm->cmd.forwardmove >= 0 ) {
		PM_ForceLegsAnim( LEGS_JUMP );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		PM_ForceLegsAnim( LEGS_JUMPB );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}


	PM_StopDash(); // implicitly stops boost and lightspeed as well

	return qtrue;
}


//============================================================================

/*
===================
PM_Transform

===================
*/
static void PM_Transform( void ) {
	// implicitly stops boost and lightspeed as well
	PM_StopDash();
	PM_StopBoost();
	pm->ps->powerups[PW_LIGHTSPEED] = 0;
	PM_ContinueLegsAnim( LEGS_TRANS_UP );
	pm->ps->eFlags |= EF_AURA;
	if ( pm->ps->powerups[PW_TRANSFORM] > 0 ) {
		pm->ps->powerups[PW_TRANSFORM] -= pml.msec;
		if ( pm->ps->powerups[PW_TRANSFORM] < 0 ) {
			pm->ps->powerups[PW_TRANSFORM] = 0;
		}
	}
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	float	scale;
	float	boostFactor;

	// normal slowdown
	PM_Friction ();

	if ( pm->ps->powerups[PW_TRANSFORM] )
		return;

	if ( PM_CheckPowerLevel() ) {
		return;
	} else if ( PM_CheckBoost() ) {
		if ( pm->ps->stats[tierCurrent] == 8 ) {
			boostFactor = 7.0f;
		} else if ( pm->ps->stats[tierCurrent] == 7) {
			boostFactor = 6.5f;
		} else if ( pm->ps->stats[tierCurrent] == 6) {
			boostFactor = 6.0f;
		} else if ( pm->ps->stats[tierCurrent] == 5) {
			boostFactor = 5.5f;
		} else if ( pm->ps->stats[tierCurrent] == 4) {
			boostFactor = 5.0f;
		} else if ( pm->ps->stats[tierCurrent] == 3) {
			boostFactor = 4.5f;
		} else if ( pm->ps->stats[tierCurrent] == 2) {
			boostFactor = 4.0f;
		} else if ( pm->ps->stats[tierCurrent] == 1) {
			boostFactor = 3.5f;
		} else {
			boostFactor = 3.0f;
		}
	} else if ( pm->cmd.buttons & BUTTON_WALKING ) {
		boostFactor = 1.25f;
	} else {
		boostFactor = 2.10f;
	}

	scale = PM_CmdScale( &pm->cmd ) * boostFactor;

	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	} else {
		for (i=0 ; i<3 ; i++) {
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove +
			             scale * pml.right[i] * pm->cmd.rightmove +
						 scale * pml.up[i] * pm->cmd.upmove;
		}
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate (wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}

	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate (wishdir, wishspeed, pm_airaccelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );
	}

	// <-- RiO: less gravity when submerged
	if ( pm->waterlevel == 3 ) {
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity * pml.frametime;
	}
	// -->

	PM_StepSlideMove ( qtrue );
}





/*
===================
PM_GrappleMove

===================
*/
static void PM_GrappleMove( void ) {
	vec3_t vel, v;
	float vlen;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize( vel );

	if (vlen <= 100)
		VectorScale(vel, 10 * vlen, vel);
	else
		VectorScale(vel, 800, vel);

	VectorCopy(vel, pm->ps->velocity);

	pml.groundPlane = qfalse;
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;

	if ( pm->ps->powerups[PW_TRANSFORM] ) {
		PM_Friction();
		return;
	}

	if ( PM_CheckPowerLevel() ) {
		PM_Friction();
		return;
	}

	if ( PM_CheckJump () ) {
		// jumped away
		PM_AirMove();
		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale * 0.80f; // * 0.80f is a fix for ZEQ2's walking speed

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate (wishdir, wishspeed, accelerate);

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	}

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
		pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}

	// <-- RiO: less gravity when submerged
	if ( pm->waterlevel == 3 ) {
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity * pml.frametime;
	}
	// -->

	PM_StepSlideMove( qfalse );
}

/*
=============
PM_DashMove
=============
*/
static void PM_DashMove( void ) {
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		boostFactor;
	float		accelerate;

	if (( pm->ps->dashDir[0] && ( Q_Sign( pm->ps->dashDir[0] ) != Q_Sign( pm->cmd.forwardmove ))) ||
		( pm->ps->dashDir[1] && ( Q_Sign( pm->ps->dashDir[1] ) != Q_Sign( pm->cmd.rightmove   ))) ||
		( pm->cmd.buttons & BUTTON_WALKING )) {

		PM_StopDash(); // implicitly stops boost and lightspeed as well

		if ( pml.walking )
			PM_WalkMove();
		else
			PM_AirMove();

		return;
	}

	if ( pm->ps->powerups[PW_TRANSFORM] ) {
		PM_Friction();
		return;
	}

	if ( PM_CheckPowerLevel() ) {
		PM_Friction();
		return;
	}

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		if ( PM_CheckJump() ) {
			// jumped away
			PM_AirMove();
			return;
		}
	}

	pm->cmd.upmove = 0; // ensure proper scaling in PM_CmdScale

	if ( PM_CheckBoost()) {
		pm->ps->pm_flags |= PMF_BOOST_HELD;
		if ( pm->ps->stats[tierCurrent] == 8 ) {
			boostFactor = 7.0f;
		} else if ( pm->ps->stats[tierCurrent] == 7) {
			boostFactor = 6.5f;
		} else if ( pm->ps->stats[tierCurrent] == 6) {
			boostFactor = 6.0f;
		} else if ( pm->ps->stats[tierCurrent] == 5) {
			boostFactor = 5.5f;
		} else if ( pm->ps->stats[tierCurrent] == 4) {
			boostFactor = 5.0f;
		} else if ( pm->ps->stats[tierCurrent] == 3) {
			boostFactor = 4.5f;
		} else if ( pm->ps->stats[tierCurrent] == 2) {
			boostFactor = 4.0f;
		} else if ( pm->ps->stats[tierCurrent] == 1) {
			boostFactor = 3.5f;
		} else {
			boostFactor = 3.0f;
		}
	} else {
		boostFactor = 1.5f;
	}

	// Apply friction
	PM_Friction();

	if ( VectorLength( pm->ps->dashDir ) == 0.0f ) {
		if ( pm->cmd.forwardmove > 0 ) {
			pm->ps->dashDir[0] = 1;
			pm->ps->dashDir[1] = 0;
		} else if ( pm->cmd.forwardmove < 0 ) {
			pm->ps->dashDir[0] = -1;
			pm->ps->dashDir[1] = 0;
		} else if ( pm->cmd.rightmove > 0 ) {
			pm->ps->dashDir[0] = 0;
			pm->ps->dashDir[1] = 1;
		} else if ( pm->cmd.rightmove < 0 ) {
			pm->ps->dashDir[0] = 0;
			pm->ps->dashDir[1] = -1;
		}
	}

	if ( pm->ps->dashDir[0] == 1 && pm->ps->dashDir[1] == 0 ) {
		pm->cmd.forwardmove = 120;
		pm->cmd.rightmove = ceil(pm->cmd.rightmove / (boostFactor * 0.75f) );

	} else if ( pm->ps->dashDir[0] == -1 && pm->ps->dashDir[1] == 0 ) {
		pm->cmd.forwardmove = -120;
		pm->cmd.rightmove = ceil(pm->cmd.rightmove / (boostFactor * 0.75f) );

	} else if ( pm->ps->dashDir[0] == 0 && pm->ps->dashDir[1] == 1 ) {
		pm->cmd.rightmove = 120;
		pm->cmd.forwardmove = ceil(pm->cmd.forwardmove / (boostFactor * 0.75f) );

	} else if ( pm->ps->dashDir[0] == 0 && pm->ps->dashDir[1] == -1 ) {
		pm->cmd.rightmove = -120;
		pm->cmd.forwardmove = ceil(pm->cmd.forwardmove / (boostFactor * 0.75f) );
	}

	cmd = pm->cmd;
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	scale = PM_CmdScale( &cmd ) * boostFactor;


	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	wishvel[2] = 0;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	} else {
		accelerate = pm_dashaccelerate;
	}

	// not on ground, so little effect on velocity
	PM_Accelerate (wishdir, wishspeed, accelerate);

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity (pm->ps->velocity, pml.groundTrace.plane.normal,
			pm->ps->velocity, OVERCLIP );
	}

	// <-- RiO: less gravity when submerged
	if ( pm->waterlevel == 3 ) {
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity * pml.frametime;
	}
	// -->

	PM_StepSlideMove ( qtrue );
}

void PM_LightSpeedMove( void ) {
	vec3_t pre_vel, post_vel;

	// no light speed movement if transforming
	if ( pm->ps->powerups[PW_TRANSFORM] ) {
		return;
	}

	pm->ps->powerups[PW_LIGHTSPEED] -= pml.msec;
	if ( pm->ps->powerups[PW_LIGHTSPEED] < 0 )
		pm->ps->powerups[PW_LIGHTSPEED] = 0;

	if ( !(pm->cmd.buttons & BUTTON_LIGHTSPEED))
		pm->ps->powerups[PW_LIGHTSPEED] = 0;

	if ( pm->ps->powerups[PW_LIGHTSPEED] ) {
		// Move in the direction of the current velocity
		VectorNormalize( pm->ps->velocity );
		VectorCopy( pm->ps->velocity, pre_vel );
		VectorScale( pm->ps->velocity, 4000, pm->ps->velocity );

		PM_StepSlideMove( qfalse );
		VectorNormalize2( pm->ps->velocity, post_vel );

		// Check if we bounced off of something or become stuck against something
		if (( DotProduct( pre_vel, post_vel ) < 0.5f ) || ( VectorLength( pm->ps->velocity ) == 0.0f )) {
			pm->ps->powerups[PW_LIGHTSPEED] = 0;
		}
	}

	if ( pm->ps->powerups[PW_LIGHTSPEED] == 0 ) {
		PM_AddEvent( EV_LIGHTSPEED_GHOSTIMAGE );
		pm->ps->powerups[PW_LIGHTSPEED] = 0;
		VectorClear(pm->ps->velocity);
		return;
	}
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float	forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength (pm->ps->velocity);
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear (pm->ps->velocity);
	} else {
		VectorNormalize (pm->ps->velocity);
		VectorScale (pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {
	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	if ( pml.groundTrace.surfaceFlags & SURF_METALSTEPS ) {
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP ) {
		PM_ForceLegsAnim( LEGS_LANDB );
	} else {
		PM_ForceLegsAnim( LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = (-b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		delta *= 2;
	}

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !(pml.groundTrace.surfaceFlags & SURF_NODAMAGE) )  {
		if ( delta > 60 ) {
			PM_AddEvent( EV_FALL_FAR );
		} else if ( delta > 40 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[powerLevel] > 0 ) {
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		} else if ( delta > 7 ) {
			PM_AddEvent( EV_FALL_SHORT );
		} else {
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}

/*
=============
PM_CheckStuck
=============
*/
/*
void PM_CheckStuck(void) {
	trace_t trace;

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask);
	if (trace.allsolid) {
		//int shit = qtrue;
	}
}
*/

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int			i, j, k;
	vec3_t		point;

	if ( pm->debugLevel ) {
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t		trace;
	vec3_t		point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if ( trace.fraction == 1.0 ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				PM_ForceLegsAnim( LEGS_JUMP );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				PM_ForceLegsAnim( LEGS_JUMPB );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}

			// ADDING FOR ZEQ2: We force-disable dashing;
			PM_StopDash(); // implicitly stops boost and lightspeed as well
		}

	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t		point;
	trace_t		trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	//point[2] = pm->ps->origin[2] - 0.25;
	point[2] = pm->ps->origin[2] - 0.75f; // Bigger limit for ZEQ2's dashing

	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid(&trace) )
			return;
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:kickoff\n", c_pmove);
		}

		// go into jump animation
		// ADDING FOR ZEQ2: But only if we're not dashing!!
		if ( pm->cmd.forwardmove >= 0 ) {
			if ( VectorLength(pm->ps->dashDir) == 0.0f ) {
				PM_ForceLegsAnim( LEGS_JUMP );
			}
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		} else {
			if ( VectorLength(pm->ps->dashDir) == 0.0f ) {
				PM_ForceLegsAnim( LEGS_JUMPB );
			}
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// ADDING FOR ZEQ2
	if ( pm->ps->powerups[PW_FLYING] ) {
		vec3_t testVec;

		VectorNormalize2(pml.up, testVec);
		// Keep flying if not upright enough, not moving down, or are using boost
		if ( (testVec[2] < 0.9f) || (pm->cmd.upmove >= 0) /*|| pm->ps->powerups[PW_BOOST]*/ ) {

			pm->ps->groundEntityNum = ENTITYNUM_NONE;
			pml.groundPlane = qfalse;
			pml.walking = qfalse;
			return;
		} else {
			pm->ps->powerups[PW_FLYING] = 0;
//			pm->ps->powerups[PW_BOOST] = 0;
			pm->ps->powerups[PW_LIGHTSPEED] = 0;
//			pm->ps->stats[bitFlags] &= ~STATBIT_BOOSTING;
		}
	}
	// END ADDING

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf("%i:Land\n", c_pmove);
		}

		// ADDING FOR ZEQ2: Don't crashland when we're dashing!
		if ( VectorLength(pm->ps->dashDir) == 0.0f ) {
			PM_CrashLand();
		}

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	// Always set zero water level if we're trying to jump out of the water
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP ) {
		return;
	}

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if ( cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck (void)
{
	// FIXME: No longer need this?
	pm->ps->pm_flags &= ~PMF_INVULEXPAND;

	pm->mins[0] = -15;
	pm->mins[1] = -15;
	pm->mins[2] = MINS_Z;

	pm->maxs[0] = 15;
	pm->maxs[1] = 15;

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	pm->maxs[2] = 32;
	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float		bobmove;
	int			old;
	qboolean	footstep;

	// If using lightspeed, don't change any animations at all.
	if ( pm->ps->powerups[PW_LIGHTSPEED] ) {
		return;
	}

	// If transforming, don't change any animations at all.
	if ( pm->ps->powerups[PW_TRANSFORM] ) {
		return;
	}

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
		+  pm->ps->velocity[1] * pm->ps->velocity[1] );

	// If changing PL, the legs' animation has already been set.
	if ( pm->ps->stats[bitFlags] & STATBIT_ALTER_PL ) {
		return;
	}

	// ADDING FOR ZEQ2
	// If we're dashing, fill in correct leg animations.
	// Leave cycle intact, but don't advance.
	if ( VectorLength(pm->ps->dashDir ) > 0.0f && pm->xyspeed >= 500 ) {

		if ( pm->ps->running ) {
		 	return;
		}

		if ( pm->ps->dashDir[0] > 0 ) {
			PM_ContinueLegsAnim( LEGS_DASH_FORWARD );
		} else if ( pm->ps->dashDir[0] < 0 ) {
			PM_ContinueLegsAnim( LEGS_DASH_BACKWARD );
		} else if ( pm->ps->dashDir[1] > 0 ) {
			PM_ContinueLegsAnim( LEGS_DASH_RIGHT );
		} else if ( pm->ps->dashDir[1] < 0 ) {
			PM_ContinueLegsAnim( LEGS_DASH_LEFT );
		} else {
			// This should never happen
		}
		return;
	}

	// take care of flight
	if ( pm->ps->powerups[PW_FLYING] ) {

		if (( !pm->cmd.forwardmove && !pm->cmd.rightmove ) || ( pm->cmd.buttons & BUTTON_WALKING )) {
			int	tempAnimIndex;

			pm->ps->bobCycle = 0;	// start at beginning of cycle again

			tempAnimIndex = pm->ps->torsoAnim & ~ANIM_TOGGLEBIT;
			if ( (tempAnimIndex >= TORSO_KI_ATTACK1_PREPARE) && (tempAnimIndex <= TORSO_KI_ATTACK6_ALT_FIRE)) {
				tempAnimIndex = tempAnimIndex - TORSO_KI_ATTACK1_PREPARE;
				tempAnimIndex = LEGS_AIR_KI_ATTACK1_PREPARE + tempAnimIndex;
				PM_ContinueLegsAnim( tempAnimIndex );
			} else {
				if ( pm->ps->stats[target] >= 0 ) {
					PM_ContinueLegsAnim( LEGS_IDLE_LOCKED );
				} else {
					PM_ContinueLegsAnim( LEGS_FLY_IDLE );
				}
			}

			return;
		}

		if ( pm->cmd.forwardmove > 0 ) {
			if ( pm->ps->pm_flags & PMF_BOOST_HELD ) {
				PM_ContinueLegsAnim( LEGS_FLY_FORWARD );
			} else {
				PM_ContinueLegsAnim( LEGS_DASH_FORWARD );
			}
		} else if ( pm->cmd.forwardmove < 0 ) {
			if ( pm->ps->pm_flags & PMF_BOOST_HELD ) {
				PM_ContinueLegsAnim( LEGS_FLY_BACKWARD );
			} else {
				PM_ContinueLegsAnim( LEGS_DASH_BACKWARD );
			}
		} else if ( pm->cmd.rightmove > 0 ) {
			PM_ContinueLegsAnim( LEGS_DASH_RIGHT );
		} else if ( pm->cmd.rightmove < 0 ) {
			PM_ContinueLegsAnim( LEGS_DASH_LEFT );
		}

		return;
	}

	// END ADDING

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {

		if ( pm->ps->powerups[PW_INVULNERABILITY] ) {
			PM_ContinueLegsAnim( LEGS_IDLECR );
		}
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 ) {
			PM_ContinueLegsAnim( LEGS_SWIM );
		}

		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			if ( pm->ps->pm_flags & PMF_DUCKED ) {
				PM_ContinueLegsAnim( LEGS_IDLECR );
			} else {
				int	tempAnimIndex;

				tempAnimIndex = pm->ps->torsoAnim & ~ANIM_TOGGLEBIT;
				if ( (tempAnimIndex >= TORSO_KI_ATTACK1_PREPARE) && (tempAnimIndex <= TORSO_KI_ATTACK6_ALT_FIRE)) {
					tempAnimIndex = tempAnimIndex - TORSO_KI_ATTACK1_PREPARE;
					tempAnimIndex = LEGS_KI_ATTACK1_PREPARE + tempAnimIndex;
					PM_ContinueLegsAnim( tempAnimIndex );
				} else {
					if ( pm->ps->stats[target] >= 0 ) {
						PM_ContinueLegsAnim( LEGS_IDLE_LOCKED );
					} else {
						PM_ContinueLegsAnim( LEGS_IDLE );
					}
				}
			}

		}
		return;
	}


	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;	// ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			PM_ContinueLegsAnim( LEGS_BACKCR );
		}
		else {
			PM_ContinueLegsAnim( LEGS_WALKCR );
		}
	} else {
		if ( !(pm->cmd.buttons & BUTTON_WALKING) ) {
			bobmove = 0.4f;	// faster speeds bob faster
			if ( !pm->ps->running ) { 
				if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN && pm->cmd.forwardmove < 0) {
					PM_ContinueLegsAnim( LEGS_DASH_BACKWARD );
				} else if ( pm->cmd.forwardmove > 0 ) {
					PM_ContinueLegsAnim( LEGS_DASH_FORWARD );
				} else if ( pm->cmd.rightmove > 0 ) {
					PM_ContinueLegsAnim( LEGS_DASH_RIGHT );
				} else if ( pm->cmd.rightmove < 0 ) {
					PM_ContinueLegsAnim( LEGS_DASH_LEFT );
				}
			} else {
				if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN && pm->cmd.forwardmove < 0) {
					PM_ContinueLegsAnim( LEGS_BACK );
				} else if ( pm->cmd.forwardmove > 0 ) {
					PM_ContinueLegsAnim( LEGS_RUN );
				} else if ( pm->cmd.rightmove > 0 ) {
					PM_ContinueLegsAnim( LEGS_DASH_RIGHT );
				} else if ( pm->cmd.rightmove < 0 ) {
					PM_ContinueLegsAnim( LEGS_DASH_LEFT );
				}
			}
			footstep = qtrue;
		} else {
			bobmove = 0.3f;	// walking bobs slow
			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
				PM_ContinueLegsAnim( LEGS_BACKWALK );
			}
			else {
				PM_ContinueLegsAnim( LEGS_WALK );
			}
		}
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 ) {
		if ( pm->waterlevel == 0 ) {
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps ) {
				PM_AddEvent( PM_FootstepForSurface() );
			}
		} else if ( pm->waterlevel == 1 ) {
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		} else if ( pm->waterlevel == 2 ) {
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		} else if ( pm->waterlevel == 3 ) {
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {		// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon ) {
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( pm->ps->stats[skills] & ( 1 << weapon ) ) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
		return;
	}

	PM_AddEvent( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 10;
	//PM_StartTorsoAnim( TORSO_DROP );
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int		weapon;

	weapon = pm->cmd.weapon;
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS ) {
		weapon = WP_NONE;
	}

	if ( !( pm->ps->stats[skills] & ( 1 << weapon ) ) ) {
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 10;
	//PM_StartTorsoAnim( TORSO_RAISE );
}


/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void ) {
	if ( pm->ps->weaponstate != WEAPON_READY ) {
		return;
	}

	switch ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) {
	case LEGS_IDLE_LOCKED:
		PM_ContinueTorsoAnim( TORSO_STAND_LOCKED );
		break;
	case LEGS_DASH_FORWARD:
		PM_ContinueTorsoAnim( TORSO_DASH_FORWARD );
		break;
	case LEGS_DASH_BACKWARD:
		PM_ContinueTorsoAnim( TORSO_DASH_BACKWARD );
		break;
	case LEGS_DASH_RIGHT:
		PM_ContinueTorsoAnim( TORSO_DASH_RIGHT );
		break;
	case LEGS_DASH_LEFT:
		PM_ContinueTorsoAnim( TORSO_DASH_LEFT );
		break;
	case LEGS_WALK:
		PM_ContinueTorsoAnim( TORSO_WALK );
		break;
	case LEGS_BACKWALK:
		PM_ContinueTorsoAnim( TORSO_BACKWALK );
		break;
	case LEGS_WALKCR:
		PM_ContinueTorsoAnim( TORSO_WALKCR );
		break;
	case LEGS_RUN:
		PM_ContinueTorsoAnim( TORSO_RUN );
		break;
	case LEGS_BACK:
		PM_ContinueTorsoAnim( TORSO_BACK );
		break;
	case LEGS_SWIM:
		PM_ContinueTorsoAnim( TORSO_SWIM );
		break;
	case LEGS_JUMP:
		PM_ContinueTorsoAnim( TORSO_JUMP );
		break;
	case LEGS_LAND:
		PM_ContinueTorsoAnim( TORSO_LAND );
		break;
	case LEGS_JUMPB:
		PM_ContinueTorsoAnim( TORSO_JUMPB );
		break;
	case LEGS_LANDB:
		PM_ContinueTorsoAnim( TORSO_LANDB );
		break;
	case LEGS_KI_CHARGE:
		PM_ContinueTorsoAnim( TORSO_KI_CHARGE );
		break;
	case LEGS_PL_UP:
		PM_ContinueTorsoAnim( TORSO_PL_UP );
		break;
	case LEGS_PL_DOWN:
		PM_ContinueTorsoAnim( TORSO_PL_DOWN );
		break;
	case LEGS_TRANS_UP:
		PM_ContinueTorsoAnim( TORSO_TRANS_UP );
		break;
	case LEGS_TRANS_BACK:
		PM_ContinueTorsoAnim( TORSO_TRANS_BACK );
		break;
	case LEGS_FLY_IDLE:
		PM_ContinueTorsoAnim( TORSO_FLY_IDLE );
		break;
	case LEGS_FLY_FORWARD:
		PM_ContinueTorsoAnim( TORSO_FLY_FORWARD );
		break;
	case LEGS_FLY_BACKWARD:
		PM_ContinueTorsoAnim( TORSO_FLY_BACKWARD );
		break;
	default:
		// if we're not doing anything special with the legs, then
		// we default to the stand still animation
		if ( pm->ps->stats[target] >= 0 ) {
			PM_ContinueTorsoAnim( TORSO_STAND_LOCKED );
		} else {
			PM_ContinueTorsoAnim( TORSO_STAND );
		}
		break;
	}
}


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void ) {
	int					*weaponInfo;
	int					*alt_weaponInfo;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// ignore if charging, or transforming
	if ( pm->cmd.buttons & BUTTON_POWER_UP || pm->ps->powerups[PW_TRANSFORM] > 0 ) {
		if ( pm->ps->weaponstate == WEAPON_GUIDING || pm->ps->weaponstate == WEAPON_ALTGUIDING ) {
			PM_AddEvent( EV_DETONATE_WEAPON );
		}
		pm->ps->weaponstate = WEAPON_READY;
		pm->ps->stats[chargePercentPrimary] = 0;
		pm->ps->stats[chargePercentSecondary] = 0;
		return;
	}

	// check for dead player
	if ( pm->ps->stats[powerLevel] <= 0 ) {
		if ( pm->ps->weaponstate == WEAPON_GUIDING || pm->ps->weaponstate == WEAPON_ALTGUIDING ) {
			PM_AddEvent( EV_DETONATE_WEAPON );
		}
		pm->ps->weaponstate = WEAPON_READY;
		pm->ps->stats[chargePercentPrimary] = 0;
		pm->ps->stats[chargePercentSecondary] = 0;
		pm->ps->weapon = WP_NONE;
		return;
	}

	// check if we're set to use an unitialized weapon (WP_NONE)
	if ( pm->ps->weapon == WP_NONE ) {
		return;
	}

	// Retrieve our weapon's settings
	weaponInfo = pm->ps->ammo;
	if ( weaponInfo[WPbitFlags] & WPF_ALTWEAPONPRESENT ) {
		alt_weaponInfo = &weaponInfo[WPSTAT_ALT_KICOST];
		// Offset the array to map the WPSTAT set onto the WPSTAT_ALT set.
	} else {
		alt_weaponInfo = weaponInfo;	 // Keep both sets the same.

		if (pm->cmd.buttons & BUTTON_ALT_ATTACK) {	// If we have no altfire,
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;	// override ALT_ATTACK to ATTACK.
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
	}

	/*
	We're not allowed to do anything with the weapon until the
	client has received the relevant changed data about the weapon's
	configuration from the server. This will cause a bit of stuttering
	clientside, as it 'disables' client's weapon prediction for the own
	player temporarily.
	Even then, it shouldn't happen unless you run into a bad
	network transfer rate right when you immmediately fire a weapon
	after switching. Most unlikely to occur, but a 'jump' in graphics
	is still better than a possible glitch in graphics or game physics.
	*/
	if ( weaponInfo[WPSTAT_NUMCHECK] != pm->ps->weapon ) {
		return;
	}

	switch( pm->ps->weaponstate ) {
	case WEAPON_READY:
	case WEAPON_DROPPING:
	case WEAPON_RAISING:
	case WEAPON_COOLING:
		{
			if ( pm->ps->weaponTime > 0 ) {
				pm->ps->weaponTime -= pml.msec;
			}

			if ( pm->ps->weaponTime > 0 ) {
				break;
			} else {
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
			}

			if ( pm->ps->weapon != pm->cmd.weapon ) {
				PM_BeginWeaponChange( pm->cmd.weapon );
			}

			if ( pm->ps->weaponstate == WEAPON_DROPPING ) {
				PM_FinishWeaponChange();
				break;
			}

			if ( pm->ps->weaponstate == WEAPON_RAISING ) {
				pm->ps->weaponstate = WEAPON_READY;
				PM_StartTorsoAnim( TORSO_STAND );
				break;
			}

			// Starting a primary attack
			if (pm->cmd.buttons & BUTTON_ATTACK) {
				// Does the weapon require a charge?
				if ( weaponInfo[WPbitFlags] & WPF_NEEDSCHARGE ) {
					pm->ps->weaponstate = WEAPON_CHARGING;

					// Adding the weapon number like this should give us the correct animation
					PM_StartTorsoAnim( TORSO_KI_ATTACK1_PREPARE + (pm->ps->weapon - 1) * 2 );
					break;
				}

				// See if we have enough in reserve to start firing the weapon
				if ( !PM_DeductFromHealth( weaponInfo[WPSTAT_KICOST] )) {
					break;
				}

				// Is the weapon a continuous fire weapon?
				if ( weaponInfo[WPbitFlags] & WPF_CONTINUOUS ) {
					PM_AddEvent( EV_FIRE_WEAPON );
					pm->ps->weaponstate = WEAPON_FIRING;
				} else {
					PM_AddEvent( EV_FIRE_WEAPON );
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				}

				// Adding the weapon number like this should give us the correct animation
				PM_StartTorsoAnim( TORSO_KI_ATTACK1_FIRE + (pm->ps->weapon - 1) * 2 );
				break;
			}

			// NOTE: Primary attack always takes precedence over alternate attack this way,
			//       which is exactly what we want.

			// Starting an alternate attack
			if (pm->cmd.buttons & BUTTON_ALT_ATTACK) {
				// Does the weapon require a charge?
				if ( alt_weaponInfo[WPbitFlags] & WPF_NEEDSCHARGE ) {
					pm->ps->weaponstate = WEAPON_ALTCHARGING;

					// Adding the weapon number like this should give us the correct animation
					PM_StartTorsoAnim( TORSO_KI_ATTACK1_ALT_PREPARE + (pm->ps->weapon - 1) * 2 );
					break;
				}

				// See if we have enough in reserve to start firing the weapon
				if ( !PM_DeductFromHealth( alt_weaponInfo[WPSTAT_KICOST] )) {
					break;
				}

				// Is the weapon a continuous fire weapon?
				if ( alt_weaponInfo[WPbitFlags] & WPF_CONTINUOUS ) {
					pm->ps->weaponstate = WEAPON_ALTFIRING;
					PM_AddEvent( EV_ALTFIRE_WEAPON );
				} else {
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
					PM_AddEvent( EV_ALTFIRE_WEAPON );
				}

				// Adding the weapon number like this should give us the correct animation
				PM_StartTorsoAnim( TORSO_KI_ATTACK1_ALT_FIRE + (pm->ps->weapon - 1) * 2 );
				break;
			}

			PM_ContinueTorsoAnim( TORSO_STAND );
		}
		break;
	case WEAPON_GUIDING:
		{
			if ( pm->cmd.buttons & BUTTON_ATTACK ) {
				PM_AddEvent( EV_DETONATE_WEAPON );
				pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim( TORSO_STAND );
			}
		}
		break;
	case WEAPON_ALTGUIDING:
		{
			if ( pm->cmd.buttons & BUTTON_ALT_ATTACK ) {
				PM_AddEvent( EV_DETONATE_WEAPON );
				pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim( TORSO_STAND );
			}
		}
		break;
	case WEAPON_CHARGING:
		{
			if ( !( pm->cmd.buttons & BUTTON_ATTACK ) ) {

				if ( weaponInfo[WPbitFlags] & WPF_READY ) {
					weaponInfo[WPbitFlags] &= ~WPF_READY;

					// If the weapon is meant to be guided, use WEAPON_GUIDING
					// and don't set the cooldown time yet.
					if ( weaponInfo[WPbitFlags] & WPF_GUIDED ) {
						pm->ps->weaponstate = WEAPON_GUIDING;
					} else {
						pm->ps->weaponstate = WEAPON_COOLING;
						pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
					}

					// NOTE: chargePercentPrimary is still used in this event's handler,
					// so don't reset it yet.
					PM_AddEvent( EV_FIRE_WEAPON );

					// Adding the weapon number like this should give us the correct animation
					PM_StartTorsoAnim( TORSO_KI_ATTACK1_FIRE + (pm->ps->weapon - 1) * 2 );

				} else {
					pm->ps->weaponTime = 0;
					pm->ps->weaponstate = WEAPON_READY;
					pm->ps->stats[chargePercentPrimary] = 0;

					PM_StartTorsoAnim( TORSO_STAND );
				}
			}
		}
		break;
	case WEAPON_ALTCHARGING:
		{
			if ( !( pm->cmd.buttons & BUTTON_ALT_ATTACK ) ) {

				if ( alt_weaponInfo[WPbitFlags] & WPF_READY ) {
					alt_weaponInfo[WPbitFlags] &= ~WPF_READY;

					// If the weapon is meant to be guided, use WEAPON_ALTGUIDING
					// and don't set the cooldown time yet.
					if ( alt_weaponInfo[WPbitFlags] & WPF_GUIDED ) {
						pm->ps->weaponstate = WEAPON_ALTGUIDING;
					} else {
						pm->ps->weaponstate = WEAPON_COOLING;
						pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
					}

					// NOTE: chargePercentSecondary is still used in this event's handler,
					// so don't reset it yet.
					PM_AddEvent( EV_ALTFIRE_WEAPON );

					// Adding the weapon number like this should give us the correct animation
					PM_StartTorsoAnim( TORSO_KI_ATTACK1_ALT_FIRE + (pm->ps->weapon - 1) * 2 );

				} else {
					pm->ps->weaponTime = 0;
					pm->ps->weaponstate = WEAPON_READY;
					pm->ps->stats[chargePercentSecondary] = 0;

					PM_StartTorsoAnim( TORSO_STAND );
				}
			}
		}
		break;
	case WEAPON_FIRING:
		{
			if ( !(pm->cmd.buttons & BUTTON_ATTACK) ) {
				pm->ps->weaponstate = WEAPON_COOLING;
				pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim( TORSO_STAND );
				break;
			}

			// Hijacked weaponTime as a timer for costs
			pm->ps->weaponTime += pml.msec;
			while ( pm->ps->weaponTime > 100 ) {

				// Couldn't afford upkeep for the weapon any longer
				if ( !PM_DeductFromHealth( weaponInfo[WPSTAT_KICOST] )) {
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
					PM_StartTorsoAnim( TORSO_STAND );
					break;
				}
			}

		}
		break;
	case WEAPON_ALTFIRING:
		{
			if ( !(pm->cmd.buttons & BUTTON_ALT_ATTACK) ) {
				pm->ps->weaponstate = WEAPON_COOLING;
				pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim( TORSO_STAND );
				break;
			}

			// Hijacked weaponTime as a timer for costs
			pm->ps->weaponTime += pml.msec;
			while ( pm->ps->weaponTime > 100 ) {
				// Couldn't afford upkeep for the weapon any longer
				if ( !PM_DeductFromHealth( alt_weaponInfo[WPSTAT_KICOST] )) {
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
					PM_StartTorsoAnim( TORSO_STAND );
					break;
				}
			}

		}
		break;
	default:
		// Should never happen
		break;
	}
}



/*
================
PM_Animate
================
*/

static void PM_Animate( void ) {

	if ( pm->cmd.buttons & BUTTON_GESTURE ) { 
		if ( pm->ps->torsoTimer == 0 ) {
//			PM_StartTorsoAnim( TORSO_GESTURE );
			pm->ps->torsoTimer = TIMER_GESTURE;
			PM_AddEvent( EV_TAUNT );
		}
	}
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}
}


/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int			i;
	// ADDING FOR ZEQ2
	int			roll = 0;
	float		oldCmdAngle;
	vec3_t		offset_angles;
	vec4_t		quatOrient;
	vec4_t		quatRot;
	vec4_t		quatResult;
	vec3_t		new_angles;
	float		pitchNorm;
	// END ADDING

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[powerLevel] <= 0 ) {
		return;		// no view changes at all
	}

	// ADDING FOR ZEQ2
	// If we're flying, use quaternion multiplication to work on player's
	// local axes.
	if ( pm->ps->powerups[PW_FLYING]) {
		// See if we need to add degrees for rolling.
			
		if (cmd->buttons & BUTTON_ROLL_LEFT) {
			roll -= 28 * (pml.msec / 200.0f);
		}
		if (cmd->buttons & BUTTON_ROLL_RIGHT) {
			roll += 28 * (pml.msec / 200.0f);
		}

		for (i=0; i<3; i++) {
			// Get the offsets for the angles
			oldCmdAngle = ps->viewangles[i] - SHORT2ANGLE(ps->delta_angles[i]);
			offset_angles[i] = AngleNormalize180(SHORT2ANGLE(cmd->angles[i]) - oldCmdAngle);
		}
		// Don't forget to add our customized roll function if
		// we're not guiding a weapon!
		if (ps->weaponstate != WEAPON_GUIDING && ps->weaponstate != WEAPON_ALTGUIDING) {
			offset_angles[ROLL] = AngleNormalize180(offset_angles[ROLL] + roll);
		}

		// There's a drifting problem with the quaternions' rounding, so when we DON'T change
		// our viewangles at all, we don't run through the quaternions to prevent drifting.
		// (As a bonus; this also means we won't be doing needless matrix manipulations.)

		if (((offset_angles[0] == 0) && (offset_angles[1] == 0)) && (offset_angles[2] == 0)) {
			return;
		}

		AnglesToQuat(ps->viewangles, quatOrient);
		AnglesToQuat(offset_angles, quatRot);
		QuatMul(quatOrient, quatRot, quatResult);
		QuatToAngles(quatResult, new_angles);

		// HACK: Because an exact pitch of -90 degrees STILL causes trouble troughout the
		//       code due to 'bad mathematics', we give a slight nudge if this threatens
		//       to happen. 2 degrees is not really noticeable, and is big enough to
		//       compensate for round off errors leading to -90 degrees of pitch.
		// NOTE: For some reason the program 'auto-magically' prevents angles of 90 degree
		//       pitch, by forcing you into a bit more yaw. Strange...
		AngleNormalize180(new_angles[PITCH]);
		if ( ( new_angles[PITCH] < -89.0f ) && ( new_angles[PITCH] > -91.0f ) ) {
			if ( ps->viewangles[PITCH] < new_angles[PITCH] ) {
				new_angles[PITCH] += 2;
			} else {
				new_angles[PITCH] -= 2;
			}
		}

		// Correct the 3 delta_angles;
		for (i=0; i<3; i++) {
			ps->viewangles[i] = AngleNormalize180(new_angles[i]);
			ps->delta_angles[i] = ANGLE2SHORT(ps->viewangles[i]) - cmd->angles[i];
		}
		return;
	}

	// We've been flying and we've inverted our pitch to a good amount upside down.
	// Cheat the view, by adjusting the yaw to 'swing around' and adjusting the pitch to match.
	pitchNorm = AngleNormalize180(ps->viewangles[PITCH]);
	if ( pitchNorm > 100 ) {
		ps->delta_angles[YAW] += ANGLE2SHORT(180);
		ps->delta_angles[PITCH] -= ANGLE2SHORT(pitchNorm);
		ps->delta_angles[PITCH] += ANGLE2SHORT(180 - pitchNorm);
	}
	if ( pitchNorm < -100 ) {
		ps->delta_angles[YAW] += ANGLE2SHORT(180);
		ps->delta_angles[PITCH] -= ANGLE2SHORT(pitchNorm);
		ps->delta_angles[PITCH] += ANGLE2SHORT(-180 - pitchNorm);
	}

	// END ADDING

	if (lockedOn){
		vec3_t dir;
		vec3_t angles;

		VectorSubtract(pm->target, ps->origin, dir);
		//dir[2] -= ps->viewheight;	// viewers viewheight
		vectoangles(dir, angles);
		if (angles[PITCH] > 180) angles[PITCH] -= 360;
		else if (angles[PITCH] < -180) angles[PITCH] += 360;
		for (i = 0; i < 3; i++) {
			if (i == YAW && (angles[PITCH] > 65 || angles[PITCH] < -65)) continue;
			ps->delta_angles[i] = ANGLE2SHORT(angles[i]) - cmd->angles[i];
		}
	}
	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		// ADDING FOR ZEQ2
		// Incase we've been flying, our roll might've been changed.
		if ( i == ROLL && ps->viewangles[i] != 0) {
			ps->viewangles[i] = 0;
			ps->delta_angles[i] = 0 - cmd->angles[i];
		}
		// END ADDING
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

/*
================
PM_UpdateViewAngles2

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles2( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int			i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return;		// no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[powerLevel] <= 0 ) {
		return;		// no view changes at all
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}

		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}


/*
================
PmoveSingle

================
*/
void trap_SnapVector( float *v ); // <-- forward declaration

void PmoveSingle (pmove_t *pmove) {

	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[powerLevel] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;	// corpses can fly through bodies
	}

	// reset aura and PL state
	pm->ps->eFlags &= ~EF_AURA;

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// don't allow usage of the ki charge button until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		pm->cmd.buttons &= ~BUTTON_BOOST;
	}

	// disable PMF_BOOST_HELD if BUTTON_BOOST is no longer held
	if ( !(pm->cmd.buttons & BUTTON_BOOST) ) {
		pm->ps->pm_flags &= ~PMF_BOOST_HELD;
	}

	// disable PMF_JUMP_HELD if BUTTON_JUMP is no longer positive
	if ( !( pm->cmd.buttons & BUTTON_JUMP )) {
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// disable PMF_LIGHTSPEED HELD if BUTTON_LIGHTSPEED is no longer held
	if ( !(pm->cmd.buttons & BUTTON_LIGHTSPEED) ) {
		pm->ps->pm_flags &= ~PMF_LIGHTSPEED_HELD;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK ) {
		pm->ps->eFlags |= EF_TALK;
	} else {
		pm->ps->eFlags &= ~EF_TALK;
	}

	// clear the respawned flag if attack, alt attack and ki recharge are cleared
	if ( pm->ps->stats[powerLevel] > 0 &&
		!( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_ALT_ATTACK | BUTTON_BOOST ))) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// if we're guiding a weapon, we can't move.
	if ( pmove->ps->weaponstate == WEAPON_GUIDING || pmove->ps->weaponstate == WEAPON_ALTGUIDING ) {
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;

		// disable dashing, boosting and lightspeed
		PM_StopDash();
	}

	// if we're moving up a tier, don't allow movement until the transformation is complete
	if ( pm->ps->powerups[PW_TRANSFORM] > 0 ) {
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;

		// disable dashing, boosting and lightspeed
		PM_StopDash();
		PM_StopBoost();
	}


	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( pml.msec < 1 ) {
		pml.msec = 1;
	} else if ( pml.msec > 200 ) {
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy (pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy (pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// <-- RiO; Build the powerLevel buffer representing regen rate
	PM_BuildBufferHealth();
	// -->

	// update the viewangles
//	if ( pmove->ps->rolling ) {
		PM_UpdateViewAngles( pm->ps, &pm->cmd );
//	} else {
//		PM_UpdateViewAngles2( pm->ps, &pm->cmd );
//	}
	AngleVectors (pm->ps->viewangles, pml.forward, pml.right, pml.up);


	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD ) {
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;

		// Disable flight to prevent suspended corpses.
		pmove->ps->powerups[PW_FLYING] = 0;

		PM_StopDash(); // implicitly stops boost and lightspeed as well
		PM_StopBoost();
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		PM_CheckDuck ();
		PM_FlyMove ();
		PM_DropTimers ();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove ();
		PM_DropTimers ();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE) {
		return;		// no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION) {
		return;		// no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck ();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove ();
	}

	PM_DropTimers();


	// Activate flight if necessary
	if ( !pm->ps->powerups[PW_FLYING] && ( pm->cmd.upmove > 0 || ( !pml.walking && pm->cmd.upmove < 0 ))) {

		// Disable any dashing
		if ( VectorLength( pm->ps->dashDir ) > 0.0f ) {
			PM_StopDash();
		}

		pm->ps->powerups[PW_FLYING] = 1;
	}

	// Activate lightspeed if necessary
	if ( (pm->cmd.buttons & BUTTON_LIGHTSPEED) && 
		!(pm->ps->pm_flags & PMF_LIGHTSPEED_HELD) && 
		!(pm->ps->powerups[PW_TRANSFORM]) &&
		!(pm->ps->weaponstate == WEAPON_GUIDING) && 
		!(pm->ps->weaponstate == WEAPON_ALTGUIDING)) {
		if ( PM_DeductFromHealth( 100 )) {

			// Disable any dashing
			if ( VectorLength( pm->ps->dashDir ) > 0.0f ) {
				PM_StopDash();
			}

			PM_AddEvent( EV_LIGHTSPEED_GHOSTIMAGE );
			pm->ps->powerups[PW_LIGHTSPEED] = 750; // max 0.75 seconds lightspeed
			pm->ps->pm_flags |= PMF_LIGHTSPEED_HELD;
		}
	}

	// Activate transform if necessary
	if (pm->ps->powerups[PW_TRANSFORM]){
		if ( VectorLength( pm->ps->dashDir ) > 0.0f ) {
			PM_StopDash();
			PM_StopBoost();
		}
		PM_Transform();
	} else if ( pm->ps->powerups[PW_LIGHTSPEED] ) {
		PM_LightSpeedMove();
	} else if ( pm->ps->powerups[PW_FLYING] ) {
		PM_FlyMove();
	} else if ( VectorLength( pm->ps->dashDir ) > 0.0f ) {
		PM_DashMove();
	} else if ( pml.walking ) {
		pmove->ps->powerups[PW_FLYING] = 0;
		if ( !(pm->cmd.buttons & BUTTON_WALKING) &&
			  (pm->cmd.forwardmove || pm->cmd.rightmove) &&
			 !(pm->ps->stats[bitFlags] & STATBIT_ALTER_PL) ) {
			PM_DashMove();
		} else {
			PM_WalkMove();
		}
	} else {
		// airborne
		PM_AirMove();
	}

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// footstep events / legs animations
	PM_Footsteps();

	// torso animation
	PM_TorsoAnimation();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector( pm->ps->velocity );

	// set boost aura state
	if ( pm->ps->powerups[PW_BOOST] ) {
		pm->ps->eFlags |= EF_AURA;
	}

	// set tier
	PM_CheckTier();
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove) {
	int			finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return;	// should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int		msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		}
		else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.buttons |= BUTTON_JUMP;
		}

		if ( pmove->ps->pm_flags & PMF_BOOST_HELD ) {
			pmove->cmd.buttons |= BUTTON_BOOST;
		}
	}
}

