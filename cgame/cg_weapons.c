// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

/*
=============
CG_DrawLine
=============
*/
void CG_DrawLine (vec3_t start, vec3_t end, float width, qhandle_t shader, float RGBModulate) {
	vec3_t line, offset, viewLine;
	polyVert_t verts[4];
	float len;
	int i, j;
	
	VectorSubtract (end, start, line);
	VectorSubtract (start, cg.refdef.vieworg, viewLine);
	CrossProduct (viewLine, line, offset);
	len = VectorNormalize (offset);
	
	if (!len) {
		return;
	}
	
	VectorMA (end, -width, offset, verts[0].xyz);
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	VectorMA (end, width, offset, verts[1].xyz);
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	VectorMA (start, width, offset, verts[2].xyz);
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	VectorMA (start, -width, offset, verts[3].xyz);
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			verts[i].modulate[j] = 255 * RGBModulate;
		}
	}

	trap_R_AddPolyToScene( shader, 4, verts);
}

void CG_DrawLineRGBA (vec3_t start, vec3_t end, float width, qhandle_t shader, vec4_t RGBA) {
	vec3_t line, offset, viewLine;
	polyVert_t verts[4];
	float len;
	int i, j;
	
	VectorSubtract (end, start, line);
	VectorSubtract (start, cg.refdef.vieworg, viewLine);
	CrossProduct (viewLine, line, offset);
	len = VectorNormalize (offset);
	
	if (!len) {
		return;
	}
	
	VectorMA (end, -width, offset, verts[0].xyz);
	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	VectorMA (end, width, offset, verts[1].xyz);
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	VectorMA (start, width, offset, verts[2].xyz);
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	VectorMA (start, -width, offset, verts[3].xyz);
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
	
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			verts[i].modulate[j] = RGBA[j];
		}
	}

	trap_R_AddPolyToScene( shader, 4, verts);
}


/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	float			waterScale = 1.0f;
	vec3_t			v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	int				i;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	for ( i = 0; i < 2; i++ ) {
		float	waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if ( i == 0 ) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis( cent->lerpAngles, v );

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
		VectorCopy( re->origin, le->pos.trBase );
		if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale( xvelocity, waterScale, le->pos.trDelta );

		AxisCopy( axisDefault, re->axis );
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_BRASS;
		le->leMarkType = LEMT_NONE;
	}
}


#ifdef MISSIONPACK
/*
==========================
CG_NailgunEjectBrass
==========================
*/
static void CG_NailgunEjectBrass( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			v[3];
	vec3_t			offset;
	vec3_t			xoffset;
	vec3_t			up;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, origin );

	VectorSet( up, 0, 0, 64 );

	smoke = CG_SmokePuff( origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader );
	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}
#endif


/*
==========================
CG_RailTrail
==========================
*/
void CG_RailTrail (clientInfo_t *ci, vec3_t start, vec3_t end) {
	vec3_t axis[36], move, move2, next_move, vec, temp;
	float  len;
	int    i, j, skip;
 
	localEntity_t *le;
	refEntity_t   *re;
 
#define RADIUS   4
#define ROTATION 1
#define SPACING  5
 
	start[2] -= 4;
	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	PerpendicularVector(temp, vec);
	for (i = 0 ; i < 36; i++) {
		RotatePointAroundVector(axis[i], vec, temp, i * 10);//banshee 2.4 was 10
	}
 
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA[0] = ci->color1[0] * 255;
    re->shaderRGBA[1] = ci->color1[1] * 255;
    re->shaderRGBA[2] = ci->color1[2] * 255;
    re->shaderRGBA[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );
 
	VectorMA(move, 20, vec, move);
	VectorCopy(move, next_move);
	VectorScale (vec, SPACING, vec);

	if (cg_oldRail.integer != 0) {
		// nudge down a bit so it isn't exactly in center
		re->origin[2] -= 8;
		re->oldorigin[2] -= 8;
		return;
	}
	skip = -1;
 
	j = 18;
    for (i = 0; i < len; i += SPACING) {
		if (i != skip) {
			skip = i + SPACING;
			le = CG_AllocLocalEntity();
            re = &le->refEntity;
            le->leFlags = LEF_PUFF_DONT_SCALE;
			le->leType = LE_MOVE_SCALE_FADE;
            le->startTime = cg.time;
            le->endTime = cg.time + (i>>1) + 600;
            le->lifeRate = 1.0 / (le->endTime - le->startTime);

            re->shaderTime = cg.time / 1000.0f;
            re->reType = RT_SPRITE;
            re->radius = 1.1f;
			re->customShader = cgs.media.railRingsShader;

            re->shaderRGBA[0] = ci->color2[0] * 255;
            re->shaderRGBA[1] = ci->color2[1] * 255;
            re->shaderRGBA[2] = ci->color2[2] * 255;
            re->shaderRGBA[3] = 255;

            le->color[0] = ci->color2[0] * 0.75;
            le->color[1] = ci->color2[1] * 0.75;
            le->color[2] = ci->color2[2] * 0.75;
            le->color[3] = 1.0f;

            le->pos.trType = TR_LINEAR;
            le->pos.trTime = cg.time;

			VectorCopy( move, move2);
            VectorMA(move2, RADIUS , axis[j], move2);
            VectorCopy(move2, le->pos.trBase);

            le->pos.trDelta[0] = axis[j][0]*6;
            le->pos.trDelta[1] = axis[j][1]*6;
            le->pos.trDelta[2] = axis[j][2]*6;
		}

        VectorAdd (move, vec, move);

        j = j + ROTATION < 36 ? j + ROTATION : (j + ROTATION) % 36;
	}
}

