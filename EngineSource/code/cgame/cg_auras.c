// Copyright (C) 2003-2004 RiO
//
// cg_auras.c -- generates and displays auras

#include "cg_local.h"

#define	AURA_TAILLENGTH		  48
#define AURA_WIDTH			  36

#define AURA_BURSTTIME		 300		// 300 ms duration of aura burst
#define AURA_RUBBLE_DIST	  48		// Distance allowed from ground before debris comes up

#define AURA_TEXTUREBREAK	0.72f		// Defines where the 'break' between tail and lobe is
#define AURA_BREAKANGLE		0.58f		// At which angle does the aura break between two lobes and a lobe and tail

#define	NR_AURASPIKES		  20
#define AURASPIKE_SIZE		14.0f
#define AURASPIKE_BORDER	 2.5f
#define AURASPIKE_TIME		 500


// What types of aura we are working with
typedef enum {
	AURA_FLAME,
	AURA_CONVEXHULL,
	AURA_ORB_OUTW,
	AURA_ORB_INW
} auraType_t;

typedef struct auraTag_s {
	vec3_t		pos_world;
	vec2_t		pos_screen;
	vec3_t		normal;
	float		length;
	qboolean	is_tail;
} auraTag_t;

// info needed for an aura system
typedef struct {
	auraType_t		type;
	
	vec3_t			RGBColor;
	qhandle_t		shader;
	qhandle_t		trailShader;
	float			scale;

	sfxHandle_t		startSound;
	sfxHandle_t		loopSound;

	qboolean		active;				// Is the aura active?

	int				burstTimer;			// Client time when last aura burst was started

	vec3_t			origin;
	vec3_t			delta;

	// VOLUMETRIC SPRITE AURA
	vec3_t			tailTip;			// Where the tip of the tail is located
	vec3_t			leftLobeTip;		// Where the tip of the left 'lobe' is located
	vec3_t			rightLobeTip;		// Where the tip of the right 'lobe' is located
	
	// CONVEX HULL AURA
	auraTag_t		hullPoints[MAX_AURATAGS + 1]; // +1 for tail point
	int				nr_hullPoints;
	float			hullCircumference;
} auraSystem_t;

// Make this static so it is not removed from memory
static auraSystem_t	playerAura[MAX_CLIENTS];


/*
===============
CG_QSortAngle
===============
Quicksort by ascending angles
*/
void CG_QSortAngle( auraTag_t *points, float* angles, int lowbound, int highbound ) {
	int			low, high;
	float		mid;
	auraTag_t	tempPoint;
	float		tempAngle;

	low = lowbound;
	high = highbound;
	mid = angles[(low + high) / 2];

	do {
		while ( angles[low]  < mid ) low++;
		while ( angles[high] > mid ) high--;

		if ( low <= high ) {
			// swap points
			tempPoint = points[low];
			points[low] = points[high];
			points[high] = tempPoint;

			// swap angles
			tempAngle = angles[low];
			angles[low] = angles[high];
			angles[high] = tempAngle;

			low++;
			high--;
		}

	} while ( !(low > high));

	if ( high > lowbound ) CG_QSortAngle( points, angles, lowbound, high );
	if ( low < highbound ) CG_QSortAngle( points, angles, low, highbound );	
}


