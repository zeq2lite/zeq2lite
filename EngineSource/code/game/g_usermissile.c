#include "g_local.h"

#define	MIN_SKIM_NORMAL	0.5f

#define	MISSILE_PRESTEP_TIME	 50
#define SOLID_ADD				400  // Add extra to guide line to go behind solids with weapon
#define BEAM_SECTION_TIME		520  // Timeframe to spawn a new beam section

#define SKIM_MIN_GROUNDCLEARANCE	 5	// Amount the attack will 'hover' above ground
#define	SKIM_MAX_GROUNDCLEARANCE	25	// How much distance are we allowed to take to reach ground level again.
										// Used in a check for passing steep cliffs, etc. while skimming.

#define DEF_FORTITUDE	0.66


/*
   -------------------------------
     M I S C   F U N C T I O N S
   -------------------------------
*/


/*
=======================
GetMissileOwnerEntity
=======================
*/
gentity_t *GetMissileOwnerEntity (gentity_t *missile) {
	gentity_t *parent;

	parent = missile->parent;
	while ( parent->s.eType != ET_PLAYER ) {
		parent = GetMissileOwnerEntity( parent );
	}
	return parent;
}



/*
   ---------------------------------
     T H I N K   F U N C T I O N S
   ---------------------------------
*/




/*
===============
Think_Arch
===============
*/
void Think_Arch( gentity_t *self ) {

	// DUMMY FUNCTION UNTIL WE CAN FIGURE OUT HOW THE #$&^#$& HELL TO GET
	// THIS WORKING PROPERLY. 
	

	// If the weapon has existed too long, make the next think detonate it.
	if ( ( self->missileSpawnTime + self->maxMissileTime ) <= level.time) {
	  self->think = G_ExplodeUserWeapon;
	}
	self->nextthink = level.time + FRAMETIME;
}



/*
===============
Think_ProxDet
===============
*/
void Think_ProxDet( gentity_t *self ) {
	int			i; // loop variable
	gentity_t	*target_ent;
	gentity_t	*target_owner;
	vec3_t		midbody;
	float		proxDistance;	// The distance between a potential
								// target and the missile.

	for (i = 0; i < level.maxclients; i++) {
		// Here we use target_ent to point to potential targets
		target_ent = &g_entities[i];
		target_owner = GetMissileOwnerEntity( self );

		// We don't bother with non-used clients, ourselves,
		// or clients on our team.
		if ( !target_ent->inuse ) continue;
		if ( target_ent == target_owner ) continue;
		if ( OnSameTeam( target_ent, target_owner ) ) continue;

		// Judge the distance from the middle of the body, not the feet
		midbody[0] = target_ent->r.currentOrigin[0] + 
			(target_ent->r.mins[0] + target_ent->r.maxs[0]) * 0.5;
		midbody[1] = target_ent->r.currentOrigin[1] + 
			(target_ent->r.mins[1] + target_ent->r.maxs[1]) * 0.5;
		midbody[2] = target_ent->r.currentOrigin[2] + 
			(target_ent->r.mins[2] + target_ent->r.maxs[2]) * 0.5;

		proxDistance = Distance(midbody, self->r.currentOrigin);

		// We don't detonate on this entity if it's not within homing range.
		if ( proxDistance > self->homRange ) continue;

		
		// We're within range and have to detonate with the next think.
		self->think = G_ExplodeUserWeapon;

		// Force premature exit of Bounded Linear Search
		i = level.maxclients;
	}

	// If the weapon has existed too long, make the next think detonate it.
	if ( ( self->missileSpawnTime + self->maxMissileTime ) <= level.time) {
	  self->think = G_ExplodeUserWeapon;
	}
	self->nextthink = level.time + FRAMETIME;
}


/*
==============
Think_Guided
==============
*/
void Think_Guided (gentity_t *self) {
	vec3_t forward, right, up; 
	vec3_t muzzle;
	float dist;
	gentity_t *owner = GetMissileOwnerEntity(self);

	// If our weapon's owner can't be found, skip anything related to guiding.
	if ( !owner )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: Think_Guided reports unowned fired weapon!\n" );
	} else {
		// Calculate where we are now aiming.
		AngleVectors( owner->client->ps.viewangles, forward, right, up );
		CalcMuzzlePoint( owner, forward, right, up, muzzle );

		// Copy our starting position to the trajectory.
		VectorCopy( self->r.currentOrigin, self->s.pos.trBase );	
 
		// Setup the new directional vector for the fired weapon.
		VectorSubtract( muzzle, self->r.currentOrigin, muzzle );
		dist = VectorLength( muzzle ) + SOLID_ADD;
	
		VectorScale( forward, dist, forward );
		VectorAdd( forward, muzzle, muzzle );
		VectorNormalize( muzzle );

		// Set the weapon's direction and speed in the trajectory,
		// and set its angles.
		VectorScale( muzzle, self->speed, forward );
		VectorCopy( forward, self->s.pos.trDelta );
		vectoangles( muzzle, self->s.angles );

		// This will save net bandwidth.
		SnapVector( self->s.pos.trDelta );

		self->s.pos.trType = TR_LINEAR;
		self->s.pos.trTime = level.time; //- 50;
	}

	// If we're dealing with a beam, we have to record beamsections.
	if (self->s.eType == ET_BEAMHEAD) {

		// Have we reached the time at which to start a new section?
		if ( ( self->sectionSpawnTime + self->newSectionTime ) <= level.time ) {

			// FIXME: HOOKUP TO SERVERSIDE BEAMTABLES

			// Reset the timer for the drop of the next beamsection.
			self->sectionSpawnTime = level.time;
		}

	} // END OF BEAM SECTIONS

	// If the weapon has existed too long, make the next think detonate it.
	if ( ( self->missileSpawnTime + self->maxMissileTime ) <= level.time ) {
	  self->think = G_RemoveUserWeapon;
	}
	self->nextthink = level.time + FRAMETIME;
}


