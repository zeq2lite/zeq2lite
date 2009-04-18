// Copyright (C) 2003-2004 RiO
//
// bg_userweapons.c -- Both games custom weapons database and parser functions.
//                     Does NOT contain the actual loading of a file, but expects
//                     a pointer to a loaded string instead.

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "bg_userweapons.h"

static userWeapon_t		weaponDatabase[MAX_USERWEAPONS];
static int				num_regWeapons;	// Initialize right away!


int BG_GetWeaponListLength( void ) {
	return num_regWeapons;
}

userWeapon_t *BG_FindUserWeaponWithIndex( int index ) {
	if ( index  >= num_regWeapons ) {
		return NULL;
	}
	return &weaponDatabase[index];
}

void BG_ParseWeaponList( void ) {
	// NOTE: No parser written yet; The language hasn't been defined yet.
	//		 For now, we hardcode a few weapons.

	num_regWeapons = 7;

	// --< KIKOU >--

	weaponDatabase[0].accel = 0;
	weaponDatabase[0].altFireNr = 1;
	weaponDatabase[0].autoFire = qfalse;
	weaponDatabase[0].blinding = qfalse;
	weaponDatabase[0].blockable = qtrue;
	weaponDatabase[0].bounceAmt = 0;
	weaponDatabase[0].bounceDmPct = 0;
	weaponDatabase[0].bounceVPct = 0;
	weaponDatabase[0].charge = qtrue;
	weaponDatabase[0].chargeRatio  = 5;
	weaponDatabase[0].chargeReady = 0;
	weaponDatabase[0].chargeTime = 100;
	weaponDatabase[0].chrgDamMod = 2;
	weaponDatabase[0].chrgRadMod = 1;
	weaponDatabase[0].chrgSplMod = 1;
	weaponDatabase[0].coneAngleX = 0;
	weaponDatabase[0].coneAngleY = 0;
	weaponDatabase[0].contExplsn = qfalse;
	weaponDatabase[0].coolTime = 1000;
	weaponDatabase[0].damage = 20;
	weaponDatabase[0].damageSelf = qfalse;
	weaponDatabase[0].decayRate = 0;
	weaponDatabase[0].deflecting = qfalse;
	weaponDatabase[0].destroyable = qtrue;
	weaponDatabase[0].detonatable = qfalse;
	weaponDatabase[0].gravity = 0;
	weaponDatabase[0].hold = qfalse;
	weaponDatabase[0].homingAccel = 0;
	weaponDatabase[0].homingAngle = 0;
	weaponDatabase[0].homingRange = 0;
	weaponDatabase[0].homingType = HOM_NONE;
	weaponDatabase[0].hpCost = 0;
	weaponDatabase[0].kiCost = 5;
	weaponDatabase[0].knockback = 500;
	weaponDatabase[0].lifetime = 10000;
	weaponDatabase[0].lowPLBound = 0;
	weaponDatabase[0].max_speed = 500;
	weaponDatabase[0].MOD = UMOD_KI;
	weaponDatabase[0].multiShot = 1;
	weaponDatabase[0].offsetX = 0;
	weaponDatabase[0].offsetY = 0;
	weaponDatabase[0].overrideDir[0] = 0;
	weaponDatabase[0].overrideDir[1] = 0;
	weaponDatabase[0].overrideDir[2] = 0;
	weaponDatabase[0].piercing = qfalse;
	weaponDatabase[0].radius = 100;
	weaponDatabase[0].range = 0;
	weaponDatabase[0].spawnAmnt = 0;
	weaponDatabase[0].spawnNr = -1;
	weaponDatabase[0].speed = 500;
	weaponDatabase[0].splRadius = 200;
	weaponDatabase[0].stamCost = 0;
	weaponDatabase[0].struggling = qfalse;
	weaponDatabase[0].transKi2Hp = qtrue;
	weaponDatabase[0].uppPLBound = 100;
	weaponDatabase[0].useDecay = qfalse;
	weaponDatabase[0].vortex = qfalse;
	weaponDatabase[0].weapType = WPT_MISSILE;


	// --< RENZOKU ENERGY DAN >--

	weaponDatabase[1].accel = 0;
	weaponDatabase[1].altFireNr = -1;	// <-- Can't altfire an altfire attack
	weaponDatabase[1].autoFire = qfalse;
	weaponDatabase[1].blinding = qfalse;
	weaponDatabase[1].blockable = qtrue;
	weaponDatabase[1].bounceAmt = 0;
	weaponDatabase[1].bounceDmPct = 0;
	weaponDatabase[1].bounceVPct = 0;
	weaponDatabase[1].charge = qfalse;
	weaponDatabase[1].chargeRatio  = 0;
	weaponDatabase[1].chargeReady = 0;
	weaponDatabase[1].chargeTime = 0;
	weaponDatabase[1].chrgDamMod = 0;
	weaponDatabase[1].chrgRadMod = 0;
	weaponDatabase[1].chrgSplMod = 0;
	weaponDatabase[1].coneAngleX = 0;
	weaponDatabase[1].coneAngleY = 0;
	weaponDatabase[1].contExplsn = qfalse;
	weaponDatabase[1].coolTime = 400;
	weaponDatabase[1].damage = 15;
	weaponDatabase[1].damageSelf = qfalse;
	weaponDatabase[1].decayRate = 0;
	weaponDatabase[1].deflecting = qfalse;
	weaponDatabase[1].destroyable = qtrue;
	weaponDatabase[1].detonatable = qfalse;
	weaponDatabase[1].gravity = 0;
	weaponDatabase[1].hold = qfalse;
	weaponDatabase[1].homingAccel = 0;
	weaponDatabase[1].homingAngle = 0;
	weaponDatabase[1].homingRange = 0;
	weaponDatabase[1].homingType = HOM_NONE;
	weaponDatabase[1].hpCost = 0;
	weaponDatabase[1].kiCost = 2;
	weaponDatabase[1].knockback = 500;
	weaponDatabase[1].lifetime = 10000;
	weaponDatabase[1].lowPLBound = 0;
	weaponDatabase[1].max_speed = 650;
	weaponDatabase[1].MOD = UMOD_KI;
	weaponDatabase[1].multiShot = 1;
	weaponDatabase[1].offsetX = 0;
	weaponDatabase[1].offsetY = 0;
	weaponDatabase[1].overrideDir[0] = 0;
	weaponDatabase[1].overrideDir[1] = 0;
	weaponDatabase[1].overrideDir[2] = 0;
	weaponDatabase[1].piercing = qfalse;
	weaponDatabase[1].radius = 100;
	weaponDatabase[1].range = 0;
	weaponDatabase[1].spawnAmnt = 0;
	weaponDatabase[1].spawnNr = -1;
	weaponDatabase[1].speed = 650;
	weaponDatabase[1].splRadius = 200;
	weaponDatabase[1].stamCost = 0;
	weaponDatabase[1].struggling = qfalse;
	weaponDatabase[1].transKi2Hp = qfalse;
	weaponDatabase[1].uppPLBound = 100;
	weaponDatabase[1].useDecay = qfalse;
	weaponDatabase[1].vortex = qfalse;
	weaponDatabase[1].weapType = WPT_MISSILE;


	// --< BIG BANG ATTACK (BALL) >--

	weaponDatabase[2].accel = 0;
	weaponDatabase[2].altFireNr = 3;
	weaponDatabase[2].autoFire = qfalse;
	weaponDatabase[2].blinding = qfalse;
	weaponDatabase[2].blockable = qtrue;
	weaponDatabase[2].bounceAmt = 0;
	weaponDatabase[2].bounceDmPct = 0;
	weaponDatabase[2].bounceVPct = 0;
	weaponDatabase[2].charge = qtrue;
	weaponDatabase[2].chargeRatio  = 3;
	weaponDatabase[2].chargeReady = 0;
	weaponDatabase[2].chargeTime = 250;
	weaponDatabase[2].chrgDamMod = 2;
	weaponDatabase[2].chrgRadMod = 1;
	weaponDatabase[2].chrgSplMod = 1;
	weaponDatabase[2].coneAngleX = 0;
	weaponDatabase[2].coneAngleY = 0;
	weaponDatabase[2].contExplsn = qfalse;
	weaponDatabase[2].coolTime = 1500;
	weaponDatabase[2].damage = 40;
	weaponDatabase[2].damageSelf = qfalse;
	weaponDatabase[2].decayRate = 0;
	weaponDatabase[2].deflecting = qfalse;
	weaponDatabase[2].destroyable = qtrue;
	weaponDatabase[2].detonatable = qfalse;
	weaponDatabase[2].gravity = 0;
	weaponDatabase[2].hold = qfalse;
	weaponDatabase[2].homingAccel = 0;
	weaponDatabase[2].homingAngle = 0;
	weaponDatabase[2].homingRange = 0;
	weaponDatabase[2].homingType = HOM_NONE;
	weaponDatabase[2].hpCost = 0;
	weaponDatabase[2].kiCost = 0;
	weaponDatabase[2].knockback = 500;
	weaponDatabase[2].lifetime = 10000;
	weaponDatabase[2].lowPLBound = 0;
	weaponDatabase[2].max_speed = 600;
	weaponDatabase[2].MOD = UMOD_KI;
	weaponDatabase[2].multiShot = 1;
	weaponDatabase[2].offsetX = 0;
	weaponDatabase[2].offsetY = 0;
	weaponDatabase[2].overrideDir[0] = 0;
	weaponDatabase[2].overrideDir[1] = 0;
	weaponDatabase[2].overrideDir[2] = 0;
	weaponDatabase[2].piercing = qfalse;
	weaponDatabase[2].radius = 100;
	weaponDatabase[2].range = 0;
	weaponDatabase[2].spawnAmnt = 0;
	weaponDatabase[2].spawnNr = -1;
	weaponDatabase[2].speed = 600;
	weaponDatabase[2].splRadius = 200;
	weaponDatabase[2].stamCost = 0;
	weaponDatabase[2].struggling = qfalse;
	weaponDatabase[2].transKi2Hp = qfalse;
	weaponDatabase[2].uppPLBound = 100;
	weaponDatabase[2].useDecay = qfalse;
	weaponDatabase[2].vortex = qfalse;
	weaponDatabase[2].weapType = WPT_MISSILE;


	// --< BIG BANG ATTACK (BEAM) >--

	weaponDatabase[3].accel = 0;
	weaponDatabase[3].altFireNr = -1;
	weaponDatabase[3].autoFire = qfalse;
	weaponDatabase[3].blinding = qfalse;
	weaponDatabase[3].blockable = qtrue;
	weaponDatabase[3].bounceAmt = 0;
	weaponDatabase[3].bounceDmPct = 0;
	weaponDatabase[3].bounceVPct = 0;
	weaponDatabase[3].charge = qtrue;
	weaponDatabase[3].chargeRatio  = 3;
	weaponDatabase[3].chargeReady = 0;
	weaponDatabase[3].chargeTime = 325;
	weaponDatabase[3].chrgDamMod = 2;
	weaponDatabase[3].chrgRadMod = 1;
	weaponDatabase[3].chrgSplMod = 1;
	weaponDatabase[3].coneAngleX = 0;
	weaponDatabase[3].coneAngleY = 0;
	weaponDatabase[3].contExplsn = qfalse;
	weaponDatabase[3].coolTime = 1500;
	weaponDatabase[3].damage = 50;
	weaponDatabase[3].damageSelf = qfalse;
	weaponDatabase[3].decayRate = 0;
	weaponDatabase[3].deflecting = qfalse;
	weaponDatabase[3].destroyable = qtrue;
	weaponDatabase[3].detonatable = qfalse;
	weaponDatabase[3].gravity = 0;
	weaponDatabase[3].hold = qfalse;
	weaponDatabase[3].homingAccel = 0;
	weaponDatabase[3].homingAngle = 0;
	weaponDatabase[3].homingRange = 0;
	weaponDatabase[3].homingType = HOM_GUIDED;
	weaponDatabase[3].hpCost = 0;
	weaponDatabase[3].kiCost = 0;
	weaponDatabase[3].knockback = 500;
	weaponDatabase[3].lifetime = 10000;
	weaponDatabase[3].lowPLBound = 0;
	weaponDatabase[3].max_speed = 400;
	weaponDatabase[3].MOD = UMOD_KI;
	weaponDatabase[3].multiShot = 1;
	weaponDatabase[3].offsetX = 0;
	weaponDatabase[3].offsetY = 0;
	weaponDatabase[3].overrideDir[0] = 0;
	weaponDatabase[3].overrideDir[1] = 0;
	weaponDatabase[3].overrideDir[2] = 0;
	weaponDatabase[3].piercing = qfalse;
	weaponDatabase[3].radius = 100;
	weaponDatabase[3].range = 0;
	weaponDatabase[3].spawnAmnt = 0;
	weaponDatabase[3].spawnNr = -1;
	weaponDatabase[3].speed = 150;
	weaponDatabase[3].splRadius = 200;
	weaponDatabase[3].stamCost = 0;
	weaponDatabase[3].struggling = qfalse;
	weaponDatabase[3].transKi2Hp = qfalse;
	weaponDatabase[3].uppPLBound = 100;
	weaponDatabase[3].useDecay = qfalse;
	weaponDatabase[3].vortex = qfalse;
	weaponDatabase[3].weapType = WPT_MISSILE;


	// --< FINAL FLASH >--

	weaponDatabase[4].accel = 0;
	weaponDatabase[4].altFireNr = -1;
	weaponDatabase[4].autoFire = qfalse;
	weaponDatabase[4].blinding = qfalse;
	weaponDatabase[4].blockable = qfalse;
	weaponDatabase[4].bounceAmt = 0;
	weaponDatabase[4].bounceDmPct = 0;
	weaponDatabase[4].bounceVPct = 0;
	weaponDatabase[4].charge = qtrue;
	weaponDatabase[4].chargeRatio  = 3;
	weaponDatabase[4].chargeReady = 0;
	weaponDatabase[4].chargeTime = 325;
	weaponDatabase[4].chrgDamMod = 2;
	weaponDatabase[4].chrgRadMod = 1;
	weaponDatabase[4].chrgSplMod = 1;
	weaponDatabase[4].coneAngleX = 0;
	weaponDatabase[4].coneAngleY = 0;
	weaponDatabase[4].contExplsn = qtrue;
	weaponDatabase[4].coolTime = 1500;
	weaponDatabase[4].damage = 50;
	weaponDatabase[4].damageSelf = qfalse;
	weaponDatabase[4].decayRate = 0;
	weaponDatabase[4].deflecting = qfalse;
	weaponDatabase[4].destroyable = qtrue;
	weaponDatabase[4].detonatable = qfalse;
	weaponDatabase[4].gravity = 0;
	weaponDatabase[4].hold = qfalse;
	weaponDatabase[4].homingAccel = 0;
	weaponDatabase[4].homingAngle = 0;
	weaponDatabase[4].homingRange = 0;
	weaponDatabase[4].homingType = HOM_NONE;
	weaponDatabase[4].hpCost = 0;
	weaponDatabase[4].kiCost = 0;
	weaponDatabase[4].knockback = 500;
	weaponDatabase[4].lifetime = 10000;
	weaponDatabase[4].lowPLBound = 0;
	weaponDatabase[4].max_speed = 400;
	weaponDatabase[4].MOD = UMOD_KI;
	weaponDatabase[4].multiShot = 1;
	weaponDatabase[4].offsetX = 0;
	weaponDatabase[4].offsetY = 0;
	weaponDatabase[4].overrideDir[0] = 0;
	weaponDatabase[4].overrideDir[1] = 0;
	weaponDatabase[4].overrideDir[2] = 0;
	weaponDatabase[4].piercing = qfalse;
	weaponDatabase[4].radius = 100;
	weaponDatabase[4].range = 0;
	weaponDatabase[4].spawnAmnt = 0;
	weaponDatabase[4].spawnNr = -1;
	weaponDatabase[4].speed = 150;
	weaponDatabase[4].splRadius = 200;
	weaponDatabase[4].stamCost = 0;
	weaponDatabase[4].struggling = qtrue;
	weaponDatabase[4].transKi2Hp = qfalse;
	weaponDatabase[4].uppPLBound = 100;
	weaponDatabase[4].useDecay = qfalse;
	weaponDatabase[4].vortex = qfalse;
	weaponDatabase[4].weapType = WPT_BEAM;


	// --< SHYOGEKI HA >--

	weaponDatabase[5].accel = 0;
	weaponDatabase[5].altFireNr = -1;
	weaponDatabase[5].autoFire = qfalse;
	weaponDatabase[5].blinding = qfalse;
	weaponDatabase[5].blockable = qtrue;
	weaponDatabase[5].bounceAmt = 0;
	weaponDatabase[5].bounceDmPct = 0;
	weaponDatabase[5].bounceVPct = 0;
	weaponDatabase[5].charge = qfalse;
	weaponDatabase[5].chargeRatio  = 0;
	weaponDatabase[5].chargeReady = 0;
	weaponDatabase[5].chargeTime = 0;
	weaponDatabase[5].chrgDamMod = 0;
	weaponDatabase[5].chrgRadMod = 0;
	weaponDatabase[5].chrgSplMod = 0;
	weaponDatabase[5].coneAngleX = 10;
	weaponDatabase[5].coneAngleY = 10;
	weaponDatabase[5].contExplsn = qfalse;
	weaponDatabase[5].coolTime = 300;
	weaponDatabase[5].damage = 15;
	weaponDatabase[5].damageSelf = qfalse;
	weaponDatabase[5].decayRate = 0;
	weaponDatabase[5].deflecting = qfalse;
	weaponDatabase[5].destroyable = qtrue;
	weaponDatabase[5].detonatable = qfalse;
	weaponDatabase[5].gravity = 0;
	weaponDatabase[5].hold = qfalse;
	weaponDatabase[5].homingAccel = 0;
	weaponDatabase[5].homingAngle = 0;
	weaponDatabase[5].homingRange = 0;
	weaponDatabase[5].homingType = HOM_NONE;
	weaponDatabase[5].hpCost = 0;
	weaponDatabase[5].kiCost = 2;
	weaponDatabase[5].knockback = 500;
	weaponDatabase[5].lifetime = 0;
	weaponDatabase[5].lowPLBound = 0;
	weaponDatabase[5].max_speed = 0;
	weaponDatabase[5].MOD = UMOD_KI;
	weaponDatabase[5].multiShot = 1;
	weaponDatabase[5].offsetX = 0;
	weaponDatabase[5].offsetY = 0;
	weaponDatabase[5].overrideDir[0] = 0;
	weaponDatabase[5].overrideDir[1] = 0;
	weaponDatabase[5].overrideDir[2] = 0;
	weaponDatabase[5].piercing = qfalse;
	weaponDatabase[5].radius = 100;
	weaponDatabase[5].range = 500;
	weaponDatabase[5].spawnAmnt = 0;
	weaponDatabase[5].spawnNr = -1;
	weaponDatabase[5].speed = 0;
	weaponDatabase[5].splRadius = 200;
	weaponDatabase[5].stamCost = 0;
	weaponDatabase[5].struggling = qfalse;
	weaponDatabase[5].transKi2Hp = qfalse;
	weaponDatabase[5].uppPLBound = 100;
	weaponDatabase[5].useDecay = qfalse;
	weaponDatabase[5].vortex = qfalse;
	weaponDatabase[5].weapType = WPT_HITSCAN;


	// --< MAKANKOSAPPO >--

	weaponDatabase[6].accel = 0;
	weaponDatabase[6].altFireNr = -1;
	weaponDatabase[6].autoFire = qfalse;
	weaponDatabase[6].blinding = qfalse;
	weaponDatabase[6].blockable = qfalse;
	weaponDatabase[6].bounceAmt = 0;
	weaponDatabase[6].bounceDmPct = 0;
	weaponDatabase[6].bounceVPct = 0;
	weaponDatabase[6].charge = qtrue;
	weaponDatabase[6].chargeRatio  = 3;
	weaponDatabase[6].chargeReady = 0;
	weaponDatabase[6].chargeTime = 325;
	weaponDatabase[6].chrgDamMod = 2;
	weaponDatabase[6].chrgRadMod = 1;
	weaponDatabase[6].chrgSplMod = 1;
	weaponDatabase[6].coneAngleX = 0;
	weaponDatabase[6].coneAngleY = 0;
	weaponDatabase[6].contExplsn = qtrue;
	weaponDatabase[6].coolTime = 1500;
	weaponDatabase[6].damage = 50;
	weaponDatabase[6].damageSelf = qfalse;
	weaponDatabase[6].decayRate = 0;
	weaponDatabase[6].deflecting = qfalse;
	weaponDatabase[6].destroyable = qtrue;
	weaponDatabase[6].detonatable = qfalse;
	weaponDatabase[6].gravity = 0;
	weaponDatabase[6].hold = qfalse;
	weaponDatabase[6].homingAccel = 0;
	weaponDatabase[6].homingAngle = 0;
	weaponDatabase[6].homingRange = 0;
	weaponDatabase[6].homingType = HOM_NONE;
	weaponDatabase[6].hpCost = 0;
	weaponDatabase[6].kiCost = 0;
	weaponDatabase[6].knockback = 500;
	weaponDatabase[6].lifetime = 10000;
	weaponDatabase[6].lowPLBound = 0;
	weaponDatabase[6].max_speed = 400;
	weaponDatabase[6].MOD = UMOD_KI;
	weaponDatabase[6].multiShot = 1;
	weaponDatabase[6].offsetX = 0;
	weaponDatabase[6].offsetY = 0;
	weaponDatabase[6].overrideDir[0] = 0;
	weaponDatabase[6].overrideDir[1] = 0;
	weaponDatabase[6].overrideDir[2] = 0;
	weaponDatabase[6].piercing = qfalse;
	weaponDatabase[6].radius = 100;
	weaponDatabase[6].range = 0;
	weaponDatabase[6].spawnAmnt = 0;
	weaponDatabase[6].spawnNr = -1;
	weaponDatabase[6].speed = 500;
	weaponDatabase[6].splRadius = 200;
	weaponDatabase[6].stamCost = 0;
	weaponDatabase[6].struggling = qtrue;
	weaponDatabase[6].transKi2Hp = qfalse;
	weaponDatabase[6].uppPLBound = 100;
	weaponDatabase[6].useDecay = qfalse;
	weaponDatabase[6].vortex = qfalse;
	weaponDatabase[6].weapType = WPT_BEAM;
}

