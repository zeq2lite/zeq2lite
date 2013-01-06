#include "g_local.h"
#define	MISSILE_PRESTEP_TIME	 50
#define SOLID_ADD				400  // Add extra to guide line to go behind solids with weapon
#define BEAM_SECTION_TIME		520  // Timeframe to spawn a new beam section
/*-------------------------------
 M I S C   F U N C T I O N S
-------------------------------*/
/*=======================
GetMissileOwnerEntity
=======================*/
gentity_t *GetMissileOwnerEntity (gentity_t *missile) {
	gentity_t *parent;

	parent = missile->parent;
	while((parent->s.eType != ET_PLAYER)&&(parent->s.eType != ET_INVISIBLE)) {
		parent = GetMissileOwnerEntity( parent );
	}
	return parent;
}
/*---------------------------------
  T H I N K   F U N C T I O N S
---------------------------------*/

// Prototypes
void Think_NormalMissile (gentity_t *self);
void Think_NormalMissileStruggle (gentity_t *self);
static void Think_NormalMissileBurnPlayer (gentity_t *self);
static void Think_NormalMissileRidePlayer (gentity_t *self);
static void Think_NormalMissileStrugglePlayer (gentity_t *self);
static void G_StruggleUserMissile( gentity_t *self, gentity_t *other );
static void G_PushUserMissile( gentity_t *self, gentity_t *other );
static void G_BounceUserMissile( gentity_t *self, trace_t *trace );
void G_UserWeaponDamage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int dflags,int knockback);
void G_LocationImpact(vec3_t point, gentity_t* targ, gentity_t* attacker);

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

	self->s.dashDir[1] = self->powerLevelCurrent; // Use this free field to transfer current power level
	if(self->isDrainable && self->s.eType != ET_BEAMHEAD){
		self->powerLevelCurrent -= (float)self->powerLevelTotal * 0.01;
	}
	if (self->powerLevelCurrent <= 0) {
		G_RemoveUserWeapon(self);
		return;
	}
	if((owner->client->ps.bitFlags & usingBoost) && self->s.eType == ET_BEAMHEAD){
		self->powerLevelCurrent += 1 + ((float)self->powerLevelTotal * 0.005);
		self->speed += 10 + ((float)owner->client->ps.powerLevel[plMaximum] * 0.0003);
	}

	// If our weapon's owner can't be found, skip anything related to guiding.
	if (!owner){G_Printf( S_COLOR_YELLOW "WARNING: Think_Guided reports unknown fired weapon!\n" );}
	else{
		AngleVectors( owner->client->ps.viewangles, forward, right, up );
		CalcMuzzlePoint( owner, forward, right, up, muzzle );
		VectorCopy( self->r.currentOrigin, self->s.pos.trBase );	
		VectorSubtract( muzzle, self->r.currentOrigin, muzzle );
		dist = VectorLength( muzzle ) + SOLID_ADD;
		VectorScale( forward, dist, forward );
		VectorAdd( forward, muzzle, muzzle );
		VectorNormalize( muzzle );
		VectorScale( muzzle, self->speed, forward );
		VectorCopy( forward, self->s.pos.trDelta );
		vectoangles( muzzle, self->s.angles );
		SnapVector( self->s.pos.trDelta );
		self->s.pos.trType = TR_LINEAR;
		self->s.pos.trTime = level.time;
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
	gentity_t	*missileOwner = GetMissileOwnerEntity(self);

	self->s.dashDir[1] = self->powerLevelCurrent; // Use this free field to transfer current power level
	if(self->isDrainable){
		self->powerLevelCurrent -= (float)self->powerLevelTotal * 0.01;
	}
	if (self->powerLevelCurrent <= 0) {
		G_RemoveUserWeapon(self);
		return;
	}

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
	gentity_t	*missileOwner = GetMissileOwnerEntity(self);

	self->s.dashDir[1] = self->powerLevelCurrent; // Use this free field to transfer current power level
	if(self->isDrainable){
		self->powerLevelCurrent -= (float)self->powerLevelTotal * 0.01;
	}
	if (self->powerLevelCurrent <= 0) {
		G_RemoveUserWeapon(self);
		return;
	}

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
	gentity_t	*missileOwner = GetMissileOwnerEntity(self);

	if(level.time > self->startTimer && self->radius){
		int radius = self->radius;
		self->radius = 0;
		VectorSet( self->r.mins, -radius, -radius, -radius );
		VectorSet( self->r.maxs, radius, radius, radius );
		VectorCopy( self->r.mins, self->r.absmin );
		VectorCopy( self->r.maxs, self->r.absmax );
	}

	self->s.dashDir[1] = self->powerLevelCurrent; // Use this free field to transfer current power level

	if ((self->missileSpawnTime + self->maxMissileTime) <= level.time){
		self->think = G_ExplodeUserWeapon;
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	if(self->isDrainable && self->s.eType != ET_BEAMHEAD){
		self->powerLevelCurrent -= (float)self->powerLevelTotal * 0.01;
	}
	if (self->powerLevelCurrent <= 0) {
		G_RemoveUserWeapon(self);
		return;
	}
	if((missileOwner->client->ps.bitFlags & usingBoost) && self->s.eType == ET_BEAMHEAD){
		self->powerLevelCurrent += 10 + ((float)self->powerLevelTotal * 0.005);
		self->speed += 10 + ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003);
	}
	self->nextthink = level.time + FRAMETIME;
}

