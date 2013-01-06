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
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

/*
======================
CG_DrawBoxFace

Draws a bounding box face
======================
*/
static void CG_DrawBoxFace( vec3_t a, vec3_t b, vec3_t c, vec3_t d )
{
  polyVert_t  verts[ 4 ];
  vec4_t      color = { 255.0f, 0.0f, 0.0f, 128.0f };
	qhandle_t bboxShader;

  VectorCopy( d, verts[ 0 ].xyz );
  verts[ 0 ].st[ 0 ] = 1;
  verts[ 0 ].st[ 1 ] = 1;
  Vector4Copy( color, verts[ 0 ].modulate );

  VectorCopy( c, verts[ 1 ].xyz );
  verts[ 1 ].st[ 0 ] = 1;
  verts[ 1 ].st[ 1 ] = 0;
  Vector4Copy( color, verts[ 1 ].modulate );

  VectorCopy( b, verts[ 2 ].xyz );
  verts[ 2 ].st[ 0 ] = 0;
  verts[ 2 ].st[ 1 ] = 0;
  Vector4Copy( color, verts[ 2 ].modulate );

  VectorCopy( a, verts[ 3 ].xyz );
  verts[ 3 ].st[ 0 ] = 0;
  verts[ 3 ].st[ 1 ] = 1;
  Vector4Copy( color, verts[ 3 ].modulate );

  bboxShader = trap_R_RegisterShader( "bbox_nocull" );

  trap_R_AddPolyToScene( bboxShader, 4, verts );
}

