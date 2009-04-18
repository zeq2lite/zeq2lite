// Copyright (C) 2003-2004 RiO
//
// cg_particlesystem.c -- maintains particle systems

#include "cg_local.h"


static particleSystem_t	cg_particleSystems[MAX_PARTICLESYSTEMS];
static particleSystem_t	cg_particleSystems_activeList;
static particleSystem_t *cg_particleSystems_freeList;


/*
   -- Helper Functions --
*/

void CG_VectorLocalDeviation(vec3_t inVector, float rotationAngle, vec3_t outVector) {
	float  inVectorLength;
	vec3_t inAngles, rotAngles;
	vec4_t inQuat, rotQuat, outQuat;
	
	// First retrieve the vector's length and angles, randomizing the roll.
	vectoangles( inVector, inAngles );
	inAngles[2] = random() * 360;
	inVectorLength = VectorLength( inVector );

	// Convert them to a quaternion for local coordinate rotation.
	// FIXME: How the heck does Q3 implement rotation matrices anyway?
	//        Those would probably be a bit faster in execution.
	AnglesToQuat( inAngles, inQuat );
	VectorSet( rotAngles, rotationAngle, 0, 0 );
	AnglesToQuat( rotAngles, rotQuat );
	
	// Multiply quaternions and output forward component of axes,
	// which is the locally rotated inVector.
	QuatMul( inQuat, rotQuat, outQuat );
	QuatToVector( outQuat, outVector );

	// Set the length correctly again
	VectorNormalize( outVector );
	VectorScale( outVector, inVectorLength, outVector );
}

/*
   -----------------------------------------------------
     P A R T I C L E S Y S T E M   M A N A G E M E N T
   -----------------------------------------------------
*/

/*
========================
CG_InitParticleSystems
========================
  This is called at startup and for tournement restarts.
*/
void CG_InitParticleSystems( void ) {
	int		i;

	memset( cg_particleSystems, 0, sizeof( cg_particleSystems ) );
	cg_particleSystems_activeList.next = &cg_particleSystems_activeList;
	cg_particleSystems_activeList.prev = &cg_particleSystems_activeList;
	cg_particleSystems_freeList = cg_particleSystems;

	// singly link the free list and set some properties
	for ( i = 0 ; i < MAX_PARTICLESYSTEMS - 1 ; i++ ) {
		cg_particleSystems[i].next = &cg_particleSystems[i+1];
	}
}



/*
=======================
CG_FreeParticleSystem
=======================
*/
void CG_FreeParticleSystem( particleSystem_t *psys ) {
	centity_t * cent;
	int i;

	if ( !psys->prev ) {
		CG_Error( "CG_FreeParticleSystem: not active" );
	}

	// find and unlink any parent entity
	if ( psys->parentNr != ENTITYNUM_NONE ) {
		cent = &cg_entities[psys->parentNr];
		for ( i = 0; i < 5; i++ ) {
			if ( cent->particleSystems[i] == psys ) {
				cent->particleSystems[i] = 0;
			}
		}
	}

	// remove from the doubly linked active list
	psys->prev->next = psys->next;
	psys->next->prev = psys->prev;

	// the free list is only singly linked
	psys->next = cg_particleSystems_freeList;
	cg_particleSystems_freeList = psys;
}



/*
========================
CG_AllocParticleSystem
========================
  Will always succeed, even if it requires freeing an old active system.
*/
particleSystem_t *CG_AllocParticleSystem( void ) {
	particleSystem_t	*psys;

	if ( !cg_particleSystems_freeList ) {
		// No free entities, so free the one at the end of the chain,
		// removing the oldest active entity.
		// FIXME: Is it worth the time-complexity of a linear search to find
		//        the system closest to expiring its lifetime instead?
		CG_FreeParticleSystem( cg_particleSystems_activeList.prev );
	}

	psys = cg_particleSystems_freeList;
	cg_particleSystems_freeList = cg_particleSystems_freeList->next;

	memset( psys, 0, sizeof( *psys ) );
	psys->parentNr = ENTITYNUM_NONE;

	// Set the activation times of the system;
	psys->systemTime = cg.time;
	psys->lastSpawnTime = cg.time;

	// link into the active list
	psys->next = cg_particleSystems_activeList.next;
	psys->prev = &cg_particleSystems_activeList;
	cg_particleSystems_activeList.next->prev = psys;
	cg_particleSystems_activeList.next = psys;

	return psys;
}



