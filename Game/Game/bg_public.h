/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_public.h -- definitions shared by both the server game and client game modules

#define EARTHQUAKE_SYSTEM	1	// JUHOX
#define MAPLENSFLARES		1	// JUHOX

#define	DEFAULT_GRAVITY		3000
#define	GIB_HEALTH			-40
#define	ARMOR_PROTECTION	0.66

#define	MAX_ITEMS			256

#define	RANK_TIED_FLAG		0x4000

#define DEFAULT_SHOTGUN_SPREAD	700
#define DEFAULT_SHOTGUN_COUNT	11

#define	ITEM_RADIUS			15		// item sizes are needed for client side pickup detection

#define	LIGHTNING_RANGE		768

#define	SCORE_NOT_PRESENT	-9999	// for the CS_SCORES[12] when only one player is present

#define	VOTE_TIME			30000	// 30 seconds before vote times out

#define	MINS_Z				-24
#define	DEFAULT_VIEWHEIGHT	26
#define CROUCH_VIEWHEIGHT	12
#define	DEAD_VIEWHEIGHT		-16

#define RADAR_WARN	1
#define RADAR_BURST 2

typedef struct {
	int valid;
	int	team;
	int pl;
	int plMax;
	int properties;
	int clientNum;
	vec3_t pos;	
} radar_t;

//
// config strings are a general means of communicating variable length strings
// from the server to all connected clients.
//

// CS_SERVERINFO and CS_SYSTEMINFO are defined in q_shared.h
#define	CS_MUSIC				2
#define	CS_MESSAGE				3		// from the map worldspawn's message field
#define	CS_MOTD					4		// g_motd string for server message of the day
#define	CS_WARMUP				5		// server time when the match will be restarted
#define	CS_SCORES1				6
#define	CS_SCORES2				7
#define CS_VOTE_TIME			8
#define CS_VOTE_STRING			9
#define	CS_VOTE_YES				10
#define	CS_VOTE_NO				11

#define CS_TEAMVOTE_TIME		12
#define CS_TEAMVOTE_STRING		14
#define	CS_TEAMVOTE_YES			16
#define	CS_TEAMVOTE_NO			18

#define	CS_PRODUCT_VERSION		20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24

#define	CS_ITEMS				27		// string of 0's and 1's that tell which items are present
#define CS_CLIENTSREADY			29

#define	CS_MODELS				32
#define	CS_SOUNDS				(CS_MODELS+MAX_MODELS)
#define	CS_PLAYERS				(CS_SOUNDS+MAX_SOUNDS)
#define CS_LOCATIONS			(CS_PLAYERS+MAX_CLIENTS)
#define CS_PARTICLES			(CS_LOCATIONS+MAX_LOCATIONS) 

#define CS_MAX					(CS_PARTICLES+MAX_LOCATIONS)

#if (CS_MAX) > MAX_CONFIGSTRINGS
#error overflow: (CS_MAX) > MAX_CONFIGSTRINGS
#endif

typedef enum {
	GT_FFA,				// free for all
	GT_TOURNAMENT,		// one on one tournament
	GT_SINGLE_PLAYER,	// single player ffa
	GT_STRUGGLE,
	//-- team games go after this --

	GT_TEAM,			// team deathmatch
	GT_CTF,				// capture the flag
	GT_1FCTF,
	GT_OBELISK,
	GT_HARVESTER,
	GT_MAX_GAME_TYPE
} gametype_t;

// JUHOX: global definitions for map lens flares
#if MAPLENSFLARES
typedef enum {
	EM_none,
	EM_mlf		// map lens flares
} editMode_t;
#endif

/*
===================================================================================

PMOVE MODULE

The pmove code takes a player_state_t and a usercmd_t and generates a new player_state_t
and some other output data.  Used for local prediction on the client game and true
movement on the server game.
===================================================================================
*/

typedef enum {
	PM_NORMAL,		// can accelerate and turn
	PM_NOCLIP,		// noclip movement
	PM_SPECTATOR,	// still run into walls
	PM_DEAD,		// no acceleration or turning, but free falling
	PM_FREEZE,		// stuck in place with no control
	PM_INTERMISSION,	// no movement or status bar
	PM_SPINTERMISSION	// no movement or status bar
} pmtype_t;

