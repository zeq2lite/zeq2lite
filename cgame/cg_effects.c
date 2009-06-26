// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_effects.c -- these functions generate localentities, usually as a result
// of event processing

#include "cg_local.h"


/*
==================
CG_BubbleTrail

Bullets shot underwater
==================
*/
void CG_BubbleTrail( vec3_t start, vec3_t end, float spacing ) {
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// advance a random amount first
	i = rand() % (int)spacing;
	VectorMA( move, i, vec, move );

	VectorScale (vec, spacing, vec);

	for ( ; i < len; i += spacing ) {
		localEntity_t	*le;
		refEntity_t		*re;

		le = CG_AllocLocalEntity();
		le->leFlags = LEF_PUFF_DONT_SCALE;
		le->leType = LE_MOVE_SCALE_FADE;
		le->startTime = cg.time;
		le->endTime = cg.time + 1000 + random() * 250;
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );

		re = &le->refEntity;
		re->shaderTime = cg.time / 1000.0f;

		re->reType = RT_SPRITE;
		re->rotation = 0;
		re->radius = 3;
		re->customShader = cgs.media.waterBubbleShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		le->color[3] = 1.0;

		le->pos.trType = TR_LINEAR;
		le->pos.trTime = cg.time;
		VectorCopy( move, le->pos.trBase );
		le->pos.trDelta[0] = crandom()*5;
		le->pos.trDelta[1] = crandom()*5;
		le->pos.trDelta[2] = crandom()*5 + 6;

		VectorAdd (move, vec, move);
	}
}

/*
=====================
CG_SmokePuff

Adds a smoke puff or blood trail localEntity.
=====================
*/
localEntity_t *CG_SmokePuff( const vec3_t p, const vec3_t vel, 
				   float radius,
				   float r, float g, float b, float a,
				   float duration,
				   int startTime,
				   int fadeInTime,
				   int leFlags,
				   qhandle_t hShader ) {
	static int	seed = 0x92;
	localEntity_t	*le;
	refEntity_t		*re;
//	int fadeInTime = startTime + duration / 2;

	le = CG_AllocLocalEntity();
	le->leFlags = leFlags;
	le->radius = radius;

	re = &le->refEntity;
	re->rotation = Q_random( &seed ) * 360;
	re->radius = radius;
	re->shaderTime = startTime / 1000.0f;

	le->leType = LE_MOVE_SCALE_FADE;
	le->startTime = startTime;
	le->fadeInTime = fadeInTime;
	le->endTime = startTime + duration;
	if ( fadeInTime > startTime ) {
		le->lifeRate = 1.0 / ( le->endTime - le->fadeInTime );
	}
	else {
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	}
	le->color[0] = r;
	le->color[1] = g; 
	le->color[2] = b;
	le->color[3] = a;


	le->pos.trType = TR_LINEAR;
	le->pos.trTime = startTime;
	VectorCopy( vel, le->pos.trDelta );
	VectorCopy( p, le->pos.trBase );

	VectorCopy( p, re->origin );
	re->customShader = hShader;

	// rage pro can't alpha fade, so use a different shader
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		re->customShader = cgs.media.smokePuffRageProShader;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;
	} else {
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;
	}

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	return le;
}

/*
==================
CG_SpawnEffect

Player teleporting in or out
==================
*/
void CG_SpawnEffect( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_FADE_ALPHA;
	le->startTime = cg.time;
	le->endTime = cg.time + 250;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->radius = 64;

	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;

	re->reType = RT_SPRITE;
	re->radius = le->radius;

	re->shaderRGBA[0] = le->color[0] * 0xff;
	re->shaderRGBA[1] = le->color[1] * 0xff;
	re->shaderRGBA[2] = le->color[2] * 0xff;
	re->shaderRGBA[3] = 0xff;

	re->customShader = cgs.media.teleportEffectShader;


	AxisClear( re->axis );

	VectorCopy( org, re->origin );

	re->origin[2] += 16;
}

