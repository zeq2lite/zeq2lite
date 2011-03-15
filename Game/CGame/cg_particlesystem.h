// Copyright (C) 2003-2004 RiO
//
// cg_particlesystem.h -- particle system headers


#define MAX_PARTICLES			  1024
#define MAX_PARTICLESYSTEMS		   128
#define MAX_EMITTERS			   256
#define MAX_FORCES				   256
#define MAX_CONSTRAINTS			   256
#define MAX_PARTICLE_TEMPLATES	     3
#define MAX_PARTICLESYSTEM_MEMBERS   8

typedef enum {
	CTYPE_DISTANCE_MAX,
	CTYPE_DISTANCE_MIN,
	CTYPE_DISTANCE_EXACT,
	CTYPE_PLANE
} PSys_ConstraintType_t;

typedef enum {
	FTYPE_DIRECTIONAL,
	FTYPE_SPHERICAL,
	FTYPE_DRAG,
	FTYPE_SWIRL
} PSys_ForceType_t;

typedef enum {
	AOI_INFINITE,
	AOI_SPHERE,
	AOI_CYLINDER
} PSys_AOIType_t;

typedef enum {
	ETYPE_POINT,
	ETYPE_RADIUS,
	ETYPE_SPHERE,

	ETYPE_POINT_SURFACE, // NOTE: Must be first surface type!
	ETYPE_RADIUS_SURFACE,	
} PSys_EmitterType_t;

typedef enum {
	RTYPE_DEFAULT,		// Model or sprite, based on whether or not the model field is filled in.
	RTYPE_SPARK,		// Only uses shader field. Stretches from current position back to old position.
	RTYPE_RAY			// Only uses shader field. Shoots a ray from particle's origin to its current position.
} PSys_RenderType_t;

// Need these prototyped
typedef struct PSys_Particle_s;
typedef struct PSys_System_s;

typedef struct PSys_FloatTimeLerp_s {
	float	startVal;
	float	midVal;
	float	endVal;

	int		startTime;
	int		endTime;
} PSys_FloatTimeLerp_t;

typedef struct PSys_Vec4TimeLerp_s {
	vec4_t	startVal;
	vec4_t	midVal;
	vec4_t	endVal;

	int		startTime;
	int		endTime;
} PSys_Vec4TimeLerp_t;

typedef struct PSys_ParticleTemplate_s {
	float					mass;
	int						lifeTime;
	int						speed;

	PSys_RenderType_t		rType;
	qhandle_t				shader;
	qhandle_t				model;
	PSys_FloatTimeLerp_t	scale;
	PSys_Vec4TimeLerp_t		rotate;
	PSys_Vec4TimeLerp_t		rgba;
} PSys_ParticleTemplate_t;

typedef struct PSys_Orientation_s {
	orientation_t	geometry;

	// If we want to link the orientation to a cgame entity's
	// lerpOrigin and lerpAngles instead of the particle system's
	// root, set this.
	centity_t		*entity;

	// If we want to link the orientation to a tag on a player
	// entity's model instead of the particle system's root,
	// set this accompanied by the the entity parameter.
	char			tagName[MAX_QPATH];

	// Local offset from wherever the orientation is linked to.
	// Be it system root, entity or tag.
	vec3_t			offset;

	// If length(dir) != 0, then dir contains an override
	// direction to point emitters / forces to.
	vec3_t			dir; 

	// Deactivate the system as soon as the aura vanishes.
	// Not visible to scripts.
	qboolean		linkAuraState;

	// Decativate the system as soon as the weapon charge
	// or firing flash in question is gone.
	// Not visible to scripts.
	qboolean		linkWeapon;
	int				weaponNr;
	int				weaponState;

	// Needed for force and emitter templates.
	// FIXME: Not too efficient to store here.
	qboolean		autoLink;
	qboolean		entityLink;
} PSys_Orientation_t;

typedef struct PSys_Emitter_s {
	// IMPORTANT: Nothing else should go between these five pointers and the type field,
	//            or a memory error will occur within the loading from cache code.
	struct PSys_System_s	*parent;	// Emitters need to know their parent so they can kill any
										// particles that are rayOrigin-ed to them!
	struct PSys_Emitter_s	*prev, *next;
	struct PSys_Emitter_s	*prev_local, *next_local;
	
	// IMPORTANT: type -MUST- be the next field, or a memory copy error
	//            will occur within the loading from cache code
	PSys_EmitterType_t		type;
	PSys_Orientation_t		orientation;

	float					grndDist;	// Max distance to ground to spawn at ( surface types only )

	int						amount;		// Amount of particles to spawn each session
	float					radius;		// Spawn radius
	float					offset;		// Spawn offset ( radius types only )
	float					posJit;		// Amount of jitter to put into spawn position

	int						lifeTime;	// Lifetime for the emitter
	int						startTime;	// Time the emitter was spawned

	int						waitTime;	// Time to wait between spawn sessions, 0 means only spawn once
	int						lastTime;	// Last time a spawn session took place
	int						initTime;	// Time to wait before the first spawn session

	PSys_ParticleTemplate_t	particleTemplates[MAX_PARTICLE_TEMPLATES];
	int						nrTemplates;

} PSys_Emitter_t;

