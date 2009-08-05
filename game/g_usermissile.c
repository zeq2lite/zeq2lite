#include "g_local.h"
#define	MIN_SKIM_NORMAL	0.5f
#define	MISSILE_PRESTEP_TIME	 50
#define SOLID_ADD				400  // Add extra to guide line to go behind solids with weapon
#define BEAM_SECTION_TIME		520  // Timeframe to spawn a new beam section
#define SKIM_MIN_GROUNDCLEARANCE	 5	// Amount the attack will 'hover' above ground
#define	SKIM_MAX_GROUNDCLEARANCE	25	// How much distance are we allowed to take to reach ground level again.
										// Used in a check for passing steep cliffs, etc. while skimming.

/*-------------------------------
 M I S C   F U N C T I O N S
-------------------------------*/


/*
=======================
GetMissileOwnerEntity
=======================
*/
gentity_t *GetMissileOwnerEntity (gentity_t *missile) {
	gentity_t *parent;

	parent = missile->parent;
	while((parent->s.eType != ET_PLAYER)&&(parent->s.eType != ET_INVISIBLE)) {
		parent = GetMissileOwnerEntity( parent );
	}
	return parent;
}



/*
   ---------------------------------
     T H I N K   F U N C T I O N S
   ---------------------------------
*/

// Prototypes
void Think_NormalMissile (gentity_t *self);
void Think_NormalMissileStruggle (gentity_t *self);
static void Think_NormalMissileStrugglePlayer (gentity_t *self);
static void G_StruggleUserMissile( gentity_t *self, gentity_t *other );
static void G_PushUserMissile( gentity_t *self, gentity_t *other );
static void G_BounceUserMissile( gentity_t *self, trace_t *trace );
void G_UserWeaponDamage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int dflags,int methodOfDeath,int knockback);
/*
=============
Think_Torch
=============
*/
void Think_Torch( gentity_t *self ) {
	// NOTE: This function is called to override and destroy a torch type continuous
	//       attack when its lifetime runs out. It is specificly optimized for torches,
	//       and doesn't need to be run every frame.
	g_userWeapon_t	*weaponInfo;

	weaponInfo = G_FindUserWeaponData( self->r.ownerNum, self->s.weapon );

	// Set the weapon state to WEAPON_COOLING
	g_entities[self->r.ownerNum].client->ps.weaponstate = WEAPON_COOLING;
	
	// Set the cooldown time on the weapon
	g_entities[self->r.ownerNum].client->ps.weaponTime = weaponInfo->costs_cooldownTime;
	
	// Free the entity
	G_FreeEntity( self );
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

	self->s.dashDir[1] = owner->client->ps.attackPowerCurrent = self->powerLevelCurrent; // Use this free field to transfer current power level

	if ((self->strugglingAttack && self->enemy->strugglingAttack) || (self->strugglingAllyAttack && self->enemy->strugglingAttack)){
		self->think = Think_NormalMissileStruggle;
		self->nextthink = level.time;
		return;
	} else if (self->strugglingPlayer) {
		self->think = Think_NormalMissileStrugglePlayer;
		self->nextthink = level.time;
		return;
	}

	if(self->isDrainable){
		self->powerLevelCurrent -= 1 + ((float)self->powerLevelTotal * 0.005);
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
	if ( !owner )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: Think_Guided reports unknown fired weapon!\n" );
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
	gentity_t	*missileOwner = GetMissileOwnerEntity(self);

	self->s.dashDir[1] = missileOwner->client->ps.attackPowerCurrent = self->powerLevelCurrent; // Use this free field to transfer current power level

	if ((self->strugglingAttack && self->enemy->strugglingAttack) || (self->strugglingAllyAttack && self->enemy->strugglingAttack)){
		self->think = Think_NormalMissileStruggle;
		self->nextthink = level.time;
		return;
	} else if (self->strugglingPlayer) {
		self->think = Think_NormalMissileStrugglePlayer;
		self->nextthink = level.time;
		return;
	}

	if(self->isDrainable){
		self->powerLevelCurrent -= 1 + ((float)self->powerLevelTotal * 0.005);
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

	self->s.dashDir[1] = missileOwner->client->ps.attackPowerCurrent = self->powerLevelCurrent; // Use this free field to transfer current power level

	if ((self->strugglingAttack && self->enemy->strugglingAttack) || (self->strugglingAllyAttack && self->enemy->strugglingAttack)){
		self->think = Think_NormalMissileStruggle;
		self->nextthink = level.time;
		return;
	} else if (self->strugglingPlayer) {
		self->think = Think_NormalMissileStrugglePlayer;
		self->nextthink = level.time;
		return;
	}

	if(self->isDrainable){
		self->powerLevelCurrent -= 1 + ((float)self->powerLevelTotal * 0.005);
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

	self->s.dashDir[1] = missileOwner->client->ps.attackPowerCurrent = self->powerLevelCurrent; // Use this free field to transfer current power level

	if ((self->missileSpawnTime + self->maxMissileTime) <= level.time){
		self->think = G_ExplodeUserWeapon;
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	if(self->isDrainable){
		self->powerLevelCurrent -= 1 + ((float)self->powerLevelTotal * 0.005);
	}
	if (self->powerLevelCurrent <= 0) {
		G_RemoveUserWeapon(self);
		return;
	}
	if((missileOwner->client->ps.bitFlags & usingBoost) && self->s.eType == ET_BEAMHEAD){
		self->powerLevelCurrent += 10 + ((float)self->powerLevelTotal * 0.005);
		self->speed += 10 + ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003);
	}
	if ((self->strugglingAttack && self->enemy->strugglingAttack) || (self->strugglingAllyAttack && self->enemy->strugglingAttack)){
		self->think = Think_NormalMissileStruggle;
		self->nextthink = level.time;
	} else if (self->strugglingPlayer) {
		self->think = Think_NormalMissileStrugglePlayer;
		self->nextthink = level.time;
	} else {
		self->nextthink = level.time + FRAMETIME;
	}
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
	self->s.dashDir[1] = missileOwner->client->ps.attackPowerCurrent = self->powerLevelCurrent;
	// Our players have a slight power level drain for the duration of a struggle
	missileOwner->client->ps.powerLevel[plUseCurrent] += ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003);
	// Our attacks have a slight constant drain if we try to go over the attacks's normal power.
	if(self->powerLevelCurrent > self->powerLevelTotal){
		self->powerLevelCurrent -= 1;// + ((float)self->powerLevelTotal * 0.005);
	}
	// If we're beams, set our origins to be at the same point
	if(self->s.eType == ET_BEAMHEAD && self->enemy->s.eType == ET_BEAMHEAD){
		if(self->powerLevelCurrent > self->enemy->powerLevelCurrent){
			//G_SetOrigin(self,self->enemy->s.pos.trBase);
		}
	}
	// If we ran out of power.
	if(self->powerLevelCurrent <= 10){
		// And we are not a beam.
		if(self->s.eType != ET_BEAMHEAD) {
			// Remove the ball.
			self->strugglingAttack = qfalse;
			G_RemoveUserWeapon ( self );
			return;
		// Or we're a beam and both our powers are drained.
		}else if(self->client->ps.powerLevel[plCurrent] <= 1 && self->enemy->client->ps.powerLevel[plCurrent] <= 1){
			// Remove us.
			G_ExplodeUserWeapon ( self );
			return;
		// Else we're a beam and we arn't helping an ally
		}else if(!self->strugglingAllyAttack){
			// We stick around for the beam struggle regardless of our drained power.
			self->powerLevelCurrent = 10;
		// Else
		}else{
			// Remove the beam.
			self->strugglingAttack = qfalse;
			G_RemoveUserWeapon ( self );
			return;
		}
	}
	// If we're not a beam.
	if(self->s.eType != ET_BEAMHEAD){
		// And we're winning.
		if(self->powerLevelCurrent > self->enemy->powerLevelCurrent){
			// Our attack is absorbing the other attack's power and speed instead.
			self->powerLevelCurrent += 5 + ((float)self->powerLevelTotal * 0.005);
			self->enemy->powerLevelCurrent -= 5 + ((float)self->powerLevelTotal * 0.005);
			self->speed += 5;
			self->enemy->speed -= 5;
		}else{
			// Our attack is being absorbing instead.
			self->powerLevelCurrent -= 5 + ((float)self->powerLevelTotal * 0.005);
			self->enemy->powerLevelCurrent += 5 + ((float)self->powerLevelTotal * 0.005);
			self->speed -= 5;
			self->enemy->speed += 5;
		}
	}
	// If we're using boost and we're a beam.
	if((missileOwner->client->ps.bitFlags & usingBoost) && self->s.eType == ET_BEAMHEAD && missileOwner->client->ps.powerLevel[plCurrent] > 1){
		// Make us stronger and faster.
		self->powerLevelCurrent += 5 + ((float)self->powerLevelTotal * 0.005);
		self->speed += 5 + ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003);
		missileOwner->client->ps.powerLevel[plUseCurrent] += ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003);
	}
	// If we're helping an allies attack.
	if(self->strugglingAllyAttack){
		// And we are a beam.
		if(self->s.eType == ET_BEAMHEAD){
			// Add to the allies power and speed directly.
			self->ally->powerLevelCurrent += 5 + ((float)self->powerLevelCurrent * 0.0003);
			self->ally->speed += 5 + ((float)self->speed * 0.0003);
			self->powerLevelCurrent -= 5 + ((float)self->powerLevelCurrent * 0.0003);
			self->speed -= 5 + ((float)self->speed * 0.0003);
		// Else we're a ball.
		}else{
			// Let the allies attack absorb our power and speed instead.
			self->powerLevelCurrent -= 5;
			self->speed -= 5;
			self->ally->powerLevelCurrent += 5;
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
	// If the other attack stopped struggling for whatever reason, our attack will resume moving forward.
	if(!self->enemy->strugglingAttack || !self->ally->strugglingAttack){
		VectorCopy(self->r.currentOrigin,start);
		VectorCopy(self->s.pos.trDelta,dir);
		VectorCopy(self->r.currentAngles,dir2);
		VectorNormalize(dir);
		VectorScale(dir,0.2,dir);
		VectorAdd(dir,dir2,dir);
		VectorNormalize(dir);
		VectorCopy(start,self->s.pos.trBase);
		VectorScale(dir,self->speed,self->s.pos.trDelta);
		VectorCopy(start,self->r.currentOrigin);
		VectorCopy(dir,self->r.currentAngles);
		self->s.pos.trType = TR_LINEAR;
		self->s.pos.trTime = level.time;
		if(self->enemy->s.eType == ET_BEAMHEAD){
			self->enemy->client->ps.bitFlags &= ~isStruggling;
		}
		if(self->s.eType == ET_BEAMHEAD){
			missileOwner->client->ps.bitFlags &= ~isStruggling;
		}
		self->enemy = NULL;
		self->strugglingAttack = qfalse;
		self->strugglingAllyAttack = qfalse;
		self->s.dashDir[2] = 0.0f;
		self->think = Think_NormalMissile;
		self->nextthink = level.time;
		// check for stop
		if ( self->s.eType != ET_BEAMHEAD && VectorLength( self->s.pos.trDelta ) < 40 ) {
			G_ExplodeUserWeapon ( self );
			return;
		}
		//G_Printf("Moving forward again!\n");
		return;
	}
	// Help ally with struggle!
	if(self->strugglingAllyAttack){
		VectorCopy(self->ally->s.pos.trBase,self->s.pos.trBase);
		VectorCopy(self->ally->s.pos.trDelta,self->s.pos.trDelta);
		VectorCopy(self->ally->r.currentOrigin,self->r.currentOrigin);
		VectorCopy(self->ally->r.currentAngles,self->r.currentAngles);
		self->s.pos.trType = TR_LINEAR;
		self->s.pos.trTime = level.time;
		//G_Printf("Helping ally!\n");
	// Struggle away!
	}else{
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
		//G_Printf("Power: %i Speed: %i Power Difference: %i Speed Difference: %f Result: %i\n",self->powerLevelCurrent, self->speed, powerDifference, speedDifference, result);
	}
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
void G_UserWeaponDamage(gentity_t *target,gentity_t *inflictor,gentity_t *attacker,vec3_t dir,vec3_t point,int damage,int dflags,int methodOfDeath,int knockback){
	gclient_t *tgClient;
	float damageScale;
	if(!target->takedamage){return;}
	if(level.intermissionQueued){return;}
	if(target->flags & FL_GODMODE){return;}
	if(!inflictor){inflictor = &g_entities[ENTITYNUM_WORLD];}
	if(!attacker){attacker = &g_entities[ENTITYNUM_WORLD];}
	tgClient = target->client;
	if(tgClient && tgClient->noclip){return;}
	if(!dir){dflags |= DAMAGE_NO_KNOCKBACK;}
	else{VectorNormalize(dir);}
	if(knockback > 1000) {knockback = 1000;}
	if(knockback < 0) {knockback = 0;}
	if(target == attacker){knockback = 0;}
	if(target->flags & FL_NO_KNOCKBACK) {knockback = 0;}
	if(dflags & DAMAGE_NO_KNOCKBACK) {knockback = 0;}
	if (knockback && tgClient) {
		vec3_t	kvel;
		VectorScale (dir, knockback, kvel);
		VectorAdd (tgClient->ps.velocity, kvel, tgClient->ps.velocity);
		if (!tgClient->ps.pm_time) {
			tgClient->ps.pm_time = 200;
			tgClient->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			if(!tgClient->ps.lockedTarget){
				tgClient->ps.lockedTarget = attacker->client->ps.clientNum+1;
				tgClient->ps.clientLockedTarget = attacker->client->ps.clientNum+1;
			}
		}
	}
	damageScale = ((float)attacker->client->ps.powerLevel[plMaximum] * 0.0003f) * attacker->client->ps.stats[stEnergyAttack] / tgClient->ps.stats[stEnergyDefense];
	damage *= damageScale;
	if(tgClient){
		//VectorCopy(dir ? dir : target->r.currentOrigin,tgClient->damage_from);
		tgClient->ps.persistant[PERS_ATTACKER] = attacker ? attacker->s.number : ENTITYNUM_WORLD;
		tgClient->damage_fromWorld = dir ? qfalse : qtrue;
		tgClient->lasthurt_client = attacker->s.number;
		tgClient->lasthurt_mod = methodOfDeath;
	}
	if(damage){
		if(tgClient){
			if(target == attacker){damage *= 0.2f;}
			tgClient->ps.powerLevel[plDamageFromEnergy] += damage;
			if ( tgClient->ps.powerLevel[plHealth] <= damage && tgClient->ps.powerLevel[plHealth] > 0 ) {
				target->enemy = attacker;
				target->die (target, inflictor, attacker, damage, methodOfDeath);
				return;
			} else if ( target->pain ) {
				target->pain (target, attacker, damage);
			}
		}
		else{
			target->powerLevelCurrent = target->powerLevelCurrent - damage;
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

		if(CanDamage(ent, origin)){
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_UserWeaponDamage(ent,NULL,attacker,dir,origin,(int)realDamage,DAMAGE_RADIUS,methodOfDeath,extraKnockback);
		}
	}
	return hitClient;
}


/*
   -----------------------------------
     F I R I N G   F U N C T I O N S
   -----------------------------------
*/


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
			if( LogAccuracyHit( traceEnt, self ) ) {
				self->client->accuracy_hits++;
			}

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
		tempEnt2 = G_TempEntity( tr.endpos, EV_RAILTRAIL );
		tempEnt2->s.clientNum = self->s.number;
		tempEnt2->s.weapon = weaponNum;
		VectorCopy( muzzle, tempEnt2->s.origin2 );

		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			return;
		}
		
		if ( traceEnt->takedamage) {
		G_UserWeaponDamage( traceEnt, self, self, forward, tr.endpos,
							weaponInfo->damage_damage, 0,
							MOD_KI + weaponInfo->damage_meansOfDeath,
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
	vec3_t			muzzle, forward, right, up;
	vec3_t			firingDir, firingStart; // <-- Contain the altered aiming and origin vectors.

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
		bolt->isSwattable = weaponInfo->physics_swat;
		bolt->isDrainable = weaponInfo->physics_drain;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[stChargePercentSecondary];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			bolt->powerLevelCurrent = bolt->powerLevelTotal = bolt->damage * (1.0f+(float)bolt->chargelvl / 100.0f);
			bolt->s.dashDir[0] = self->client->ps.attackPowerTotal = bolt->powerLevelTotal; // Use this free field to transfer total power level
			//G_Printf(va("ki cost = %i\n",weaponInfo->costs_ki));
			//G_Printf(va("charge percent = %i\n",self->client->ps.stats[stChargePercentSecondary]));
			self->client->ps.stats[stChargePercentSecondary] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[stChargePercentPrimary];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			bolt->powerLevelCurrent = bolt->powerLevelTotal = bolt->damage * (1.0f+(float)bolt->chargelvl / 100.0f);
			bolt->s.dashDir[0] = self->client->ps.attackPowerTotal = bolt->powerLevelTotal; // Use this free field to transfer total power level
			//G_Printf(va("ki cost = %i\n",weaponInfo->costs_ki));
			//G_Printf(va("charge percent = %i\n",self->client->ps.stats[stChargePercentPrimary]));
			self->client->ps.stats[stChargePercentPrimary] = 0; // Only reset it here!
		}
		
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;
		bolt->takedamage = qtrue;
		bolt->strugglingAllyAttack = qfalse;
		bolt->strugglingAttack = qfalse;
		bolt->strugglingPlayer = qfalse;
		self->s.dashDir[2] = 0.0f;
		bolt->count = 0;
		{
			float radius;
			radius = weaponInfo->physics_radius + weaponInfo->physics_radiusMultiplier * (bolt->chargelvl / 100.0f);
			radius = sqrt((radius * radius) / 3); // inverse of Pythagoras

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
			bolt->gravity = (weaponInfo->physics_gravity / 100) * g_gravity.value; 
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

			{ // Open new block to get some local variables in here
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
		bolt->isSwattable = weaponInfo->physics_swat;
		bolt->isDrainable = weaponInfo->physics_drain;
		if (altfire) {
			bolt->chargelvl = self->client->ps.stats[stChargePercentSecondary];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			bolt->powerLevelCurrent = bolt->powerLevelTotal = bolt->damage * (1.0f+(float)bolt->chargelvl / 100.0f);
			bolt->s.dashDir[0] = self->client->ps.attackPowerTotal = bolt->powerLevelTotal; // Use this free field to transfer total power level
			self->client->ps.stats[stChargePercentSecondary] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[stChargePercentPrimary];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			bolt->powerLevelCurrent = bolt->powerLevelTotal = bolt->damage * (1.0f+(float)bolt->chargelvl / 100.0f);
			bolt->s.dashDir[0] = self->client->ps.attackPowerTotal = bolt->powerLevelTotal; // Use this free field to transfer total power level
			self->client->ps.stats[stChargePercentPrimary] = 0; // Only reset it here!
		}
		
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->takedamage = qtrue;
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
				
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;
		bolt->strugglingAllyAttack = qfalse;
		bolt->strugglingAttack = qfalse;
		bolt->strugglingPlayer = qfalse;
		self->s.dashDir[2] = 0.0f;
		bolt->count = 0;
		{
			float radius;
			radius = weaponInfo->physics_radius + weaponInfo->physics_radiusMultiplier * (bolt->chargelvl / 100.0f);
			radius = sqrt((radius * radius) / 3); // inverse of Pythagoras

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
		// This saves network bandwidth.
		SnapVector( bolt->s.pos.trDelta );
		// move a bit on the very first frame
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( start, bolt->s.pos.trBase );
		VectorCopy( start, bolt->r.currentOrigin );
		VectorCopy( firingDir, bolt->movedir );

		VectorCopy( self->client->ps.viewangles, self->s.angles );
		VectorCopy( self->client->ps.viewangles, bolt->s.angles2 );

		// Set the correct think based on homing properties.
		switch (weaponInfo->homing_type) {
		case HOM_GUIDED:
			bolt->think = Think_Guided;
			bolt->nextthink = level.time + FRAMETIME;

			// Set guidetarget.
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

			// Set guidetarget.
			//bolt->s.eFlags |= EF_GUIDED;
			self->client->guidetarget = bolt;

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
			UserHitscan_Fire( self, weaponInfo, self->s.weapon, firingStart, firingDir );
			self->client->ps.stats[stChargePercentPrimary] = 0; // Only reset it here!
		} else {
			UserHitscan_Fire( self, weaponInfo, self->s.weapon + ALTWEAPON_OFFSET, firingStart, firingDir );
			self->client->ps.stats[stChargePercentSecondary] = 0; // Only reset it here!
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
			bolt->chargelvl = self->client->ps.stats[stChargePercentSecondary];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[stChargePercentSecondary] = 0; // Only reset it here!
		} else {
			bolt->chargelvl = self->client->ps.stats[stChargePercentPrimary];
			bolt->s.powerups = bolt->chargelvl; // Use this free field to transfer chargelvl
			self->client->ps.stats[stChargePercentPrimary] = 0; // Only reset it here!
		}
		
		// FIXME: Hack into the old mod style, since it's still needed for now
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;

		bolt->speed = weaponInfo->physics_speed;
		bolt->accel = 0;
		bolt->s.pos.trType = TR_LINEAR;
		
		VectorScale( firingDir, bolt->speed, bolt->s.pos.trDelta );
		// This saves network bandwidth.
		SnapVector( bolt->s.pos.trDelta );
		// move a bit on the very first frame
		bolt->s.pos.trTime = level.time - MISSILE_PRESTEP_TIME;
		VectorCopy( firingStart, bolt->s.pos.trBase );
		VectorCopy( firingStart, bolt->r.currentOrigin );

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
		bolt = G_Spawn();
		bolt->classname = "user_torch";
		bolt->s.eType = ET_TORCH;

		// Set the properties we need for a torch
		bolt->clipmask = MASK_SHOT;
		bolt->target_ent = NULL;
		bolt->r.ownerNum = self->s.number;
		bolt->s.clientNum = self->s.number;

		// Set the weapon number correct, depending on altfire status.
		if ( !altfire ) {
			bolt->s.weapon = self->s.weapon;
		} else {
			bolt->s.weapon = self->s.weapon + ALTWEAPON_OFFSET;
		}

		// Make sure the entity is in the PVS if you should be able to spot part of the torch
		VectorSet( bolt->r.maxs,  weaponInfo->physics_range_max,  weaponInfo->physics_range_max,  weaponInfo->physics_range_max );
		VectorSet( bolt->r.mins, -weaponInfo->physics_range_max, -weaponInfo->physics_range_max, -weaponInfo->physics_range_max );

		// Hijack homRange and homAngle for the torch range and torch width angle.
		// The latter is built from the physics_radius parameter, which contains
		// the radius of the torch cone at the maximum distance, and the physics_range_max
		// parameter, which contains the maximum range of the torch attack.
		bolt->homRange = Q_fabs(weaponInfo->physics_range_max);
		bolt->homAngle = atan2( bolt->homRange, Q_fabs(weaponInfo->physics_radius) ); // NOTE: Keep this in radians

		// Set the movement type for the torch to interpolation.
		bolt->s.pos.trType = TR_INTERPOLATE;
		bolt->s.apos.trType = TR_INTERPOLATE;
		VectorCopy( start, bolt->s.pos.trBase );
		vectoangles( dir, bolt->s.apos.trBase );

		// Hijack trDelta for getting physics_range_max and physics_radius to the client
		bolt->s.pos.trDelta[0] = Q_fabs(weaponInfo->physics_range_max);
		bolt->s.pos.trDelta[1] = Q_fabs(weaponInfo->physics_radius);
	
		// Set the MoD
		bolt->methodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;
		bolt->splashMethodOfDeath = MOD_KI + weaponInfo->damage_meansOfDeath;

		// Set the think function that will demolish the torch if it persists too long
		bolt->think = Think_Torch;
		bolt->nextthink = weaponInfo->physics_lifetime;
		break;

	case WPT_TRIGGER:
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
				} else if ( trigTarget->timesTriggered >= weaponInfo->firing_nrShots ) {
					continue;
				} else if ( !altfire && ( trigTarget->s.weapon != self->s.weapon + ALTWEAPON_OFFSET )) {
					continue;
				} else if ( altfire && ( trigTarget->s.weapon != self->s.weapon )) {
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
				traceDist = VectorLength( newDir ) + SOLID_ADD;
	
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
					trigTarget->gravity = (weaponInfo->physics_gravity / 100) * g_gravity.value; 
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

void G_DieUserWeapon( gentity_t *self, gentity_t *inflictor,
					  gentity_t *attacker, int damage, int mod ) {
	if (inflictor == self)
		return;
	self->takedamage = qfalse;
	self->think = G_ExplodeUserWeapon;
	self->nextthink = level.time;
}

void G_ExplodeUserWeapon( gentity_t *self ) {
	// Handles actual detonation of the weapon in question.

	vec3_t		dir = { 0, 0, 1};
	vec3_t		origin;

	self->takedamage = qfalse;

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

	if(self->s.eType == ET_BEAMHEAD && (self->strugglingPlayer || self->strugglingAttack)){
		self->enemy->client->ps.bitFlags &= ~isStruggling;
	}

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
	VectorNormalize( fwd );
	self->s.dashDir[1] = missileOwner->client->ps.attackPowerCurrent = self->powerLevelCurrent;
	self->bounceFrac = 0.0f;
	G_HoldUserMissile(self,self->enemy);
	if(self->enemy->client->ps.bitFlags & usingBoost){
		self->powerLevelCurrent -= 4 * self->enemy->client->tiers->energyDefense;
	}else{
		self->powerLevelCurrent -= 2 * self->enemy->client->tiers->energyDefense;
	}
	if((missileOwner->client->ps.bitFlags & usingBoost) && self->s.eType == ET_BEAMHEAD){
		self->powerLevelCurrent += 1 + ((float)self->powerLevelTotal * 0.005f);
		self->speed += 10 + ((float)missileOwner->client->ps.powerLevel[plMaximum] * 0.0003f);
	}
	// If the missile has lost over 1/3 its power against the player
	if(self->powerLevelCurrent < (self->powerLevelTotal / 1.5f)){
		self->strugglingPlayer = qfalse;
		self->bounceFrac = 1.0f;
		G_PushUserMissile(self, self->enemy);
		self->enemy->client->ps.bitFlags &= ~isStruggling;
		//G_Printf("Missile Lost to %i!\n",self->enemy->s.clientNum);
		if(self->enemy->client->ps.lockedTarget >= MAX_CLIENTS){
			self->enemy->client->ps.lockedPosition = NULL;
			self->enemy->client->ps.lockedTarget = 0;
			self->enemy->client->ps.clientLockedTarget = 0;
		}
		// Turn the beam into a ball attack.
		if(self->s.eType == ET_BEAMHEAD){
			// Terminate guidance
			if (self->guided){
				g_entities[self->s.clientNum].client->ps.weaponstate = WEAPON_READY;
			}
			self->s.eType = ET_MISSILE;
		}
		return;
		//G_Printf("Missile Losing to %i!\n",self->enemy->s.clientNum);
	}
	// If the missile is beating the player
	else if(self->enemy->client->ps.powerLevel[plCurrent] <= 1){
		G_ExplodeUserWeapon ( self );
		self->strugglingPlayer = qfalse;
		self->enemy->client->ps.bitFlags &= ~isStruggling;
		//G_Printf("Missile Won against %i!\n",self->enemy->s.clientNum);
		if(self->enemy->client->ps.lockedTarget >= MAX_CLIENTS){
			self->enemy->client->ps.lockedPosition = NULL;
			self->enemy->client->ps.lockedTarget = 0;
			self->enemy->client->ps.clientLockedTarget = 0;
		}
		return;
	}
	//G_Printf("Missile Power Level %i. Player Power Level %i\n",self->powerLevelCurrent,self->enemy->client->ps.powerLevel[plCurrent]);
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
	
	// Get a vector aiming from the client to the bullet hit 
	VectorSubtract(targ->r.currentOrigin, point, attackPath); 
	// Convert it into PITCH, ROLL, YAW
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
	other = &g_entities[trace->entityNum];
	G_LocationImpact(trace->endpos,other,GetMissileOwnerEntity(self));
	// If the attack is already in a struggle with a player,
	// or the attack is helping an allies attack and the ally is struggling, ignore interation with anything.
	if(self->strugglingPlayer || (self->strugglingAllyAttack && self->ally->strugglingAttack)){return;}
	// Initiate Power Struggle
	if((other->s.eType == ET_MISSILE || other->s.eType == ET_BEAMHEAD)	// If it's a beam or ball attack
		&& !other->client												// And it's not a player
		&& !self->strugglingAttack										// And we are not struggling an attack
		&& !self->strugglingAllyAttack									// And we are not helping an ally
		&& !other->strugglingAttack										// And the other attack isn't struggling with some other attack
		&& !other->strugglingPlayer){									// And the other attack isn't struggling with a player
		self->enemy = other;
		other->enemy = self;
		self->strugglingAttack = qtrue;
		self->enemy->strugglingAttack = qtrue;
		self->s.dashDir[2] = 1.0f;
		self->enemy->s.dashDir[2] = 1.0f;
		if(self->s.eType == ET_BEAMHEAD){
			self->client->ps.bitFlags |= isStruggling;
		}
		if(other->s.eType == ET_BEAMHEAD){
			other->client->ps.bitFlags |= isStruggling;
		}
		if(self->s.eFlags & EF_GUIDED){
			self->s.eFlags &= ~EF_GUIDED;
		}
		if(other->s.eFlags & EF_GUIDED){
			other->s.eFlags &= ~EF_GUIDED;
		}
		G_AddEvent( self, EV_POWER_STRUGGLE_START, DirToByte( trace->plane.normal ) );
		//G_Printf("Started Power Struggle!\n");
		return;
	// Initiate attack absorbtion
	} else if ((other->s.eType == ET_MISSILE || other->s.eType == ET_BEAMHEAD)	// If it's a beam or ball attack
		&& !other->client														// And it's not a player
		&& !self->strugglingAttack												// And we are not struggling an attack
		&& !self->strugglingAllyAttack											// And we are not helping an ally
		&& other->strugglingAttack){											// And the other attack IS struggling with some other attack
		// If our attack and the other attack is on the same team
		if ( self->client->ps.persistant[PERS_TEAM] == other->client->ps.persistant[PERS_TEAM] && self->client->ps.persistant[PERS_TEAM] != TEAM_FREE ) {
			// Get absorbed by our enemies enemy, which should be someone on our team.
			self->ally = other->enemy;
		// Else get absorbed by whichever attack you hit first.
		}else{
			self->ally = other;
		}
		self->r.ownerNum = self->ally->s.number;
		self->strugglingAllyAttack = qtrue;
		//G_Printf("Attack Absorbtion Started!\n");
		return;
	// Initiate Swat / Push Struggle
	} else if ((other->s.eType != ET_MISSILE || other->s.eType != ET_BEAMHEAD)	// If it's not a beam or ball attack
		&& other->client														// If it's a player
		&& (other->client->ps.bitFlags & usingBlock)							// And they pushed block
		&& !(other->client->ps.bitFlags & isStruggling)							// And they are not struggling something
		&& !self->strugglingAttack												// And we are not struggling an attack
		&& !self->strugglingAllyAttack											// And we are not helping an ally
		&& other->client->lasthurt_location == LOCATION_FRONT){					// And we hit the front of the player
		if(self->isSwattable){
			self->bounceFrac = 1.0f;
			G_PushUserMissile(self, other);
			//G_Printf("Attack Deflected!\n");
			return;
		}else{
			self->strugglingPlayer = qtrue;
			other->client->ps.bitFlags |= isStruggling;
			self->enemy = other;
			// If the player isn't already locked onto another player, lock onto the attack.
			if(!other->client->ps.lockedTarget){
				other->client->ps.lockedTarget = self->s.number;
				other->client->ps.clientLockedTarget = self->s.number;
				other->client->ps.lockedPosition = &self->r.currentOrigin;
			}
			//G_Printf("Missile Started Struggle with %i!\n",other->s.clientNum);
			return;
		}
	// Bounce off the world
	} else if (self->bounceFrac   
		&& self->bouncesLeft 
		&& !other->takedamage 
		&& !self->strugglingAttack 
		&& !self->strugglingAllyAttack 
		&& !other->strugglingAttack){
		G_BounceUserMissile( self, trace );
		G_AddEvent( self, EV_GRENADE_BOUNCE, 0 );
		self->bouncesLeft--;
		//G_Printf("Attack Bounced!\n");
		return;
	// Hit a player or the world
	} else if(other->client || !other->takedamage){ 
		self->takedamage = qtrue;
		if(other->takedamage && self->damage) {
			BG_EvaluateTrajectoryDelta(&self->s, &self->s.pos, level.time, velocity);
			if(VectorLength( velocity ) == 0){
				velocity[2] = 1;
			}
			G_UserWeaponDamage(other,self,GetMissileOwnerEntity(self),velocity,self->s.origin,self->powerLevelCurrent,0,self->methodOfDeath, self->extraKnockback);
			//G_Printf("Hit Player!\n");
		}
		if((self->powerLevelCurrent <= 0 || !(other->takedamage)) || (other->s.eType == ET_PLAYER)){
			// Terminate guidance
			if(self->guided){g_entities[self->s.clientNum].client->ps.weaponstate = WEAPON_READY;}
			if(other->takedamage && other->client){
				G_AddEvent( self, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
				self->s.otherEntityNum = other->s.number;
			}
			else if(trace->surfaceFlags & SURF_METALSTEPS){
				G_AddEvent( self, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
				//G_Printf("Hit World!\n");
			}
			else{
				G_AddEvent( self, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
				//G_Printf("Hit World!\n");
			}
			self->freeAfterEvent = qtrue;
			// Change over to a normal stationary entity right at the point of impact
			self->s.eType = ET_GENERAL;
			SnapVectorTowards( trace->endpos, self->s.pos.trBase );	// save net bandwidth
			G_SetOrigin( self, trace->endpos );
			if( G_UserRadiusDamage( trace->endpos, GetMissileOwnerEntity(self), self, self->powerLevelCurrent, self->splashRadius, self->methodOfDeath, self->extraKnockback )) {
				if( !hitClient ) {
					g_entities[self->s.clientNum].client->accuracy_hits++;
				}
			}
			trap_LinkEntity( self );
		}
	}
}

// Wrapper function to safely remove a user weapon, incase
// things like fading trails need to be handled.
void G_RemoveUserWeapon (gentity_t *self) {
	gentity_t *other;
/*
	if(self->strugglingAttack || self->strugglingPlayer){
		return;
	}
*/
	other = self->enemy;
	other->client->ps.bitFlags &= ~isStruggling;
	self->client->ps.bitFlags &= ~isStruggling;
	if(other->client->ps.lockedTarget >= MAX_CLIENTS){
		other->client->ps.lockedPosition = NULL;
		other->client->ps.lockedTarget = 0;
		other->client->ps.clientLockedTarget = 0;
	}
	if (self->guided) {
		g_entities[ self->s.clientNum ].client->ps.weaponstate = WEAPON_READY;
	}
	if (self->s.eType == ET_BEAMHEAD && self->powerLevelCurrent > 0 && (!self->strugglingAllyAttack || !self->strugglingAttack || !self->strugglingPlayer)) {
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

	// get current position
	BG_EvaluateTrajectory( &ent->s, &ent->s.pos, level.time, origin );

	// attacks that left the owner bbox will hit anything, even the owner
	if (ent->count) {
		pass_ent = ent->s.number;//ENTITYNUM_NONE;
	// if the attack is in any type of struggle, ignore interaction with the attacker but hit the owner.
	} else if (ent->strugglingPlayer || ent->strugglingAttack || ent->strugglingAllyAttack){
		pass_ent = ent->s.number;
	// ignore interactions with the missile owner
	} else {
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

		if ((ent->s.eType != ET_MISSILE) && (ent->s.eType != ET_BEAMHEAD)/* && ((ent->powerLevelCurrent <= 0)||(g_entities[trace.entityNum].client->ps.powerLevel[plHealth] <=0)) */) {
			// Missile has changed to ET_GENERAL and has exploded, so we don't want
			// to run the think function!
			// Return immediately instead.
			return;	
		}
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
			beamKnockBack = ent->powerLevelCurrent < (ent->powerLevelTotal / 3.0f) ? 0 : 100;
			G_UserWeaponDamage(traceEnt2,ent,missileOwner,forward,trace2.endpos,1+(ent->powerLevelCurrent / 4),0,ent->methodOfDeath,beamKnockBack);
		// Else an attack entered our beam
		// FIXME: This block won't work unless you comment out self->strugglingAllyAttack in the main struggle function.
		// Even then, it still dosn't work right.
		}
		/*
		else if (!traceEnt2->client && traceEnt2->takedamage && trace2.fraction != 1){
			// If our attack and the other attack is on the same team
			if (traceEnt2->client->ps.persistant[PERS_TEAM] == ent->client->ps.persistant[PERS_TEAM] && traceEnt2->client->ps.persistant[PERS_TEAM] != TEAM_FREE ) {
				// Get absorbed by our enemies enemy, which should be someone on our team.
				traceEnt2->ally = ent->enemy;
			// Else get absorbed by whichever attack you hit first.
			}else{
				traceEnt2->ally = ent;
			}
			traceEnt2->r.ownerNum = traceEnt2->ally->s.number;
			traceEnt2->strugglingAllyAttack = qtrue;
		}
		*/
		if ((ent->s.eType != ET_MISSILE) && (ent->s.eType != ET_BEAMHEAD)/* && ((ent->powerLevelCurrent <= 0)||(g_entities[trace2.entityNum].client->ps.powerLevel[plHealth] <=0)) */) {
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


#define RIFT_MAX_GROUNDLEVEL_VARIATION 25 // Maximum fluctuation of 25 game units positive/negative in the height
#define RIFT_DEGRADE_TIME	1000 //msec
#define RIFT_HEIGHT			200
void G_RunRiftFrame( gentity_t *ent, int time ) {
	vec3_t		origin, traceTarget;	
	trace_t		trace;
	float		up;
	int			pass_ent;

	if ( time > level.time ) return;
	if ( time < (level.time - RIFT_DEGRADE_TIME)) return;

	pass_ent = ent->r.ownerNum;

	// get origin and target point
	BG_EvaluateTrajectory( &ent->s, &ent->s.pos, time, origin );
	VectorCopy( origin, traceTarget );
	traceTarget[2] -= RIFT_MAX_GROUNDLEVEL_VARIATION;
	origin[2] += RIFT_MAX_GROUNDLEVEL_VARIATION;

	// trace a line between the valid vertical margin for the rift
	trap_Trace( &trace, origin, NULL, NULL, traceTarget, ENTITYNUM_NONE, MASK_SOLID );

	if ( trace.allsolid ) {
		// The virtual piercing missile ended up staying inside a volume.
		// The missile is removed.
		G_RemoveUserWeapon( ent );
	}
	else if (time == level.time) {
		VectorCopy( origin, ent->r.currentOrigin );
		ent->r.currentOrigin[2] -= RIFT_MAX_GROUNDLEVEL_VARIATION;
	}

	// trace upwards from the point of origin to do damge,
	// trace length modified by timeframe using second degree polynomial
	VectorCopy( trace.endpos, origin );
	VectorAdd( origin, trace.plane.normal, origin );
	up = 2.0f * (float)(level.time - time ) / (float)RIFT_DEGRADE_TIME - 1;
	up = -1 * (up * up) + 1;
	up *= RIFT_HEIGHT; // FIXME: Must be replaced by weapon script based field...
	
	VectorCopy( origin, traceTarget );
	traceTarget[2] += up;
	trap_Trace( &trace, origin, NULL, NULL, traceTarget, pass_ent, ent->clipmask );

	if ( trace.startsolid || trace.allsolid ) {
		// make sure the trace.entityNum is set to the entity we're stuck in
		trap_Trace( &trace, ent->r.currentOrigin, NULL, NULL, ent->r.currentOrigin, pass_ent, ent->clipmask );
		trace.fraction = 0;
	}

	if ( trace.fraction != 1 ) {
		// we hit something, now see if we can damage it
		gentity_t	*traceEnt;
		vec3_t		upVec = { 0, 0, 1 };

		traceEnt = &g_entities[ trace.entityNum ];

		if ( traceEnt->takedamage) {

			G_UserWeaponDamage( traceEnt, &g_entities[ent->r.ownerNum], ent, upVec,
								trace.endpos, ent->damage, 0, ent->methodOfDeath, 0 );
		}
	}
}

void G_RunRiftWeaponClass( gentity_t *ent ) {
	int i;

	for ( i = level.time - RIFT_DEGRADE_TIME; i <= level.time; i += 100 ) {
		G_RunRiftFrame( ent, i );
	}
	
}


void G_RunUserTorch( gentity_t *ent ) {
	gentity_t	*owner, *target;
	vec3_t		forward, right, up, muzzle, targetDir, torchEnd;
	trace_t		trace;
	int			i;
	float		targetRange, torchRange, targetAngle;

	owner = &g_entities[ent->r.ownerNum];

	// If the owner no longer is firing the weapon
	if ( owner->client->ps.weaponstate != WEAPON_FIRING ) {
		G_FreeEntity( ent );
		return;
	}

	// Calculate where we are now aiming.
	AngleVectors( owner->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( owner, forward, right, up, muzzle );

	// Give the entity the new aiming settings
	VectorCopy( muzzle, ent->s.pos.trBase );
	vectoangles( forward, ent->s.apos.trBase );

	// Get the new end point of the torch at the maximum distance
	VectorMA( ent->s.pos.trBase, ent->homRange, forward, torchEnd );

	// Clamp the end point, so we don't shoot torches straight through walls, but only locally.
	trap_Trace( &trace, muzzle, NULL, NULL, torchEnd, ent->r.ownerNum, MASK_SOLID );
	torchRange = Distance( trace.endpos, muzzle );
	
	// Do damage to any clients stuck inside the blast, not including the owner client.
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		float dmgFrac;

		if ( ent->r.ownerNum == i ) {
			continue;
		}
		
		target = &g_entities[i];
		VectorSubtract( target->s.pos.trBase, muzzle, targetDir );
		targetRange = VectorNormalize( targetDir );
		if ( targetRange > torchRange ) {
			continue;
		}

		targetAngle = hack_acos( DotProduct( targetDir, forward ));
		if ( ent->homAngle < targetAngle ) {
			continue;
		}

		if ( ent->homAngle ) {
			dmgFrac = 1 - (targetAngle / ent->homAngle);

			// Making sure floating point rounding errors don't cause problems
			if ( dmgFrac < 0 ) {
				dmgFrac = 0;
			}
		} else {
			dmgFrac = 1;
		}
		
		G_UserWeaponDamage( target, owner, ent, forward, muzzle, dmgFrac * ent->damage, 0, ent->methodOfDeath, 0 );
	}

	// Round off stored vectors
	SnapVector( ent->s.pos.trBase );
	SnapVector( ent->s.apos.trBase );
}

