#include "g_local.h"

static int					weaponPhysicsMasks[MAX_CLIENTS];
static g_userWeapon_t		weaponPhysicsDatabase[MAX_CLIENTS][ALTSPAWN_OFFSET + MAX_PLAYERWEAPONS];
							// NOTE: SPAWN_OFFSET is the upper bound because it is the first
							//       element after the alternate fires!


int G_FindUserWeaponMask( int clientNum ) {
	return weaponPhysicsMasks[clientNum];
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
		ps->ammo[WPSTAT_BITFLAGS] = ps->ammo[WPSTAT_BITFLAGS] | WPF_READY;
	} else {
		ps->ammo[WPSTAT_BITFLAGS] = ps->ammo[WPSTAT_BITFLAGS] & ~WPF_READY;
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


		// Set the ready state of a charged altfire weapon
		if ( ( ps->stats[STAT_CHARGELVL_SEC] >= alt_weaponSettings->costs_chargeReady ) && ( ps->ammo[WPSTAT_ALT_BITFLAGS] & WPF_NEEDSCHARGE ) ) {
			ps->ammo[WPSTAT_ALT_BITFLAGS] = ps->ammo[WPSTAT_ALT_BITFLAGS] | WPF_READY;
		} else {
			ps->ammo[WPSTAT_ALT_BITFLAGS] = ps->ammo[WPSTAT_ALT_BITFLAGS] & ~WPF_READY;
		}
	}

}