typedef enum {
	WEAPON_READY,
	WEAPON_FIRING, // ZEQ2: Will be used for continuous fire, regular firing goes immediately to WEAPON_COOLING.
	WEAPON_GUIDING,
	WEAPON_CHARGING,

	WEAPON_ALTFIRING,
	WEAPON_ALTGUIDING,
	WEAPON_ALTCHARGING,

	// NOTE: Must make these last to preserve enumeration in client entityState;
	//       These are remapped onto WEAPON_READY for entityState.
	WEAPON_COOLING,	
	WEAPON_RAISING,
	WEAPON_DROPPING
} weaponstate_t;


// pmove->pm_flags
#define	PMF_ATTACK1_HELD	1
#define	PMF_ATTACK2_HELD	2		// jump key held
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_FORWARDS_JUMP	32		// pm_time is time before rejump
#define	PMF_TIME_KNOCKBACK	64		// pm_time is an air-accelerate only time
#define	PMF_TIME_WATERJUMP	256		// pm_time is waterjump
#define	PMF_CAN_MOVE		512		// clear after attack and jump buttons come up
#define PMF_BOOST_HELD		1024	// boost key still held
#define PMF_ZANZOKEN		2048	// ZANZOKEN key still held
#define PMF_GRAPPLE_PULL	2048	// pull towards grapple location
#define PMF_FOLLOW			4096	// spectate following another player
#define PMF_SCOREBOARD		8192	// spectate as a scoreboa0rd
#define PMF_LOCK_HELD		16384	// 
#define PMF_BLOCK_HELD		32768	// Block, swat, push etc.

#define	MAXTOUCH	32
typedef struct {
	// state (in / out)
	playerState_t	*ps;
	// command (in)
	usercmd_t	cmd;
	int			tracemask;			// collide against these types of surfaces
	int			debugLevel;			// if set, diagnostic output will be printed
	qboolean	noFootsteps;		// if the game is setup for no footsteps by the server
	qboolean	gauntletHit;		// true if a gauntlet attack would actually hit something
	int			framecount;
	vec3_t		target;
	// results (out)
	int			numtouch;
	int			touchents[MAXTOUCH];
	vec3_t		mins, maxs;			// bounding box size
	int			watertype;
	int			waterlevel;
	float		xyspeed;
	float		xyzspeed;
	// for fixed msec Pmove
	int			pmove_fixed;
	int			pmove_msec;
	// callbacks to test the world
	// these will be different functions during game and cgame
	void		(*trace)( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask );
	int			(*pointcontents)( const vec3_t point, int passEntityNum );
} pmove_t;

// if a full pmove isn't done on the client, you can just update the angles
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd );
void Pmove (pmove_t *pmove);

//===================================================================================