/*
===================
CG_FindConvexHull
===================
Finds the convex hull of a set of points
*/
static qboolean CG_FindConvexHull( auraTag_t *points, int *nr_points ) {
	float		angles[MAX_AURATAGS + 1]; // +1 for tail
	int			amount, index, pivotIndex;
	auraTag_t	pivot;
	auraTag_t	behind, infront;
	qboolean	rightTurn;
	auraTag_t	vecPoint;
	auraTag_t	buffer[MAX_AURATAGS + 1];
	

	amount = *nr_points;	

	if ( amount == 3 ) return qtrue; // Already a convex hull
	if ( amount <  3 ) {
		return qfalse;
	}

	pivotIndex = 0;
	// Find pivot point, which is known to be on the hull.
	// Point with lowest y - if there are multiple, point with highest x.
	for ( index = 1; index < amount; index++ ) {

		if ( points[index].pos_screen[1] < points[pivotIndex].pos_screen[1] ) {
			pivotIndex = index;

		} else if ( points[index].pos_screen[1] == points[pivotIndex].pos_screen[1] ) {
			
			if (points[index].pos_screen[0] > points[pivotIndex].pos_screen[0] ) {
				pivotIndex = index;
			}
		}
	}

	// Put pivot into seperate variable and remove from array.
	pivot = points[pivotIndex];
	points[pivotIndex] = points[amount - 1];
	amount--;

	// Calculate angle to pivot for each point in the array.
    for ( index = 0; index < amount; index++ ) {
 
		// point vector
		vecPoint.pos_screen[0] = pivot.pos_screen[0] - points[index].pos_screen[0];
		vecPoint.pos_screen[1] = pivot.pos_screen[1] - points[index].pos_screen[1];
    
		// reduce to a unit-vector - length 1
		angles[index] = vecPoint.pos_screen[0] / Q_hypot(vecPoint.pos_screen[0], vecPoint.pos_screen[1]);
	}

	// Sort the points by angle.
	CG_QSortAngle(points, angles, 0, amount - 1);

	// Step through array to remove points that are not part of the convex hull.
	index = 1;

	do {
		 // Assign points behind and in front of current point.
		if ( index == 0) {
			rightTurn = qtrue;
		} else {
			behind = points[index - 1];

			if ( index == (amount - 1)) {
				infront = pivot;
			} else {
				infront = points[index + 1];
			}

			// Work out if we are making a right or left turn using vector product.
			if ( ( (behind.pos_screen[0]  - points[index].pos_screen[0]) * (infront.pos_screen[1] - points[index].pos_screen[1]) -
				   (infront.pos_screen[0] - points[index].pos_screen[0]) * (behind.pos_screen[1] - points[index].pos_screen[1]) ) < 0 ) {
				rightTurn = qtrue;
			}else {
				rightTurn = qfalse;
			}
		}

		if ( rightTurn ) {
			// point is currently considered part of the hull
			index++;
		} else {
			// point is not part of the hull

			// remove point from convex hull
			if ( index == (amount - 1) ) {
				amount--;
			} else {
				// move everything after the current value one step forward
				memcpy( buffer, &points[index + 1], sizeof(auraTag_t) * (amount - index - 1) );
				memcpy( &points[index], buffer, sizeof(auraTag_t) * (amount - index - 1) );
				amount--;
			}

			// backtrack to previous point
			index--;
		}

	} while ( !(index == (amount - 1)) );
	
	// add pivot back into points array
	points[amount] = pivot;
	amount++;

	*nr_points = amount;
	return qtrue;
}

/*
=============================
CG_SetHullNormalsAndLengths
=============================
*/
static float CG_SetHullNormalsAndLengths( auraTag_t *points, int nr_points, float *tailpos ) {
	int		index, behind, infront;
	vec3_t	line_behind, line_infront, viewLine;
	vec3_t	temp_behind, temp_infront;
	float	circumference;

	circumference = 0.0f;
	// initialize negative, by which we'll see if there's a tail to travel to to begin with.
	*tailpos = -1.0f;
	for ( index = 0; index < nr_points; index++ ) {

		// Set successor and predeccessor
		if (index == 0) {
			behind = nr_points - 1;
			infront = index + 1;
		} else if (index == nr_points - 1) {
			behind = index - 1;
			infront = 0;
		} else {
			behind = index - 1;
			infront = index + 1;
		}

		VectorSubtract (points[index].pos_world, cg.refdef.vieworg, viewLine);

		// Calculate the normal
		VectorSubtract( points[index].pos_world, points[behind].pos_world,  line_behind  );
		VectorSubtract( points[infront].pos_world, points[index].pos_world, line_infront );

		VectorNormalize( line_behind  );
		circumference += (points[index].length = VectorNormalize( line_infront ));

		// save position of the tail
		if ( points[infront].is_tail ) {
			*tailpos = circumference;
		}

		CrossProduct( line_behind, viewLine, temp_behind );
		CrossProduct( line_infront, viewLine, temp_infront );
		if (!VectorNormalize ( temp_behind )) {
			viewLine[2] += 0.1f;
			CrossProduct( line_behind, viewLine, temp_behind );
			VectorNormalize ( temp_behind );
			viewLine[2] -= 0.1f;
		}
		
		if (!VectorNormalize ( temp_infront )) {
			viewLine[2] += 0.1f;
			CrossProduct( line_behind, viewLine, temp_behind );
			VectorNormalize ( temp_behind );
			viewLine[2] -= 0.1f;
		}
		
		VectorAdd( temp_behind, temp_infront, points[index].normal );
		VectorNormalize( points[index].normal );
	}

	return circumference;
}

/*
=============
CG_DrawSpike
=============
*/
static void CG_DrawSpike (vec3_t start, vec3_t end, float width, qhandle_t shader, vec4_t RGBModulate) {
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
			verts[i].modulate[j] = 255 * RGBModulate[j];
		}
	}

	trap_R_AddPolyToScene( shader, 4, verts);
}