/*
==============
Think_Homing
==============
*/
void Think_Homing (gentity_t *self) {
	gentity_t	*target_ent, *target_owner;
	float		target_length;
	int			i;
	vec3_t		target_dir, forward, midbody;
	trace_t		tr;
	
	vec3_t		chosen_dir;
	float		chosen_length;

	// Best way to get forward vector for this rocket?
	VectorCopy(self->s.pos.trDelta, forward);
	VectorNormalize(forward);

	// Set the base position
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	self->s.pos.trTime = level.time;
	self->s.pos.trType = TR_LINEAR;

	// Set the first choice to an 'empty' one.
	VectorCopy(forward, chosen_dir);
	chosen_length = -1;

	// Cycle through the clients for a qualified target
	for (i = 0; i < level.maxclients; i++) {
		// Here we use target_ent to point to potential targets
		target_ent = &g_entities[i];
		target_owner = GetMissileOwnerEntity( self );

		if (!target_ent->inuse) continue;
		if (target_ent == target_owner) continue;
		if ( OnSameTeam( target_ent, target_owner ) ) continue;

		// Aim for the body, not the feet
		midbody[0] = target_ent->r.currentOrigin[0] + 
			(target_ent->r.mins[0] + target_ent->r.maxs[0]) * 0.5;
		midbody[1] = target_ent->r.currentOrigin[1] + 
			(target_ent->r.mins[1] + target_ent->r.maxs[1]) * 0.5;
		midbody[2] = target_ent->r.currentOrigin[2] + 
			(target_ent->r.mins[2] + target_ent->r.maxs[2]) * 0.5;

		// Get the distance
		VectorSubtract(midbody, self->r.currentOrigin, target_dir);
		target_length = VectorLength(target_dir);

		// We don't home in on this entity if its not within range.
		if ( target_length > self->homRange ) continue;

		// Quick normalization of target_dir since 
		// we already have the length
		target_dir[0] /= target_length;
		target_dir[1] /= target_length;
		target_dir[2] /= target_length;

		// We don't home in on this entity if its outside our view 'funnel' either.
		if ( DotProduct(forward, target_dir) < cos(DEG2RAD(self->homAngle)) ) continue;

		trap_Trace( &tr,  self->r.currentOrigin, NULL, NULL, 
			self->r.currentOrigin, ENTITYNUM_NONE, MASK_SHOT );
		if ( target_ent != &g_entities[tr.entityNum] ) continue;

		// Only pick this target if it's the closest to the missile,
		// but keep in account that there might not have been a pick
		// yet!
		if ( (target_length > chosen_length) && (chosen_length >= 0) ) continue;

		VectorCopy(target_dir, chosen_dir);
		chosen_length = target_length;
	}

	// Set the new direction.
	// FIXME: Does this remain the same if chosen_dir == forward? (aka no target found)
	VectorMA(forward, 0.05, chosen_dir, chosen_dir);
	VectorNormalize(chosen_dir);
	VectorScale(chosen_dir, self->speed, self->s.pos.trDelta);

	// If the weapon has existed too long, make the next think detonate it.
	if ( ( self->missileSpawnTime + self->maxMissileTime ) <= level.time ) {
	  self->think = G_ExplodeUserWeapon;
	}
	self->nextthink = level.time + FRAMETIME;
}


/*
======================
Think_CylinderHoming
======================
*/
void Think_CylinderHoming (gentity_t *self) {
	gentity_t	*target_ent, *target_owner;
	float		target_length;
	int			i;
	vec3_t		target_dir, forward, midbody;
	trace_t		tr;
	
	vec3_t		chosen_dir;
	float		chosen_length;

	// Best way to get forward vector for this rocket?
	VectorCopy(self->s.pos.trDelta, forward);
	VectorNormalize(forward);

	// Set the base position
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	self->s.pos.trTime = level.time;
	self->s.pos.trType = TR_LINEAR;

	// Set the first choice to an 'empty' one.
	VectorCopy(forward, chosen_dir);
	chosen_length = -1;

	// Cycle through the clients for a qualified target
	for (i = 0; i < level.maxclients; i++) {
		// Here we use target_ent to point to potential targets
		target_ent = &g_entities[i];
		target_owner = GetMissileOwnerEntity( self );

		if (!target_ent->inuse) continue;
		if (target_ent == target_owner) continue;
		if ( OnSameTeam( target_ent, target_owner ) ) continue;

		// Aim for the body, not the feet
		midbody[0] = target_ent->r.currentOrigin[0] + 
			(target_ent->r.mins[0] + target_ent->r.maxs[0]) * 0.5;
		midbody[1] = target_ent->r.currentOrigin[1] + 
			(target_ent->r.mins[1] + target_ent->r.maxs[1]) * 0.5;
		midbody[2] = target_ent->r.currentOrigin[2] + 
			(target_ent->r.mins[2] + target_ent->r.maxs[2]) * 0.5;

		// Get the distance on the horizontal plane
		VectorSubtract(midbody, self->r.currentOrigin, target_dir);
		target_dir[2] = 0; // <-- eliminate vertical difference here!
		target_length = VectorLength(target_dir);

		// We don't home in on this entity if its not within range on the
		// horizontal plane.
		if ( target_length > self->homRange ) continue;

		// We don't home in on this entity if it is 'higher up' than the missile is.
		if (midbody[2] > self->r.currentOrigin[2]) continue;

		// Now set the true direction, including the vertical offset!
		VectorSubtract(midbody, self->r.currentOrigin, target_dir);
		target_length = VectorLength(target_dir);

		// Quick normalization of target_dir since 
		// we already have the length
		target_dir[0] /= target_length;
		target_dir[1] /= target_length;
		target_dir[2] /= target_length;


		trap_Trace( &tr,  self->r.currentOrigin, NULL, NULL, 
			self->r.currentOrigin, ENTITYNUM_NONE, MASK_SHOT );
		if ( target_ent != &g_entities[tr.entityNum] ) continue;

		// Only pick this target if it's the closest to the missile,
		// but keep in account that there might not have been a pick
		// yet!
		if ( (target_length > chosen_length) && (chosen_length >= 0) ) continue;

		VectorCopy(target_dir, chosen_dir);
		chosen_length = target_length;
	}

	// Set the new direction.
	// FIXME: Does this remain the same if chosen_dir == forward? (aka no target found)
	VectorMA(forward, 0.05, chosen_dir, chosen_dir);
	VectorNormalize(chosen_dir);
	VectorScale(chosen_dir, self->speed, self->s.pos.trDelta);

	// If the weapon has existed too long, make the next think detonate it.
	if ( ( self->missileSpawnTime + self->maxMissileTime ) <= level.time ) {
	  self->think = G_ExplodeUserWeapon;
	}
	self->nextthink = level.time + FRAMETIME;
}


