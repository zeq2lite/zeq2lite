// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_public.h -- definitions shared by both the server game and client game modules

// because games can change separately from the main system version, we need a
// second version that must match between game and cgame

#define	GAME_VERSION		"zeq2lite"

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

#define	CS_GAME_VERSION			20
#define	CS_LEVEL_START_TIME		21		// so the timer only shows the current level
#define	CS_INTERMISSION			22		// when 1, fraglimit/timelimit has been hit and intermission will start in a second or two
#define CS_FLAGSTATUS			23		// string indicating flag status in CTF
#define CS_SHADERSTATE			24
#define CS_BOTINFO				25

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

typedef enum { GENDER_MALE, GENDER_FEMALE, GENDER_NEUTER } gender_t;

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
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2		// jump key held
#define	PMF_BACKWARDS_JUMP	8		// go into backwards land
#define	PMF_BACKWARDS_RUN	16		// coast down to backwards run
#define	PMF_TIME_LAND		32		// pm_time is time before rejump
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

#define	PMF_ALL_TIMES	(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_KNOCKBACK)

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


// player_state->stats[] indexes
// NOTE: may not have more than 16
typedef enum {
	powerLevel,
	powerLevelTotal,
	powerLevelTimer,
	powerLevelTimerAuto,
	skills,					// 16 bit bitmask
	tierCurrent,
	tierTotal,
	chargePercentPrimary,	// % of primary attack charged
	chargePercentSecondary,	// % of secondary attack charged
	bitFlags,				// Set of bitflags for player
	target
} statIndex_t;


// ADDING FOR ZEQ2
// Player stat bits
#define hasFlipOffset	0x00000001
#define usingBoost		0x00000002
#define usingBlock		0x00000004
#define usingAlter		0x00000008
#define usingJump		0x00000010
#define usingZanzoken	0x00000020
#define usingWeapon		0x00000040
#define usingFlight		0x00000080
#define usingMelee		0x00000100
#define usingTap		0x00000200
#define keyTierUp		0x00000400
#define keyTierDown		0x00000800
#define keyTapLeft		0x00001000
#define keyTapForward	0x00002000
#define keyTapRight		0x00004000
#define keyTapBack		0x00008000
#define keyTapDown		0x00010000
#define keyTapUp		0x00020000
#define isTransforming	0x00040000
#define isStruggling	0x00080000
#define isUnconcious	0x00100000
#define isDead			0x00200000

// player_state->persistant[] indexes
// NOTE: may not have more than 16
typedef enum {
	PERS_SCORE,						// !!! MUST NOT CHANGE, SERVER AND GAME BOTH REFERENCE !!!
	PERS_HITS,						// total points damage inflicted so damage beeps can sound on change
	PERS_RANK,						// player rank or team rank
	PERS_TEAM,						// player team
	PERS_SPAWN_COUNT,				// incremented every respawn
	PERS_PLAYEREVENTS,				// 16 bits that can be flipped for events
	PERS_ATTACKER,					// clientnum of last damage inflicter
	PERS_ATTACKEE_ARMOR,			// powerLevel/armor of last person we attacked
	PERS_KILLED,					// count of the number of times you died
	// player awards tracking
	PERS_IMPRESSIVE_COUNT,			// two railgun hits in a row
	PERS_EXCELLENT_COUNT,			// two successive kills in a short amount of time
	PERS_DEFEND_COUNT,				// defend awards
	PERS_ASSIST_COUNT,				// assist awards
	PERS_GAUNTLET_FRAG_COUNT,		// kills with the gauntlet
	PERS_CAPTURES,					// captures
	powerLevelMaximum				// current powerLevel cap should be saved across spawns
} persEnum_t;


