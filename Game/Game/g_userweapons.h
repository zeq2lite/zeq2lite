typedef enum{
	skillTypeMISSILE,	//	Default missile type; ki blasts, discs, etc.
	skillTypeBEAM,		//	Beam type
	skillTypeHITSCAN,	//	Rail- / lightninggun style hitscan attack
	skillTypeTRIGGER,	//  For redirecting an existing weapon.
	skillTypeNONE		//	Useful for self-healing, since it spawns nothing.
}userWeaponType_t;
typedef enum{
	HOM_NONE,		//	Attack doesn't home in any way.
	HOM_PROX,		//	As soon as a target enters homing range the attack detonates.
	HOM_GUIDED,		//	Attack is guided by the player. ( NOTE: Bots will use a special kind of homing. )
	HOM_REGULAR,	//	Attack can pickup targets within its range and vision.
	HOM_ARCH,		//	Attack will arch outwards and curve back inwards towards opponents registered location.
	HOM_DRUNKEN,	//	Attack will wobble around drunkenly
	HOM_CYLINDER	//	Attack can only pickup targets in a cylinder, extending downwards from
					//	a little below its current position. ( NOTE: A little below, to prevent
					//	turning into horizontal travel.)
					//	FIXME: Do we really need this special case for Kakusan Ha?
					//	NOTE:  We probably do...
}userWeaponHoming_t;
typedef struct{
	// General
	userWeaponType_t	general_type;				// Weapon's type
	int					general_bitflags;			// Bitflag settings for weapon (see above)

	// Reactions
	qboolean			isSwattable;

	// Homing
	userWeaponHoming_t	homing_type;				// homing AI for weapon (see above)
	int					homing_range;				// range within which the weapon scans for targets
	int					homing_FOV;					// FOV of the weapon's range of vision when scanning for targets	

	// Restrictions	
	int					restrict_movement;

	// Requirements
	int					require_maximum;
	int					require_minPowerLevel;
	int					require_maxPowerLevel;
	int					require_minHealth;
	int					require_maxHealth;
	int					require_minFatigue;
	int					require_maxFatigue;
	int					require_minTier;
	int					require_maxTier;
	int					require_minTotalTier;
	int					require_maxTotalTier;
	int					require_ground;
	int					require_flight;
	int					require_water;

	// Costs
	int					costs_powerLevel;
	int					costs_maximum;
	int					costs_health;
	int					costs_fatigue;
	int					costs_chargeTime;			// Time to charge 1% of the weapon
	int					costs_chargeReady;			// Percentage at which the weapon can be fired
	int					costs_cooldownTime;			// Time the weapon must rest after being fired, before it can be fired again.

	// Damage
	int					damage_damage;				// Amount of damage, done at the impact point.
	int					damage_impede;
	int					damage_radius;				// Splash radius of damage. Actual damage linearly decreases the further away you are from the impact point.
	int					damage_radiusDuration;
	int					damage_multiplier;			// Damage dealt = damage_damage + multiplier * charge level / 100.
	int					damage_radiusMultiplier;	// Actual radius = damage_radius + multiplier * charge level / 100.
	int					damage_extraKnockback;		// Additional knockback to add to an attack's knockback caused directly by damage

	// Physics
	float				physics_speed;				// Speed at which the missile initially travels.
	float				physics_acceleration;		// Acceleration the missile has.
	float				physics_gravity;			// Percentage of the map's gravity the attack is affected by.
	int					physics_radius;				// Spherical collision radius of the attack
	int					physics_radiusMultiplier;	// Actual radius = physics_radius * multiplier * charge level / 100.
	int					physics_range_min;			// Minimum range a hitscan attack has.
	int					physics_range_max;			// Maximum range a hitscan attack has.
	int					physics_lifetime;			// Maximum time the attack can exist before 'blowing up'.
	float				physics_bounceFrac;			// How much of the energy to pass on in a bounce. 0 == no bounce
	int					physics_maxBounces;			// How many times at max the missile can bounce.
	qboolean			physics_swat;				// If the attack can be swatted away regardless of it's power.
	qboolean			physics_drain;				// Does the attack's power drain during it's lifetime?
	qboolean			physics_blind;				// Are you vulnerable to your own attack's blinding ability?
	
	// Firing
	int					firing_angleW_min;			// Minimum width angle ( + and - ) a projectile can deviate from its firing path.
	int					firing_angleW_max;			// Maximum width angle ( + and - ) a projectile can deviate from its firing path.
	int					firing_angleH_min;			// Minimum height angle ( + and - ) a projectile can deviate from its firing path.
	int					firing_angleH_max;			// Maximum height angle ( + and - ) a projectile can deviate from its firing path.
	int					firing_offsetW_max;			// Maximum width offset a shot will deviate from the player's regular muzzle point.
	int					firing_offsetW_min;			// Minimum width offset a shot will deviate from the player's regular muzzle point.
	int					firing_offsetH_max;			// Maximum height offset a shot will deviate from the player's regular muzzle point.
	int					firing_offsetH_min;			// Minimum height offset a shot will deviate from the player's regular muzzle point.
	qboolean			firing_offsetWFlip;			// Flip the width offset every alternating shot. ( NOTE: For left-right-left-... weapons. )
	qboolean			firing_offsetHFlip;			// Flip the height offset every alternating shot. ( NOTE: For up-down-up-... weapons. )
	int					firing_nrShots;				// Nr of projectiles fired in one shot.
} g_userWeapon_t;

// For use in the physics parser
typedef g_userWeapon_t g_userWeaponParseBuffer_t; // <-- is just the same.

// function declarations for g_userweapons.c

g_userWeapon_t *G_FindUserWeaponData( int clientNum, int weaponNum );
g_userWeapon_t *G_FindUserWeaponSpawnData( int clientNum, int weaponNum );
g_userWeapon_t *G_FindUserAltWeaponData( int clientNum, int weaponNum );
g_userWeapon_t *G_FindUserAltWeaponSpawnData( int clientNum, int weaponNum );
void G_LinkUserWeaponData( playerState_t *ps );
int *G_FindUserWeaponMask( int clientNum );
