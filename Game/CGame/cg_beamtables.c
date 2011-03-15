// Copyright (C) 2003-2004 RiO
//
// cg_beamtables.c -- Stores information for and renders curved beams.

#include "cg_local.h"

#define BEAMTABLE_UPDATE	 750	// 750 ms between updates
#define	MAX_BEAMTABLE_SIZE	  32	// Should be more than enough for any beam, unless
									// it lasts MUCH longer than is good for gameplay.
/*
#define TESS_DISTANCE		  20    // The distance required for one tesselation of a
									// beam segment.
*/

typedef struct beamTableElem_s {
	struct beamTableElem_s		*prev, *next;
	vec3_t						pos, tangent;
} beamTableElem_t;

typedef struct {
	beamTableElem_t		table[MAX_BEAMTABLE_SIZE];
	beamTableElem_t		table_activeList;
	beamTableElem_t		*table_freeList;
	qboolean			activeThisFrame;
	qboolean			alreadyWiped;
	int					updateTime;
	qhandle_t			shader;
	char				tagName[MAX_QPATH];
	float				width;
} beamTable_t;


static beamTable_t		beamTablePrimary[MAX_CLIENTS];
static beamTable_t		beamTableAlternate[MAX_CLIENTS];


/*
   -- Table Management Functions --
*/

/*
==================
CG_WipeBeamTable
==================
  Wipes the given beamtable clean, preparing it for a new set of waypoints.
*/
static void CG_WipeBeamTable( beamTable_t *beamTable ) {
	int i;

	if (!beamTable->alreadyWiped) {

		memset( beamTable, 0, sizeof(beamTable_t) );
	
		beamTable->table_activeList.next = &(beamTable->table_activeList);
		beamTable->table_activeList.prev = &(beamTable->table_activeList);
		beamTable->table_freeList = beamTable->table;

		// singly link the free list
		for ( i = 0 ; i < MAX_BEAMTABLE_SIZE - 1 ; i++ ) {
			beamTable->table[i].next = &(beamTable->table[i+1]);
		}

		beamTable->alreadyWiped = qtrue;
	}
	
	// Ensure we can always do an update after a shorter time
	// for the first segment. This keeps the beam from 'weirding out',
	// and doing things like cutting through the ground
	beamTable->updateTime = cg.time + BEAMTABLE_UPDATE / 3.0f; 
}


/*
===================
CG_InitBeamTables
===================
  This is called at startup and for tournement restarts.
*/
void CG_InitBeamTables( void ) {
	int		i;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		// Explicitly set these two to enforce proper initialization
		// in CG_WipeBeamTable.
		beamTablePrimary[i].alreadyWiped = qfalse;
		beamTableAlternate[i].alreadyWiped = qfalse;

		CG_WipeBeamTable( &beamTablePrimary[i] );
		CG_WipeBeamTable( &beamTableAlternate[i] );
	}
}


/*
========================
CG_AllocBeamTableElem
========================
  Will always succeed, even if it requires returning the previous tableElem.
*/
static beamTableElem_t *CG_AllocBeamTableElem( beamTable_t *beamTable ) {
	beamTableElem_t	*tableElem;

	if ( !(tableElem = beamTable->table_freeList) ) {
		// No free entities, so use the one at the end of the chain.
		tableElem = beamTable->table_activeList.next;
		return tableElem;
	}

	beamTable->table_freeList = beamTable->table_freeList->next;
		
	// link into the active list
	tableElem->next = beamTable->table_activeList.next;
	tableElem->prev = &(beamTable->table_activeList);
	beamTable->table_activeList.next->prev = tableElem;
	beamTable->table_activeList.next = tableElem;

	return tableElem;
}


