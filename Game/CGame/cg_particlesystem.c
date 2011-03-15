// Copyright (C) 2003-2004 RiO
//
// cg_particlesystem.c -- maintains particle systems

#include "cg_local.h"
#include "cg_particlesystem.h"

#define MAX_ITERATIONS		10 // NOTE -RiO; Will this be enough?
#define MIN_BOUNCE_DELTA	 8

// Linked list storage for the systems, particles, forces and constraints.
static PSys_System_t			PSys_Systems[MAX_PARTICLESYSTEMS];
static PSys_System_t			PSys_Systems_inuse;
static PSys_System_t			*PSys_Systems_free;

static PSys_Emitter_t			PSys_Emitters[MAX_EMITTERS];
static PSys_Emitter_t			PSys_Emitters_inuse;
static PSys_Emitter_t			*PSys_Emitters_free;

static PSys_Particle_t			PSys_Particles[MAX_PARTICLES];
static PSys_Particle_t			PSys_Particles_inuse;
static PSys_Particle_t			*PSys_Particles_free;

static PSys_Force_t				PSys_Forces[MAX_FORCES];
static PSys_Force_t				PSys_Forces_inuse;
static PSys_Force_t				*PSys_Forces_free;

static PSys_Constraint_t		PSys_Constraints[MAX_CONSTRAINTS];
static PSys_Constraint_t		PSys_Constraints_inuse;
static PSys_Constraint_t		*PSys_Constraints_free;

static float					PSys_LastTimeStep;

/*
-------------------------------

  I N I T I A L I Z A T I O N

-------------------------------
*/

static void PSys_InitSystems( void ) {
	int		i;

	memset( PSys_Systems, 0, sizeof( PSys_Systems ) );
	PSys_Systems_inuse.next = &PSys_Systems_inuse;
	PSys_Systems_inuse.prev = &PSys_Systems_inuse;
	PSys_Systems_free = PSys_Systems;

	// singly link the free list
	for ( i = 0 ; i < MAX_PARTICLESYSTEMS - 1 ; i++ ) {
		PSys_Systems[i].next = &PSys_Systems[i+1];
	}
}

static void PSys_InitParticles( void ) {
	int		i;

	memset( PSys_Particles, 0, sizeof( PSys_Particles ) );
	PSys_Particles_inuse.next = &PSys_Particles_inuse;
	PSys_Particles_inuse.prev = &PSys_Particles_inuse;
	PSys_Particles_free = PSys_Particles;

	// singly link the free list
	for ( i = 0 ; i < MAX_PARTICLES - 1 ; i++ ) {
		PSys_Particles[i].next = &PSys_Particles[i+1];
	}
}

static void PSys_InitEmitters( void ) {
	int		i;

	memset( PSys_Emitters, 0, sizeof( PSys_Emitters ) );
	PSys_Emitters_inuse.next = &PSys_Emitters_inuse;
	PSys_Emitters_inuse.prev = &PSys_Emitters_inuse;
	PSys_Emitters_free = PSys_Emitters;

	// singly link the free list
	for ( i = 0 ; i < MAX_EMITTERS - 1 ; i++ ) {
		PSys_Emitters[i].next = &PSys_Emitters[i+1];
	}
}

static void PSys_InitForces( void ) {
	int		i;

	memset( PSys_Forces, 0, sizeof( PSys_Forces ) );
	PSys_Forces_inuse.next = &PSys_Forces_inuse;
	PSys_Forces_inuse.prev = &PSys_Forces_inuse;
	PSys_Forces_free = PSys_Forces;

	// singly link the free list
	for ( i = 0 ; i < MAX_FORCES - 1 ; i++ ) {
		PSys_Forces[i].next = &PSys_Forces[i+1];
	}
}

static void PSys_InitConstraints( void ) {
	int		i;

	memset( PSys_Constraints, 0, sizeof( PSys_Constraints ) );
	PSys_Constraints_inuse.next = &PSys_Constraints_inuse;
	PSys_Constraints_inuse.prev = &PSys_Constraints_inuse;
	PSys_Constraints_free = PSys_Constraints;

	// singly link the free list
	for ( i = 0 ; i < MAX_CONSTRAINTS - 1 ; i++ ) {
		PSys_Constraints[i].next = &PSys_Constraints[i+1];
	}
}

void CG_InitParticleSystems( void ) {
	PSys_InitSystems();
	PSys_InitParticles();
	PSys_InitEmitters();
	PSys_InitForces();
	PSys_InitConstraints();
	PSys_InitCache();

	PSys_LastTimeStep = 1;
}


/*
-------------------

  S P A W N I N G

-------------------
*/

static void PSys_FreeParticle( PSys_Particle_t *particle ) {

	if ( !particle->prev ) {
		CG_Error( "PSys_FreeParticle: not active" );
	}

	// remove from the doubly linked global active list
	particle->prev->next = particle->next;
	particle->next->prev = particle->prev;

	// remove from the doubly linked local active list
	particle->prev_local->next_local = particle->next_local;
	particle->next_local->prev_local = particle->prev_local;

	// the free list is only singly linked
	particle->next = PSys_Particles_free;
	PSys_Particles_free = particle;
}


static PSys_Particle_t *PSys_SpawnParticle( PSys_System_t *system ) {
	PSys_Particle_t	*particle;

	if ( !PSys_Particles_free ) {
		// No free entities, so free the one at the end of the chain,
		// removing the oldest active entity.
		PSys_FreeParticle( PSys_Particles_inuse.prev );
	}

	particle = PSys_Particles_free;
	PSys_Particles_free = PSys_Particles_free->next;

	memset( particle, 0, sizeof( PSys_Particle_t ) );

	// link into the global active list
	particle->next = PSys_Particles_inuse.next;
	particle->prev = &PSys_Particles_inuse;
	PSys_Particles_inuse.next->prev = particle;
	PSys_Particles_inuse.next = particle;

	// link into the local active list of the system
	particle->next_local = system->particles.next_local;
	particle->prev_local = &(system->particles);
	system->particles.next_local->prev_local = particle;
	system->particles.next_local = particle;

	return particle;
}

static void PSys_FreeEmitter( PSys_Emitter_t *emitter ) {
	PSys_System_t	*system;
	PSys_Particle_t	*particle, *next_p;

	if ( !emitter->prev ) {
		CG_Error( "PSys_FreeEmitter: not active" );
	}

	system = emitter->parent;

	// unlink the local list of particles that are rayParent linked to this emitter
	particle = system->particles.prev_local;
	for ( ; particle != &(system->particles) ; particle = next_p ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next_p = particle->prev_local;

		if ( particle->rayParent == emitter ) {
			particle->rayParent = NULL;
		}
	}

	// remove from the doubly linked global active list
	emitter->prev->next = emitter->next;
	emitter->next->prev = emitter->prev;

	// remove from the doubly linked local active list
	emitter->prev_local->next_local = emitter->next_local;
	emitter->next_local->prev_local = emitter->prev_local;

	// the free list is only singly linked
	emitter->next = PSys_Emitters_free;
	PSys_Emitters_free = emitter;
}