/*
=========================
CG_ConvexHullAuraRender
=========================
*/
void CG_ConvexHullAuraRender ( auraSystem_t *auraSys ) {
	int	i, j, k;
	float length_pos, tail_pos, origin_pos, length_sofar;
	float progress, progress_pct, lifetime_pct, lifetime_pct_pingpong, RGBModulate;
	vec3_t lerpNormal, lerpPos, edge, endPos;
	vec4_t RGBColor;

	// Set up the tail hullpoint location.
	if ( VectorLength( auraSys->delta ) < 180 ) {
		VectorSet(auraSys->tailTip, 0, 0, 1);
	} else {
		VectorNormalize2( auraSys->delta, auraSys->tailTip );
		VectorInverse( auraSys->tailTip );
	}
	VectorScale( auraSys->tailTip, (AURA_TAILLENGTH * auraSys->scale) - AURASPIKE_SIZE, auraSys->tailTip );
	VectorAdd( auraSys->tailTip, auraSys->origin, auraSys->tailTip );

	VectorCopy( auraSys->tailTip, auraSys->hullPoints[auraSys->nr_hullPoints].pos_world );
	if (CG_WorldCoordToScreenCoordVec( auraSys->hullPoints[auraSys->nr_hullPoints].pos_world,
		auraSys->hullPoints[auraSys->nr_hullPoints].pos_screen ) ) {
		auraSys->hullPoints[auraSys->nr_hullPoints].is_tail = qtrue;
		auraSys->nr_hullPoints++;
	}

	// build the convex hull
	if ( !CG_FindConvexHull( auraSys->hullPoints, &auraSys->nr_hullPoints ) ) {
		// We couldn't build a hull.
		return;
	}

	auraSys->hullCircumference = CG_SetHullNormalsAndLengths( auraSys->hullPoints, auraSys->nr_hullPoints, &tail_pos );

	if ( cg.time > (auraSys->burstTimer + AURASPIKE_TIME) ) {
		auraSys->burstTimer = cg.time;
	}

	lifetime_pct = 1.0f - (float)( AURASPIKE_TIME - ( cg.time - auraSys->burstTimer ) ) / AURASPIKE_TIME;
	RGBModulate =  -4 * lifetime_pct * lifetime_pct + 4 * lifetime_pct;
	
	

	// first set
	for ( i = 0; i < NR_AURASPIKES; i++ ) {
		// map i onto the circumference of the convex hull
		if ( tail_pos < 0 ) {
			// If we don't have a tail, we do it the simple way
			length_pos = auraSys->hullCircumference * ( (float)i / (float)(NR_AURASPIKES - 1));
			length_pos += auraSys->hullCircumference * ( 1.0f / (float)(2.0f * (NR_AURASPIKES -1)) );
			while ( length_pos > auraSys->hullCircumference ) {
				length_pos -= auraSys->hullCircumference;
			}
			lifetime_pct_pingpong = lifetime_pct;
		} else {
			// otherwise we complicate matters a bit to let the spikes travel towards the tail
			lifetime_pct_pingpong = RGBModulate;
			origin_pos = tail_pos + 0.5f * auraSys->hullCircumference;
			while ( origin_pos > auraSys->hullCircumference ) {
				origin_pos -= auraSys->hullCircumference;
			}

			length_pos = auraSys->hullCircumference * ( (float)i / (float)(NR_AURASPIKES - 1));

			if ( tail_pos > origin_pos ) {
				// This means the break in the cycle is between tail-origin
				if ( (length_pos < origin_pos) || (length_pos > tail_pos) ) {
					length_pos -= auraSys->hullCircumference * (lifetime_pct / (float)(NR_AURASPIKES - 1));
				} else {
					length_pos += auraSys->hullCircumference * (lifetime_pct / (float)(NR_AURASPIKES - 1));
				}
			} else {
				// This means the break in the cycle is between origin-tail
				if ( (length_pos > origin_pos) || (length_pos < tail_pos) ) {
					length_pos += auraSys->hullCircumference * (lifetime_pct / (float)(NR_AURASPIKES - 1));
				} else {
					length_pos -= auraSys->hullCircumference * (lifetime_pct / (float)(NR_AURASPIKES - 1));
				}
			}
		}

		while ( length_pos > auraSys->hullCircumference ) {
			length_pos -= auraSys->hullCircumference;
		}
		while ( length_pos < 0 ) {
			length_pos += auraSys->hullCircumference;
		}

		// find the segment we are in right now
		length_sofar = 0;
		for ( j = 0; ((length_sofar + auraSys->hullPoints[j].length) < length_pos ) && (j < NR_AURASPIKES) ; j++ ) {
			length_sofar += auraSys->hullPoints[j].length;
		}
		k = j + 1;
		if ( k == auraSys->nr_hullPoints ) k = 0;

		// find how far we have progressed on that segment
		progress = (length_pos - length_sofar);
		progress_pct = progress / auraSys->hullPoints[j].length;

		// interpolate the normal and position
		VectorSet( lerpNormal, 0.0f, 0.0f, 0.0f);
		VectorMA( lerpNormal, 1.0f - progress_pct, auraSys->hullPoints[j].normal, lerpNormal );
		VectorMA( lerpNormal, progress_pct, auraSys->hullPoints[k].normal, lerpNormal );
		VectorNormalize ( lerpNormal );

		VectorSubtract( auraSys->hullPoints[k].pos_world, auraSys->hullPoints[j].pos_world, edge );
		VectorMA( auraSys->hullPoints[j].pos_world, progress_pct, edge, lerpPos );

		// do the drawing				
		VectorMA( lerpPos, AURASPIKE_SIZE * ( 1 + lifetime_pct_pingpong) + AURASPIKE_BORDER * (1 + 2 * lifetime_pct_pingpong), lerpNormal, endPos );
		VectorMA( lerpPos, AURASPIKE_BORDER * (1 + 2 * lifetime_pct_pingpong), lerpNormal, lerpPos );
				
		RGBColor[0] = auraSys->RGBColor[0] * RGBModulate;
		RGBColor[1] = auraSys->RGBColor[1] * RGBModulate;
		RGBColor[2] = auraSys->RGBColor[2] * RGBModulate;
		RGBColor[3] = 1.0f;

		CG_DrawSpike( lerpPos, endPos, AURASPIKE_SIZE / 1.25f * (1 + lifetime_pct_pingpong), auraSys->shader, RGBColor );
	}


	
	lifetime_pct = (1.0f - (float)( AURASPIKE_TIME - ( cg.time - auraSys->burstTimer ) ) / AURASPIKE_TIME);
	lifetime_pct += 0.5;
	while (lifetime_pct > 1.0f) {
		lifetime_pct -= 1.0f;
	}
	RGBModulate = -4 * lifetime_pct * lifetime_pct + 4 * lifetime_pct;
	lifetime_pct_pingpong = lifetime_pct;

	// second (smaller) set
	for ( i = 0; i < ceil(1.5f * NR_AURASPIKES); i++ ) {
		// map i onto the circumference of the convex hull
		length_pos = auraSys->hullCircumference * ( (float)i / (float)(ceil(1.5f * NR_AURASPIKES) - 1) );
		
		
		// find the segment we are in right now
		length_sofar = 0;
		for ( j = 0; ((length_sofar + auraSys->hullPoints[j].length) < length_pos ) && (j < NR_AURASPIKES) ; j++ ) {
			length_sofar += auraSys->hullPoints[j].length;
		}
		k = j + 1;
		if ( k == auraSys->nr_hullPoints ) k = 0;

		// find how far we have progressed on that segment
		progress = (length_pos - length_sofar);
		progress_pct = progress / auraSys->hullPoints[j].length;

		// interpolate the normal and position
		VectorSet( lerpNormal, 0.0f, 0.0f, 0.0f);
		VectorMA( lerpNormal, 1.0f - progress_pct, auraSys->hullPoints[j].normal, lerpNormal );
		VectorMA( lerpNormal, progress_pct, auraSys->hullPoints[k].normal, lerpNormal );
		VectorNormalize ( lerpNormal );

		VectorSubtract( auraSys->hullPoints[k].pos_world, auraSys->hullPoints[j].pos_world, edge );
		VectorMA( auraSys->hullPoints[j].pos_world, progress_pct, edge, lerpPos );

		// do the drawing				
		VectorMA( lerpPos, AURASPIKE_SIZE * ( 1 + lifetime_pct_pingpong) + 0.8f * AURASPIKE_BORDER * (1 + 2 * lifetime_pct_pingpong), lerpNormal, endPos );
		VectorMA( lerpPos, 0.8f * AURASPIKE_BORDER * (1 + 2 * lifetime_pct_pingpong), lerpNormal, lerpPos );
				
		RGBColor[0] = auraSys->RGBColor[0] * RGBModulate;
		RGBColor[1] = auraSys->RGBColor[1] * RGBModulate;
		RGBColor[2] = auraSys->RGBColor[2] * RGBModulate;
		RGBColor[3] = 1.0f;

		CG_DrawSpike( lerpPos, endPos, (AURASPIKE_SIZE / 1.25f) * (1 + lifetime_pct_pingpong), auraSys->shader, RGBColor );
	}
}