/*
==================
CG_LightningEffect

Player lightning effect.
==================
*/
void CG_LightningEffect( vec3_t org, clientInfo_t *ci, int tier ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int r,r1,r2,r3,r4,r5,r6;
	r = random() * 60;
	r1 = random() * 24 + 8;
	r2 = random() * 24 + 8;
	r3 = random() * 32 + 16;
	r4 = random() * 24 + 8;
	r5 = random() * 24 + 8;
	r6 = random() * 40;
	if (r > 58) {
		localEntity_t	*le;
		refEntity_t		*re;

		le = CG_AllocLocalEntity();

		le->leFlags = 0;
		le->leType = LE_FADE_ALPHA;
		le->startTime = cg.time;
		le->endTime = cg.time + 250;
		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
		le->radius = 16;
		le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

		re = &le->refEntity;

		re->reType = RT_SPRITE;
		re->radius = le->radius;
		re->shaderRGBA[0] = 0xff;
		re->shaderRGBA[1] = 0xff;
		re->shaderRGBA[2] = 0xff;
		re->shaderRGBA[3] = 0xff;

		re->customShader = ci->auraConfig[tier]->lightningShader;

		AxisClear( re->axis );

		VectorCopy( org, re->origin );

		re->origin[0] += r1;
		re->origin[1] += r2;
		re->origin[2] += r3;
		re->origin[0] -= r4;
		re->origin[1] -= r5;
		re->origin[2] -= r6;

		if ((random() * 7)< 1){
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound1 );
		}else if ((random() * 7) < 2){
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound2 );
		}else if ((random() * 7) < 3){
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound3 );
		}else if ((random() * 7) < 4){
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound4 );
		}else if ((random() * 7) < 5){
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound5 );
		}else if ((random() * 7) < 6){
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound6 );
		} else {
			trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound7 );
		}
	}
}

/*
==================
CG_BigLightningEffect

Lightning sparks for high tiers.
==================
*/
void CG_BigLightningEffect( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int r,s;

	r = random() * 200;

	if ((r >= 1) && (r <= 198)){
		return;
	}

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->startTime = cg.time;
	le->leType = LE_FADE_NO;
	le->endTime = cg.time + 200;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = 128;
	le->light = 200;
	le->lightColor[0] = 1.0;
	le->lightColor[1] = 1.0;
	le->lightColor[2] = 1.0;
	le->lightColor[3] = 1.0;

	re = &le->refEntity;
	re->reType = RT_SPRITE;
	re->radius = le->radius;
	re->shaderRGBA[0] = le->color[0] * 0xff;
	re->shaderRGBA[1] = le->color[1] * 0xff;
	re->shaderRGBA[2] = le->color[2] * 0xff;
	re->shaderRGBA[3] = 0xff;

	AxisClear( re->axis );
	VectorCopy( org, re->origin );
	re->origin[2] += 16;

	if ( r < 2 ) {
		re->customShader = cgs.media.auraLightningSparks1;
	}
	if ( r > 198) {
		re->customShader = cgs.media.auraLightningSparks2;
	}

	if ((random() > 0) && (random() < 0.1)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound1 );
	}
	if ((random() > 0.1) && (random() < 0.2)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound2 );
	}
	if ((random() > 0.2) && (random() < 0.3)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound3 );
	}
	if ((random() > 0.3) && (random() < 0.4)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound4 );
	}
	if ((random() > 0.4) && (random() < 0.5)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound5 );
	}
	if ((random() > 0.5) && (random() < 0.6)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound6 );
	}
	if ((random() > 0.6) && (random() < 0.7)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound7 );
	}
	if ((random() > 0.7) && (random() < 0.8)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound1 );
	}
	if ((random() > 0.8) && (random() < 0.9)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound2 );
	}
	if ((random() > 0.9) && (random() < 1)){
		trap_S_StartSound( org, ENTITYNUM_NONE, CHAN_AUTO, cgs.media.bigLightningSound3 );
	}
	//CG_Printf("%f\n",random());
}

/*
==================
CG_SpeedMeleeEffect

Player receiving hit melee effect
==================
*/
void CG_SpeedMeleeEffect( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int r,r1,r2,r3,r4,r5,r6;

	r = random() * 60;
	r1 = random() * 24 + 8;
	r2 = random() * 24 + 8;
	r3 = random() * 32 + 16;
	r4 = random() * 24 + 8;
	r5 = random() * 24 + 8;
	r6 = random() * 40;

	if (r > 50) {
		le = CG_AllocLocalEntity();
		le->leFlags = 0;
		le->startTime = cg.time;
		le->leType = LE_SCALE_FADE_RGB;
		le->endTime = cg.time + 100;

		if (r > 75){
			le->radius = 64;
		}else{
			le->radius = 32;
		}

		le->lifeRate = 1.0 / ( le->endTime - le->startTime );
		le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

		re = &le->refEntity;
		re->reType = RT_SPRITE;
		re->radius = le->radius;
		re->shaderRGBA[0] = le->color[0] * 0xff;
		re->shaderRGBA[1] = le->color[1] * 0xff;
		re->shaderRGBA[2] = le->color[2] * 0xff;
		re->shaderRGBA[3] = 0xff;

		re->customShader = cgs.media.meleeSpeedEffectShader;

		AxisClear( re->axis );

		VectorCopy( org, re->origin );

		re->origin[0] += r1;
		re->origin[1] += r2;
		re->origin[2] += r3;
		re->origin[0] -= r4;
		re->origin[1] -= r5;
		re->origin[2] -= r6;
	}
}