static PSys_Emitter_t *PSys_SpawnEmitter( PSys_System_t *system ) {
	PSys_Emitter_t	*emitter;

	if ( !PSys_Emitters_free ) {
		// No free entities, so free the one at the end of the chain,
		// removing the oldest active entity.
		PSys_FreeEmitter( PSys_Emitters_inuse.prev );
	}

	emitter = PSys_Emitters_free;
	PSys_Emitters_free = PSys_Emitters_free->next;

	memset( emitter, 0, sizeof( PSys_Emitter_t ) );

	// link into the global active list
	emitter->next = PSys_Emitters_inuse.next;
	emitter->prev = &PSys_Emitters_inuse;
	PSys_Emitters_inuse.next->prev = emitter;
	PSys_Emitters_inuse.next = emitter;

	// link into the local active list of the system
	emitter->next_local = system->emitters.next_local;
	emitter->prev_local = &(system->emitters);
	system->emitters.next_local->prev_local = emitter;
	system->emitters.next_local = emitter;

	emitter->parent = system;

	return emitter;
}

static void PSys_FreeForce( PSys_Force_t *force ) {

	if ( !force->prev ) {
		CG_Error( "PSys_FreeForce: not active" );
	}

	// remove from the doubly linked global active list
	force->prev->next = force->next;
	force->next->prev = force->prev;

	// remove from the doubly linked local active list
	force->prev_local->next_local = force->next_local;
	force->next_local->prev_local = force->prev_local;

	// the free list is only singly linked
	force->next = PSys_Forces_free;
	PSys_Forces_free = force;
}


static PSys_Force_t *PSys_SpawnForce( PSys_System_t *system ) {
	PSys_Force_t	*force;

	if ( !PSys_Forces_free ) {
		// No free entities, so free the one at the end of the chain,
		// removing the oldest active entity.
		PSys_FreeForce( PSys_Forces_inuse.prev );
	}

	force = PSys_Forces_free;
	PSys_Forces_free = PSys_Forces_free->next;

	memset( force, 0, sizeof( PSys_Force_t ) );

	// link into the global active list
	force->next = PSys_Forces_inuse.next;
	force->prev = &PSys_Forces_inuse;
	PSys_Forces_inuse.next->prev = force;
	PSys_Forces_inuse.next = force;

	// link into the local active list of the system
	force->next_local = system->forces.next_local;
	force->prev_local = &(system->forces);
	system->forces.next_local->prev_local = force;
	system->forces.next_local = force;

	return force;
}


static void PSys_FreeConstraint( PSys_Constraint_t *constraint ) {

	if ( !constraint->prev ) {
		CG_Error( "PSys_FreeConstraint: not active" );
	}

	// remove from the doubly linked global active list
	constraint->prev->next = constraint->next;
	constraint->next->prev = constraint->prev;

	// remove from the doubly linked local active list
	constraint->prev_local->next_local = constraint->next_local;
	constraint->next_local->prev_local = constraint->prev_local;

	// the free list is only singly linked
	constraint->next = PSys_Constraints_free;
	PSys_Constraints_free = constraint;
}


static PSys_Constraint_t *PSys_SpawnConstraint( PSys_System_t *system ) {
	PSys_Constraint_t	*constraint;

	if ( !PSys_Constraints_free ) {
		// No free entities, so free the one at the end of the chain,
		// removing the oldest active entity.
		PSys_FreeConstraint( PSys_Constraints_inuse.prev );
	}

	constraint = PSys_Constraints_free;
	PSys_Constraints_free = PSys_Constraints_free->next;

	memset( constraint, 0, sizeof( PSys_Constraint_t ) );

	// link into the global active list
	constraint->next = PSys_Constraints_inuse.next;
	constraint->prev = &PSys_Constraints_inuse;
	PSys_Constraints_inuse.next->prev = constraint;
	PSys_Constraints_inuse.next = constraint;

	// link into the local active list of the system
	constraint->next_local = system->constraints.next_local;
	constraint->prev_local = &(system->constraints);
	system->constraints.next_local->prev_local = constraint;
	system->constraints.next_local = constraint;

	return constraint;
}


static void PSys_FreeSystem( PSys_System_t *system ) {
	PSys_Particle_t		*particle,	*next_p;
	PSys_Emitter_t		*emitter,	*next_e;
	PSys_Force_t		*force,		*next_f;
	PSys_Constraint_t	*constraint,*next_c;

	if ( !system->prev ) {
		CG_Error( "PSys_FreeSystem: not active" );
	}

	// free the local list of particles
	particle = system->particles.prev_local;
	for ( ; particle != &(system->particles) ; particle = next_p ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next_p = particle->prev_local;

		PSys_FreeParticle( particle );
	}

	// free the local list of emitters
	emitter = system->emitters.prev_local;
	for ( ; emitter != &(system->emitters) ; emitter = next_e ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next_e = emitter->prev_local;

		PSys_FreeEmitter( emitter );
	}

	// free the local list of forces
	force = system->forces.prev_local;
	for ( ; force != &(system->forces) ; force = next_f ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next_f = force->prev_local;

		PSys_FreeForce( force );
	}

	// free the local list of constraints
	constraint = system->constraints.prev_local;
	for ( ; constraint != &(system->constraints) ; constraint = next_c ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next_c = constraint->prev_local;

		PSys_FreeConstraint( constraint );
	}

	// remove from the doubly linked active list
	system->prev->next = system->next;
	system->next->prev = system->prev;

	// the free list is only singly linked
	system->next = PSys_Systems_free;
	PSys_Systems_free = system;
}


static PSys_System_t *PSys_SpawnSystem( void ) {
	PSys_System_t	*system;

	if ( !PSys_Systems_free ) {
		// No free entities, so free the one at the end of the chain,
		// removing the oldest active entity.
		PSys_FreeSystem( PSys_Systems_inuse.prev );
	}

	system = PSys_Systems_free;
	PSys_Systems_free = PSys_Systems_free->next;

	memset( system, 0, sizeof( PSys_System_t ) );

	// don't forget to set up the inuse lists inside the system!
	system->particles.next_local = &(system->particles);
	system->particles.prev_local = &(system->particles);
	system->emitters.next_local = &(system->emitters);
	system->emitters.prev_local = &(system->emitters);
	system->forces.next_local = &(system->forces);
	system->forces.prev_local = &(system->forces);
	system->constraints.next_local = &(system->constraints);
	system->constraints.prev_local = &(system->constraints);

	// link into the active list
	system->next = PSys_Systems_inuse.next;
	system->prev = &PSys_Systems_inuse;
	PSys_Systems_inuse.next->prev = system;
	PSys_Systems_inuse.next = system;

	return system;
}