/*
======================
G_InitUserWeaponData
====================== 
*/
void G_InitUserWeaponData( void ) {
	int i,j,k;

	memset( weaponPhysicsDatabase, 0, sizeof( weaponPhysicsDatabase ) );

	for (i = 0; i < MAX_CLIENTS; i++) {

		// TEST
		// Try out the parser functions
		if ( G_weapPhysParser_MainParse( "weap_freeza.phys", i, &weaponPhysicsMasks[i] ) ) {
			continue;
		}

		weaponPhysicsMasks[i] = 0;
		weaponPhysicsMasks[i] |= ( 1 << 1 );
		weaponPhysicsMasks[i] |= ( 1 << 2 );
		weaponPhysicsMasks[i] |= ( 1 << 3 );
		weaponPhysicsMasks[i] |= ( 1 << 4 );
		weaponPhysicsMasks[i] |= ( 1 << 5 );
		weaponPhysicsMasks[i] |= ( 1 << 6 );

		// END TEST
		
		// --< 1: Kikou >--
		k = 0;
		j = k;
		
		weaponPhysicsDatabase[i][j].general_type = WPT_MISSILE;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_NEEDSCHARGE | WPF_ALTWEAPONPRESENT;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 1;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1000;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 20;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 5;
		
		weaponPhysicsDatabase[i][j].damage_damage = 20;
		weaponPhysicsDatabase[i][j].damage_multiplier = 20;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 200;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 400;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 500;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		//weaponPhysicsDatabase[i][j].physics_bounceFrac = 1.0f;
		//weaponPhysicsDatabase[i][j].physics_maxBounces = 2;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 100;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;



		// --< 1, Altfire: Renzoku Energy Dan >--
		j = k + ALTWEAPON_OFFSET;
		
		weaponPhysicsDatabase[i][j].general_type = WPT_MISSILE;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_ALTWEAPONPRESENT;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 25;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qtrue;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 400;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 0;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 0;
		
		weaponPhysicsDatabase[i][j].damage_damage = 15;
		weaponPhysicsDatabase[i][j].damage_multiplier = 0;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 300;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 12;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qtrue;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 650;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;



		// --< 2: Shyogeki Ha >--
		k++;
		j = k;

		weaponPhysicsDatabase[i][j].general_type = WPT_HITSCAN;
		weaponPhysicsDatabase[i][j].general_bitflags = 0;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 20;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 300;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 0;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 0;
		
		weaponPhysicsDatabase[i][j].damage_damage = 15;
		weaponPhysicsDatabase[i][j].damage_multiplier = 0;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 400;

		weaponPhysicsDatabase[i][j].firing_deviateW = 10;
		weaponPhysicsDatabase[i][j].firing_deviateH = 10;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 650;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 500; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;



		// --< 3: Big Bang Attack  ( Ball ) >--
		// NOTE: Now --< 3: Freeza Beam >--
		k++;
		j = k;

		weaponPhysicsDatabase[i][j].general_type = WPT_HITSCAN;
		weaponPhysicsDatabase[i][j].general_bitflags = 0;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 20;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1500;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 0;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 0;
		
		weaponPhysicsDatabase[i][j].damage_damage = 30;
		weaponPhysicsDatabase[i][j].damage_multiplier = 0;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_PIERCE;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 0;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 650;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 5000; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;

		/*
		weaponPhysicsDatabase[i][j].general_type = WPT_MISSILE;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_NEEDSCHARGE | WPF_ALTWEAPONPRESENT;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 5;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1500;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 125;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 30;
		
		weaponPhysicsDatabase[i][j].damage_damage = 40;
		weaponPhysicsDatabase[i][j].damage_multiplier = 40;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 650;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;



		// --< 3, Altfire: Big Bang Attack  ( Beam ) >--
		j = k + ALTWEAPON_OFFSET;

		weaponPhysicsDatabase[i][j].general_type = WPT_BEAM;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_NEEDSCHARGE;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 5;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1500;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 160;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 30;
		
		weaponPhysicsDatabase[i][j].damage_damage = 50;
		weaponPhysicsDatabase[i][j].damage_multiplier = 50;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_GUIDED;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 400;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;
		*/

		// --< 4: Daichiretsuzan >--
		k++;
		j = k;
		
		weaponPhysicsDatabase[i][j].general_type = WPT_GROUNDSKIM;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_NEEDSCHARGE;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 4;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1000;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 100;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 50;
		
		weaponPhysicsDatabase[i][j].damage_damage = 50;
		weaponPhysicsDatabase[i][j].damage_multiplier = 20;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 200;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 600;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 500;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 100;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;


		// --< 5: Final Flash >--
		// NOTE: Now turned into --< 5: Deathball >--
		k++;
		j = k;

		weaponPhysicsDatabase[i][j].general_type = WPT_MISSILE; // WPT_BEAM;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_NEEDSCHARGE;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 7;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1500;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 200;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 40;
		
		weaponPhysicsDatabase[i][j].damage_damage = 70;
		weaponPhysicsDatabase[i][j].damage_multiplier = 30;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_KI;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 1000;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 400;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;



		// --< 6: Makankosappo >--
		// NOTE: Now turned into --< 6: Tsuibi Kienzan >--
		k++;
		j = k;

		weaponPhysicsDatabase[i][j].general_type = WPT_MISSILE; //WPT_BEAM;
		weaponPhysicsDatabase[i][j].general_bitflags = WPF_NEEDSCHARGE;

		weaponPhysicsDatabase[i][j].costs_hp = 0;
		weaponPhysicsDatabase[i][j].costs_ki = 3;
		weaponPhysicsDatabase[i][j].costs_stamina = 0;
		weaponPhysicsDatabase[i][j].costs_transKi2HP = qfalse;
		weaponPhysicsDatabase[i][j].costs_cooldownTime = 1500;
		weaponPhysicsDatabase[i][j].costs_chargeTime = 160;
		weaponPhysicsDatabase[i][j].costs_chargeReady = 20;
		
		weaponPhysicsDatabase[i][j].damage_damage = 200; // FIXME: Create 'lethal' flag?
		weaponPhysicsDatabase[i][j].damage_multiplier = 30;
		weaponPhysicsDatabase[i][j].damage_radius = 200;
		weaponPhysicsDatabase[i][j].damage_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].damage_continuous = qfalse;
		weaponPhysicsDatabase[i][j].damage_piercing = qfalse;
		weaponPhysicsDatabase[i][j].damage_meansOfDeath = UMOD_SLICE;
		weaponPhysicsDatabase[i][j].damage_extraKnockback = 0;

		weaponPhysicsDatabase[i][j].firing_deviateW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetW = 0;
		weaponPhysicsDatabase[i][j].firing_deviateH = 0;
		weaponPhysicsDatabase[i][j].firing_offsetWFlip = qfalse;
		weaponPhysicsDatabase[i][j].firing_nrShots = 1;

		weaponPhysicsDatabase[i][j].homing_type = HOM_GUIDED; //HOM_NONE;
		weaponPhysicsDatabase[i][j].homing_range = 0;
		weaponPhysicsDatabase[i][j].homing_angleW = 0;
		weaponPhysicsDatabase[i][j].homing_angleH = 0;
		
		weaponPhysicsDatabase[i][j].physics_speed = 300; //500;
		weaponPhysicsDatabase[i][j].physics_acceleration = 0;
		weaponPhysicsDatabase[i][j].physics_gravity = 0;
		weaponPhysicsDatabase[i][j].physics_radius = 100;
		weaponPhysicsDatabase[i][j].physics_radiusMultiplier = 0;
		weaponPhysicsDatabase[i][j].physics_range = 0; // ( Only important for hitscan type, until dissipation is coded )
		weaponPhysicsDatabase[i][j].physics_lifetime = 10000;
	}
}
