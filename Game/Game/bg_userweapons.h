// Copyright (C) 2003-2004 RiO
//
// bg_userweapons.h -- Both games custom weapons database headers.


// Weapon amounts
#define MAX_PLAYERWEAPONS	6
#define ALTWEAPON_OFFSET	MAX_PLAYERWEAPONS
#define	SPAWN_OFFSET		ALTWEAPON_OFFSET + MAX_PLAYERWEAPONS	// NOTE: Currently inactive
#define ALTSPAWN_OFFSET		SPAWN_OFFSET + MAX_PLAYERWEAPONS		// NOTE: Currently inactive

// Bit flags for weapon statistics

#define	WPF_ALTWEAPONPRESENT	0x00000001	// there is an alternate weapon available
#define WPF_NEEDSCHARGE			0x00000002	// determines whether or not the weapon needs to be charged before firing
#define WPF_READY				0x00000004  // will be set by the server when the weapon is ready to be fired
#define WPF_CONTINUOUS			0x00000008	// determines whether or not the attack is continuous fire
#define WPF_GUIDED				0x00000010  // determines whether or not the attack is supposed to be guided


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
	WPSTAT_POWER,
	WPSTAT_POWERLEVELCOST,
	WPSTAT_HEALTHCOST,
	WPSTAT_FATIGUECOST,
	WPSTAT_MAXIMUMCOST,
	WPSTAT_CHRGTIME,
	WPSTAT_CHRGREADY,
	WPSTAT_COOLTIME,
	WPSTAT_RESTRICT_MOVEMENT,
	WPSTAT_BITFLAGS,

	WPSTAT_ALT_POWER,
	WPSTAT_ALT_POWERLEVELCOST,
	WPSTAT_ALT_HEALTHCOST,
	WPSTAT_ALT_FATIGUECOST,
	WPSTAT_ALT_MAXIMUMCOST,
	WPSTAT_ALT_CHRGTIME,
	WPSTAT_ALT_CHRGREADY,
	WPSTAT_ALT_COOLTIME,
	WPSTAT_ALT_RESTRICT_MOVEMENT,
	WPSTAT_ALT_BITFLAGS,

	WPSTAT_NUMCHECK,
	WPSTAT_CHANGED
} bg_weaponInfos_t;