/*
-------------------------------------------

  I N T E G R A T I N G   T I M E S T E P

-------------------------------------------
*/

static qboolean PSys_UpdateOrientation( PSys_System_t *system, PSys_Orientation_t *orient ) {
	
	if ( orient->entity ) {

		// Check if the linked entity is not in the PVS	
		if (!CG_FrameHist_IsInPVS( orient->entity->currentState.number )) {
			return qfalse;
		}

		// Check if we should link against the aura and if it is still up and going
		if ( orient->linkAuraState && !CG_FrameHist_HasAura(orient->entity->currentState.number)) {
			return qfalse;
		}

		// Check if we should link against the weapon
		if ( orient->linkWeapon ) {
			// If we switched weapons
			if ( CG_FrameHist_IsWeaponNr( orient->entity->currentState.number ) != orient->weaponNr ) {
				return qfalse;
			}
			// If we switched weaponstate (aka stopped charging or stopped firing)
			if ( CG_FrameHist_IsWeaponState( orient->entity->currentState.number ) != orient->weaponState ) {
				return qfalse;
			}
		}

		if ( !CG_GetTagOrientationFromPlayerEntity( orient->entity, orient->tagName, &(orient->geometry)) ) {
			VectorCopy( orient->entity->lerpOrigin, orient->geometry.origin );
			AnglesToAxis( orient->entity->lerpAngles, orient->geometry.axis );
		}
	} else {
		VectorCopy( system->rootPos, orient->geometry.origin );
		VectorCopy( system->rootAxis[0], orient->geometry.axis[0] );
		VectorCopy( system->rootAxis[1], orient->geometry.axis[1] );
		VectorCopy( system->rootAxis[2], orient->geometry.axis[2] );
	}

	// Local offset
	VectorMA( orient->geometry.origin, orient->offset[0], orient->geometry.axis[0], orient->geometry.origin );
	VectorMA( orient->geometry.origin, orient->offset[1], orient->geometry.axis[1], orient->geometry.origin );
	VectorMA( orient->geometry.origin, orient->offset[2], orient->geometry.axis[2], orient->geometry.origin );

	// Possibly override the resulting direction with a new global direction
	if ( VectorLength( orient->dir ) > 0.0f ) {
		VectorNormalize2( orient->dir, orient->geometry.axis[0] );
		MakeNormalVectors( orient->geometry.axis[0], orient->geometry.axis[1], orient->geometry.axis[2] );
	}

	return qtrue;
}

static void PSys_UpdateForces( PSys_System_t *system ) {
	PSys_Force_t	*force, *next;

	force = system->forces.prev_local;
	for ( ; force != &(system->forces) ; force = next ) {
		next = force->prev_local;

		if ( !PSys_UpdateOrientation( system, &(force->orientation))) {
			// PSys_UpdateOrientation returned false, indicating a broken link,
			// meaning the force should be dropped
			PSys_FreeForce( force );
		}
	}
}

static void PSys_UpdateEmitters( PSys_System_t *system ) {
	PSys_Emitter_t	*emitter, *next;

	emitter = system->emitters.prev_local;
	for ( ; emitter != &(system->emitters) ; emitter = next ) {
		orientation_t	root;
		vec3_t			groundPoint;
		trace_t			trace;

		next = emitter->prev_local;

		// If there are no particle templates, just trash the emitter
		if ( emitter->nrTemplates == 0 ) {
			PSys_FreeEmitter( emitter );
			continue;
		}

		// Check if the emitter had a maximum life time, and if that lifetime has been exceeded proceed to destroy
		// the emitter.
		if ( emitter->lifeTime > 0 ) {
			if ((emitter->startTime + emitter->lifeTime) < cg.time ) {
				PSys_FreeEmitter( emitter );
				continue;
			}
		}

		// Updates emitter's position and direction
		if ( !PSys_UpdateOrientation( system, &(emitter->orientation))) {
			// PSys_UpdateOrientation returned false, indicating a broken link,
			// meaning the emitter should be dropped.
			PSys_FreeEmitter( emitter );
			continue;
		}

		// Check whether the emitter is still within its initialization period or not.
		if ( emitter->initTime > cg.time ) {
			// Make sure we don't build up more particles while waiting for the
			// initialization to end.
			emitter->lastTime = cg.time - emitter->waitTime;
			continue;
		}

		// Handle regular and surface emitters slightly differently
		if ( emitter->type < ETYPE_POINT_SURFACE ) {
			// Regular emitters
			VectorCopy( emitter->orientation.geometry.origin, root.origin );
			AxisCopy( emitter->orientation.geometry.axis, root.axis );
			trace.fraction = 0;
		} else {
			// Ground emitters
			VectorCopy( emitter->orientation.geometry.origin, groundPoint );
			groundPoint[2] -= emitter->grndDist;

			CG_Trace( &trace, emitter->orientation.geometry.origin, NULL, NULL, groundPoint, -1, CONTENTS_SOLID );
			if ( trace.allsolid || trace.startsolid ) {
				trace.fraction = 1.0f;
			}
			
			if ( trace.fraction < 1.0f ) {
				// Don't do these calculations when they're not necessary
				VectorMA( trace.endpos, 1, trace.plane.normal, root.origin ); // <-- Make sure we don't start exactly on the surface!
				VectorCopy( trace.plane.normal, root.axis[0] );
				MakeNormalVectors( root.axis[0], root.axis[1], root.axis[2] );
			} else {
				// Make sure we don't build up more and more particles while not connected to
				// the ground.
				emitter->lastTime = cg.time - emitter->waitTime;

				// If no waitTime is set, then we should kill the emitter if it can't spawn its set of particles
				// immediately.
				if ( emitter->waitTime == 0 ) {
					PSys_FreeEmitter( emitter );
					continue;
				}
			}			
		}

		while ((( emitter->lastTime + emitter->waitTime ) <= cg.time ) && ( trace.fraction < 1.0f )) {
			PSys_Particle_t *particle;
			vec3_t	jitVec, sphereVec;
			vec3_t	tempAxis[3];
			int		i, templateIndex;

			for ( i = 0; i < emitter->amount; i++ ) {
				particle = PSys_SpawnParticle( system );

				// Set starting point based on emitter type
				VectorSet( jitVec,
							(crandom() - crandom()) * emitter->posJit,
							(crandom() - crandom()) * emitter->posJit,
							(crandom() - crandom()) * emitter->posJit );

				switch ( emitter->type ) {
					case ETYPE_POINT:
					case ETYPE_POINT_SURFACE:
						VectorAdd( root.origin, jitVec, particle->position );
						break;

					case ETYPE_RADIUS:
					case ETYPE_RADIUS_SURFACE:
						AxisCopy( root.axis, tempAxis );

						// NOTE: This function takes deg, not rad
						RotateAroundDirection( tempAxis, crandom() * 360 );
						
						VectorMA( root.origin, emitter->radius, tempAxis[1], particle->position );
						VectorMA( particle->position, emitter->offset, root.axis[0], particle->position );
						VectorAdd( particle->position, jitVec, particle->position );
						break;

					case ETYPE_SPHERE:
						VectorSet( sphereVec, crandom() - crandom(), crandom() - crandom(), crandom() - crandom() );
						VectorNormalize( sphereVec );
						VectorMA( root.origin, emitter->radius, sphereVec, particle->position );
						VectorAdd( particle->position, jitVec, particle->position );						
						break;

					default:
						VectorCopy( root.origin, particle->position );
						break;
				}

				templateIndex = rand() % emitter->nrTemplates;

				// Set initial speed
				VectorMA( particle->position, -emitter->particleTemplates[templateIndex].speed, root.axis[0], particle->oldPosition );

				// Set other initial particle physics
				particle->lifeTime = emitter->particleTemplates[templateIndex].lifeTime;
				particle->spawnTime = cg.time;
				particle->mass = emitter->particleTemplates[templateIndex].mass;

				// If the particle has infinite aka zero mass, then the inverse mass must be zero.
				// Avoid division by zero error.
				if ( particle->mass ) {
					particle->invMass = 1 / particle->mass;
				} else {
					particle->invMass = 0;
				}

				// Assign a look to the particle
				particle->shader = emitter->particleTemplates[templateIndex].shader;
				particle->model = emitter->particleTemplates[templateIndex].model;
				memcpy( &(particle->scale), &(emitter->particleTemplates[templateIndex].scale), sizeof(PSys_FloatTimeLerp_t));
				memcpy( &(particle->rotate), &(emitter->particleTemplates[templateIndex].rotate), sizeof(PSys_Vec4TimeLerp_t));
				memcpy( &(particle->rgba), &(emitter->particleTemplates[templateIndex].rgba), sizeof(PSys_Vec4TimeLerp_t));
				
				// If the particle is a ray, and the emitter is not a ground type, set the point of origin as well.
				// Don't bother otherwise.
				if ( (particle->rType = emitter->particleTemplates[templateIndex].rType) == RTYPE_RAY ) {
					VectorCopy( particle->position, particle->rayOrigin );

					if ( emitter->type < ETYPE_POINT_SURFACE ) {
						particle->rayParent = emitter;						
						VectorSubtract( particle->rayOrigin, root.origin, particle->rayOffset );
					}
				}
			}

			emitter->lastTime += emitter->waitTime;

			// With no waitTime set, we only spawn one set of particles, then destroy the emitter
			if ( emitter->waitTime == 0 ) {
				PSys_FreeEmitter( emitter );
				break;
			}
		}
	}	
}