/*
==================
CG_PowerMeleeEffect

Player receiving hit melee effect
==================
*/
void CG_PowerMeleeEffect( vec3_t org ) {
	localEntity_t	*le;
	refEntity_t		*re;
	int r,r1,r2,r3,r4,r5,r6;

	r1 = random() * 24 + 8;
	r2 = random() * 24 + 8;
	r3 = random() * 32 + 16;
	r4 = random() * 24 + 8;
	r5 = random() * 24 + 8;
	r6 = random() * 40;

	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->startTime = cg.time;
	le->leType = LE_SCALE_FADE;
	le->endTime = cg.time + 250;
	le->radius = 64;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;

	re = &le->refEntity;
	re->reType = RT_SPRITE;
	re->radius = le->radius;
	re->shaderRGBA[0] = le->color[0] * 0xff;
	re->shaderRGBA[1] = le->color[1] * 0xff;
	re->shaderRGBA[2] = le->color[2] * 0xff;
	re->shaderRGBA[3] = 0xff;
	re->customShader = cgs.media.meleePowerEffectShader;

	AxisClear( re->axis );

	VectorCopy( org, re->origin );

	re->origin[0] += r1;
	re->origin[1] += r2;
	re->origin[2] += r3;
	re->origin[0] -= r4;
	re->origin[1] -= r5;
	re->origin[2] -= r6;
}

/*
==================
CG_ScorePlum
==================
*/
void CG_ScorePlum( int client, vec3_t org, int score ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t	angles;
	static vec3_t lastPos;
	if (/*client != cg.predictedPlayerState.clientNum || */cg_scorePlum.integer == 0) {return;}
	le = CG_AllocLocalEntity();
	le->leFlags = 0;
	le->leType = LE_SCOREPLUM;
	le->startTime = cg.time;
	le->endTime = cg.time + 5000;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );
	le->color[0] = le->color[1] = le->color[2] = le->color[3] = 1.0;
	le->radius = score;
	VectorCopy( org, le->pos.trBase );
	if (org[2] >= lastPos[2] - 10 && org[2] <= lastPos[2] + 10) {
		le->pos.trBase[2] -= 10;
	}
	VectorCopy(org, lastPos);
	re = &le->refEntity;
	re->reType = RT_SPRITE;
	re->radius = 16;
	VectorClear(angles);
	AnglesToAxis( angles, re->axis );
}


