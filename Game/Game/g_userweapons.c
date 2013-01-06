#include "g_local.h"

static int					weaponPhysicsMasks[MAX_CLIENTS];
static g_userWeapon_t		weaponPhysicsDatabase[MAX_CLIENTS][ALTSPAWN_OFFSET + MAX_PLAYERWEAPONS];
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
	return &weaponPhysicsDatabase[clientNum][weaponNum - 1 + ALTWEAPON_OFFSET];
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
		weapon += 1;
	}
}

/*
======================
G_LinkUserWeaponData
======================
NOTE: This needs to be called on the server by the EV_CHANGE_WEAPON event.
There is currently not a case statement included for it.
*/
void G_LinkUserWeaponData( playerState_t *ps ) {
	g_userWeapon_t	*weaponSettings;
	g_userWeapon_t	*alt_weaponSettings;
	// Prevent reading below bound of array pointed to by weaponSettings.
	if ( ps->weapon == WP_NONE ) {
		memset(ps->currentSkill, 0, sizeof(ps->currentSkill) );
		return;
	}

	// NOTE: This function will NEVER be able to be called with a weapon that
	//       was not defined, so the next statement is SAFE.
	weaponSettings = G_FindUserWeaponData( ps->clientNum, ps->weapon );
	ps->currentSkill[WPSTAT_NUMCHECK] = ps->weapon;
	ps->currentSkill[WPSTAT_POWER] = weaponSettings->damage_damage;
	ps->currentSkill[WPSTAT_POWERLEVELCOST] = weaponSettings->costs_powerLevel;
	ps->currentSkill[WPSTAT_MAXIMUMCOST] = weaponSettings->costs_maximum;
	ps->currentSkill[WPSTAT_HEALTHCOST] = weaponSettings->costs_health;
	ps->currentSkill[WPSTAT_FATIGUECOST] = weaponSettings->costs_fatigue;
	ps->currentSkill[WPSTAT_CHRGTIME] = weaponSettings->costs_chargeTime;
	ps->currentSkill[WPSTAT_CHRGREADY] = weaponSettings->costs_chargeReady;
	ps->currentSkill[WPSTAT_COOLTIME] = weaponSettings->costs_cooldownTime;
	ps->currentSkill[WPSTAT_RESTRICT_MOVEMENT] = weaponSettings->restrict_movement;
	ps->currentSkill[WPSTAT_BITFLAGS] = weaponSettings->general_bitflags;
	ps->currentSkill[WPSTAT_RESTRICT_MOVEMENT] = weaponSettings->restrict_movement;	
	// Set the ready state of a charged weapon
	if ( ( ps->stats[stChargePercentPrimary] >= weaponSettings->costs_chargeReady ) && ( ps->currentSkill[WPSTAT_BITFLAGS] & WPF_NEEDSCHARGE ) ) {
		ps->currentSkill[WPSTAT_BITFLAGS] |= WPF_READY;
	} else {
		ps->currentSkill[WPSTAT_BITFLAGS] &= ~WPF_READY;
	}
	// Guided trajectories and beams should use WEAPON_GUIDING state
	if ( weaponSettings->homing_type == HOM_GUIDED || weaponSettings->general_type == skillTypeBEAM ) {
		ps->currentSkill[WPSTAT_BITFLAGS] |= WPF_GUIDED;
	}

	// Only attempt to link altfire weapon when it exists.
	if (ps->currentSkill[WPSTAT_BITFLAGS] & WPF_ALTWEAPONPRESENT) {
		alt_weaponSettings = G_FindUserAltWeaponData( ps->clientNum, ps->weapon );
		ps->currentSkill[WPSTAT_ALT_POWER] = alt_weaponSettings->damage_damage;
		ps->currentSkill[WPSTAT_ALT_POWERLEVELCOST] = alt_weaponSettings->costs_powerLevel;
		ps->currentSkill[WPSTAT_ALT_MAXIMUMCOST] = alt_weaponSettings->costs_maximum;
		ps->currentSkill[WPSTAT_ALT_HEALTHCOST] = alt_weaponSettings->costs_health;
		ps->currentSkill[WPSTAT_ALT_FATIGUECOST] = alt_weaponSettings->costs_fatigue;
		ps->currentSkill[WPSTAT_ALT_CHRGTIME] = alt_weaponSettings->costs_chargeTime;
		ps->currentSkill[WPSTAT_ALT_CHRGREADY] = alt_weaponSettings->costs_chargeReady;
		ps->currentSkill[WPSTAT_ALT_COOLTIME] = alt_weaponSettings->costs_cooldownTime;
		ps->currentSkill[WPSTAT_ALT_RESTRICT_MOVEMENT] = alt_weaponSettings->restrict_movement;
		ps->currentSkill[WPSTAT_ALT_BITFLAGS] = alt_weaponSettings->general_bitflags;
		ps->currentSkill[WPSTAT_ALT_RESTRICT_MOVEMENT] = alt_weaponSettings->restrict_movement;
		// Guided trajectories and beams should use WEAPON_GUIDING state
		if ( alt_weaponSettings->homing_type == HOM_GUIDED || alt_weaponSettings->general_type == skillTypeBEAM ) {
			ps->currentSkill[WPSTAT_ALT_BITFLAGS] |= WPF_GUIDED;
		}

		// Set the ready state of a charged altfire weapon
		if ( ( ps->stats[stChargePercentSecondary] >= alt_weaponSettings->costs_chargeReady ) && ( ps->currentSkill[WPSTAT_ALT_BITFLAGS] & WPF_NEEDSCHARGE ) ) {
			ps->currentSkill[WPSTAT_ALT_BITFLAGS] |= WPF_READY;
		} else {
			ps->currentSkill[WPSTAT_ALT_BITFLAGS] &= ~WPF_READY;
		}
	}

}