/*
======================
CG_DrawBoundingBox

Draws a bounding box
======================
*/
void CG_DrawBoundingBox( vec3_t origin, vec3_t mins, vec3_t maxs )
{
  vec3_t  ppp, mpp, mmp, pmp;
  vec3_t  mmm, pmm, ppm, mpm;

  ppp[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  ppp[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  ppp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  mpp[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mpp[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  mpp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  mmp[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mmp[ 1 ] = origin[ 1 ] + mins[ 1 ];
  mmp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  pmp[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  pmp[ 1 ] = origin[ 1 ] + mins[ 1 ];
  pmp[ 2 ] = origin[ 2 ] + maxs[ 2 ];

  ppm[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  ppm[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  ppm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  mpm[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mpm[ 1 ] = origin[ 1 ] + maxs[ 1 ];
  mpm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  mmm[ 0 ] = origin[ 0 ] + mins[ 0 ];
  mmm[ 1 ] = origin[ 1 ] + mins[ 1 ];
  mmm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  pmm[ 0 ] = origin[ 0 ] + maxs[ 0 ];
  pmm[ 1 ] = origin[ 1 ] + mins[ 1 ];
  pmm[ 2 ] = origin[ 2 ] + mins[ 2 ];

  //phew!

  CG_DrawBoxFace( ppp, mpp, mmp, pmp );
  CG_DrawBoxFace( ppp, pmp, pmm, ppm );
  CG_DrawBoxFace( mpp, ppp, ppm, mpm );
  CG_DrawBoxFace( mmp, mpp, mpm, mmm );
  CG_DrawBoxFace( pmp, mmp, mmm, pmm );
  CG_DrawBoxFace( mmm, mpm, ppm, pmm );
}


/*
======================
CG_GetTagPosition

Retrieves the position of a tag directly
======================
*/
void CG_GetTagPosition( refEntity_t *parent, char *tagName, vec3_t outpos) {
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag( &lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	VectorCopy( parent->origin, outpos );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( outpos, lerped.origin[i], parent->axis[i], outpos );
	}
}

/*
======================
CG_GetTagOrientation

Retrieves the orientation of a tag directly
======================
*/
void CG_GetTagOrientation( refEntity_t *parent, char *tagName, vec3_t dir) {
	orientation_t lerped;
	vec3_t	temp_axis[3];

	// lerp the tag
	trap_R_LerpTag( &lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, temp_axis );
	VectorCopy( temp_axis[0], dir );
}


/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		} else {
			trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}


	// constant light glow
	if(cent->currentState.constantLight)
	{
		int		cl;
		float		i, r, g, b;

		cl = cent->currentState.constantLight;
		r = (float) (cl & 0xFF) / 255.0;
		g = (float) ((cl >> 8) & 0xFF) / 255.0;
		b = (float) ((cl >> 16) & 0xFF) / 255.0;
		i = (float) ((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerpOrigin, i, r, g, b);
	}

}


/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	int num;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset (&ent, 0, sizeof(ent));

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	trap_R_AddRefEntityToScene (&ent);
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

/*
===============
JUHOX: CG_AddMissileLensFlare
===============
*/
static void CG_AddMissileLensFlare(centity_t* cent) {
	lensFlareEntity_t lfent;

	if (!cg_lensFlare.integer) return;

	memset(&lfent, 0, sizeof(lfent));
	lfent.lfeff = cgs.lensFlareEffectEnergyGlowDarkBackground; //cgs.lensFlareEffectSolarFlare;
	lfent.angle = -1;
	//VectorNegate(cent->currentState.pos.trDelta, lfent.dir);
	VectorCopy(cent->currentState.pos.trDelta, lfent.dir);
	VectorNormalize(lfent.dir);

	if (!lfent.lfeff) return;

	VectorCopy(cent->lerpOrigin, lfent.origin);

	CG_ComputeMaxVisAngle(&lfent);

	CG_AddLensFlare(&lfent, 1);
}

/*
===========================
CG_TrailFunc_StraightBeam
===========================
*/
void CG_TrailFunc_StraightBeam( centity_t *ent ) {
	entityState_t	*es;
	centity_t		*owner_ent;
	cg_userWeapon_t	*weaponGraphics;
	float			radius;
	orientation_t	orient;
	int			beamPowerLevelCurrent;
	int			beamPowerLevelTotal;
	float		radiusScale;

	// Initialize some things for quick reference
	es = &ent->currentState;
	owner_ent = &cg_entities[es->clientNum];
	weaponGraphics = CG_FindUserWeaponGraphics( es->clientNum, es->weapon );

	if (!weaponGraphics->missileTrailShader) {
		return;
	}

	// The beam's power level was stored in this field. We hijacked it on the
	// server to be able to transmit the beam's own power level.
	beamPowerLevelTotal = es->dashDir[0];
	beamPowerLevelCurrent = es->dashDir[1];

	if(beamPowerLevelCurrent >= (beamPowerLevelTotal * 2)){
		beamPowerLevelCurrent = beamPowerLevelTotal * 2;
	}

	// Obtain the scale the missile must have.
	if (weaponGraphics->missileTrailRadius) {
		radiusScale = (float)beamPowerLevelCurrent / (float)beamPowerLevelTotal;
		radius = weaponGraphics->missileTrailRadius * radiusScale;
	} else {
		radius = 10;
	}

	if (radiusScale > 1.0f){
		radiusScale = 1.0f;
	}else if(radiusScale < 0.0f){
		radiusScale = 0.0f;
	}

	if ( CG_GetTagOrientationFromPlayerEntity( &cg_entities[es->clientNum], weaponGraphics->chargeTag[0], &orient )) {
		CG_DrawLine (orient.origin, ent->lerpOrigin, radius, weaponGraphics->missileTrailShader, 1/*radiusScale*/);
	}
}

/*
========================
CG_TrailFunc_BendyBeam
========================
*/
void CG_TrailFunc_BendyBeam( centity_t *ent ) {
	entityState_t	*es;
	cg_userWeapon_t	*weaponGraphics;
	float			radius;
	//vec3_t			tangent;
	int			beamPowerLevelCurrent;
	int			beamPowerLevelTotal;
	float		radiusScale;

	// Set up shortcut references
	es = &ent->currentState;
	weaponGraphics = CG_FindUserWeaponGraphics( es->clientNum, es->weapon );
	
	// The beam's power level was stored in this field. We hijacked it on the
	// server to be able to transmit the beam's own power level.
	beamPowerLevelTotal = es->dashDir[0];
	beamPowerLevelCurrent = es->dashDir[1];

	if(beamPowerLevelCurrent >= (beamPowerLevelTotal * 2)){
		beamPowerLevelCurrent = beamPowerLevelTotal * 2;
	}

	// Obtain the scale the missile must have.
	if (weaponGraphics->missileTrailRadius) {
		radiusScale = (float)beamPowerLevelCurrent / (float)beamPowerLevelTotal;
		radius = weaponGraphics->missileTrailRadius * radiusScale;
	} else {
		radius = 10;
	}

	if (radiusScale > 1.0f){
		radiusScale = 1.0f;
	}else if(radiusScale < 0.0f){
		radiusScale = 0.0f;
	}

	if ( weaponGraphics->missileTrailShader ) {
		CG_BeamTableUpdate( ent, radius, weaponGraphics->missileTrailShader, weaponGraphics->chargeTag[0] );
	}
}



/*
================================
CG_TrailFunc_SpiralBeam_Helper
================================
*/
// Helper function to make CG_TrailFunc_SpiralBeam
// more readable.
void CG_TrailFunc_SpiralBeam_Helper ( entityState_t *es, centity_t *ent, int time, vec3_t origin) {
	cg_userWeapon_t	*weaponGraphics;
	vec3_t			tmpAxis[3];


	weaponGraphics = CG_FindUserWeaponGraphics( es->clientNum, es->weapon );

	BG_EvaluateTrajectory( es, &es->pos, time, origin );

	// convert direction of travel into axis
	if (VectorNormalize2( es->pos.trDelta, tmpAxis[0] ) == 0 ) {
		tmpAxis[0][2] = 1;
	}
	RotateAroundDirection( tmpAxis, time * 10 );
	
	// upscale the offset of the coil to the main beam
	if ( weaponGraphics->missileTrailSpiralOffset ) {
		VectorScale( tmpAxis[2], weaponGraphics->missileTrailSpiralOffset, tmpAxis[2] );
	}
	VectorAdd( origin, tmpAxis[2], origin );
}
	


/*
=========================
CG_TrailFunc_SpiralBeam
=========================
*/
void CG_TrailFunc_SpiralBeam( centity_t *ent ) {
	vec3_t			lastPos, lastPos2;
	int				step, t;
	int				startTime;
	entityState_t	*es;
	localEntity_t	*le;
	refEntity_t		*re;
	cg_userWeapon_t	*weaponGraphics;

	// Draw the central beam
	CG_TrailFunc_StraightBeam( ent );

	step = cg_tailDetail.value / 4;
	if(step < 1){step =1;}

	es = &ent->currentState;
	weaponGraphics = CG_FindUserWeaponGraphics( es->clientNum, es->weapon );

	if (!weaponGraphics->missileTrailSpiralShader) {
		return;
	}

	startTime = ent->trailTime;
	ent->trailTime = cg.time;
	t = startTime + step;
	
	// if object (e.g. grenade) is stationary, don't show tail
	if ( es->pos.trType == TR_STATIONARY ) {
		return;
	}

	CG_TrailFunc_SpiralBeam_Helper ( es, ent, startTime, lastPos);
	
	// Build segments
		
	for (; t <= ent->trailTime; t += step) {
		CG_TrailFunc_SpiralBeam_Helper ( es, ent, t, lastPos2);

		le = CG_AllocLocalEntity();
		re = &le->refEntity;
		le->leType = LE_FADE_ALPHA;
		le->startTime = t;
		le->endTime = t + 15000;
		le->lifeRate = 1.0 / (le->endTime - le->startTime);
	
		re->reType = RT_RAIL_CORE;
		re->customShader = weaponGraphics->missileTrailSpiralShader;
	 
		VectorCopy(lastPos, re->origin);
		VectorCopy(lastPos2, re->oldorigin);
 
		re->shaderRGBA[0] = 0xff;
	    re->shaderRGBA[1] = 0xff;
	    re->shaderRGBA[2] = 0xff;
	    re->shaderRGBA[3] = 0xff;

		le->color[0] = 1.00f;
		le->color[1] = 1.00f;
		le->color[2] = 1.00f;
		le->color[3] = 1.00f;

		AxisClear( re->axis );
		VectorCopy(lastPos2, lastPos);
	}

	CG_TrailFunc_SpiralBeam_Helper ( es, ent, ent->trailTime, lastPos2);

	le = CG_AllocLocalEntity();
	re = &le->refEntity;
	le->leType = LE_FADE_ALPHA;
	le->startTime = ent->trailTime;
	le->endTime = ent->trailTime + 15000;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
	
	re->reType = RT_RAIL_CORE;
	re->customShader = weaponGraphics->missileTrailSpiralShader;
	 
	VectorCopy(lastPos, re->origin);
	VectorCopy(lastPos2, re->oldorigin);
 
	re->shaderRGBA[0] = 0xff;
	re->shaderRGBA[1] = 0xff;
	re->shaderRGBA[2] = 0xff;
	re->shaderRGBA[3] = 0xff;

	le->color[0] = 1.00f;
	le->color[1] = 1.00f;
	le->color[2] = 1.00f;
	le->color[3] = 1.00f;

	AxisClear( re->axis );
}


/*
=======================
CG_TrailFunc_FadeTail
=======================
*/
void CG_TrailFunc_FadeTail( centity_t *cent ) {
	entityState_t	*es;
	cg_userWeapon_t	*weaponGraphics;
	vec3_t			delta;

	es = &cent->currentState;
	
	weaponGraphics = CG_FindUserWeaponGraphics( es->clientNum, es->weapon );
	
	if ( !weaponGraphics->missileTrailShader || !weaponGraphics->missileTrailRadius ) {
		return;
	}

	// If we didn't draw the tail last frame this is a new instantiation
	// of the entity and we will have to reset the tail positions.
	if ( cent->lastTrailTime < (cg.time - cg.frametime - 200) ) { // -200; give 0.2 sec leeway, just in case
		BG_EvaluateTrajectoryDelta( es, &es->pos, cg.time, delta );
		CG_ResetTrail( es->number, cent->lerpOrigin, VectorLength( delta ),
			weaponGraphics->missileTrailRadius, weaponGraphics->missileTrailShader, NULL );
	}
	
	CG_UpdateTrailHead( es->number, cent->lerpOrigin );

	cent->lastTrailTime = cg.time;
}


/*
===============
CG_Torch
===============
*/
static void CG_Torch( centity_t *cent ) {
	entityState_t		*s1;
	cg_userWeapon_t		*weaponGraphics;

	s1 = &cent->currentState;
	weaponGraphics = CG_FindUserWeaponGraphics(s1->clientNum, s1->weapon);
}


/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	cg_userWeapon_t		*weaponGraphics;
	float				missileScale;
	int					missileChargeLvl;
	playerState_t		*ps;
	int					missilePowerLevelCurrent;
	int					missilePowerLevelTotal;
	float				radiusScale;
	qboolean			missileIsStruggling;
	qboolean			splash;
	vec3_t				origin, lastPos;
	int					contents;
	int					lastContents;
	vec3_t				start, end;
	trace_t				trace;
	ps = &cg.predictedPlayerState;
	s1 = &cent->currentState;
	weaponGraphics = CG_FindUserWeaponGraphics(s1->clientNum, s1->weapon);
/*
	// Water bubble/splash setup

	BG_EvaluateTrajectory( s1, &s1->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	BG_EvaluateTrajectory( s1, &s1->pos, cent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 64;

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 64;

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	splash = qtrue;

	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		splash = qfalse;
	}

	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		splash = qfalse;
	}

	if ( trace.fraction == 1.0 ) {
		splash = qfalse;
	}

	cent->trailTime = cg.time;
*/
	// The missile's charge level was stored in this field. We hijacked it on the
	// server to be able to transmit the missile's own charge level.
	missileChargeLvl = s1->powerups;
	missilePowerLevelTotal = s1->dashDir[0];
	missilePowerLevelCurrent = s1->dashDir[1];
	missileIsStruggling = s1->dashDir[2];

	// Obtain the scale the missile must have.
	if (weaponGraphics->chargeGrowth) {
		// below the start, we use the lowest form
		if (weaponGraphics->chargeStartPct >= missileChargeLvl) {
			missileScale = weaponGraphics->chargeStartsize;
		} else {
			// above the end, we use the highest form
			if (weaponGraphics->chargeEndPct <= missileChargeLvl) {
				missileScale = weaponGraphics->chargeEndsize;
			} else {
				float	PctRange;
				float	PctVal;
				float	SizeRange;
				float	SizeVal;

				// inbetween, we work out the value
				PctRange = weaponGraphics->chargeEndPct - weaponGraphics->chargeStartPct;
				PctVal = missileChargeLvl - weaponGraphics->chargeStartPct;

				SizeRange = weaponGraphics->chargeEndsize - weaponGraphics->chargeStartsize;
				SizeVal = (PctVal / PctRange) * SizeRange;
				missileScale = SizeVal + weaponGraphics->chargeStartsize;
			}
		}
	} else {
		missileScale = 1;
	}
	missileScale = missileScale * weaponGraphics->missileSize;

	if(missilePowerLevelCurrent >= (missilePowerLevelTotal * 2)){
		missilePowerLevelCurrent = missilePowerLevelTotal * 2;
	}

	radiusScale = (float)missilePowerLevelCurrent / (float)missilePowerLevelTotal;
	missileScale = missileScale * radiusScale;

	if (radiusScale > 1.0f){
		radiusScale = 1.0f;
	}else if(radiusScale < 0.0f){
		radiusScale = 0.0f;
	}

	// calculate the axis
	 VectorCopy( s1->angles, cent->lerpAngles);
	
	// If it's a guided missile, and belongs to this client, return its position to cg.
	if((cent->currentState.clientNum == cg.clientNum) && (cent->currentState.eFlags & EF_GUIDED)){
		VectorCopy(cent->lerpOrigin, cg.guide_target);
		cg.guide_view = qtrue;
	}
/*
	if (splash) {
		CG_WaterSplash(trace.endpos, missileScale);
	}
*/
	// add trails
	if ( weaponGraphics->missileTrailShader && weaponGraphics->missileTrailRadius ) {
		if ( cent->currentState.eType == ET_MISSILE ) {
			CG_TrailFunc_FadeTail( cent );
			if ( contents & lastContents & CONTENTS_WATER ) {
				CG_BubbleTrail( lastPos, origin, 64 );
			}
		} else if ( cent->currentState.eType == ET_BEAMHEAD ) {
			if ( cent->currentState.eFlags & EF_GUIDED  &&
						!weaponGraphics->missileTrailSpiralShader &&
				        !weaponGraphics->missileTrailSpiralRadius &&
						!weaponGraphics->missileTrailSpiralOffset ) {
				CG_TrailFunc_BendyBeam( cent );
			} else if ( weaponGraphics->missileTrailSpiralShader &&
						weaponGraphics->missileTrailSpiralRadius &&
						weaponGraphics->missileTrailSpiralOffset ) {
				CG_TrailFunc_SpiralBeam( cent );
			} else {
				CG_TrailFunc_StraightBeam( cent );
			}
		}
	}

	// add dynamic light
	if ( weaponGraphics->missileDlightRadius ) {
		trap_R_AddLightToScene(cent->lerpOrigin,
			100 * weaponGraphics->missileDlightRadius, 
			weaponGraphics->missileDlightColor[0],
			weaponGraphics->missileDlightColor[1],
			weaponGraphics->missileDlightColor[2] );
	}

	// add missile sound
	if ( weaponGraphics->missileSound ) {
		vec3_t	velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState, &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weaponGraphics->missileSound );
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	// JUHOX: draw BeamHead missile lens flare effects
	CG_AddMissileLensFlare(cent);

	if ( ! (weaponGraphics->missileModel && weaponGraphics->missileSkin) ) {
		ent.reType = RT_SPRITE;
		ent.radius = 4 * missileScale;
		ent.rotation = 0;
		ent.customShader = weaponGraphics->missileShader;
		//ent.shaderRGBA[0] = radiusScale;
		//ent.shaderRGBA[1] = radiusScale;
		//ent.shaderRGBA[2] = radiusScale;
		//ent.shaderRGBA[3] = radiusScale;

		trap_R_AddRefEntityToScene( &ent );
		CG_PlayerSplash(cent, 500 / (missileScale / 4));
	} else {
		ent.reType = RT_MODEL;
		if ( (weaponGraphics->missileStruggleModel && weaponGraphics->missileStruggleSkin) && missileIsStruggling/*(s1->dashDir[2] != 0.0f)*/) {
			ent.hModel = weaponGraphics->missileStruggleModel;
			ent.customSkin = weaponGraphics->missileStruggleSkin;
		} else {
			ent.hModel = weaponGraphics->missileModel;
			ent.customSkin = weaponGraphics->missileSkin;
		}

		ent.renderfx = RF_NOSHADOW;
		//ent.shaderRGBA[0] = radiusScale;
		//ent.shaderRGBA[1] = radiusScale;
		//ent.shaderRGBA[2] = radiusScale;
		//ent.shaderRGBA[3] = radiusScale;

		// We want something simple for simple roll-spinning missiles,
		// but will use quaternions to get a correct yaw rotation for
		// disk attacks and the like.
		if ( !weaponGraphics->missileSpin[0] && !weaponGraphics->missileSpin[1] ) {
					
			// convert direction of travel into axis
			if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
				ent.axis[0][2] = 1;
			}
			// spin as it moves
			if ( s1->pos.trType != TR_STATIONARY ) {
				RotateAroundDirection( ent.axis, weaponGraphics->missileSpin[2] * cg.time / 4.0f );
			} else {
				RotateAroundDirection( ent.axis, weaponGraphics->missileSpin[2] * s1->time );
			}
		} else {
			vec3_t tempAngles;
			vec4_t quatOrient;
			vec4_t quatRot;
			vec4_t quatResult;
			vec3_t spinRotate;

			// convert direction of travel into axis
			if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
				ent.axis[0][0] = 1;
			}

			if ( s1->pos.trType != TR_STATIONARY ) {
				VectorSet( spinRotate, cg.time / 4.0f, cg.time / 4.0f, cg.time / 4.0f );
			} else {
				VectorSet( spinRotate, s1->time, s1->time, s1->time );
			}
			VectorPieceWiseMultiply( spinRotate, weaponGraphics->missileSpin, spinRotate );

			vectoangles( ent.axis[0], tempAngles );
			AnglesToQuat( tempAngles, quatOrient );
			AnglesToQuat( spinRotate, quatRot );
			QuatMul( quatOrient, quatRot, quatResult );
			QuatToAxis( quatResult, ent.axis );
		}

		ent.nonNormalizedAxes = qtrue;
		VectorScale(ent.axis[0], missileScale, ent.axis[0]);
		VectorScale(ent.axis[1], missileScale, ent.axis[1]);
		VectorScale(ent.axis[2], missileScale, ent.axis[2]);

		if(cg_drawBBox.value){
			vec3_t	mins,maxs;
			trap_R_ModelBounds( ent.hModel, mins, maxs, ent.frame );
			VectorScale(mins, missileScale, mins);
			VectorScale(maxs, missileScale, maxs);
			CG_DrawBoundingBox( ent.origin, mins, maxs );
		}

		trap_R_AddRefEntityToScene( &ent );
		CG_PlayerSplash(cent, 500 / missileScale);
	}

	// Check if the missile is in a struggle, and if so, do effects for it.
	if( missileIsStruggling ){
		CG_AddEarthquake(cent->lerpOrigin, 10000, 1, 0, 1, 50);
	}

	// Check if we should activate a missile specific particle system
	//if ( cent->lastPVSTime < ( cg.time - cg.frametime - 100) ) {
	if ( !CG_FrameHist_WasInPVS( s1->number )) {
		if ( weaponGraphics->missileParticleSystem[0] ) {
			vec3_t tempAxis[3];

			AnglesToAxis( cent->lerpAngles, tempAxis );
			PSys_SpawnCachedSystem( weaponGraphics->missileParticleSystem, cent->lerpOrigin, tempAxis, cent, NULL, qfalse, qfalse );
		}
	}
}

/*
===============
CG_Mover
===============
*/
/*static*/ void CG_Mover( centity_t *cent ) {	// JUHOX: also called from cg_draw.c for lens flare editor
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene(&ent);
	}

}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out ) {
	centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	//VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );

	// FIXME: origin change when on a rotating object
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		CG_Error( "CG_InterpolateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState, &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState, &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );
}


/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {

	// if this player does not want to see extrapolated players
	if ( !cg_smoothClients.integer ) {
		// make sure the clients use TR_INTERPOLATE
		if ( cent->currentState.number < MAX_CLIENTS ) {
			cent->currentState.pos.trType = TR_INTERPOLATE;
			cent->nextState.pos.trType = TR_INTERPOLATE;
		}
	}

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
											cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState, &cent->currentState.apos, cg.time, cent->lerpAngles );

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum, 
		cg.snap->serverTime, cg.time, cent->lerpOrigin );
	}
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase( centity_t *cent ){}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_EXPLOSION:
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	case ET_INVISIBLE:
		break;
	case ET_PUSH_TRIGGER:
		break;
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_BEAMHEAD:
		CG_Missile( cent );
		break;
	}

	//cent->lastPVSTime = cg.time;
	CG_FrameHist_SetPVS( cent->currentState.number );
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because 
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		CG_AddCEntity( cent );
	}
}