/*
==========================
CG_RocketTrail
==========================
*/
static void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( es, &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( es, &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( es, &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.smokePuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}

#ifdef MISSIONPACK
/*
==========================
CG_NailTrail
==========================
*/
static void CG_NailTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( es, &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( es, &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( es, &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.nailPuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}
#endif

/*
==========================
CG_PlasmaTrail
==========================
*/
static void CG_PlasmaTrail( centity_t *cent, const weaponInfo_t *wi ) {
	localEntity_t	*le;
	refEntity_t		*re;
	entityState_t	*es;
	vec3_t			velocity, xvelocity, origin;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	int				t, startTime, step;

	float	waterScale = 1.0f;

	if ( cg_noProjectileTrail.integer || cg_oldPlasma.integer ) {
		return;
	}

	step = 50;

	es = &cent->currentState;
	startTime = cent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( es, &es->pos, cg.time, origin );

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 60 - 120 * crandom();
	velocity[1] = 40 - 80 * crandom();
	velocity[2] = 100 - 200 * crandom();

	le->leType = LE_MOVE_SCALE_FADE;
	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_NONE;
	le->leMarkType = LEMT_NONE;

	le->startTime = cg.time;
	le->endTime = le->startTime + 600;

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 2;
	offset[1] = 2;
	offset[2] = 2;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];

	VectorAdd( origin, xoffset, re->origin );
	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
    re->shaderTime = cg.time / 1000.0f;
    re->reType = RT_SPRITE;
    re->radius = 0.25f;
	re->customShader = cgs.media.railRingsShader;
	le->bounceFactor = 0.3f;

    re->shaderRGBA[0] = wi->flashDlightColor[0] * 63;
    re->shaderRGBA[1] = wi->flashDlightColor[1] * 63;
    re->shaderRGBA[2] = wi->flashDlightColor[2] * 63;
    re->shaderRGBA[3] = 63;

    le->color[0] = wi->flashDlightColor[0] * 0.2;
    le->color[1] = wi->flashDlightColor[1] * 0.2;
    le->color[2] = wi->flashDlightColor[2] * 0.2;
    le->color[3] = 0.25f;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 1;
	le->angles.trDelta[1] = 0.5;
	le->angles.trDelta[2] = 0;

}
/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi ) {
	vec3_t	origin;
	entityState_t	*es;
	vec3_t			forward, up;
	refEntity_t		beam;

	es = &ent->currentState;

	BG_EvaluateTrajectory( es, &es->pos, cg.time, origin );
	ent->trailTime = cg.time;

	memset( &beam, 0, sizeof( beam ) );
	//FIXME adjust for muzzle position
	VectorCopy ( cg_entities[ ent->currentState.otherEntityNum ].lerpOrigin, beam.origin );
	beam.origin[2] += 26;
	AngleVectors( cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, forward, NULL, up );
	VectorMA( beam.origin, -6, up, beam.origin );
	VectorCopy( origin, beam.oldorigin );

	if (Distance( beam.origin, beam.oldorigin ) < 64 )
		return; // Don't draw if close

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;

	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene( &beam );
}

/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_RocketTrail( ent, wi );
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {

	return;
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( &itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel( item->world_model[0] );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame ) {
/*
	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
*/
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
	return;
}
/*
===============
JUHOX: CG_Draw3DLine
===============
*/
void CG_Draw3DLine(const vec3_t start, const vec3_t end, qhandle_t shader) {
	refEntity_t line;

	//if (DistanceSquared(start, end) < 10*10) return;	
	memset(&line, 0, sizeof(line));
	line.reType = RT_LIGHTNING;
	line.customShader = shader;
	VectorCopy(start, line.origin);
	VectorCopy(end, line.oldorigin);
	trap_R_AddRefEntityToScene(&line);
}

/*
===============
CG_SpawnRailTrail

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
===============
*/
static void CG_SpawnRailTrail( centity_t *cent, vec3_t origin ) {
	return;

	/*
	clientInfo_t	*ci;

	if ( cent->currentState.weapon != WP_RAILGUN ) {
		return;
	}
	if ( !cent->pe.railgunFlash ) {
		return;
	}
	cent->pe.railgunFlash = qtrue;
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	CG_RailTrail( ci, origin, cent->pe.railgunImpact );
	*/
}


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);
#ifdef MISSIONPACK
		if ( cent->currentState.weapon == WP_CHAINGUN && !cent->pe.barrelSpinning ) {
			trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, trap_S_RegisterSound( "sound/weapons/vulcan/wvulwind.ogg", qfalse ) );
		}
#endif
	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {}


/*
=============
CG_AddPlayerWeaponCharge

Used for both the primary and the alternate fire of weapons.
Used case depends on input.
=============
*/
static void CG_AddPlayerWeaponCharge( refEntity_t *parent, cg_userWeapon_t *weaponGraphics,
									  refEntity_t *charge, float chargeLvl ) {
	float				chargeScale;
	float				chargeDlightScale;
	
	// Obtain the scale the charge must have.
	if (weaponGraphics->chargeGrowth) {
		// above the end, we use the highest form
		if (weaponGraphics->chargeEndPct <= chargeLvl) {
			chargeScale = weaponGraphics->chargeEndsize;
			chargeDlightScale = weaponGraphics->chargeDlightEndRadius;
		} else {
			// inbetween start and end, we work out the value
			float	PctRange;
			float	PctVal;
			float	SizeRange;
			float	SizeVal;
					
			PctRange = weaponGraphics->chargeEndPct - weaponGraphics->chargeStartPct;
			PctVal = chargeLvl - weaponGraphics->chargeStartPct;

			SizeRange = weaponGraphics->chargeEndsize - weaponGraphics->chargeStartsize;
			SizeVal = (PctVal / PctRange) * SizeRange;
			chargeScale = SizeVal + weaponGraphics->chargeStartsize;

			SizeRange = weaponGraphics->chargeDlightEndRadius - weaponGraphics->chargeDlightStartRadius;
			SizeVal = (PctVal / PctRange) * SizeRange;
			chargeDlightScale = SizeVal + weaponGraphics->chargeDlightStartRadius;
		}
	} else {
		chargeScale = weaponGraphics->chargeStartsize;
		chargeDlightScale = weaponGraphics->chargeDlightStartRadius;
	}

	// Add the charge model or sprite

	VectorCopy( parent->lightingOrigin, charge->lightingOrigin );
	charge->shadowPlane = parent->shadowPlane;
	charge->renderfx = parent->renderfx;
	if ( ! (weaponGraphics->chargeModel && weaponGraphics->chargeSkin) ) {
		charge->reType = RT_SPRITE;
		charge->radius = 4 * chargeScale;
		charge->rotation = 0;
		charge->customShader = weaponGraphics->chargeShader;
	} else {
		charge->reType = RT_MODEL;
		charge->hModel = weaponGraphics->chargeModel;
		charge->customSkin = weaponGraphics->chargeSkin;
	
		charge->nonNormalizedAxes = qtrue;
		VectorScale(charge->axis[0], chargeScale, charge->axis[0]);
		VectorScale(charge->axis[1], chargeScale, charge->axis[1]);
		VectorScale(charge->axis[2], chargeScale, charge->axis[2]);
	}

	trap_R_AddRefEntityToScene( charge );


	// add dynamic light
	if ( chargeDlightScale ) {
		trap_R_AddLightToScene( charge->origin,
								100 * chargeDlightScale,
								weaponGraphics->chargeDlightColor[0],
								weaponGraphics->chargeDlightColor[1],
								weaponGraphics->chargeDlightColor[2] );
	}
}

/*
=============
CG_AddPlayerWeaponChargeVoices

Used for both the primary and the alternate fire of weapons.
Used case depends on input.
=============
*/
static void CG_AddPlayerWeaponChargeVoices( centity_t *player, cg_userWeapon_t *weaponGraphics, float curChargeLvl, float prevChargeLvl ) {
	int i;
	chargeVoice_t *voice;

	for ( i = MAX_CHARGE_VOICES - 1; i >= 0; i-- ) {
		voice = &weaponGraphics->chargeVoice[i];
		if ( voice->voice ) { // no sound; no complex boolean eval
			if ( (voice->startPct > prevChargeLvl) && (voice->startPct < curChargeLvl) && ( prevChargeLvl < curChargeLvl ) ) {
				trap_S_StartSound( NULL , player->currentState.number, CHAN_VOICE, voice->voice );
				break;
			}
		}
	}
}

/*
=============
CG_AddPlayerWeaponFlash

Used for both the primary and the alternate fire of weapons.
Used case depends on input.
=============
*/
static void CG_AddPlayerWeaponFlash( refEntity_t *parent, cg_userWeapon_t *weaponGraphics,
									  refEntity_t *flash ) {
	
	// Add the charge model or sprite

	VectorCopy( parent->lightingOrigin, flash->lightingOrigin );
	flash->shadowPlane = parent->shadowPlane;
	flash->renderfx = parent->renderfx;
	if ( ! (weaponGraphics->flashModel && weaponGraphics->flashSkin) ) {
		flash->reType = RT_SPRITE;
		flash->radius = 4 * weaponGraphics->flashSize;
		flash->rotation = 0;
		flash->customShader = weaponGraphics->flashShader;
	} else {
		flash->reType = RT_MODEL;
		flash->hModel = weaponGraphics->flashModel;
		flash->customSkin = weaponGraphics->flashSkin;
	
		flash->nonNormalizedAxes = qtrue;
		VectorScale(flash->axis[0], weaponGraphics->flashSize, flash->axis[0]);
		VectorScale(flash->axis[1], weaponGraphics->flashSize, flash->axis[1]);
		VectorScale(flash->axis[2], weaponGraphics->flashSize, flash->axis[2]);
	}

	trap_R_AddRefEntityToScene( flash );


	// add dynamic light
	if ( weaponGraphics->flashDlightRadius ) {
		trap_R_AddLightToScene( flash->origin,
								100 * weaponGraphics->flashDlightRadius,
								weaponGraphics->flashDlightColor[0],
								weaponGraphics->flashDlightColor[1],
								weaponGraphics->flashDlightColor[2] );
	}
}


/*
====================
CG_AddPlayerWeapon
====================
*/
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team ) {
	cg_userWeapon_t		*weaponGraphics;
	orientation_t		orient;
	refEntity_t			refEnt;
	entityState_t		*ent;
	float				lerp;
	float				backLerp;
	int					newLerp;
	weaponstate_t		weaponState;

	// Set some shorthands
	ent = &cent->currentState;
	weaponState = ent->weaponstate;
	
	// Any of the charging states
	if ( weaponState == WEAPON_CHARGING || weaponState == WEAPON_ALTCHARGING ) {

		// Set properties depending on primary or alternate fire
		if ( weaponState == WEAPON_CHARGING ) {
			weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon);

			newLerp = ent->charge1.chBase;
			backLerp = cent->lerpPrim;
			if ( newLerp < backLerp ) {
				cent->lerpPrim = newLerp;
			} else {
				cent->lerpPrim = (cent->lerpPrim + newLerp ) / 2.0f;
			}

			lerp = cent->lerpPrim;
			cent->lerpSec = 0;
		} else {
			weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon + ALTWEAPON_OFFSET );

			newLerp = ent->charge2.chBase;
			backLerp = cent->lerpSec;
			if ( newLerp < backLerp ) {
				cent->lerpSec = newLerp;
			} else {
				cent->lerpSec = (cent->lerpSec + newLerp ) / 2.0f;
			}

			lerp = cent->lerpSec;
			cent->lerpPrim = 0;
		}

		// Only bother with anything else if the charge in question is larger than the minimum used for display
		if ( lerp > weaponGraphics->chargeStartPct ) {			

			// Locate a tag wherever on the model's parts. Don't process further if the tag is not found
			if (CG_GetTagOrientationFromPlayerEntity( cent, weaponGraphics->chargeTag[0], &orient )) {
				memset( &refEnt, 0, sizeof(refEnt));

				if ( VectorLength(weaponGraphics->chargeSpin) != 0.0f ) {
					vec3_t	tempAngles;
					vec3_t	lerpAxis[3], tempAxis[3];
					
					VectorCopy( orient.origin, refEnt.origin );
					VectorSet( tempAngles, cg.time / 4.0f, cg.time / 4.0f, cg.time / 4.0f );
					VectorPieceWiseMultiply( tempAngles, weaponGraphics->chargeSpin, tempAngles );
					AnglesToAxis( tempAngles, lerpAxis );

					AxisClear( refEnt.axis );
					MatrixMultiply( refEnt.axis, lerpAxis, tempAxis );
					MatrixMultiply( tempAxis, orient.axis, refEnt.axis );					

				} else {

					VectorCopy( orient.origin, refEnt.origin );
					AxisCopy( orient.axis, refEnt.axis );
				}

				CG_AddPlayerWeaponCharge( parent, weaponGraphics, &refEnt, lerp );
			}
		}
		
		if ( weaponGraphics->chargeLoopSound ) {
			trap_S_AddLoopingSound( ent->number, cent->lerpOrigin, vec3_origin, weaponGraphics->chargeLoopSound );
		}

		CG_AddPlayerWeaponChargeVoices( cent, weaponGraphics, lerp, backLerp );

		// Set up any charging particle systems
		if ( weaponGraphics->chargeParticleSystem[0] ) {
			// If the entity wasn't previously in the PVS, if the weapon nr switched, or if the weaponstate switched
			// we need to start a new system
			if ( !CG_FrameHist_WasInPVS(ent->number) ||
				 CG_FrameHist_IsWeaponNr(ent->number) != CG_FrameHist_WasWeaponNr(ent->number) ||
				 CG_FrameHist_IsWeaponState(ent->number) != CG_FrameHist_WasWeaponState(ent->number) ) {
				PSys_SpawnCachedSystem( weaponGraphics->chargeParticleSystem, cent->lerpOrigin, NULL, cent, weaponGraphics->chargeTag[0], qfalse, qtrue );
			}
		}		

	// Any of the firing or guiding states
	} else if ( weaponState == WEAPON_GUIDING || weaponState == WEAPON_ALTGUIDING || weaponState == WEAPON_FIRING || weaponState == WEAPON_ALTFIRING ) { 

		// Set properties depending on primary or alternate fire
		if ( weaponState == WEAPON_GUIDING || weaponState == WEAPON_FIRING ) {
			weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon );
		} else {
			weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, ent->weapon + ALTWEAPON_OFFSET );
		}

		// Locate a tag wherever on the model's parts. Don't process further if the tag is not found
		if (CG_GetTagOrientationFromPlayerEntity( cent, weaponGraphics->chargeTag[0], &orient )) {
			memset( &refEnt, 0, sizeof(refEnt));

			VectorCopy( orient.origin, refEnt.origin );
			AxisCopy( orient.axis, refEnt.axis );
			
			CG_AddPlayerWeaponFlash( parent, weaponGraphics, &refEnt );
		}

		if ( weaponGraphics->firingSound ) {
			trap_S_AddLoopingSound( ent->number, cent->lerpOrigin, vec3_origin, weaponGraphics->firingSound );
		}

		// Set up any firing particle systems
		if ( weaponGraphics->firingParticleSystem[0] ) {
			// If the entity wasn't previously in the PVS, if the weapon nr switched, or if the weaponstate switched
			// we need to start a new system
			if ( !CG_FrameHist_WasInPVS(ent->number) ||
				 CG_FrameHist_IsWeaponNr(ent->number) != CG_FrameHist_WasWeaponNr(ent->number) ||
				 CG_FrameHist_IsWeaponState(ent->number) != CG_FrameHist_WasWeaponState(ent->number) ) {
				PSys_SpawnCachedSystem( weaponGraphics->firingParticleSystem, cent->lerpOrigin, NULL, cent, weaponGraphics->chargeTag[0], qfalse, qtrue );
			}
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {

	return;
	
/*
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;



	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}


	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fov.integer > 90 ) {
		fovOffset = -0.2 * ( cg_fov.integer - 90 );
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->weapon );
	weapon = &cg_weapons[ ps->weapon ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// map torso animations to weapon animations
	if ( cg_gun_frame.integer ) {
		// development tool
		hand.frame = hand.oldframe = cg_gun_frame.integer;
		hand.backlerp = 0;
	} else {
		// get clientinfo for animation map
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
		hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
		hand.backlerp = cent->pe.torso.backlerp;
	}

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM] );
*/
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

static void CG_DrawWeaponSelectQuarterFan( void ) {
	int				i;
	int				bits;
	int				count;
	int				x, y;
	int				textX, textY;
	int				iconsPerLine, iconW, iconH;
	int				rootX, rootY;
	int				startOffset, lineOffset;
	float			startAngle, endAngle;
	float			angleUnit;
	char			name[MAX_WEAPONNAME * 2 + 4]; // Need a little more leeway
	float			*color;
	cg_userWeapon_t	*weaponInfo;
	cg_userWeapon_t	*alt_weaponInfo;

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	textX = 110; textY = 380;

	rootX = 64;	rootY = 424;
	iconsPerLine = 3; iconW = 32; iconH = 32;
	startAngle = DEG2RAD(-20); endAngle = DEG2RAD(40);
	startOffset = 80; lineOffset = 40;

	if (iconsPerLine != 1) {
		angleUnit = (endAngle - startAngle) / (iconsPerLine - 1);
	} else {
		angleUnit = 0;
	}
	
	bits = cg.snap->ps.stats[ skills ];
	count = 0;
	for ( i = 1 ; i < 16 ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		weaponInfo = CG_FindUserWeaponGraphics( cg.snap->ps.clientNum, i );

		// set position
		x = rootX - (iconW / 2) +
			sin(startAngle + angleUnit * (count % 3)) * (startOffset + lineOffset * (count / 3));
		y = rootY - (iconH / 2) +
			-1 * cos(startAngle + angleUnit * (count % 3)) * (startOffset + lineOffset * (count / 3));
		
		// draw weapon icon
		if ( weaponInfo->weaponIcon ) {
			CG_DrawPic( x, y, 32, 32, weaponInfo->weaponIcon );
		}

		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}

		count++;
	}

	weaponInfo = CG_FindUserWeaponGraphics( cg.snap->ps.clientNum, cg.weaponSelect );
	alt_weaponInfo = CG_FindUserWeaponGraphics ( cg.snap->ps.clientNum, ALTWEAPON_OFFSET + cg.weaponSelect);

	// draw the selected name
	if ( weaponInfo->weaponName[0] != 0 ) {

		if ( alt_weaponInfo->weaponName[0] != 0 ) {
			Com_sprintf( name, sizeof(name), "%s / %s", weaponInfo->weaponName, alt_weaponInfo->weaponName );
		} else {
			Com_sprintf( name, sizeof(name), "%s", weaponInfo->weaponName );
		}

		CG_DrawMediumStringColor(textX, textY, name, color);
	}

	trap_R_SetColor( NULL );
	
}

/*
===============================
CG_DrawWeaponSelectHorCenterBar
===============================
*/
static void CG_DrawWeaponSelectHorCenterBar( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y, w;
	char	name[MAX_WEAPONNAME * 2 + 4]; // Need a little more leeway
	float	*color;
	cg_userWeapon_t	*weaponInfo;
	cg_userWeapon_t	*alt_weaponInfo;

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ skills ];
	count = 0;
	for ( i = 1 ; i < 16 ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	x = 164 - count * 15;
	y = 418;

	for ( i = 1 ; i < 16 ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		//CG_RegisterWeapon( i );

		weaponInfo = CG_FindUserWeaponGraphics( cg.snap->ps.clientNum, i );

		// draw weapon icon
		//CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );
		CG_DrawPic( x, y, 24, 24, weaponInfo->weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 32, 32, cgs.media.selectShader );
		}

		// ADDING FOR ZEQ2

		// We never draw the ammo cross on top!
		
		/*
		// no ammo cross on top
		if ( !cg.snap->ps.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		}
		*/
		// END ADDING

		x += 30;
	}

	weaponInfo = CG_FindUserWeaponGraphics( cg.snap->ps.clientNum, cg.weaponSelect );
	alt_weaponInfo = CG_FindUserWeaponGraphics ( cg.snap->ps.clientNum, ALTWEAPON_OFFSET + cg.weaponSelect);

	// draw the selected name
	if ( weaponInfo->weaponName ) {

		if ( alt_weaponInfo->weaponName ) {
			Com_sprintf( name, sizeof(name), "%s / %s", weaponInfo->weaponName, alt_weaponInfo->weaponName );
		} else {
			Com_sprintf( name, sizeof(name), "%s", weaponInfo->weaponName );
		}

//		w = CG_DrawStrlen( name ) * MEDIUMCHAR_WIDTH; //BIGCHAR_WIDTH;
//		x = ( SCREEN_WIDTH - w ) / 2;
		//CG_DrawSmallStringColor(6, y - 30, name, color);
	}

	trap_R_SetColor( NULL );
}