/*
=====================
Think_NormalMissile
=====================
*/
void Think_NormalMissile (gentity_t *self) {
	// This is just a normal missile, so we only have to check if it's
	// existed too long or not.

	// If the weapon has existed too long, make the next think detonate it.
	if ( ( self->missileSpawnTime + self->maxMissileTime ) <= level.time ) {
	  self->think = G_ExplodeUserWeapon;
	}
	self->nextthink = level.time + FRAMETIME;
}



/*
   -----------------------------------
     D A M A G E   F U N C T I O N S
   -----------------------------------
*/

int CheckFortitude (gentity_t *ent, int damage, int dflags) {
	gclient_t	*client;
	int			save;
	int			count;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client) {
		return 0;
	}

	if (dflags & DAMAGE_NO_ARMOR) {
		return 0;
	}

	// PL will determine your fortitude
	count = client->ps.stats[STAT_PL] & ~PL_CHANGEBITS;
	// Amount saved will be 2/3 of the PL-determined percentage of the damage
	save = ceil( damage * DEF_FORTITUDE * count / 100.0f );

	return save;
}

/*
====================
G_UserWeaponDamage
====================
Does damage to a client or object
target = entity to be damaged
inflictor = entity doing the damage
attacker = entity the inflictor belongs to
dir = direction for knockback
point = origin of attack
*/
void G_UserWeaponDamage( gentity_t *target, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int methodOfDeath, int extraKnockback ) {
	gclient_t	*tgClient;
	int			take;
	int			save;
	int			asave;
	int			knockback;


	if (!target->takedamage) {
		return;
	}

	// the intermission has already been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	// FIXME: Eventually disable for ZEQ2, or have to keep for func_breakable?
	if ( target->s.eType == ET_MOVER ) {
		if ( target->use && target->moverState == MOVER_POS1 ) {
			target->use( target, inflictor, attacker );
		}
		return;
	}

	
	tgClient = target->client;

	if ( tgClient ) {
		if ( tgClient->noclip ) {
			return;
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize( dir );
	}

	
	if ( inflictor == &g_entities[ENTITYNUM_WORLD] ) {
		knockback = damage;
	} else {
		knockback = damage + extraKnockback;
	}
	/*
	if ( knockback > 200 ) {
		knockback = 200;
	}
	*/
	// Don't allow more knockback than this (though it's already very high)
	if ( knockback > 5000 ) {
		knockback = 5000;
	}
	if ( knockback < 0 ) {
		knockback = 0;
	}
	if ( target->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && tgClient ) {
		vec3_t	kvel;
		float	mass;

		mass = 200;

		VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		VectorAdd (tgClient->ps.velocity, kvel, tgClient->ps.velocity);

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !tgClient->ps.pm_time ) {
			int		t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			tgClient->ps.pm_time = t;
			tgClient->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the targetet
		// if the attacker was on the same team
		if ( target != attacker && OnSameTeam (target, attacker)  ) {
			if ( !g_friendlyFire.integer ) {
				return;
			}
		}

		// check for godmode
		if ( target->flags & FL_GODMODE ) {
			return;
		}
	}


	// add to the attacker's hit counter (if the targetet isn't a general entity like a prox mine)
	if ( attacker->client && target != attacker && target->health > 0
			&& target->s.eType != ET_MISSILE
			&& target->s.eType != ET_GENERAL) {
		if ( OnSameTeam( target, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS]--;
		} else {
			attacker->client->ps.persistant[PERS_HITS]++;
		}
		//attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (target->health<<8)|(client->ps.stats[STAT_ARMOR]);
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (target->health<<8)|(tgClient->ps.stats[STAT_PL] & ~PL_CHANGEBITS);
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( target == attacker) {
		damage *= 0.5;
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;
	save = 0;

	// save some from armor
	asave = CheckFortitude (target, take, dflags);
	take -= asave;

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( tgClient ) {
		if ( attacker ) {
			tgClient->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			tgClient->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		//tgClient->damage_armor += asave;
		tgClient->damage_blood += take;
		tgClient->damage_knockback += knockback;
		if ( dir ) {
			VectorCopy ( dir, tgClient->damage_from );
			tgClient->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( target->r.currentOrigin, tgClient->damage_from );
			tgClient->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	if( g_gametype.integer == GT_CTF) {
		Team_CheckHurtCarrier(target, attacker);
	}

	if (tgClient) {
		// set the last client who damaged the target
		tgClient->lasthurt_client = attacker->s.number;
		tgClient->lasthurt_mod = methodOfDeath;
	}

	// do the damage
	if (take) {
		target->health = target->health - take;
		if ( tgClient ) {
			tgClient->ps.stats[STAT_HEALTH] = target->health;
		}
			
		if ( target->health <= 0 ) {
			if ( tgClient )
				target->flags |= FL_NO_KNOCKBACK;

			if (target->health < -999)
				target->health = -999;

			target->enemy = attacker;
			target->die (target, inflictor, attacker, take, methodOfDeath);
			return;
		} else if ( target->pain ) {
			target->pain (target, attacker, take);
		}
	}

}



/*
=====================
G_UserRadiusDamage
=====================
*/
qboolean G_UserRadiusDamage ( vec3_t origin, gentity_t *attacker, gentity_t *ignore, float centerDamage, float radius, int methodOfDeath, int extraKnockback ) {
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	
	float		realDamage, distance;
	gentity_t	*ent;
	
	vec3_t		mins, maxs;
	vec3_t		deltaRadius;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		
		// Find the distance between the perimeter of the entities' bounding box
		// and the explosion's origin.
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				deltaRadius[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				deltaRadius[i] = origin[i] - ent->r.absmax[i];
			} else {
				deltaRadius[i] = 0;
			}
		}

		distance = VectorLength( deltaRadius );
		if ( distance >= radius ) {
			continue;
		}

		realDamage = centerDamage * ( 1.0 - distance / radius );

		if( CanDamage (ent, origin) ) {
			if( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_UserWeaponDamage( ent, NULL, attacker, dir, origin, (int)realDamage, DAMAGE_RADIUS, methodOfDeath, extraKnockback );
		}
	}

	return hitClient;
}


/*
   -----------------------------------
     F I R I N G   F U N C T I O N S
   -----------------------------------
*/


void UserHitscan_Fire (gentity_t *self, g_userWeapon_t *weaponInfo, int weaponNum ) {
	trace_t		tr;

	// muzzle settings that need to be retrieved from g_weapons.c
	vec3_t		muzzle, forward, right, up;
	
	float		rndH, rndV;
	float		upScale, rightScale;
	vec3_t		end;

	gentity_t	*tempEnt;
	gentity_t	*tempEnt2;
	gentity_t	*traceEnt;
	int			i, passent;

	float		spreadHor, spreadVer, range;

#ifdef MISSIONPACK
	vec3_t		impactpoint, bouncedir;
#endif

	// Retrieve the muzzle that was set in g_weapons.c
	G_GetMuzzleSettings(muzzle, forward, right, up);

	// Set some shortcuts
	spreadHor = weaponInfo->firing_deviateW;
	spreadVer = weaponInfo->firing_deviateH;
	range     = weaponInfo->physics_range;


	// Calculate a random position within the spread cone
	rndH = (2 * random() - 1) * spreadHor;
	rndV = (2 * random() - 1) * spreadVer;
	upScale    = sin(DEG2RAD(rndH)) * range;
	rightScale = sin(DEG2RAD(rndV)) * range;
	VectorMA (muzzle, range, forward, end);
	VectorMA (end, rightScale, right, end);
	VectorMA (end, upScale, up, end);

	// Set the player to not be able of being hurt by this shot
	passent = self->s.number;


	// Note; if bounce is enabled, this will let the projectile bounce a max. of 10 times
	for (i = 0; i < 10; i++) {

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}

		traceEnt = &g_entities[ tr.entityNum ];
		// snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( tr.endpos, muzzle );


		// Add an explosion event entity, and make it free itself after the event.
		tempEnt = G_Spawn();
		tempEnt->s.eType = ET_GENERAL;
		tempEnt->s.clientNum = self->s.number;
		tempEnt->s.weapon = weaponNum;
		SnapVectorTowards( tr.endpos, tempEnt->s.pos.trBase ); // save net bandwidth
		G_SetOrigin( tempEnt, tr.endpos );

		if ( traceEnt->takedamage && traceEnt->client ) {
			G_AddEvent( tempEnt, EV_MISSILE_HIT, DirToByte( tr.plane.normal ) );
			tempEnt->s.otherEntityNum = traceEnt->s.number;
			
			// Log an accuracy hit for this weapon
			if( LogAccuracyHit( traceEnt, self ) ) {
				self->client->accuracy_hits++;
			}

		} else if( tr.fraction == 1.0f ) {
			G_AddEvent( tempEnt, EV_MISSILE_MISS_AIR, DirToByte( tr.plane.normal ) );
		} else if( tr.surfaceFlags & SURF_METALSTEPS ) {
			G_AddEvent( tempEnt, EV_MISSILE_MISS_METAL, DirToByte( tr.plane.normal ) );
		} else  {
			G_AddEvent( tempEnt, EV_MISSILE_MISS, DirToByte( tr.plane.normal ) );
		}
		tempEnt->freeAfterEvent = qtrue;
		trap_LinkEntity( tempEnt );

		// the final trace endpos will be the terminal point of the rail trail


		// Send railgun beam effect. The client will draw it only if set through the 
		// weapon's scripted graphics clientside, but we _always_ have to send the event.
		tempEnt2 = G_TempEntity( tr.endpos, EV_RAILTRAIL );
		tempEnt2->s.clientNum = self->s.number;
		tempEnt2->s.weapon = weaponNum;
		VectorCopy( muzzle, tempEnt2->s.origin2 );
		

		if ( traceEnt->takedamage) {

/*
#ifdef MISSIONPACK
			if ( traceEnt->client && traceEnt->client->invulnerabilityTime > level.time ) {
				if (G_InvulnerabilityEffect( traceEnt, forward, tr.endpos, impactpoint, bouncedir )) {
					G_BounceProjectile( muzzle, impactpoint, bouncedir, end );
					VectorCopy( impactpoint, muzzle );
					// the player can hit him/herself with the bounced rail
					passent = ENTITYNUM_NONE;
				}
				else {
					VectorCopy( tr.endpos, muzzle );
					passent = traceEnt->s.number;
				}
				continue;
			}
			else {
#endif
*/
		
		G_UserWeaponDamage( traceEnt, self, self, forward, tr.endpos,
							weaponInfo->damage_damage, 0,
							MOD_KI + weaponInfo->damage_meansOfDeath,
							weaponInfo->damage_extraKnockback );

/*
#ifdef MISSIONPACK
			}
#endif
*/
		}
		break;
	}
}


/*
=================
Fire_UserWeapon
=================
*/
void Fire_UserWeapon (gentity_t *self, vec3_t start, vec3_t dir, qboolean altfire) {
	gentity_t		*bolt;
	g_userWeapon_t	*weaponInfo;
	vec3_t			muzzle, forward, right, up;

	// Get a hold of the weapon we're firing.
	// If altfire is used and it exists, use altfire, else use regular fire
	// Due to the nature of altfire presence detection, the regular fire must
	// always be loaded first. Since this is only one pointer dereference it is
	// negligable.
	weaponInfo = G_FindUserWeaponData( self->s.clientNum, self->s.weapon );
	if ( ( weaponInfo->general_bitflags & WPF_ALTWEAPONPRESENT ) && altfire ) {
		weaponInfo = G_FindUserAltWeaponData( self->s.clientNum, self->s.weapon );
	}
		

	// Retrieve the muzzle that was set in g_weapons.c
	G_GetMuzzleSettings(muzzle, forward, right, up);

	// Take the necessary steps to configure and fire the weapon.
	switch ( weaponInfo->general_type ) {

	/* MISSILES */

	case WPT_MISSILE:

		bolt = G_Spawn();
		bolt->classname = "rocket";
		bolt->s.eType = ET_MISSILE;

		bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;

		// Set the weapon number correct, depending on altfire status.
		if ( !altfire ) {
			bolt->s.weapon = self->s.weapon;
		} else {
			bolt->s.weapon = self->s.weapon + ALTWEAPON_OFFSET;
		}

		bolt->r.ownerNum = self->s.number;
		bolt->s.clientNum = self->s.number;	// <-- clientNum is free on all but ET_PLAYER so
		bolt->parent = self;				//     use it to store the owner for clientside.
		
		bolt->damage = weaponInfo->damage_damage;
		bolt->splashRadius = weaponInfo->damage_radius;
		bolt->extraKnockback = weaponInfo->damage_extraKnockback;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[STAT_CHARGELVL_SEC];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[STAT_CHARGELVL_SEC] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[STAT_CHARGELVL_PRI];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[STAT_CHARGELVL_PRI] = 0; // Only reset it here!
		}
		
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;

		bolt->speed = weaponInfo->physics_speed;
		bolt->accel = weaponInfo->physics_acceleration;
		bolt->bounceFrac = weaponInfo->physics_bounceFrac;
		bolt->bouncesLeft = weaponInfo->physics_maxBounces;
		
		// Configure the type correctly if we're using gravity or acceleration
		// Gravity affection takes precedence over acceleration.
		// FIXME: Can we get both simultaneously?
		if (weaponInfo->physics_gravity > 0) {
			bolt->gravity = (weaponInfo->physics_gravity / 100) * g_gravity.value; 
			bolt->s.pos.trType = TR_MAPGRAVITY;
			bolt->s.pos.trDuration = bolt->gravity;
		} else {
			if (bolt->accel > 0) {
				bolt->s.pos.trType = TR_ACCEL;
				bolt->s.pos.trDuration = bolt->accel;
			} else {
				bolt->s.pos.trType = TR_LINEAR;
			}
		}
		
		// move a bit on the very first frame
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( start, bolt->s.pos.trBase );
		if ( ( self->client->ps.stats[STAT_BITFLAGS] & STATBIT_FLIPOFFSETW ) && weaponInfo->firing_offsetWFlip ) {
			VectorMA( bolt->s.pos.trBase, -weaponInfo->firing_offsetW, right, bolt->s.pos.trBase );
		} else {
			VectorMA( bolt->s.pos.trBase, weaponInfo->firing_offsetW, right, bolt->s.pos.trBase );
		}
		VectorMA( bolt->s.pos.trBase, weaponInfo->firing_offsetH, up, bolt->s.pos.trBase );
		VectorCopy( start, bolt->r.currentOrigin );

		VectorCopy( self->client->ps.viewangles, self->s.angles );


/*
		// If an override direction was specified, take it.
		if ( (weaponInfo->overrideDir[0] > 0) ||
			 (weaponInfo->overrideDir[1] > 0) ||
			 (weaponInfo->overrideDir[2] > 0)   ) {
			VectorCopy( weaponInfo->overrideDir, dir );
			VectorNormalize( dir );
		}
*/
		
		VectorScale( dir, bolt->speed, bolt->s.pos.trDelta );
		// This saves network bandwidth.
		SnapVector( bolt->s.pos.trDelta );

		// Set the correct think based on homing properties.
		switch (weaponInfo->homing_type) {
		case HOM_PROX:
			bolt->think = Think_ProxDet;
			bolt->nextthink = level.time + FRAMETIME;
			bolt->homRange = weaponInfo->homing_range;
			break;

		case HOM_GUIDED:
			bolt->think = Think_Guided;
			bolt->nextthink = level.time + FRAMETIME;
			
			// Reference back to the client for motion tracking.
			// bolt->s.otherEntityNum = self->s.number;
			// FIXME: This is now implicitly present in bolt->s.clientNum for
			//        displaying the correct graphics clientside.
			
			// Kill off previously set accel, bounce and gravity. Guided missiles
			// do not accelerate, bounce or experience gravity.
			bolt->accel = 0;
			bolt->bounceFrac = 0;
			bolt->bouncesLeft = 0;
			bolt->gravity = 0;

			// Set guidetarget.
			bolt->s.eFlags |= EF_GUIDED;
			self->client->guidetarget = bolt;
			
			// Set the player's weapon state to guiding and forcibly disable the
			// dash.
			self->client->ps.weaponstate = WEAPON_GUIDING;
			bolt->guided = qtrue;
			break;

		case HOM_REGULAR:
			bolt->think = Think_Homing;
			bolt->nextthink = level.time + FRAMETIME;
			bolt->homRange = weaponInfo->homing_range;
			//bolt->homAccel = weaponInfo->homing_accel;
			break;

		case HOM_CYLINDER:
			bolt->think = Think_CylinderHoming;
			bolt->nextthink = level.time + FRAMETIME;
			bolt->homRange = weaponInfo->homing_range;
			//bolt->homAccel = weaponInfo->homing_accel;
			break;

		case HOM_ARCH:
			bolt->think = Think_NormalMissile;
			bolt->nextthink = level.time + FRAMETIME;

			bolt->bounceFrac = 0;
			bolt->bouncesLeft = 0;
			bolt->accel = 0;
			bolt->gravity = 0;

			{ // Open new block to get some local variables in here
				trace_t	trace;
				vec3_t  begin, mid, end, aim, temp;
				float	length;

				VectorCopy( bolt->s.pos.trBase, begin );
				VectorMA( begin, weaponInfo->homing_range, dir, end );
				trap_Trace( &trace, begin, NULL, NULL, end, self->s.number, MASK_SHOT );
				VectorCopy( trace.endpos, end );

				length = Distance( begin, end );

				VectorCopy( dir, aim);
				if ( weaponInfo->homing_angleW ) {
					if ( ( self->client->ps.stats[STAT_BITFLAGS] & STATBIT_FLIPOFFSETW ) && weaponInfo->firing_offsetWFlip ) {
						RotatePointAroundVector( temp, up, aim, weaponInfo->homing_angleW );
					} else {
						RotatePointAroundVector( temp, up, aim, -weaponInfo->homing_angleW );
					}
					VectorCopy( temp, aim );
				}
				if ( weaponInfo->homing_angleH ) {
					RotatePointAroundVector( temp, right, aim, weaponInfo->homing_angleH );
					VectorCopy( temp, aim );
				}

				VectorMA( begin, length / 2.0f, aim, mid );


				bolt->s.pos.trType = TR_ARCH;
				bolt->s.pos.trTime = level.time;
				bolt->s.pos.trDuration = 1000.0f * length / weaponInfo->physics_speed;
				VectorCopy( begin, bolt->s.pos.trBase );
				VectorCopy( mid, bolt->s.angles2 );
				VectorCopy( end, bolt->s.pos.trDelta );
			}
			break;
		
		case HOM_NONE:
			bolt->think = Think_NormalMissile;
			bolt->nextthink = level.time + FRAMETIME;
			break;

		default:
			// This should never happen!
			break;
		}

		// Set the lifespan of the weapon.
		bolt->missileSpawnTime = level.time;
		bolt->maxMissileTime = weaponInfo->physics_lifetime;
		break; // END OF MISSILE
	
		
	/* BEAMS */

	case WPT_BEAM:

		bolt = G_Spawn();
		bolt->classname = "user_beam";
		bolt->s.eType = ET_BEAMHEAD;
		bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;

		// Set the weapon number correct, depending on altfire status.
		if ( !altfire ) {
			bolt->s.weapon = self->s.weapon;
		} else {
			bolt->s.weapon = self->s.weapon + ALTWEAPON_OFFSET;
		}

		bolt->r.ownerNum = self->s.number;
		bolt->s.clientNum = self->s.number;	// <-- clientNum is free on all but ET_PLAYER so
		bolt->parent = self;				//     use it to store the owner for clientside.
		VectorCopy( self->client->ps.viewangles, bolt->s.angles ); // store the correct angles
		
		bolt->damage = weaponInfo->damage_damage;
		bolt->splashRadius = weaponInfo->damage_radius;
		bolt->extraKnockback = weaponInfo->damage_extraKnockback;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[STAT_CHARGELVL_SEC];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[STAT_CHARGELVL_SEC] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[STAT_CHARGELVL_PRI];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[STAT_CHARGELVL_PRI] = 0; // Only reset it here!
		}
		
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
				
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;

		bolt->speed = weaponInfo->physics_speed;
		bolt->accel = 0;
		bolt->bounceFrac = 0;
		bolt->bouncesLeft = 0;
		bolt->gravity = 0;
		bolt->s.pos.trType = TR_LINEAR;
		
/*
		// If an override direction was specified, take it.
		if ( (weaponInfo->overrideDir[0] > 0) ||
			 (weaponInfo->overrideDir[1] > 0) ||
			 (weaponInfo->overrideDir[2] > 0)   ) {
			VectorCopy( weaponInfo->overrideDir, dir );
			VectorNormalize( dir );
		}
*/

		VectorScale( dir, bolt->speed, bolt->s.pos.trDelta );
		// This saves network bandwidth.
		SnapVector( bolt->s.pos.trDelta );
		// move a bit on the very first frame
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( start, bolt->s.pos.trBase );
		VectorCopy( start, bolt->r.currentOrigin );

		VectorCopy( self->client->ps.viewangles, self->s.angles );

		// Set the correct think based on homing properties.
		switch (weaponInfo->homing_type) {
		case HOM_GUIDED:
			bolt->think = Think_Guided;
			bolt->nextthink = level.time + FRAMETIME;

			// Set guidetarget.
			bolt->s.eFlags |= EF_GUIDED;
			self->client->guidetarget = bolt;

			// Set the player's weapon state to guiding and forcibly disable the
			// dash.
			self->client->ps.weaponstate = WEAPON_GUIDING;
			bolt->guided = qtrue;
			break;

		case HOM_ARCH:
		case HOM_PROX:			
		case HOM_REGULAR:
		case HOM_CYLINDER:
		case HOM_NONE:
			bolt->think = Think_NormalMissile;
			bolt->nextthink = level.time + FRAMETIME;

			// Set guidetarget.
			bolt->s.eFlags |= EF_GUIDED;
			self->client->guidetarget = bolt;

			// Set the player's weapon state to guiding and forcibly disable the
			// dash.
			self->client->ps.weaponstate = WEAPON_GUIDING;
			bolt->guided = qtrue;			
			break;

		default:
			// This should never happen!
			break;
		}

		// Set the lifespan of the weapon.
		bolt->missileSpawnTime = level.time;
		bolt->maxMissileTime = weaponInfo->physics_lifetime;
		break; // END OF BEAM


	/* HITSCAN */

	case WPT_HITSCAN:
		if ( !altfire ) {
			UserHitscan_Fire( self, weaponInfo, self->s.weapon );
		} else {
			UserHitscan_Fire( self, weaponInfo, self->s.weapon + ALTWEAPON_OFFSET );
		}
		break;

	
	/* GROUNDSKIMMER */

	case WPT_GROUNDSKIM:

		bolt = G_Spawn();
		bolt->classname = "user_groundskim";
		bolt->s.eType = ET_SKIMMER;
		bolt->onGround = qfalse;

		bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;

		// Set the weapon number correct, depending on altfire status.
		if ( !altfire ) {
			bolt->s.weapon = self->s.weapon;
		} else {
			bolt->s.weapon = self->s.weapon + ALTWEAPON_OFFSET;
		}

		bolt->r.ownerNum = self->s.number;
		bolt->s.clientNum = self->s.number;	// <-- clientNum is free on all but ET_PLAYER so
		bolt->parent = self;				//     use it to store the owner for clientside.
		
		bolt->damage = weaponInfo->damage_damage;
		bolt->splashRadius = weaponInfo->damage_radius;
		bolt->extraKnockback = weaponInfo->damage_extraKnockback;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[STAT_CHARGELVL_SEC];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[STAT_CHARGELVL_SEC] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[STAT_CHARGELVL_PRI];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[STAT_CHARGELVL_PRI] = 0; // Only reset it here!
		}
		
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;

		bolt->speed = weaponInfo->physics_speed;
		bolt->accel = 0;
		bolt->s.pos.trType = TR_LINEAR;
		
/*
		// If an override direction was specified, take it.
		if ( (weaponInfo->overrideDir[0] > 0) ||
			 (weaponInfo->overrideDir[1] > 0) ||
			 (weaponInfo->overrideDir[2] > 0)   ) {
			VectorCopy( weaponInfo->overrideDir, dir );
			VectorNormalize( dir );
		}
*/

		VectorScale( dir, bolt->speed, bolt->s.pos.trDelta );
		// This saves network bandwidth.
		SnapVector( bolt->s.pos.trDelta );
		// move a bit on the very first frame
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( start, bolt->s.pos.trBase );
		VectorCopy( start, bolt->r.currentOrigin );

		// Set the correct think, which for a groundskim is only regular think. 
		bolt->think = Think_NormalMissile;
		bolt->nextthink = level.time + FRAMETIME;

		// Set the lifespan of the weapon.
		bolt->missileSpawnTime = level.time;
		bolt->maxMissileTime = weaponInfo->physics_lifetime;
		break; // END OF MISSILE


	case WPT_FORCEFIELD:
//		bolt = G_Spawn();
//		bolt->classname = "user_forcefield";
//		bolt->s.eType = ET_FORCEFIELD;
		break;


	case WPT_TORCH:
//		bolt = G_Spawn();
//		bolt->classname = "user_torch";
//		bolt->s.eType = ET_TORCH;
		break;


	case WPT_NONE:
		// We don't need to generate any kind of attack
		break;
	default:
		// Something is not defined right, so don't fire anything as a precaution
		break;
	}
}



/*
   -----------------------------------------
     E X P L O S I O N   F U N C T I O N S
   -----------------------------------------
*/

void G_ExplodeUserWeapon (gentity_t *self) {
	// Handles actual detonation of the weapon in question.

	vec3_t		dir = { 0, 0, 1};
	vec3_t		origin;

	// Terminate guidance
	if ( self->guided ) {
		g_entities[ self->s.clientNum ].client->ps.weaponstate = WEAPON_READY;
	}
	
	// Calculate current position
	BG_EvaluateTrajectory( &self->s, &self->s.pos, level.time, origin );
	SnapVector( origin ); // save net bandwith
	SnapVectorTowards( origin, self->s.pos.trBase ); // save net bandwidth
	G_SetOrigin( self, origin );

	self->s.eType = ET_GENERAL;
	G_AddEvent( self, EV_MISSILE_MISS_AIR, DirToByte( dir ) );
	self->freeAfterEvent = qtrue;


	trap_LinkEntity( self );
}



static void G_BounceUserMissile( gentity_t *self, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &self->s, &self->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, self->s.pos.trDelta );

	VectorScale( self->s.pos.trDelta, self->bounceFrac, self->s.pos.trDelta );
	
	// check for stop
	if ( trace->plane.normal[2] > 0.2 && VectorLength( self->s.pos.trDelta ) < 40 ) {
		G_ExplodeUserWeapon ( self );
		return;
	}
	
	VectorAdd( self->r.currentOrigin, trace->plane.normal, self->r.currentOrigin);
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	self->s.pos.trTime = level.time;
}



