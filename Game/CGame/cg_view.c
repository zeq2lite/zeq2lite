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
// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

#define MASK_CAMERACLIP (MASK_SOLID|CONTENTS_PLAYERCLIP)
#define CAMERA_SIZE	4

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like 
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f (void) {
	vec3_t		angles;

	memset( &cg.testModelEntity, 0, sizeof(cg.testModelEntity) );
	if ( trap_Argc() < 2 ) {
		return;
	}

	Q_strncpyz (cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	if ( trap_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if (! cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f (void) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f (void) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f (void) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f (void) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f (void) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel (void) {
	int		i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	if (! cg.testModelEntity.hModel ) {
		CG_Printf ("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for (i=0 ; i<3 ; i++) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect (void) {
	int		size;

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		size = 100;
	} else {
		// bound normal viewsize
		if (cg_viewsize.integer < 30) {
			trap_Cvar_Set ("cg_viewsize","30");
			size = 30;
		} else if (cg_viewsize.integer > 100) {
			trap_Cvar_Set ("cg_viewsize","100");
			size = 100;
		} else {
			size = cg_viewsize.integer;
		}

	}
	cg.refdef.width = cgs.glconfig.vidWidth*size/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*size/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;
}



//==============================================================================


/*
=================================

CG_WorldCoordToScreenCoordFloat

=================================
Gives screen projection of point in worldspace.
Returns false if out of view.
*/
qboolean CG_WorldCoordToScreenCoordFloat(vec3_t worldCoord, float *x, float *y) {
	float xcenter, ycenter;
	vec3_t local, transformed;
	vec3_t vforward;
	vec3_t vright;
	vec3_t vup;
	float xzi;
	float yzi;

	xcenter = 640.0f / 2.0f;//gives screen coords in virtual 640x480, to be adjusted when drawn
	ycenter = 480.0f / 2.0f;//gives screen coords in virtual 640x480, to be adjusted when drawn

	AngleVectors( cg.refdefViewAngles, vforward, vright, vup );

	VectorSubtract( worldCoord, cg.refdef.vieworg, local );

	transformed[0] = DotProduct( local,vright );
	transformed[1] = DotProduct( local,vup );
	transformed[2] = DotProduct( local,vforward );

	// Make sure Z is not negative.
	if(transformed[2] < 0.01f) {
		return qfalse;
	}

	xzi = xcenter / transformed[2] * (  96.0f / cg.refdef.fov_x );
	yzi = ycenter / transformed[2] * ( 102.0f / cg.refdef.fov_y );

	*x = xcenter + xzi * transformed[0];
	*y = ycenter - yzi * transformed[1];

	return qtrue;
}


qboolean CG_WorldCoordToScreenCoordVec( vec3_t world, vec2_t screen ) {
	return CG_WorldCoordToScreenCoordFloat( world, &screen[0], &screen[1] );
}


//==============================================================================

//==============================================================================
//==============================================================================

#define CAMERA_DAMP_INTERVAL	50

static vec3_t	cameramins = { -CAMERA_SIZE, -CAMERA_SIZE, -CAMERA_SIZE };
static vec3_t	cameramaxs = { CAMERA_SIZE, CAMERA_SIZE, CAMERA_SIZE };
vec3_t	camerafwd, cameraup;

vec3_t	cameraFocusAngles,			cameraFocusLoc;
vec3_t	cameraIdealTarget,			cameraIdealLoc;
vec3_t	cameraCurTarget={0,0,0},	cameraCurLoc={0,0,0};
vec3_t	cameraOldLoc={0,0,0},		cameraNewLoc={0,0,0};
int		cameraLastFrame=0;

float	cameraLastYaw=0;
float	cameraStiffFactor=0.0f;

float qvm_powf ( float x, int y )
{
	float r = x;
	for ( y--; y>0; y-- )
		r = r * r;
	return r;
}

/*
===============
Notes on the camera viewpoint in and out...

cg.refdef.vieworg
--at the start of the function holds the player actor's origin (center of player model).
--it is set to the final view location of the camera at the end of the camera code.
cg.refdef.viewangles
--at the start holds the client's view angles
--it is set to the final view angle of the camera at the end of the camera code.

===============
*/

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewTarget(void)
{
	float vertOffset = cg_thirdPersonHeight.value;
	float horzOffset = cg_thirdPersonSlide.value;

	VectorCopy(cg.refdef.vieworg, cameraFocusLoc);

	// Add in the new viewheight
	cameraFocusLoc[2] += cg.snap->ps.viewheight;
	
	// Add in a vertical offset from the viewpoint, which puts the actual target above the head, regardless of angle.
	VectorCopy( cameraFocusLoc, cameraIdealTarget );

	cameraIdealTarget[2] += vertOffset;
	cameraIdealTarget[1] += horzOffset;
}

/*
===============
CG_CalcTargetThirdPersonViewLocation

===============
*/
static void CG_CalcIdealThirdPersonViewLocation(void)
{
	float thirdPersonRange = 80; //cg_thirdPersonRange.value;

	VectorMA(cameraIdealTarget, -(thirdPersonRange), camerafwd, cameraIdealLoc);
}

static void CG_ResetThirdPersonViewDamp(void)
{
	trace_t trace;

	// Cap the pitch within reasonable limits
	if (cameraFocusAngles[PITCH] > 89.0)
	{
		cameraFocusAngles[PITCH] = 89.0;
	}
	else if (cameraFocusAngles[PITCH] < -89.0)
	{
		cameraFocusAngles[PITCH] = -89.0;
	}

	AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

	// Set the cameraIdealTarget
	//CG_CalcIdealThirdPersonViewTarget();

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// Now, we just set everything to the new positions.
	VectorCopy(cameraIdealLoc, cameraCurLoc);
	VectorCopy(cameraIdealTarget, cameraCurTarget);

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction <= 1.0)
	{
		VectorCopy(trace.endpos, cameraCurLoc);
	}

	cameraLastFrame = cg.time;
	cameraLastYaw = cameraFocusAngles[YAW];
	cameraStiffFactor = 0.0f;
}

// This is called every frame.
static void CG_UpdateThirdPersonTargetDamp(void)
{
	trace_t trace;
	vec3_t	targetdiff;
	float	dampfactor, dtime, ratio;

	// Set the cameraIdealTarget
	// Automatically get the ideal target, to avoid jittering.
	CG_CalcIdealThirdPersonViewTarget();

	if (cg_thirdPersonTargetDamp.value>=1.0||cg_thirdPersonMeleeTargetDamp.value>=1.0||cg.thisFrameTeleport||cg.snap->ps.bitFlags & usingZanzoken)
	{	// No damping.
		VectorCopy(cameraIdealTarget, cameraCurTarget);
	}
	else if (cg_thirdPersonTargetDamp.value>=0.0||cg_thirdPersonMeleeTargetDamp.value>=0.0)
	{	
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealTarget, cameraCurTarget, targetdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-cg_thirdPersonTargetDamp.value;
		dtime = (float)(cg.time-cameraLastFrame) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible, 
		// the ratio should be initialized into a chart ultimately.
		ratio = qvm_powf(dampfactor, dtime);
		
		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealTarget, -ratio, targetdiff, cameraCurTarget);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace to see if the new location is cool or not.

	// First thing we do is trace from the first person viewpoint out to the new target location.
	CG_Trace(&trace, cameraFocusLoc, cameramins, cameramaxs, cameraCurTarget, cg.snap->ps.clientNum, MASK_CAMERACLIP);
	if (trace.fraction < 1.0)
	{
		VectorCopy(trace.endpos, cameraCurTarget);
	}
}

// This can be called every interval, at the user's discretion.
//extern void CG_CalcEntityLerpPositions( centity_t *cent ); //cg_ents.c
static void CG_UpdateThirdPersonCameraDamp(void)
{
	trace_t trace;
	vec3_t	locdiff;
	float dampfactor, dtime, ratio;

	// Set the cameraIdealLoc
	CG_CalcIdealThirdPersonViewLocation();

	// First thing we do is calculate the appropriate damping factor for the camera.
	dampfactor=0.0;
	if (cg_thirdPersonCameraDamp.value != 0.0 || cg_thirdPersonMeleeCameraDamp.value != 0.0)
	{
		float pitch;
		float dFactor;
		dFactor = cg_thirdPersonCameraDamp.value;

		// Note that the camera pitch has already been capped off to 89.
		pitch = Q_fabs(cameraFocusAngles[PITCH]);

		// The higher the pitch, the larger the factor, so as you look up, it damps a lot less.
		pitch /= 115.0;	
		dampfactor = (1.0-dFactor)*(pitch*pitch);

		dampfactor += dFactor;

		// Now we also multiply in the stiff factor, so that faster yaw changes are stiffer.
		if (cameraStiffFactor > 0.0f)
		{	// The cameraStiffFactor is how much of the remaining damp below 1 should be shaved off, i.e. approach 1 as stiffening increases.
			dampfactor += (1.0-dampfactor)*cameraStiffFactor;
		}
	}

	if (dampfactor>=1.0||cg.thisFrameTeleport||cg.snap->ps.bitFlags & usingZanzoken)
	{	// No damping.
		VectorCopy(cameraIdealLoc, cameraCurLoc);
	}
	else if (dampfactor>=0.0)
	{	
		// Calculate the difference from the current position to the new one.
		VectorSubtract(cameraIdealLoc, cameraCurLoc, locdiff);

		// Now we calculate how much of the difference we cover in the time allotted.
		// The equation is (Damp)^(time)
		dampfactor = 1.0-dampfactor;	// We must exponent the amount LEFT rather than the amount bled off
		dtime = (float)(cg.time-cameraLastFrame) * (1.0/(float)CAMERA_DAMP_INTERVAL);	// Our dampfactor is geared towards a time interval equal to "1".

		// Note that since there are a finite number of "practical" delta millisecond values possible, 
		// the ratio should be initialized into a chart ultimately.
		ratio = qvm_powf(dampfactor, dtime);
		
		// This value is how much distance is "left" from the ideal.
		VectorMA(cameraIdealLoc, -ratio, locdiff, cameraCurLoc);
		/////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

	// Now we trace from the new target location to the new view location, to make sure there is nothing in the way.
	CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);

	if (trace.fraction < 1.0)
	{
		if (trace.entityNum < ENTITYNUM_WORLD &&
			cg_entities[trace.entityNum].currentState.solid == SOLID_BMODEL &&
			cg_entities[trace.entityNum].currentState.eType == ET_MOVER)
		{ //get a different position for movers
			centity_t *mover = &cg_entities[trace.entityNum];

			//this is absolutely hackiful, since we calc view values before we add packet ents and lerp,
			//if we hit a mover we want to update its lerp pos and force it when we do the trace against
			//it.
			if (mover->currentState.pos.trType != TR_STATIONARY &&
				mover->currentState.pos.trType != TR_LINEAR)
			{
				int curTr = mover->currentState.pos.trType;
				vec3_t curTrB;

				VectorCopy(mover->currentState.pos.trBase, curTrB);

				//calc lerporigin for this client frame
				//CG_CalcEntityLerpPositions(mover);

				//force the calc'd lerp to be the base and say we are stationary so we don't try to extrapolate
				//out further.
				mover->currentState.pos.trType = TR_STATIONARY;
				VectorCopy(mover->lerpOrigin, mover->currentState.pos.trBase);
				
				//retrace
				CG_Trace(&trace, cameraCurTarget, cameramins, cameramaxs, cameraCurLoc, cg.snap->ps.clientNum, MASK_CAMERACLIP);

				//copy old data back in
				mover->currentState.pos.trType = (trType_t) curTr;
				VectorCopy(curTrB, mover->currentState.pos.trBase);
			}
			if (trace.fraction < 1.0f)
			{ //still hit it, so take the proper trace endpos and use that.
				VectorCopy(trace.endpos, cameraCurLoc);
			}
		}
		else
		{
			VectorCopy( trace.endpos, cameraCurLoc );
		}
	}
}

/*
===============
CG_OffsetThirdPersonView2

===============
*/
static void CG_OffsetThirdPersonView2( void ) 
{
	vec3_t diff;
	vec3_t forward;
	float deltayaw;
	float		transformPercent;
	float		orbit,pan,zoom;
	int 		clientNum;
	float		newAngle,newRange,newHeight;
	entityState_t *ent;
	clientInfo_t *ci;
	tierConfig_cg *tier;

	clientNum = cg.predictedPlayerState.clientNum;
	ci = &cgs.clientinfo[clientNum];

	cameraStiffFactor = 0.0;

	// Set camera viewing direction.
	VectorCopy( cg.refdefViewAngles, cameraFocusAngles );

	// Add in the third Person Angle.
	cameraFocusAngles[YAW] += cg_thirdPersonAngle.value;

	if(cg_beamControl.value == 0){
		if(((cg.snap->ps.weaponstate == WEAPON_GUIDING) || (cg.snap->ps.weaponstate == WEAPON_ALTGUIDING) ) && (cg.guide_view)) {
			float oldRoll;
			VectorSubtract( cg.guide_target, cg.snap->ps.origin, forward );
			VectorNormalize( forward );
			oldRoll = cameraFocusAngles[ROLL];
			vectoangles( forward, cameraFocusAngles );
			cg.refdefViewAngles[ROLL] = oldRoll;
			VectorCopy( cg.snap->ps.origin,cg.guide_target);
			cg.guide_view = qfalse;
		}
	}
	// TRANSFORMATIONS
	if(cg.snap->ps.timers[tmTransform] > 1){
		tier = &ci->tierConfig[ci->tierCurrent];
		if(!ci->transformStart){
			tier = &ci->tierConfig[ci->tierCurrent+1];
			ci->transformStart = qtrue;
			ci->transformLength = cg.snap->ps.timers[tmTransform];
		}
		transformPercent = 1.0 - ((float)cg.snap->ps.timers[tmTransform] / (float)ci->transformLength);
		orbit = (float)abs(tier->transformCameraOrbit[1] - tier->transformCameraOrbit[0]);
		pan = (float)abs(tier->transformCameraPan[1] - tier->transformCameraPan[0]);
		zoom = (float)abs(tier->transformCameraZoom[1] - tier->transformCameraZoom[0]);
		newAngle = (orbit*transformPercent) - abs(tier->transformCameraOrbit[0]);
		newHeight = (pan*transformPercent) - abs(tier->transformCameraPan[0]);
		newRange = (zoom*transformPercent) - abs(tier->transformCameraZoom[0]);
		cg_thirdPersonAngle.value = orbit > 0 ? newAngle : tier->transformCameraDefault[0];
		cg_thirdPersonHeight.value = pan > 0 ? newHeight : tier->transformCameraDefault[1];
		cg_thirdPersonRange.value = zoom > 0 ? newRange : tier->transformCameraDefault[2];
	}
	else if(ci->transformStart){
		ci->transformStart = qfalse;
		cg_thirdPersonAngle.value = ci->cameraBackup[0];
		cg_thirdPersonHeight.value = ci->cameraBackup[1];
		cg_thirdPersonRange.value = ci->cameraBackup[2];
		cg_thirdPersonSlide.value = ci->cameraBackup[3];
	}

	// The next thing to do is to see if we need to calculate a new camera target location.

	// If we went back in time for some reason, or if we just started, reset the sample.
	if (cameraLastFrame == 0 || cameraLastFrame > cg.time)
	{
		CG_ResetThirdPersonViewDamp();
	}
	else
	{
		// Cap the pitch within reasonable limits

		if (cameraFocusAngles[PITCH] > 80.0)
		{
			cameraFocusAngles[PITCH] = 80.0;
		}
		else if (cameraFocusAngles[PITCH] < -80.0)
		{
			cameraFocusAngles[PITCH] = -80.0;
		}

		AngleVectors(cameraFocusAngles, camerafwd, NULL, cameraup);

		deltayaw = fabs(cameraFocusAngles[YAW] - cameraLastYaw);
		if (deltayaw > 180.0f)
		{ // Normalize this angle so that it is between 0 and 180.
			deltayaw = fabs(deltayaw - 360.0f);
		}
		cameraStiffFactor = deltayaw / (float)(cg.time-cameraLastFrame);
		if (cameraStiffFactor < 1.0)
		{
			cameraStiffFactor = 0.0;
		}
		else if (cameraStiffFactor > 2.5)
		{
			cameraStiffFactor = 0.75;
		}
		else
		{	// 1 to 2 scales from 0.0 to 0.5
			cameraStiffFactor = (cameraStiffFactor-1.0f)*0.5f;
		}
		cameraLastYaw = cameraFocusAngles[YAW];

		// Move the target to the new location.
		CG_UpdateThirdPersonTargetDamp();
		CG_UpdateThirdPersonCameraDamp();
	}

	// We must now take the angle taken from the camera target and location.
	VectorSubtract(cameraCurTarget, cameraCurLoc, diff);
	{
		float dist = VectorNormalize(diff);
		//under normal circumstances, should never be 0.00000 and so on.
		if ( !dist || (diff[0] == 0 || diff[1] == 0) )
		{//must be hitting something, need some value to calc angles, so use cam forward
			VectorCopy( camerafwd, diff );
		}
	}

	vectoangles(diff, cg.refdefViewAngles);

	// ...and of course we should copy the new view location to the proper spot too.
	VectorCopy(cameraCurLoc, cg.refdef.vieworg);

	cameraLastFrame=cg.time;
}

/*
===============
CG_OffsetThirdPersonView

===============
*/
#define	FOCUS_DISTANCE	512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t		forward, right, up;
	vec3_t		view;
	vec3_t		focusAngles;
	trace_t		trace;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	vec3_t		focusPoint;
	float		focusDist;
	float		forwardScale, sideScale;
	// ADDING FOR ZEQ2
	playerState_t	*ps;
	vec3_t		overrideAngles;
	vec3_t		overrideOrg;
	vec3_t		rotationOffsetAngles;
	vec3_t		focusDirCam;
	vec3_t		focusDirOrig;
	vec4_t		quatOrient;
	vec4_t		quatRot;
	vec4_t		quatResult;
	float		transformPercent;
	float		orbit,pan,zoom;
	int 		clientNum;
	float		newAngle,newRange,newHeight;
	entityState_t *ent;
	clientInfo_t *ci;
	tierConfig_cg *tier;
	ps = &cg.predictedPlayerState;
	clientNum = cg.predictedPlayerState.clientNum;
	ci = &cgs.clientinfo[clientNum];
	if(cg_beamControl.value == 0){
		if(((ps->weaponstate == WEAPON_GUIDING) || (ps->weaponstate == WEAPON_ALTGUIDING) ) && (cg.guide_view)) {
			float oldRoll;
			VectorSubtract( cg.guide_target, ps->origin, forward );
			VectorNormalize( forward );
			oldRoll = cg.refdefViewAngles[ROLL];
			vectoangles( forward, cg.refdefViewAngles );
			cg.refdefViewAngles[ROLL] = oldRoll;
			VectorCopy( ps->origin,cg.guide_target);
			cg.guide_view = qfalse;
		}	
	}
	// backup for when flying
	VectorCopy( cg.refdefViewAngles, overrideAngles );
	VectorCopy( cg.refdef.vieworg, overrideOrg );
	// END ADDING
	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;
	VectorCopy( cg.refdefViewAngles, focusAngles );
	AngleVectors( focusAngles, forward, NULL, NULL );
	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );
	VectorCopy( cg.refdef.vieworg, view );	
	AngleVectors( cg.refdefViewAngles, forward, right, up );
	// LOCKED ON
	if(ps->lockedTarget > 0){
		if(!cg.lockonStart){cg.lockonStart = qtrue;}
		cg_thirdPersonAngle.value = cg_lockedAngle.value;
		cg_thirdPersonHeight.value = cg_lockedHeight.value;
		cg_thirdPersonRange.value = cg_lockedRange.value;
		cg_thirdPersonSlide.value = cg_lockedSlide.value;
	}
	else if(cg.lockonStart && ci->cameraBackup[2]){
		cg.lockonStart = qfalse;
		cg_thirdPersonAngle.value = ci->cameraBackup[0];
		cg_thirdPersonHeight.value = ci->cameraBackup[1];
		cg_thirdPersonRange.value = ci->cameraBackup[2];
		cg_thirdPersonSlide.value = ci->cameraBackup[3];
	}
	// TRANSFORMATIONS
	if(ps->timers[tmTransform] > 1){
		tier = &ci->tierConfig[ci->tierCurrent];
		if(!ci->transformStart){
			tier = &ci->tierConfig[ci->tierCurrent+1];
			ci->transformStart = qtrue;
			ci->transformLength = ps->timers[tmTransform];
		}
		transformPercent = 1.0 - ((float)ps->timers[tmTransform] / (float)ci->transformLength);
		orbit = (float)abs(tier->transformCameraOrbit[1] - tier->transformCameraOrbit[0]);
		pan = (float)abs(tier->transformCameraPan[1] - tier->transformCameraPan[0]);
		zoom = (float)abs(tier->transformCameraZoom[1] - tier->transformCameraZoom[0]);
		newAngle = (orbit*transformPercent) - abs(tier->transformCameraOrbit[0]);
		newHeight = (pan*transformPercent) - abs(tier->transformCameraPan[0]);
		newRange = (zoom*transformPercent) - abs(tier->transformCameraZoom[0]);
		cg_thirdPersonAngle.value = orbit > 0 ? newAngle : tier->transformCameraDefault[0];
		cg_thirdPersonHeight.value = pan > 0 ? newHeight : tier->transformCameraDefault[1];
		cg_thirdPersonRange.value = zoom > 0 ? newRange : tier->transformCameraDefault[2];
	}
	else if(ci->transformStart){
		ci->transformStart = qfalse;
		cg_thirdPersonAngle.value = ci->cameraBackup[0];
		cg_thirdPersonHeight.value = ci->cameraBackup[1];
		cg_thirdPersonRange.value = ci->cameraBackup[2];
		cg_thirdPersonSlide.value = ci->cameraBackup[3];
	}
	//view[1] += cg_thirdPersonSlide.value;
	VectorMA(view,cg_thirdPersonSlide.value,right,view);
	view[2] += cg_thirdPersonHeight.value;
	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );	
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
	if (!cg_cameraMode.integer) {
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, clientNum, MASK_SOLID );

		if ( trace.fraction != 1.0 ) {
			VectorCopy( trace.endpos, view );
			view[2] += (1.0 - trace.fraction) * 32;
			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out

			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, clientNum, MASK_SOLID );
			VectorCopy( trace.endpos, view );
		}
	}
	VectorCopy( view, cg.refdef.vieworg );
	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if(focusDist < 1){
		focusDist = 1;	// should never happen
	}
	//cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	// ADDING FOR ZEQ2
	if (cg_thirdPersonCamera.value == 0) {
		//if(cg.snap->ps.bitFlags & usingFlight){
			VectorCopy( overrideAngles, cg.refdefViewAngles );
			// Apply offset for thirdperson angle, if it's present in LOCAL(!) coordinate system


			AngleVectors( cg.refdefViewAngles, forward, NULL, up );
			VectorMA( overrideOrg, cg.predictedPlayerState.viewheight, up, overrideOrg );
			VectorMA( overrideOrg, FOCUS_DISTANCE, forward, focusPoint );
			VectorMA( overrideOrg, cg_thirdPersonHeight.value, up, overrideOrg);
			VectorMA( overrideOrg, cg_thirdPersonSlide.value, right, cg.refdef.vieworg );
			VectorMA( cg.refdef.vieworg, -cg_thirdPersonRange.value, forward, cg.refdef.vieworg );
			if (!cg_cameraMode.integer) {
				CG_Trace( &trace, overrideOrg, mins, maxs, cg.refdef.vieworg, clientNum, MASK_SOLID );
				if ( trace.fraction != 1.0f ) {
					VectorCopy( trace.endpos, cg.refdef.vieworg );
					VectorMA(cg.refdef.vieworg, (1.0f - trace.fraction) * 32, up, cg.refdef.vieworg);

					VectorCopy( trace.endpos, cg.refdef.vieworg );
				}
			}
			// Get the required pitch rotation for the LOCAL(!) coordinate system of the player
			VectorSubtract(focusPoint, cg.refdef.vieworg, focusDirCam);
			VectorSubtract(focusPoint, overrideOrg, focusDirOrig);
			rotationOffsetAngles[PITCH] = RAD2DEG( hack_acos( DotProduct( focusDirCam, focusDirOrig ) ) );
			rotationOffsetAngles[YAW] = 0;
			rotationOffsetAngles[ROLL] = 0;
			// Use quaternions to correctly alter the angles in the local coordinate system
			AnglesToQuat(cg.refdefViewAngles, quatOrient);
			AnglesToQuat(rotationOffsetAngles, quatRot);
			QuatMul(quatOrient, quatRot, quatResult);
			QuatToAngles(quatResult, cg.refdefViewAngles);
			AngleNormalize180(cg.refdefViewAngles[0]);
			AngleNormalize180(cg.refdefViewAngles[1]);
			AngleNormalize180(cg.refdefViewAngles[2]);
		//}	
	}
	// END ADDING
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int		timeDelta;
	
	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange 
			* (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
//static playerEntity_t	playerInfoDuplicate[MAX_GENTITIES];
static void CG_OffsetFirstPersonView( void ) {
	float			*origin;
	float			*angles;
	float			bob;
	float			ratio;
	float			delta;
	float			speed;
	float			f;
	vec3_t			predictedVelocity;
	int				timeDelta;
	int 			clientNum;
	int				i;
	orientation_t	*tagOrient;
	orientation_t	lerped;
	playerEntity_t	*pe;
	vec3_t			tempAxis[3];
	clientNum = cg.predictedPlayerState.clientNum;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	// HACK: Use this copy, which is sure to be stored correctly, unlike
	//       reading it from cg_entities, which tends to clear out its
	//       fields every now and then. WTF?!
	//pe = &playerInfoDuplicate[clientNum];
	
	// Prepare the destination orientation_t
	//AxisClear( tagOrient->axis );

	// Try to find the tag and return its coordinates
	if ( trap_R_LerpTag( &lerped, pe->headRef.hModel, pe->head.oldFrame, pe->head.frame, 1.0 - pe->head.backlerp, "tag_eyes" ) ) {
		VectorCopy( pe->headRef.origin, tagOrient->origin );
		for ( i = 0 ; i < 3 ; i++ ) {
			VectorMA( tagOrient->origin, lerped.origin[i], pe->headRef.axis[i], tagOrient->origin );
		}

		MatrixMultiply( tagOrient->axis, lerped.axis, tempAxis );
		MatrixMultiply( tempAxis, pe->headRef.axis, tagOrient->axis );

		VectorCopy( tagOrient->origin, origin);
		//origin = cg.refdef.vieworg;
		angles = cg.refdefViewAngles;
	} else {
		origin = cg.refdef.vieworg;
		angles = cg.refdefViewAngles;
	}

/*
	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.powerLevel[current] <= 0 ) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[deathCameraAngle];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}
*/
	// add angles based on weapon kick
	VectorAdd (angles, cg.kick_angles, angles);

	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );

	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;
	
	delta = DotProduct ( predictedVelocity, cg.refdef.viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if (cg.bobcycle & 1)
		delta = -delta;
	angles[ROLL] += delta;

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < DUCK_TIME) {
		cg.refdef.vieworg[2] -= cg.duckChange 
			* (DUCK_TIME - timeDelta) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if (bob > 6) {
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	// add kick offset

	VectorAdd (origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define	NECK_LENGTH		8
	vec3_t			forward, up;
 
	cg.refdef.vieworg[2] -= NECK_LENGTH;
	AngleVectors( cg.refdefViewAngles, forward, NULL, up );
	VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

//======================================================================

void CG_ZoomDown_f( void ) { 
	if ( cg.zoomed ) {
		return;
	}
	cg.zoomed = qtrue;
	cg.zoomTime = cg.time;
}

void CG_ZoomUp_f( void ) { 
	if ( !cg.zoomed ) {
		return;
	}
	cg.zoomed = qfalse;
	cg.zoomTime = cg.time;
}


/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define	WAVE_AMPLITUDE	1
#define	WAVE_FREQUENCY	0.4

static int CG_CalcFov( void ) {
	float	x;
	float	phase;
	float	v;
	int		contents;
	float	fov_x, fov_y;
	float	zoomFov;
	float	f;
	int		inwater;

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		fov_x = 90;
	} else {
		// user selectable
		if ( cgs.dmflags & DF_FIXED_FOV ) {
			// dmflag to prevent wide fov for all clients
			fov_x = 90;
		} else {
			fov_x = cg_fov.value;
			if ( fov_x < 1 ) {
				fov_x = 1;
			} else if ( fov_x > 160 ) {
				fov_x = 160;
			}
		}

		// account for zooms
		zoomFov = cg_zoomFov.value;
		if ( zoomFov < 1 ) {
			zoomFov = 1;
		} else if ( zoomFov > 160 ) {
			zoomFov = 160;
		}

		if ( cg.zoomed ) {
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
			if ( f > 1.0 ) {
				fov_x = zoomFov;
			} else {
				fov_x = fov_x + f * ( zoomFov - fov_x );
			}
		} else {
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
			if ( f > 1.0 ) {
				fov_x = fov_x;
			} else {
				fov_x = zoomFov + f * ( fov_x - zoomFov );
			}
		}
	}

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
	}
	else {
		inwater = qfalse;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if ( !cg.zoomed ) {
		cg.zoomSensitivity = 1;
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
}



/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void ) {
	int			t;
	int			maxTime;
	refEntity_t		ent;

	if ( !cg.damageValue ) {
		return;
	}

	//if (cg.cameraMode) {
	//	return;
	//}

	// ragePro systems can't fade blends, so don't obscure the screen
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		return;
	}

	maxTime = DAMAGE_TIME;
	t = cg.time - cg.damageTime;
	if ( t <= 0 || t >= maxTime ) {
		return;
	}


	memset( &ent, 0, sizeof( ent ) );
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_FIRST_PERSON;

	VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
	VectorMA( ent.origin, cg.damageX * -8, cg.refdef.viewaxis[1], ent.origin );
	VectorMA( ent.origin, cg.damageY * 8, cg.refdef.viewaxis[2], ent.origin );

	ent.radius = cg.damageValue * 3;
	ent.customShader = cgs.media.viewBloodShader;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 200 * ( 1.0 - ((float)t / maxTime) );
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
JUHOX: CG_AddEarthquake
===============
*/
#if EARTHQUAKE_SYSTEM
void CG_AddEarthquake(
	const vec3_t origin, float radius,
	float duration, float fadeIn, float fadeOut,	// in seconds
	float amplitude
) {
	int i;

	// So camera dosn't make your head explode from too much shaking ...
	if ( amplitude > 1500 ) {
		amplitude = 1500;
	}

	if (duration <= 0) {
		float a;

		a = amplitude / 100;

		if (radius > 0) {
			float distance;
			
			distance = Distance(cg.refdef.vieworg, origin);
			if (distance >= radius) return;

			a *= 1 - (distance / radius);
		}

		cg.additionalTremble += a;
		return;
	}

	for (i = 0; i < MAX_EARTHQUAKES; i++) {
		earthquake_t* quake;

		quake = &cg.earthquakes[i];
		if (quake->startTime) continue;

		quake->startTime = cg.time;
		quake->endTime = (int) floor(cg.time + 1000 * duration + 0.5);
		quake->fadeInTime = (int) floor(1000 * fadeIn + 0.5);
		quake->fadeOutTime = (int) floor(1000 * fadeOut + 0.5);
		quake->amplitude = amplitude;
		VectorCopy(origin, quake->origin);
		quake->radius = radius;
		break;
	}
}
#endif

/*
===============
JUHOX: CG_AdjustEarthquakes
===============
*/
#if EARTHQUAKE_SYSTEM
void CG_AdjustEarthquakes(const vec3_t delta) {
	int i;

	for (i = 0; i < MAX_EARTHQUAKES; i++) {
		earthquake_t* quake;

		quake = &cg.earthquakes[i];
		if (!quake->startTime) continue;
		if (quake->radius <= 0) continue;

		VectorAdd(quake->origin, delta, quake->origin);
	}
}
#endif

/*
===============
JUHOX: AddEarthquakeTremble
===============
*/
#if EARTHQUAKE_SYSTEM
static void AddEarthquakeTremble(earthquake_t* quake) {
	int time;
	float a;
	const float offsetAmplitude = 0.2;
	const float angleAmplitude = 0.2;

	if (quake) {
		if (cg.time >= quake->endTime) {
			memset(quake, 0, sizeof(*quake));
			return;
		}

		if (quake->radius > 0) {
			float distance;
			
			distance = Distance(cg.refdef.vieworg, quake->origin);
			if (distance >= quake->radius) return;

			a = 1 - (distance / quake->radius);
		}
		else {
			a = 1;
		}

		time = cg.time - quake->startTime;
		a *= quake->amplitude / 100;
		if (time < quake->fadeInTime) {
			a *= (float)time / (float)(quake->fadeInTime);
		}
		else if (cg.time > quake->endTime - quake->fadeOutTime) {
			a *= (float)(quake->endTime - cg.time) / (float)(quake->fadeOutTime);
		}
	}
	else {
		a = cg.additionalTremble;
	}

	cg.refdef.vieworg[0] += offsetAmplitude * a * crandom();
	cg.refdef.vieworg[1] += offsetAmplitude * a * crandom();
	cg.refdef.vieworg[2] += offsetAmplitude * a * crandom();
	cg.refdefViewAngles[YAW] += angleAmplitude * a * crandom();
	cg.refdefViewAngles[PITCH] += angleAmplitude * a * crandom();
	cg.refdefViewAngles[ROLL] += angleAmplitude * a * crandom();
}
#endif

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues( void ) {
	playerState_t	*ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;
/*
	if (cg.cameraMode) {
		vec3_t origin, angles;
		if (trap_getCameraInfo(cg.time, &origin, &angles)) {
			VectorCopy(origin, cg.refdef.vieworg);
			angles[ROLL] = 0;
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
			return CG_CalcFov();
		} else {
			cg.cameraMode = qfalse;
		}
	}
*/
	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );		
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
		ps->velocity[1] * ps->velocity[1] );


	VectorCopy( ps->origin, cg.refdef.vieworg );
	VectorCopy( ps->viewangles, cg.refdefViewAngles );
	
	if (cg_cameraOrbit.integer) {
		if (cg.time > cg.nextOrbitTime) {
			cg.nextOrbitTime = cg.time + cg_cameraOrbitDelay.integer;
			cg_thirdPersonAngle.value += cg_cameraOrbit.value;
		}
	}
	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int		t;
		float	f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	if ( cg.renderingThirdPerson ) {
		// back away from character
		if(cg_thirdPersonCamera.value >= 2){
			CG_OffsetThirdPersonView2();
		}else{
			CG_OffsetThirdPersonView();
		}
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();
	}

#if EARTHQUAKE_SYSTEM	// JUHOX: add earthquakes
	{
		int i;

		for (i = 0; i < MAX_EARTHQUAKES; i++) {
			earthquake_t* quake;

			quake = &cg.earthquakes[i];
			if (!quake->startTime) continue;

			AddEarthquakeTremble(quake);
		}
		AddEarthquakeTremble(NULL);	// additional tremble
	}
#endif

	// position eye reletive to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	// JUHOX: offset vieworg for lens flare editor fine move mode
#if MAPLENSFLARES
	/*
	if (
		cgs.editMode == EM_mlf &&
		cg.lfEditor.selectedLFEnt &&
		cg.lfEditor.editMode > LFEEM_none &&
		cg.lfEditor.selectedLFEnt->lock
	) {
		VectorAdd(cg.lfEditor.selectedLFEnt->lock->lerpOrigin, cg.lfEditorMoverOffset, cg.refdef.vieworg);
	}
	*/
	if (
		cgs.editMode == EM_mlf &&
		cg.lfEditor.selectedLFEnt &&
		cg.lfEditor.editMode > LFEEM_none &&
		cg.lfEditor.moveMode == LFEMM_fine
	) {
		vec3_t cursor;

		CG_LFEntOrigin(cg.lfEditor.selectedLFEnt, cursor);
		if (cg.lfEditor.editMode == LFEEM_pos) {
			VectorAdd(cg.refdef.vieworg, cg.lfEditor.fmm_offset, cursor);
			CG_SetLFEntOrigin(cg.lfEditor.selectedLFEnt, cursor);
		}
		VectorMA(cursor, -cg.lfEditor.fmm_distance, cg.refdef.viewaxis[0], cg.refdef.vieworg);
	}
#endif

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int		i;
	int		t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

/*
=====================
CG_AddBufferedSound
=====================
*/
void CG_AddBufferedSound( sfxHandle_t sfx ) {
	if ( !sfx )
		return;
	cg.soundBuffer[cg.soundBufferIn] = sfx;
	cg.soundBufferIn = (cg.soundBufferIn + 1) % MAX_SOUNDBUFFER;
	if (cg.soundBufferIn == cg.soundBufferOut) {
		cg.soundBufferOut++;
	}
}

/*
=====================
CG_PlayBufferedSounds
=====================
*/
static void CG_PlayBufferedSounds( void ) {
	if ( cg.soundTime < cg.time ) {
		if (cg.soundBufferOut != cg.soundBufferIn && cg.soundBuffer[cg.soundBufferOut]) {
			trap_S_StartLocalSound(cg.soundBuffer[cg.soundBufferOut], CHAN_ANNOUNCER);
			cg.soundBuffer[cg.soundBufferOut] = 0;
			cg.soundBufferOut = (cg.soundBufferOut + 1) % MAX_SOUNDBUFFER;
			cg.soundTime = cg.time + 750;
		}
	}
}


/*
===============
JUHOX: CG_DrawMapLensFlare
===============
*/
#if MAPLENSFLARES
static void CG_DrawMapLensFlare(
	const lensFlare_t* lf,
	float distance,
	vec3_t center, vec3_t dir, vec3_t angles,
	float alpha, float visibleLight
) {
	refEntity_t ent;
	float radius;

	memset(&ent, 0, sizeof(ent));

	radius = lf->size;

	switch (lf->mode) {
	case LFM_reflexion:
		alpha *= 0.2 * lf->rgba[3];

		radius *= cg.refdef.fov_x / 90;	// lens flares do not change size through zooming
		//alpha /= radius;
		break;
	case LFM_glare:
		alpha *= 0.14 * lf->rgba[3];
		radius *= visibleLight * 1000000.0 / Square(distance);
		break;
	case LFM_star:
		/*
		alpha *= lf->rgba[3];
		radius *= 40000.0 / (distance * sqrt(distance) * sqrt(sqrt(sqrt(distance))));

		radius *= cg.refdef.fov_x / 90;	// lens flares do not change size through zooming
		alpha /= radius;
		*/
		alpha *= lf->rgba[3];
		radius *= visibleLight * 40000.0 / (distance * sqrt(distance) * sqrt(sqrt(sqrt(distance))));
		break;
	}

	alpha *= visibleLight;
	if (alpha > 255) alpha = 255;

	ent.reType = RT_SPRITE;
	ent.customShader = lf->shader;
	ent.shaderRGBA[0] = lf->rgba[0];
	ent.shaderRGBA[1] = lf->rgba[1];
	ent.shaderRGBA[2] = lf->rgba[2];
	ent.shaderRGBA[3] = alpha;
	ent.radius = radius;

	ent.rotation =
		lf->rotationOffset +
		lf->rotationYawFactor * angles[YAW] +
		lf->rotationPitchFactor * angles[PITCH] +
		lf->rotationRollFactor * angles[ROLL];

	VectorMA(center, lf->pos, dir, ent.origin);
	trap_R_AddRefEntityToScene(&ent);
}
#endif

/*
=====================
JUHOX: CG_AddLensFlareMarker
=====================
*/
#if MAPLENSFLARES
static void CG_AddLensFlareMarker(int lfe) {
	const lensFlareEntity_t* lfent;
	float radius;
	refEntity_t ent;
	vec3_t origin;

	lfent = &cgs.lensFlareEntities[lfe];
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_MODEL;
	ent.hModel = trap_R_RegisterModel("models/powerups/plHealth/small_sphere.md3");
	ent.customShader = trap_R_RegisterShader("lfeditorcursor");
	radius = lfent->radius;
	ent.shaderRGBA[0] = 0x00;
	ent.shaderRGBA[1] = 0x80;
	ent.shaderRGBA[2] = 0x00;
	if (lfent->angle >= 0) {
		ent.shaderRGBA[0] = 0x00;
		ent.shaderRGBA[1] = 0x00;
		ent.shaderRGBA[2] = 0x80;
	}
	if (
		!cg.lfEditor.selectedLFEnt &&
		lfe == cg.lfEditor.markedLFEnt
	) {
		int c;

		c = 0x40 * (1 + sin(0.01 * cg.time));
		ent.shaderRGBA[0] += c;
		ent.shaderRGBA[1] += c;
		ent.shaderRGBA[2] += c;
	}
	else if (cg.lfEditor.selectedLFEnt == lfent) {
		ent.shaderRGBA[0] = 0xff;
		ent.shaderRGBA[1] >>= 1;
		ent.shaderRGBA[2] >>= 1;
		if (cg.lfEditor.editMode == LFEEM_radius) {
			radius = cg.lfEditor.selectedLFEnt->lightRadius;
		}
		else if (cg.lfEditor.cursorSize == LFECS_small) {
			radius = 2;
		}
		else if (cg.lfEditor.cursorSize == LFECS_lightRadius) {
			radius = cg.lfEditor.selectedLFEnt->lightRadius;
		}
		else {
			radius = cg.lfEditor.selectedLFEnt->radius;
		}
	}
	CG_LFEntOrigin(lfent, origin);
	VectorCopy(origin, ent.origin);

	ent.origin[2] -= 0.5 * radius;

	ent.axis[0][0] = 0.1 * radius;
	ent.axis[1][1] = 0.1 * radius;
	ent.axis[2][2] = 0.1 * radius;
	ent.nonNormalizedAxes = qtrue;
	trap_R_AddRefEntityToScene(&ent);

	if (lfent->angle >= 0) {
		float len;
		vec3_t end;

		len = 2 * lfent->radius + 10;
		VectorMA(origin, len, lfent->dir, end);
		CG_Draw3DLine(origin, end, trap_R_RegisterShader("dischargeFlash"));

		if (lfent->angle < 70) {
			float size;
			vec3_t right, up;
			vec3_t p1, p2;

			size = len * tan(DEG2RAD(lfent->angle));
			MakeNormalVectors(lfent->dir, right, up);

			VectorMA(end, size, right, p1);
			VectorMA(end, -size, right, p2);
			CG_Draw3DLine(p1, p2, trap_R_RegisterShader("dischargeFlash"));

			VectorMA(end, size, up, p1);
			VectorMA(end, -size, up, p2);
			CG_Draw3DLine(p1, p2, trap_R_RegisterShader("dischargeFlash"));
		}
	}
}
#endif

/*
=====================
JUHOX: CG_IsLFVisible
=====================
*/
#if MAPLENSFLARES
static qboolean CG_IsLFVisible(const vec3_t origin, const vec3_t pos, float lfradius) {
	trace_t trace;

	CG_SmoothTrace(&trace, cg.refdef.vieworg, NULL, NULL, pos, cg.snap->ps.clientNum, MASK_OPAQUE|CONTENTS_BODY);
	//return (1.0 - trace.fraction) * distance <= lfradius;
	return Distance(trace.endpos, origin) <= lfradius;
}
#endif

/*
=====================
JUHOX: CG_ComputeVisibleLightSample
=====================
*/
#if MAPLENSFLARES
#define NUMVISSAMPLES 50
static float CG_ComputeVisibleLightSample(
	lensFlareEntity_t* lfent,
	const vec3_t origin,		// redundant, but we have this already
	float distance,				// ditto
	vec3_t visOrigin,
	int quality
) {
	/*
	static const float angleTab[48] = {
		0, 45, 90, 135, 180, 225, 270, 315,
		7.5, 15, 22.5, 30, 37.5, 52.5, 60, 67.5,
		75, 82.5, 97.5, 105, 112.5, 120, 127.5, 142.5,
		150, 157.5, 165, 172.5, 187.5, 195, 202.5, 210,
		217.5, 232.5, 240, 247.5, 255, 262.5, 277.5, 285,
		292.5, 300, 307.5, 322.5, 330, 337.5, 345, 352.5
	};
	*/
	vec3_t vx, vy;
	int visCount;
	int i;

	if (lfent->lightRadius <= 1 || quality < 2) {
		VectorCopy(origin, visOrigin);
		return CG_IsLFVisible(origin, origin, lfent->radius);
	}

	visCount = 0;
	for (i = 0; i < 8; i++) {
		vec3_t corner;

		VectorCopy(origin, corner);
		corner[0] += i&1? lfent->lightRadius : -lfent->lightRadius;
		corner[1] += i&2? lfent->lightRadius : -lfent->lightRadius;
		corner[2] += i&4? lfent->lightRadius : -lfent->lightRadius;
		if (!CG_IsLFVisible(origin, corner, 1.8 * lfent->radius)) continue;	// 1.8 = rough approx. of sqrt(3)
		visCount++;
	}
	if (visCount == 0) {
		VectorClear(visOrigin);
		return 0;
	}
	else if (visCount == 8) {
		VectorCopy(origin, visOrigin);
		return 1;
	}

	{
		vec3_t vz;

		VectorSubtract(origin, cg.refdef.vieworg, vz);
		VectorNormalize(vz);
		CrossProduct(vz, axisDefault[2], vx);
		VectorNormalize(vx);
		CrossProduct(vz, vx, vy);
		// NOTE: the handedness of (vx, vy, vz) is not important
	}
	
	visCount = 0;
	VectorClear(visOrigin);
	//offset = 45 * random();
	for (i = 0; i < NUMVISSAMPLES; i++) {
		vec3_t end;

		VectorCopy(origin, end);
		{
			float angle;
			float radius;
			float x, y;

			/*
			if (i == 8) {
				if (visCount <= 0) return 0;
				if (visCount >= 8) {
					VectorCopy(origin, visOrigin);
					return 1;
				}
			}
			*/
			/*
			angle = (M_PI/180) * (angleTab[i] + offset);
			x = 0.95 * lfent->lightRadius * cos(angle);
			y = 0.95 * lfent->lightRadius * sin(angle);
			*/
			angle = (2*M_PI) * /*random()*/i / (float)NUMVISSAMPLES;
			radius = 0.95 * lfent->lightRadius * sqrt(random());

			x = radius * cos(angle);
			y = radius * sin(angle);

			VectorMA(end, x, vx, end);
			VectorMA(end, y, vy, end);
		}

		if (!CG_IsLFVisible(origin, end, lfent->radius)) continue;

		VectorAdd(visOrigin, end, visOrigin);
		visCount++;
	}

	if (visCount > 0) {
		_VectorScale(visOrigin, 1.0 / visCount, visOrigin);
	}

	return (float)visCount / (float)NUMVISSAMPLES;
}
#endif

/*
=====================
JUHOX: CG_SetVisibleLightSample
=====================
*/
#if MAPLENSFLARES
static void CG_SetVisibleLightSample(lensFlareEntity_t* lfent, float visibleLight, const vec3_t visibleOrigin) {
	vec3_t vorg;

	lfent->lib[lfent->libPos].light = visibleLight;
	VectorCopy(visibleOrigin, vorg);
#if ESCAPE_MODE
	if (cgs.gametype == GT_EFH) {
		vorg[0] += cg.currentReferenceX;
		vorg[1] += cg.currentReferenceY;
		vorg[2] += cg.currentReferenceZ;
	}
#endif
	VectorCopy(vorg, lfent->lib[lfent->libPos].origin);
	lfent->libPos++;
	if (lfent->libPos >= LIGHT_INTEGRATION_BUFFER_SIZE) {
		lfent->libPos = 0;
	}
	lfent->libNumEntries++;
}
#endif

/*
=====================
JUHOX: CG_GetVisibleLight
=====================
*/
#if MAPLENSFLARES
static float CG_GetVisibleLight(lensFlareEntity_t* lfent, vec3_t visibleOrigin) {
	int maxLibEntries;
	int i;
	float visLight;
	vec3_t visOrigin;
	int numVisPoints;

	if (lfent->lightRadius < 1) {
		maxLibEntries = 1;
	}
	else if (cg.viewMovement > 1 || lfent->lock) {
		maxLibEntries = LIGHT_INTEGRATION_BUFFER_SIZE / 2;
	}
	else {
		maxLibEntries = LIGHT_INTEGRATION_BUFFER_SIZE;
	}

	if (lfent->libNumEntries > maxLibEntries) {
		lfent->libNumEntries = maxLibEntries;
	}

	visLight = 0;
	VectorClear(visOrigin);
	numVisPoints = 0;
	for (i = 1; i <= lfent->libNumEntries; i++) {
		const lightSample_t* sample;

		sample = &lfent->lib[(lfent->libPos - i) & (LIGHT_INTEGRATION_BUFFER_SIZE - 1)];
		if (sample->light > 0) {
			vec3_t sorg;

			visLight += sample->light;
			VectorCopy(sample->origin, sorg);
#if ESCAPE_MODE
			if (cgs.gametype == GT_EFH) {
				sorg[0] -= cg.currentReferenceX;
				sorg[1] -= cg.currentReferenceY;
				sorg[2] -= cg.currentReferenceZ;
			}
#endif
			VectorAdd(visOrigin, sorg, visOrigin);
			numVisPoints++;
		}
	}
	if (lfent->libNumEntries > 0) visLight /= lfent->libNumEntries;
	if (numVisPoints > 0) {
		VectorScale(visOrigin, 1.0 / numVisPoints, visibleOrigin);
	}
	else {
		VectorCopy(visOrigin, visibleOrigin);
	}
	return visLight;
}
#endif

/*
=====================
JUHOX: CG_AddLensFlare
=====================
*/
#if MAPLENSFLARES
#define SPRITE_DISTANCE 8
void CG_AddLensFlare(lensFlareEntity_t* lfent, int quality) {
	const lensFlareEffect_t* lfeff;
	vec3_t origin;
	float distanceSqr;
	float distance;
	vec3_t dir;
	vec3_t angles;
	float cosViewAngle;
	float viewAngle;
	float angleToLightSource;
	vec3_t virtualOrigin;
	vec3_t visibleOrigin;
	float visibleLight;
	float alpha;
	vec3_t center;
	int j;

	lfeff = lfent->lfeff;
	if (!lfeff) return;

	CG_LFEntOrigin(lfent, origin);

	distanceSqr = DistanceSquared(origin, cg.refdef.vieworg);
	if (lfeff->range > 0 && distanceSqr >= lfeff->rangeSqr) {
		SkipLF:
		lfent->libNumEntries = 0;
		return;
	}
	if (distanceSqr < Square(16)) goto SkipLF;

	VectorSubtract(origin, cg.refdef.vieworg, dir);

	distance = VectorNormalize(dir);
	cosViewAngle = DotProduct(dir, cg.refdef.viewaxis[0]);
	viewAngle = acos(cosViewAngle) * (180.0 / M_PI);
	if (viewAngle >= 89.99) goto SkipLF;

	// for spotlights
	angleToLightSource = acos(-DotProduct(dir, lfent->dir)) * (180.0 / M_PI);
	if (angleToLightSource > lfent->maxVisAngle) goto SkipLF;

	if (
		cg.numFramesWithoutViewMovement <= LIGHT_INTEGRATION_BUFFER_SIZE ||
		lfent->lock ||
		lfent->libNumEntries <= 0
	) {
		float vls;

		vls = CG_ComputeVisibleLightSample(lfent, origin, distance, visibleOrigin, quality);
		CG_SetVisibleLightSample(lfent, vls, visibleOrigin);
	}

	VectorCopy(origin, visibleOrigin);
	visibleLight = CG_GetVisibleLight(lfent, visibleOrigin);
	if (visibleLight <= 0) return;

	VectorSubtract(visibleOrigin, cg.refdef.vieworg, dir);
	VectorNormalize(dir);
	vectoangles(dir, angles);
	angles[YAW] = AngleSubtract(angles[YAW], cg.predictedPlayerState.viewangles[YAW]);
	angles[PITCH] = AngleSubtract(angles[PITCH], cg.predictedPlayerState.viewangles[PITCH]);

	VectorMA(cg.refdef.vieworg, SPRITE_DISTANCE / cosViewAngle, dir, virtualOrigin);
	if (lfeff->range < 0) {
		alpha = -lfeff->range / distance;
	}
	else {
		alpha = 1.0 - distance / lfeff->range;
	}

	if (viewAngle > 0.5 * cg.refdef.fov_x) {
		alpha *= 1.0 - (viewAngle - 0.5 * cg.refdef.fov_x) / (90 - 0.5 * cg.refdef.fov_x);
	}

	VectorMA(cg.refdef.vieworg, SPRITE_DISTANCE, cg.refdef.viewaxis[0], center);
	VectorSubtract(virtualOrigin, center, dir);
	
	{
		vec3_t v;

		VectorRotate(dir, cg.refdef.viewaxis, v);
		angles[ROLL] = 90.0 - atan2(v[2], v[1]) * (180.0/M_PI);
	}

	for (j = 0; j < lfeff->numLensFlares; j++) {
		float a;
		float vl;
		const lensFlare_t* lf;

		a = alpha;
		vl = visibleLight;
		lf = &lfeff->lensFlares[j];
		if (lfent->angle >= 0) {
			float innerAngle;
			
			innerAngle = lfent->angle * lf->entityAngleFactor;
			if (angleToLightSource > innerAngle) {
				float fadeAngle;

				fadeAngle = lfeff->fadeAngle * lf->fadeAngleFactor;
				if (fadeAngle < 0.1) continue;
				if (angleToLightSource >= innerAngle + fadeAngle) continue;

				vl *= 1.0 - (angleToLightSource - innerAngle) / fadeAngle;
			}
		}
		if (lf->intensityThreshold > 0) {
			float threshold;
			float intensity;

			threshold = lf->intensityThreshold;
			intensity = a * vl;
			if (intensity < threshold) continue;
			intensity -= threshold;
			if (lfeff->range >= 0) intensity /= 1 - threshold;
			a = intensity / vl;
		}
		CG_DrawMapLensFlare(lf, distance, center, dir, angles, a, vl);
	}
}
#endif

/*
=====================
JUHOX: CG_AddMapLensFlares
=====================
*/
#if MAPLENSFLARES
#define SPRITE_DISTANCE 8
static void CG_AddMapLensFlares(void) {
	int i;

	cg.viewMovement = Distance(cg.refdef.vieworg, cg.lastViewOrigin);
	if (cg.viewMovement > 0) {
		cg.numFramesWithoutViewMovement = 0;
	}
	else {
		cg.numFramesWithoutViewMovement++;
	}

	if (cgs.editMode == EM_mlf) {
		if (cg.lfEditor.drawMode == LFEDM_none) {
			if (!cg.lfEditor.selectedLFEnt && cg.lfEditor.markedLFEnt >= 0) {
				CG_AddLensFlareMarker(cg.lfEditor.markedLFEnt);
			}
			return;
		}
		if (cg.lfEditor.drawMode == LFEDM_marks) {
			int selectedLFEntNum;

			selectedLFEntNum = cg.lfEditor.selectedLFEnt - cgs.lensFlareEntities;
			for (i = 0; i < cgs.numLensFlareEntities; i++) {
				if (i == selectedLFEntNum) continue;

				CG_AddLensFlareMarker(i);
			}
			return;
		}
	}
	else if (
		!cg_lensFlare.integer ||
		(
			!cg_mapFlare.integer &&
			!cg_sunFlare.integer
		)
	) {
		return;
	}

//	if (cg.viewMode == VIEW_scanner) return;
	if (cg.clientFrame < 5) return;

	for (i = -1; i < cgs.numLensFlareEntities; i++) {
		lensFlareEntity_t* lfent;
		const lensFlareEffect_t* lfeff;
		vec3_t origin;
		int quality;
		float distanceSqr;
		float distance;
		vec3_t dir;
		vec3_t angles;
		float cosViewAngle;
		float viewAngle;
		float angleToLightSource;
		vec3_t virtualOrigin;
		vec3_t visibleOrigin;
		float visibleLight;
		float alpha;
		vec3_t center;
		int j;

		if (i < 0) {
			if (!cg_sunFlare.integer && cgs.editMode != EM_mlf) continue;

			lfent = &cgs.sunFlare;
			lfeff = lfent->lfeff;
			if (!lfeff) continue;

			VectorAdd(lfent->origin, cg.refdef.vieworg, origin);

			quality = cg_sunFlare.integer;
		}
		else {
			if (!cg_mapFlare.integer && cgs.editMode != EM_mlf) continue;

			lfent = &cgs.lensFlareEntities[i];
			lfeff = lfent->lfeff;
			if (!lfeff) continue;

			/*
			if (cgs.editMode == EM_mlf && !cg.lfEditor.selectedLFEnt && cg.lfEditor.markedLFEnt == i) {
				CG_AddLensFlareMarker(i);
			}
			*/

			CG_LFEntOrigin(lfent, origin);

			quality = cg_mapFlare.integer;
		}

		distanceSqr = DistanceSquared(origin, cg.refdef.vieworg);
		if (lfeff->range > 0 && distanceSqr >= lfeff->rangeSqr) {
			SkipLF:
			lfent->libNumEntries = 0;
			continue;
		}
		if (distanceSqr < Square(16)) goto SkipLF;

		VectorSubtract(origin, cg.refdef.vieworg, dir);

		distance = VectorNormalize(dir);
		cosViewAngle = DotProduct(dir, cg.refdef.viewaxis[0]);
		viewAngle = acos(cosViewAngle) * (180.0 / M_PI);
		if (viewAngle >= 89.99) goto SkipLF;

		// for spotlights
		angleToLightSource = acos(-DotProduct(dir, lfent->dir)) * (180.0 / M_PI);
		if (angleToLightSource > lfent->maxVisAngle) goto SkipLF;

		if (
			cg.numFramesWithoutViewMovement <= LIGHT_INTEGRATION_BUFFER_SIZE ||
			lfent->lock ||
			lfent->libNumEntries <= 0
		) {
			float vls;

			vls = CG_ComputeVisibleLightSample(lfent, origin, distance, visibleOrigin, quality);
			CG_SetVisibleLightSample(lfent, vls, visibleOrigin);
		}

		VectorCopy(origin, visibleOrigin);
		visibleLight = CG_GetVisibleLight(lfent, visibleOrigin);
		if (visibleLight <= 0) continue;

		VectorSubtract(visibleOrigin, cg.refdef.vieworg, dir);
		VectorNormalize(dir);
		vectoangles(dir, angles);
		angles[YAW] = AngleSubtract(angles[YAW], cg.predictedPlayerState.viewangles[YAW]);
		angles[PITCH] = AngleSubtract(angles[PITCH], cg.predictedPlayerState.viewangles[PITCH]);

		VectorMA(cg.refdef.vieworg, SPRITE_DISTANCE / cosViewAngle, dir, virtualOrigin);
		if (lfeff->range < 0) {
			alpha = -lfeff->range / distance;
		}
		else {
			alpha = 1.0 - distance / lfeff->range;
		}

		/*
		if (fabs(angles[YAW]) > 0.5 * cg.refdef.fov_x) {
			alpha *= 1.0 - (fabs(angles[YAW]) - 0.5 * cg.refdef.fov_x) / (90 - 0.5 * cg.refdef.fov_x);
		}
		if (fabs(angles[PITCH]) > 0.5 * cg.refdef.fov_y) {
			alpha *= 1.0 - (fabs(angles[PITCH]) - 0.5 * cg.refdef.fov_y) / (90 - 0.5 * cg.refdef.fov_y);
		}
		*/
		if (viewAngle > 0.5 * cg.refdef.fov_x) {
			alpha *= 1.0 - (viewAngle - 0.5 * cg.refdef.fov_x) / (90 - 0.5 * cg.refdef.fov_x);
		}

		VectorMA(cg.refdef.vieworg, SPRITE_DISTANCE, cg.refdef.viewaxis[0], center);
		VectorSubtract(virtualOrigin, center, dir);
		
		{
			vec3_t v;

			VectorRotate(dir, cg.refdef.viewaxis, v);
			angles[ROLL] = 90.0 - atan2(v[2], v[1]) * (180.0/M_PI);
		}

		for (j = 0; j < lfeff->numLensFlares; j++) {
			float a;
			float vl;
			const lensFlare_t* lf;

			a = alpha;
			vl = visibleLight;
			lf = &lfeff->lensFlares[j];
			if (lfent->angle >= 0) {
				float innerAngle;
				
				innerAngle = lfent->angle * lf->entityAngleFactor;
				if (angleToLightSource > innerAngle) {
					float fadeAngle;

					fadeAngle = lfeff->fadeAngle * lf->fadeAngleFactor;
					if (fadeAngle < 0.1) continue;
					if (angleToLightSource >= innerAngle + fadeAngle) continue;

					vl *= 1.0 - (angleToLightSource - innerAngle) / fadeAngle;
				}
			}
			if (lf->intensityThreshold > 0) {
				float threshold;
				float intensity;

				threshold = lf->intensityThreshold;
				intensity = a * vl;
				if (intensity < threshold) continue;
				intensity -= threshold;
				if (lfeff->range >= 0) intensity /= 1 - threshold;
				a = intensity / vl;
			}
			CG_DrawMapLensFlare(lf, distance, center, dir, angles, a, vl);
		}
	}

	VectorCopy(cg.refdef.vieworg, cg.lastViewOrigin);
}
#endif

/*
=====================
JUHOX: CG_AddLFEditorCursor
=====================
*/
#if MAPLENSFLARES
void CG_AddLFEditorCursor(void) {
	trace_t trace;
	vec3_t end;
	refEntity_t ent;

	if (cgs.editMode != EM_mlf) return;

	cg.lfEditor.markedLFEnt = -1;
	if (!cg.lfEditor.selectedLFEnt) {
		int i;
		float lowestWeight;

		lowestWeight = 10000000.0;
		for (i = 0; i < cgs.numLensFlareEntities; i++) {
			const lensFlareEntity_t* lfent;
			vec3_t origin;
			vec3_t dir;
			float distance;
			float alpha;
			float weight;

			lfent = &cgs.lensFlareEntities[i];
			if (!lfent->lfeff) continue;

			CG_LFEntOrigin(lfent, origin);
			VectorSubtract(origin, cg.refdef.vieworg, dir);
			distance = VectorNormalize(dir);
			if (distance > 2000) continue;

			alpha = acos(DotProduct(dir, cg.refdef.viewaxis[0])) * (180.0 / M_PI);
			if (alpha > 10.0) continue;

			weight = alpha * distance;
			if (weight >= lowestWeight) continue;

			/* NOTE: with this trace enabled one would not be able to select entities within solids
			CG_SmoothTrace(&trace, cg.refdef.vieworg, NULL, NULL, origin, -1, MASK_SOLID);
			if (trace.fraction < 1.0) continue;
			*/

			lowestWeight = weight;
			cg.lfEditor.markedLFEnt = i;
		}
		return;
	}

	if (cg.lfEditor.editMode == LFEEM_pos) {
		if (cg.lfEditor.moveMode == LFEMM_coarse) {
			vec3_t cursor;

			VectorMA(cg.refdef.vieworg, 10000, cg.refdef.viewaxis[0], end);
			CG_SmoothTrace(&trace, cg.refdef.vieworg, NULL, NULL, end, -1, MASK_OPAQUE|CONTENTS_BODY);
			VectorMA(trace.endpos, -1, cg.refdef.viewaxis[0], cursor);
			CG_SetLFEntOrigin(cg.lfEditor.selectedLFEnt, cursor);
		}
		// NOTE: LFEMM_fine handled in CG_CalcViewValues()
	}

	CG_AddLensFlareMarker(cg.lfEditor.selectedLFEnt - cgs.lensFlareEntities);

	{
		int i;

		for (i = 0; i < 50; i++) {
			vec3_t dir;
			float len;
			int grey;

			dir[0] = crandom();
			dir[1] = crandom();
			dir[2] = crandom();
			len = VectorNormalize(dir);
			if (len > 1 || len < 0.01) continue;

			CG_LFEntOrigin(cg.lfEditor.selectedLFEnt, end);
			VectorMA(end, cg.lfEditor.selectedLFEnt->radius, dir, end);
			CG_SmoothTrace(&trace, cg.refdef.vieworg, NULL, NULL, end, -1, MASK_OPAQUE|CONTENTS_BODY);
			if (trace.fraction < 1) continue;

			VectorSubtract(end, cg.refdef.vieworg, dir);
			VectorNormalize(dir);
			VectorMA(cg.refdef.vieworg, 8, dir, end);

			memset(&ent, 0, sizeof(ent));
			ent.reType = RT_SPRITE;
			VectorCopy(end, ent.origin);
			ent.customShader = trap_R_RegisterShader("tssgroupTemporary");
			grey = rand() & 0xff;
			ent.shaderRGBA[0] = grey;
			ent.shaderRGBA[1] = grey;
			ent.shaderRGBA[2] = grey;
			ent.shaderRGBA[3] = 0xff;
			ent.radius = 0.05;
			trap_R_AddRefEntityToScene(&ent);
		}
	}
}
#endif


//=========================================================================

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int		inwater;
	float	attenuation;
	char	var[MAX_TOKEN_CHARS];

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	// update cvars
	CG_UpdateCvars();

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation();
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue( cg.weaponSelect, cg.zoomSensitivity );

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();


	// ADDING FOR ZEQ2

	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer ||
							 (cg.snap->ps.powerLevel[plFatigue] <= 0) ||
							 (cg.snap->ps.bitFlags & isCrashed) ||
							 (cg.snap->ps.weaponstate == WEAPON_GUIDING) ||
							 (cg.snap->ps.weaponstate == WEAPON_ALTGUIDING);
/*
	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.powerLevel[current] <= 0);
*/

	// END ADDING

	// build cg.refdef
	inwater = CG_CalcViewValues();

#if EARTHQUAKE_SYSTEM
	cg.additionalTremble = 0;	// JUHOX
#endif

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_DamageBlendBlob();
	}

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_FrameHist_NextFrame();
		CG_AddPacketEntities();			// adter calcViewValues, so predicted player state is correct
		CG_AddBeamTables();
		CG_AddTrailsToScene();
		CG_AddMarks();
		CG_AddParticles ();
		CG_AddLocalEntities();
		CG_AddParticleSystems();
#if MAPLENSFLARES	// JUHOX: add map lens flares
		CG_AddMapLensFlares();
#endif
	}
	//CG_AddViewWeapon( &cg.predictedPlayerState );

	// add buffered sounds
	CG_PlayBufferedSounds();

	// play buffered voice chats
	CG_PlayBufferedVoiceChats();

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	attenuation = cg_soundAttenuation.value; // 0.0001f; // Quake 3 default was 0.0008f;

	// update audio positions
	trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater, attenuation );

	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
	}
	if (cg_timescale.value != cg_timescaleFadeEnd.value) {
		if (cg_timescale.value < cg_timescaleFadeEnd.value) {
			cg_timescale.value += cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value > cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		else {
			cg_timescale.value -= cg_timescaleFadeSpeed.value * ((float)cg.frametime) / 1000;
			if (cg_timescale.value < cg_timescaleFadeEnd.value)
				cg_timescale.value = cg_timescaleFadeEnd.value;
		}
		if (cg_timescaleFadeSpeed.value) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value));
		}
	}
	// actually issue the rendering calls
	CG_DrawActive( stereoView );
	CG_CheckMusic();

	trap_Cvar_VariableStringBuffer( "cl_paused", var, sizeof( var ) );
	cgs.clientPaused = atoi( var );

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}

}

