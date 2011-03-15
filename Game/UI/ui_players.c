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
// ui_players.c

#include "ui_local.h"


#define UI_TIMER_GESTURE		2300
#define UI_TIMER_JUMP			1000
#define UI_TIMER_LAND			130
#define UI_TIMER_WEAPON_SWITCH	300
#define UI_TIMER_ATTACK			500
#define	UI_TIMER_MUZZLE_FLASH	20
#define	UI_TIMER_WEAPON_DELAY	250

#define JUMP_HEIGHT				56

#define SWINGSPEED				0.3f

#define SPIN_SPEED				0.9f
#define COAST_TIME				1000


static int			dp_realtime;
static float		jumpHeight;


/*
===============
UI_PlayerInfo_SetWeapon
===============
*/
static void UI_PlayerInfo_SetWeapon( playerInfo_t *pi, weapon_t weaponNum ) {
	char		path[MAX_QPATH];

	pi->currentWeapon = weaponNum;
tryagain:
	pi->realWeapon = weaponNum;
	pi->weaponModel = 0;
	pi->barrelModel = 0;
	pi->flashModel = 0;

	if ( weaponNum == WP_NONE ) {
		return;
	}
	if( pi->weaponModel == 0 ) {
		if( weaponNum == WP_MACHINEGUN ) {
			weaponNum = WP_NONE;
			goto tryagain;
		}
		weaponNum = WP_MACHINEGUN;
		goto tryagain;
	}

	if ( weaponNum == WP_MACHINEGUN || weaponNum == WP_GAUNTLET || weaponNum == WP_BFG ) {
		COM_StripExtension( path, path, sizeof(path) );
		strcat( path, "_barrel.md3" );
		pi->barrelModel = trap_R_RegisterModel( path );
	}

	COM_StripExtension( path, path, sizeof(path) );
	strcat( path, "_flash.md3" );
	pi->flashModel = trap_R_RegisterModel( path );

	switch( weaponNum ) {
	case WP_GAUNTLET:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_MACHINEGUN:
		MAKERGB( pi->flashDlightColor, 1, 1, 0 );
		break;

	case WP_SHOTGUN:
		MAKERGB( pi->flashDlightColor, 1, 1, 0 );
		break;

	case WP_GRENADE_LAUNCHER:
		MAKERGB( pi->flashDlightColor, 1, 0.7f, 0.5f );
		break;

	case WP_ROCKET_LAUNCHER:
		MAKERGB( pi->flashDlightColor, 1, 0.75f, 0 );
		break;

	case WP_LIGHTNING:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_RAILGUN:
		MAKERGB( pi->flashDlightColor, 1, 0.5f, 0 );
		break;

	case WP_PLASMAGUN:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_BFG:
		MAKERGB( pi->flashDlightColor, 1, 0.7f, 1 );
		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	default:
		MAKERGB( pi->flashDlightColor, 1, 1, 1 );
		break;
	}
}