/*
===================
CG_UpdateParticle
===================
  Updates the information for a single particle within a system.
*/
void CG_UpdateParticle( particleSystem_t *psys, particleTrajectory_t *particle ) {
	vec3_t	trAcc, trTurbForce, trTurbAngles, newBase, newDelta, bounceDelta;
	trace_t	trace;
	float   timeDelta, bounceTimeDelta;
	float	timeDeltaSquared;
	float	dot;

	// If the system rejuvenates particles, reset the trTime field of the particle
	if ( psys->rejuvenate ) {
		particle->trTime = cg.time;
	}

	// set total acceleration on this frame
	if ( psys->maxForceDistance != 0.0f && ( Distance(particle->trBase, psys->origin) > psys->maxForceDistance ) ) {
		VectorCopy( psys->gravity, trAcc );
	} else {
		// Add the system's turbulence to the force.
		if ( psys->force_turbulence[0] || psys->force_turbulence[1] || psys->force_turbulence[2] ) {
			vectoangles( psys->force, trTurbAngles );
			VectorMA( trTurbAngles, crandom(), psys->force_turbulence, trTurbAngles );
			// NOTE: Angle normalizing is not necessary here, so don't bother with it.
			AngleVectors( trTurbAngles, trTurbForce, NULL, NULL );
			// Give new vector its old length back.
			// NOTE: Created vector is already normalized.
			VectorScale ( trTurbForce, VectorLength( psys->force ), trTurbForce );
		} else {
			VectorCopy( psys->force, trTurbForce );
		}
		VectorScale( trTurbForce, 1.0f / (float)particle->trMass, trTurbForce );
		VectorAdd( psys->gravity, trTurbForce, trAcc );
	}
	
	timeDelta = (float)(cg.frametime) / 100.0f;
	timeDeltaSquared = timeDelta * timeDelta;

	// perform physics for frame
	newBase[0] = particle->trBase[0] + particle->trDelta[0] * timeDelta +
					0.5f * trAcc[0] * timeDeltaSquared;
	newBase[1] = particle->trBase[1] + particle->trDelta[1] * timeDelta +
					0.5f * trAcc[1] * timeDeltaSquared;
	newBase[2] = particle->trBase[2] + particle->trDelta[2] * timeDelta +
					0.5f * trAcc[2] * timeDeltaSquared;
	newDelta[0] = particle->trDelta[0] + trAcc[0] * timeDelta;
	newDelta[1] = particle->trDelta[1] + trAcc[1] * timeDelta;
	newDelta[2] = particle->trDelta[2] + trAcc[2] * timeDelta;

	// trace to detect collisions
	CG_Trace( &trace, particle->trBase, NULL, NULL, newBase, -1, CONTENTS_SOLID );
	if ( trace.startsolid || trace.allsolid ) {
		// make sure the entityNum is set to the one we're stuck in
		CG_Trace( &trace, particle->trBase, NULL, NULL, particle->trBase, -1, CONTENTS_SOLID );
		trace.fraction = 0.0f;
	}

	if ( trace.fraction < 1.0f ) {

		if ( psys->bounceFactor == 0.0f ) { // FIXME: Necessary to avoid problem interpreting float as boolean, or not?

			particle->trTime = cg.time - psys->particle_lifetime - 50; // make sure particle is dead
			return;
		}

		// reflect the velocity on the trace plane
		bounceTimeDelta = timeDelta * trace.fraction;
		// NOTE: Doing this linearly is not totally accurate, but you shouldn't
		//       be able to notice it much.

		bounceDelta[0] = particle->trDelta[0] + trAcc[0] * bounceTimeDelta;
		bounceDelta[1] = particle->trDelta[1] + trAcc[1] * bounceTimeDelta;
		bounceDelta[2] = particle->trDelta[2] + trAcc[2] * bounceTimeDelta;

		dot = DotProduct( newDelta, trace.plane.normal );
		VectorMA( bounceDelta, -2 * dot, trace.plane.normal, newDelta );

		VectorScale( newDelta, psys->bounceFactor, newDelta );
		
		// check for stop
		if ( trace.plane.normal[2] > 0.2 && VectorLength( newDelta ) < MIN_BOUNCE_DELTA ) {
			particle->trTime = cg.time - psys->particle_lifetime - 50; // make sure particle is dead
			return;				
		}
		
		VectorAdd( trace.endpos, trace.plane.normal, newBase);
	}

	particle->trBaseSpin += particle->trDeltaSpin * (timeDelta / 10);
	AngleNormalize180(particle->trBaseSpin);
	
	
	VectorCopy( newBase, particle->trBase );
	VectorCopy( newDelta, particle->trDelta );	
}