typedef struct PSys_Force_s {
	// IMPORTANT: Nothing else should go between these four pointers and the type field,
	//            or a memory error will occur within the loading from cache code.
	struct PSys_Force_s		*prev, *next;
	struct PSys_Force_s		*prev_local, *next_local; // Same, but local within the system

	// IMPORTANT: type -MUST- be the next field, or a memory copy error
	//            will occur within the loading from cache code
	PSys_ForceType_t		type;
	PSys_Orientation_t		orientation;
	
	PSys_AOIType_t			AOItype;	// Represents type of area of influence
	float					AOIrange[3];// Represents the x,y,z ratios of the force's area of influence.
										// Not all AOI types may use all three floats.
	
	float					falloff;	// Amount of area over which the force declines from full to no effect.
	float					value;		// Amount of force applied
	float					pullIn;		// Only used for swirl type. Determines inward pull to center of swirl
} PSys_Force_t;

typedef struct PSys_Constraint_s {
	// IMPORTANT: Nothing else should go between these four pointers and the type field,
	//            or a memory error will occur within the loading from cache code.
	struct PSys_Constraint_s	*prev, *next;
	struct PSys_Constraint_s	*prev_local, *next_local; // Same, but local within the system

	// IMPORTANT: type -MUST- be the next field, or a memory copy error
	//            will occur within the loading from cache code	
	PSys_ConstraintType_t	type;
	float					value;	
} PSys_Constraint_t;


typedef struct PSys_Particle_s {
	struct PSys_Particle_s	*prev, *next; // Singly or doubly linked list of particles
	struct PSys_Particle_s	*prev_local, *next_local; // Same, but local within the system

	// Position and movement
	vec3_t		position;
	vec3_t		oldPosition;
	vec3_t		forceAccum;
	vec3_t		accelAccum;	

	// Properties
	float		mass;
	float		invMass;		// Inverse mass
	int			lifeTime;
	int			spawnTime;
	
	/*
	vec3_t		inertia;		// Distribution of mass in object (ie, a long skinny rod)
	vec4_t		orientation;	// Quaternion for orientation of particle
	vec4_t		momentum;		// Quaternion for angular momentum of particle
	vec3_t		torque;			// Accumulated spinning force this frame
	*/

	// Rendering information
	PSys_RenderType_t		rType;
	qhandle_t				shader;
	qhandle_t				model;
	PSys_FloatTimeLerp_t	scale;
	PSys_Vec4TimeLerp_t		rotate;
	PSys_Vec4TimeLerp_t		rgba;

	// Ray information
	struct PSys_Emitter_s	*rayParent;	// parent emitter used to update rayOrigin
	vec3_t					rayOffset;	// original offset from emitter
	vec3_t					rayOrigin;	// last updated starting point of ray
} PSys_Particle_t;

typedef struct PSys_System_s {	
	struct PSys_System_s			*prev, *next;	// Singly or doubly linked list of particle systems

	struct PSys_Particle_s			particles;		// Head of linked list of particles
	struct PSys_Emitter_s			emitters;		// Head of linked list of emitters
	struct PSys_Force_s				forces;			// Head of linked list of forces
	struct PSys_Constraint_s		constraints;	// Head of linked list of constraints

	vec3_t		gravity;
	vec3_t		rootPos;
	vec3_t		rootAxis[3];
} PSys_System_t;

typedef enum {
	MEM_NONE, // Nonactive element
	MEM_EMITTER,
	MEM_FORCE,
	MEM_CONSTRAINT
} PSys_MemberType_t;

typedef struct PSys_MemberTemplate_s {
	PSys_MemberType_t		type;

	union {
		PSys_Emitter_t		emitter;
		PSys_Force_t		force;
		PSys_Constraint_t	constraint;
	}						data;
} PSys_MemberTemplate_t;

typedef struct PSys_SystemTemplate_s {
	char					name[MAX_QPATH];
	float					gravity;
	PSys_MemberTemplate_t	members[MAX_PARTICLESYSTEM_MEMBERS];
} PSys_SystemTemplate_t;

static void PSys_AccumulateSystem( PSys_System_t *system );
static void PSys_AccumulateParticle( PSys_System_t *system, PSys_Particle_t *particle );

static void PSys_IntegrateSystem( PSys_System_t *system, float timeStepSquare, float timeStepCorrected );
static void PSys_IntegrateParticle( PSys_Particle_t *particle, float timeStepSquare, float timeStepCorrected );

static qboolean PSys_ConstrainSystem( PSys_System_t *system );
static qboolean PSys_ApplyConstraint( PSys_System_t *system, PSys_Constraint_t *constraint );
	static qboolean PSys_ApplyDistanceMaxConstraint( PSys_System_t *system, float value );
	static qboolean PSys_ApplyDistanceMinConstraint( PSys_System_t *system, float value );
	static qboolean PSys_ApplyDistanceConstraint( PSys_System_t *system, float value );
	static qboolean PSys_ApplyPlaneConstraint( PSys_System_t *system, float value );

void PSys_InitCache( void );
PSys_SystemTemplate_t* PSys_LoadSystemFromCache( char *systemName );