/*
===============
UI_ForceLegsAnim
===============
*/
static void UI_ForceLegsAnim( playerInfo_t *pi, int anim ) {
	pi->legsAnim = ( ( pi->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

	if ( anim == ANIM_JUMP_UP ) {
		pi->legsAnimationTimer = UI_TIMER_JUMP;
	}
}


/*
===============
UI_SetLegsAnim
===============
*/
static void UI_SetLegsAnim( playerInfo_t *pi, int anim ) {
	if ( pi->pendingLegsAnim ) {
		anim = pi->pendingLegsAnim;
		pi->pendingLegsAnim = 0;
	}
	UI_ForceLegsAnim( pi, anim );
}


/*
===============
UI_ForceTorsoAnim
===============
*/
static void UI_ForceTorsoAnim( playerInfo_t *pi, int anim ) {
	pi->torsoAnim = ( ( pi->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
/*
	if ( anim == ANIM_GESTURE ) {
		pi->torsoAnimationTimer = UI_TIMER_GESTURE;
	}

	if ( anim == ANIM_ATTACK || anim == ANIM_ATTACK2 ) {
		pi->torsoAnimationTimer = UI_TIMER_ATTACK;
	}
*/
}


/*
===============
UI_SetTorsoAnim
===============
*/
static void UI_SetTorsoAnim( playerInfo_t *pi, int anim ) {
	if ( pi->pendingTorsoAnim ) {
		anim = pi->pendingTorsoAnim;
		pi->pendingTorsoAnim = 0;
	}

	UI_ForceTorsoAnim( pi, anim );
}


/*
===============
UI_TorsoSequencing
===============
*/
static void UI_TorsoSequencing( playerInfo_t *pi ) {
	int		currentAnim;

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;
/*
	if ( pi->weapon != pi->currentWeapon ) {
		if ( currentAnim != ANIM_DROP ) {
			pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
			UI_ForceTorsoAnim( pi, ANIM_DROP );
		}
	}
*/
	if ( pi->torsoAnimationTimer > 0 ) {
		return;
	}
/*
	if( currentAnim == ANIM_GESTURE ) {
		UI_SetTorsoAnim( pi, ANIM_IDLE );
		return;
	}

	if( currentAnim == ANIM_ATTACK || currentAnim == ANIM_ATTACK2 ) {
		UI_SetTorsoAnim( pi, ANIM_IDLE );
		return;
	}

	if ( currentAnim == ANIM_DROP ) {
		UI_PlayerInfo_SetWeapon( pi, pi->weapon );
		pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
		UI_ForceTorsoAnim( pi, ANIM_RAISE );
		return;
	}

	if ( currentAnim == ANIM_RAISE ) {
		UI_SetTorsoAnim( pi, ANIM_IDLE );
		return;
	}
*/
}


/*
===============
UI_LegsSequencing
===============
*/
static void UI_LegsSequencing( playerInfo_t *pi ) {
	int		currentAnim;

	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;

	if ( pi->legsAnimationTimer > 0 ) {
		if ( currentAnim == ANIM_JUMP_UP ) {
			jumpHeight = JUMP_HEIGHT * sin( M_PI * ( UI_TIMER_JUMP - pi->legsAnimationTimer ) / UI_TIMER_JUMP );
		}
		return;
	}

	if ( currentAnim == ANIM_JUMP_UP ) {
		UI_ForceLegsAnim( pi, ANIM_LAND_UP );
		pi->legsAnimationTimer = UI_TIMER_LAND;
		jumpHeight = 0;
		return;
	}

	if ( currentAnim == ANIM_LAND_UP ) {
		UI_SetLegsAnim( pi, ANIM_IDLE );
		return;
	}
}


/*
======================
UI_PositionEntityOnTag
======================
*/
static void UI_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							clipHandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// cast away const because of compiler problems
	MatrixMultiply( lerped.axis, ((refEntity_t*)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
UI_PositionRotatedEntityOnTag
======================
*/
static void UI_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							clipHandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// cast away const because of compiler problems
	MatrixMultiply( entity->axis, ((refEntity_t *)parent)->axis, tempAxis );
	MatrixMultiply( lerped.axis, tempAxis, entity->axis );
}


/*
===============
UI_SetLerpFrameAnimation
===============
*/
static void UI_SetLerpFrameAnimation( playerInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_ANIMATIONS ) {
		trap_Error( va("Bad animation number: %i", newAnimation) );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}


/*
===============
UI_RunLerpFrame
===============
*/
static void UI_RunLerpFrame( playerInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	int			f;
	animation_t	*anim;

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		UI_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( dp_realtime >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( dp_realtime < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = dp_realtime;
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( dp_realtime > lf->frameTime ) {
			lf->frameTime = dp_realtime;
		}
	}

	if ( lf->frameTime > dp_realtime + 200 ) {
		lf->frameTime = dp_realtime;
	}

	if ( lf->oldFrameTime > dp_realtime ) {
		lf->oldFrameTime = dp_realtime;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( dp_realtime - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
UI_PlayerAnimation
===============
*/
static void UI_PlayerAnimation( playerInfo_t *pi, 
						int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp,
						int *headOld, int *head, float *headBackLerp) {

	// legs animation
	pi->legsAnimationTimer -= uis.frametime;
	if ( pi->legsAnimationTimer < 0 ) {
		pi->legsAnimationTimer = 0;
	}

	UI_LegsSequencing( pi );

	if ( pi->legs.yawing && ( pi->legsAnim & ~ANIM_TOGGLEBIT ) == ANIM_IDLE ) {
		UI_RunLerpFrame( pi, &pi->legs, ANIM_TURN );
	} else {
		UI_RunLerpFrame( pi, &pi->legs, pi->legsAnim );
	}
	*legsOld = pi->legs.oldFrame;
	*legs = pi->legs.frame;
	*legsBackLerp = pi->legs.backlerp;

	// torso animation
	pi->torsoAnimationTimer -= uis.frametime;
	if ( pi->torsoAnimationTimer < 0 ) {
		pi->torsoAnimationTimer = 0;
	}

	UI_TorsoSequencing( pi );

	UI_RunLerpFrame( pi, &pi->torso, pi->torsoAnim );
	*torsoOld = pi->torso.oldFrame;
	*torso = pi->torso.frame;
	*torsoBackLerp = pi->torso.backlerp;

	// ADDING FOR ZEQ2
	{
		int				torsoAnimNum;
		int				legsAnimNum;

		// NOTE: Torso animations take precedence over leg animations when deciding which
		//       head animation to play.
		torsoAnimNum = pi->torsoAnim & ~ANIM_TOGGLEBIT;
		if ( ANIM_FLY_UP == torsoAnimNum && pi->overrideHead ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KI_CHARGE );
		} else if ( ANIM_FLY_DOWN == torsoAnimNum && pi->overrideHead ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KI_CHARGE );
		} else if ( ANIM_FLOOR_RECOVER == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLOOR_RECOVER );
		} else if ( ANIM_WALK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_WALK );
		} else if ( ANIM_RUN == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_RUN );
		} else if ( ANIM_JUMP_UP == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_JUMP_UP );
		} else if ( ANIM_LAND_UP == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_LAND_UP );
		} else if ( ANIM_JUMP_FORWARD == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_JUMP_FORWARD );
		} else if ( ANIM_LAND_FORWARD == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_LAND_FORWARD );
		} else if ( ANIM_JUMP_BACK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_JUMP_BACK );
		} else if ( ANIM_LAND_BACK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_LAND_BACK );
		} else if ( ANIM_SWIM_IDLE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_SWIM_IDLE );
		} else if ( ANIM_SWIM == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_SWIM );
		} else if ( ANIM_DASH_RIGHT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_DASH_RIGHT );
		} else if ( ANIM_DASH_LEFT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_DASH_LEFT );
		} else if ( ANIM_DASH_FORWARD == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_DASH_FORWARD );
		} else if ( ANIM_DASH_BACKWARD == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_DASH_BACKWARD );
		} else if ( ANIM_KI_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KI_CHARGE );
		} else if ( ANIM_PL_DOWN == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_PL_DOWN );
		} else if ( ANIM_PL_UP == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_PL_UP );
		} else if ( ANIM_TRANS_UP == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_TRANS_UP );
		} else if ( ANIM_TRANS_BACK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_TRANS_BACK );
		} else if ( ANIM_FLY_IDLE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLY_IDLE );
		} else if ( ANIM_FLY_START == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLY_START );
		} else if ( ANIM_FLY_FORWARD == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLY_FORWARD );
		} else if ( ANIM_FLY_BACKWARD == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLY_BACKWARD );
		} else if ( ANIM_FLY_UP == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLY_UP );
		} else if ( ANIM_FLY_DOWN == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_FLY_DOWN );
		} else if ( ANIM_STUNNED == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_STUNNED );
		} else if ( ANIM_PUSH == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_PUSH );
		} else if ( ANIM_DEFLECT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_DEFLECT );
		} else if ( ANIM_BLOCK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BLOCK );
		} else if ( ANIM_SPEED_MELEE_ATTACK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_SPEED_MELEE_ATTACK );
		} else if ( ANIM_SPEED_MELEE_DODGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_SPEED_MELEE_DODGE );
		} else if ( ANIM_SPEED_MELEE_BLOCK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_SPEED_MELEE_BLOCK );
		} else if ( ANIM_SPEED_MELEE_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_SPEED_MELEE_HIT );
		} else if ( ANIM_BREAKER_MELEE_ATTACK1 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_ATTACK1 );
		} else if ( ANIM_BREAKER_MELEE_ATTACK2 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_ATTACK2 );
		} else if ( ANIM_BREAKER_MELEE_ATTACK3 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_ATTACK3 );
		} else if ( ANIM_BREAKER_MELEE_ATTACK4 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_ATTACK4 );
		} else if ( ANIM_BREAKER_MELEE_ATTACK5 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_ATTACK5 );
		} else if ( ANIM_BREAKER_MELEE_ATTACK6 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_ATTACK6 );
		} else if ( ANIM_BREAKER_MELEE_HIT1 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_HIT1 );
		} else if ( ANIM_BREAKER_MELEE_HIT2 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_HIT2 );
		} else if ( ANIM_BREAKER_MELEE_HIT3 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_HIT3 );
		} else if ( ANIM_BREAKER_MELEE_HIT4 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_HIT4 );
		} else if ( ANIM_BREAKER_MELEE_HIT5 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_HIT5 );
		} else if ( ANIM_BREAKER_MELEE_HIT6 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_BREAKER_MELEE_HIT6 );
		} else if ( ANIM_POWER_MELEE_1_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_1_CHARGE );
		} else if ( ANIM_POWER_MELEE_2_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_2_CHARGE );
		} else if ( ANIM_POWER_MELEE_3_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_3_CHARGE );
		} else if ( ANIM_POWER_MELEE_4_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_4_CHARGE );
		} else if ( ANIM_POWER_MELEE_5_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_5_CHARGE );
		} else if ( ANIM_POWER_MELEE_6_CHARGE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_6_CHARGE );
		} else if ( ANIM_POWER_MELEE_1_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_1_HIT );
		} else if ( ANIM_POWER_MELEE_2_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_2_HIT );
		} else if ( ANIM_POWER_MELEE_3_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_3_HIT );
		} else if ( ANIM_POWER_MELEE_4_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_4_HIT );
		} else if ( ANIM_POWER_MELEE_5_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_5_HIT );
		} else if ( ANIM_POWER_MELEE_6_HIT == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_POWER_MELEE_6_HIT );
		} else if ( ANIM_STUNNED_MELEE == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_STUNNED_MELEE );
		} else if ( ANIM_KNOCKBACK == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KNOCKBACK );
		} else if ( ANIM_KNOCKBACK_HIT_WALL == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KNOCKBACK_HIT_WALL );
		} else if ( ANIM_KNOCKBACK_RECOVER_1 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KNOCKBACK_RECOVER_1 );
		} else if ( ANIM_KNOCKBACK_RECOVER_2 == torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, ANIM_KNOCKBACK_RECOVER_2 );
		} else if ( ANIM_KI_ATTACK1_PREPARE <= torsoAnimNum && ANIM_KI_ATTACK6_ALT_FIRE >= torsoAnimNum ) {
			UI_RunLerpFrame( pi, &pi->head, torsoAnimNum - ANIM_KI_ATTACK1_PREPARE + ANIM_KI_ATTACK1_PREPARE );
		} else {
			legsAnimNum = pi->legsAnim & ~ANIM_TOGGLEBIT;
			if ( 0 ) {
			} else {
				UI_RunLerpFrame( pi, &pi->head, ANIM_IDLE );
			}
		}
	}

	*headOld = pi->head.oldFrame;
	*head = pi->head.frame;
	*headBackLerp = pi->head.backlerp;

	// END ADDING
}