void CG_DrawWeaponSelect( void ) {
	cg.itemPickupTime = 0;
	CG_DrawWeaponSelectHorCenterBar();
	//CG_DrawWeaponSelectQuarterFan();
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {
//	if ( !cg.snap->ps.ammo[i] ) {
//		return qfalse;
//	}
	if ( ! (cg.snap->ps.stats[ skills ] & ( 1 << i ) ) ) {
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap || cg.snap->ps.powerups[PW_MELEE_STATE] ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	// JUHOX: select effect in lens flare editor
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) {
		cg.lfEditor.selectedEffect++;
		if (cg.lfEditor.selectedEffect < 0) cg.lfEditor.selectedEffect = 0;
		if (cg.lfEditor.selectedEffect >= cgs.numLensFlareEffects) {
			cg.lfEditor.selectedEffect = cgs.numLensFlareEffects - 1;
		}
		return;
	}
#endif

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect++;
		if ( cg.weaponSelect == 16 ) {
			cg.weaponSelect = 0;
		}
//		if ( cg.weaponSelect == WP_GAUNTLET ) {
//			continue;		// never cycle to gauntlet
//		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	// JUHOX: select effect in lens flare editor
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) {
		cg.lfEditor.selectedEffect--;
		if (cg.lfEditor.selectedEffect < 0) cg.lfEditor.selectedEffect = 0;
		if (cg.lfEditor.selectedEffect >= cgs.numLensFlareEffects) {
			cg.lfEditor.selectedEffect = cgs.numLensFlareEffects - 1;
		}
		return;
	}
#endif

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 0 ; i < 16 ; i++ ) {
		cg.weaponSelect--;
		if ( cg.weaponSelect == -1 ) {
			cg.weaponSelect = 15;
		}
//		if ( cg.weaponSelect == WP_GAUNTLET ) {
//			continue;		// never cycle to gauntlet
//		}
		if ( CG_WeaponSelectable( cg.weaponSelect ) ) {
			break;
		}
	}
	if ( i == 16 ) {
		cg.weaponSelect = original;
	}
}