/*
=====================
Think_NormalMissileStruggle
=====================
*/
void Think_NormalMissileStruggle (gentity_t *self) {
	int			powerDifference;
	int			speedDifference;
	int			power1,power2;
	int			speed1,speed2;
	int			result;
	vec3_t		forward,dir,dir2,start;
	trace_t		*trace;
	gentity_t	*missileOwner = GetMissileOwnerEntity( self );
	self->s.dashDir[1] = self->powerLevelCurrent;
	if(self->powerLevelCurrent < 1){
		G_RemoveUserWeapon(self);
		return;
	}
	self->enemy->client->ps.bitFlags |= isStruggling;
	missileOwner->client->ps.bitFlags |= isStruggling;
	if((missileOwner->client->ps.bitFlags & usingBoost) && self->s.eType == ET_BEAMHEAD && missileOwner->client->ps.powerLevel[plCurrent] > 1){
		self->powerLevelCurrent += 5 + ((float)self->powerLevelTotal * 0.005);
		self->speed += 5 + ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003);
	}
	if(self->strugglingAllyAttack){
		if(self->s.eType == ET_BEAMHEAD){
			self->ally->speed += 5 + ((float)self->speed * 0.0003);
			self->speed -= 5 + ((float)self->speed * 0.0003);
		}else{
			self->speed -= 5;
			self->ally->speed += 5;
		}
	}
	speed1 = self->speed;
	speed2 = self->enemy->speed;
	power1 = self->powerLevelCurrent;
	power2 = self->enemy->powerLevelCurrent;
	speedDifference = speed1-speed2;
	powerDifference = power1-power2;
	result = /*speedDifference +*/ powerDifference;
	VectorCopy(self->r.currentOrigin,start);
	VectorCopy(self->s.pos.trDelta,dir);
	VectorCopy(self->r.currentAngles,dir2);
	VectorNormalize(dir);
	VectorScale(dir,0.2,dir);
	VectorAdd(dir,dir2,dir);
	VectorNormalize(dir);
	VectorCopy(start,self->s.pos.trBase);
	VectorScale(dir,result,self->s.pos.trDelta);
	VectorCopy(start,self->r.currentOrigin);
	VectorCopy(dir,self->r.currentAngles);
	self->s.pos.trType = TR_LINEAR;
	self->s.pos.trTime = level.time;
	self->nextthink = level.time + FRAMETIME;
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
void G_UserWeaponDamage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int dflags,int knockback){
	gclient_t *tgClient;
	if(!target->takedamage){return;}
	if(level.intermissionQueued){return;}
	if(!inflictor){inflictor = &g_entities[ENTITYNUM_WORLD];}
	if(!attacker){attacker = &g_entities[ENTITYNUM_WORLD];}
	tgClient = target->client;
	if(tgClient && tgClient->noclip){return;}
	else{VectorNormalize(dir);}
	if(target == attacker){knockback = 0;}
	if(target->flags & FL_NO_KNOCKBACK) {knockback = 0;}
	if(tgClient){
		//VectorCopy(dir ? dir : target->r.currentOrigin,tgClient->damage_from);
		tgClient->ps.persistant[PERS_ATTACKER] = attacker ? attacker->s.number : ENTITYNUM_WORLD;
		tgClient->damage_fromWorld = dir ? qfalse : qtrue;
		tgClient->lasthurt_client = attacker->s.number;
		tgClient->lasthurt_mod = 1;
	}
	if(damage){
		if(tgClient){
			attacker->client->ps.states |= causedDamage;
			G_LocationImpact(inflictor->r.currentOrigin,target,inflictor);
			if(target == attacker){damage *= 0.2f;}
			attacker->client->ps.powerLevel[plHealthPool] += damage * 0.7;
			attacker->client->ps.powerLevel[plMaximumPool] += damage * 0.3;
			tgClient->ps.powerLevel[plDamageFromEnergy] += damage;
			if(inflictor->impede){tgClient->ps.timers[tmImpede] = inflictor->impede;}
			if(knockback){
				tgClient->ps.timers[tmKnockback] = knockback;
				tgClient->ps.powerups[PW_KNOCKBACK_SPEED] = knockback;
				if(tgClient->lasthurt_location == LOCATION_BACK){tgClient->ps.knockBackDirection = 6;}
				else if(tgClient->lasthurt_location == LOCATION_LEFT){tgClient->ps.knockBackDirection = 4;}
				else if(tgClient->lasthurt_location == LOCATION_RIGHT){tgClient->ps.knockBackDirection = 3;}
				else{tgClient->ps.knockBackDirection = 5;}
			}
		}
	}
}

/*
=====================
G_UserRadiusDamage
=====================
*/
qboolean G_UserRadiusDamage ( vec3_t origin, gentity_t *attacker, gentity_t *ignore, float centerDamage, float radius,int extraKnockback ) {
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	
	float		realDamage, distance;
	gentity_t	*ent;
	gentity_t	*owner = GetMissileOwnerEntity(ignore);
	
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
		if (ent == owner && ignore->isBlindable)
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

		realDamage = centerDamage /** ( 1.0 - distance / radius )*/;

		if(CanDamage(ent, origin)){
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_LocationImpact(origin,ent,attacker);
			if(ent->client){
				ent->client->ps.powerLevel[plDamageGeneric] += realDamage;
				if(ent->pain){ent->pain(ent,attacker,realDamage);}
				/*if(ent->client->lasthurt_location == LOCATION_FRONT){
					ent->client->ps.timers[tmBlind] = 10000;
				}*/
			}
		}
	}
	return hitClient;
}
void UserHitscan_Fire (gentity_t *self, g_userWeapon_t *weaponInfo, int weaponNum, vec3_t muzzle, vec3_t forward ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tempEnt;
	gentity_t	*tempEnt2;
	gentity_t	*traceEnt;
	int			i, passent;
	float		rnd;
	float		physics_range;
	// Get the end point
	if ( weaponInfo->physics_range_min != weaponInfo->physics_range_max ) {
		rnd = crandom();
		physics_range = ( 1.0f - rnd ) * weaponInfo->physics_range_min + rnd * weaponInfo->physics_range_max;
	} else {
		physics_range = weaponInfo->physics_range_max;
	}	
	VectorMA (muzzle, physics_range, forward, end);

	// Set the player to not be able of being hurt by this shot
	passent = self->s.number;

	// Note; if bounce is enabled, this will let the projectile bounce a max. of 10 times
	for (i = 0; i < 10; i++) {

		trap_Trace (&tr, muzzle, NULL, NULL, end, passent, MASK_SHOT);
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

		} else if( tr.fraction == 1.0f || tr.surfaceFlags & SURF_NOIMPACT ) {
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
		tempEnt2 = G_TempEntity( tr.endpos, EV_NULL );
		tempEnt2->s.clientNum = self->s.number;
		tempEnt2->s.weapon = weaponNum;
		VectorCopy( muzzle, tempEnt2->s.origin2 );

		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}
		
		if ( traceEnt->takedamage) {
		G_UserWeaponDamage( traceEnt, self, self, forward, tr.endpos,
							weaponInfo->damage_damage, 0,
							weaponInfo->damage_extraKnockback );
		}
		break;
	}
}