/*
==================
CG_SpawnParticle
==================
  Spawns a new particle in the system.
*/
void CG_SpawnParticle( particleSystem_t *psys, particleTrajectory_t *particle ) {
	int i;
	float varSpreadAngle;
	vec3_t speedDeviation;
	float varOffset;
	vec3_t offsetDir;

	if ( psys->variateSpread ) {
		varSpreadAngle = random() * psys->spreadAngle;
	} else {
		varSpreadAngle = psys->spreadAngle;
	}

	if (psys->spreadAngle != 0 ) {
		CG_VectorLocalDeviation( psys->direction, varSpreadAngle, particle->trDelta );
	} else {
		VectorCopy( psys->direction, particle->trDelta );
	}

	VectorNormalize2( particle->trDelta, speedDeviation );
	VectorMA( particle->trDelta, (1.0f - 2.0f * random()) * psys->speedDeviation, speedDeviation, particle->trDelta );

	if ( (psys->offset > 0) || (psys->offsetDeviation > 0) ) {
		trace_t trace;

		varOffset = (1.0f - 2.0f * random() ) * psys->offsetDeviation + psys->offset;
		CG_VectorLocalDeviation( psys->direction, 90.0f, offsetDir );
		
		VectorNormalize( offsetDir );
		VectorScale( offsetDir, varOffset, offsetDir );

		VectorAdd( psys->origin, offsetDir, particle->trBase );
		VectorNormalize( offsetDir );
		
		// Trace to make sure the particle would end up in a valid position, otherwise
		// don't spawn it at all.
		CG_Trace( &trace, psys->origin, NULL, NULL, particle->trBase, -1, CONTENTS_SOLID );
		if ( trace.startsolid || trace.allsolid ) {
			trace.fraction = 0;
		}		
		if ( trace.fraction < 1.0f ) {
			VectorCopy( trace.endpos, particle->trBase );
		}

		// Nudge it back just a little, just in case.
		VectorSubtract( particle->trBase, offsetDir, particle->trBase );

	} else {
		VectorCopy( psys->origin, particle->trBase );
		
	}

	particle->trMass = 1;
	particle->trTime = cg.time;
	particle->trSpawnedBefore = qtrue;

	particle->trBaseSpin = random() * 360.0f;
	particle->trDeltaSpin = psys->spin + ( 1.0f - 2.0f * random() ) * psys->spinDeviation;

	// Don't forget to randomize the spinning direction as well
	if (random() > 0.5f) {
		particle->trDeltaSpin = -particle->trDeltaSpin;
	}	

	// Select random shader for this particle
	for ( i = 0 ; i < 4 ; i++ ) {
		if ( !psys->shaders[i] ) {
			break;
		}
	}
	if ( i > 0 ) {
		i = rand() % i;
		if ( psys->shaders[i] )
		{
			particle->trShader = psys->shaders[i];
		}
	}

	psys->lastSpawnTime += psys->spawndelay;
}