/*
====================
CG_FlameAuraRender
====================
*/
void CG_FlameAuraRender ( auraSystem_t *auraSys, float RGBModulate, float scaleMod ) {
	vec3_t			tailLine, viewLine, lobeLine, offset1, offset2;
	
	float			len;
	int				i, j;

	polyVert_t		Tail_verts[4];
	polyVert_t		LeftLobe_verts[4];
	polyVert_t		RightLobe_verts[4];

	
	// Set up the tailTip location.
	if ( VectorLength( auraSys->delta ) < 180 ) {
		VectorSet(auraSys->tailTip, 0, 0, 1);
	} else {
		VectorNormalize2( auraSys->delta, auraSys->tailTip );
		VectorInverse( auraSys->tailTip );
	}
	VectorScale( auraSys->tailTip, AURA_TAILLENGTH * auraSys->scale * scaleMod, auraSys->tailTip );
	VectorAdd( auraSys->tailTip, auraSys->origin, auraSys->tailTip );


	// -( Left and right tips )-
	
	// Set up viewlines for the tail, crossproduct to determine coordinates
	VectorSubtract ( auraSys->tailTip, auraSys->origin,   tailLine );
	VectorSubtract ( auraSys->origin,  cg.refdef.vieworg, viewLine );
	VectorNormalize( tailLine );
	VectorNormalize( viewLine );

	CrossProduct (viewLine, tailLine, offset1);
	len = VectorNormalize (offset1);

	// If crossproduct has too little length, then the tail is too close to being parallel
	// to the view direction and it won't be displayed.
	if ( len < AURA_BREAKANGLE ) {

		// The tail isn't visible so we'll only display the two lobes,
		// which will appear perpendicular to the view vector.

		// Normalize the vectors if for some god-awful reason they were NOT originally.
		AngleVectors( cg.refdefViewAngles, NULL, offset1, offset2 );
		VectorNormalize( offset1 );
		VectorNormalize( offset2 );

		VectorMA( auraSys->origin,       -AURA_WIDTH * auraSys->scale * scaleMod, offset1, LeftLobe_verts[0].xyz );
		VectorMA( LeftLobe_verts[0].xyz, -AURA_WIDTH * auraSys->scale * scaleMod, offset2, LeftLobe_verts[0].xyz );
		LeftLobe_verts[0].st[0] = 0;
		LeftLobe_verts[0].st[1] = 1;
		VectorMA( auraSys->origin,       -AURA_WIDTH * auraSys->scale * scaleMod, offset1, LeftLobe_verts[1].xyz );
		LeftLobe_verts[1].st[0] = 0;
		LeftLobe_verts[1].st[1] = AURA_TEXTUREBREAK;
		VectorMA (auraSys->origin,        AURA_WIDTH * auraSys->scale * scaleMod, offset1, LeftLobe_verts[2].xyz );
		LeftLobe_verts[2].st[0] = 1;
		LeftLobe_verts[2].st[1] = AURA_TEXTUREBREAK;
		VectorMA (auraSys->origin,        AURA_WIDTH * auraSys->scale * scaleMod, offset1, LeftLobe_verts[3].xyz );
		VectorMA (LeftLobe_verts[3].xyz, -AURA_WIDTH * auraSys->scale * scaleMod, offset2, LeftLobe_verts[3].xyz );
		LeftLobe_verts[3].st[0] = 1;
		LeftLobe_verts[3].st[1] = 1;
		
		// Set vertex modulation
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (j==4) {
					LeftLobe_verts[i].modulate[j] = 255 * RGBModulate;
					continue;
				}
				LeftLobe_verts[i].modulate[j] = 255 * auraSys->RGBColor[j] * RGBModulate;
			}
		}

		VectorMA( auraSys->origin,        -AURA_WIDTH * auraSys->scale * scaleMod, offset1, RightLobe_verts[0].xyz );
		RightLobe_verts[0].st[0] = 0;
		RightLobe_verts[0].st[1] = AURA_TEXTUREBREAK;
		VectorMA( auraSys->origin,        -AURA_WIDTH * auraSys->scale * scaleMod, offset1, RightLobe_verts[1].xyz );
		VectorMA( RightLobe_verts[1].xyz,  AURA_WIDTH * auraSys->scale * scaleMod, offset2, RightLobe_verts[1].xyz );
		RightLobe_verts[1].st[0] = 0;
		RightLobe_verts[1].st[1] = 1;
		VectorMA (auraSys->origin,         AURA_WIDTH * auraSys->scale * scaleMod, offset1, RightLobe_verts[2].xyz );
		VectorMA (RightLobe_verts[2].xyz,  AURA_WIDTH * auraSys->scale * scaleMod, offset2, RightLobe_verts[2].xyz );
		RightLobe_verts[2].st[0] = 1;
		RightLobe_verts[2].st[1] = 1;
		VectorMA (auraSys->origin,         AURA_WIDTH * auraSys->scale * scaleMod, offset1, RightLobe_verts[3].xyz );
		RightLobe_verts[3].st[0] = 1;
		RightLobe_verts[3].st[1] = AURA_TEXTUREBREAK;

		// Set vertex modulation
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (j==4) {
					RightLobe_verts[i].modulate[j] = 255 * RGBModulate;
					continue;
				}
				RightLobe_verts[i].modulate[j] = 255 * auraSys->RGBColor[j] * RGBModulate;
			}
		}


		// Add the two pieces of the aura to the scene
		trap_R_AddPolyToScene( auraSys->shader, 4, LeftLobe_verts  );
		trap_R_AddPolyToScene( auraSys->shader, 4, RightLobe_verts );

	} else {
		// Take another cross-product between viewLine and previous offset line.
		// Retrieves lobeLine, extending out from tail's 'face'.
		CrossProduct( viewLine, offset1, lobeLine );
		VectorNormalize( lobeLine );
		VectorMA (auraSys->origin,  AURA_WIDTH * auraSys->scale * scaleMod, lobeLine, auraSys->leftLobeTip);
		VectorMA (auraSys->origin, -AURA_WIDTH * auraSys->scale * scaleMod, lobeLine, auraSys->rightLobeTip);

		// Set tail's vertex data
		// NOTE: offset is still the offset we calculated earlier, so this holds!
		VectorMA (auraSys->tailTip, -AURA_WIDTH * auraSys->scale * scaleMod, offset1, Tail_verts[0].xyz);
		Tail_verts[0].st[0] = 1;
		Tail_verts[0].st[1] = 0;
		VectorMA (auraSys->tailTip,  AURA_WIDTH * auraSys->scale * scaleMod, offset1, Tail_verts[1].xyz);
		Tail_verts[1].st[0] = 0;
		Tail_verts[1].st[1] = 0;
		VectorMA (auraSys->origin,   AURA_WIDTH * auraSys->scale * scaleMod, offset1, Tail_verts[2].xyz);
		Tail_verts[2].st[0] = 0;
		Tail_verts[2].st[1] = AURA_TEXTUREBREAK;
		VectorMA (auraSys->origin,  -AURA_WIDTH * auraSys->scale * scaleMod, offset1, Tail_verts[3].xyz);
		Tail_verts[3].st[0] = 1;
		Tail_verts[3].st[1] = AURA_TEXTUREBREAK;
	
		// Set vertex modulation
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (j==4) {
					Tail_verts[i].modulate[j] = 255 * RGBModulate;
					continue;
				}
				Tail_verts[i].modulate[j] = 255 * auraSys->RGBColor[j] * RGBModulate;
			}
		}

		// Calculate the new offset setting for the lobeLine now.
		CrossProduct (viewLine, lobeLine, offset2);
		VectorNormalize (offset2);

		// Set Left lobe's vertex data
		// NOTE: offset is still the offset we calculated earlier, so this holds!
		VectorMA (auraSys->leftLobeTip, -AURA_WIDTH * auraSys->scale * scaleMod, offset2, LeftLobe_verts[0].xyz);
		LeftLobe_verts[0].st[0] = 1;
		LeftLobe_verts[0].st[1] = 1;
		VectorMA (auraSys->leftLobeTip,  AURA_WIDTH * auraSys->scale * scaleMod, offset2, LeftLobe_verts[1].xyz);
		LeftLobe_verts[1].st[0] = 0;
		LeftLobe_verts[1].st[1] = 1;
		VectorMA (auraSys->origin,       AURA_WIDTH * auraSys->scale * scaleMod, offset2, LeftLobe_verts[2].xyz);
		LeftLobe_verts[2].st[0] = 0;
		LeftLobe_verts[2].st[1] = AURA_TEXTUREBREAK;
		VectorMA (auraSys->origin,      -AURA_WIDTH * auraSys->scale * scaleMod, offset2, LeftLobe_verts[3].xyz);
		LeftLobe_verts[3].st[0] = 1;
		LeftLobe_verts[3].st[1] = AURA_TEXTUREBREAK;

		// Set vertex modulation
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				if (j==4) {
					LeftLobe_verts[i].modulate[j] = 255 * RGBModulate;
					continue;
				}
				LeftLobe_verts[i].modulate[j] = 255 * auraSys->RGBColor[j] * RGBModulate;
			}
		}

		// Add the two pieces of the aura to the scene
		trap_R_AddPolyToScene( auraSys->shader, 4, Tail_verts );
		trap_R_AddPolyToScene( auraSys->shader, 4, LeftLobe_verts );

	} 	
}



