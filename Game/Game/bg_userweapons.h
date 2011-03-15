// Copyright (C) 2003-2004 RiO
//
// bg_userweapons.h -- Both games custom weapons database headers.


// Weapon amounts
#define MAX_PLAYERWEAPONS	6
#define ALTWEAPON_OFFSET	MAX_PLAYERWEAPONS
#define	SPAWN_OFFSET		ALTWEAPON_OFFSET + MAX_PLAYERWEAPONS	// NOTE: Currently inactive
#define ALTSPAWN_OFFSET		SPAWN_OFFSET + MAX_PLAYERWEAPONS		// NOTE: Currently inactive

// Bit flags for weapon statistics

#define	WPF_NOMOVEFIRE			0x00000001	// can not move while firing weapon 
#define WPF_NOMOVECHARGE		0x00000002	// can not move while charging weapon
#define	WPF_NOMOVECOOLDOWN		0x00000004	// can not move while weapon is in 'cooldown'
#define	WPF_ALTWEAPONPRESENT	0x00000008	// there is an alternate weapon available
#define	WPF_DUALUSE				0x00000010	// weapon and alternate weapon are usable simultaneously
#define WPF_NEEDSCHARGE			0x00000020	// determines whether or not the weapon needs to be charged before firing
#define WPF_READY				0x00000040  // will be set by the server when the weapon is ready to be fired
#define WPF_CONTINUOUS			0x00000080	// determines whether or not the attack is continuous fire
#define WPF_GUIDED				0x00000100  // determines whether or not the attack is supposed to be guided
#define WPF_GROUND				0x00000200
#define WPF_FLIGHT				0x00000400
#define WPF_WATER				0x00000800


// NOTE: References for numbers when adding future flags
/*
#define	EF_BLA				0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define EF_TEAMVOTED		0x00080000		// already cast a team vote
*/

#define USABLE_SKILL1		0x00000001
#define USABLE_SKILL2		0x00000002
#define USABLE_SKILL3		0x00000004
#define USABLE_SKILL4		0x00000008
#define USABLE_SKILL5		0x00000010
#define USABLE_SKILL6		0x00000020

typedef enum {
	WPSTAT_POWERLEVELCOST,
	WPSTAT_HEALTHCOST,
	WPSTAT_FATIGUECOST,
	WPSTAT_MAXIMUMCOST,
	WPSTAT_CHRGTIME,
	WPSTAT_COOLTIME,
	WPSTAT_RESTRICT_MOVEMENT,
	WPSTAT_BITFLAGS,

	WPSTAT_ALT_POWERLEVELCOST,
	WPSTAT_ALT_HEALTHCOST,
	WPSTAT_ALT_FATIGUECOST,
	WPSTAT_ALT_MAXIMUMCOST,
	WPSTAT_ALT_CHRGTIME,
	WPSTAT_ALT_COOLTIME,
	WPSTAT_ALT_RESTRICT_MOVEMENT,
	WPSTAT_ALT_BITFLAGS,

	WPSTAT_NUMCHECK
} bg_weaponInfos_t;


