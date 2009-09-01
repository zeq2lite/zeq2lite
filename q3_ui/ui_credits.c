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
	int		x,y;

	y = 105;
	x = 30;

	UI_DrawProportionalString( x, y, "Design/Management", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Zeth, MDave", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Programming", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "RiO, Zeth, MDave", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "2D/3D Animation", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "MDave", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "2D Artists", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Super Vegetto, MDave", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "3D Artists", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Blaize, MDave, Super Vegetto, DarkLink", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Mappers", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "MDave, Zeth", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );

	y += 1.42 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Sounds/Music", UI_LEFT|UI_SMALLFONT|UI_DROPSHADOW, color_lightBlue );
	y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
	UI_DrawProportionalString( x, y, "Circlerun", UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, color_white );}


/*===============
UI_CreditMenu
===============*/
void UI_CreditMenu( void ) {

	uis.menuamount = 0;
	trap_S_StopBackgroundTrack();
	trap_S_StartBackgroundTrack("music/credits.ogg", "music/credits.ogg");
	trap_S_StartLocalSound( menu_exit_sound, CHAN_LOCAL_SOUND );

	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = UI_CreditMenu_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );
}