/*
=======================
CG_DrawParticleSystem
=======================
  Updates the information for a particle system, then draws it.
*/
void CG_DrawParticleSystem( particleSystem_t *psys ) {
	int			i, j, k;
	qboolean	deadSystem;
	polyVert_t	verts[4];
	vec3_t		axis[3], viewangles;
	centity_t	*cent;

	// FAILSAFE: Check if the set amount of particles exceeds the max.
	//			 If it does, set to the max.
	if ( psys->nrParticles > MAX_PARTICLES_PER_SYSTEM ) {
		psys->nrParticles = MAX_PARTICLES_PER_SYSTEM;
	}

	deadSystem = qtrue;

	// Check if the system life time has not yet expired. If it hasn't,
	// disable deadSystem, as the system should still be alive.
	if ( (( psys->systemTime + psys->system_lifetime ) >= cg.time) ) {
		deadSystem = qfalse;
	}

	// If the system is linked to a client entity.
	
	if ( psys->parentNr != ENTITYNUM_NONE) {
		cent = &cg_entities[psys->parentNr]; 
		if ( !cent->particleActiveToggle ) {
			// The parent entity was invisible this frame, so we undo the
			// links with it and disable all the entity's particle systems'
			// respawn flags incase any were active.
			for ( i = 0; i < 5; i++ ) {
				if ( cent->particleSystems[i] ) {
					cent->particleSystems[i]->parentNr = ENTITYNUM_NONE;
					cent->particleSystems[i]->respawn = qfalse;
					cent->particleSystems[i]->rejuvenate = qfalse;

					if ( cent->particleSystems[i]->nullForceOnParentLoss ) {
						VectorSet( cent->particleSystems[i]->force, 0, 0, 0 );
						VectorSet( cent->particleSystems[i]->force_turbulence, 0, 0, 0) ;
					}

					cent->particleSystems[i] = 0;
					
				}
			}
		}

		// Get the new origin of the particle generator
		if ( cent->particleSystems[4] == psys ) {
			VectorCopy(cent->groundPoint, psys->origin);
		} else {
			VectorCopy(cent->lerpOrigin, psys->origin);
		}
	}



	// Walk through all the particles
	for ( i = 0; i < psys->nrParticles; i++ ) {
		
		// If we encounter a particle that has passed its time to exist
		if ( ( psys->trajectory[i].trTime + psys->particle_lifetime ) < cg.time ) {

			// Check if we should respawn the particle
			if ( psys->respawn || !psys->trajectory[i].trSpawnedBefore ) {
				// Since the system is set to still (re)spawn particles, we
				// can switch off the deadSystem indicator.
				deadSystem = qfalse;

				// Check if we have passed the respawn time yet, then spawn
				// and position a new particle.
				if ( ( psys->lastSpawnTime + psys->spawndelay ) < cg.time ) {
					CG_SpawnParticle( psys, &psys->trajectory[i] );
					CG_UpdateParticle( psys, &psys->trajectory[i] );
				}
			}

		} else {
			// Since we're encountering an active particle, we
			// can switch off the deadSystem indicator.
			deadSystem = qfalse;

			CG_UpdateParticle( psys, &psys->trajectory[i] );
		}
	}

	// If we've never encountered an active particle and particles won't respawn,
	// then we can free the system, because it no longer has activity.
	if (deadSystem) {
		CG_FreeParticleSystem( psys );
		return;
	}

	
	// Do a check of the draw distance. The system can then be ignored if it is beyond
	// the allowed drawing distance. This will save a lot of resources.
	
	//if ( ( Distance( cg.refdef.vieworg, psys->origin ) ) > psys->drawDist ) {
	//	return;
	//}

	for ( i = 0; i < psys->nrParticles; i++ ) {

		// If we come across a 'dead' particle, skip to the next.
		if ( ( psys->trajectory[i].trTime + psys->particle_lifetime ) < cg.time ) {
			continue;
		}

		vectoangles( cg.refdef.viewaxis[0], viewangles );
		viewangles[2] = psys->trajectory[i].trBaseSpin; // NOTE: Already normalized
		AnglesToAxis( viewangles, axis );
		

		VectorMA (psys->trajectory[i].trBase, -0.5f * psys->max_size, axis[1], verts[0].xyz );
		VectorMA (verts[0].xyz, -0.5f * psys->max_size, axis[2], verts[0].xyz );
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;

		VectorMA (psys->trajectory[i].trBase, 0.5f * psys->max_size, axis[1], verts[1].xyz );
		VectorMA (verts[1].xyz, -0.5f * psys->max_size, axis[2], verts[1].xyz );
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;

		VectorMA (psys->trajectory[i].trBase, 0.5f * psys->max_size, axis[1], verts[2].xyz );
		VectorMA (verts[2].xyz, 0.5f * psys->max_size, axis[2], verts[2].xyz );
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;

		VectorMA (psys->trajectory[i].trBase, -0.5f * psys->max_size, axis[1], verts[3].xyz );
		VectorMA (verts[3].xyz, 0.5f * psys->max_size, axis[2], verts[3].xyz );
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;

		for ( j = 0; j < 4; j++ ) {
			for ( k = 0; k < 4; k++ ) {
				verts[j].modulate[k] = psys->max_RGBA;
			}
		}

		trap_R_AddPolyToScene( psys->trajectory[i].trShader, 4, verts );
	}
}