void Release_UserWeapon( gentity_t *self, qboolean altfire ) {
	// Work on primary or alternate fire.
	if ( altfire ) {

		// If the altfired entity exists, then destroy it and null the pointer (just incase).
		if ( self->contAltfire_ent ) {
			G_FreeEntity( self->contAltfire_ent );
			self->contAltfire_ent = NULL;
		}
	} else {
		// If the fired entity exists, then destroy it and null the pointer (just incase).
		if ( self->contFire_ent ) {
			G_FreeEntity( self->contAltfire_ent );
			self->contAltfire_ent = NULL;
		}
	}
}

/*
=================
Fire_UserWeapon
=================
*/
void Fire_UserWeapon( gentity_t *self, vec3_t start, vec3_t dir, qboolean altfire ) {
	gentity_t		*bolt;
	g_userWeapon_t	*weaponInfo;
	float powerScale;
	int homingType;
	vec3_t			muzzle, forward, right, up;
	vec3_t			firingDir, firingStart; // <-- Contain the altered aiming and origin vectors.
	powerScale = ((float)self->client->ps.powerLevel[plCurrent] / 1000.0) * self->client->ps.baseStats[stEnergyAttack];
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


	// Determine the corrected firing angle and firing origin

	VectorCopy( dir, firingDir);
	if ( weaponInfo->firing_angleW_min || weaponInfo->firing_angleW_max ) {
		vec3_t	temp;
		float	rnd;
		float	firing_angleW;

		rnd = crandom(); // <-- between 0.0f and 1.0f
		firing_angleW = ( 1.0f - rnd ) * weaponInfo->firing_angleW_min + rnd * weaponInfo->firing_angleW_max;

		if ( ( self->client->ps.bitFlags & hasFlipOffset ) && weaponInfo->firing_offsetWFlip ) {
			RotatePointAroundVector( temp, up, firingDir, firing_angleW );
		} else {
			RotatePointAroundVector( temp, up, firingDir, -firing_angleW );
		}
		VectorCopy( temp, firingDir );
	}
	if ( weaponInfo->firing_angleH_min || weaponInfo->firing_angleH_max ) {
		vec3_t temp;
		float	rnd;
		float	firing_angleH;

		rnd = crandom(); // <-- between 0.0f and 1.0f
		firing_angleH = ( 1.0f - rnd ) * weaponInfo->firing_angleH_min + rnd * weaponInfo->firing_angleH_max;

		if ( ( self->client->ps.bitFlags & hasFlipOffset ) && weaponInfo->firing_offsetHFlip ) {
			RotatePointAroundVector( temp, right, firingDir, firing_angleH );
		} else {
			RotatePointAroundVector( temp, right, firingDir, -firing_angleH );
		}
		VectorCopy( temp, firingDir );
	}
	VectorNormalize( firingDir );

	VectorCopy( start, firingStart );
	if ( weaponInfo->firing_offsetW_min || weaponInfo->firing_offsetW_max ) {
		float rnd;
		float firing_offsetW;

		rnd = crandom(); // <-- between 0.0f and 1.0f
		firing_offsetW = ( 1.0f - rnd ) * weaponInfo->firing_offsetW_min + rnd * weaponInfo->firing_offsetW_max;

		if ( ( self->client->ps.bitFlags & hasFlipOffset ) && weaponInfo->firing_offsetWFlip ) {
			VectorMA( firingStart, -firing_offsetW, right, firingStart );
		} else {
			VectorMA( firingStart, firing_offsetW, right, firingStart );
		}
	}
	if ( weaponInfo->firing_offsetH_min || weaponInfo->firing_offsetH_max ) {
		float rnd;
		float firing_offsetH;

		rnd = crandom(); // <-- between 0.0f and 1.0f
		firing_offsetH = ( 1.0f - rnd ) * weaponInfo->firing_offsetH_min + rnd * weaponInfo->firing_offsetH_max;

		if ( ( self->client->ps.bitFlags & hasFlipOffset ) && weaponInfo->firing_offsetHFlip ) {
			VectorMA( firingStart, -firing_offsetH, up, firingStart );
		} else {
			VectorMA( firingStart, firing_offsetH, up, firingStart );
		}
	}
	

	// Take the necessary steps to configure and fire the weapon.
	switch ( weaponInfo->general_type ) {

	/* MISSILES */

	case skillTypeMISSILE:
		bolt = G_Spawn();
		bolt->classname = "rocket";
		bolt->s.eType = ET_MISSILE;
		bolt->startTimer = level.time + 800;
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
		bolt->splashDuration = weaponInfo->damage_radiusDuration;
		bolt->extraKnockback = weaponInfo->damage_extraKnockback;
		bolt->impede = weaponInfo->damage_impede;
		bolt->isSwattable = weaponInfo->physics_swat;
		bolt->isDrainable = weaponInfo->physics_drain;
		bolt->isBlindable = weaponInfo->physics_blind;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[stChargePercentSecondary];
			self->client->ps.stats[stChargePercentSecondary] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[stChargePercentPrimary];
			self->client->ps.stats[stChargePercentPrimary] = 0; // Only reset it here!
		}
		bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
		bolt->powerLevelCurrent = bolt->powerLevelTotal = bolt->damage * ((float)bolt->chargelvl / 100.0f) * powerScale;
		bolt->s.dashDir[0] = bolt->powerLevelTotal; // Use this free field to transfer total power level
		
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;
		bolt->takedamage = qtrue;
		self->s.dashDir[2] = 0.0f;
		bolt->count = 0;
		{
			float radius;
			radius = weaponInfo->physics_radius + weaponInfo->physics_radiusMultiplier * (bolt->chargelvl / 100.0f);
			radius = sqrt((radius * radius) / 3); // inverse of Pythagoras
			bolt->radius = radius;
			radius = 1;
			VectorSet( bolt->r.mins, -radius, -radius, -radius );
			VectorSet( bolt->r.maxs, radius, radius, radius );
			VectorCopy( bolt->r.mins, bolt->r.absmin );
			VectorCopy( bolt->r.maxs, bolt->r.absmax );

			bolt->r.contents = CONTENTS_CORPSE; // So we can pass through a missile, but can still fire at it.

			bolt->die = G_DieUserWeapon;			
		}

		bolt->speed = weaponInfo->physics_speed;
		bolt->accel = weaponInfo->physics_acceleration;
		bolt->bounceFrac = weaponInfo->physics_bounceFrac;
		bolt->bouncesLeft = weaponInfo->physics_maxBounces;
		bolt->timesTriggered = 0;
		
		// Configure the type correctly if we're using gravity or acceleration
		// Gravity affection takes precedence over acceleration.
		// FIXME: Can we get both simultaneously?
		if (weaponInfo->physics_gravity > 0) {
			bolt->gravity = (weaponInfo->physics_gravity / 100) * 1000; 
			bolt->s.pos.trType = TR_MAPGRAVITY;
			bolt->s.pos.trDuration = bolt->gravity;
		} else {
			if (bolt->accel != 0.0f) {
				bolt->s.pos.trType = TR_ACCEL;
				bolt->s.pos.trDuration = bolt->accel;
			} else {
				bolt->s.pos.trType = TR_LINEAR;
			}
		}
		
		// move a bit on the very first frame
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( firingStart, bolt->s.pos.trBase );
		VectorCopy( firingStart, bolt->r.currentOrigin );

		VectorCopy( self->client->ps.viewangles, self->s.angles );
		VectorCopy( self->client->ps.viewangles, bolt->s.angles2 );

		VectorScale( firingDir, bolt->speed, bolt->s.pos.trDelta );
		VectorCopy(firingDir,bolt->movedir);
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
				// Kill off previously set accel, bounce and gravity. Guided missiles
				// do not accelerate, bounce or experience gravity.
				bolt->accel = 0;
				bolt->bounceFrac = 0;
				bolt->bouncesLeft = 0;
				bolt->gravity = 0;
				// Set guidetarget.
				bolt->s.eFlags |= EF_GUIDED;
				self->client->guidetarget = bolt;
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
				{
					trace_t	trace;
					vec3_t  begin, mid, end;
					float	length;

					VectorCopy( bolt->s.pos.trBase, begin );
					VectorMA( begin, weaponInfo->homing_range, dir, end );
					trap_Trace( &trace, begin, NULL, NULL, end, self->s.number, MASK_SHOT );
					VectorCopy( trace.endpos, end );

					length = Distance( begin, end );

					VectorMA( begin, length / 2.0f, firingDir, mid );

					bolt->s.pos.trType = TR_ARCH;
					bolt->s.pos.trDuration = 1000.0f * length / weaponInfo->physics_speed;
					VectorCopy( begin, bolt->s.pos.trBase );
					VectorCopy( mid, bolt->s.angles2 );
					VectorCopy( end, bolt->s.pos.trDelta );
				}
				break;
			case HOM_DRUNKEN:
				bolt->think = Think_NormalMissile;
				bolt->nextthink = level.time + FRAMETIME;
				bolt->s.pos.trType = TR_DRUNKEN;
				bolt->s.pos.trDuration = weaponInfo->homing_range;
				break;
			case HOM_NONE:
				bolt->think = Think_NormalMissile;
				bolt->nextthink = level.time + FRAMETIME;
				break;
			default:
				// This should never happen!
				break;
		}
		bolt->missileSpawnTime = level.time;
		bolt->maxMissileTime = weaponInfo->physics_lifetime;
		break;
	/* BEAMS */
	case skillTypeBEAM:
		homingType = weaponInfo->homing_type;
		bolt = G_Spawn();
		bolt->classname = "user_beam";
		bolt->s.eType = ET_BEAMHEAD;
		bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;
		bolt->startTimer = level.time + 800;
		// Set the weapon number correct, depending on altfire status.
		if ( !altfire ) {
			bolt->s.weapon = self->s.weapon;
		}
		else{
			bolt->s.weapon = self->s.weapon + ALTWEAPON_OFFSET;
		}
		bolt->r.ownerNum = self->s.number;
		bolt->s.clientNum = self->s.number;	// <-- clientNum is free on all but ET_PLAYER so
		bolt->parent = self;				//     use it to store the owner for clientside.
		VectorCopy( self->client->ps.viewangles, bolt->s.angles ); // store the correct angles
		bolt->damage = weaponInfo->damage_damage;
		bolt->splashRadius = weaponInfo->damage_radius;
		bolt->splashDuration = weaponInfo->damage_radiusDuration;
		bolt->extraKnockback = weaponInfo->damage_extraKnockback;
		bolt->isSwattable = weaponInfo->physics_swat;
		bolt->isDrainable = weaponInfo->physics_drain;
		bolt->isBlindable = weaponInfo->physics_blind;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[stChargePercentSecondary];
			self->client->ps.stats[stChargePercentSecondary] = 0;
		}
		else{
			bolt->chargelvl = self->client->ps.stats[stChargePercentPrimary];
			self->client->ps.stats[stChargePercentPrimary] = 0;
		}
		bolt->s.powerups = bolt->chargelvl;
		bolt->powerLevelCurrent = bolt->powerLevelTotal = bolt->damage * (1.0f+(float)bolt->chargelvl / 100.0f) * powerScale;
		bolt->s.dashDir[0] = bolt->powerLevelTotal; // Use this free field to transfer total power level
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->takedamage = qtrue;
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;
		self->s.dashDir[2] = 0.0f;
		bolt->count = 0;
		{
			float radius;
			radius = weaponInfo->physics_radius + weaponInfo->physics_radiusMultiplier * (bolt->chargelvl / 100.0f);
			radius = sqrt((radius * radius) / 3); // inverse of Pythagoras
			bolt->radius = radius;
			radius = 1;
			VectorSet( bolt->r.mins, -radius, -radius, -radius );
			VectorSet( bolt->r.maxs, radius, radius, radius );
			VectorCopy( bolt->r.mins, bolt->r.absmin );
			VectorCopy( bolt->r.maxs, bolt->r.absmax );
			bolt->r.contents = CONTENTS_CORPSE; // So we can pass through a missile, but can still fire at it.
			bolt->die = G_DieUserWeapon;			
		}
		bolt->speed = weaponInfo->physics_speed;
		bolt->accel = 0;
		bolt->bounceFrac = 0;
		bolt->bouncesLeft = 0;
		bolt->gravity = 0;
		bolt->s.pos.trType = TR_LINEAR;
		VectorScale( dir, bolt->speed, bolt->s.pos.trDelta );
		SnapVector( bolt->s.pos.trDelta );
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( start, bolt->s.pos.trBase );
		VectorCopy( start, bolt->r.currentOrigin );
		VectorCopy( firingDir, bolt->movedir );
		VectorCopy( self->client->ps.viewangles, self->s.angles );
		VectorCopy( self->client->ps.viewangles, bolt->s.angles2 );
		if(self->client->ps.lockedTarget > 0){homingType = HOM_GUIDED;}
		switch (homingType) {
		case HOM_GUIDED:
			bolt->think = Think_Guided;
			bolt->nextthink = level.time + FRAMETIME;
			bolt->s.eFlags |= EF_GUIDED;
			self->client->guidetarget = bolt;
			bolt->guided = qtrue;
			break;

		case HOM_ARCH:
		case HOM_PROX:			
		case HOM_REGULAR:
		case HOM_CYLINDER:
		case HOM_NONE:
			bolt->think = Think_NormalMissile;
			bolt->nextthink = level.time + FRAMETIME;
			self->client->guidetarget = bolt;
			bolt->guided = qtrue;			
			break;
		default:
			break;
		}
		bolt->missileSpawnTime = level.time;
		bolt->maxMissileTime = weaponInfo->physics_lifetime;
		break;
	case skillTypeHITSCAN:
		if ( !altfire ) {
			UserHitscan_Fire( self, weaponInfo, self->s.weapon, firingStart, firingDir );
			self->client->ps.stats[stChargePercentPrimary] = 0;
		} else {
			UserHitscan_Fire( self, weaponInfo, self->s.weapon + ALTWEAPON_OFFSET, firingStart, firingDir );
			self->client->ps.stats[stChargePercentSecondary] = 0;
		}
		break;
	case skillTypeTRIGGER:
		{
			gentity_t		*trigTarget;
			trace_t			trace;
			vec3_t			newBase, newDir, traceEnd;
			int				pass_ent, i;
			float			traceDist;
			
			for ( i = 0; i < MAX_GENTITIES; i++ ) {
			
				// Select the entity
				trigTarget = &g_entities[i];
			
				// See if it is affected by the trigger
				if ( !trigTarget->inuse ) {
					continue;
				} else if ( trigTarget->s.eType != ET_MISSILE ) {
					continue;
				} else if ( !altfire && ( trigTarget->s.weapon != self->s.weapon + ALTWEAPON_OFFSET )) {
					continue;
				} else if ( altfire && ( trigTarget->s.weapon != self->s.weapon )) {
					continue;
				} else if (trigTarget->s.clientNum != self->s.number){
					continue;
				}

				// Get the target's current position
				BG_EvaluateTrajectory( &trigTarget->s, &trigTarget->s.pos, level.time, newBase );

				// Don't process any missiles beyond the maximum range of control
				if ( Distance(newBase, start) > weaponInfo->physics_range_max ) {
					continue;
				}

				// Set the target's new starting position and increase its trigger count
				VectorCopy( newBase, trigTarget->s.pos.trBase );
				trigTarget->timesTriggered++;
				 
				// Setup the new directional vector for the triggered weapon.
				VectorSubtract( start, newBase, newDir );
				traceDist = VectorLength( newDir ) + 100000;
	
				VectorScale( dir, traceDist, traceEnd );
				VectorAdd( traceEnd, start, traceEnd );
				
				pass_ent = self->s.number;
				trap_Trace( &trace, start, NULL, NULL, traceEnd, pass_ent, MASK_PLAYERSOLID );
				VectorSubtract( trace.endpos, start, traceEnd );

				VectorAdd( traceEnd, newDir, newDir );
				VectorNormalize( newDir );

				// Set the triggered weapon's new direction and speed
				VectorScale( newDir, weaponInfo->physics_speed, trigTarget->s.pos.trDelta );
				vectoangles( newDir, trigTarget->s.angles );
				trigTarget->s.pos.trTime = level.time;

				// Configure the type correctly if we're using gravity or acceleration
				// Gravity affection takes precedence over acceleration.
				// FIXME: Can we get both simultaneously?
				if (weaponInfo->physics_gravity > 0) {
					trigTarget->accel = 0;
					trigTarget->gravity = (weaponInfo->physics_gravity / 100) * 1000; 
					trigTarget->s.pos.trType = TR_MAPGRAVITY;
					trigTarget->s.pos.trDuration = trigTarget->gravity;
				} else if (weaponInfo->physics_acceleration != 0.0f) {
					trigTarget->gravity = 0;
					trigTarget->accel = weaponInfo->physics_acceleration;
					trigTarget->s.pos.trType = TR_ACCEL;
					trigTarget->s.pos.trDuration = trigTarget->accel;
				} else {
					trigTarget->accel = 0;
					trigTarget->gravity = 0;
					trigTarget->s.pos.trType = TR_LINEAR;
				}
			}
		}
		break;
	case skillTypeNONE:
		break;
	default:
		break;
	}
}
void G_DieUserWeapon(gentity_t *self, gentity_t *inflictor,gentity_t *attacker, int damage, int mod ) {
	if (inflictor == self)
		return;
	self->takedamage = qfalse;
	self->think = G_ExplodeUserWeapon;
	self->nextthink = level.time;
}
void G_ExplodeUserWeapon( gentity_t *self ) {
	// Handles actual detonation of the weapon in question.
	vec3_t dir = { 0, 0, 1};
	vec3_t origin;
	self->takedamage = qfalse;

	// Terminate guidance
	if ( self->guided ) {
		g_entities[ self->s.clientNum ].client->ps.weaponstate = WEAPON_READY;
	}
	
	// Calculate current position
	BG_EvaluateTrajectory(&self->s,&self->s.pos,level.time,origin);
	SnapVector( origin ); // save net bandwith
	SnapVectorTowards( origin, self->s.pos.trBase ); // save net bandwidth
	G_SetOrigin( self, origin );
	self->s.eType = ET_EXPLOSION;
	self->splashEnd = level.time + self->splashDuration;
	self->splashTimer = level.time;
	G_AddEvent( self, EV_MISSILE_MISS_AIR, DirToByte( dir ) );
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

static void G_HoldUserMissile( gentity_t *self, gentity_t *other ) {
	vec3_t	velocity;
	vec3_t	dir;
	float	dot;
	int		hitTime;

	if(other->s.eType == ET_PLAYER){
		if(self->s.eType == ET_MISSILE){
			VectorCopy(self->s.angles,dir);
		}else{
			VectorCopy(self->client->ps.viewangles,dir);
		}
		hitTime = level.previousTime + ( level.time - level.previousTime ) * 0;
		BG_EvaluateTrajectoryDelta( &self->s, &self->s.pos, hitTime, velocity );
		dot = DotProduct( velocity, dir );
		VectorMA( velocity, -2 * dot, dir, self->s.pos.trDelta );

		VectorScale( self->s.pos.trDelta, self->bounceFrac, self->s.pos.trDelta );

		VectorAdd( self->r.currentOrigin, dir, self->r.currentOrigin);
		VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	}

	self->s.pos.trTime = level.time;
}

static void G_PushUserMissile( gentity_t *self, gentity_t *other ) {
	vec3_t forward, right, up; 
	vec3_t muzzle;
	float dist;

	// Calculate based on the other player view angles.
	AngleVectors( other->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( other, forward, right, up, muzzle );

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
	VectorScale( self->s.pos.trDelta, self->bounceFrac, self->s.pos.trDelta );
	vectoangles( muzzle, self->s.angles );

	// This will save net bandwidth.
	SnapVector( self->s.pos.trDelta );

	self->s.pos.trType = TR_LINEAR;
	self->s.pos.trTime = level.time;
}

static void Think_NormalMissileStrugglePlayer( gentity_t *self ) {
	vec3_t fwd;
	gentity_t	*missileOwner = GetMissileOwnerEntity( self );
	AngleVectors(self->enemy->r.currentAngles, fwd, NULL, NULL);
	VectorNormalize(fwd);
	G_HoldUserMissile(self,self->enemy);
	self->s.dashDir[1] = self->powerLevelCurrent;
	self->bounceFrac = 0.0f;
	self->enemy->client->ps.timers[tmStruggleBlock] += 100;
	if(self->enemy->client->ps.timers[tmStruggleBlock] >= 3000){
		self->enemy->client->ps.bitFlags &= ~isStruggling;
		missileOwner->client->ps.bitFlags &= ~isStruggling;
		G_ExplodeUserWeapon(self);
		return;
		//if(self->s.eType == ET_MISSILE){self->think = Think_NormalMissileBurnPlayer;}
		//if(self->s.eType == ET_BEAMHEAD){self->think = Think_NormalMissileRidePlayer;}
	}
	self->enemy->client->ps.bitFlags |= isStruggling;
	missileOwner->client->ps.bitFlags |= isStruggling;
	self->powerLevelCurrent -= (self->enemy->client->ps.powerLevel[plFatigue] * 0.2) * 0.1;
	if(self->enemy->client->ps.powerLevel[plFatigue] * 0.5 >= (self->powerLevelCurrent)){
		self->enemy->client->ps.bitFlags &= ~isStruggling;
		missileOwner->client->ps.bitFlags &= ~isStruggling;
		self->s.eType = ET_MISSILE;
		self->think = Think_NormalMissile;
		self->bounceFrac = 0.75;
		G_PushUserMissile(self,self->enemy);
	}
	self->nextthink = level.time + FRAMETIME;
}
static void Think_NormalMissileBurnPlayer( gentity_t *self ) {
	vec3_t fwd;
	gentity_t	*missileOwner = GetMissileOwnerEntity( self );
	AngleVectors(self->enemy->r.currentAngles, fwd, NULL, NULL);
	VectorNormalize(fwd);
	G_HoldUserMissile(self,self->enemy);
	self->s.dashDir[1] = self->powerLevelCurrent;
	self->bounceFrac = 0.0f;
	self->enemy->client->ps.states |= isBurning;
	self->enemy->client->ps.timers[tmBurning] += 100;
	G_UserWeaponDamage(self->enemy,self,missileOwner,fwd,self->s.origin,self->powerLevelCurrent / 15.0,0,self->extraKnockback);
	if(self->enemy->client->ps.timers[tmBurning] >= 1500){
		self->freeAfterEvent = qtrue;
		self->enemy->client->ps.states &= ~isBurning;
	}
	self->nextthink = level.time + FRAMETIME;
}
static void Think_NormalMissileRidePlayer( gentity_t *self ){
	vec3_t fwd;
	gentity_t	*missileOwner = GetMissileOwnerEntity( self );
	AngleVectors(self->enemy->r.currentAngles, fwd, NULL, NULL);
	VectorNormalize(fwd);
	self->s.dashDir[1] = self->powerLevelCurrent;
	self->bounceFrac = 0.0f;
	self->enemy->client->ps.states |= isRiding;
	self->enemy->client->ps.timers[tmRiding] += 100;
	G_UserWeaponDamage(self->enemy,self,missileOwner,fwd,self->s.origin,self->powerLevelCurrent / 25.0,0,self->extraKnockback);
	if(self->enemy->client->ps.timers[tmRiding] >= 2500){
		self->freeAfterEvent = qtrue;
		self->enemy->client->ps.states &= ~isRiding;
		//self->enemy->client->ps.states |= canRideEscape;
	}
	self->nextthink = level.time + FRAMETIME;
}
/* 
============
G_LocationImpact
============
*/
void G_LocationImpact(vec3_t point, gentity_t* targ, gentity_t* attacker) {
	vec3_t attackPath;
	vec3_t attackAngle;
	int clientRotation;
	int attackRotation;	// Degrees rotation around client.
	int impactRotation;	// used to check back of head vs. face
	VectorSubtract(targ->r.currentOrigin, point, attackPath); 
	vectoangles(attackPath, attackAngle);
	clientRotation = targ->client->ps.viewangles[YAW];
	attackRotation = attackAngle[YAW];
	impactRotation = abs(clientRotation-attackRotation);
	impactRotation += 45; // just to make it easier to work with
	impactRotation = impactRotation % 360; // Keep it in the 0-359 range
	if (impactRotation < 90)
		targ->client->lasthurt_location = LOCATION_BACK;
	else if (impactRotation < 180)
		targ->client->lasthurt_location = LOCATION_RIGHT;
	else if (impactRotation < 270)
		targ->client->lasthurt_location = LOCATION_FRONT;
	else if (impactRotation < 360)
		targ->client->lasthurt_location = LOCATION_LEFT;
	else
		targ->client->lasthurt_location = LOCATION_NONE;
}

// Handles impact of the weapon with map geometry or entities.
void G_ImpactUserWeapon(gentity_t *self,trace_t *trace){
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	vec3_t	velocity;
	int radius;
	other = &g_entities[trace->entityNum];
	G_LocationImpact(trace->endpos,other,GetMissileOwnerEntity(self));
	//G_Printf("Attack's power level is : %i\n",self->powerLevelCurrent);
	// Initiate Player Interaction
	if(other->s.eType == ET_PLAYER){
		SnapVectorTowards( trace->endpos, self->s.pos.trBase );
		G_SetOrigin( self, trace->endpos );
		if((other->client->ps.bitFlags & usingBlock) && other->client->lasthurt_location == LOCATION_FRONT){
			if(self->isSwattable){
				self->bounceFrac = 1.0f;
				G_PushUserMissile(self, other);
				return;
			}
			else{
				self->strugglingPlayer = qtrue;
				self->enemy = other;
				self->enemy->client->ps.timers[tmStruggleBlock] = 0;
				self->think = Think_NormalMissileStrugglePlayer;
				self->nextthink = level.time;
				if(!other->client->ps.lockedTarget){
					other->client->ps.lockedTarget = self->s.number;
					other->client->ps.lockedPosition = &self->r.currentOrigin;
				}
				return;
			}
		}
		else if(self->s.eType == ET_MISSILE){
			if(self->isSwattable){
				if(((self->powerLevelCurrent * 10) < other->client->ps.powerLevel[plFatigue]) && other->client->ps.bitFlags & usingAlter){
					self->freeAfterEvent = qtrue;
					return;
				}
				//Do Explosion Below
			}
			else{
				/*self->enemy = other;
				self->enemy->client->ps.timers[tmBurning] = 0;
				self->think = Think_NormalMissileBurnPlayer;
				self->nextthink = level.time;
				return;*/
			}
		}
		else if(self->s.eType == ET_BEAMHEAD && !(other->client->ps.states & isRiding)){
			/*self->enemy = other;
			self->enemy->client->ps.timers[tmRiding] = 0;
			self->think = Think_NormalMissileRidePlayer;
			self->nextthink = level.time;
			return;*/
		}
	}
	// Initiate Attack Interaction
	else if(self->s.eType == ET_MISSILE){
		//G_Printf("I am definitely a missile!\n");
		if(other->s.eType == ET_BEAMHEAD){
			//G_Printf("Their Beam absorbed our missile!\n");
			other->powerLevelCurrent += self->powerLevelCurrent;
			self->freeAfterEvent = qtrue;
			return;
		}
		else if(other->s.eType == ET_MISSILE){
			if(other->isSwattable){
				if(!self->isSwattable){
					//G_Printf("Our missile absorbed their swattable!\n");
					self->powerLevelCurrent += other->powerLevelCurrent;
					other->freeAfterEvent = qtrue;
					return;
				}
				else{
					//G_Printf("Swattables collide explosion!\n");
				}
			}
			else{
				if(other->powerLevelCurrent >= self->powerLevelCurrent){
					//G_Printf("Our missile absorbed their weaker missile!\n");
					other->powerLevelCurrent += self->powerLevelCurrent;
					self->freeAfterEvent = qtrue;
				}
				return;
			}
		}
	}
	else if(self->s.eType == ET_BEAMHEAD){
		if(other->s.eType == ET_BEAMHEAD){
			if(!(self->client->ps.bitFlags & isStruggling)){
				G_AddEvent(self,EV_POWER_STRUGGLE_START, DirToByte( trace->plane.normal));
				other->enemy = self;
				self->s.dashDir[2] = 1.0f;
				self->think = Think_NormalMissileStruggle;
				self->nextthink = level.time;
				self->enemy = other;
				self->enemy->s.dashDir[2] = 1.0f;
				self->enemy->think = Think_NormalMissileStruggle;
				self->enemy->nextthink = level.time;
				if(self->s.eFlags & EF_GUIDED){self->s.eFlags &= ~EF_GUIDED;}
				if(other->s.eFlags & EF_GUIDED){other->s.eFlags &= ~EF_GUIDED;}
			}
			return;
		}
		else if(other->s.eType == ET_MISSILE){return;}
	}
	// Bounce off the world
	if(self->bounceFrac && self->bouncesLeft && !other->takedamage){
		G_BounceUserMissile(self, trace);
		self->bouncesLeft--;
		return;
	}
	// Hit a player or the world
	else if(other->client || !other->takedamage){ 
		self->takedamage = qtrue;
		if(other->takedamage && self->damage) {
			BG_EvaluateTrajectoryDelta(&self->s, &self->s.pos, level.time, velocity);
			if(VectorLength( velocity ) == 0){
				velocity[2] = 1;
			}
			G_UserWeaponDamage(other,self,GetMissileOwnerEntity(self),velocity,self->s.origin,self->powerLevelCurrent,0,self->extraKnockback);
		}
	}
	// Explode
	if(!(other->takedamage) || (other->s.eType == ET_PLAYER)){
		if(self->guided){g_entities[self->s.clientNum].client->ps.weaponstate = WEAPON_READY;}
		if(other->takedamage && other->client){
			G_AddEvent( self, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
			self->s.otherEntityNum = other->s.number;
		}
		else if(trace->surfaceFlags & SURF_METALSTEPS){G_AddEvent( self, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );}
		else{G_AddEvent( self, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );}
		SnapVectorTowards( trace->endpos, self->s.pos.trBase );
		G_SetOrigin( self, trace->endpos );
		self->powerLevelCurrent *= 0.5;
		self->s.eType = ET_EXPLOSION;
		self->splashEnd = level.time + self->splashDuration;
		self->splashTimer = level.time + 100;
		trap_LinkEntity(self);
	}
}
void G_DetachUserWeapon (gentity_t *self) {
	gentity_t *other;
	other = self->enemy;
	if(self->s.eType == ET_BEAMHEAD && self->powerLevelCurrent > 0){
		g_entities[self->s.clientNum].client->ps.weaponstate = WEAPON_READY;
		self->s.eType = ET_MISSILE;
		self->think = Think_NormalMissile;
	}
}
void G_RemoveUserWeapon (gentity_t *self) {
	gentity_t *other;
	other = self->enemy;
	other->client->ps.bitFlags &= ~isStruggling;
	self->client->ps.bitFlags &= ~isStruggling;
	if(other->client->ps.lockedTarget >= MAX_CLIENTS){
		other->client->ps.lockedPosition = NULL;
		other->client->ps.lockedTarget = 0;
	}
	if (self->guided) {
		g_entities[ self->s.clientNum ].client->ps.weaponstate = WEAPON_READY;
	}
	if (self->s.eType == ET_BEAMHEAD && self->powerLevelCurrent > 0) {
		G_ExplodeUserWeapon ( self );
		return;
	}
	G_FreeEntity( self );
}

void Missile_Smooth (gentity_t *ent, vec3_t origin, trace_t *tr) {
	int touch[MAX_GENTITIES];
	vec3_t mins, maxs;
	int num;

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES);
	VectorAdd(origin,ent->r.mins, mins);
	VectorAdd(origin,ent->r.maxs, maxs);
	VectorCopy(origin,ent->s.pos.trBase);
	ent->s.pos.trTime = level.time;
}

/*
   -----------------------------------------
     R U N   F R A M E   F U N C T I O N S
   -----------------------------------------
*/
void G_RunUserExplosion(gentity_t *ent) {
	int radius,power;
	float step;
	if(level.time > ent->splashTimer && ent->powerLevelCurrent > 0){
		ent->splashTimer = level.time + 100;
		step = (1.0 - ((float)ent->splashEnd - (float)level.time) / (float)ent->splashDuration);
		radius = step * ent->splashRadius;
		power = (ent->powerLevelCurrent * (100.0/(float)ent->splashDuration));
		//G_Printf("Explosion power : %i\n",power);
		G_UserRadiusDamage(ent->r.currentOrigin,GetMissileOwnerEntity(ent),ent,power,radius,ent->extraKnockback);
	}
	if(level.time >= ent->splashEnd || ent->powerLevelCurrent <= 0){
		ent->freeAfterEvent = qtrue;
	}
}
void G_RunUserMissile( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		trace;
	int			pass_ent;
	int			beamKnockBack;
	gentity_t	*traceEnt;
	vec3_t		muzzle, forward, right, up;
	vec3_t		start,end;
	trace_t		trace2;
	gentity_t	*traceEnt2;
	gentity_t	*missileOwner = GetMissileOwnerEntity(ent);
	BG_EvaluateTrajectory( &ent->s, &ent->s.pos, level.time, origin );
	if (ent->count) {
		pass_ent = ent->s.number;
	}
	else{
		pass_ent = ent->r.ownerNum;
	}
	// trace a line from the previous position to the current position
	trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, pass_ent, ent->clipmask );

	if ( trace.startsolid || trace.allsolid ) {
		// make sure the trace.entityNum is set to the entity we're stuck in
		trap_Trace(&trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, pass_ent, ent->clipmask );
		trace.fraction = 0;
	}
	else {
		VectorCopy( trace.endpos, ent->r.currentOrigin );
	}
	//Missile_Smooth(ent,origin,&trace);
	trap_LinkEntity(ent);
	//G_Printf("%i\n",ent->r.currentOrigin);

	if(trace.fraction != 1){
		// never explode or bounce on sky
		if ( trace.surfaceFlags & SURF_NOIMPACT ) {
			G_RemoveUserWeapon( ent );
			return;
		}

		G_ImpactUserWeapon(ent, &trace);

		if ((ent->s.eType != ET_MISSILE) && (ent->s.eType != ET_BEAMHEAD)){return;}
	}
	// if the attack wasn't yet outside the player body
	if (!ent->count) {
		// check if the attack is outside the owner bbox
		trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ENTITYNUM_NONE, ent->clipmask );
		if (!trace.startsolid || trace.entityNum != ent->r.ownerNum) {
			ent->count = 1;
		}
	}

	// Here we test if something entered a beam
	if(ent->s.eType == ET_BEAMHEAD && !(ent->s.eFlags & EF_GUIDED)){
		// Get player direction
		AngleVectors( missileOwner->client->ps.viewangles, forward, right, up );
		CalcMuzzlePoint( missileOwner, forward, right, up, muzzle );
		// Get player position and beam head position
		BG_EvaluateTrajectory( &missileOwner->s, &missileOwner->s.pos, level.time, start );
		BG_EvaluateTrajectory( &ent->s, &ent->s.pos, level.time, end );
		// Trace between the two positions
		trap_Trace (&trace2, start, ent->r.mins, ent->r.maxs, end, ent->s.number, MASK_SHOT);
		traceEnt2 = &g_entities[ trace2.entityNum ];
		// Snap the endpos to integers, but nudged towards the line
		SnapVectorTowards( trace2.endpos, muzzle );
		// If a player is holding block when he entered the beam, inititate push struggle.
		if (traceEnt2->client && traceEnt2->takedamage && trace2.fraction != 1 && (traceEnt2->client->ps.bitFlags & usingBlock)) {
			G_SetOrigin(ent,trace2.endpos);
			G_ImpactUserWeapon(ent,&trace2);
		// Else if a player that entered the beam isn't blocking, burn them and/or push them.
		}else if (traceEnt2->client && traceEnt2->takedamage && trace2.fraction != 1 && !(traceEnt2->client->ps.bitFlags & usingBlock)) {
		}
		if ((ent->s.eType != ET_MISSILE) && (ent->s.eType != ET_BEAMHEAD)) {
			return;	
		}
	}

	// check think function after bouncing
	G_RunThink( ent );
}