/*
===============
JUHOX: CG_AddLensFlareEntity
===============
*/
#if MAPLENSFLARES
static lensFlareEntity_t* CG_AddLensFlareEntity(const lensFlareEntity_t* model) {
	int entnum;
	lensFlareEntity_t* lfent;

	if (cgs.numLensFlareEntities >= MAX_LIGHTS_PER_MAP) return NULL;

	entnum = cgs.numLensFlareEntities++;
	lfent = &cgs.lensFlareEntities[entnum];

	memset(lfent, 0, sizeof(*lfent));
	VectorCopy(cg.snap->ps.origin, lfent->origin);
	if (model) {
		lfent->radius = model->radius;
		lfent->lightRadius = model->lightRadius;
		lfent->lfeff = model->lfeff;
		VectorCopy(model->dir, lfent->dir);
		lfent->angle = model->angle;
	}
	else {
		lfent->radius = 5.0;
		lfent->lfeff = &cgs.lensFlareEffects[cg.lfEditor.selectedEffect];
		lfent->dir[0] = 1.0;
		lfent->angle = -1;
	}
	CG_ComputeMaxVisAngle(lfent);

	cg.lfEditor.originalLFEnt = *lfent;

	return lfent;
}
#endif

/*
===============
JUHOX: CG_DeleteLensFlareEntity
===============
*/
#if MAPLENSFLARES
static void CG_DeleteLensFlareEntity(lensFlareEntity_t* lfent) {
	int lfentnum;
	int i;

	lfentnum = lfent - cgs.lensFlareEntities;
	if (lfentnum < 0) return;
	if (lfentnum >= cgs.numLensFlareEntities) return;

	for (i = lfentnum; i < cgs.numLensFlareEntities-1; i++) {
		cgs.lensFlareEntities[i] = cgs.lensFlareEntities[i+1];
	}
	cgs.numLensFlareEntities--;
}
#endif