/*
====================
CG_BeamTableUpdate
====================
  Updates the beamtable associated with the client entity.
*/
void CG_BeamTableUpdate( centity_t *cent, float width, qhandle_t shader, char *tagName ) {
	beamTable_t *currentTable;

	// Failsafe against arrays running out of bounds.
	if ( ( cent->currentState.clientNum < 0 ) || ( cent->currentState.clientNum >= MAX_CLIENTS ) ) {
		CG_Error( "Bad clientNum on beamhead entity" );
		return; // Don't know if return is necessary, but do it anyway.
	}

	// Do we work on the alternate fire tables, or the primary fire ones?
	if ( cent->currentState.weapon > ALTWEAPON_OFFSET) {
		currentTable = &(beamTableAlternate[cent->currentState.clientNum]);
	} else {
		currentTable = &(beamTablePrimary[cent->currentState.clientNum]);
	}

	// set the width, shader and tag to lock on to.
	currentTable->width = width;
	currentTable->shader = shader;
	Q_strncpyz(currentTable->tagName, tagName, sizeof(currentTable->tagName));

	// always update the terminator symbol
	VectorCopy(cent->lerpOrigin, currentTable->table_activeList.pos);
	BG_EvaluateTrajectoryDelta( &(cent->currentState), &(cent->currentState.pos), cg.time, currentTable->table_activeList.tangent );
	if ( !VectorNormalize( currentTable->table_activeList.tangent ) ) {
		VectorSet( currentTable->table_activeList.tangent, 0, 0, 1);
	}

	// If the entity has reached or exceeded the spawntime of a new waypoint,
	// place a new one.
	if ( currentTable->updateTime < cg.time ) {
		beamTableElem_t *tableElem;

		// allocate a waypoint cell
		tableElem = CG_AllocBeamTableElem( currentTable );
		
		// get the position and tangent from the terminator, and store them
		VectorCopy( currentTable->table_activeList.pos, tableElem->pos );
		VectorCopy( currentTable->table_activeList.tangent, tableElem->tangent );

		// set the next update time
		currentTable->updateTime = cg.time + BEAMTABLE_UPDATE;
	}

	// mark the table as having been active
	currentTable->activeThisFrame = qtrue;
	currentTable->alreadyWiped = qfalse;
}


/*
   -- Drawing Functions --
*/

/*
====================
CG_BezierMidPoints
====================
  Calculates the mid points to form a set of four control points.
  Uses the tangent (aka beamhead direction) to do so.
*/
static void CG_BezierMidPoints( const vec3_t startPos,   const vec3_t endPos,
								const vec3_t startTan,   const vec3_t endTan,
								      vec3_t midPos1,          vec3_t midPos2 ) {
	float		tangentScale;

	// Set the scale of the tangents to reach halfway. This will give fluent curves
	// through the calculated midpoints we're going to calculate.
	tangentScale = 0.5 * Distance(startPos, endPos);

	// Scale and add startTan to the startPos to get midPos1
	VectorCopy( startTan, midPos1 );
	VectorNormalize( midPos1 );
	VectorScale( midPos1, tangentScale, midPos1 );
	VectorAdd( startPos, midPos1, midPos1 );

	// Invert, scale and add endTan to the endPos to get midPos2
	VectorCopy( endTan, midPos2 );
	VectorNormalize( midPos2 );
	VectorScale( midPos2, -tangentScale, midPos2 );
	VectorAdd( endPos, midPos2, midPos2 );
}

/*
================
CG_BezierPoint
================
  Calculates the position and tangent within a bezier curve at a set
  parameter value between 0 and 1.
*/
static void CG_BezierPoint( const vec3_t point0, const vec3_t point1,
							const vec3_t point2, const vec3_t point3,
							float t, vec3_t outPoint, vec3_t outTangent ) {
	float T[4];
	float H[4];
	vec3_t tmp[4];

	// Initialize the T-matrix for the point;
	T[0] = t * t * t;
	T[1] = t * t;
	T[2] = t;
	T[3] = 1;

	// Initialize the H-matrix
	H[0] = -1 * T[0] + 3 * T[1] - 3 * T[2] + 1 * T[3];
	H[1] =  3 * T[0] - 6 * T[1] + 3 * T[2];
	H[2] = -3 * T[0] + 3 * T[1];
	H[3] =  1 * T[0];

	// Calculate the new point
	VectorScale(point0, H[0], tmp[0]);
	VectorScale(point1, H[1], tmp[1]);
	VectorScale(point2, H[2], tmp[2]);
	VectorScale(point3, H[3], tmp[3]);
	VectorAdd( tmp[0], tmp[1], outPoint);
	VectorAdd( tmp[2], outPoint, outPoint);
	VectorAdd( tmp[3], outPoint, outPoint);


	// Initialize the T-matrix for the tangent;
	T[0] = 3 * t * t;
	T[1] = 2 * t;
	T[2] = 1;
	T[3] = 0;

	// Re-initialize the H-matrix
	H[0] = -1 * T[0] + 3 * T[1] - 3 * T[2] + 1 * T[3];
	H[1] =  3 * T[0] - 6 * T[1] + 3 * T[2];
	H[2] = -3 * T[0] + 3 * T[1];
	H[3] =  1 * T[0];

	// Calculate the new point's tangent
	VectorScale(point0, H[0], tmp[0]);
	VectorScale(point1, H[1], tmp[1]);
	VectorScale(point2, H[2], tmp[2]);
	VectorScale(point3, H[3], tmp[3]);
	VectorAdd( tmp[0], tmp[1], outTangent);
	VectorAdd( tmp[2], outTangent, outTangent);
	VectorAdd( tmp[3], outTangent, outTangent);
}