// entityState_t->eFlags
#define	EF_DEAD				0x00000001		// don't draw a foe marker over players with EF_DEAD
#define	EF_TELEPORT_BIT		0x00000004		// toggled every time the origin abruptly changes
#define	EF_AWARD_EXCELLENT	0x00000008		// draw an excellent sprite
#define EF_PLAYER_EVENT		0x00000010
#define	EF_BOUNCE			0x00000010		// for missiles
#define	EF_BOUNCE_HALF		0x00000020		// for missiles
#define	EF_AWARD_GAUNTLET	0x00000040		// draw a gauntlet sprite
#define	EF_NODRAW			0x00000080		// may have an event, but no model (unspawned items)
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
#define EF_GUIDED			0x00004000		// To distinguish a guided missile (Missiles can't vote anyway)
#define EF_AURA				0x00000010		// used to make players display their aura

// NOTE: may not have more than 16
typedef enum {
	PW_NONE,
	PW_BOOST,
	PW_INVULNERABILITY,
	PW_ZANZOKEN,
	PW_TRANSFORM,
	PW_MELEE1,
	PW_MELEE2,
	PW_MELEE_STATE,
	PW_DEFENSE,
	PW_ATTACK1,
	PW_ATTACK2,
	PW_STUN,
	PW_KNOCKBACK,
	PW_NUM_POWERUPS
}powerup_t;

typedef enum {
	HI_NONE,

	HI_TELEPORTER,
	HI_MEDKIT,
	HI_KAMIKAZE,
	HI_PORTAL,
	HI_INVULNERABILITY,

	HI_NUM_HOLDABLE
} holdable_t;


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
#ifdef MISSIONPACK
	WP_NAILGUN,
	WP_PROX_LAUNCHER,
	WP_CHAINGUN,
#endif

	WP_NUM_WEAPONS
} weapon_t;