/*
=========================
JUHOX: CG_NearestMover
=========================
*/
/*
#if MAPLENSFLARES
static centity_t* CG_NearestMover(const vec3_t pos) {
	int i;
	centity_t* nearestMover;
	float nearestDistanceSqr;

	nearestMover = NULL;
	nearestDistanceSqr = Square(100000.0);
	for (i = MAX_CLIENTS; i < ENTITYNUM_WORLD; i++) {
		centity_t* cent;
		float distanceSqr;

		cent = &cg_entities[i];
		if (cent->currentState.eType != ET_MOVER) continue;

		distanceSqr = DistanceSquared(pos, cent->lerpOrigin);
		if (distanceSqr > nearestDistanceSqr) continue;

		nearestDistanceSqr = distanceSqr;
		nearestMover = cent;
	}
	return nearestMover;
}
#endif
*/

/*
=========================
JUHOX: CG_FindNextEntityByEffect
=========================
*/
#if MAPLENSFLARES
static int CG_FindNextEntityByEffect(void) {
	const lensFlareEffect_t* lfeff;
	int start;
	int i;

	lfeff = &cgs.lensFlareEffects[cg.lfEditor.selectedEffect];
	start = -1;
	if (cg.lfEditor.selectedLFEnt) {
		start = cg.lfEditor.selectedLFEnt - cgs.lensFlareEntities;
	}
	for (i = 1; i <= cgs.numLensFlareEntities; i++) {
		int j;
		const lensFlareEntity_t* lfent;

		j = (start + i) % cgs.numLensFlareEntities;
		lfent = &cgs.lensFlareEntities[j];
		if (lfent->lfeff == lfeff) return j;
	}
	return -1;
}
#endif

/*
=========================
JUHOX: CG_EvaluateViewDir
=========================
*/
#if MAPLENSFLARES
#define MAX_VIEWTESTS 200
static vec3_t viewDirList[MAX_VIEWTESTS];
static float CG_EvaluateViewDir(const vec3_t viewDir, int listSize) {
	float evaluation;
	int i;

	evaluation = 0;
	for (i = 0; i < listSize; i++) {
		float d;

		d = DotProduct(viewDir, viewDirList[i]);
		if (d <= 0) continue;

		evaluation += d;
	}
	return evaluation;
}
#endif

