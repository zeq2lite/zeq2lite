#include "g_local.h"

static int					weaponPhysicsMasks[MAX_CLIENTS];
static g_userWeapon_t		weaponPhysicsDatabase[MAX_CLIENTS][ALTSPAWN_OFFSET + skMaximumSkillPairs];
							// NOTE: SPAWN_OFFSET is the upper bound because it is the first
							//       element after the alternate fires!

/*
======================
G_FindUserWeaponMask
======================
*/
int *G_FindUserWeaponMask( int clientNum ) {
	return &weaponPhysicsMasks[clientNum];
}

/*
======================
G_FindUserWeaponData
======================
*/
g_userWeapon_t *G_FindUserWeaponData( int clientNum, int weaponNum ) {
	return &weaponPhysicsDatabase[clientNum][weaponNum - 1];
}

/*
===========================
G_FindUserWeaponSpawnData
===========================
*/
g_userWeapon_t *G_FindUserWeaponSpawnData( int clientNum, int weaponNum ) {
	return &weaponPhysicsDatabase[clientNum][weaponNum - 1 + SPAWN_OFFSET];
}

/*
=========================
G_FindUserAltWeaponData
=========================
*/
g_userWeapon_t *G_FindUserAltWeaponData( int clientNum, int weaponNum ) {
	return &weaponPhysicsDatabase[clientNum][weaponNum - 1 + skAltOffset];
}

/*
==============================
G_FindUserAltWeaponSpawnData
==============================
*/
g_userWeapon_t *G_FindUserAltWeaponSpawnData( int clientNum, int weaponNum ) {
	return &weaponPhysicsDatabase[clientNum][weaponNum - 1 + ALTSPAWN_OFFSET];
}

/*======================
G_CheckSkills
======================*/
void G_CheckSkills(playerState_t *ps){
	int	weapon;
	weapon = 1;
	ps->powerups[PW_SKILLS] &= ~ USABLE_SKILL1;
	ps->powerups[PW_SKILLS] &= ~ USABLE_SKILL2;
	ps->powerups[PW_SKILLS] &= ~ USABLE_SKILL3;
	ps->powerups[PW_SKILLS] &= ~ USABLE_SKILL4;
	ps->powerups[PW_SKILLS] &= ~ USABLE_SKILL5;
	ps->powerups[PW_SKILLS] &= ~ USABLE_SKILL6;
	while(weapon < 7){
		qboolean usable;
		g_userWeapon_t	*weaponSettings;
		usable = qtrue;
		weaponSettings = G_FindUserWeaponData(ps->clientNum,weapon);
		// ========================= 
		// REQUIREMENTS 
		// ========================= 
		if(weaponSettings->require_maximum && ps->powerLevel[plMaximum] < weaponSettings->require_maximum){usable = qfalse;} 
		if(weaponSettings->require_minPowerLevel && ps->powerLevel[plCurrent] < weaponSettings->require_minPowerLevel){usable = qfalse;} 
		if(weaponSettings->require_maxPowerLevel && ps->powerLevel[plCurrent] > weaponSettings->require_maxPowerLevel){usable = qfalse;} 
		if(weaponSettings->require_minFatigue && ps->powerLevel[plFatigue] < weaponSettings->require_minFatigue){usable = qfalse;} 
		if(weaponSettings->require_maxFatigue && ps->powerLevel[plFatigue] > weaponSettings->require_maxFatigue){usable = qfalse;} 
		if(weaponSettings->require_minHealth && ps->powerLevel[plHealth] < weaponSettings->require_minHealth){usable = qfalse;} 
		if(weaponSettings->require_maxHealth && ps->powerLevel[plHealth] > weaponSettings->require_maxHealth){usable = qfalse;} 
		if(weaponSettings->require_minTier && ps->powerLevel[plTierCurrent] < weaponSettings->require_minTier-1){usable = qfalse;} 
		if(weaponSettings->require_maxTier && ps->powerLevel[plTierCurrent] > weaponSettings->require_maxTier-1){usable = qfalse;} 
		if(weaponSettings->require_minTotalTier && ps->powerLevel[plTierTotal] < weaponSettings->require_minTotalTier-1){usable = qfalse;} 
		if(weaponSettings->require_maxTotalTier && ps->powerLevel[plTierTotal] > weaponSettings->require_maxTotalTier-1){usable = qfalse;} 
		if(weaponSettings->require_ground && !(ps->bitFlags & atopGround)){usable = qfalse;} 
		if(weaponSettings->require_flight && !(ps->bitFlags & usingFlight)){usable = qfalse;} 
		if(weaponSettings->require_water && !(ps->bitFlags & underWater)){usable = qfalse;} 
		if(usable){ 
			if(weapon == 1){ps->powerups[PW_SKILLS] |= USABLE_SKILL1;}
			if(weapon == 2){ps->powerups[PW_SKILLS] |= USABLE_SKILL2;}
			if(weapon == 3){ps->powerups[PW_SKILLS] |= USABLE_SKILL3;}
			if(weapon == 4){ps->powerups[PW_SKILLS] |= USABLE_SKILL4;}
			if(weapon == 5){ps->powerups[PW_SKILLS] |= USABLE_SKILL5;}
			if(weapon == 6){ps->powerups[PW_SKILLS] |= USABLE_SKILL6;}
		}
		else if(ps->weapon == weapon){
			ps->weapon -= 1;
		}
		weapon += 1;
	}
}

/*======================
G_SyncSkills
======================*/
void G_SyncSkill(playerState_t *ps,int index,qboolean alternate){
	g_userWeapon_t *weaponSettings;
	int offset = alternate ? skAttributeOffset : 0;
	weaponSettings = alternate ? G_FindUserAltWeaponData(ps->clientNum,index) : G_FindUserWeaponData(ps->clientNum,index);
	ps->skills[index][offset+skCostCurrent] = weaponSettings->costs_powerLevel;
	ps->skills[index][offset+skCostMaximum] = weaponSettings->costs_maximum;
	ps->skills[index][offset+skCostHealth] = weaponSettings->costs_health;
	ps->skills[index][offset+skCostFatigue] = weaponSettings->costs_fatigue;
	ps->skills[index][offset+skFireTime] = weaponSettings->costs_fireTime;
	ps->skills[index][offset+skChargeTimeMinimum] = weaponSettings->costs_chargeTimeMinimum;
	ps->skills[index][offset+skCooldownLength] = weaponSettings->costs_cooldownTime;
	ps->skills[index][offset+skRestrictChargeMovement] = weaponSettings->restrict_movement;
	ps->skills[index][offset+skBitflags] = weaponSettings->general_bitflags;
	if(weaponSettings->general_type == WPT_TORCH){
		ps->skills[index][offset+skBitflags] |= skConstant;
		ps->skills[index][offset+skBitflags] &= ~skNeedsCharge;
	}
	if(weaponSettings->homing_type == HOM_GUIDED || weaponSettings->general_type == WPT_BEAM){
		ps->skills[index][offset+skBitflags] |= skGuided;
	}
}
void G_SyncAllSkills(playerState_t *ps){
	int index = 0;
	while(index < 6){
		G_SyncSkill(ps,index,qfalse);
		if(ps->skills[index][skBitflags] & skHasAlternate){
			G_SyncSkill(ps,index,qtrue);
		}		
		index += 1;
	}
}