typedef enum {
	stJumpTimed,
	stChargePercentPrimary,
	stChargePercentSecondary,
	stTransformState,
	stMeleeState,
	stAnimState,
	stSkills,
	stTransformFirstDuration,
	stTransformFirstEffectMaximum,
	stTransformFirstHealth,
	stTransformFirstFatigue,
	stTransformDuration,
	stTransformFatigue,
	stTransformHealth,
	stTransformEffectMaximum
}statIndex_t;
typedef enum{
	stJumpTimedPower,
	stSpeed,
	stAirBrakeCost,
	stMeleeAttack,
	stEnergyAttack,
	stKnockbackPower,
	stKnockbackIntensity,
	stZanzokenSpeed,
	stZanzokenDistance,
	stZanzokenCost,
	stZanzokenQuickDistance,
	stZanzokenQuickCost,
	stEnergyAttackCost,
	stBoostCost,
	stFatigueRecovery,
	stDefenseEnergy,
	stDefenseMelee,
	stDefenseCapacity,
	stDefenseRecovery,
	stDefenseRecoveryDelay,
	stTransformSubsequentDuration,
	stTransformSubsequentFatigueScale,
	stTransformSubsequentHealthScale,
	stTransformSubsequentMaximumScale,
}baseStats_t;
typedef enum{
	plCurrent,
	plFatigue,
	plHealth,
	plHealthPool,
	plMaximum,
	plMaximumPool,
	plLimit,
	plUseCurrent,
	plUseFatigue,
	plUseHealth,
	plUseMaximum,
	plDrainCurrent,
	plDrainMaximum,
	plDrainFatigue,
	plDrainHealth,
	plTierCurrent,
	plTierTotal,
	plDamageFromEnergy,
	plDamageFromMelee,
	plDamageGeneric,
	plBuffer,
	plTierDesired,
	plTierChanged,
	plTierSelectionMode //0 means tier select, 1 means previous tier, 2 means next tier
}powerLevel_t;
typedef enum{
	lkPowerCurrent,
	lkPowerHealth,
	lkPowerMaximum,
	lkAttackPower,
	lkLastLockedPlayer
}lockedStat_t;
typedef enum{
	tmUpdateTier,
	tmUpdateMelee,
	tmTransform,
	tmKnockback,
	tmZanzoken,
	tmBoost,
	tmJump,
	tmBlink,
	tmAttackLife,
	tmAttack1,
	tmAttack2,
	tmMeleeBreaker,
	tmMeleeBreakerWait,
	tmMeleeCharge,
	tmMeleeSpeed,
	tmMeleeIdle,
	tmStruggleEnergy,
	tmStruggleBlock,
	tmPowerRaise,
	tmPowerAuto,
	tmBurning,
	tmRiding,
	tmImpede,
	tmCrash,
	tmFreeze,
	tmRecover,
	tmBlind,
	tmSoar,
	tmFall,
	tmSafe,
	tmOnGround,
	tmLockon
}timers_t;
typedef enum{
	cdLockon
}cooldownTimers_t;
typedef enum{
	sqAttack,
	sqAltAttack,
	sqPowerLevel,
	sqBlock,
	sqWalk,
	sqBoost,
	sqLockon,
	sqJump,
	sqZanzoken,
	sqForward,
	sqLeft,
	sqRight,
	sqBack,
	sqUp,
	sqDown,
	sqRollRight,
	sqRollLeft
}sequenceTimers_t;
typedef enum {
	PW_NONE,
	PW_STATE,
	PW_SKILLS,
	PW_DRIFTING,
	PW_INVULNERABILITY,
	PW_KNOCKBACK_SPEED,
	PW_NUM_POWERUPS,
}powerup_t;
typedef enum {
	mtBoost,
	mtZanzoken,
	mtZanzokenDistance,
	mtDrifting,

}measureTimers_t;
typedef enum {
	bfZanzokenCost
}buffers_t;

// Player stat bits
#define hasFlipOffset		0x00000001
#define usingBoost			0x00000002
#define usingBlock			0x00000004
#define usingAlter			0x00000008
#define usingJump			0x00000010
#define usingZanzoken		0x00000020
#define usingWeapon			0x00000040
#define usingFlight			0x00000080
#define usingSoar			0x00000100
#define usingMelee			0x00000200
#define usingBallFlip		0x00000400
#define keyTierUp			0x00000800
#define keyTierDown			0x00001000
#define isTransforming		0x00002000
#define isStruggling		0x00004000
#define isUnconcious		0x00008000
#define isDead				0x00010000
#define isBreakingLimit		0x00020000
#define isCrashed			0x00040000
#define isGuiding			0x00080000
#define isCharging			0x00100000
#define isTargeted			0x00200000
#define isPreparing			0x00400000
#define isUnsafe			0x00800000
#define isBlinking			0x01000000
#define atopGround			0x02000000
#define nearGround			0x04000000
#define underWater			0x08000000
#define usingQuickZanzoken	0x10000000

// States
#define lockedPitch		0x00000001
#define lockedYaw		0x00000002
#define lockedRoll		0x00000004
#define locked360		0x00000008
#define lockedSpin		0x00000010
#define isRiding		0x00000020
#define isBurning		0x00000040
#define isHovering		0x00000080
#define isDashing		0x00000100
#define isTargetable	0x00000200
#define causedDamage	0x00000400

// Options / Capabilities
#define advancedMelee	0x00000001
#define advancedFlight	0x00000002
#define canAlter		0x00000004
#define canBlock		0x00000008
#define canMelee		0x00000010
#define canLockon		0x00000020
#define canHandspring	0x00000040
#define canGrab			0x00000080
#define canSlam			0x00000100
#define canClench		0x00000200
#define canExpulse		0x00000400
#define canDischarge	0x00000800
#define canBoost		0x00001000
#define canSwim			0x00002000
#define canFly			0x00004000
#define canJump			0x00008000
#define canZanzoken		0x00010000
#define canBallFlip		0x00020000
#define canOverheal		0x00040000
#define canBreakLimit	0x00080000
#define canTransform	0x00100000
#define canSoar			0x00200000
#define pointGravity	0x00400000

