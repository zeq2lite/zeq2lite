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
#define WPF_KI2HP				0x00000080  // determines whether or not the attack is usable through hp, if no more ki is left
#define WPF_CONTINUOUS			0x00000100	// determines whether or not the attack is continuous fire
#define WPF_GUIDED				0x00000200  // determines whether or not the attack is supposed to be guided


// NOTE: References for numbers when adding future flags
/*
#define	EF_BLA				0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_KAMIKAZE			0x00000200
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define EF_AWARD_CAP		0x00000800		// draw the capture sprite
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define	EF_AWARD_IMPRESSIVE	0x00008000		// draw an impressive sprite
#define	EF_AWARD_DEFEND		0x00010000		// draw a defend sprite
#define	EF_AWARD_ASSIST		0x00020000		// draw a assist sprite
#define EF_AWARD_DENIED		0x00040000		// denied
#define EF_TEAMVOTED		0x00080000		// already cast a team vote
*/

typedef enum {
	WPSTAT_KICOST,
	WPSTAT_HPCOST,
	WPSTAT_STAMCOST,
	WPSTAT_CHRGTIME,
	WPSTAT_COOLTIME,
	WPbitFlags,

	WPSTAT_ALT_KICOST,
	WPSTAT_ALT_HPCOST,
	WPSTAT_ALT_STAMCOST,
	WPSTAT_ALT_CHRGTIME,
	WPSTAT_ALT_COOLTIME,
	WPSTAT_ALT_BITFLAGS,

	WPSTAT_NUMCHECK		// <--	This will be set to the actual weapon number for a clientside
						//		check whether the required data has arrived or not.

} bg_weaponInfos_t;


//NOTE: Don't really belong with 'weapons', but its ridiculous to start a new unit for it
#define MAX_STAMINA		500
#define MAX_KI			1000
#define MAX_JUMPING		8000

