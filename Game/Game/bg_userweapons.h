// Copyright (C) 2003-2004 RiO
//
// bg_userweapons.h -- Both games custom weapons database headers.


// Weapon amounts
#define skMaximumSkillPairs	32
#define skAttributeOffset	16
#define skAltOffset			skMaximumSkillPairs
#define	SPAWN_OFFSET		skAltOffset + skMaximumSkillPairs		// NOTE: Currently inactive
#define ALTSPAWN_OFFSET		SPAWN_OFFSET + skMaximumSkillPairs		// NOTE: Currently inactive

// Bit flags for weapon statistics

#define	skHasAlternate		0x00000001	// there is an alternate weapon available
#define skNeedsCharge		0x00000002	// determines whether or not the weapon needs to be charged before firing
#define skReady				0x00000004  // will be set by the server when the weapon is ready to be fired
#define skConstant			0x00000008	// determines whether or not the attack is continuous fire
#define skGuided			0x00000010  // determines whether or not the attack is supposed to be guided
#define skIsUsing			0x00000020
#define skIsCharging		0x00000040
#define skIsCooling			0x00000080
#define skIsGuiding			0x00000100


#define USABLE_SKILL1		0x00000001
#define USABLE_SKILL2		0x00000002
#define USABLE_SKILL3		0x00000004
#define USABLE_SKILL4		0x00000008
#define USABLE_SKILL5		0x00000010
#define USABLE_SKILL6		0x00000020

typedef enum {
	skCostCurrent,
	skCostHealth,
	skCostFatigue,
	skCostMaximum,
	skFireTime,
	skChargeTime,
	skChargeTimeMinimum,
	skChargeTimeMaximum,
	skCooldownLength,
	skRestrictChargeMovement,
	skRestrictUsingMovement,
	skFireTimer,
	skCooldownTimer,
	skBitflags,

	skAltCostCurrent,
	skAltCostHealth,
	skAltCostFatigue,
	skAltCostMaximum,
	skAltChargeTime,
	skAltFireTime,
	skAltChargeTimeMinimum,
	skAltChargeTimeMaximum,
	skAltCooldownLength,
	skAltRestrictChargeMovement,
	skAltRestrictUsingMovement,
	skAltFireTimer,
	skAltCooldownTimer,
	skAltBitflags
} skillAttributes_t;


