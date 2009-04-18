// Copyright (C) 2003-2004 RiO
//
// cg_particlesystem.h -- particle system headers


#define MAX_PARTICLES_PER_SYSTEM	30
#define MAX_PARTICLESYSTEMS			64	// An average of two systems per client if MAX_CLIENTS
										// are connected.

#define MIN_BOUNCE_DELTA			 8	// The minium speed in units / dsec a particle must have after bouncing to
										// continue to exist.

typedef struct {
	vec3_t		trBase;
	vec3_t		trDelta;
	int			trDeltaSpin;		// spinning in degrees per second
	int			trBaseSpin;
	int			trMass;
	int			trTime;
	int			trShader;
	qboolean	trSpawnedBefore;
} particleTrajectory_t;

typedef struct particleSystem_s {	
	// Singly or doubly linked list of particleSystems
	struct particleSystem_s		*prev, *next;
	
	// Particle trajectory information
	particleTrajectory_t		trajectory[MAX_PARTICLES_PER_SYSTEM];
	
	// Particle generator settings
	vec3_t						direction;
	float						speedDeviation;
	vec3_t						origin;
	float						offset;
	float						offsetDeviation;
	float						spreadAngle;
	qboolean					variateSpread;
	int							amount;
	qboolean					respawn; // reproduce particles at spray again when destroyed
	int							spawndelay;
	int							lastSpawnTime;
	
	// Particle system settings
	int							systemTime;
	int							system_lifetime;
	int							parentNr; // to which entity the generator is attached
	float						drawDist; // maximum distance at which the system will be drawn
	int							nrParticles;


	// Forces
	vec3_t						force; // force induced by wind, etc.
	vec3_t						force_turbulence; // turbulence in the force (for wind)
	vec3_t						gravity; // force induced by gravity
	float						bounceFactor; // 0 = no bouncing
	float						maxForceDistance; // maximum distance from the generator where force still has
												  // effect on a particle
	qboolean					nullForceOnParentLoss; // decides whether or not to set the force to zero if the parent is lost.

	// particle settings
	qhandle_t					shaders[4];
	int							min_size;
	int							max_size;
	int							peakPct_size;
	int							particle_lifetime;
	qboolean					rejuvenate;

	int							min_RGBA; // RGBA value in range 0..255
	int							max_RGBA; // RGBA value in range 0..255
	int							spin;
	int							spinDeviation;
	int							peakPct_RGBA;
} particleSystem_t;



// functions from cg_particlesystem.c
void CG_InitParticleSystems( void );
particleSystem_t *CG_AllocParticleSystem( void );
void CG_AddParticleSystems( void );

// particle system templates from cg_particlesystem.c
particleSystem_t *CG_CreateParticleSystem_ExplodeDebris( vec3_t position, vec3_t direction, int amount );
particleSystem_t *CG_CreateParticleSystem_DebrisTrail( vec3_t direction, int parentNr );
particleSystem_t *CG_CreateParticleSystem_DriftDebris( int parentNr );