// Melee
typedef enum {
	stMeleeInactive,
	stMeleeAggressing,
	stMeleeDegressing,
	stMeleeIdle,
	stMeleeStartPower,
	stMeleeStartAttack,
	stMeleeStartDodge,
	stMeleeStartHit,
	stMeleeUsingSpeed,
	stMeleeUsingPower,
	stMeleeUsingStun,
	stMeleeUsingBlock,
	stMeleeUsingEvade,
	stMeleeUsingSpeedBreaker,
	stMeleeUsingChargeBreaker,
	stMeleeUsingZanzoken,
	stMeleeChargingPower,
	stMeleeChargingStun
} melee_t;


// player_state->persistant[] indexes
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_KILLED					// count of the number of times you died
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
#define	EF_FIRING			0x00000100		// for lightning gun
#define	EF_MOVER_STOP		0x00000400		// will push otherwise
#define	EF_TALK				0x00001000		// draw a talk balloon
#define	EF_CONNECTION		0x00002000		// draw a connection trouble sprite
#define	EF_VOTED			0x00004000		// already cast a vote
#define EF_TEAMVOTED		0x00080000		// already cast a team vote
#define EF_GUIDED			0x00004000		// To distinguish a guided missile (Missiles can't vote anyway)
#define EF_AURA				0x00000010		// used to make players display their aura

typedef enum {
	WP_NONE,
	WP_GAUNTLET,
	WP_MACHINEGUN,
	WP_SHOTGUN,
	WP_GRENADE_LAUNCHER,
	WP_ROCKET_LAUNCHER,
	WP_LIGHTNING,
	WP_RAILGUN,
	WP_PLASMAGUN,
	WP_BFG,
	WP_GRAPPLING_HOOK,
	WP_NUM_WEAPONS
} weapon_t;


// entityState_t->event values
// entity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.

// two bits at the top of the entityState->event field
// will be incremented with each change in the event so
// that an identical event started twice in a row can
// be distinguished.  And off the value with ~EV_EVENT_BITS
// to retrieve the actual event number
#define	EV_EVENT_BIT1		0x00000100
#define	EV_EVENT_BIT2		0x00000200
#define	EV_EVENT_BITS		(EV_EVENT_BIT1|EV_EVENT_BIT2)

#define	EVENT_VALID_MSEC	300