/*
================
CG_BezierVerts
================
  Takes a point and tangent and makes a drawable set of two vertices out of it
*/
static void CG_BezierVerts (vec3_t point, vec3_t tangent, float width, polyVert_t *verts) {
	vec3_t offset, viewLine, dummyPoint;
	float len;
	// int i, j;
	
	VectorSubtract (point, cg.refdef.vieworg, viewLine);
	CrossProduct (viewLine, tangent, offset);
	len = VectorNormalize (offset);
	
	// HACK: if len would be 0, then we wouldn't get a proper width, so
	//       in this case nudge the point upwards by 0.1 on its z-coordinate.
	if ( !len ) {
		VectorCopy( point, dummyPoint );
		dummyPoint[2] += 0.1f;
		VectorSubtract (dummyPoint, cg.refdef.vieworg, viewLine);
		CrossProduct (viewLine, tangent, offset);
		VectorNormalize (offset);
	}

	VectorMA (point, width, offset, verts[0].xyz);
	VectorMA (point, -width, offset, verts[1].xyz);
	
	/*
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			verts[i].modulate[j] = 255;
		}
	}
	*/
}

static void CG_ShiftVerts( polyVert_t *verts ) {
	VectorCopy( verts[0].xyz, verts[3].xyz );
	VectorCopy( verts[1].xyz, verts[2].xyz );
}


// NOTE: This old way of doing the tesscount is bad for beams that
//       have wide curves. It uses more tesselations than is
//       affordable.
/*
static int CG_TessCount (const vec3_t startPos, const vec3_t endPos) {
	float	dist;
	int		tess;
	

	dist = Distance(startPos, endPos);
	tess = dist / TESS_DISTANCE;
	if ( tess < 2 ) {
		tess = 2;
	}

	return tess;
}
*/
static int CG_TessCount( const beamTableElem_t *startElem, const beamTableElem_t *endElem ) {
	float	directTangentRatio;
	float	indirectTangentRatio;
	float	tangentRatio;
	float   dist;
	vec3_t	dir;
	int		detail;

	VectorSubtract( endElem->pos, startElem->pos, dir );
	if ( (dist = VectorNormalize( dir )) == 0.0f ) {
		//If the distance is 0, then we don't make ANY tesselations at all.
		return 0;
	}

	directTangentRatio = 1 - DotProduct( startElem->tangent, endElem->tangent );
	indirectTangentRatio = 1 - DotProduct( dir, endElem->tangent );
	// NOTE: These should produce numbers between 0 and 2. 2 When the tangents
	//       differ 180 degrees, 0 when they do not differ.
	
	// Pick the biggest one
	tangentRatio = indirectTangentRatio > directTangentRatio ? indirectTangentRatio : directTangentRatio;

	// Clamp the detail value
	if ( r_beamDetail.value <= 0 ) {
		r_beamDetail.value = 0;
		detail = 0;
	}

	if ( r_beamDetail.value >= 4 ) {
		detail = 40;
		if ( tangentRatio < 1 ) {
			tangentRatio = 1;
		}
	} else if ( r_beamDetail.value == 3 ) {
		detail = 30;
		if ( tangentRatio < 0.5 ) {
			tangentRatio = 0.5;
		}
	} else if ( r_beamDetail.value == 2 ) {
		detail = 20;
		if ( tangentRatio < 0.25 ) {
			tangentRatio = 0.25;
		}
	} else if ( r_beamDetail.value == 1 ) {
		detail = 10;
		if ( tangentRatio < 0.1 ) {
			tangentRatio = 0.1;
		}
	}

	// On small distances always use the maximum tangent ratio
	if ( dist < 100 ) {
		tangentRatio = 2;
	}

	return 1 + floor( tangentRatio * detail );
}