/*
====================
CG_MakeExplosion
====================
*/
localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir, 
								qhandle_t hModel, qhandle_t shader,
								int msec, qboolean isSprite ) {
	float			ang;
	localEntity_t	*ex;
	int				offset;
	vec3_t			tmpVec, newOrigin;

	if ( msec <= 0 ) {
		CG_Error( "CG_MakeExplosion: msec = %i", msec );
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	if ( isSprite ) {
		ex->leType = LE_SPRITE_EXPLOSION;

		// randomly rotate sprite orientation
		ex->refEntity.rotation = rand() % 360;
		VectorScale( dir, 16, tmpVec );
		VectorAdd( tmpVec, origin, newOrigin );
	} else {
		ex->leType = LE_EXPLOSION;
		VectorCopy( origin, newOrigin );

		// set axis with random rotate
		if ( !dir ) {
			AxisClear( ex->refEntity.axis );
		} else {
			ang = rand() % 360;
			VectorCopy( dir, ex->refEntity.axis[0] );
			RotateAroundDirection( ex->refEntity.axis, ang );
		}
	}

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	ex->refEntity.customShader = shader;

	// set origin
	VectorCopy( newOrigin, ex->refEntity.origin );
	VectorCopy( newOrigin, ex->refEntity.oldorigin );

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
=================
CG_Bleed

This is the spurt of blood when a character gets hit
=================
*/
void CG_Bleed( vec3_t origin, int entityNum ) {
	localEntity_t	*ex;

	if ( !cg_blood.integer ) {
		return;
	}

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 500;
	
	VectorCopy ( origin, ex->refEntity.origin);
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = rand() % 360;
	ex->refEntity.radius = 24;

	ex->refEntity.customShader = cgs.media.bloodExplosionShader;

	// don't show player's own blood in view
	if ( entityNum == cg.snap->ps.clientNum ) {
		ex->refEntity.renderfx |= RF_THIRD_PERSON;
	}
}



/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchGib( vec3_t origin, vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 5000 + random() * 3000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.6f;

	le->leBounceSoundType = LEBS_BLOOD;
	le->leMarkType = LEMT_BLOOD;
}

/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
#define	GIB_VELOCITY	250
#define	GIB_JUMP		250
void CG_GibPlayer( vec3_t playerOrigin ) {
	vec3_t	origin, velocity;

	if ( !cg_blood.integer ) {
		return;
	}

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	if ( rand() & 1 ) {
		CG_LaunchGib( origin, velocity, cgs.media.gibSkull );
	} else {
		CG_LaunchGib( origin, velocity, cgs.media.gibBrain );
	}

	// allow gibs to be turned off for speed
	if ( !cg_gibs.integer ) {
		return;
	}

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibAbdomen );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibArm );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibChest );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibFist );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibFoot );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibForearm );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibIntestine );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibLeg );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*GIB_VELOCITY;
	velocity[1] = crandom()*GIB_VELOCITY;
	velocity[2] = GIB_JUMP + crandom()*GIB_VELOCITY;
	CG_LaunchGib( origin, velocity, cgs.media.gibLeg );
}

/*
==================
CG_LaunchGib
==================
*/
void CG_LaunchExplode( vec3_t origin, vec3_t velocity, qhandle_t hModel ) {
	localEntity_t	*le;
	refEntity_t		*re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 10000 + random() * 6000;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );
	re->hModel = hModel;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.1f;

	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

#define	EXP_VELOCITY	100
#define	EXP_JUMP		150
/*
===================
CG_GibPlayer

Generated a bunch of gibs launching out from the bodies location
===================
*/
void CG_BigExplode( vec3_t playerOrigin ) {
	vec3_t	origin, velocity;

	if ( !cg_blood.integer ) {
		return;
	}

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY;
	velocity[1] = crandom()*EXP_VELOCITY;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*1.5;
	velocity[1] = crandom()*EXP_VELOCITY*1.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*2.0;
	velocity[1] = crandom()*EXP_VELOCITY*2.0;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );

	VectorCopy( playerOrigin, origin );
	velocity[0] = crandom()*EXP_VELOCITY*2.5;
	velocity[1] = crandom()*EXP_VELOCITY*2.5;
	velocity[2] = EXP_JUMP + crandom()*EXP_VELOCITY;
	CG_LaunchExplode( origin, velocity, cgs.media.smoke2 );
}