static void PSys_GetParticleVelocity( PSys_Particle_t *particle, vec3_t v ) {
	VectorCopy( particle->position, v );
	VectorSubtract( v, particle->oldPosition, v );
}

static void PSys_SetParticleVelocity( PSys_Particle_t *particle, vec3_t v ) {
	VectorCopy( particle->position, particle->oldPosition );
	VectorSubtract( particle->oldPosition, v, particle->oldPosition );
}

static void PSys_AccumulateSystem( PSys_System_t *system ) {
	PSys_Particle_t *particle, *next;

	particle = system->particles.prev_local;
	for ( ; particle != &(system->particles) ; particle = next ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next = particle->prev_local;

		// The best place to check the remaining lifetime of the particle is here, just before accumulation.
		if (( cg.time - particle->spawnTime ) >= particle->lifeTime ) {
			PSys_FreeParticle( particle );
			continue;
		}

		PSys_AccumulateParticle( system, particle );
	}
}


static float PSys_ApplyFalloff( PSys_Force_t *force, vec3_t particlePos, float sourceVal ) {
	vec3_t	distance;
	float	lerp;

	switch ( force->AOItype ) {
	case AOI_INFINITE:
		return sourceVal;
		break;

	case AOI_SPHERE:
		distance[0] = Distance( force->orientation.geometry.origin, particlePos );
		if ( force->falloff ) {
			lerp = 1 - ((distance[0] - force->AOIrange[0]) / force->falloff);
			if ( lerp < 0 ) lerp = 0;
			if ( lerp > 1 ) lerp = 1;
		} else {
			if ( distance[0] > force->AOIrange[0] ) lerp = 0; else lerp = 1;
		}
		return ( lerp * sourceVal );
		break;

	case AOI_CYLINDER:
		// Distance towards either of the horizontal axes is the vertical distance component
		distance[0] = DistancePointToLine( particlePos, force->orientation.geometry.origin, force->orientation.geometry.axis[1]);		
		distance[0] = distance[0] - force->AOIrange[0];
		if ( distance[0] < 0 ) distance[0] = 0;
		// Distance towards the vertical axis is the horizontal distance component (in the circle)
		distance[1] = DistancePointToLine( particlePos, force->orientation.geometry.origin, force->orientation.geometry.axis[0]);		
		distance[1] = distance[1] - force->AOIrange[1];
		if ( distance[1] < 0 ) distance[1] = 0;
		if ( force->falloff ) {
			distance[2] = sqrt( distance[0] * distance[0] + distance[1] * distance[1] );
			lerp = 1 - (distance[2] / force->falloff);
			if ( lerp < 0 ) lerp = 0;
			if ( lerp > 1 ) lerp = 1;
		} else {
			if ( distance[0] > force->AOIrange[0] || distance[1] > force->AOIrange[1] ) lerp = 0; else lerp = 1;
		}
		return ( lerp * sourceVal );
		break;

	default:
		break;
	}

	return sourceVal;
}