// reward sounds (stored in ps->persistant[PERS_PLAYEREVENTS])
#define	PLAYEREVENT_DENIEDREWARD		0x0001
#define	PLAYEREVENT_GAUNTLETREWARD		0x0002
#define PLAYEREVENT_HOLYSHIT			0x0004

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
	EV_ITEM_PICKUP,			// normal item pickups are predictable
	EV_GLOBAL_ITEM_PICKUP,	// powerup / team sounds are broadcast to everyon
	EV_NOAMMO,
	EV_CHANGE_WEAPON,
	EV_FIRE_WEAPON,
	// ADDING FOR ZEQ2
	EV_DETONATE_WEAPON,
	EV_ALTFIRE_WEAPON,
	EV_TIERCHECK,
	EV_TIERUP,
	EV_TIERDOWN,
	EV_MELEE_SPEED,
	EV_MELEE_KNOCKBACK,
	EV_MELEE_STUN,
	EV_MELEE_CHECK,
	EV_ZANZOKEN_END,
	EV_ZANZOKEN_START,
	// END ADDING
	EV_USE_ITEM0,
	EV_USE_ITEM1,
	EV_USE_ITEM2,
	EV_USE_ITEM3,
	EV_USE_ITEM4,
	EV_USE_ITEM5,
	EV_USE_ITEM6,
	EV_USE_ITEM7,
	EV_USE_ITEM8,
	EV_USE_ITEM9,
	EV_USE_ITEM10,
	EV_USE_ITEM11,
	EV_USE_ITEM12,
	EV_USE_ITEM13,
	EV_USE_ITEM14,
	EV_USE_ITEM15,
	EV_ITEM_RESPAWN,
	EV_ITEM_POP,
	EV_PLAYER_TELEPORT_IN,
	EV_PLAYER_TELEPORT_OUT,
	EV_GRENADE_BOUNCE,		// eventParm will be the soundindex
	EV_GENERAL_SOUND,
	EV_GLOBAL_SOUND,		// no attenuation
	EV_GLOBAL_TEAM_SOUND,
	EV_BULLET_HIT_FLESH,
	EV_BULLET_HIT_WALL,
	EV_MISSILE_HIT,
	EV_MISSILE_MISS,
	EV_MISSILE_MISS_METAL,
	EV_MISSILE_MISS_AIR,
	EV_RAILTRAIL,
	EV_SHOTGUN,
	EV_BULLET,				// otherEntity is the shooter
	EV_PAIN,
	EV_DEATH,
	EV_DEATH1,
	EV_DEATH2,
	EV_DEATH3,
	EV_OBITUARY,
	EV_POWERUP_QUAD,
	EV_POWERUP_BATTLESUIT,
	EV_POWERUP_REGEN,
	EV_GIB_PLAYER,			// gib a previously living player
	EV_SCOREPLUM,			// score plum
	EV_DEBUG_LINE,
	EV_STOPLOOPINGSOUND,
	EV_LOCKON_START,
	EV_LOCKON_CHECK,
	EV_LOCKON_END,
	EV_LOCKON_START_YES,
	EV_LOCKON_START_NO,
	EV_LOCKON_START_FOLLOWME,
	EV_LOCKON_START_GETFLAG,
	EV_LOCKON_START_GUARDBASE,
	EV_LOCKON_START_PATROL,
	EV_STUNNED,
	EV_BLOCK,
	EV_PUSH,
	EV_SWAT,
	// ADDING FOR ZEQ2
	EV_BEAM_FADE,
	EV_EARTHQUAKE
	// END ADDING
} entity_event_t;
typedef enum {
	GTS_RED_CAPTURE,
	GTS_BLUE_CAPTURE,
	GTS_RED_RETURN,
	GTS_BLUE_RETURN,
	GTS_RED_TAKEN,
	GTS_BLUE_TAKEN,
	GTS_REDOBELISK_ATTACKED,
	GTS_BLUEOBELISK_ATTACKED,
	GTS_REDTEAM_SCORED,
	GTS_BLUETEAM_SCORED,
	GTS_REDTEAM_TOOK_LEAD,
	GTS_BLUETEAM_TOOK_LEAD,
	GTS_TEAMS_ARE_TIED,
	GTS_KAMIKAZE
} global_team_sound_t;
typedef enum {
// DEATH
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,
// IDLE
	HEAD_IDLE,
	HEAD_IDLE_LOCKED,
	TORSO_STAND,
	TORSO_STAND_LOCKED,
	TORSO_CROUCH,
	LEGS_IDLE,
	LEGS_IDLE_LOCKED,
	LEGS_IDLECR,
	LEGS_TURN,
// WALK
	HEAD_WALK,
	TORSO_WALKCR,
	TORSO_WALK,
	LEGS_WALKCR,
	LEGS_WALK,
// RUN
	HEAD_RUN,
	HEAD_BACK,
	TORSO_RUN,
	TORSO_BACK,
	LEGS_RUN,
	LEGS_BACK,
// SWIM
	HEAD_SWIM,
	TORSO_SWIM,
	LEGS_SWIM,
// JUMP
	HEAD_JUMP,
	HEAD_LAND,
	HEAD_JUMPB,
	HEAD_LANDB,
	TORSO_JUMP,
	TORSO_LAND,
	TORSO_JUMPB,
	TORSO_LANDB,
	LEGS_JUMP,
	LEGS_LAND,
	LEGS_JUMPB,
	LEGS_LANDB,
// DASH
	HEAD_DASH_RIGHT,
	HEAD_DASH_LEFT,
	HEAD_DASH_FORWARD,
	HEAD_DASH_BACKWARD,
	TORSO_DASH_RIGHT,
	TORSO_DASH_LEFT,
	TORSO_DASH_FORWARD,
	TORSO_DASH_BACKWARD,
	LEGS_DASH_RIGHT,
	LEGS_DASH_LEFT,
	LEGS_DASH_FORWARD,
	LEGS_DASH_BACKWARD,
// FLY
	HEAD_FLY_IDLE,
	HEAD_FLY_FORWARD,
	HEAD_FLY_BACKWARD,
	HEAD_FLY_UP,
	HEAD_FLY_DOWN,
	TORSO_FLY_IDLE,
	TORSO_FLY_FORWARD,
	TORSO_FLY_BACKWARD,
	TORSO_FLY_UP,
	TORSO_FLY_DOWN,
	LEGS_FLY_IDLE,
	LEGS_FLY_FORWARD,
	LEGS_FLY_BACKWARD,
	LEGS_FLY_UP,
	LEGS_FLY_DOWN,
// KI
	HEAD_KI_CHARGE,
	HEAD_PL_UP,
	HEAD_PL_DOWN,
	TORSO_KI_CHARGE,
	TORSO_PL_UP,
	TORSO_PL_DOWN,
	LEGS_KI_CHARGE,
	LEGS_PL_UP,
	LEGS_PL_DOWN,
// TRANSFORM
	TORSO_TRANS_UP,
	TORSO_TRANS_BACK,
	LEGS_TRANS_UP,
	LEGS_TRANS_BACK,
	HEAD_TRANS_UP,
	HEAD_TRANS_BACK,
// DEFENSE
	HEAD_STUNNED,
	HEAD_PUSH,
	HEAD_DEFLECT,
	HEAD_BLOCK,
	TORSO_STUNNED,
	TORSO_PUSH,
	TORSO_DEFLECT,
	TORSO_BLOCK,
	LEGS_STUNNED,
	LEGS_PUSH,
	LEGS_DEFLECT,
	LEGS_BLOCK,
// SPEED MELEE ATTACKS
	HEAD_SPEED_MELEE_ATTACK,
	HEAD_SPEED_MELEE_DODGE,
	HEAD_SPEED_MELEE_BLOCK,
	HEAD_SPEED_MELEE_HIT,
	TORSO_SPEED_MELEE_ATTACK,
	TORSO_SPEED_MELEE_DODGE,
	TORSO_SPEED_MELEE_BLOCK,
	TORSO_SPEED_MELEE_HIT,
	LEGS_SPEED_MELEE_ATTACK,
	LEGS_SPEED_MELEE_DODGE,
	LEGS_SPEED_MELEE_BLOCK,
	LEGS_SPEED_MELEE_HIT,
// POWER MELEE ATTACKS
	HEAD_POWER_MELEE_1_CHARGE,
	HEAD_POWER_MELEE_2_CHARGE,
	HEAD_POWER_MELEE_3_CHARGE,
	HEAD_POWER_MELEE_4_CHARGE,
	HEAD_POWER_MELEE_5_CHARGE,
	HEAD_POWER_MELEE_6_CHARGE,
	HEAD_POWER_MELEE_1_HIT,
	HEAD_POWER_MELEE_2_HIT,
	HEAD_POWER_MELEE_3_HIT,
	HEAD_POWER_MELEE_4_HIT,
	HEAD_POWER_MELEE_5_HIT,
	HEAD_POWER_MELEE_6_HIT,
	TORSO_POWER_MELEE_1_CHARGE,
	TORSO_POWER_MELEE_2_CHARGE,
	TORSO_POWER_MELEE_3_CHARGE,
	TORSO_POWER_MELEE_4_CHARGE,
	TORSO_POWER_MELEE_5_CHARGE,
	TORSO_POWER_MELEE_6_CHARGE,
	TORSO_POWER_MELEE_1_HIT,
	TORSO_POWER_MELEE_2_HIT,
	TORSO_POWER_MELEE_3_HIT,
	TORSO_POWER_MELEE_4_HIT,
	TORSO_POWER_MELEE_5_HIT,
	TORSO_POWER_MELEE_6_HIT,
	LEGS_POWER_MELEE_1_CHARGE,
	LEGS_POWER_MELEE_2_CHARGE,
	LEGS_POWER_MELEE_3_CHARGE,
	LEGS_POWER_MELEE_4_CHARGE,
	LEGS_POWER_MELEE_5_CHARGE,
	LEGS_POWER_MELEE_6_CHARGE,
	LEGS_POWER_MELEE_1_HIT,
	LEGS_POWER_MELEE_2_HIT,
	LEGS_POWER_MELEE_3_HIT,
	LEGS_POWER_MELEE_4_HIT,
	LEGS_POWER_MELEE_5_HIT,
	LEGS_POWER_MELEE_6_HIT,
// KNOCKBACK
	HEAD_KNOCKBACK,
	HEAD_KNOCKBACK_RECOVER_1,
	HEAD_KNOCKBACK_RECOVER_2,
	TORSO_KNOCKBACK,
	TORSO_KNOCKBACK_RECOVER_1,
	TORSO_KNOCKBACK_RECOVER_2,
	LEGS_KNOCKBACK,
	LEGS_KNOCKBACK_RECOVER_1,
	LEGS_KNOCKBACK_RECOVER_2,
// KI ATTACKS
	HEAD_KI_ATTACK1_PREPARE,
	HEAD_KI_ATTACK1_FIRE,
	HEAD_KI_ATTACK2_PREPARE,
	HEAD_KI_ATTACK2_FIRE,
	HEAD_KI_ATTACK3_PREPARE,
	HEAD_KI_ATTACK3_FIRE,
	HEAD_KI_ATTACK4_PREPARE,
	HEAD_KI_ATTACK4_FIRE,
	HEAD_KI_ATTACK5_PREPARE,
	HEAD_KI_ATTACK5_FIRE,
	HEAD_KI_ATTACK6_PREPARE,
	HEAD_KI_ATTACK6_FIRE,
	HEAD_KI_ATTACK1_ALT_PREPARE,
	HEAD_KI_ATTACK1_ALT_FIRE,
	HEAD_KI_ATTACK2_ALT_PREPARE,
	HEAD_KI_ATTACK2_ALT_FIRE,
	HEAD_KI_ATTACK3_ALT_PREPARE,
	HEAD_KI_ATTACK3_ALT_FIRE,
	HEAD_KI_ATTACK4_ALT_PREPARE,
	HEAD_KI_ATTACK4_ALT_FIRE,
	HEAD_KI_ATTACK5_ALT_PREPARE,
	HEAD_KI_ATTACK5_ALT_FIRE,
	HEAD_KI_ATTACK6_ALT_PREPARE,
	HEAD_KI_ATTACK6_ALT_FIRE,
	TORSO_KI_ATTACK1_PREPARE,
	TORSO_KI_ATTACK1_FIRE,
	TORSO_KI_ATTACK2_PREPARE,
	TORSO_KI_ATTACK2_FIRE,
	TORSO_KI_ATTACK3_PREPARE,
	TORSO_KI_ATTACK3_FIRE,
	TORSO_KI_ATTACK4_PREPARE,
	TORSO_KI_ATTACK4_FIRE,
	TORSO_KI_ATTACK5_PREPARE,
	TORSO_KI_ATTACK5_FIRE,
	TORSO_KI_ATTACK6_PREPARE,
	TORSO_KI_ATTACK6_FIRE,
	TORSO_KI_ATTACK1_ALT_PREPARE,
	TORSO_KI_ATTACK1_ALT_FIRE,
	TORSO_KI_ATTACK2_ALT_PREPARE,
	TORSO_KI_ATTACK2_ALT_FIRE,
	TORSO_KI_ATTACK3_ALT_PREPARE,
	TORSO_KI_ATTACK3_ALT_FIRE,
	TORSO_KI_ATTACK4_ALT_PREPARE,
	TORSO_KI_ATTACK4_ALT_FIRE,
	TORSO_KI_ATTACK5_ALT_PREPARE,
	TORSO_KI_ATTACK5_ALT_FIRE,
	TORSO_KI_ATTACK6_ALT_PREPARE,
	TORSO_KI_ATTACK6_ALT_FIRE,
	LEGS_KI_ATTACK1_PREPARE,
	LEGS_KI_ATTACK1_FIRE,
	LEGS_KI_ATTACK2_PREPARE,
	LEGS_KI_ATTACK2_FIRE,
	LEGS_KI_ATTACK3_PREPARE,
	LEGS_KI_ATTACK3_FIRE,
	LEGS_KI_ATTACK4_PREPARE,
	LEGS_KI_ATTACK4_FIRE,
	LEGS_KI_ATTACK5_PREPARE,
	LEGS_KI_ATTACK5_FIRE,
	LEGS_KI_ATTACK6_PREPARE,
	LEGS_KI_ATTACK6_FIRE,
	LEGS_KI_ATTACK1_ALT_PREPARE,
	LEGS_KI_ATTACK1_ALT_FIRE,
	LEGS_KI_ATTACK2_ALT_PREPARE,
	LEGS_KI_ATTACK2_ALT_FIRE,
	LEGS_KI_ATTACK3_ALT_PREPARE,
	LEGS_KI_ATTACK3_ALT_FIRE,
	LEGS_KI_ATTACK4_ALT_PREPARE,
	LEGS_KI_ATTACK4_ALT_FIRE,
	LEGS_KI_ATTACK5_ALT_PREPARE,
	LEGS_KI_ATTACK5_ALT_FIRE,
	LEGS_KI_ATTACK6_ALT_PREPARE,
	LEGS_KI_ATTACK6_ALT_FIRE,
	LEGS_AIR_KI_ATTACK1_PREPARE,
	LEGS_AIR_KI_ATTACK1_FIRE,
	LEGS_AIR_KI_ATTACK2_PREPARE,
	LEGS_AIR_KI_ATTACK2_FIRE,
	LEGS_AIR_KI_ATTACK3_PREPARE,
	LEGS_AIR_KI_ATTACK3_FIRE,
	LEGS_AIR_KI_ATTACK4_PREPARE,
	LEGS_AIR_KI_ATTACK4_FIRE,
	LEGS_AIR_KI_ATTACK5_PREPARE,
	LEGS_AIR_KI_ATTACK5_FIRE,
	LEGS_AIR_KI_ATTACK6_PREPARE,
	LEGS_AIR_KI_ATTACK6_FIRE,
	LEGS_AIR_KI_ATTACK1_ALT_PREPARE,
	LEGS_AIR_KI_ATTACK1_ALT_FIRE,
	LEGS_AIR_KI_ATTACK2_ALT_PREPARE,
	LEGS_AIR_KI_ATTACK2_ALT_FIRE,
	LEGS_AIR_KI_ATTACK3_ALT_PREPARE,
	LEGS_AIR_KI_ATTACK3_ALT_FIRE,
	LEGS_AIR_KI_ATTACK4_ALT_PREPARE,
	LEGS_AIR_KI_ATTACK4_ALT_FIRE,
	LEGS_AIR_KI_ATTACK5_ALT_PREPARE,
	LEGS_AIR_KI_ATTACK5_ALT_FIRE,
	LEGS_AIR_KI_ATTACK6_ALT_PREPARE,
	LEGS_AIR_KI_ATTACK6_ALT_FIRE,
	// END ADDING
	MAX_ANIMATIONS,
	LEGS_BACKCR,
	LEGS_BACKWALK,
	TORSO_BACKWALK,
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,
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
// means of death
typedef enum {
	MOD_UNKNOWN,
	MOD_SHOTGUN,
	MOD_GAUNTLET,
	MOD_MACHINEGUN,
	MOD_GRENADE,
	MOD_GRENADE_SPLASH,
	MOD_ROCKET,
	MOD_ROCKET_SPLASH,
	MOD_PLASMA,
	MOD_PLASMA_SPLASH,
	MOD_RAILGUN,
	MOD_LIGHTNING,
	MOD_BFG,
	MOD_BFG_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_GRAPPLE,
	// ADDING FOR ZEQ2
	// FIXME: Until all of the bot-chat is fixed as well, we have to keep the old ones around
	//        in combination with our own ones.
	MOD_KI,		//	Ki attack
	MOD_MELEE,	//	Punch / Kick
	MOD_SLICE,	//	Sliced in half by disc attack / melee weapon ( Trunks' sword )
	MOD_PIERCE, //  Pierced / stabbed ( Freeza beam, Ki no Tsurugi, etc. )
	MOD_STONE,	//	Turned to stone ( Dabura's spit )
	MOD_BURN,	//	Burned ( Dabura's firebreath, 4-star Dragon's attacks )
	MOD_CANDY	//	'Turn to chocolate!'
	// END ADDING
} meansOfDeath_t;


//---------------------------------------------------------

// gitem_t->type
typedef enum {
	IT_BAD,
	IT_WEAPON,				// EFX: rotate + upscale + minlight
	IT_AMMO,				// EFX: rotate
	IT_ARMOR,				// EFX: rotate + minlight
	IT_HEALTH,				// EFX: static external sphere + rotating internal
	IT_POWERUP,				// instant on, timer based
							// EFX: rotate + external ring that rotates
	IT_HOLDABLE,			// single use, holdable item
							// EFX: rotate + bob
	IT_PERSISTANT_POWERUP,
	IT_TEAM
} itemType_t;

#define MAX_ITEM_MODELS 4

typedef struct gitem_s {
	char		*classname;		// spawning name
	char		*pickup_sound;
	char		*world_model[MAX_ITEM_MODELS];

	char		*icon;
	char		*pickup_name;	// for printing on pickup

	int			quantity;		// for ammo how much, or duration of powerup
	itemType_t  giType;			// IT_* flags

	int			giTag;

	char		*precaches;		// string of all models and images this item will use
	char		*sounds;		// string of all sounds this item will use
} gitem_t;

// included in both the game dll and the client
extern	gitem_t	bg_itemlist[];
extern	int		bg_numItems;

gitem_t	*BG_FindItem( const char *pickupName );
gitem_t	*BG_FindItemForWeapon( weapon_t weapon );
gitem_t	*BG_FindItemForPowerup( powerup_t pw );
gitem_t	*BG_FindItemForHoldable( holdable_t pw );
#define	ITEM_INDEX(x) ((x)-bg_itemlist)

qboolean	BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps );


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
	ET_FORCEFIELD,
	ET_TORCH,
	ET_SKIMMER,
	ET_RIFT,
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

qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime );

#define ARENAS_PER_TIER		4
#define MAX_ARENAS			1024
#define	MAX_ARENAS_TEXT		8192

#define MAX_BOTS			1024
#define MAX_BOTS_TEXT		8192


// Kamikaze

// 1st shockwave times
#define KAMI_SHOCKWAVE_STARTTIME		0
#define KAMI_SHOCKWAVEFADE_STARTTIME	1500
#define KAMI_SHOCKWAVE_ENDTIME			2000
// explosion/implosion times
#define KAMI_EXPLODE_STARTTIME			250
#define KAMI_IMPLODE_STARTTIME			2000
#define KAMI_IMPLODE_ENDTIME			2250
// 2nd shockwave times
#define KAMI_SHOCKWAVE2_STARTTIME		2000
#define KAMI_SHOCKWAVE2FADE_STARTTIME	2500
#define KAMI_SHOCKWAVE2_ENDTIME			3000
// radius of the models without scaling
#define KAMI_SHOCKWAVEMODEL_RADIUS		88
#define KAMI_BOOMSPHEREMODEL_RADIUS		72
// maximum radius of the models during the effect
#define KAMI_SHOCKWAVE_MAXRADIUS		1320
#define KAMI_BOOMSPHERE_MAXRADIUS		720
#define KAMI_SHOCKWAVE2_MAXRADIUS		704