/*
=======================
CG_AddParticleSystems
=======================
  Master function for updating all particle systems.
  Called in cg_view.c each frame to add our new particles to render lists.
*/
void CG_AddParticleSystems( void ) {
	particleSystem_t	*psys, *next;
	int i;

	// Walk the list backwards, so any new particle systems generated
	// will be present this frame
	psys = cg_particleSystems_activeList.prev;
	for ( ; psys != &cg_particleSystems_activeList ; psys = next ) {
		// grab next now, so if the system is freed we
		// still have it
		next = psys->prev;

		// perform updating / drawing of selected system from list
		CG_DrawParticleSystem( psys );
	}

	// flip particleActiveToggle. Because we don't know which are
	// currently active and which are not, we just do it for all entities.
	for (i = 0; i < MAX_GENTITIES; i++) {
		cg_entities[i].particleActiveToggle = qfalse;
	}
}



/*
   ---------------------------------------------------
     P A R T I C L E S Y S T E M   P R E B U I L T S
   ---------------------------------------------------
*/


particleSystem_t *CG_CreateParticleSystem_ExplodeDebris( vec3_t position, vec3_t direction, int amount ) {
	particleSystem_t *psys;
	int i;

	psys = CG_AllocParticleSystem();

	psys->system_lifetime = 1000;
	psys->respawn = qfalse;
	psys->drawDist = 1024;
	psys->nrParticles = amount;
	psys->bounceFactor = 0.32f;
	psys->speedDeviation = 5;

	VectorCopy  ( position, psys->origin );
	VectorNormalize2( direction, psys->direction );
	VectorScale ( psys->direction, 40, psys->direction);

	VectorSet( psys->force, 0, 0, 0 );
	VectorSet( psys->gravity, 0, 0, -9.81f );

	psys->particle_lifetime = 10000;
	psys->spawndelay = 0;
	psys->max_size = 8;
	psys->max_RGBA = 255;

	psys->spin = 250;
	psys->spinDeviation = 30;
	
	psys->spreadAngle = 40;
	psys->variateSpread = qfalse;

	psys->shaders[0] = cgs.media.rockParticle1Shader;
	psys->shaders[1] = cgs.media.rockParticle2Shader;
	psys->shaders[2] = cgs.media.rockParticle3Shader;
	psys->shaders[3] = 0;

	// Make sure the particles are wiped and available
	for ( i = 0; i < MAX_PARTICLES_PER_SYSTEM; i++ ) {
		psys->trajectory[i].trTime = (cg.time - psys->particle_lifetime) - 500;
	}

	return psys;
}