static void PSys_AccumulateParticle( PSys_System_t *system, PSys_Particle_t *particle ) {
	PSys_Force_t	*force, *next;
	vec3_t			sphereForce;
	vec3_t			dragForce;
	vec3_t			swirlForce, swirlOut;
	vec3_t			v;

	VectorAdd( particle->accelAccum, system->gravity, particle->accelAccum );

	force = system->forces.prev_local;
	for ( ; force != &(system->forces) ; force = next ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next = force->prev_local;

		switch ( force->type ) {
		case FTYPE_DIRECTIONAL:		
			VectorMA( particle->forceAccum, PSys_ApplyFalloff( force, particle->position, force->value ), force->orientation.geometry.axis[0], particle->forceAccum );
			break;

		case FTYPE_SPHERICAL:
			VectorSubtract( particle->position, force->orientation.geometry.origin, sphereForce );
			if ( VectorNormalize( sphereForce ) == 0.0f ) {
				// If the point is placed exactly on the point of force, shoot it upwards instead
				VectorSet( sphereForce, 0, 0, 1 );
			}
			
			VectorMA( particle->forceAccum, PSys_ApplyFalloff( force, particle->position, force->value ), sphereForce, particle->forceAccum );
			break;

		case FTYPE_DRAG:
			PSys_GetParticleVelocity( particle, v );
			VectorScale( v, PSys_ApplyFalloff( force, particle->position, force->value ) * -1, dragForce );
			VectorAdd( particle->forceAccum, dragForce, particle->forceAccum );
			break;

		case FTYPE_SWIRL:
			VectorSubtract( particle->position, force->orientation.geometry.origin, swirlOut );
			if ( VectorNormalize( swirlOut ) == 0.0f ) {
				// zero radius means we're at the 'center of the storm'
				break;
			}
			CrossProduct( force->orientation.geometry.axis[0], swirlOut, swirlForce );
			VectorNormalize( swirlForce );
			VectorMA( particle->forceAccum, PSys_ApplyFalloff( force, particle->position, force->value ), swirlForce, particle->forceAccum );
			VectorMA( particle->forceAccum, -1 * PSys_ApplyFalloff( force, particle->position, force->pullIn ), swirlOut, particle->forceAccum );
			break;

		default:
			// should never happen
			break;
		}
	}
}

static void PSys_IntegrateSystem( PSys_System_t *system, float timeStepSquare, float timeStepCorrected ) {
	PSys_Particle_t	*particle, *next;

	particle = system->particles.prev_local;
	for ( ; particle != &(system->particles) ; particle = next ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next = particle->prev_local;

		PSys_IntegrateParticle( particle, timeStepSquare, timeStepCorrected );
	}
}


static void PSys_IntegrateParticle( PSys_Particle_t *particle, float timeStepSquare, float timeStepCorrected ) {
	vec3_t vel;

	// Handle (infinite) mass
	VectorScale( particle->forceAccum, particle->invMass, particle->forceAccum );
	if ( particle->mass != 0 ) {
		VectorAdd( particle->forceAccum, particle->accelAccum, particle->forceAccum );
	}
	
	// Save the old position
	VectorCopy( particle->position, particle->accelAccum );


	// Timestep corrected Verlet integration:
	// xi+1 = xi + (xi - xi-1) * (dti / dti-1) + a * dti * dti

	VectorScale( particle->forceAccum, timeStepSquare, particle->forceAccum );
	VectorSubtract( particle->position, particle->oldPosition, vel );
	VectorScale( vel, timeStepCorrected, vel );
	VectorAdd( particle->position, vel, particle->position );
	VectorAdd( particle->position, particle->forceAccum, particle->position );
	
	// Restore the old position
	VectorCopy( particle->accelAccum, particle->oldPosition );

	// Reset the accumulators for the next frame
	VectorSet( particle->forceAccum, 0, 0, 0 );
	VectorSet( particle->accelAccum, 0, 0, 0 );


	// Update ray information if necessary
	if ( particle->rayParent ) {
		VectorAdd( particle->rayParent->orientation.geometry.origin, particle->rayOffset, particle->rayOrigin );
	}
}


static qboolean PSys_ConstrainSystem( PSys_System_t *system ) {
	PSys_Constraint_t *constraint, *next;
	qboolean retval;

	retval = qtrue;
	constraint = system->constraints.prev_local;
	for ( ; constraint != &(system->constraints) ; constraint = next ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next = constraint->prev_local;

		retval = retval && PSys_ApplyConstraint( system, constraint );
	}

	return retval;
}


static qboolean PSys_ApplyConstraint( PSys_System_t *system, PSys_Constraint_t *constraint ) {
	qboolean retval;

	switch( constraint->type ) {
	case CTYPE_DISTANCE_MAX:
		retval = PSys_ApplyDistanceMaxConstraint( system, constraint->value );
		break;
	case CTYPE_DISTANCE_MIN:
		retval = PSys_ApplyDistanceMinConstraint( system, constraint->value );
		break;
	case CTYPE_DISTANCE_EXACT:
		retval = PSys_ApplyDistanceConstraint( system, constraint->value );
		break;
	case CTYPE_PLANE:
		retval = PSys_ApplyPlaneConstraint( system, constraint->value );
		break;
	default:
		// Shouldn't happen
		retval = qtrue;
		break;
	}

	return retval;
}


static qboolean PSys_ApplyDistanceMaxConstraint( PSys_System_t *system, float value ) {
	PSys_Particle_t		*pt1, *pt2, *next1, *next2, *minDistPt;
	float				dist, tempDist;
	vec3_t				dir;
	qboolean			retval;	

	retval = qtrue;
	pt1 = system->particles.prev_local;
	for ( ; pt1 != &(system->particles) ; pt1 = next1 ) {
		next1 = pt1->prev_local;		

		// Determine shortest distance to another particle that has
		// not yet been affected by the constraint.
		// NOTE: This last bit is important! Otherwise the particles
		//       will form seperate clusters instead of one cluster!
		pt2 = next1;
		dist = -1; // Start with 'infinite' distance
		for ( ; pt2 != &(system->particles) ; pt2 = next2 ) {
			next2 = pt2->prev_local;
			
			if ( dist == -1 ) {
				dist = Distance( pt1->position, pt2->position );
				minDistPt = pt2;
			} else if ( dist > ( tempDist = Distance( pt1->position, pt2->position ))) {
				dist = tempDist;
				minDistPt = pt2;
			}			
		}

		// If the distance is still 'infinite', then there is only one particle in the system,
		// and it is always properly constrained.
		if ( dist == -1 ) return qtrue;

		// If the minimum distance to another particle in the system is greater than the
		// distance allowed by the constraint, ...
		if ( dist > value ) {
			VectorSubtract( pt1->position, minDistPt->position, dir );
			VectorNormalize( dir );
			// ... slide both half the distance overshoot closer together and ...
			dist = (dist - value) / 2.0f;
			VectorMA( pt1->position, -dist, dir, pt1->position );
			VectorMA( minDistPt->position, dist, dir, minDistPt->position );

			// ... report a constraint violation.
			retval = qfalse;
		}

	}
	
	return retval;
}