/*
===============
CG_AuraRender
===============
*/
void CG_AuraRender( centity_t *cent ) {
	auraSystem_t	*auraSys;
	int				clientNum;

	float			RGBModulate;
	float			scaleMod;

	// Get the aura system corresponding to the player
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
		return;
	}
	auraSys = &playerAura[ clientNum ];



	// Update some general values.
	VectorCopy( cent->lerpOrigin, auraSys->origin );
	VectorCopy( cent->currentState.pos.trDelta, auraSys->delta );
	RGBModulate = (float)( AURA_BURSTTIME - ( cg.time - auraSys->burstTimer ) ) / AURA_BURSTTIME;
	if ( RGBModulate < 0 ) {
		RGBModulate = 0;
	}
	if ( RGBModulate > 1.0f ) {
		RGBModulate = 1.0f;
	}
	scaleMod = 1.2f + (1 - RGBModulate) * 0.45f;

	switch ( auraSys->type ) {
	case AURA_FLAME:
		// If the aura is not active don't draw the aura, only the burst.
		if ( auraSys->active ) {
			CG_FlameAuraRender ( auraSys, 1.0f, 1.0f );
		}
		// If there's still some color left in the burst, draw it.
		if ( RGBModulate > 0 ) {
			CG_FlameAuraRender ( auraSys, RGBModulate, scaleMod );
		}
		break;
	case AURA_CONVEXHULL:
		// FIXME: Always turned on for now, for testing
		if ( auraSys->active ) {
			CG_ConvexHullAuraRender( auraSys );
		}
		break;
	default:
		// shouldn't happen
		break;
	}

	//  if aura is active
	if ( auraSys->active ) {
		vec3_t groundPoint;
		trace_t trace;

		// Add looping sound
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, auraSys->loopSound );

		// Add dynamic light
		trap_R_AddLightToScene( cent->lerpOrigin, 200, auraSys->RGBColor[0], auraSys->RGBColor[1], auraSys->RGBColor[2] );

		// Handle the drifting debris cloud
		VectorCopy( cent->lerpOrigin, groundPoint );
		groundPoint[2] -= AURA_RUBBLE_DIST;
		CG_Trace( &trace, cent->lerpOrigin, NULL, NULL, groundPoint, cent->currentState.number, CONTENTS_SOLID );
		if ( trace.allsolid || trace.startsolid ) {
			trace.fraction = 1.0f;
		}
		if ( trace.fraction < 1.0f ) {
			// Place the origin of the system just a bit off the surface
			VectorNormalize2(trace.plane.normal, groundPoint );
			VectorMA( trace.endpos, 5, groundPoint, cent->groundPoint );
			if ( !cent->particleSystems[4] ) {
				cent->particleSystems[4] = CG_CreateParticleSystem_DriftDebris( cent->currentState.number );
			}
		} else {
			// too far away, stop rubble spawn and rejuvenation,
			// and break link if system is active
			if ( cent->particleSystems[4] ) {
				cent->particleSystems[4]->respawn = qfalse;
				cent->particleSystems[4]->rejuvenate = qfalse;
				VectorSet( cent->particleSystems[4]->force, 0, 0, 0);
				VectorSet( cent->particleSystems[4]->force_turbulence, 0, 0, 0);
				cent->particleSystems[4]->parentNr = ENTITYNUM_NONE;
				cent->particleSystems[4] = 0;
			}
		}

		// Update the tail if we're using the boost aura
		if ( cent->currentState.powerups & ( 1 << PW_BOOST ) ) {
		
			// If we didn't draw the tail last frame this is a new instantiation
			// of the entity and we will have to reset the tail positions.
			if ( cent->lastTrailTime < (cg.time - cg.frametime - 1500) ) {
				// Give 1.5 second leeway for 'snapping' the tail
				// incase we (almost) immediately restart boosting.

				CG_ResetTrail( cent->currentState.clientNum, cent->lerpOrigin, 1000,
					10, auraSys->trailShader, auraSys->RGBColor );
			}
	
			CG_UpdateTrailHead( cent->currentState.clientNum, cent->lerpOrigin );

			cent->lastTrailTime = cg.time;
		}
	}
	
	return;
}