typedef enum {
	EV_NONE,
	EV_FOOTSTEP,
	EV_FOOTSTEP_METAL,
	EV_FOOTSPLASH,
	EV_FOOTWADE,
	EV_SWIM,
	EV_STEP_4,
	EV_STEP_8,
	EV_STEP_12,
	EV_STEP_16,
	EV_FALL_SHORT,
	EV_FALL_MEDIUM,
	EV_FALL_FAR,
	EV_JUMP_PAD,			// boing sound at origin, jump sound on player
	EV_HIGHJUMP,
	EV_JUMP,
	EV_WATER_TOUCH,	// foot touches
	EV_WATER_LEAVE,	// foot leaves
	EV_WATER_UNDER,	// head touches
	EV_WATER_CLEAR,	// head leaves
	EV_WATER_SPLASH,	// something hits water fast
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	// ADDING FOR ZEQ2
	EV_CRASH,
	EV_NULL,
	EV_DETONATE_WEAPON,
	EV_ALTFIRE_WEAPON,
	EV_TIERUP_FIRST,
	EV_TIERUP,
	EV_TIERDOWN,
	EV_SYNCTIER,
	EV_ALTERUP_START,
	EV_ALTERDOWN_START,
	EV_POWERINGUP_START,
	EV_BOOST_START,
	EV_BALLFLIP,
	EV_MELEE_SPEED,
	EV_MELEE_MISS,
	EV_MELEE_KNOCKBACK,
	EV_MELEE_BREAKER,
	EV_MELEE_STUN,
	EV_MELEE_CHECK,
	EV_MELEE_KNOCKOUT,
	EV_ZANZOKEN_END,
	EV_ZANZOKEN_START,
	EV_POWER_STRUGGLE_START,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,
	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,
	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_MISSILE_MISS_AIR,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter
	EV_AIRBRAKE,
	EV_PAIN_LIGHT,
	EV_PAIN_MEDIUM,
	EV_PAIN_HEAVY,
	EV_DEATH,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_UNCONCIOUS,
	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_LOCKON_START,
	EV_LOCKON_RESTART,
	EV_LOCKON_END,
	EV_STUNNED,
	EV_BLOCK,
	EV_PUSH,
	EV_SWAT,
	EV_HOVER,
	EV_HOVER_FAST,
	EV_HOVER_LONG,
	// ADDING FOR ZEQ2
	EV_BEAM_FADE,
	EV_EARTHQUAKE
	// END ADDING
} entity_event_t;
typedef enum {
	// DEATH
	ANIM_DEATH_GROUND,
	ANIM_DEATH_AIR,
	ANIM_DEATH_AIR_LAND,
	// STUN
	ANIM_STUN,
	// RECOVER
	ANIM_FLOOR_RECOVER,
	// IDLE
	ANIM_IDLE,
	ANIM_IDLE_LOCKED,
	ANIM_TURN,
	// WALK
	ANIM_WALK,
	// RUN
	ANIM_RUN,
	// SWIM
	ANIM_SWIM_IDLE,
	ANIM_SWIM,
	// JUMP
	ANIM_JUMP_UP,
	ANIM_LAND_UP,
	ANIM_JUMP_FORWARD,
	ANIM_LAND_FORWARD,
	ANIM_JUMP_BACK,
	ANIM_LAND_BACK,
	// DASH
	ANIM_DASH_RIGHT,
	ANIM_DASH_LEFT,
	ANIM_DASH_FORWARD,
	ANIM_DASH_BACKWARD,
	// FLY
	ANIM_FLY_IDLE,
	ANIM_FLY_START,
	ANIM_FLY_FORWARD,
	ANIM_FLY_BACKWARD,
	ANIM_FLY_UP,
	ANIM_FLY_DOWN,
	// KI
	ANIM_KI_CHARGE,
	ANIM_PL_UP,
	ANIM_PL_DOWN,
	// TRANSFORM
	ANIM_TRANS_UP,
	ANIM_TRANS_BACK,
	// DEFENSE
	ANIM_STUNNED,
	ANIM_PUSH,
	ANIM_DEFLECT,
	ANIM_BLOCK,
	// SPEED MELEE ATTACKS
	ANIM_SPEED_MELEE_ATTACK,
	ANIM_SPEED_MELEE_DODGE,
	ANIM_SPEED_MELEE_BLOCK,
	ANIM_SPEED_MELEE_HIT,
	// BREAKER MELEE ATTACK
	ANIM_BREAKER_MELEE_ATTACK1,
	ANIM_BREAKER_MELEE_ATTACK2,
	ANIM_BREAKER_MELEE_ATTACK3,
	ANIM_BREAKER_MELEE_ATTACK4,
	ANIM_BREAKER_MELEE_ATTACK5,
	ANIM_BREAKER_MELEE_ATTACK6,
	ANIM_BREAKER_MELEE_HIT1,
	ANIM_BREAKER_MELEE_HIT2,
	ANIM_BREAKER_MELEE_HIT3,
	ANIM_BREAKER_MELEE_HIT4,
	ANIM_BREAKER_MELEE_HIT5,
	ANIM_BREAKER_MELEE_HIT6,
	// POWER MELEE ATTACKS
	ANIM_POWER_MELEE_1_CHARGE,
	ANIM_POWER_MELEE_2_CHARGE,
	ANIM_POWER_MELEE_3_CHARGE,
	ANIM_POWER_MELEE_4_CHARGE,
	ANIM_POWER_MELEE_5_CHARGE,
	ANIM_POWER_MELEE_6_CHARGE,
	ANIM_POWER_MELEE_1_HIT,
	ANIM_POWER_MELEE_2_HIT,
	ANIM_POWER_MELEE_3_HIT,
	ANIM_POWER_MELEE_4_HIT,
	ANIM_POWER_MELEE_5_HIT,
	ANIM_POWER_MELEE_6_HIT,
	// STUN MELEE
	ANIM_STUNNED_MELEE,
	// KNOCKBACK
	ANIM_KNOCKBACK,
	ANIM_KNOCKBACK_RECOVER_1,
	ANIM_KNOCKBACK_RECOVER_2,
	ANIM_KNOCKBACK_HIT_WALL,
// KI ATTACKS
	ANIM_KI_ATTACK1_PREPARE,
	ANIM_KI_ATTACK1_FIRE,
	ANIM_KI_ATTACK2_PREPARE,
	ANIM_KI_ATTACK2_FIRE,
	ANIM_KI_ATTACK3_PREPARE,
	ANIM_KI_ATTACK3_FIRE,
	ANIM_KI_ATTACK4_PREPARE,
	ANIM_KI_ATTACK4_FIRE,
	ANIM_KI_ATTACK5_PREPARE,
	ANIM_KI_ATTACK5_FIRE,
	ANIM_KI_ATTACK6_PREPARE,
	ANIM_KI_ATTACK6_FIRE,
	ANIM_KI_ATTACK1_ALT_PREPARE,
	ANIM_KI_ATTACK1_ALT_FIRE,
	ANIM_KI_ATTACK2_ALT_PREPARE,
	ANIM_KI_ATTACK2_ALT_FIRE,
	ANIM_KI_ATTACK3_ALT_PREPARE,
	ANIM_KI_ATTACK3_ALT_FIRE,
	ANIM_KI_ATTACK4_ALT_PREPARE,
	ANIM_KI_ATTACK4_ALT_FIRE,
	ANIM_KI_ATTACK5_ALT_PREPARE,
	ANIM_KI_ATTACK5_ALT_FIRE,
	ANIM_KI_ATTACK6_ALT_PREPARE,
	ANIM_KI_ATTACK6_ALT_FIRE,
	// END ADDING
	MAX_ANIMATIONS,
	ANIM_BACKWALK,
	ANIM_BACKRUN,
	MAX_TOTALANIMATIONS
} animNumber_t;