static qboolean PSys_ApplyDistanceMinConstraint( PSys_System_t *system, float value ) {
	PSys_Particle_t		*pt1, *pt2, *next1, *next2, *minDistPt;
	float				dist, tempDist;
	vec3_t				dir;
	qboolean			retval;	

	retval = qtrue;
	pt1 = system->particles.prev_local;
	for ( ; pt1 != &(system->particles) ; pt1 = next1 ) {
		next1 = pt1->prev_local;

		// Determine shortest distance to another particle that has
		// not yet been affected by the constraint.
		// NOTE: This last bit is important! Otherwise the particles
		//       will form seperate clusters instead of one cluster!
		pt2 = next1;
		dist = -1; // Start with 'infinite' distance
		for ( ; pt2 != &(system->particles) ; pt2 = next2 ) {
			next2 = pt2->prev_local;
			
			if ( dist == -1 ) {
				dist = Distance( pt1->position, pt2->position );
				minDistPt = pt2;
			} else if ( dist > ( tempDist = Distance( pt1->position, pt2->position ))) {
				dist = tempDist;
				minDistPt = pt2;
			}			
		}

		// If the distance is still 'infinite', then there is only one particle in the system,
		// and it is always properly constrained.
		if ( dist == -1 ) return qtrue;

		// If the minimum distance to another particle in the system is less than the
		// distance allowed by the constraint, ...
		if ( dist < value ) {
			VectorSubtract( pt1->position, minDistPt->position, dir );
			VectorNormalize( dir );
			// ... slide both half the distance overshoot closer together and ...
			dist = (dist - value) / 2.0f;
			VectorMA( pt1->position, dist, dir, pt1->position );
			VectorMA( minDistPt->position, -dist, dir, minDistPt->position );

			// ... report a constraint violation.
			retval = qfalse;
		}

	}
	
	return retval;
}

static qboolean PSys_ApplyDistanceConstraint( PSys_System_t *system, float value ) {
	PSys_Particle_t		*pt1, *pt2, *next1, *next2;
	float				dist;
	vec3_t				dir;
	qboolean			retval;	

	retval = qtrue;
	pt1 = system->particles.prev_local;
	for ( ; pt1 != &(system->particles) ; pt1 = next1 ) {
		next1 = pt1->prev_local;
		
		pt2 = next1;
		for ( ; pt2 != &(system->particles) ; pt2 = next2 ) {
			next2 = pt2->prev_local;
		
			// Determine distance between particles
			dist = Distance( pt1->position, pt2->position );
			
			// If distance doesn't match constraint
			if ( dist != value ) {
				VectorSubtract( pt1->position, pt2->position, dir );
				VectorNormalize( dir );
				// ... slide both half the distance overshoot closer together and ...
				dist = (dist - value) / 2.0f;
				VectorMA( pt1->position, dist, dir, pt1->position );
				VectorMA( pt2->position, -dist, dir, pt2->position );

				// ... report a constraint violation.
				retval = qfalse;
			}
		}
	}
	
	return retval;
}


static qboolean PSys_ApplyPlaneConstraint( PSys_System_t *system, float value ) {
	PSys_Particle_t	*particle, *next;
	qboolean		retval;
	trace_t			trace;

	retval = qtrue;
	particle = system->particles.prev_local;
	for ( ; particle != &(system->particles) ; particle = next ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next = particle->prev_local;

		// Find out if there were any collisions during the bit of movement the particle
		// experienced this frame.
		CG_Trace( &trace, particle->oldPosition, NULL, NULL, particle->position, -1, CONTENTS_SOLID );
		if ( trace.startsolid || trace.allsolid ) {
			// make sure the entityNum is set to the one we're stuck in
			CG_Trace( &trace, particle->position, NULL, NULL, particle->position, -1, CONTENTS_SOLID );
			trace.fraction = 0.0f;
		}

		if ( trace.surfaceFlags & SURF_NOIMPACT ) {
			PSys_FreeParticle( particle );
			continue;
		}

		// If the particle moved the whole frame without encountering a solid, then
		// everything is okay, else we have to do some math to bounce the particle.
		if ( trace.fraction < 1.0f ) {
			float dp;
			vec3_t v;

			// If we don't bounce at all, just remove the particle instead of having
			// it 'slide' along the ground indefinately.
			// FIXME: Should probably put in surface friction and make it slide to a halt...
			if ( value == 0.0f && cg_particlesStop.value) {
				PSys_FreeParticle( particle );
				continue;
			}

			// Get the velocity and bounce it
			PSys_GetParticleVelocity( particle, v );
			dp = DotProduct( trace.plane.normal, v );

			VectorMA( v, -2 * dp, trace.plane.normal, v );
			VectorScale( v, value, v );
			
			// check for stop
			if ( trace.plane.normal[2] > 0.2 && VectorLength( v ) < MIN_BOUNCE_DELTA) {

				if (VectorLength( v ) > 0.0 && VectorLength( v ) < 0.6){
					VectorSet(v, 0, 0, 0);
					particle->mass = 0.0f;
				}

				if ( cg_particlesStop.value ) {
					PSys_FreeParticle( particle );
					continue;
				}
			}

			// NOTE: Though a bit inaccurate, we have to perform this shift of the particle's
			//       position to prevent a trace.startsolid when re-evaluating constraints.
			VectorAdd( trace.endpos, trace.plane.normal, particle->position );
			VectorAdd( particle->position, v, particle->position );

			// Set the new velocity
			PSys_SetParticleVelocity( particle, v );

			retval = qfalse;
		}
	}

	return retval;
}





/*
-------------------------------

  M A I N   F U N C T I O N S

-------------------------------
*/

static void PSys_UpdateSystems( void ) {
	PSys_System_t	*system, *next;
	float			timeStep, timeStepSquare, timeStepCorrected;
	int				iterations;

	// Set the current timesteps
	timeStep = cg.frametime * 0.01f;
	timeStepSquare = timeStep * timeStep;
	if ( PSys_LastTimeStep == 0 ) {
		PSys_LastTimeStep = 0.01f; // A minimal is needed to avoid division by zero!
	}
	timeStepCorrected = timeStep / PSys_LastTimeStep;
	PSys_LastTimeStep = timeStep;

	system = PSys_Systems_inuse.prev;
	for ( ; system != &(PSys_Systems_inuse) ; system = next ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next = system->prev;

		// Update orientation of emitters and forces
		PSys_UpdateEmitters( system );
		PSys_UpdateForces( system );

		// Check if the system has any emitters or particles left.
		// If it doesn't, it can be destroyed.
		if ( (system->particles.prev_local == &(system->particles)) &&
			 (system->emitters.prev_local  == &(system->emitters)) ) {
			PSys_FreeSystem( system );
			continue;
		}
		
		// Accumulate forces and integrate new position
		PSys_AccumulateSystem( system );
		PSys_IntegrateSystem( system, timeStepSquare, timeStepCorrected );
		
		// Apply constraints
		iterations = 0;
		do {
			iterations++;
		} while (!PSys_ConstrainSystem( system ) && (iterations < MAX_ITERATIONS) );
		
	}
}


