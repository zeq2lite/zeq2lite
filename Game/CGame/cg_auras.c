// Copyright (C) 2003-2004 RiO
//
// cg_auras.c -- generates and displays auras
//

#include "cg_local.h"

static auraState_t	auraStates[MAX_CLIENTS];

// =================================================
//
//    S E T T I N G   U P   C O N V E X   H U L L
//
// =================================================

/*
========================
CG_Aura_BuildTailPoint
========================
  Builds position of the aura's tail point, dependant on velocity vector of player entity
*/
static void CG_Aura_BuildTailPoint(centity_t *player, auraState_t *state, auraConfig_t *config){
	vec3_t			tailDir;

	// Find the direction the tail should point. Low speeds will have it pointing up.
	if(VectorNormalize2( player->currentState.pos.trDelta, tailDir) < 180)
		VectorSet( tailDir, 0, 0, 1);
	else
		VectorInverse( tailDir);

	// Using the established direction, set up the tail's potential hull point.
	VectorMA( state->origin, config->auraScale * config->tailLength, tailDir, state->tailPos);
}


/*
=======================
CG_Aura_MarkRootPoint
=======================
  Marks a point as the aura's root point. The root point is the point that is placed
  furthest away from the tail. It is where the aura 'opens up'.
*/
void CG_Aura_MarkRootPoint( auraState_t *state){
	vec3_t	prjPoint;
	float	maxDist, newDist;
	int		i;

    // NOTE:
	// The root point is the point that is placed furthest down the aura.
	// This means the distance between the tail point and the root point's projection
	// onto the tail axis is maximal.

	maxDist = 0;
	for(i = 0;i < state->convexHullCount;i++){
		// Project the point onto the tail axis
		ProjectPointOnLine( state->convexHull[i].pos_world, state->origin, state->tailPos, prjPoint);

		// Measure the distance and copy over the projection of the point
		// as the new root if it's further away.
		newDist = Distance( prjPoint, state->tailPos);
		if(newDist > maxDist){
			maxDist = newDist;
			VectorCopy( prjPoint, state->rootPos);
		}
	}

	// HACK:
	// Nudge the root away a bit to account for the expansion
	// of the aura through the hull normals.
	{
		vec3_t tailDir;

		VectorSubtract( state->tailPos, state->origin, tailDir);
		VectorNormalize( tailDir);
		VectorMA( state->rootPos, -4.8f, tailDir, state->rootPos);
	}
}


