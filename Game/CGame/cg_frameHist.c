#include "cg_local.h"

typedef struct {
	qboolean	inPVS;
	qboolean	hasAura;
	int			weaponState;
	int			weapNr;
} cg_frameHist_t;

static cg_frameHist_t	frHist_thisFrame[MAX_GENTITIES];
static cg_frameHist_t	frHist_lastFrame[MAX_GENTITIES];



void CG_FrameHist_Init( void ) {
	memset( frHist_thisFrame, 0, sizeof(frHist_thisFrame));
	memset( frHist_lastFrame, 0, sizeof(frHist_lastFrame));
}

void CG_FrameHist_NextFrame( void ) {
	int i;

	for( i = 0 ; i < MAX_GENTITIES ; i++) {
		frHist_lastFrame[i].inPVS = frHist_thisFrame[i].inPVS;
		frHist_thisFrame[i].inPVS = qfalse;

		frHist_lastFrame[i].hasAura = frHist_thisFrame[i].hasAura;
		frHist_thisFrame[i].hasAura = qfalse;

		
		// NOTE: Thes last ones update automatically, so there is no 'Set' function
		//       associated with them.

		frHist_lastFrame[i].weaponState = frHist_thisFrame[i].weaponState;
		if ( i < MAX_CLIENTS ) {
			frHist_thisFrame[i].weaponState = cg_entities[i].currentState.weaponstate;
		} else {
			frHist_thisFrame[i].weaponState = WEAPON_READY;
		}


		frHist_lastFrame[i].weapNr = frHist_thisFrame[i].weapNr;
		frHist_thisFrame[i].weapNr = cg_entities[i].currentState.weapon;
	}
}



void CG_FrameHist_SetPVS( int num ) {
	frHist_thisFrame[num].inPVS = qtrue;
}

qboolean CG_FrameHist_IsInPVS( int num ) {
	return frHist_thisFrame[num].inPVS;
}

qboolean CG_FrameHist_WasInPVS( int num ) {
	return frHist_lastFrame[num].inPVS;
}



void CG_FrameHist_SetAura( int num ) {
	frHist_thisFrame[num].hasAura = qtrue;
}

qboolean CG_FrameHist_HasAura( int num ) {
	return frHist_thisFrame[num].hasAura;
}

qboolean CG_FrameHist_HadAura( int num ) {
	return frHist_lastFrame[num].hasAura;
}



int CG_FrameHist_IsWeaponState( int num ) {
	return frHist_thisFrame[num].weaponState;
}

int CG_FrameHist_WasWeaponState( int num ) {
	return frHist_lastFrame[num].weaponState;
}



int CG_FrameHist_IsWeaponNr( int num ) {
	return frHist_thisFrame[num].weapNr;
}

int CG_FrameHist_WasWeaponNr( int num ) {
	return frHist_lastFrame[num].weapNr;
}
