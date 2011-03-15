#include "cg_local.h"

#define RADAR_RANGE		16000
#define RADAR_BLIPSIZE	  24
#define RADAR_MIDSIZE	  16

radar_t				cg_playerOrigins[MAX_CLIENTS];
static qboolean		cg_radarWarningAlready;


void CG_InitRadarBlips( void ) {

	cg_radarWarningAlready = qfalse;
	memset( cg_playerOrigins, 0, sizeof(cg_playerOrigins) );
}


static void CG_DrawRadarBlips( float x, float y, float w, float h ) {
	playerState_t	*ps;
	qboolean		warning;
	int				i;
	vec3_t			projection, temp;
	vec3_t			up = { 0.0f, 0.0f, 1.0f };
	vec4_t			draw_color;
	vec4_t			drawfull_color;
	vec4_t			drawteam_color;

	float			center_x;
	float			center_y;

	float			blip_x;
	float			blip_y;
	float			blip_w;
	float			blip_h;

	float			powerLevel;
	float			powerLevel2;
	float			powerLevelAverage;
	float			powerLevelMaximum;
	float			powerLevelMaximum2;
	float			powerLevelMaximumAverage;

	float			difference, differenceCurrent, differenceMaximum;

	ps = &cg.predictedPlayerState;
	warning = qfalse;

	center_x = x + 0.5f * w;
	center_y = y + 0.5f * h;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		
		if ( !cg_playerOrigins[i].valid ) {
			continue;
		}
		if ( cg_playerOrigins[i].clientNum == cg.snap->ps.clientNum ) {
			continue;
		}

		// Calculate and check range
		VectorSubtract( cg_playerOrigins[i].pos, ps->origin, projection );
		if ( VectorLength( projection ) > RADAR_RANGE ) {
			continue;
		}

		// Rotate so that north of radar is direction we're facing (in world coordinates!)
		RotatePointAroundVector(temp, up, projection, 90 - ps->viewangles[YAW] );
		VectorCopy( temp, projection );

		blip_x = ( projection[0] / RADAR_RANGE) * 0.5f * w + center_x;
		blip_y = (-projection[1] / RADAR_RANGE) * 0.5f * h + center_y;

		blip_w = ( projection[2] / RADAR_RANGE) * 0.5f * RADAR_BLIPSIZE + RADAR_BLIPSIZE;
		blip_h = blip_w;

		powerLevel = cg.snap->ps.powerLevel[plCurrent];
		powerLevel2 = cg_playerOrigins[i].pl;
		powerLevelMaximum = cg.snap->ps.powerLevel[plMaximum];
		powerLevelMaximum2 = cg_playerOrigins[i].plMax;

		differenceCurrent = 1.0f - (powerLevel / powerLevel2);
		differenceMaximum = 1.0f - (powerLevelMaximum / powerLevelMaximum2);

		if(differenceCurrent > 1.0f){differenceCurrent = 1.0f;}
		if(differenceCurrent < 0.0f){differenceCurrent = 0.0f;}
		if(differenceMaximum > 1.0f){differenceMaximum = 1.0f;}
		if(differenceMaximum < 0.0f){differenceMaximum = 0.0f;}

		difference = differenceCurrent/* + differenceMaximum*/;

		if(difference > 1.0f){difference = 1.0f;}
		if(difference < 0.0f){difference = 0.0f;}

		//CG_Printf("Difference: %f\n",difference);

		// Set the blip color with respect to team.
		// The brighter the color, the higher the power level is compared to your own.
		// The blip fades down as the player's current power level gets lower then their maximum.
		// Should plHealth also effect the fade?
		if ( cg_playerOrigins[i].team == cg.snap->ps.persistant[PERS_TEAM] && cg_playerOrigins[i].team != TEAM_FREE ) {
			MAKERGBA( draw_color, 0.0f, difference, 0.0f, powerLevel2 / powerLevelMaximum2 );
			MAKERGBA( drawteam_color, 0.0f, 1.0f, 0.0f, powerLevel2 / powerLevelMaximum2 );
			MAKERGBA( drawfull_color, 0.0f, 1.0f, 0.0f, 1.0f );
		} else {
			MAKERGBA( draw_color, difference, 0.0f, 0.0f, powerLevel2 / powerLevelMaximum2 );
			MAKERGBA( drawfull_color, 1.0f, 0.0f, 0.0f, 1.0f );
		}

		// Draw the blip and possible warnings and bursts
		trap_R_SetColor( draw_color );
		CG_DrawPic(qfalse, blip_x - 0.5f * blip_w, blip_y - 0.5f * blip_h, blip_w, blip_h, cgs.media.RadarBlipShader );

		trap_R_SetColor( drawfull_color );		
		if ( cg_playerOrigins[i].properties & RADAR_BURST ) {
			CG_DrawPic(qfalse, blip_x - 0.5f * blip_w, blip_y - 0.5f * blip_h, blip_w, blip_h, cgs.media.RadarBurstShader );
		}

		if ( cg_playerOrigins[i].properties & RADAR_WARN ) {
			CG_DrawPic(qfalse, blip_x - 0.5f * blip_w, blip_y - 0.5f * blip_h, blip_w, blip_h, cgs.media.RadarWarningShader );
			// Atleast one warning was on the radar this screen.
			// NOTE: Used to check if a warning sound needs to be issued.
			warning = qtrue;
		}

		// Draw the team blip
		if ( cg_playerOrigins[i].team == cg.snap->ps.persistant[PERS_TEAM] && cg_playerOrigins[i].team != TEAM_FREE ) {
			trap_R_SetColor( drawteam_color );
			CG_DrawPic(qfalse, blip_x - 0.5f * blip_w, blip_y - 0.5f * blip_h, blip_w, blip_h, cgs.media.RadarBlipTeamShader );
		}

		trap_R_SetColor( NULL );
	}

	// Draw the middle point last (on top of everything else, for clarity).
	CG_DrawPic(qfalse, center_x - RADAR_MIDSIZE * 0.5f, center_y - RADAR_MIDSIZE * 0.5f, RADAR_MIDSIZE, RADAR_MIDSIZE, cgs.media.RadarMidpointShader );

	// Handle the warning sound.
	if ( warning && !cg_radarWarningAlready ) {
		cg_radarWarningAlready = qtrue;
		trap_S_StartLocalSound( cgs.media.radarwarningSound, CHAN_LOCAL_SOUND );
	}
	if ( !warning ) {
		cg_radarWarningAlready = qfalse;
	}

}

void CG_DrawRadar () {
	playerState_t	*ps;
	ps = &cg.predictedPlayerState;
	if(!(ps->lockedTarget>0)){
		CG_DrawRadarBlips( 512, 0, 128, 128 );
	}
}