/*
=======================
CG_Aura_GetHullPoints
=======================
  Reads and prepares the positions of the tags for a convex hull aura.
*/
#define MAX_AURATAGNAME 12
static void CG_Aura_GetHullPoints( centity_t *player, auraState_t *state, auraConfig_t *config){
	char		tagName[MAX_AURATAGNAME];
	int			i, j;
	clientInfo_t	*ci;
	ci = &cgs.clientinfo[player->currentState.clientNum];
	j = 0;
	
	if(ci->usingMD4)
	{
		
			//Com_Printf("Finding tags!\n");
		for (i = 0;i < config->numTags[0]+config->numTags[1]+config->numTags[2];i++){
			orientation_t tagOrient;
			
			// Lerp the tag's position
			Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i);
			
			if (!CG_GetTagOrientationFromPlayerEntityLegsModel( player, tagName, &tagOrient)) continue;
			VectorCopy( tagOrient.origin, state->convexHull[j].pos_world);
			//Com_Printf("Found aura tag bone - %s\n",tagName);
			if (CG_WorldCoordToScreenCoordVec( state->convexHull[j].pos_world, state->convexHull[j].pos_screen)){
				state->convexHull[j].is_tail = qfalse;
				j++;
			}
		}
	}
	else
	{
		for (i = 0;i < config->numTags[0];i++){
			orientation_t tagOrient;
			
			// Lerp the tag's position
			Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i);
			
			if (!CG_GetTagOrientationFromPlayerEntityHeadModel( player, tagName, &tagOrient)) continue;
			VectorCopy( tagOrient.origin, state->convexHull[j].pos_world);

			if (CG_WorldCoordToScreenCoordVec( state->convexHull[j].pos_world, state->convexHull[j].pos_screen)){
				state->convexHull[j].is_tail = qfalse;
				j++;
			}
		}

		for (i = 0;i < config->numTags[1];i++){
			orientation_t tagOrient;
			
			// Lerp the tag's position
			Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i);
			
			if(!CG_GetTagOrientationFromPlayerEntityTorsoModel( player, tagName, &tagOrient)) continue;
			VectorCopy( tagOrient.origin, state->convexHull[j].pos_world);

			if (CG_WorldCoordToScreenCoordVec( state->convexHull[j].pos_world, state->convexHull[j].pos_screen)){
				state->convexHull[j].is_tail = qfalse;
				j++;
			}
		}

		for (i = 0;i < config->numTags[2];i++){
			orientation_t tagOrient;
			
			// Lerp the tag's position
			Com_sprintf( tagName, sizeof(tagName), "tag_aura%i", i);
			
			if(!CG_GetTagOrientationFromPlayerEntityLegsModel( player, tagName, &tagOrient)) continue;
			VectorCopy( tagOrient.origin, state->convexHull[j].pos_world);

			if (CG_WorldCoordToScreenCoordVec( state->convexHull[j].pos_world, state->convexHull[j].pos_screen)){
				state->convexHull[j].is_tail = qfalse;
				j++;
			}
		}
	}
	// Find the aura's tail point
	CG_Aura_BuildTailPoint( player, state, config);

	// Add the tail tip to the hull points, if it's visible
	VectorCopy( state->tailPos, state->convexHull[j].pos_world);
	if (CG_WorldCoordToScreenCoordVec( state->convexHull[j].pos_world, state->convexHull[j].pos_screen)){
		state->convexHull[j].is_tail = qtrue;
		j++;
	}

	// Get the total number of possible vertices to account for in the hull
	state->convexHullCount = j;

	// Mark the root point
	CG_Aura_MarkRootPoint( state);
}