/*
=========================
JUHOX: CG_FindBestViewOrg
=========================
*/
#if MAPLENSFLARES
static void CG_FindBestViewOrg(const lensFlareEntity_t* lfent, vec3_t viewOrg) {
	vec3_t origin;
	float viewDistance;
	int i;
	int listSize;
	float bestEvaluation;
	int bestDir;

	CG_LFEntOrigin(lfent, origin);
	VectorCopy(origin, viewOrg);

	viewDistance = 3 * lfent->radius + 100;
	listSize = 0;

	for (i = 0; i < MAX_VIEWTESTS; i++) {
		vec3_t dir;
		vec3_t start;
		vec3_t end;
		trace_t trace;
		vec3_t candidate;
		//float evaluation;

		dir[0] = crandom();
		dir[1] = crandom();
		dir[2] = crandom();
		if (VectorNormalize(dir) < 0.001) continue;

		VectorMA(origin, lfent->radius, dir, start);
		VectorMA(origin, viewDistance, dir, end);
		CG_Trace(&trace, start, NULL, NULL, end, -1, MASK_OPAQUE|CONTENTS_BODY);
		if (trace.fraction < 0.1) continue;
		
		VectorMA(trace.endpos, -10, dir, candidate);
		CG_Trace(&trace, candidate, NULL, NULL, start, -1, MASK_OPAQUE|CONTENTS_BODY);
		if (trace.fraction < 1) continue;

		VectorSubtract(candidate, origin, viewDirList[listSize]);
		listSize++;	// CAUTION: don't add this to the line above -- VectorSubtract() is a macro!
	}

	bestEvaluation = 0;
	bestDir = -1;
	for (i = 0; i < listSize; i++) {
		float evaluation;

		evaluation = CG_EvaluateViewDir(viewDirList[i], listSize);
		if (evaluation <= bestEvaluation) continue;

		bestEvaluation = evaluation;
		bestDir = i;
	}

	if (bestDir >= 0) {
		VectorAdd(origin, viewDirList[bestDir], viewOrg);
	}
}
#endif

/*
=========================
JUHOX: CG_NextMover
=========================
*/
#if MAPLENSFLARES
static centity_t* CG_NextMover(centity_t* current) {
	int start, i;

	if (!current) current = &cg_entities[0];

	start = current - cg_entities;
	for (i = 1; i <= MAX_GENTITIES; i++) {
		current = &cg_entities[(start + i) % MAX_GENTITIES];
		if (current->currentState.eType != ET_MOVER) continue;
		if (!current->currentValid) continue;

		return current;
	}
	return NULL;
}
#endif