/*
==================
UI_SwingAngles
==================
*/
static void UI_SwingAngles( float destination, float swingTolerance, float clampTolerance,
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
		move = uis.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = uis.frametime * scale * -speed;
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
======================
UI_MovedirAdjustment
======================
*/
static float UI_MovedirAdjustment( playerInfo_t *pi ) {
	vec3_t		relativeAngles;
	vec3_t		moveVector;

	VectorSubtract( pi->viewAngles, pi->moveAngles, relativeAngles );
	AngleVectors( relativeAngles, moveVector, NULL, NULL );
	if ( Q_fabs( moveVector[0] ) < 0.01 ) {
		moveVector[0] = 0.0;
	}
	if ( Q_fabs( moveVector[1] ) < 0.01 ) {
		moveVector[1] = 0.0;
	}

	if ( moveVector[1] == 0 && moveVector[0] > 0 ) {
		return 0;
	}
	if ( moveVector[1] < 0 && moveVector[0] > 0 ) {
		return 22;
	}
	if ( moveVector[1] < 0 && moveVector[0] == 0 ) {
		return 45;
	}
	if ( moveVector[1] < 0 && moveVector[0] < 0 ) {
		return -22;
	}
	if ( moveVector[1] == 0 && moveVector[0] < 0 ) {
		return 0;
	}
	if ( moveVector[1] > 0 && moveVector[0] < 0 ) {
		return 22;
	}
	if ( moveVector[1] > 0 && moveVector[0] == 0 ) {
		return  -45;
	}

	return -22;
}


/*
===============
UI_PlayerAngles
===============
*/
static void UI_PlayerAngles( playerInfo_t *pi, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	float		adjust;

	VectorCopy( pi->viewAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( pi->legsAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE 
		|| ( pi->torsoAnim & ~ANIM_TOGGLEBIT ) != ANIM_IDLE  ) {
		// if not standing still, always point all in the same direction
		pi->torso.yawing = qtrue;	// always center
		pi->torso.pitching = qtrue;	// always center
		pi->legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	adjust = UI_MovedirAdjustment( pi );
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * adjust;


	// torso
	UI_SwingAngles( torsoAngles[YAW], 25, 90, SWINGSPEED, &pi->torso.yawAngle, &pi->torso.yawing );
	UI_SwingAngles( legsAngles[YAW], 40, 90, SWINGSPEED, &pi->legs.yawAngle, &pi->legs.yawing );

	torsoAngles[YAW] = pi->torso.yawAngle;
	legsAngles[YAW] = pi->legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75;
	} else {
		dest = headAngles[PITCH] * 0.75;
	}
	UI_SwingAngles( dest, 15, 30, 0.1f, &pi->torso.pitchAngle, &pi->torso.pitching );
	torsoAngles[PITCH] = pi->torso.pitchAngle;

	// torso rotation locked?
//	if ( pi->fixedtorso ) {
		torsoAngles[PITCH] = 0.0f;
//	}

//	if ( pi->fixedlegs ) {
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
//	}
	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


/*
===============
UI_PlayerFloatSprite
===============
*/
static void UI_PlayerFloatSprite( playerInfo_t *pi, vec3_t origin, qhandle_t shader ) {
	refEntity_t		ent;

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( origin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = 0;
	trap_R_AddRefEntityToScene( &ent );
}


/*
======================
UI_MachinegunSpinAngle
======================
*/
float	UI_MachinegunSpinAngle( playerInfo_t *pi ) {
	int		delta;
	float	angle;
	float	speed;
	int		torsoAnim;
/*
	delta = dp_realtime - pi->barrelTime;
	if ( pi->barrelSpinning ) {
		angle = pi->barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = pi->barrelAngle + delta * speed;
	}

	torsoAnim = pi->torsoAnim  & ~ANIM_TOGGLEBIT;

	if( torsoAnim == ANIM_ATTACK2 ) {
		torsoAnim = ANIM_ATTACK;
	}
	if ( pi->barrelSpinning == !(torsoAnim == ANIM_ATTACK) ) {
		pi->barrelTime = dp_realtime;
		pi->barrelAngle = AngleMod( angle );
		pi->barrelSpinning = !!(torsoAnim == ANIM_ATTACK);
	}
*/
	return angle;
}


/*
===============
UI_DrawPlayer
===============
*/
void UI_DrawPlayer( float x, float y, float w, float h, playerInfo_t *pi, int time ) {
	refdef_t		refdef;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	refEntity_t		gun;
	refEntity_t		barrel;
	refEntity_t		flash;
	vec3_t			origin;
	int				renderfx;
	vec3_t			mins = {-16, -16, -24};
	vec3_t			maxs = {16, 16, 32};
	float			len;
	float			xx;

	if(pi->legsModel && !pi->torsoModel && !pi->headModel)
	{
		UI_DrawPlayer_zMesh(x,y,w,h,pi,time);
		return;
	}
	if ( !pi->legsModel || !pi->torsoModel || !pi->headModel || !pi->animations[0].numFrames ) {
		return;
	}

	dp_realtime = time;

	if ( pi->pendingWeapon != -1 && dp_realtime > pi->weaponTimer ) {
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
	}

	UI_AdjustFrom640( &x, &y, &w, &h );

	y -= jumpHeight;

	memset( &refdef, 0, sizeof( refdef ) );
	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = (int)((float)refdef.width / 640.0f * 35.0f);
	xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2( refdef.height, xx );
	refdef.fov_y *= ( 360 / M_PI );

	// calculate distance so the player nearly fills the box
	len = 0.6 * ( maxs[2] - mins[2] );		
	origin[0] = len / tan( DEG2RAD(refdef.fov_x) * 0.5 );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	origin[2] = -0.5 * ( mins[2] + maxs[2] );

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	// get the rotation information
	UI_PlayerAngles( pi, legs.axis, torso.axis, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	UI_PlayerAnimation( pi, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp,
		 &head.oldframe, &head.frame, &head.backlerp);

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	//
	// add the legs
	//
	legs.hModel = pi->legsModel;
	legs.customSkin = pi->legsSkin;

	VectorCopy( origin, legs.origin );

	VectorCopy( origin, legs.lightingOrigin );
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);

	trap_R_AddRefEntityToScene( &legs );

	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = pi->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = pi->torsoSkin;

	VectorCopy( origin, torso.lightingOrigin );

	UI_PositionRotatedEntityOnTag( &torso, &legs, pi->legsModel, "tag_torso");

	torso.renderfx = renderfx;

	trap_R_AddRefEntityToScene( &torso );

	//
	// add the head
	//
	head.hModel = pi->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = pi->headSkin;

	VectorCopy( origin, head.lightingOrigin );

	UI_PositionRotatedEntityOnTag( &head, &torso, pi->torsoModel, "tag_head");

	head.renderfx = renderfx;

	trap_R_AddRefEntityToScene( &head );
/*
	//
	// add the gun
	//
	if ( pi->currentWeapon != WP_NONE ) {
		memset( &gun, 0, sizeof(gun) );
		gun.hModel = pi->weaponModel;
		VectorCopy( origin, gun.lightingOrigin );
		UI_PositionEntityOnTag( &gun, &torso, pi->torsoModel, "tag_weapon");
		gun.renderfx = renderfx;
		trap_R_AddRefEntityToScene( &gun );
	}

	//
	// add the spinning barrel
	//
	if ( pi->realWeapon == WP_MACHINEGUN || pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG ) {
		vec3_t	angles;

		memset( &barrel, 0, sizeof(barrel) );
		VectorCopy( origin, barrel.lightingOrigin );
		barrel.renderfx = renderfx;

		barrel.hModel = pi->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = UI_MachinegunSpinAngle( pi );
		if( pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG ) {
			angles[PITCH] = angles[ROLL];
			angles[ROLL] = 0;
		}
		AnglesToAxis( angles, barrel.axis );

		UI_PositionRotatedEntityOnTag( &barrel, &gun, pi->weaponModel, "tag_barrel");

		trap_R_AddRefEntityToScene( &barrel );
	}

	//
	// add muzzle flash
	//
	if ( dp_realtime <= pi->muzzleFlashTime ) {
		if ( pi->flashModel ) {
			memset( &flash, 0, sizeof(flash) );
			flash.hModel = pi->flashModel;
			VectorCopy( origin, flash.lightingOrigin );
			UI_PositionEntityOnTag( &flash, &gun, pi->weaponModel, "tag_flash");
			flash.renderfx = renderfx;
			trap_R_AddRefEntityToScene( &flash );
		}

		// make a dlight for the flash
		if ( pi->flashDlightColor[0] || pi->flashDlightColor[1] || pi->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 200 + (rand()&31), pi->flashDlightColor[0],
				pi->flashDlightColor[1], pi->flashDlightColor[2] );
		}
	}
*/
	//
	// add the chat icon
	//
	if ( pi->chat ) {
		UI_PlayerFloatSprite( pi, origin, trap_R_RegisterShaderNoMip( "chatBubble" ) );
	}

	//
	// add an accent light
	//
	origin[0] -= 0;//100;	// + = behind, - = in front
	origin[1] += 100;	// + = left, - = right
	origin[2] += 0;//100;	// + = above, - = below
	trap_R_AddLightToScene( origin, 500, 1.0, 1.0, 1.0 );
/*
	origin[0] -= 100;
	origin[1] -= 100;
	origin[2] -= 100;
	trap_R_AddLightToScene( origin, 500, 1.0, 0.0, 0.0 );
*/
	trap_R_RenderScene( &refdef );
}
void UI_DrawPlayer_zMesh( float x, float y, float w, float h, playerInfo_t *pi, int time ) {
	refdef_t		refdef;
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	refEntity_t		gun;
	refEntity_t		barrel;
	refEntity_t		flash;
	vec3_t			origin;
	int				renderfx;
	vec3_t			mins = {-16, -16, -24};
	vec3_t			maxs = {16, 16, 32};
	float			len;
	float			xx;

	if ( !pi->legsModel || !pi->animations[0].numFrames ) {
		return;
	}

	dp_realtime = time;

	if ( pi->pendingWeapon != -1 && dp_realtime > pi->weaponTimer ) {
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
	}

	UI_AdjustFrom640( &x, &y, &w, &h );

	y -= jumpHeight;

	memset( &refdef, 0, sizeof( refdef ) );
	memset( &legs, 0, sizeof(legs) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = (int)((float)refdef.width / 640.0f * 35.0f);
	xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2( refdef.height, xx );
	refdef.fov_y *= ( 360 / M_PI );

	// calculate distance so the player nearly fills the box
	len = 0.6 * ( maxs[2] - mins[2] );		
	origin[0] = len / tan( DEG2RAD(refdef.fov_x) * 0.5 );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	origin[2] = -0.5 * ( mins[2] + maxs[2] );

	refdef.time = dp_realtime;

	trap_R_ClearScene();

	// get the rotation information
	UI_PlayerAngles( pi, legs.axis, torso.axis, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	UI_PlayerAnimation( pi, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp,
		 &head.oldframe, &head.frame, &head.backlerp);

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

	//
	// add the legs
	//
	legs.hModel = pi->legsModel;
	legs.customSkin = pi->legsSkin;

	VectorCopy( origin, legs.origin );

	VectorCopy( origin, legs.lightingOrigin );
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);

	trap_R_AddRefEntityToScene( &legs );

	if (!legs.hModel) {
		return;
	}

	//
	// add the chat icon
	//
	if ( pi->chat ) {
		UI_PlayerFloatSprite( pi, origin, trap_R_RegisterShaderNoMip( "chatBubble" ) );
	}

	//
	// add an accent light
	//
	origin[0] -= 0;//100;	// + = behind, - = in front
	origin[1] += 100;	// + = left, - = right
	origin[2] += 0;//100;	// + = above, - = below
	trap_R_AddLightToScene( origin, 500, 1.0, 1.0, 1.0 );
/*
	origin[0] -= 100;
	origin[1] -= 100;
	origin[2] -= 100;
	trap_R_AddLightToScene( origin, 500, 1.0, 0.0, 0.0 );
*/
	trap_R_RenderScene( &refdef );
}


/*
==========================
UI_RegisterClientSkin
==========================
*/
static qboolean UI_RegisterClientSkin( playerInfo_t *pi, const char *modelName, const char *skinName ) {
	char		filename[MAX_QPATH];
	char headPrefix[8],upperPrefix[8],lowerPrefix[8];
	strcpy(headPrefix,"head_");
	strcpy(lowerPrefix,"lower_");
	strcpy(upperPrefix,"upper_");
	Com_sprintf(filename,sizeof(filename),"players//%s/tier1/%s.skin",modelName,skinName);
	if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
		strcpy(headPrefix,"");
		strcpy(lowerPrefix,"");
		strcpy(upperPrefix,"");
	}
	Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/%s%s.skin", modelName, lowerPrefix,skinName );
	pi->legsSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/%s%s.skin", modelName, upperPrefix,skinName );
	pi->torsoSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/%s%s.skin", modelName, headPrefix,skinName );
	pi->headSkin = trap_R_RegisterSkin( filename );

	if ( !pi->legsSkin || !pi->torsoSkin || !pi->headSkin ) {
		return qfalse;
	}

	return qtrue;
}


/*
======================
UI_ParseAnimationFile
======================
*/
static qboolean UI_ParseAnimationFile( const char *filename, animation_t *animations, playerInfo_t *pi ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;

	memset( animations, 0, sizeof( animation_t ) * MAX_ANIMATIONS );

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= ( sizeof( text ) - 1 ) ) {
		Com_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	pi->fixedlegs = qfalse;
	pi->fixedtorso = qfalse;
	pi->overrideHead = qfalse;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token ) {
					break;
				}
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			pi->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			pi->fixedtorso = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "overrideHead" ) ) {
			pi->overrideHead = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}

		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}

	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		animations[i].firstFrame = atoi( token );
/*
		// leg only frames are adjusted to not count the upper body only frames
		if ( i == ANIM_WALKCR ) {
			skip = animations[ANIM_WALKCR].firstFrame - animations[ANIM_GESTURE].firstFrame;
		}
		if ( i >= ANIM_WALKCR ) {
			animations[i].firstFrame -= skip;
		}
*/
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		animations[i].numFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		animations[i].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token ) {
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
		if ( i >= ANIM_KI_ATTACK1_FIRE &&
			 i < MAX_ANIMATIONS &&
			 (i - ANIM_KI_ATTACK1_FIRE) % 2 == 0 ) {
			token = COM_Parse( &text_p );
			if ( !*token ) {
				break;
			}
			animations[i].continuous = atoi( token );
		}

		// END ADDING
	}

	if ( i != MAX_ANIMATIONS ) {
		Com_Printf( "Error parsing animation file: %s", filename );
		return qfalse;
	}

	return qtrue;
}