static void PSys_RenderSystems( void ) {
	PSys_System_t	*system, *next_s;
	PSys_Particle_t	*particle, *next_p;
	
	refEntity_t		ent;

	int				lifetime_end;
	int				lifetime_cur;

	float			lerp;
	vec4_t			lerpedRGBA;
	vec4_t			lerpedRotation;
	float			lerpedScale;
	
	static int		seed = 0x92;
	vec3_t			angles;

	system = PSys_Systems_inuse.prev;
	for ( ; system != &(PSys_Systems_inuse) ; system = next_s ) {
		// Grab next now, so if the entity is freed we still have the next one.
		next_s = system->prev;

		particle = system->particles.prev_local;
		for ( ; particle != &(system->particles) ; particle = next_p ) {
			// Grab next now, so if the entity is freed we still have the next one.
			next_p = particle->prev_local;

			lifetime_end = particle->lifeTime;
			lifetime_cur = cg.time - particle->spawnTime;
			
			lerpedScale = particle->scale.midVal;			
			lerpedRGBA[0] = particle->rgba.midVal[0];
			lerpedRGBA[1] = particle->rgba.midVal[1];
			lerpedRGBA[2] = particle->rgba.midVal[2];
			lerpedRGBA[3] = particle->rgba.midVal[3];

			lerpedRotation[0] = particle->rotate.midVal[0];
			lerpedRotation[1] = particle->rotate.midVal[1];
			lerpedRotation[2] = particle->rotate.midVal[2];
			lerpedRotation[3] = particle->rotate.midVal[3];

			// Lerp scale
			if ( lifetime_cur < particle->scale.startTime ) {
				if ( particle->scale.startTime != 0 ) {
					lerp = (float)lifetime_cur / (float)particle->scale.startTime;
				} else {
					lerp = 1;
				}
				if ( lerp < 0 ) lerp = 0;
				if ( lerp > 1 ) lerp = 1;
				lerpedScale = lerp * lerpedScale + particle->scale.startVal * (1.0f - lerp);

			} else if ( lifetime_cur > (lifetime_end - particle->scale.endTime)) {
				if ( particle->scale.endTime != 0 ) {
					lerp = (float)(lifetime_end - lifetime_cur) / (float)particle->scale.endTime;
				} else {
					lerp = 1;
				}
				if ( lerp < 0 ) lerp = 0;
				if ( lerp > 1 ) lerp = 1;
				lerpedScale = lerp * lerpedScale + particle->scale.endVal * (1.0f - lerp );
			}

			// Lerp rgba
			if ( lifetime_cur < particle->rgba.startTime ) {
				if ( particle->rgba.startTime != 0 ) {
					lerp = (float)lifetime_cur / (float)particle->rgba.startTime;
				} else {
					lerp = 1;
				}
				if ( lerp < 0 ) lerp = 0;
				if ( lerp > 1 ) lerp = 1;
				lerpedRGBA[0] = lerp * lerpedRGBA[0] + particle->rgba.startVal[0] * (1.0f - lerp);
				lerpedRGBA[1] = lerp * lerpedRGBA[1] + particle->rgba.startVal[1] * (1.0f - lerp);
				lerpedRGBA[2] = lerp * lerpedRGBA[2] + particle->rgba.startVal[2] * (1.0f - lerp);
				lerpedRGBA[3] = lerp * lerpedRGBA[3] + particle->rgba.startVal[3] * (1.0f - lerp);
			
			} else if ( lifetime_cur > (lifetime_end - particle->rgba.endTime)) {
				if ( particle->rgba.endTime != 0 ) {
					lerp = (float)(lifetime_end - lifetime_cur) / (float)particle->rgba.endTime;
				} else {
					lerp = 1;
				}
				if ( lerp < 0 ) lerp = 0;
				if ( lerp > 1 ) lerp = 1;
				lerpedRGBA[0] = lerp * lerpedRGBA[0] + particle->rgba.endVal[0] * (1.0f - lerp);
				lerpedRGBA[1] = lerp * lerpedRGBA[1] + particle->rgba.endVal[1] * (1.0f - lerp);
				lerpedRGBA[2] = lerp * lerpedRGBA[2] + particle->rgba.endVal[2] * (1.0f - lerp);
				lerpedRGBA[3] = lerp * lerpedRGBA[3] + particle->rgba.endVal[3] * (1.0f - lerp);
			}

			// Lerp rotation
			if ( lifetime_cur < particle->rotate.startTime ) {
				if ( particle->rotate.startTime != 0 ) {
					lerp = (float)lifetime_cur / (float)particle->rotate.startTime;
				} else {
					lerp = 1;
				}
				if ( lerp < 0 ) lerp = 0;
				if ( lerp > 1 ) lerp = 1;
				lerpedRotation[0] = lerp * lerpedRotation[0] + particle->rotate.startVal[0] * (1.0f - lerp);
				lerpedRotation[1] = lerp * lerpedRotation[1] + particle->rotate.startVal[1] * (1.0f - lerp);
				lerpedRotation[2] = lerp * lerpedRotation[2] + particle->rotate.startVal[2] * (1.0f - lerp);
				lerpedRotation[3] = lerp * lerpedRotation[3] + particle->rotate.startVal[3] * (1.0f - lerp);
			
			} else if ( lifetime_cur > (lifetime_end - particle->rotate.endTime)) {
				if ( particle->rotate.endTime != 0 ) {
					lerp = (float)(lifetime_end - lifetime_cur) / (float)particle->rotate.endTime;
				} else {
					lerp = 1;
				}
				if ( lerp < 0 ) lerp = 0;
				if ( lerp > 1 ) lerp = 1;
				lerpedRotation[0] = lerp * lerpedRotation[0] + particle->rotate.endVal[0] * (1.0f - lerp);
				lerpedRotation[1] = lerp * lerpedRotation[1] + particle->rotate.endVal[1] * (1.0f - lerp);
				lerpedRotation[2] = lerp * lerpedRotation[2] + particle->rotate.endVal[2] * (1.0f - lerp);
				lerpedRotation[3] = lerp * lerpedRotation[3] + particle->rotate.endVal[3] * (1.0f - lerp);
			}

			switch ( particle->rType ) {
			case RTYPE_DEFAULT:
				memset( &ent, 0, sizeof( ent ));
				VectorCopy( particle->position, ent.origin );

				if ( !particle->model ) {
					ent.reType = RT_SPRITE;
					ent.radius = lerpedScale;

					if (particle->oldPosition != particle->position){
						if ((lerpedRotation[0] || lerpedRotation[1] || lerpedRotation[2]) > 0){ 
							ent.rotation = particle->position[0];
						}
					}

				} else {
					ent.hModel = particle->model;
					ent.reType = RT_MODEL;

					AxisClear( ent.axis );

					if (particle->oldPosition != particle->position){
						if (lerpedRotation[0]){ 
							lerpedRotation[0] = particle->position[0]/* * lerpedRotation[0] / 100*/;
						}
						if (lerpedRotation[1]){ 
							lerpedRotation[1] = particle->position[1]/* * lerpedRotation[1] / 100*/;
						}
						if (lerpedRotation[2]){ 
							lerpedRotation[2] = particle->position[2]/* * lerpedRotation[2] / 100*/;
						}
					}

					VectorCopy( lerpedRotation, angles );
					AnglesToAxis( angles, ent.axis );

					ent.nonNormalizedAxes = qtrue;
					VectorScale( ent.axis[0], lerpedScale, ent.axis[0] );
					VectorScale( ent.axis[1], lerpedScale, ent.axis[1] );
					VectorScale( ent.axis[2], lerpedScale, ent.axis[2] );
				}

				ent.customShader = particle->shader;
				ent.shaderRGBA[0] = lerpedRGBA[0];
				ent.shaderRGBA[1] = lerpedRGBA[1];
				ent.shaderRGBA[2] = lerpedRGBA[2];
				ent.shaderRGBA[3] = lerpedRGBA[3];

				trap_R_AddRefEntityToScene( &ent );
				break;
			
			case RTYPE_SPARK:
				CG_DrawLineRGBA( particle->oldPosition, particle->position, lerpedScale, particle->shader, lerpedRGBA );
				break;

			case RTYPE_RAY:
				CG_DrawLineRGBA( particle->rayOrigin, particle->position, lerpedScale, particle->shader, lerpedRGBA );
				break;

			default:
				// should never happen
				break;
			}
		}
	}
}