/*
==============
CG_AuraStart
==============
*/
void CG_AuraStart( centity_t *cent ) {
	int				clientNum;
	auraSystem_t	*auraSys;
	vec3_t			groundPoint;
	trace_t			trace;


	// Get the aura system to work on
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	auraSys = &playerAura[ clientNum ];

	// Initialize the system
	auraSys->active = qtrue;
	auraSys->burstTimer = cg.time;

	// play the burst sound
	trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY, auraSys->startSound );

	// Check if we're on, or near ground level
	VectorCopy( cent->lerpOrigin, groundPoint );
	groundPoint[2] -= AURA_RUBBLE_DIST;
	CG_Trace( &trace, cent->lerpOrigin, NULL, NULL, groundPoint, cent->currentState.number, CONTENTS_SOLID );
	if ( trace.allsolid || trace.startsolid ) {
		trace.fraction = 1.0f;
	}
	if ( trace.fraction < 1.0f ) {
		// Place the explosion just a bit off the surface
		VectorNormalize2(trace.plane.normal, groundPoint );
		VectorMA( trace.endpos, 5, groundPoint, groundPoint );
		CG_CreateParticleSystem_ExplodeDebris( groundPoint, trace.plane.normal, 14 );
	}
}


/*
============
CG_AuraEnd
============
*/
void CG_AuraEnd( centity_t *cent ) {
	int		clientNum;

	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}

	playerAura[ clientNum ].active = qfalse;


	if ( cent->particleSystems[4] ) {
		cent->particleSystems[4]->respawn = qfalse;
		cent->particleSystems[4]->rejuvenate = qfalse;
		cent->particleSystems[4]->maxForceDistance = 0;
		VectorSet( cent->particleSystems[4]->force, 0, 0, 0);
		VectorSet( cent->particleSystems[4]->force_turbulence, 0, 0, 0);
		cent->particleSystems[4]->parentNr = ENTITYNUM_NONE;
		cent->particleSystems[4] = 0;
	}
}