particleSystem_t *CG_CreateParticleSystem_DebrisTrail(vec3_t direction, int parentNr) {
	particleSystem_t *psys;
	int i;

	psys = CG_AllocParticleSystem();

	psys->system_lifetime = 1000;
	psys->respawn = qtrue;
	psys->drawDist = 1024;
	psys->nrParticles = 30;
	psys->bounceFactor = 0.32f;
	psys->speedDeviation = 2;
	psys->parentNr = parentNr;

	VectorNormalize2( direction, psys->direction );
	VectorScale ( psys->direction, 20, psys->direction);

	VectorSet( psys->force, 0, 0, 0 );
	VectorSet( psys->gravity, 0, 0, -9.81f );

	psys->particle_lifetime = 10000;
	psys->spawndelay = 10;
	psys->max_size = 8;
	psys->max_RGBA = 255;

	psys->spin = 250;
	psys->spinDeviation = 30;
	
	psys->spreadAngle = 40;
	psys->variateSpread = qfalse;

	psys->shaders[0] = cgs.media.rockParticle1Shader;
	psys->shaders[1] = cgs.media.rockParticle2Shader;
	psys->shaders[2] = cgs.media.rockParticle3Shader;
	psys->shaders[3] = 0;

	// Make sure the particles are wiped and available
	for ( i = 0; i < MAX_PARTICLES_PER_SYSTEM; i++ ) {
		psys->trajectory[i].trTime = (cg.time - psys->particle_lifetime) - 500;
	}

	return psys;
}


particleSystem_t *CG_CreateParticleSystem_DriftDebris( int parentNr ) {
	particleSystem_t *psys;
	int i;

	psys = CG_AllocParticleSystem();

	psys->system_lifetime = 1000;
	psys->respawn = qtrue;
	psys->rejuvenate = qtrue;
	psys->maxForceDistance = 100;
	psys->drawDist = 1024;
	psys->nrParticles = 18;
	psys->bounceFactor = 0.32f;
	psys->speedDeviation = 0.5f;
	psys->parentNr = parentNr;

	VectorSet( psys->direction, 0, 0, 6.0f );

	VectorSet( psys->force, 0, 0, 10.0f );
	VectorSet( psys->force_turbulence, 10, 6, 8 );
	VectorSet( psys->gravity, 0, 0, -9.81f );

	psys->particle_lifetime = 10000;
	psys->spawndelay = 50;
	psys->max_size = 6;
	psys->max_RGBA = 255;

	psys->spin = 250;
	psys->spinDeviation = 30;
	
	psys->offset = 40;
	psys->offsetDeviation = 10;
	psys->spreadAngle = 0;
	psys->variateSpread = qfalse;

	psys->shaders[0] = cgs.media.rockParticle1Shader;
	psys->shaders[1] = cgs.media.rockParticle2Shader;
	psys->shaders[2] = cgs.media.rockParticle3Shader;
	psys->shaders[3] = 0;

	psys->nullForceOnParentLoss = qtrue;

	// Make sure the particles are wiped and available
	for ( i = 0; i < MAX_PARTICLES_PER_SYSTEM; i++ ) {
		psys->trajectory[i].trTime = (cg.time - psys->particle_lifetime) - 500;
	}

	return psys;
}