/*
=========================
JUHOX: CG_HandleCopyOptions
=========================
*/
#if MAPLENSFLARES
static void CG_HandleCopyOptions(int command) {
	switch (command) {
	case 1:	// cancel
		cg.lfEditor.cmdMode = LFECM_main;
		break;
	case 2:	// effect
		cg.lfEditor.copyOptions ^= LFECO_EFFECT;
		break;
	case 3:	// vis radius
		cg.lfEditor.copyOptions ^= LFECO_VISRADIUS;
		break;
	case 4:	// light radius
		cg.lfEditor.copyOptions ^= LFECO_LIGHTRADIUS;
		break;
	case 5:	// spot light direction
		cg.lfEditor.copyOptions ^= LFECO_SPOT_DIR;
		break;
	case 6:	// spot light entity angle
		cg.lfEditor.copyOptions ^= LFECO_SPOT_ANGLE;
		break;
	}
}
#endif

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	if ( num < 1 || num > 15 ) {
		return;
	}

	// JUHOX: in lens flare editor weapon chooses a command
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) {
		if (num != 3) cg.lfEditor.delAck = qfalse;

		if (cg.lfEditor.cmdMode == LFECM_copyOptions) {
			CG_HandleCopyOptions(num);
			return;
		}

		switch (num) {
		case 1:	// cancel
			if (cg.lfEditor.selectedLFEnt) {
				*cg.lfEditor.selectedLFEnt = cg.lfEditor.originalLFEnt;
			}
			cg.lfEditor.selectedLFEnt = NULL;
			cg.lfEditor.editMode = LFEEM_none;
			CG_SetLFEdMoveMode(LFEMM_coarse);
			break;
		case 2:	// add lens flare entity / switch mover state
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (cg.lfEditor.editMode == LFEEM_none) {
					cg.lfEditor.selectedLFEnt = CG_AddLensFlareEntity(cg.lfEditor.selectedLFEnt);
					if (cg.lfEditor.selectedLFEnt) {
						cg.lfEditor.editMode = LFEEM_pos;
						CG_SetLFEdMoveMode(LFEMM_coarse);
					}
				}
			}
			else {
				cg.lfEditor.moversStopped = !cg.lfEditor.moversStopped;
			}
			break;
		case 3: // delete lens flare entity / select mover
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (cg.lfEditor.selectedLFEnt) {
					if (!cg.lfEditor.delAck) {
						cg.lfEditor.delAck = qtrue;
					}
					else {
						cg.lfEditor.delAck = qfalse;
						CG_DeleteLensFlareEntity(cg.lfEditor.selectedLFEnt);
						cg.lfEditor.selectedLFEnt = NULL;
						cg.lfEditor.editMode = LFEEM_none;
						CG_SetLFEdMoveMode(LFEMM_coarse);
					}
				}
			}
			else if (cg.lfEditor.moversStopped) {
				cg.lfEditor.selectedMover = CG_NextMover(cg.lfEditor.selectedMover);
			}
			break;
		case 4:	// edit position & vis radius / lock to mover
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (cg.lfEditor.selectedLFEnt) {
					if (cg.lfEditor.editMode == LFEEM_pos) {
						cg.lfEditor.editMode = LFEEM_none;
						CG_SetLFEdMoveMode(LFEMM_coarse);
					}
					else {
						cg.lfEditor.editMode = LFEEM_pos;
						CG_SetLFEdMoveMode(LFEMM_fine);
					}
				}
			}
			else if (cg.lfEditor.selectedLFEnt) {
				if (cg.lfEditor.selectedLFEnt->lock) {
					VectorAdd(
						cg.lfEditor.selectedLFEnt->origin,
						cg.lfEditor.selectedLFEnt->lock->lerpOrigin,
						cg.lfEditor.selectedLFEnt->origin
					);
					cg.lfEditor.selectedLFEnt->lock = NULL;
				}
				else {
					if (cg.lfEditor.selectedMover && cg.lfEditor.moversStopped) {
						cg.lfEditor.selectedLFEnt->lock = cg.lfEditor.selectedMover;
						VectorSubtract(
							cg.lfEditor.selectedLFEnt->origin,
							cg.lfEditor.selectedLFEnt->lock->lerpOrigin,
							cg.lfEditor.selectedLFEnt->origin
						);
					}
					else {
						CG_Printf("no mover selected\n");
					}
				}
			}
			break;
		case 5:	// edit target & angle / search entity
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (cg.lfEditor.selectedLFEnt) {
					if (cg.lfEditor.editMode == LFEEM_target) {
						cg.lfEditor.editMode = LFEEM_none;
						CG_SetLFEdMoveMode(LFEMM_coarse);
					}
					else {
						cg.lfEditor.editMode = LFEEM_target;
						CG_SetLFEdMoveMode(LFEMM_fine);
						cg.lfEditor.editTarget = qtrue;
					}
				}
			}
			else {
				int nextLFEnt;

				nextLFEnt = CG_FindNextEntityByEffect();
				if (nextLFEnt >= 0) {
					const lensFlareEntity_t* lfent;
					vec3_t viewOrg;
					vec3_t dir;
					vec3_t angles;

					CG_SelectLFEnt(nextLFEnt);
					
					lfent = &cgs.lensFlareEntities[nextLFEnt];
					CG_FindBestViewOrg(lfent, viewOrg);
					CG_LFEntOrigin(lfent, dir);
					VectorSubtract(dir, viewOrg, dir);
					vectoangles(dir, angles);
					trap_SendClientCommand(
						va(
							"lfemm %d %f %f %f %f %f %f",
							cg.lfEditor.moveMode,
							viewOrg[0], viewOrg[1], viewOrg[2],
							angles[0], angles[1], angles[3]
						)
					);
				}
				else {
					CG_Printf("No flare entity found with '%s'\n", cgs.lensFlareEffects[cg.lfEditor.selectedEffect]);
				}
			}
			break;
		case 6:	// edit light size & vis radius / copy options
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (cg.lfEditor.selectedLFEnt) {
					if (cg.lfEditor.editMode == LFEEM_radius) {
						cg.lfEditor.editMode = LFEEM_none;
						CG_SetLFEdMoveMode(LFEMM_coarse);
					}
					else {
						cg.lfEditor.editMode = LFEEM_radius;
						CG_SetLFEdMoveMode(LFEMM_fine);
					}
				}
			}
			else {
				cg.lfEditor.cmdMode = LFECM_copyOptions;
			}
			break;
		case 7:	// assign effect / paste lf entity
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (
					cg.lfEditor.selectedLFEnt &&
					cg.lfEditor.selectedEffect >= 0 &&
					cg.lfEditor.selectedEffect < cgs.numLensFlareEffects
				) {
					cg.lfEditor.selectedLFEnt->lfeff = &cgs.lensFlareEffects[cg.lfEditor.selectedEffect];
					CG_ComputeMaxVisAngle(cg.lfEditor.selectedLFEnt);
				}
			}
			else if (cg.lfEditor.selectedLFEnt) {
				lensFlareEntity_t* lfent;

				lfent = cg.lfEditor.selectedLFEnt;
				if (cg.lfEditor.copyOptions & LFECO_EFFECT) {
					if (cg.lfEditor.copiedLFEnt.lfeff) {
						lfent->lfeff = cg.lfEditor.copiedLFEnt.lfeff;
						CG_ComputeMaxVisAngle(lfent);
					}
				}
				if (cg.lfEditor.copyOptions & LFECO_VISRADIUS) {
					lfent->radius = cg.lfEditor.copiedLFEnt.radius;
					if (lfent->lightRadius > lfent->radius) {
						lfent->lightRadius = lfent->radius;
					}
				}
				if (cg.lfEditor.copyOptions & LFECO_LIGHTRADIUS) {
					lfent->lightRadius = cg.lfEditor.copiedLFEnt.lightRadius;
					if (lfent->radius < lfent->lightRadius) {
						lfent->radius = lfent->lightRadius;
					}
				}
				if (cg.lfEditor.copyOptions & LFECO_SPOT_DIR) {
					VectorCopy(cg.lfEditor.copiedLFEnt.dir, lfent->dir);
				}
				if (cg.lfEditor.copyOptions & LFECO_SPOT_ANGLE) {
					lfent->angle = cg.lfEditor.copiedLFEnt.angle;
				}
			}
			break;
		case 8:	// note effect / copy lf entity
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				if (
					cg.lfEditor.selectedLFEnt &&
					cg.lfEditor.selectedLFEnt->lfeff &&
					cgs.numLensFlareEffects > 0
				) {
					cg.lfEditor.selectedEffect = cg.lfEditor.selectedLFEnt->lfeff - cgs.lensFlareEffects;
				}
			}
			else {
				if (cg.lfEditor.selectedLFEnt) {
					cg.lfEditor.copiedLFEnt = *(cg.lfEditor.selectedLFEnt);
				}
			}
			break;
		case 9:	// draw mode / cursor size
			if (!(cg.lfEditor.oldButtons & BUTTON_WALKING)) {
				cg.lfEditor.drawMode++;
				if (cg.lfEditor.drawMode < 0 || cg.lfEditor.drawMode > LFEDM_none) {
					cg.lfEditor.drawMode = 0;
				}
			}
			else {
				if (cg.lfEditor.selectedLFEnt) {
					cg.lfEditor.cursorSize++;
					if (cg.lfEditor.cursorSize < 0 || cg.lfEditor.cursorSize > LFECS_visRadius) {
						cg.lfEditor.cursorSize = 0;
					}
				}
			}
			break;
		}
		return;
	}
#endif

	cg.weaponSelectTime = cg.time;

	if ( ! ( cg.snap->ps.stats[skills] & ( 1 << num ) ) ) {
		return;		// don't have the weapon
	}

	cg.weaponSelect = num;
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int		i;

	cg.weaponSelectTime = cg.time;

	for ( i = 15 ; i > 0 ; i-- ) {
		if ( CG_WeaponSelectable( i ) ) {
			cg.weaponSelect = i;
			break;
		}
	}
}



/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent, qboolean altFire ) {
	cg_userWeapon_t		*weaponGraphics;
	entityState_t		*ent;
	int					c, weapNr;

	ent = &cent->currentState;

	if ( ent->weapon == WP_NONE ) {
		return;
	}

	// Set the muzzle flash weapon Nr
	if ( altFire ) {
		weapNr = ent->weapon + ALTWEAPON_OFFSET;
	} else {
		weapNr = ent->weapon;
	}
	weaponGraphics = CG_FindUserWeaponGraphics(ent->clientNum, weapNr );
	
	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model.
	cent->muzzleFlashTime = cg.time;

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( weaponGraphics->flashSound[c] == 0 ) {
			break;
		}
	}
	
	if ( c > 0 ) {
		c = rand() % c;
		if ( weaponGraphics->flashSound[c] )
		{
			trap_S_StartSound( NULL , ent->number, CHAN_WEAPON, weaponGraphics->flashSound[c] );
		}
	}

	if ( weaponGraphics->flashParticleSystem[0] ) {
		PSys_SpawnCachedSystem( weaponGraphics->flashParticleSystem, cent->lerpOrigin, NULL, cent, weaponGraphics->chargeTag[0], qfalse, qfalse );
	}
}