/*
===================
CG_StoreAuraVerts
===================
  Reads and prepares the positions of the tags for a convex hull aura.
*/
#define MAX_AURATAGNAME 10
void CG_StoreAuraVerts( refEntity_t *head, refEntity_t *torso, refEntity_t *legs, int clientNum ) {
	int				i, j;
	clientInfo_t	*ci;
	auraSystem_t	*auraSys;
	char			tagName[MAX_AURATAGNAME];

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}

	ci = &cgs.clientinfo[clientNum];
	auraSys = &playerAura[clientNum];
	
	j = 0;
	// aura_tags in head.md3
	for (i = 0; i < ci->nrAuraTags[AURATAGS_HEAD]; i++ ) {
		
		// Lerp the tag's position
		Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i );
		CG_GetTagPosition( head, tagName, auraSys->hullPoints[j].pos_world);
		if (CG_WorldCoordToScreenCoordVec( auraSys->hullPoints[j].pos_world,
									   auraSys->hullPoints[j].pos_screen ) ) {
			auraSys->hullPoints[j].is_tail = qfalse;
			j++;
		}
	}

	// aura_tags in upper.md3
	for (i = 0; i < ci->nrAuraTags[AURATAGS_TORSO]; i++ ) {
		
		Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i );
		CG_GetTagPosition( torso, tagName, auraSys->hullPoints[j].pos_world);
		if (CG_WorldCoordToScreenCoordVec( auraSys->hullPoints[j].pos_world,
									   auraSys->hullPoints[j].pos_screen ) ) {
			auraSys->hullPoints[j].is_tail = qfalse;
			j++;
		}
	}

	// aura_tags in lower.md3
	for (i = 0; i < ci->nrAuraTags[AURATAGS_LEGS]; i++ ) {
		
		Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i );
		CG_GetTagPosition( legs, tagName, auraSys->hullPoints[j].pos_world);
		if (CG_WorldCoordToScreenCoordVec( auraSys->hullPoints[j].pos_world,
									   auraSys->hullPoints[j].pos_screen ) ) {
			auraSys->hullPoints[j].is_tail = qfalse;
			j++;
		}
	}

	// Get the total number of possible vertices to account for in the hull
	auraSys->nr_hullPoints = j;
}



/*
==============
CG_InitAuras
==============
*/
void CG_InitAuras() {
	int i;
	
	// The structure is built so we intialize by only memsetting everything
	// to null.
	memset(playerAura, 0, sizeof(auraSystem_t) * MAX_CLIENTS);

	for ( i = 0; i < MAX_CLIENTS; i++ ) {

		playerAura[i].type = AURA_CONVEXHULL;
		
		playerAura[i].shader = trap_R_RegisterShader( "Aura_Spike" );
		playerAura[i].trailShader = trap_R_RegisterShader( "Aura_Trail" );

		VectorSet( playerAura[i].RGBColor, 1.0f, 1.0f, 1.0f );

		playerAura[i].burstTimer = cg.time - (AURA_BURSTTIME + 50); // <-- Makes sure the burst isn't active
		playerAura[i].scale = 1.5f;

		playerAura[i].startSound = trap_S_RegisterSound( "sound/aura/AuraStart.wav", qfalse );
		playerAura[i].loopSound  = trap_S_RegisterSound( "sound/aura/AuraLoop.wav",  qfalse );
	}
}