void CG_AddParticleSystems( void ) {
	PSys_UpdateSystems();
	PSys_RenderSystems();
}


/*
========================
PSys_SpawnCachedSystem
========================
  Spawns a particle system from a cached script.
  if link auto is specified, then link state is determined by
  whether or not NULL was passed for cent and tagName.
  If NULL is passed for axis, then a normalized world-oriented
  set of axes will be used.
  
  IMPORTANT: AuraLink must only be enabled for player entities!
  It dictates whether or not the life of emitters and forces
  should be bound to the state of the entity's aura.
*/
void PSys_SpawnCachedSystem( char* systemName, vec3_t origin, vec3_t *axis,
							 centity_t *cent, char* tagName,
							 qboolean auraLink, qboolean weaponLink ) {
	PSys_SystemTemplate_t*	cache;
	PSys_System_t			*system;
	PSys_Force_t			*force;
	PSys_Constraint_t		*constraint;
	PSys_Emitter_t			*emitter;
	int						i;

	// Load from cache. Abort system's spawn if element not in cache
	cache = PSys_LoadSystemFromCache( systemName );
	if ( !cache ) {
		CG_Printf( S_COLOR_YELLOW "WARNING: '%s': can not find particle system\n", systemName );
		return;
	}

	// Spawn the system and set its parameters
	system = PSys_SpawnSystem();
		VectorCopy( origin, system->rootPos );
		if ( axis ) {
			AxisCopy( axis, system->rootAxis );
		} else {
			AxisClear( system->rootAxis );
		}
		VectorSet( system->gravity, 0, 0, -1 * cache->gravity );

	// Start copying in the members
	for ( i = 0; i < MAX_PARTICLESYSTEM_MEMBERS; i++ ) {
		// Place a stricter upper bound
		if ( cache->members[i].type == MEM_NONE ) {
			break;
		}

		switch ( cache->members[i].type ) {
		case MEM_EMITTER:
			emitter = PSys_SpawnEmitter( system );

			// IMPORTANT: This will copy all data on the emitter from the cache memory,
			//            excluding the linked list connections. Those must NEVER be included!!
			//
			//            This is provided the rules in cg_particlesystem.h regarding the
			//            PSys_Emitter_t structure have not been broken!
			memcpy( &(emitter->type), &(cache->members[i].data.emitter.type), sizeof(PSys_Emitter_t) - 4 * sizeof(PSys_Emitter_t*) - sizeof(PSys_System_t*));
			
			// Set spawn-time properties
			emitter->startTime = cg.time;
			emitter->initTime = cg.time + emitter->initTime;
			emitter->lastTime  = cg.time - emitter->waitTime;

			// Handle the various link states accordingly
			if ( emitter->orientation.autoLink ) {
				if ( cent ) {
					if ( tagName ) {
						Q_strncpyz( emitter->orientation.tagName, tagName, sizeof(emitter->orientation.tagName));
					}
					emitter->orientation.entity = cent;
				}
			} else if ( emitter->orientation.entityLink && cent ) {
				emitter->orientation.entity = cent;
			}

			if ( emitter->orientation.entity && auraLink ) {
					emitter->orientation.linkAuraState = qtrue;
			}

			if ( emitter->orientation.entity && weaponLink ) {
				emitter->orientation.linkWeapon = qtrue;
				emitter->orientation.weaponNr = cent->currentState.weapon;
				emitter->orientation.weaponState = cent->currentState.weaponstate;
			}

			break;

		case MEM_FORCE:
			force = PSys_SpawnForce( system );

			// IMPORTANT: This will copy all data on the force from the cache memory,
			//            excluding the linked list connections. Those must NEVER be included!!
			//
			//            This is provided the rules in cg_particlesystem.h regarding the
			//            PSys_Force_t structure have not been broken!
			memcpy( &(force->type), &(cache->members[i].data.force.type), sizeof(PSys_Force_t) - 4 * sizeof(PSys_Force_t*));
			
			// Handle the various link states accordingly
			if ( force->orientation.autoLink ) {
				if ( cent ) {
					if ( tagName ) {
						Q_strncpyz( force->orientation.tagName, tagName, sizeof(force->orientation.tagName));
					}
					force->orientation.entity = cent;
				}
			} else if ( force->orientation.entityLink && cent ) {
				force->orientation.entity = cent;
			}

			if ( force->orientation.entity && auraLink ) {
					force->orientation.linkAuraState = qtrue;
			}
			break;
		
		case MEM_CONSTRAINT:

			// IMPORTANT: This will copy all data on the constraint from the cache memory,
			//            excluding the linked list connections. Those must NEVER be included!!
			//
			//            This is provided the rules in cg_particlesystem.h regarding the
			//            PSys_Constraint_t structure have not been broken!
			constraint = PSys_SpawnConstraint( system );
			memcpy( &(constraint->type), &(cache->members[i].data.constraint.type), sizeof(PSys_Constraint_t) - 4 * sizeof(PSys_Constraint_t*));
			break;
		
		default:
			// should never happen
			break;
		}
	}
}