void G_ImpactUserWeapon (gentity_t *self, trace_t *trace) {
// Handles impact of the weapon with map geometry or entities.
	gentity_t		*other;
	qboolean		hitClient = qfalse;			
	
	other = &g_entities[trace->entityNum];

	
	if ( !other->takedamage && self->bounceFrac && self->bouncesLeft) {
		G_BounceUserMissile( self, trace );
		G_AddEvent( self, EV_GRENADE_BOUNCE, 0 );
		self->bouncesLeft--;
		return;
	}

	// Can the target take damage?
	if (other->takedamage) {
		// FIXME: wrong damage direction?
		// Does the missile do damage?
		if ( self->damage ) {
			vec3_t	velocity;

			// Log accuracy hits
			if( LogAccuracyHit( other, &g_entities[self->s.clientNum] ) ) {
				g_entities[self->s.clientNum].client->accuracy_hits++;
				hitClient = qtrue;
			}

			// Determine impact's knockback direction
			BG_EvaluateTrajectoryDelta( &self->s, &self->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}

			/*
			// Do the damage
			G_Damage (other, self, &g_entities[self->r.ownerNum], velocity,
				self->s.origin, self->damage, 
				0, self->methodOfDeath);
			*/
		}
	}

	// Terminate guidance
	if ( self->guided ) {
		g_entities[ self->s.clientNum ].client->ps.weaponstate = WEAPON_READY;
	}

	// Add the explosion event to the entity, and make it free itself after event.
	if ( other->takedamage && other->client ) {
		G_AddEvent( self, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		self->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( self, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else {
		G_AddEvent( self, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}
	self->freeAfterEvent = qtrue;

	// change over to a normal stationary entity right at the point of impact
	self->s.eType = ET_GENERAL;
	SnapVectorTowards( trace->endpos, self->s.pos.trBase );	// save net bandwidth
	G_SetOrigin( self, trace->endpos );
	

	if( G_UserRadiusDamage( trace->endpos, GetMissileOwnerEntity(self), self, self->damage, self->damageRadius, self->methodOfDeath, self->extraKnockback )) {
		if( !hitClient ) {
			g_entities[self->s.clientNum].client->accuracy_hits++;
		}
	
	}
	trap_LinkEntity( self );
}


void G_RemoveUserWeapon (gentity_t *self) {
// Wrapper function to safely remove a user weapon, incase
// things like fading trails need to be handled.

	if ( self->guided ) {
		g_entities[ self->s.clientNum ].client->ps.weaponstate = WEAPON_READY;
	}
	G_FreeEntity( self );
}



/*
   -----------------------------------------
     R U N   F R A M E   F U N C T I O N S
   -----------------------------------------
*/

void G_RunUserMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		trace;
	int			pass_ent;

	// get current position
	BG_EvaluateTrajectory( &ent->s, &ent->s.pos, level.time, origin );

	// ignore interactions with the missile owner
	pass_ent = ent->r.ownerNum;

	// trace a line from the previous position to the current position
	trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, pass_ent, ent->clipmask );

	if ( trace.startsolid || trace.allsolid ) {
		// make sure the trace.entityNum is set to the entity we're stuck in
		trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, pass_ent, ent->clipmask );
		trace.fraction = 0;
	}
	else {
		VectorCopy( trace.endpos, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );

	if ( trace.fraction != 1 ) {
		// never explode or bounce on sky
		if ( trace.surfaceFlags & SURF_NOIMPACT ) {
			G_RemoveUserWeapon( ent );
			return;
		}

		G_ImpactUserWeapon( ent, &trace );

		if ( (ent->s.eType != ET_MISSILE) && (ent->s.eType != ET_BEAMHEAD) ) {
			// Missile has changed to ET_GENERAL and has exploded, so we don't want
			// to run the think function!
			// Return immediately instead.
			return;	
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}


void G_RunUserSkimmer( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		trace;
	trace_t		groundTrace;
	int			pass_ent;
	vec3_t		min_bounds = {0, 0, 0};
	vec3_t		max_bounds = {0, 0, 0};
	vec3_t		up = {0, 0, SKIM_MIN_GROUNDCLEARANCE};
	vec3_t		down = {0, 0, -SKIM_MAX_GROUNDCLEARANCE};
	vec3_t		floor_target, cross_temp;
	
	// extrapolate current position to get total required travel length
	BG_EvaluateTrajectory( &ent->s, &ent->s.pos, level.time, origin );

	// ignore interactions with the missile owner
	pass_ent = ent->r.ownerNum;

	// trace a line from the previous position to the current position
	trap_Trace( &trace, ent->r.currentOrigin, NULL, NULL, origin, pass_ent, ent->clipmask );

	if ( trace.startsolid || trace.allsolid ) {
		// make sure the trace.entityNum is set to the entity we're stuck in
		trap_Trace( &trace, ent->r.currentOrigin, NULL, NULL, ent->r.currentOrigin, pass_ent, ent->clipmask );
		trace.fraction = -1;
	} else {
		// Place the endpoint of the trace backwards a small bit, using the calculated
		// bounds. This prevents a trace.startsolid from happening next time (we hope).
		VectorAdd( trace.endpos, up, ent->r.currentOrigin );
	}

	trap_LinkEntity( ent );
		
	if ( trace.fraction == -1 ) {
		// We were stuck in an entity.
			
		// A skimmer either is removed or explodes.
		if ( trace.surfaceFlags & SURF_NOIMPACT ) {
			G_RemoveUserWeapon( ent );
		} else {
			G_ImpactUserWeapon( ent, &trace );
		}
		
		return;
	}

	if ( trace.fraction < 1 ) {

		if ( trace.plane.normal[2] < MIN_SKIM_NORMAL ) {
			// The groundplane was too steep to land on

			if ( trace.surfaceFlags & SURF_NOIMPACT ) {
				G_RemoveUserWeapon( ent );
			} else {
				G_ImpactUserWeapon( ent, &trace );
			}

			return;
		}

		// Place the currentOrigin on the new polygon.
		VectorAdd( trace.endpos, up, ent->r.currentOrigin );

		// Copy our collision point to the start position of the trajectory.
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	 
		// Setup the new direction for the skimmer.
		CrossProduct( ent->s.pos.trDelta, trace.plane.normal, cross_temp );
		CrossProduct( trace.plane.normal, cross_temp, ent->s.pos.trDelta );
		VectorNormalize( ent->s.pos.trDelta );
		VectorScale( ent->s.pos.trDelta, ent->speed, ent->s.pos.trDelta);
		vectoangles( ent->s.pos.trDelta, ent->s.angles);

		// Set the new type and time for the trajectory
		ent->s.pos.trType = TR_LINEAR;
		ent->s.pos.trTime = level.time;

		// This will save net bandwidth.
		SnapVector( ent->s.pos.trDelta );

		// Set the skimmer to have hit ground.
		ent->onGround = qtrue;

		trap_LinkEntity( ent );
		return;
	}

	// NOTE: Getting to this point means trace.fraction == 1

	VectorAdd( ent->r.currentOrigin, down, floor_target );
	trap_Trace( &groundTrace, ent->r.currentOrigin, NULL, NULL, floor_target, pass_ent, MASK_SOLID );
	
	if ( ( groundTrace.fraction == 1 ) && ent->onGround ) {
		// We didn't reach the ground within specified length.

		if ( groundTrace.surfaceFlags & SURF_NOIMPACT ) {
			G_RemoveUserWeapon( ent );
		} else {
			G_ImpactUserWeapon( ent, &trace );
		}

		return;
	}

	if ( groundTrace.fraction < 1 ) {

		if ( groundTrace.plane.normal[2] < MIN_SKIM_NORMAL ) {
			// The groundplane was too steep to land on

			if ( groundTrace.surfaceFlags & SURF_NOIMPACT ) {
				G_RemoveUserWeapon( ent );
			} else {
				G_ImpactUserWeapon( ent, &trace );
			}

			return;
		}

		// Place the currentOrigin on the new ground polygon.
		VectorAdd( groundTrace.endpos, up, ent->r.currentOrigin );

		// Copy our collision point to the start position of the trajectory.
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	 
		// Setup the new direction for the skimmer.
		CrossProduct( ent->s.pos.trDelta, groundTrace.plane.normal, cross_temp );
		CrossProduct( groundTrace.plane.normal, cross_temp, ent->s.pos.trDelta );
		VectorNormalize( ent->s.pos.trDelta );
		VectorScale( ent->s.pos.trDelta, ent->speed, ent->s.pos.trDelta);
		vectoangles( ent->s.pos.trDelta, ent->s.angles);

		// Set the new type and time for the trajectory
		ent->s.pos.trType = TR_LINEAR;
		ent->s.pos.trTime = level.time;

		// This will save net bandwidth.
		SnapVector( ent->s.pos.trDelta );

		// Set the skimmer to have hit ground.
		ent->onGround = qtrue;

		trap_LinkEntity( ent );
	}
}