/*
==================
CG_DrawBeamTable
==================
  Draws one beam table.
*/
static void CG_DrawBeamTable ( int clientNum, qboolean alternate ) {
	beamTable_t			*currentTable;
	beamTableElem_t		*currentElem;
	beamTableElem_t		*prevElem;
	beamTableElem_t		starter;

	polyVert_t			verts[4];
	
	vec3_t				midPos1;
	vec3_t				midPos2;
	
	int					tessSize;
	vec3_t				tessPoint;
	vec3_t				tessTangent;

	int					i, j;
	float				t;

	// Initialize the polyVerts.
	memset( verts, 0, sizeof(verts) );

	verts[0].st[0] = 1;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 0;
	verts[2].st[0] = 0;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 1;
/*
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
*/
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			verts[i].modulate[j] = 255;
		}
	}

	// Failsafe against arrays running out of bounds.
	if ( ( clientNum < 0 ) || ( clientNum >= MAX_CLIENTS ) ) {
		CG_Error( "Bad clientNum in beamtable drawing" );
		return; // FIXME: Don't know if return is necessary, but it can't hurt either way.
	}

	// Do we work on the alternate fire tables, or the primary fire ones?
	if ( alternate ) {
		orientation_t orient;

		currentTable = &(beamTableAlternate[clientNum]);

		// retrieve the correct starter point
		if (!CG_GetTagOrientationFromPlayerEntity( &(cg_entities[clientNum]), currentTable->tagName, &orient)) {
			// If the tag can not be found, wipe the table and don't display the beam
			currentTable->activeThisFrame = qfalse;
		} else {
			VectorCopy( orient.origin, starter.pos);
			VectorCopy( orient.axis[0], starter.tangent );
		}


	} else {
		orientation_t orient;

		currentTable = &(beamTablePrimary[clientNum]);

		// retrieve the correct starter point
		if (!CG_GetTagOrientationFromPlayerEntity( &(cg_entities[clientNum]), currentTable->tagName, &orient)) {
			// If the tag can not be found, wipe the table and don't display the beam
			currentTable->activeThisFrame = qfalse;
		} else {
			VectorCopy( orient.origin, starter.pos);
			VectorCopy( orient.axis[0], starter.tangent );
		}
	}

	// If the beam table wasn't rendered last time, then no beam will be rendered this
	// time. Wipe the table in preperation for a new beam and return.
	if ( !currentTable->activeThisFrame ) {
		CG_WipeBeamTable( currentTable );
		return;
	}

	// Set the first set of vertices
	prevElem = &starter;
	CG_BezierVerts( prevElem->pos, prevElem->tangent, currentTable->width, verts );
	CG_ShiftVerts( verts );

	// Start going through the waypoint table
	currentElem = currentTable->table_activeList.prev;
	while ( 1 ) {

		// get the tesselation count for this segment
		//tessSize = CG_TessCount( prevElem->pos, currentElem->pos );
		tessSize = CG_TessCount( prevElem, currentElem );

		// get the midpoints for this segment
		CG_BezierMidPoints( prevElem->pos, currentElem->pos,
							prevElem->tangent, currentElem->tangent,
							midPos1, midPos2 );

		// generate the tesselations
		for ( i = 1; i <= tessSize; i++ ) {
			t = (float)i / (float)tessSize;

			// Get the next set of vertices to add to our polygon
			CG_BezierPoint( prevElem->pos, midPos1, midPos2, currentElem->pos, t, tessPoint, tessTangent );
			CG_BezierVerts( tessPoint, tessTangent, currentTable->width, verts );
			
			// Draw our polygon
			trap_R_AddPolyToScene( currentTable->shader, 4, verts );

			// Shift the new vertices to the back, over the old ones, to save them.
			CG_ShiftVerts( verts );
		}

		// break from the loop if we've just dealt with our final element
		if ( currentElem == &currentTable->table_activeList ) {
			break;
		}

		// save the previous element for next loop
		prevElem = currentElem;
		currentElem = currentElem->prev;
	}

	// reset the activeThisFrame marker for use by the next frame.
	currentTable->activeThisFrame = qfalse;
}


/*
==================
CG_AddBeamTables
==================
  Adds the beam tables to the render list.
*/
void CG_AddBeamTables( void ) {
	int i;

	for (i = 0; i < MAX_CLIENTS; i++) {
		CG_DrawBeamTable( i, qfalse );
		CG_DrawBeamTable( i, qtrue  );
	}
}
