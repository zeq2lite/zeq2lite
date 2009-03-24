// Copyright (C) 2003-2004 RiO
//
// ui_dynamicvolume.c -- contains management code for crossfading two
//                       music tracks
//
// This unit of code was written for the Quake 3 modification 'ZEQ 2'
// (Z Enters Quake 2). Nothing in this unit may be taken and used
// without prior and explicit consent of the ZEQ2 development team.

#include "ui_local.h"

typedef struct {
	char		trackname_intro[MAX_QPATH];
	char		trackname_loop[MAX_QPATH];
	int			time_fadeIn;
	int			time_fadeOut;
	qboolean	inuse;

} ui_crossfadeElem_t;

static ui_crossfadeElem_t	ui_crossfadeQueue[2];
static int					ui_infadeStart;
static int					ui_outfadeStart;
static float				ui_currentmusicvolume;


/*
======================
UI_InitDynamicVolume
======================
Initialize the crossfade queue. The two integers
don't need initialization. They are implicitly only
used once already initialized.
*/
void UI_InitDynamicVolume( void ) {
	memset( ui_crossfadeQueue, 0, sizeof(ui_crossfadeQueue) );
	ui_currentmusicvolume = trap_Cvar_VariableValue( "s_mastermusicvolume" );
	trap_Cvar_SetValue( "s_musicvolume", ui_currentmusicvolume );
}


/*
==========================
UI_FadeInBackGroundTrack
==========================
Set a new background track and its fade times.
After the current one is faded out, this one will be loaded.
Blank tracks are possible, and will cause only the current
track to stop playing.
*/
void UI_FadeInBackGroundTrack( char *intro, char *loop, int inTime, int outTime ) {
	// Set the tracknames. Be sure to set empty names if a null pointer is passed.
	if ( intro ) {
		Q_strncpyz( ui_crossfadeQueue[1].trackname_intro, intro, sizeof(ui_crossfadeQueue[1].trackname_intro) );
	} else {
		ui_crossfadeQueue[1].trackname_intro[0] = 0;
	}
	if ( loop ) {
		Q_strncpyz( ui_crossfadeQueue[1].trackname_loop,  loop,  sizeof(ui_crossfadeQueue[1].trackname_loop ) );
	} else {
		ui_crossfadeQueue[1].trackname_loop[0] = 0;
	}

	// Set the remaining properties.
	ui_crossfadeQueue[1].time_fadeIn = inTime;
	ui_crossfadeQueue[1].time_fadeOut = outTime;
	ui_crossfadeQueue[1].inuse = qtrue;	

	// start a fade out of the current track
	ui_outfadeStart = uis.realtime;
}


/*
===========================
UI_FadeOutBackGroundTrack
===========================
Shorthand method to only fade out the current track
and not fade in a new one.
*/
void UI_FadeOutBackGroundTrack( void ) {
	// Fading in a zero soundtrack will work as a forced fade out while
	// not starting a new track.
	UI_FadeInBackGroundTrack( NULL, NULL, 0, 0 );
}


/*
==============
UI_LerpMusic
==============
Lerps the volume of the music and handles the loading / unloading
of the correct track.
*/
void UI_LerpMusic( void ) {
	float lerpVolume;
	float lerpVal;

	// Use a fade out of the current music if a new track is located
	// in the queue.
	if ( ui_crossfadeQueue[1].inuse ) {
		// Avoid division by zero!
		if ( ui_crossfadeQueue[0].time_fadeOut == 0 ) {
			lerpVal = 0.0f;
		} else {
			lerpVal = 1.0f - (float)(uis.realtime - ui_outfadeStart) / (float)ui_crossfadeQueue[0].time_fadeOut;
		}
		if ( lerpVal > 1.0f ) lerpVal = 1.0f;
		if ( lerpVal < 0.0f ) lerpVal = 0.0f;
		lerpVolume = lerpVal * trap_Cvar_VariableValue( "s_mastermusicvolume" );

		// if there is no music currently playing, the volume is already zero.
		if ( !ui_crossfadeQueue[0].inuse ) {
			lerpVolume = 0.0f;
		}

		// If the old track has faded out completely, start the fade in of the new track.
		if ( lerpVolume == 0.0f ) {
			ui_infadeStart = uis.realtime;
			memcpy(&ui_crossfadeQueue[0], &ui_crossfadeQueue[1], sizeof(ui_crossfadeElem_t) );
			ui_crossfadeQueue[1].inuse = qfalse;

			// If the new track is empty, stop the background music and
			// flag the active element as not in use, else start the new track.
			if ( !strcmp( ui_crossfadeQueue[0].trackname_intro, "" ) &&
			     !strcmp( ui_crossfadeQueue[0].trackname_loop, "" ) ) {
				trap_S_StopBackgroundTrack();
				ui_crossfadeQueue[0].inuse = qfalse;
			} else {
				trap_S_StartBackgroundTrack( ui_crossfadeQueue[0].trackname_intro, ui_crossfadeQueue[1].trackname_loop );
			}
		}

	// Use a fade in if no new track is given
	} else {
		// Avoid division by zero!
		if ( ui_crossfadeQueue[0].time_fadeIn == 0 ) {
			lerpVal = 1.0f;
		} else {
			lerpVal = (float)(uis.realtime - ui_infadeStart) / (float)ui_crossfadeQueue[0].time_fadeIn;
		}
		if ( lerpVal > 1.0f ) lerpVal = 1.0f;
		if ( lerpVal < 0.0f ) lerpVal = 0.0f;
		lerpVolume = lerpVal * trap_Cvar_VariableValue( "s_mastermusicvolume" );
	}

	// Make no unnecessary changes to the s_musicvolume cvar to avoid constant
	// writing of the desired volume to q3config.cfg.
	if ( lerpVolume != ui_currentmusicvolume ) {
		trap_Cvar_SetValue( "s_musicvolume", lerpVolume );
		ui_currentmusicvolume = lerpVolume;
	}
}
