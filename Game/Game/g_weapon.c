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
// g_weapon.c 
// perform the server side effects of a weapon firing

#include "g_local.h"

static	float	s_quadFactor;
static	vec3_t	forward, right, up;
static	vec3_t	muzzle;

#define NUM_NAILSHOTS 15

/*
=====================
G_GetMuzzleSettings
=====================
*/
// Botched this together because Q3 stores the muzzle calculations as globals in this unit,
// instead of passing them as parameters. When the old weapons unit is killed off, the
// muzzle calculatons can switch to g_usermissile.c and this function will no longer be
// needed.
void G_GetMuzzleSettings(vec3_t muzzle_g, vec3_t forward_g, vec3_t right_g, vec3_t up_g) {
	VectorCopy(muzzle, muzzle_g);
	VectorCopy(forward, forward_g);
	VectorCopy(right, right_g);
	VectorCopy(up, up_g);
}

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


/*======================================================================
GAUNTLET
======================================================================*/
void Weapon_Gauntlet( gentity_t *ent ){}
qboolean CheckGauntletAttack( gentity_t *ent ) {return qtrue;}

/*======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating 
into a wall.
======================*/
void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int		i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = (int)v[i];
		} else {
			v[i] = (int)v[i] + 1;
		}
	}
}

#ifdef MISSIONPACK
#define CHAINGUN_SPREAD		600
#endif
#define MACHINEGUN_SPREAD	200
#define	MACHINEGUN_DAMAGE	7

void Bullet_Fire (gentity_t *ent, float spread, int damage ) {}

/*======================================================================
BFG
======================================================================*/
void BFG_Fire ( gentity_t *ent ) {}

/*======================================================================
SHOTGUN
======================================================================*/
// DEFAULT_SHOTGUN_SPREAD and DEFAULT_SHOTGUN_COUNT	are in bg_public.h, because
// client predicts same spreads
#define	DEFAULT_SHOTGUN_DAMAGE	10
qboolean ShotgunPellet( vec3_t start, vec3_t end, gentity_t *ent ) {return qtrue;}
void ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ){}
void weapon_supershotgun_fire (gentity_t *ent) {}
void weapon_grenadelauncher_fire (gentity_t *ent){}
void Weapon_RocketLauncher_Fire (gentity_t *ent) {}
void Weapon_Plasmagun_Fire (gentity_t *ent) {}
#define	MAX_RAIL_HITS	4
void weapon_railgun_fire (gentity_t *ent) {}


/*======================================================================
GRAPPLING HOOK
======================================================================*/
void Weapon_GrapplingHook_Fire (gentity_t *ent){}
void Weapon_HookFree (gentity_t *ent){}
void Weapon_HookThink (gentity_t *ent){}
void Weapon_LightningFire( gentity_t *ent ) {}

/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	//FIXME: Old Q3A style adding to the vertical component will no longer work because of
	//       the barrel rolling, switched to a vectorMA function.
	//muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, ent->client->ps.viewheight, up, muzzlePoint );
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

/*
===============
CalcMuzzlePointOrigin

set muzzle location relative to pivoting eye
===============
*/
// FIXME: Why does this function do EXACTLY the same as CalcMuzzlePoint without using the extra
//        vec3_t origin? Stupid! Propose to change all calls to this function to the other one.
void CalcMuzzlePointOrigin ( gentity_t *ent, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	//FIXME: Old Q3A style adding to the vertical component will no longer work because of
	//       the barrel rolling, switched to a vectorMA function.
	//muzzlePoint[2] += ent->client->ps.viewheight;
	VectorMA( muzzlePoint, ent->client->ps.viewheight, up, muzzlePoint );
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

/*===============
FireWeapon
===============*/
void FireWeapon( gentity_t *ent, qboolean altfire ) {
	AngleVectors (ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePointOrigin ( ent, ent->client->oldOrigin, forward, right, up, muzzle );
	Fire_UserWeapon ( ent, muzzle, forward, altfire );
	ent->client->ps.bitFlags ^= hasFlipOffset;
}