/*
====================
CG_Aura_QSortAngle
====================
  Quicksort by ascending angles
*/
static void CG_Aura_QSortAngle( auraTag_t *points, float* angles, int lowbound, int highbound){
	int			low, high;
	float		mid;
	auraTag_t	tempPoint;
	float		tempAngle;

	low = lowbound;
	high = highbound;
	mid = angles[(low + high) / 2];

	do{
		while(angles[low]  < mid) low++;
		while(angles[high] > mid) high--;

		if(low <= high){
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

	} while(!(low > high));

	if(high > lowbound) CG_Aura_QSortAngle( points, angles, lowbound, high);
	if(low < highbound) CG_Aura_QSortAngle( points, angles, low, highbound);	
}


/*
===========================
CG_Aura_ArrangeConvexHull
===========================
  Rearranges *points to contain its convex hull in the first *nr_points points.
*/
static qboolean CG_Aura_ArrangeConvexHull( auraTag_t *points, int *nr_points){
	float		angles[MAX_AURATAGS + 1];// +1 for tail
	int			amount, index, pivotIndex;
	auraTag_t	pivot;
	auraTag_t	behind, infront;
	qboolean	rightTurn;
	auraTag_t	vecPoint;
	auraTag_t	buffer[MAX_AURATAGS + 1];
	

	amount = *nr_points;	

	if(amount == 3) return qtrue;// Already a convex hull
	if(amount <  3){
		return qfalse;
	}

	pivotIndex = 0;
	// Find pivot point, which is known to be on the hull.
	// Point with lowest y - if there are multiple, point with highest x.
	for(index = 1;index < amount;index++){

		if(points[index].pos_screen[1] < points[pivotIndex].pos_screen[1]){
			pivotIndex = index;

		} else if(points[index].pos_screen[1] == points[pivotIndex].pos_screen[1]){
			
			if (points[index].pos_screen[0] > points[pivotIndex].pos_screen[0]){
				pivotIndex = index;
			}
		}
	}

	// Put pivot into seperate variable and remove from array.
	pivot = points[pivotIndex];
	points[pivotIndex] = points[amount - 1];
	amount--;

	// Calculate angle to pivot for each point in the array.
    for(index = 0;index < amount;index++){
 
		// point vector
		vecPoint.pos_screen[0] = pivot.pos_screen[0] - points[index].pos_screen[0];
		vecPoint.pos_screen[1] = pivot.pos_screen[1] - points[index].pos_screen[1];
    
		// reduce to a unit-vector - length 1
		angles[index] = vecPoint.pos_screen[0] / Q_hypot(vecPoint.pos_screen[0], vecPoint.pos_screen[1]);
	}

	// Sort the points by angle.
	CG_Aura_QSortAngle(points, angles, 0, amount - 1);

	// Step through array to remove points that are not p of the convex hull.
	index = 1;

	do{
		 // Assign points behind and in front of current point.
		if(index == 0){
			rightTurn = qtrue;
		} else{
			behind = points[index - 1];

			if(index == (amount - 1)){
				infront = pivot;
			} else{
				infront = points[index + 1];
			}

			// Work out if we are making a right or left turn using vector product.
			if(( (behind.pos_screen[0]  - points[index].pos_screen[0]) * (infront.pos_screen[1] - points[index].pos_screen[1]) -
				   (infront.pos_screen[0] - points[index].pos_screen[0]) * (behind.pos_screen[1] - points[index].pos_screen[1])) < 0){
				rightTurn = qtrue;
			}else{
				rightTurn = qfalse;
			}
		}

		if(rightTurn){
			// point is currently considered part of the hull
			index++;
		} else{
			// point is not part of the hull

			// remove point from convex hull
			if(index == (amount - 1)){
				amount--;
			} else{
				// move everything after the current value one step forward
				memcpy( buffer, &points[index + 1], sizeof(auraTag_t) * (amount - index - 1));
				memcpy( &points[index], buffer, sizeof(auraTag_t) * (amount - index - 1));
				amount--;
			}

			// backtrack to previous point
			index--;
		}

	} while(!(index == (amount - 1)));
	
	// add pivot back into points array
	points[amount] = pivot;
	amount++;

	*nr_points = amount;
	return qtrue;
}


/*
===========================
CG_Aura_SetHullAttributes
===========================
  Set segment lengths, normals, circumference, etc.
*/
static void CG_Aura_SetHullAttributes( auraState_t *state){
	int		index, behind, infront;
	vec3_t	line_behind, line_infront, viewLine;
	vec3_t	temp_behind, temp_infront;
	float	circumference;

	auraTag_t *points;
	int nr_points;

	points = state->convexHull;
	nr_points = state->convexHullCount;

	circumference = 0.0f;

	for(index = 0;index < nr_points;index++){
		
		// Set successor and predeccessor
		if(index == 0){
			behind = nr_points - 1;
			infront = index + 1;
		} else if(index == nr_points - 1){
			behind = index - 1;
			infront = 0;
		} else{
			behind = index - 1;
			infront = index + 1;
		}

		VectorSubtract( points[index].pos_world, cg.refdef.vieworg, viewLine);

		// Calculate the normal
		VectorSubtract( points[index].pos_world, points[behind].pos_world,  line_behind );
		VectorSubtract( points[infront].pos_world, points[index].pos_world, line_infront);

		VectorNormalize( line_behind );
		circumference += (points[index].length = VectorNormalize( line_infront));

		CrossProduct( line_behind, viewLine, temp_behind);
		CrossProduct( line_infront, viewLine, temp_infront);
		if(!VectorNormalize(temp_behind)){
			viewLine[2] += 0.1f;
			CrossProduct( line_behind, viewLine, temp_behind);
			VectorNormalize(temp_behind);
			viewLine[2] -= 0.1f;
		}
		
		if(!VectorNormalize(temp_infront)){
			viewLine[2] += 0.1f;
			CrossProduct( line_behind, viewLine, temp_behind);
			VectorNormalize(temp_behind);
			viewLine[2] -= 0.1f;
		}
		
		VectorAdd( temp_behind, temp_infront, points[index].normal);
		VectorNormalize( points[index].normal);
	}

	state->convexHullCircumference = circumference;
}


/*
=========================
CG_Aura_BuildConvexHull
=========================
  Calls all relevant functions to build up the aura's convex hull.
  Returns false if no hull can be made.
*/
static qboolean CG_Aura_BuildConvexHull( centity_t *player, auraState_t *state, auraConfig_t *config){

	// Retrieve hull points
	CG_Aura_GetHullPoints( player, state, config);

	// Arrange hull. Don't continue if there aren't enough points to form a hull.
	if(!CG_Aura_ArrangeConvexHull( state->convexHull, &state->convexHullCount)){
		return qfalse;
	}

	// Set hull's attributes
	CG_Aura_SetHullAttributes( state);

	// Hull building completed succesfully
	return qtrue;
}




// =================================
//
//    A U R A   R E N D E R I N G
//
// =================================

// Some constants needed for rendering.
// FIXME: Should become dynamic, but NR_AURASPIKES may pose a problem with allocation of poly buffer.
#define NR_AURASPIKES           50
#define AURA_FLATTEN_NORMAL		0.75f
#define AURA_ROOTCUTOFF_FRAQ	0.80f
#define AURA_ROOTCUTOFF_DIST	7.00f

/*
===================
CG_Aura_DrawSpike
===================
  Draws the polygons for one aura spike
*/
static void CG_Aura_DrawSpike (vec3_t start, vec3_t end, float width, qhandle_t shader, vec4_t RGBModulate){
	vec3_t line, offset, viewLine;
	polyVert_t verts[4];
	float len;
	int i, j;
	
	VectorSubtract (end, start, line);
	VectorSubtract (start, cg.refdef.vieworg, viewLine);
	CrossProduct (viewLine, line, offset);
	len = VectorNormalize (offset);
	
	if (!len){
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
	
	for (i = 0;i < 4;i++){
		for (j = 0;j < 4;j++){
			verts[i].modulate[j] = 255 * RGBModulate[j];
		}
	}

	trap_R_AddPolyToScene( shader, 4, verts);
}


/*
==========================
CG_Aura_LerpSpikeSegment
==========================
  Lerps the position the aura spike should have along a segment of the convex hull
*/
static void CG_Aura_LerpSpikeSegment( auraState_t *state, int spikeNr, int *start, int *end, float *progress_pct){
	float length_pos, length_sofar;
	int i, j;

	// Map i onto the circumference of the convex hull.
	length_pos = state->convexHullCircumference *((float)spikeNr / (float)(NR_AURASPIKES - 1));
				
	// Find the segment we are in right now.
	length_sofar = 0;
	for(i = 0;(( length_sofar + state->convexHull[i].length) < length_pos) &&(i < NR_AURASPIKES);i++){
		length_sofar += state->convexHull[i].length;
	}
	j = i + 1;
	if(j == state->convexHullCount){
		j = 0;
	}

	// Return found values.
	*start = i;
	*end = j;
	*progress_pct = (length_pos - length_sofar) / state->convexHull[i].length;
}


/*
==============
CG_LerpSpike
==============
  Lerps one spike in the aura
*/
static void CG_LerpSpike( auraState_t *state, auraConfig_t *config, int spikeNr, float alphaModulate){
	int start, end;
	float progress_pct;
	vec3_t viewLine;

	
	vec3_t lerpPos, lerpNormal, lerpDir, endPos, basePos;
	float lerpTime, lerpModulate, lerpBorder, baseBorder, lerpSize;
	vec4_t lerpColor;

	vec3_t edge, tailDir, tempVec;

	// Decide on which type of spike to use.
	if(!( spikeNr % 3)){
		lerpTime =(cg.time % 400) / 400.0f;
		lerpSize = 14.0f * (1 + lerpTime);
		lerpBorder = 2.5f * (1.75f + lerpTime);
		baseBorder = 2.5f * 1.75f;

	} else if(!( spikeNr % 2)){
		lerpTime = (( cg.time + 200) % 400) / 400.0f;
		lerpSize = 12.0f * (1 + lerpTime);
		lerpBorder = 3.0f * (1.75f + lerpTime);
		baseBorder = 3.0f * 1.75f;

	} else{
		lerpTime =(cg.time % 500) / 500.0f;
		lerpSize = 10.0f * (1 + lerpTime);
		lerpBorder = 2.75f * (1.75f + lerpTime);
		baseBorder = 2.75f * 1.75f;
	}
	lerpModulate = -4 * lerpTime * lerpTime + 4 * lerpTime;

	// NOTE: Prepared for a cvar switch between additive and
	//       blended aura.
	if(0){
		lerpColor[0] = config->auraColor[0] * lerpModulate * alphaModulate;
		lerpColor[1] = config->auraColor[1] * lerpModulate * alphaModulate;
		lerpColor[2] = config->auraColor[2] * lerpModulate * alphaModulate;
		lerpColor[3] = 1.0f;
	} else{
		lerpColor[0] = config->auraColor[0];
		lerpColor[1] = config->auraColor[1];
		lerpColor[2] = config->auraColor[2];
		lerpColor[3] = lerpModulate * alphaModulate;
	}

	// Get our position in the hull
	CG_Aura_LerpSpikeSegment( state, spikeNr, &start, &end, &progress_pct);

	// Lerp the position using the stored normal to expand the aura a bit
	VectorSet( lerpNormal, 0.0f, 0.0f, 0.0f);
	VectorMA( lerpNormal, 1.0f - progress_pct, state->convexHull[start].normal, lerpNormal);
	VectorMA( lerpNormal, progress_pct, state->convexHull[end].normal, lerpNormal);
	VectorNormalize(lerpNormal);

	VectorSubtract( state->convexHull[end].pos_world, state->convexHull[start].pos_world, edge);
	VectorMA( state->convexHull[start].pos_world, progress_pct, edge, lerpPos);
	VectorMA( lerpPos, baseBorder, lerpNormal, basePos);
	VectorMA( lerpPos, lerpBorder, lerpNormal, lerpPos);
	

	// Create the direction
	VectorSubtract( lerpPos, state->rootPos, lerpDir);
		
	// Flatten the direction a bit so it doesn't point to the tip too drasticly.
	VectorSubtract( state->tailPos, state->origin, tailDir);
	VectorPllComponent( lerpDir, tailDir, edge);
	VectorScale( edge, AURA_FLATTEN_NORMAL, edge);
	VectorSubtract( lerpDir, edge, lerpDir);
	VectorNormalize( lerpDir);

	// Set the viewing direction
	VectorSubtract( lerpPos, cg.refdef.vieworg, viewLine);
	VectorNormalize( viewLine);

	// Don't display this spike if it would be travelling into / out of
	// the screen too much: It is part of the blank area surrounding the root.
	CrossProduct( viewLine, lerpDir, tempVec);
	if(VectorLength(tempVec) < AURA_ROOTCUTOFF_FRAQ){

		// Only disallow drawing if it's actually a segment originating
		// from close enough to the root.
		if(Distance( basePos, state->rootPos) < AURA_ROOTCUTOFF_DIST){
			return;
		}
	}

	VectorMA( lerpPos, lerpBorder, lerpDir, lerpPos);
	VectorMA( lerpPos, lerpSize, lerpDir, endPos);
	CG_Aura_DrawSpike( lerpPos, endPos, lerpSize / 1.25f, config->auraShader, lerpColor);
}


/*
==========================
CG_Aura_ConvexHullRender
==========================
*/
static void CG_Aura_ConvexHullRender( centity_t *player, auraState_t *state, auraConfig_t *config){
	int i;

	// Don't draw the aura if it isn't active and the modulation is zero
	if(!( state->isActive ||(state->modulate > 0.0f))){
		return;
	}

	// Don't draw the aura if configuration says we shouldn't.
	if(!config->showAura){
		return;
	}

	// Build the hull. Don't continue if it can't be built.
	if(!CG_Aura_BuildConvexHull( player, state, config)){
		return;
	}

	// Clear the poly buffer
	
	// For each spike add it to the poly buffer
	// FIXME: Uses old style direct adding with trap call until buffer system is built
	for(i = 0;i < NR_AURASPIKES;i++){
		CG_LerpSpike( state, config, i, state->modulate);		
	}
}
// ===================================
//
//    A U R A   M A N A G E M E N T
//
// ===================================
/*==================
CG_Aura_AddTrail
==================*/
static void CG_Aura_AddTrail( centity_t *player, auraState_t *state, auraConfig_t *config){
	// Don't draw a trail if the aura isn't active
	if(!state->isActive){
		return;
	}

	// Don't draw a trail if configuration says we shouldn't.
	if(!config->showTrail){
		return;
	}

	// Update the trail only if we're using the boost aura
	if(!(player->currentState.powerups &(1 << PW_BOOST))){
		return;
	}

	// If we didn't draw the tail last frame, this is a new instantiation
	// of the entity and we will have to reset the tail positions.
	// NOTE: Give 1.5 second leeway for 'snapping' the tail
	//       incase we (almost) immediately restart boosting.
	if(player->lastTrailTime < (cg.time - cg.frametime - 1500)){		
		CG_ResetTrail( player->currentState.clientNum, player->lerpOrigin, 1000,
			config->trailWidth, config->trailShader, config->trailColor);
	}
	
	CG_UpdateTrailHead( player->currentState.clientNum, player->lerpOrigin);

	player->lastTrailTime = cg.time;
}


/*===================
CG_Aura_AddDebris
===================*/
static void CG_Aura_AddDebris( centity_t *player, auraState_t *state, auraConfig_t *config){
	// Don't add debris if the aura isn't active
	if(!state->isActive){
		return;
	}

	// Don't add debris could if configuration says we shouldn't.
	if(!config->generatesDebris){
		return;
	}
	
	// Generate the debris cloud only if we aren't using the boost aura (and thus
	// are using the charge aura).
	if(player->currentState.powerups &(1 << PW_BOOST)){
		return;
	}

	// Spawn the debris system if the player has just entered PVS
	if(!CG_FrameHist_HadAura( player->currentState.number)){
		PSys_SpawnCachedSystem( "AuraDebris", player->lerpOrigin, NULL, player, NULL, qtrue, qfalse);
	}

	CG_FrameHist_SetAura( player->currentState.number);
}


/*===================
CG_Aura_AddSounds
===================*/
static void CG_Aura_AddSounds( centity_t *player, auraState_t *state, auraConfig_t *config){
	// Don't add sounds if the aura isn't active
	if(!state->isActive){
		return;
	}

	// Add looping sounds depending on aura type (boost or charge)
	if(player->currentState.powerups &(1 << PW_BOOST)){
		if(config->boostLoopSound){
			trap_S_AddLoopingSound( player->currentState.number, player->lerpOrigin, vec3_origin, config->boostLoopSound);
		}
	} else{
		if(config->chargeLoopSound){
			trap_S_AddLoopingSound( player->currentState.number, player->lerpOrigin, vec3_origin, config->chargeLoopSound);
		}
	}
}


/*===================
CG_Aura_AddDLight
===================*/
static void CG_Aura_AddDLight( centity_t *player, auraState_t *state, auraConfig_t *config){
	vec3_t	lightPos;

	// add dynamic light when necessary
	if(state->isActive ||(state->lightAmt > config->lightMin)){

		// Since lerpOrigin is the lightingOrigin for the player, this will add a backsplash light for the aura.
		VectorAdd( player->lerpOrigin, cg.refdef.viewaxis[0], lightPos);

		trap_R_AddLightToScene( lightPos, state->lightAmt, // +(cos(cg.time / 50.0f) * state->lightDev),
								config->lightColor[0] * state->modulate,
								config->lightColor[1] * state->modulate,
								config->lightColor[2] * state->modulate);
	}
}


/*==================
CG_Aura_DimLight
==================*/
static void CG_Aura_DimLight( centity_t *player, auraState_t *state, auraConfig_t *config){

	if(state->isActive){

		if (state->lightAmt < config->lightMax){

			state->lightAmt += config->lightGrowthRate *(cg.frametime / 25.0f);
			if(state->lightAmt > config->lightMax){
				state->lightAmt = config->lightMax;
			}

			state->lightDev = state->lightAmt / 10;
		}

	} else{

		if (state->lightAmt > config->lightMin){
			state->lightAmt -= config->lightGrowthRate *(cg.frametime / 50.0f);
			if(state->lightAmt < config->lightMin){
				state->lightAmt = config->lightMin;
			}
		}
	}

	state->modulate = (float)(state->lightAmt - config->lightMin) / (float)(config->lightMax - config->lightMin);
	if(state->modulate < 0   ) state->modulate = 0;
	if(state->modulate > 1.0f) state->modulate = 1.0f;
}


/*========================
CG_AddAuraToScene
========================*/
void CG_AddAuraToScene( centity_t *player){
	int				clientNum, tier;
	auraState_t		*state;
	auraConfig_t	*config;

	// Get the aura system corresponding to the player
	clientNum = player->currentState.clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		CG_Error( "Bad clientNum on player entity");
		return;
	}
	state = &auraStates[ clientNum ];
	tier = cgs.clientinfo[clientNum].tierCurrent;
	config = &(state->configurations[tier]);	
	
	
	// Update origin
	VectorCopy( player->lerpOrigin, state->origin);

	// Calculate modulation for dimming
	CG_Aura_DimLight( player, state, config);

	// Add aura effects
	CG_Aura_AddSounds( player, state, config);
	CG_Aura_AddTrail( player, state, config);
	CG_Aura_AddDebris( player, state, config);
	CG_Aura_AddDLight( player, state, config);

	// Render the aura
	CG_Aura_ConvexHullRender( player, state, config);
}


/*==============
CG_AuraStart
==============*/
void CG_AuraStart( centity_t *player){
	int				clientNum, tier;
	auraState_t		*state;
	auraConfig_t	*config;

	vec3_t			groundPoint;
	trace_t			trace;

	// Get the aura system corresponding to the player
	clientNum = player->currentState.clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		CG_Error( "Bad clientNum on player entity");
		return;
	}
	state = &auraStates[ clientNum ];
	tier = cgs.clientinfo[clientNum].tierCurrent;
	config = &(state->configurations[tier]);	
	

	// If the aura is already active, don't continue activating it again.
	if(state->isActive){
		return;
	}

	// Initialize the system
	state->isActive = qtrue;
	state->lightAmt = config->lightMin;

	// create a small camerashake
	CG_AddEarthquake( player->lerpOrigin, 1000, 1, 0, 1, 200);

	// We don't want smoke jets if this is a boost aura instead of a charge aura.
	if(!(player->currentState.powerups &(1 << PW_BOOST))){
		// Check if we're on, or near ground level
		VectorCopy( player->lerpOrigin, groundPoint);
		groundPoint[2] -= 48;
		CG_Trace( &trace, player->lerpOrigin, NULL, NULL, groundPoint, player->currentState.number, CONTENTS_SOLID);
		if(trace.allsolid || trace.startsolid){
			trace.fraction = 1.0f;
		}
		if(trace.fraction < 1.0f){
			vec3_t tempAxis[3];

			// Place the explosion just a bit off the surface
			VectorNormalize2(trace.plane.normal, groundPoint);
			VectorMA( trace.endpos, 5, groundPoint, groundPoint);

			VectorNormalize2( trace.plane.normal, tempAxis[0]);
			MakeNormalVectors( tempAxis[0], tempAxis[1], tempAxis[2]);
			PSys_SpawnCachedSystem( "AuraSmokeBurst", groundPoint, tempAxis, NULL, NULL, qfalse, qfalse);
		}
	}
}


