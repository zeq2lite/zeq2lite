// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

CREDITS

=======================================================================
*/


#include "ui_local.h"


typedef struct {
	menuframework_s	menu;
} creditsmenu_t;

static creditsmenu_t	s_credits;


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
	return 0;
}


/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_CreditMenu_Draw( void ) {
	int		y;

	y = 12;
	UI_DrawProportionalString( 320, y, "ZEQ II Lite Team is:", UI_CENTER|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Programming", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "RiO, Kaiopunk, Brad, MDave", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Animations", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "MDave", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Modellers", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Blaize, MDave, Super Vegetto, RiO, DarkLink", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Mappers", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Element X, Brad, MDave, RiO, Rammek, NJ", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Skinners", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "MDave, Super Vegetto", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Sounds", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Wolf, Son Davie, Circlerun", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Music", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Circlerun", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Retired Members", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "Eclipse, IrishBlood, Bruno", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( 320, y, "An@kin, Mystic Mike, Tenken", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );

	y += 1.35 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawString( 320, y, "A Dragon Ball Z Quake III Arena Total Conversion.  www.zeq2.com/lite", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
	y += SMALLCHAR_HEIGHT;
	UI_DrawString( 320, y, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, color_white );
}


/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( void ) {

	trap_S_StopBackgroundTrack();
	trap_S_StartBackgroundTrack("music/yamamoto/credits02.ogg", "music/yamamoto/credits02.ogg");
	trap_S_StartLocalSound( menu_exit_sound, CHAN_LOCAL_SOUND );

	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );
}