/*
==========================
UI_RegisterClientModelname
==========================
*/
qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName ) {
	char		modelName[MAX_QPATH];
	char		skinName[MAX_QPATH];
	char		filename[MAX_QPATH];
	char		*slash;

	pi->torsoModel = 0;
	pi->headModel = 0;

	if ( !modelSkinName[0] ) {
		return qfalse;
	}

	Q_strncpyz( modelName, modelSkinName, sizeof( modelName ) );

	slash = strchr( modelName, '/' );
	if ( !slash ) {
		// modelName did not include a skin name
		Q_strncpyz( skinName, "default", sizeof( skinName ) );
	} else {
		Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
		// truncate modelName
		*slash = 0;
	}

	// load cmodels before models so filecache works
	
	Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/body.zMesh", modelName );
	pi->legsModel = trap_R_RegisterModel( filename );
	if ( !pi->legsModel ) {
		//Com_Printf( "Failed to load model file %s, trying players//%s/tier1/body.zMesh\n", filename, modelName );
	

		Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/lower.md3", modelName );
		pi->legsModel = trap_R_RegisterModel( filename );
		if ( !pi->legsModel ) {
			//Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}

		Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/upper.md3", modelName );
		pi->torsoModel = trap_R_RegisterModel( filename );
		if ( !pi->torsoModel ) {
			//Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}

		Com_sprintf( filename, sizeof( filename ), "players//%s/tier1/head.md3", modelName );
		pi->headModel = trap_R_RegisterModel( filename );
		if ( !pi->headModel ) {
			//Com_Printf( "Failed to load model file %s\n", filename );
			return qfalse;
		}
	}
	// if any skins failed to load, fall back to default
	if ( !UI_RegisterClientSkin( pi, modelName, skinName ) ) {
		if ( !UI_RegisterClientSkin( pi, modelName, "default" ) ) {
			//Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
			return qfalse;
		}
	}

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "players//%s/animation.cfg", modelName );
	if ( !UI_ParseAnimationFile( filename, pi->animations, pi ) ) {
		//Com_Printf( "Failed to load animation file %s\n", filename );
		return qfalse;
	}

	return qtrue;
}


/*
===============
UI_PlayerInfo_SetModel
===============
*/
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model ) {
	memset( pi, 0, sizeof(*pi) );
	UI_RegisterClientModelname( pi, model );
	pi->weapon = WP_MACHINEGUN;
	pi->currentWeapon = pi->weapon;
	pi->lastWeapon = pi->weapon;
	pi->pendingWeapon = -1;
	pi->weaponTimer = 0;
	pi->chat = qfalse;
	pi->newModel = qtrue;
	UI_PlayerInfo_SetWeapon( pi, pi->weapon );
}