/*
==================
CG_UserRailTrail
==================
Caused by an EV_RAILTRAIL event
*/
void CG_UserRailTrail( int weapon, int clientNum, vec3_t start, vec3_t end) {
	cg_userWeapon_t		*weaponGraphics;
	localEntity_t		*le;
	refEntity_t			*re;
	
	weaponGraphics = CG_FindUserWeaponGraphics( clientNum, weapon );

	if ( !weaponGraphics->missileTrailShader ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = weaponGraphics->missileTrailShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA[0] = 255;
    re->shaderRGBA[1] = 255;
    re->shaderRGBA[2] = 255;
    re->shaderRGBA[3] = 255;

	le->color[0] = 0.75;
	le->color[1] = 0.75;
	le->color[2] = 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );
 
	/*
	// nudge the rail down just a tiny bit
	re->origin[2] -=8
	re->oldorigin[2] -= 8;
	*/
}


/*
======================
CG_UserMissileHitWall
======================
Caused by an EV_MISSILE_MISS event
*/
void CG_UserMissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, qboolean inAir ) {
	cg_userWeapon_t		*weaponGraphics;
	vec3_t end;
	trace_t tr;
	//qhandle_t			mark;
	int					c;

	weaponGraphics = CG_FindUserWeaponGraphics( clientNum, weapon );
	//mark = cgs.media.burnMarkShader;

	
	// play an explosion sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weaponGraphics->explosionSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weaponGraphics->explosionSound[c] )
		{
			trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, weaponGraphics->explosionSound[c] );
		}
	}

	// Create Explosion
	CG_MakeUserExplosion( origin, dir, weaponGraphics);

	if ( !inAir ) {
		vec3_t tempAxis[3];
		VectorNormalize2( dir, tempAxis[0] );

		MakeNormalVectors( tempAxis[0], tempAxis[1], tempAxis[2] );

		VectorCopy(origin, end);
		end[2] -= 64;
		CG_Trace( &tr, origin, NULL, NULL, end, -1, MASK_PLAYERSOLID );

		if (cg_particlesQuality.value) {
			if (tr.surfaceFlags & SURF_METALSTEPS){
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			} else if (tr.surfaceFlags & SURF_FLESH){
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			} else if (tr.surfaceFlags & SURF_DUST){
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			} else {
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebris", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			}
		}else{
			if (tr.surfaceFlags & SURF_METALSTEPS){
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			} else if (tr.surfaceFlags & SURF_FLESH){
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			} else if (tr.surfaceFlags & SURF_DUST){
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			} else {
				if(weaponGraphics->explosionSize <= 10){
					PSys_SpawnCachedSystem( "SmallExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 25){
					PSys_SpawnCachedSystem( "NormalExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else if(weaponGraphics->explosionSize <= 50){
					PSys_SpawnCachedSystem( "LargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}else{
					PSys_SpawnCachedSystem( "ExtraLargeExplosionDebrisLow", origin, tempAxis, NULL, NULL, qfalse, qfalse );
				}
			}
		}

		if (weaponGraphics->markSize && weaponGraphics->markShader) {
			// Draw Impactmark
			CG_ImpactMark( weaponGraphics->markShader, origin, dir, random()*360, 1,1,1,1, qfalse,60, qfalse );
		}
	}

	
/* NOTE: Find another way of doing this in the new system...
	if ( weaponGraphics->missileTrailFunc == CG_TrailFunc_StraightBeam ||
		 weaponGraphics->missileTrailFunc == CG_TrailFunc_SpiralBeam ) {
		CG_CreateStraightBeamFade( cgs.clientinfo[ clientNum ].weaponTagPos0, origin, weaponGraphics);
	}
*/

}


/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	vec3_t			sprOrg;
	vec3_t			sprVel;

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:
	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_lghit2;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		if (cg_oldRocket.integer == 0) {
			// explosion sprite animation
			VectorMA( origin, 24, dir, sprOrg );
			VectorScale( dir, 64, sprVel );

			CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 20, 30 );
		}
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		sfx = cgs.media.sfx_plasmaexp;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;
	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;

		r = rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
		if ( weapon == WP_RAILGUN ) {
			// colorize with client color
			VectorCopy( cgs.clientinfo[clientNum].color1, le->color );
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if ( weapon == WP_RAILGUN ) {
		float	*color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].color2;
		CG_ImpactMark( mark, origin, dir, random()*360, color[0],color[1], color[2],1, alphaFade, radius, qfalse );
	} else {
		CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
	}
}


void CG_UserMissileHitPlayer( int weapon, int clientNum, vec3_t origin, vec3_t dir, int entityNum ) {
	CG_Bleed( origin, entityNum );

	CG_UserMissileHitWall( weapon, clientNum, origin, dir, qtrue );
}

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:
#ifdef MISSIONPACK
	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER:
#endif
		CG_MissileHitWall( weapon, 0, origin, dir, IMPACTSOUND_FLESH );
		break;
	default:
		break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet( vec3_t start, vec3_t end, int skipNum ) {
	trace_t		tr;
	int sourceContentType, destContentType;

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );

	sourceContentType = trap_CM_PointContents( start, 0 );
	destContentType = trap_CM_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );
	}

	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		CG_MissileHitPlayer( WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum );
	} else {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if ( tr.surfaceFlags & SURF_METALSTEPS ) {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
		} else {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
static void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_SHOTGUN_COUNT ; i++ ) {
		r = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
		u = Q_crandom( &seed ) * DEFAULT_SHOTGUN_SPREAD * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		CG_ShotgunPellet( origin, end, otherEntNum );
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es ) {
	vec3_t	v;
	int		contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t			up;

		contents = trap_CM_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 8 );
			CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
		}
	}
	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}

/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );

}


/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = trap_CM_PointContents( start, 0 );
			destContentType = trap_CM_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, 32 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, 32 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( trace.endpos, end, 32 );
			}

			// draw a tracer
			if ( random() < cg_tracerChance.value ) {
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		CG_Bleed( end, fleshEntityNum );
	} else {
		CG_MissileHitWall( WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT );
	}

}