/*
======================
CG_MakeUserExplosion
======================
*/
void CG_MakeUserExplosion( vec3_t origin, vec3_t dir, cg_userWeapon_t *weaponGraphics) {
	float			angle, start, end;
	localEntity_t	*expShell;
	localEntity_t	*expShock;
	int				offset;
	vec3_t			tmpVec, newOrigin;

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;	

	if ( !(weaponGraphics->explosionModel && weaponGraphics->explosionSkin) ) {
		if (weaponGraphics->explosionShader) {
			// allocate the entity as a sprite explosion
			expShell = CG_AllocLocalEntity();
			expShell->leFlags = 0;
			expShell->leType = LE_SPRITE_EXPLOSION;

			// set the type as sprite and link the image
			expShell->refEntity.reType = RT_SPRITE;
			expShell->refEntity.customShader = weaponGraphics->explosionShader;
			expShell->refEntity.radius = 4 * weaponGraphics->explosionSize;

			// randomly rotate sprite orientation
			expShell->refEntity.rotation = rand() % 360;
			
			// set origin
			VectorScale( dir, 16, tmpVec );
			VectorAdd( tmpVec, origin, newOrigin );
			VectorCopy( newOrigin, expShell->refEntity.origin );
			VectorCopy( newOrigin, expShell->refEntity.oldorigin );

			// set the explosion's duration
			expShell->startTime = cg.time - offset;
			expShell->endTime = expShell->startTime + weaponGraphics->explosionTime;
			expShell->lifeRate = 1.0 / ( expShell->endTime - expShell->startTime );

			// create a camera shake
			CG_AddEarthquake( origin, 
				weaponGraphics->explosionSize / 2 
				, weaponGraphics->explosionTime / 1000
				, ( weaponGraphics->explosionTime / 1000 ) / 2
				, ( weaponGraphics->explosionTime / 1000 ) * 2
				, weaponGraphics->explosionSize / 2 );

			if ( weaponGraphics->explosionDlightRadius >= 50 )
			{
				start = weaponGraphics->explosionDlightRadius * 100;
				end = weaponGraphics->explosionDlightRadius * 1000;

				// start, end, red, green, blue, opacity, mode, hint.
				trap_R_AddFogToScene( start, end, 0, 0, 0, expShell->lifeRate, 2, 2 );
			}

			// bias the time so all shader effects start correctly
			expShell->refEntity.shaderTime = expShell->startTime / 1000.0f;

			// set the Dlight RGB values
			expShell->lightColor[0] = weaponGraphics->explosionDlightColor[0];
			expShell->lightColor[1] = weaponGraphics->explosionDlightColor[1];
			expShell->lightColor[2] = weaponGraphics->explosionDlightColor[2];
			expShell->light = weaponGraphics->explosionDlightRadius * 100;
		}
	} else {
		// allocate the entity as a ZEQ explosion
		expShell = CG_AllocLocalEntity();
		expShell->leFlags = 0;
		expShell->leType = LE_ZEQEXPLOSION;

		// set the type as model and link the model and skin
		expShell->refEntity.reType = RT_MODEL;
		expShell->refEntity.hModel = weaponGraphics->explosionModel;
		expShell->refEntity.customSkin = weaponGraphics->explosionSkin;
		
		// set axis with random rotate
		if ( !dir ) {
			AxisClear( expShell->refEntity.axis );
		} else {
			angle = rand() % 360;
			VectorCopy( dir, expShell->refEntity.axis[0] );
			RotateAroundDirection( expShell->refEntity.axis, angle );
		}

		// scale axes to explosion's full size
		expShell->refEntity.nonNormalizedAxes = qtrue;
		VectorNormalize(expShell->refEntity.axis[0]);
		VectorNormalize(expShell->refEntity.axis[1]);
		VectorNormalize(expShell->refEntity.axis[2]);
		VectorScale(expShell->refEntity.axis[0], weaponGraphics->explosionSize, expShell->refEntity.axis[0]);
		VectorScale(expShell->refEntity.axis[1], weaponGraphics->explosionSize, expShell->refEntity.axis[1]);
		VectorScale(expShell->refEntity.axis[2], weaponGraphics->explosionSize, expShell->refEntity.axis[2]);

		// set origin
		VectorCopy( origin, newOrigin );
		VectorCopy( newOrigin, expShell->refEntity.origin );
		VectorCopy( newOrigin, expShell->refEntity.oldorigin );

		// set the explosion's duration
		expShell->startTime = cg.time - offset;
		expShell->endTime = expShell->startTime + weaponGraphics->explosionTime;
		expShell->lifeRate = 1.0 / ( expShell->endTime - expShell->startTime );

		// create a camera shake
		CG_AddEarthquake( origin, 
			weaponGraphics->explosionSize * 100 
			, weaponGraphics->explosionTime / 1000
			, ( weaponGraphics->explosionTime / 1000 ) / 2
			, ( weaponGraphics->explosionTime / 1000 ) * 2
			, weaponGraphics->explosionSize * 100 );

		if ( weaponGraphics->explosionDlightRadius >= 50 )
		{
			start = weaponGraphics->explosionDlightRadius * 100;
			end = weaponGraphics->explosionDlightRadius * 1000;

			// start, end, red, green, blue, opacity, mode, hint.
			trap_R_AddFogToScene( start, end, 0, 0, 0, expShell->lifeRate, 2, 2 );
		}

		// bias the time so all shader effects start correctly
		expShell->refEntity.shaderTime = expShell->startTime / 1000.0f;

		// set the Dlight RGB values
		expShell->lightColor[0] = weaponGraphics->explosionDlightColor[0];
		expShell->lightColor[1] = weaponGraphics->explosionDlightColor[1];
		expShell->lightColor[2] = weaponGraphics->explosionDlightColor[2];
		expShell->light = weaponGraphics->explosionDlightRadius * 100;
	}




	if ( weaponGraphics->shockwaveModel && weaponGraphics->shockwaveSkin ) {
		// allocate the entity as a ZEQ explosion
		expShock = CG_AllocLocalEntity();
		expShock->leFlags = 0;
		expShock->leType = LE_ZEQEXPLOSION;

		// set the type as model and link the model and skin
		expShock->refEntity.reType = RT_MODEL;
		expShock->refEntity.hModel = weaponGraphics->shockwaveModel;
		expShock->refEntity.customSkin = weaponGraphics->shockwaveSkin;
		
		// set axis with random rotate
		if ( !dir ) {
			AxisClear( expShock->refEntity.axis );
		} else {
			angle = rand() % 360;
			VectorCopy( dir, expShock->refEntity.axis[0] );
			RotateAroundDirection( expShock->refEntity.axis, angle );
		}

		// scale axes to explosion's full size
		expShock->refEntity.nonNormalizedAxes = qtrue;
		VectorNormalize(expShock->refEntity.axis[0]);
		VectorNormalize(expShock->refEntity.axis[1]);
		VectorNormalize(expShock->refEntity.axis[2]);
		VectorScale(expShock->refEntity.axis[0], 2 * weaponGraphics->explosionSize, expShock->refEntity.axis[0]);
		VectorScale(expShock->refEntity.axis[1], 2 * weaponGraphics->explosionSize, expShock->refEntity.axis[1]);
		VectorScale(expShock->refEntity.axis[2], 2 * weaponGraphics->explosionSize, expShock->refEntity.axis[2]);

		// set origin
		VectorCopy( origin, newOrigin );
		VectorCopy( newOrigin, expShock->refEntity.origin );
		VectorCopy( newOrigin, expShock->refEntity.oldorigin );

		// set the explosion's duration
		expShock->startTime = cg.time - offset - (weaponGraphics->explosionTime / 4);
		expShock->endTime = expShock->startTime + (weaponGraphics->explosionTime / 1.5);
		expShock->lifeRate = 1.0 / ( expShock->endTime - expShock->startTime );

/*
		// NOTE: camera shakes for shockwaves?
		// create a camera shake
		CG_AddEarthquake( origin, 
			weaponGraphics->explosionSize * 100 
			, weaponGraphics->explosionTime / 1000
			, ( weaponGraphics->explosionTime / 1000 ) / 2
			, ( weaponGraphics->explosionTime / 1000 ) * 2
			, weaponGraphics->explosionSize * 100 );
*/
		// bias the time so all shader effects start correctly
		expShock->refEntity.shaderTime = expShock->startTime / 1000.0f;

		// disable this Dlight
		expShock->light = 0;
	}

	if ( weaponGraphics->explosionParticleSystem[0] ) {
		vec3_t tempAxis[3];

		VectorCopy( dir, tempAxis[0] );
		MakeNormalVectors( tempAxis[0], tempAxis[1], tempAxis[2] );
		PSys_SpawnCachedSystem( weaponGraphics->explosionParticleSystem, origin, tempAxis, NULL, NULL, qfalse, qfalse );
	}

	if ( weaponGraphics->smokeParticleSystem[0] ) {
		vec3_t tempAxis[3];

		VectorCopy( dir, tempAxis[0] );
		MakeNormalVectors( tempAxis[0], tempAxis[1], tempAxis[2] );
		PSys_SpawnCachedSystem( weaponGraphics->smokeParticleSystem, origin, tempAxis, NULL, NULL, qfalse, qfalse );
	}
}

/*
===========================
CG_CreateStraightBeamFade
===========================
*/
void CG_CreateStraightBeamFade(vec3_t start, vec3_t end, cg_userWeapon_t *weaponGraphics) {
	localEntity_t	*beam;
	int				duration;

	if ( !weaponGraphics->missileTrailShader ) {
		return;
	}

	beam = CG_AllocLocalEntity();
	beam->leFlags = 0;
	beam->leType = LE_STRAIGHTBEAM_FADE;

	if ( weaponGraphics->missileTrailRadius ) {
		beam->radius = weaponGraphics->missileTrailRadius;
	} else {
		beam->radius = 10;
	}

	beam->refEntity.customShader = weaponGraphics->missileTrailShader;
	VectorCopy( start, beam->refEntity.origin );
	VectorCopy( end, beam->refEntity.oldorigin );

	duration = Distance( start, end );

	beam->startTime = cg.time;
	beam->endTime = cg.time + duration;
	beam->lifeRate = 1.0 / ( beam->endTime - beam->startTime );

	beam->refEntity.shaderTime = beam->startTime / 1000.0f;
}
