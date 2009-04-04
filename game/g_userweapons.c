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
		memset(ps->ammo, 0, sizeof(ps->ammo) );
		return;
	}

	// NOTE: This function will NEVER be able to be called with a weapon that
	//       was not defined, so the next statement is SAFE.
	weaponSettings = G_FindUserWeaponData( ps->clientNum, ps->weapon );
	
	ps->ammo[WPSTAT_NUMCHECK] = ps->weapon;
	ps->ammo[WPSTAT_KICOST] = weaponSettings->costs_ki;
	ps->ammo[WPSTAT_HPCOST] = weaponSettings->costs_hp;
	ps->ammo[WPSTAT_STAMCOST] = weaponSettings->costs_stamina;
	ps->ammo[WPSTAT_CHRGTIME] = weaponSettings->costs_chargeTime;
	ps->ammo[WPSTAT_COOLTIME] = weaponSettings->costs_cooldownTime;
	ps->ammo[WPSTAT_BITFLAGS] = weaponSettings->general_bitflags;
	// Ensures this is set if the flag isn't taken into the general_bitflags field
	// during initialization, but is only present as costs_transKi2HP
	if (weaponSettings->costs_transKi2HP) {
		ps->ammo[WPSTAT_BITFLAGS] |= WPF_KI2HP;
	}

	// Set the ready state of a charged weapon
	if ( ( ps->stats[STAT_CHARGELVL_PRI] >= weaponSettings->costs_chargeReady ) && ( ps->ammo[WPSTAT_BITFLAGS] & WPF_NEEDSCHARGE ) ) {
		ps->ammo[WPSTAT_BITFLAGS] |= WPF_READY;
	} else {
		ps->ammo[WPSTAT_BITFLAGS] &= ~WPF_READY;
	}

	// Torch attacks are implicitly continuous and can't be charged.
	if ( weaponSettings->general_type == WPT_TORCH ) {
		ps->ammo[WPSTAT_BITFLAGS] |= WPF_CONTINUOUS;
		ps->ammo[WPSTAT_BITFLAGS] &= ~WPF_NEEDSCHARGE;
	}

	// Guided trajectories and beams should use WEAPON_GUIDING state
	if ( weaponSettings->homing_type == HOM_GUIDED || weaponSettings->general_type == WPT_BEAM ) {
		ps->ammo[WPSTAT_BITFLAGS] |= WPF_GUIDED;
	}

	// Only attempt to link altfire weapon when it exists.
	if (ps->ammo[WPSTAT_BITFLAGS] & WPF_ALTWEAPONPRESENT) {
		alt_weaponSettings = G_FindUserAltWeaponData( ps->clientNum, ps->weapon );
	
		ps->ammo[WPSTAT_ALT_KICOST] = alt_weaponSettings->costs_ki;
		ps->ammo[WPSTAT_ALT_HPCOST] = alt_weaponSettings->costs_hp;
		ps->ammo[WPSTAT_ALT_STAMCOST] = alt_weaponSettings->costs_stamina;
		ps->ammo[WPSTAT_ALT_CHRGTIME] = alt_weaponSettings->costs_chargeTime;
		ps->ammo[WPSTAT_ALT_COOLTIME] = alt_weaponSettings->costs_cooldownTime;
		ps->ammo[WPSTAT_ALT_BITFLAGS] = alt_weaponSettings->general_bitflags;
		// Ensures this is set if the flag isn't taken into the general_bitflags field
		// during initialization, but is only present as costs_transKi2HP
		if (alt_weaponSettings->costs_transKi2HP) {
			ps->ammo[WPSTAT_ALT_BITFLAGS] |= WPF_KI2HP;
		}

		// Torch attacks are implicitly continuous and can't be charged.
		if ( alt_weaponSettings->general_type == WPT_TORCH ) {
			ps->ammo[WPSTAT_ALT_BITFLAGS] |= WPF_CONTINUOUS;
			ps->ammo[WPSTAT_ALT_BITFLAGS] &= ~WPF_NEEDSCHARGE;
		}

		// Guided trajectories and beams should use WEAPON_GUIDING state
		if ( alt_weaponSettings->homing_type == HOM_GUIDED || alt_weaponSettings->general_type == WPT_BEAM ) {
			ps->ammo[WPSTAT_ALT_BITFLAGS] |= WPF_GUIDED;
		}

		// Set the ready state of a charged altfire weapon
		if ( ( ps->stats[STAT_CHARGELVL_SEC] >= alt_weaponSettings->costs_chargeReady ) && ( ps->ammo[WPSTAT_ALT_BITFLAGS] & WPF_NEEDSCHARGE ) ) {
			ps->ammo[WPSTAT_ALT_BITFLAGS] |= WPF_READY;
		} else {
			ps->ammo[WPSTAT_ALT_BITFLAGS] &= ~WPF_READY;
		}
	}

}