/*============
CG_AuraEnd
============*/
void CG_AuraEnd( centity_t *player){
	int			clientNum;
	auraState_t	*state;

	clientNum = player->currentState.clientNum;
	if(clientNum < 0 || clientNum >= MAX_CLIENTS){
		CG_Error( "Bad clientNum on player entity");
	}

	state = &auraStates[ clientNum ];

	// If the aura is already deactivated, don't continue deactivating it again.
	if(!state->isActive){
		return;
	}

	state->isActive = qfalse;
}


/*=======================
CG_RegisterClientAura
=======================*/
#define MAX_AURA_FILELEN 32000
void parseAura(char *path,auraConfig_t *aura);
void CG_RegisterClientAura(int clientNum,clientInfo_t *ci){
	int	i;
	char filename[MAX_QPATH * 2];
	char auraPath[MAX_QPATH];
	memset(&(auraStates[clientNum]), 0, sizeof(auraState_t));
	for(i = 0;i < 8;i++){ 
		ci->auraConfig[i] = &(auraStates[clientNum].configurations[i]);
		memset(ci->auraConfig[i],0,sizeof(auraConfig_t));
		Com_sprintf(filename,sizeof(filename),"players/tierDefault.cfg",ci->modelName,i+1);
		parseAura(filename,ci->auraConfig[i]);
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/tier.cfg",ci->modelName,i+1);
		parseAura(filename,ci->auraConfig[i]);
	}
}
void parseAura(char *path,auraConfig_t *aura){
	fileHandle_t auraCFG;
	int i;
	char *token,*parse;
	int fileLength;
	char fileContents[32000];
	if(trap_FS_FOpenFile(path,0,FS_READ)>0){
		fileLength = trap_FS_FOpenFile(path,&auraCFG,FS_READ);
		trap_FS_Read(fileContents,fileLength,auraCFG);
		fileContents[fileLength] = 0;
		trap_FS_FCloseFile(auraCFG);
		parse = fileContents;
		while(1){
			token = COM_Parse(&parse);
			if(!token[0]){break;}
			else if(!Q_stricmp(token,"auraExists")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->showAura = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"auraAlways")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->auraAlways = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"hasAuraLight")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->showLight = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"hasAuraTrail")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->showTrail = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"hasAuraDebris")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->generatesDebris = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp( token, "auraTagCount")){
				for(i = 0;i < 3;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->numTags[i] = atoi( token);
				}
			}
			else if(!Q_stricmp( token, "auraShader")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->auraShader = trap_R_RegisterShader(token);
			}
			else if(!Q_stricmp( token, "auraTrailShader")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->trailShader = trap_R_RegisterShader(token);
			}
			else if(!Q_stricmp( token, "auraTrailWidth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->trailWidth = atoi(token);
			}
			else if(!Q_stricmp( token, "auraChargeLoop")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->chargeLoopSound = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp( token, "auraChargeStart")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->chargeStartSound = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp( token, "auraBoostLoop")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->boostLoopSound = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp( token, "auraBoostStart")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->boostStartSound = trap_S_RegisterSound(token,qfalse);
			}
			else if (!Q_stricmp( token, "auraColor")){
				for(i = 0;i < 3;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->auraColor[i] = atof( token);
					if(aura->auraColor[i] < 0) aura->auraColor[i] = 0;
					if(aura->auraColor[i] > 1) aura->auraColor[i] = 1;
				}
			}
			else if(!Q_stricmp( token, "auraLightColor")){
				for(i = 0;i < 3;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->lightColor[i] = atof( token);
					if(aura->lightColor[i] < 0) aura->lightColor[i] = 0;
					if(aura->lightColor[i] > 1) aura->lightColor[i] = 1;
				}
			}
			else if(!Q_stricmp( token, "auraTrailColor")){
				for(i = 0;i < 3;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					aura->trailColor[i] = atof( token);
					if(aura->trailColor[i] < 0) aura->trailColor[i] = 0;
					if(aura->trailColor[i] > 1) aura->trailColor[i] = 1;
				}
			}
			else if(!Q_stricmp( token, "auraScale")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->auraScale = atof(token);
			}
			else if(!Q_stricmp( token, "auraLength")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->tailLength = atoi(token);
			}
			else if(!Q_stricmp( token, "auraLightMin")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->lightMin = atoi(token);
			}
			else if(!Q_stricmp( token, "auraLightMax")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->lightMax = atoi(token);
			}
			else if(!Q_stricmp( token, "auraLightGrowthRate")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->lightGrowthRate = atoi(token);
			}
			else if(!Q_stricmp(token,"showLightning")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->showLightning = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"lightningShader")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				aura->lightningShader = trap_R_RegisterShaderNoMip(token);
			}
		}
	}
}
void CG_CopyClientAura( int from, int to){
	memcpy(&auraStates[to], &auraStates[from], sizeof(auraState_t));
}