typedef struct animation_s {
	int		firstFrame;
	int		numFrames;
	int		loopFrames;			// 0 to numFrames
	int		frameLerp;			// msec between frames
	int		initialLerp;		// msec to get to first frame
	int		reversed;			// true if animation is reversed
	int		flipflop;			// true if animation should flipflop back to base
	int		continuous;			// true if the animation should not restart, even if
								// the toggle bit is set
} animation_t;


// flip the togglebit every time an animation
// changes so a restart of the same anim can be detected
// #define	ANIM_TOGGLEBIT		128
#define ANIM_TOGGLEBIT			32768 // HACK: Let's hope this works...

// Relative direction strike came from
#define LOCATION_NONE		0x00000000
#define LOCATION_LEFT		0x00000100
#define LOCATION_RIGHT		0x00000200
#define LOCATION_FRONT		0x00000400
#define LOCATION_BACK		0x00000800

typedef enum {
	TEAM_FREE,
	TEAM_RED,
	TEAM_BLUE,
	TEAM_SPECTATOR,
	TEAM_NUM_TEAMS
} team_t;
// Time between location updates
#define TEAM_LOCATION_UPDATE_TIME		1000

// How many players on the overlay
#define TEAM_MAXOVERLAY		32

//team task
typedef enum {
	TEAMTASK_NONE,
	TEAMTASK_OFFENSE, 
	TEAMTASK_DEFENSE,
	TEAMTASK_PATROL,
	TEAMTASK_FOLLOW,
	TEAMTASK_RETRIEVE,
	TEAMTASK_ESCORT,
	TEAMTASK_CAMP
} teamtask_t;
//---------------------------------------------------------

// g_dmflags->integer flags
#define	DF_NO_FALLING			8
#define DF_FIXED_FOV			16
#define	DF_NO_FOOTSTEPS			32

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE)

//
// entityState_t->eType
//
typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_GRAPPLE,				// grapple hooked on wall
	ET_TEAM,
	// <-- RiO: New entity types for ZEQ2
	ET_BEAMHEAD,			// a beam's beamhead
	ET_EXPLOSION,
	// -->
	ET_EVENTS				// any of the EV_* events can be added freestanding
							// by setting eType to ET_EVENTS + eventNum
							// this avoids having to set eFlags and eventNum
} entityType_t;
void	BG_EvaluateTrajectory( entityState_t *es, const trajectory_t *tr, int atTime, vec3_t result );
void	BG_EvaluateTrajectoryDelta( entityState_t *es, const trajectory_t *tr, int atTime, vec3_t result );
// <-- RiO
float	BG_EvaluateWeaponChargeLevel( charge_t *ch, int atTime );
// -->

void	BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps );

void	BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad );

void	BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s,qboolean snap );

int		BG_IntLoBits( const int i );
int		BG_IntHiBits( const int i );
int		BG_IntMergeBits( const int hi, const int lo );

#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192