/*
===============
UI_PlayerInfo_SetInfo
===============
*/
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNumber, qboolean chat ) {
	int			currentAnim;
	weapon_t	weaponNum;

	pi->chat = chat;

	// view angles
	VectorCopy( viewAngles, pi->viewAngles );

	// move angles
	VectorCopy( moveAngles, pi->moveAngles );

	if ( pi->newModel ) {
		pi->newModel = qfalse;

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );
		pi->legs.yawAngle = viewAngles[YAW];
		pi->legs.yawing = qfalse;

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );
		pi->torso.yawAngle = viewAngles[YAW];
		pi->torso.yawing = qfalse;
/*
		pi->pendingHeadAnim = 0;
		UI_ForceHeadAnim( pi, headAnim );
		pi->head.yawAngle = viewAngles[YAW];
		pi->head.yawing = qfalse;
*/
		if ( weaponNumber != -1 ) {
			pi->weapon = weaponNumber;
			pi->currentWeapon = weaponNumber;
			pi->lastWeapon = weaponNumber;
			pi->pendingWeapon = -1;
			pi->weaponTimer = 0;
			UI_PlayerInfo_SetWeapon( pi, pi->weapon );
		}

		return;
	}

	// weapon
	if ( weaponNumber == -1 ) {
		pi->pendingWeapon = -1;
		pi->weaponTimer = 0;
	}
	else if ( weaponNumber != WP_NONE ) {
		pi->pendingWeapon = weaponNumber;
		pi->weaponTimer = dp_realtime + UI_TIMER_WEAPON_DELAY;
	}
	weaponNum = pi->lastWeapon;
	pi->weapon = weaponNum;

	if ( torsoAnim == ANIM_DEATH_GROUND || legsAnim == ANIM_DEATH_GROUND ) {
		torsoAnim = legsAnim = ANIM_DEATH_GROUND;
		pi->weapon = pi->currentWeapon = WP_NONE;
		UI_PlayerInfo_SetWeapon( pi, pi->weapon );

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );

		return;
	}

	// leg animation
	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;
	if ( legsAnim != ANIM_JUMP_UP && ( currentAnim == ANIM_JUMP_UP || currentAnim == ANIM_LAND_UP ) ) {
		pi->pendingLegsAnim = legsAnim;
	}
	else if ( legsAnim != currentAnim ) {
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );
	}

	// torso animation
	if ( torsoAnim == ANIM_IDLE || torsoAnim == ANIM_IDLE_LOCKED ) {
		if ( weaponNum == WP_NONE || weaponNum == WP_GAUNTLET ) {
			torsoAnim = ANIM_IDLE;
		}
		else {
			torsoAnim = ANIM_IDLE;
		}
	}
/*
	if ( torsoAnim == ANIM_ATTACK || torsoAnim == ANIM_ATTACK2 ) {
		if ( weaponNum == WP_NONE || weaponNum == WP_GAUNTLET ) {
			torsoAnim = ANIM_ATTACK2;
		}
		else {
			torsoAnim = ANIM_ATTACK;
		}
		pi->muzzleFlashTime = dp_realtime + UI_TIMER_MUZZLE_FLASH;
		//FIXME play firing sound here
	}
*/
	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;
/*
	if ( weaponNum != pi->currentWeapon || currentAnim == ANIM_RAISE || currentAnim == ANIM_DROP ) {
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if ( ( currentAnim == ANIM_GESTURE || currentAnim == ANIM_ATTACK ) && ( torsoAnim != currentAnim ) ) {
		pi->pendingTorsoAnim = torsoAnim;
	}
	else */if ( torsoAnim != currentAnim ) {
		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );
	}

}
