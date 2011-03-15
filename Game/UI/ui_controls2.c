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
/*
=======================================================================

CONTROLS MENU

=======================================================================
*/


#include "ui_local.h"

#define ART_BACK0			"interface/art/back_0"
#define ART_BACK1			"interface/art/back_1"
#define ART_FRAMEL			"interface/art/frame2_l"
#define ART_FRAMER			"interface/art/frame1_r"


typedef struct {
	char	*command;
	char	*label;
	int		id;
	int		anim;
	int		defaultbind1;
	int		defaultbind2;
	int		bind1;
	int		bind2;
} bind_t;

typedef struct
{
	char*	name;
	float	defaultvalue;
	float	value;	
} configcvar_t;

#define SAVE_NOOP		0
#define SAVE_YES		1
#define SAVE_NO			2
#define SAVE_CANCEL		3

#define ID_DEFAULTS		100
#define ID_SYSTEM		101
#define ID_CONTROLS		102
#define ID_GENERAL		103
#define ID_BACK			104
#define ID_SAVEANDEXIT	105
#define ID_EXIT			106

// bindable actions
#define ID_SPEED		0
#define ID_FORWARD		1	
#define ID_BACKPEDAL	2
#define ID_MOVELEFT		3
#define ID_MOVERIGHT	4
#define ID_MOVEUP		5	
#define ID_MOVEDOWN		6
#define ID_LEFT			7	
#define ID_RIGHT		8
#define ID_ATTACK		9
#define ID_WEAPPREV		10
#define ID_WEAPNEXT		11
#define ID_GESTURE		12
#define ID_CHAT			13

// zeq2 specific

#define ID_ATTACK2		14
#define ID_ROLLLEFT		15
#define ID_ROLLRIGHT	16
#define ID_BOOST		17
#define ID_CHARGEPL		18
#define ID_BLOCK		19
#define ID_ZANZOKEN		20
#define ID_JUMP			21

// all others
//#define ID_INVERTMOUSE	22
#define ID_MOUSESPEED	22
//#define ID_SMOOTHMOUSE	24

#define UI_ANIM_IDLE		0
#define UI_ANIM_RUN		1
#define UI_ANIM_WALK		2
#define UI_ANIM_BACK		3
#define UI_ANIM_JUMP		4
#define UI_ANIM_CROUCH		5
#define UI_ANIM_STEPLEFT	6
#define UI_ANIM_STEPRIGHT	7
#define UI_ANIM_TURNLEFT	8
#define UI_ANIM_TURNRIGHT	9
#define UI_ANIM_ATTACK		10
#define UI_ANIM_LOCKON		11
#define UI_ANIM_DIE		12
#define UI_ANIM_CHAT		13
// Added for ZEQ2
#define UI_ANIM_DASH_LEFT	14
#define UI_ANIM_DASH_RIGHT	15
#define UI_ANIM_DASH_FORWARD	16
#define UI_ANIM_DASH_BACKWARD	17
#define UI_ANIM_KI_CHARGE	18
#define UI_ANIM_PL_UP		19
#define UI_ANIM_PL_DOWN	20
#define UI_ANIM_FLY_IDLE	21
#define UI_ANIM_FLY_FORWARD	22
#define UI_ANIM_FLY_BACKWARD	23
#define UI_ANIM_FLY_UP		24
#define UI_ANIM_FLY_DOWN	25
#define UI_ANIM_BLOCK		26
// End adding

typedef struct
{
	menuframework_s		menu;

	menubitmap_s		framel;
	menubitmap_s		framer;
	menubitmap_s		player;

	menutext_s			system;
	menutext_s			controls;
	menutext_s			general;	

	menuaction_s		walkforward;
	menuaction_s		backpedal;
	menuaction_s		stepleft;
	menuaction_s		stepright;
	menuaction_s		moveup;
	menuaction_s		movedown;
	menuaction_s		turnleft;
	menuaction_s		turnright;
	menuaction_s		run;
	menuaction_s		machinegun;
	menuaction_s		chainsaw;
	menuaction_s		shotgun;
	menuaction_s		grenadelauncher;
	menuaction_s		rocketlauncher;
	menuaction_s		lightning;
	menuaction_s		attack;
	menuaction_s		prevweapon;
	menuaction_s		nextweapon;
	menuaction_s		gesture;
//	menuradiobutton_s	invertmouse;
	menuslider_s		sensitivity;
//	menuradiobutton_s	smoothmouse;
	playerInfo_t		playerinfo;
	qboolean			changesmade;
	menuaction_s		chat;
	int					section;
	qboolean			waitingforkey;
	char				playerModel[64];
	vec3_t				playerViewangles;
	vec3_t				playerMoveangles;
	int					playerLegs;
	int					playerTorso;
	int					playerWeapon;
	qboolean			playerChat;

	// Added for ZEQ2
	menuaction_s		attack2;
	menuaction_s		jump;
	menuaction_s		zanzoken;
	menuaction_s		boost;
	menuaction_s		rollleft;
	menuaction_s		rollright;
	menuaction_s		chargepl;
	menuaction_s		block;
	// End added

	menubitmap_s		back;
	menutext_s			name;
} controls_t; 	

static controls_t s_controls;

static vec4_t controls_binding_color  = {0.75f, 0.75f, 1.00f, 1.00f}; // bk: Win32 C4305

static bind_t g_bindings[] = 
{
	{"+speed", 			"dash / walk",		ID_SPEED,		UI_ANIM_WALK,			K_SHIFT,		-1,		-1,	-1},
	{"+forward", 		"move forward",		ID_FORWARD,		UI_ANIM_DASH_FORWARD,	K_UPARROW,		-1,		-1, -1},
	{"+back", 			"move backward",	ID_BACKPEDAL,	UI_ANIM_DASH_BACKWARD,	K_DOWNARROW,	-1,		-1, -1},
	{"+moveleft", 		"move left",		ID_MOVELEFT,	UI_ANIM_DASH_LEFT,		',',			-1,		-1, -1},
	{"+moveright", 		"move right",		ID_MOVERIGHT,	UI_ANIM_DASH_RIGHT,	'.',			-1,		-1, -1},
	{"+moveup",			"fly up",			ID_MOVEUP,		UI_ANIM_FLY_UP,		K_SPACE,		-1,		-1, -1},
	{"+movedown",		"fly down",			ID_MOVEDOWN,	UI_ANIM_FLY_DOWN,		'c',			-1,		-1, -1},
	{"+left", 			"turn left",		ID_LEFT,		UI_ANIM_TURNLEFT,		K_LEFTARROW,	-1,		-1, -1},
	{"+right", 			"turn right",		ID_RIGHT,		UI_ANIM_TURNRIGHT,		K_RIGHTARROW,	-1,		-1, -1},
	{"+attack", 		"attack primary",	ID_ATTACK,		UI_ANIM_ATTACK,		K_CTRL,			-1,		-1, -1},
	{"weapprev",		"prev weapon",		ID_WEAPPREV,	UI_ANIM_IDLE,			'[',			-1,		-1, -1},
	{"weapnext", 		"next weapon",		ID_WEAPNEXT,	UI_ANIM_IDLE,			']',			-1,		-1, -1},
	{"+button3", 		"lock on",			ID_GESTURE,		UI_ANIM_LOCKON,		K_MOUSE3,		-1,		-1, -1},
	{"messagemode", 	"chat",				ID_CHAT,		UI_ANIM_CHAT,			't',			-1,		-1, -1},
	{"+button10",		"attack secondary",	ID_ATTACK2,		UI_ANIM_ATTACK,		K_MOUSE2,		-1,		-1, -1},
	{"+button5",		"roll left",		ID_ROLLLEFT,	UI_ANIM_IDLE,			-1,				-1,		-1, -1},
	{"+button6",		"roll right",		ID_ROLLRIGHT,	UI_ANIM_IDLE,			-1,				-1,		-1, -1},
	{"+button7",		"ki boost",			ID_BOOST,		UI_ANIM_KI_CHARGE,		-1,				-1,		-1, -1},
	{"+button12",		"charge power",		ID_CHARGEPL,	UI_ANIM_PL_UP,			-1,				-1,		-1, -1},
	{"+button13",		"block",			ID_BLOCK,		UI_ANIM_BLOCK,			-1,				-1,		-1, -1},
	{"+button9",		"zanzoken",			ID_ZANZOKEN,	UI_ANIM_IDLE,			-1,				-1,		-1, -1},
	{"+button14",		"jump",				ID_JUMP,		UI_ANIM_JUMP,			-1,				-1,		-1, -1},
	{(char*)NULL,		(char*)NULL,		0,				0,					-1,				-1,		-1,	-1},
};

static configcvar_t g_configcvars[] =
{
	{"m_pitch",			0,					0},
	{"sensitivity",		0,					0},
	{"m_filter",		0,					0},
	{NULL,				0,					0}
};

static menucommon_s *g_main_controls[] =
{
	(menucommon_s *)&s_controls.run,            
	(menucommon_s *)&s_controls.walkforward,
	(menucommon_s *)&s_controls.backpedal,
	(menucommon_s *)&s_controls.stepleft,      
	(menucommon_s *)&s_controls.stepright,     
	(menucommon_s *)&s_controls.moveup,        
	(menucommon_s *)&s_controls.movedown,      
	(menucommon_s *)&s_controls.turnleft,      
	(menucommon_s *)&s_controls.turnright,     
	(menucommon_s *)&s_controls.jump,
	(menucommon_s *)&s_controls.zanzoken,
	(menucommon_s *)&s_controls.rollleft,
	(menucommon_s *)&s_controls.rollright,
	(menucommon_s *)&s_controls.boost,
	(menucommon_s *)&s_controls.attack,
	(menucommon_s *)&s_controls.attack2,
	(menucommon_s *)&s_controls.nextweapon,
	(menucommon_s *)&s_controls.prevweapon,
	(menucommon_s *)&s_controls.chargepl,
	(menucommon_s *)&s_controls.block,
	(menucommon_s *)&s_controls.gesture,
	(menucommon_s *)&s_controls.chat,
	NULL,
};

static menucommon_s **g_controls[] = {
	g_main_controls,
};

/*
=================
Controls_InitCvars
=================
*/
static void Controls_InitCvars( void )
{
	int				i;
	configcvar_t*	cvarptr;

	cvarptr = g_configcvars;
	for (i=0; ;i++,cvarptr++)
	{
		if (!cvarptr->name)
			break;

		// get current value
		cvarptr->value = trap_Cvar_VariableValue( cvarptr->name );

		// get default value
		trap_Cvar_Reset( cvarptr->name );
		cvarptr->defaultvalue = trap_Cvar_VariableValue( cvarptr->name );

		// restore current value
		trap_Cvar_SetValue( cvarptr->name, cvarptr->value );
	}
}

/*
=================
Controls_GetCvarDefault
=================
*/
static float Controls_GetCvarDefault( char* name )
{
	configcvar_t*	cvarptr;
	int				i;

	cvarptr = g_configcvars;
	for (i=0; ;i++,cvarptr++)
	{
		if (!cvarptr->name)
			return (0);

		if (!strcmp(cvarptr->name,name))
			break;
	}

	return (cvarptr->defaultvalue);
}

/*
=================
Controls_GetCvarValue
=================
*/
static float Controls_GetCvarValue( char* name )
{
	configcvar_t*	cvarptr;
	int				i;

	cvarptr = g_configcvars;
	for (i=0; ;i++,cvarptr++)
	{
		if (!cvarptr->name)
			return (0);

		if (!strcmp(cvarptr->name,name))
			break;
	}

	return (cvarptr->value);
}


/*
=================
Controls_UpdateModel
=================
*/
static void Controls_UpdateModel( int anim ) {
	VectorClear( s_controls.playerViewangles );
	VectorClear( s_controls.playerMoveangles );
	s_controls.playerViewangles[YAW] = 180 /*- 30*/;
	s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW];
	s_controls.playerLegs		     = ANIM_IDLE;
	s_controls.playerTorso			 = ANIM_IDLE;
	s_controls.playerWeapon			 = -1;
	s_controls.playerChat			 = qfalse;

	switch( anim ) {
	case UI_ANIM_DASH_LEFT:	
		s_controls.playerLegs = ANIM_DASH_LEFT;
		s_controls.playerTorso = ANIM_DASH_LEFT;
		break;

	case UI_ANIM_DASH_RIGHT:	
		s_controls.playerLegs = ANIM_DASH_RIGHT;
		s_controls.playerTorso = ANIM_DASH_RIGHT;
		break;

	case UI_ANIM_DASH_FORWARD:	
		s_controls.playerLegs = ANIM_DASH_FORWARD;
		s_controls.playerTorso = ANIM_DASH_FORWARD;
		break;

	case UI_ANIM_DASH_BACKWARD:	
		s_controls.playerLegs = ANIM_DASH_BACKWARD;
		s_controls.playerTorso = ANIM_DASH_BACKWARD;
		break;

	case UI_ANIM_RUN:	
		s_controls.playerLegs = ANIM_RUN;
		s_controls.playerTorso = ANIM_RUN;
		break;

	case UI_ANIM_WALK:	
		s_controls.playerLegs = ANIM_WALK;
		s_controls.playerTorso = ANIM_WALK;
		break;

	case UI_ANIM_BACK:	
		s_controls.playerLegs = ANIM_RUN;
		s_controls.playerTorso = ANIM_RUN;
		break;

	case UI_ANIM_JUMP:	
		s_controls.playerLegs = ANIM_JUMP_UP;
		s_controls.playerTorso = ANIM_JUMP_UP;
		break;

	case UI_ANIM_TURNLEFT:
		s_controls.playerViewangles[YAW] += 90;
		break;

	case UI_ANIM_TURNRIGHT:
		s_controls.playerViewangles[YAW] -= 90;
		break;

	case UI_ANIM_STEPLEFT:
		s_controls.playerLegs = ANIM_WALK;
		s_controls.playerTorso = ANIM_WALK;
		s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW] + 90;
		break;

	case UI_ANIM_STEPRIGHT:
		s_controls.playerLegs = ANIM_WALK;
		s_controls.playerTorso = ANIM_WALK;
		s_controls.playerMoveangles[YAW] = s_controls.playerViewangles[YAW] - 90;
		break;

	case UI_ANIM_ATTACK:
		s_controls.playerLegs = ANIM_KI_ATTACK1_FIRE;
		s_controls.playerTorso = ANIM_KI_ATTACK1_FIRE;
		break;

	case UI_ANIM_KI_CHARGE:
		s_controls.playerLegs = ANIM_KI_CHARGE;
		s_controls.playerTorso = ANIM_KI_CHARGE;
		break;

	case UI_ANIM_PL_UP:
		s_controls.playerLegs = ANIM_PL_UP;
		s_controls.playerTorso = ANIM_PL_UP;
		break;

	case UI_ANIM_PL_DOWN:
		s_controls.playerLegs = ANIM_PL_DOWN;
		s_controls.playerTorso = ANIM_PL_DOWN;
		break;

	case UI_ANIM_BLOCK:
		s_controls.playerLegs = ANIM_BLOCK;
		s_controls.playerTorso = ANIM_BLOCK;
		break;

	case UI_ANIM_FLY_IDLE:
		s_controls.playerLegs = ANIM_FLY_IDLE;
		s_controls.playerTorso = ANIM_FLY_IDLE;
		break;

	case UI_ANIM_FLY_FORWARD:
		s_controls.playerLegs = ANIM_FLY_FORWARD;
		s_controls.playerTorso = ANIM_FLY_FORWARD;
		break;

	case UI_ANIM_FLY_BACKWARD:
		s_controls.playerLegs = ANIM_FLY_BACKWARD;
		s_controls.playerTorso = ANIM_FLY_BACKWARD;
		break;

	case UI_ANIM_FLY_UP:
		s_controls.playerLegs = ANIM_FLY_UP;
		s_controls.playerTorso = ANIM_FLY_UP;
		break;

	case UI_ANIM_FLY_DOWN:
		s_controls.playerLegs = ANIM_FLY_DOWN;
		s_controls.playerTorso = ANIM_FLY_DOWN;
		break;

	case UI_ANIM_LOCKON:
		s_controls.playerTorso = ANIM_IDLE_LOCKED;
		s_controls.playerLegs = ANIM_IDLE_LOCKED;
		break;

	case UI_ANIM_DIE:
		s_controls.playerLegs = ANIM_DEATH_GROUND;
		s_controls.playerTorso = ANIM_DEATH_GROUND;
		s_controls.playerWeapon = WP_NONE;
		break;

	case UI_ANIM_CHAT:
		s_controls.playerChat = qtrue;
		break;

	default:
		break;
	}

	UI_PlayerInfo_SetInfo( &s_controls.playerinfo, s_controls.playerLegs, s_controls.playerTorso, s_controls.playerViewangles, s_controls.playerMoveangles, s_controls.playerWeapon, s_controls.playerChat );
}


/*
=================
Controls_Update
=================
*/
static void Controls_Update( void ) {
	int		i;
	int		j;
	int		y;
	menucommon_s	**controls;
	menucommon_s	*control;


	controls = g_controls[s_controls.section];

	// enable controls in active group (and count number of items for vertical centering)
	for( j = 0;  (control = controls[j]) ; j++ ) {
		control->flags &= ~(QMF_GRAYED|QMF_HIDDEN|QMF_INACTIVE);
	}

	// position controls
	y = ( SCREEN_HEIGHT - j * SMALLCHAR_HEIGHT ) / 2;
	for( j = 0;	(control = controls[j]) ; j++, y += SMALLCHAR_HEIGHT ) {
		control->x      = 320;
		control->y      = y;
		control->left   = 320 - 19*SMALLCHAR_WIDTH;
		control->right  = 320 + 21*SMALLCHAR_WIDTH;
		control->top    = y;
		control->bottom = y + SMALLCHAR_HEIGHT;
	}

	if( s_controls.waitingforkey ) {
		// disable everybody
		for( i = 0; i < s_controls.menu.nitems; i++ ) {
			((menucommon_s*)(s_controls.menu.items[i]))->flags |= QMF_GRAYED;
		}

		// enable action item
		((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->flags &= ~QMF_GRAYED;

		// don't gray out player's name
		s_controls.name.generic.flags &= ~QMF_GRAYED;

		return;
	}

	// enable everybody
	for( i = 0; i < s_controls.menu.nitems; i++ ) {
		((menucommon_s*)(s_controls.menu.items[i]))->flags &= ~QMF_GRAYED;
	}

}


/*
=================
Controls_DrawKeyBinding
=================
*/
static void Controls_DrawKeyBinding( void *self )
{
	menuaction_s*	a;
	int				x;
	int				y;
	int				b1;
	int				b2;
	qboolean		c;
	char			name[32];
	char			name2[32];

	a = (menuaction_s*) self;

	x =	a->generic.x;
	y = a->generic.y;

	c = (Menu_ItemAtCursor( a->generic.parent ) == a);

	b1 = g_bindings[a->generic.id].bind1;
	if (b1 == -1)
		strcpy(name,"???");
	else
	{
		trap_Key_KeynumToStringBuf( b1, name, 32 );
		Q_strupr(name);

		b2 = g_bindings[a->generic.id].bind2;
		if (b2 != -1)
		{
			trap_Key_KeynumToStringBuf( b2, name2, 32 );
			Q_strupr(name2);

			strcat( name, " or " );
			strcat( name, name2 );
		}
	}

	if (c)
	{
		UI_FillRect( a->generic.left, a->generic.top, a->generic.right-a->generic.left+1, a->generic.bottom-a->generic.top+1, listbar_color ); 

		UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_TINYFONT|UI_DROPSHADOW, text_color_highlight );
		UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_TINYFONT|UI_PULSE|UI_DROPSHADOW, text_color_highlight );

		if (s_controls.waitingforkey)
		{
			UI_DrawChar( x, y, '=', UI_CENTER|UI_BLINK|UI_TINYFONT|UI_DROPSHADOW, text_color_highlight);
			UI_DrawString(SCREEN_WIDTH * 0.80, SCREEN_HEIGHT * 0.82, "Waiting for new key ... ESCAPE to cancel", UI_TINYFONT|UI_CENTER|UI_PULSE|UI_DROPSHADOW, colorWhite );
		}
		else
		{
			UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_TINYFONT|UI_DROPSHADOW, text_color_highlight);
			UI_DrawString(SCREEN_WIDTH * 0.85, SCREEN_HEIGHT * 0.82, "Press ENTER or CLICK to change", UI_TINYFONT|UI_CENTER|UI_DROPSHADOW, colorWhite );
			UI_DrawString(SCREEN_WIDTH * 0.86, SCREEN_HEIGHT * 0.86, "Press BACKSPACE to clear", UI_TINYFONT|UI_CENTER|UI_DROPSHADOW, colorWhite );
		}
	}
	else
	{
		if (a->generic.flags & QMF_GRAYED)
		{
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_TINYFONT|UI_DROPSHADOW, text_color_disabled );
			UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, text_color_disabled );
		}
		else
		{
			UI_DrawString( x - SMALLCHAR_WIDTH, y, g_bindings[a->generic.id].label, UI_RIGHT|UI_TINYFONT|UI_DROPSHADOW, controls_binding_color );
			UI_DrawString( x + SMALLCHAR_WIDTH, y, name, UI_LEFT|UI_TINYFONT|UI_DROPSHADOW, controls_binding_color );
		}
	}
}

/*
=================
Controls_StatusBar
=================
*/
static void Controls_StatusBar( void *self )
{
	UI_DrawString(SCREEN_WIDTH * 0.85, SCREEN_HEIGHT * 0.82, "Use Arrow Keys or CLICK", UI_TINYFONT|UI_CENTER|UI_DROPSHADOW, colorWhite );
	UI_DrawString(SCREEN_WIDTH * 0.85, SCREEN_HEIGHT * 0.86, "to change it", UI_TINYFONT|UI_CENTER|UI_DROPSHADOW, colorWhite );
}


/*
=================
Controls_DrawPlayer
=================
*/
static void Controls_DrawPlayer( void *self ) {
	menubitmap_s	*b;
	char			buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );
	if ( strcmp( buf, s_controls.playerModel ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_controls.playerinfo, buf );
		strcpy( s_controls.playerModel, buf );
		Controls_UpdateModel( ANIM_IDLE );
	}

	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_controls.playerinfo, uis.realtime / PLAYER_MODEL_SPEED );
}


/*
=================
Controls_GetKeyAssignment
=================
*/
static void Controls_GetKeyAssignment (char *command, int *twokeys)
{
	int		count;
	int		j;
	char	b[256];

	twokeys[0] = twokeys[1] = -1;
	count = 0;

	for ( j = 0; j < 256; j++ )
	{
		trap_Key_GetBindingBuf( j, b, 256 );
		if ( *b == 0 ) {
			continue;
		}
		if ( !Q_stricmp( b, command ) ) {
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

/*
=================
Controls_GetConfig
=================
*/
static void Controls_GetConfig( void )
{
	int		i;
	int		twokeys[2];
	bind_t*	bindptr;

	// put the bindings into a local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)
			break;

		Controls_GetKeyAssignment(bindptr->command, twokeys);

		bindptr->bind1 = twokeys[0];
		bindptr->bind2 = twokeys[1];
	}

//	s_controls.invertmouse.curvalue  = Controls_GetCvarValue( "m_pitch" ) < 0;
//	s_controls.smoothmouse.curvalue  = UI_ClampCvar( 0, 1, Controls_GetCvarValue( "m_filter" ) );
	s_controls.sensitivity.curvalue  = UI_ClampCvar( 2, 30, Controls_GetCvarValue( "sensitivity" ) );
}

/*
=================
Controls_SetConfig
=================
*/
static void Controls_SetConfig( void )
{
	int		i;
	bind_t*	bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, get its numeric binding
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)
			break;

		if (bindptr->bind1 != -1)
		{	
			trap_Key_SetBinding( bindptr->bind1, bindptr->command );

			if (bindptr->bind2 != -1)
				trap_Key_SetBinding( bindptr->bind2, bindptr->command );
		}
	}

/*	if ( s_controls.invertmouse.curvalue )
		trap_Cvar_SetValue( "m_pitch", -fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );
	else
		trap_Cvar_SetValue( "m_pitch", fabs( trap_Cvar_VariableValue( "m_pitch" ) ) );
*/
//	trap_Cvar_SetValue( "m_filter", s_controls.smoothmouse.curvalue );
	trap_Cvar_SetValue( "sensitivity", s_controls.sensitivity.curvalue );
	trap_Cmd_ExecuteText( EXEC_APPEND, "in_restart\n" );
}

/*
=================
Controls_SetDefaults
=================
*/
static void Controls_SetDefaults( void )
{
	int	i;
	bind_t*	bindptr;

	// set the bindings from the local store
	bindptr = g_bindings;

	// iterate each command, set its default binding
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)
			break;

		bindptr->bind1 = bindptr->defaultbind1;
		bindptr->bind2 = bindptr->defaultbind2;
	}

//	s_controls.invertmouse.curvalue  = Controls_GetCvarDefault( "m_pitch" ) < 0;
//	s_controls.smoothmouse.curvalue  = Controls_GetCvarDefault( "m_filter" );
	s_controls.sensitivity.curvalue  = Controls_GetCvarDefault( "sensitivity" );
}

/*
=================
Controls_MenuKey
=================
*/
static sfxHandle_t Controls_MenuKey( int key )
{
	int			id;
	int			i;
	qboolean	found;
	bind_t*		bindptr;
	found = qfalse;

	if (!s_controls.waitingforkey)
	{
		switch (key)
		{
			case K_BACKSPACE:
			case K_DEL:
			case K_KP_DEL:
				key = -1;
				break;
		
			case K_MOUSE2:
			case K_ESCAPE:
				if (s_controls.changesmade)
					Controls_SetConfig();
				goto ignorekey;	

			default:
				goto ignorekey;
		}
	}
	else
	{
		if (key & K_CHAR_FLAG)
			goto ignorekey;

		switch (key)
		{
			case K_ESCAPE:
				s_controls.waitingforkey = qfalse;
				Controls_Update();
				return (menu_out_sound);
	
			case '`':
				goto ignorekey;
		}
	}

	s_controls.changesmade = qtrue;
	
	if (key != -1)
	{
		// remove from any other bind
		bindptr = g_bindings;
		for (i=0; ;i++,bindptr++)
		{
			if (!bindptr->label)	
				break;

			if (bindptr->bind2 == key)
				bindptr->bind2 = -1;

			if (bindptr->bind1 == key)
			{
				bindptr->bind1 = bindptr->bind2;	
				bindptr->bind2 = -1;
			}
		}
	}

	// assign key to local store
	id      = ((menucommon_s*)(s_controls.menu.items[s_controls.menu.cursor]))->id;
	bindptr = g_bindings;
	for (i=0; ;i++,bindptr++)
	{
		if (!bindptr->label)	
			break;
		
		if (bindptr->id == id)
		{
			found = qtrue;
			if (key == -1)
			{
				if( bindptr->bind1 != -1 ) {
					trap_Key_SetBinding( bindptr->bind1, "" );
					bindptr->bind1 = -1;
				}
				if( bindptr->bind2 != -1 ) {
					trap_Key_SetBinding( bindptr->bind2, "" );
					bindptr->bind2 = -1;
				}
			}
			else if (bindptr->bind1 == -1) {
				bindptr->bind1 = key;
			}
			else if (bindptr->bind1 != key && bindptr->bind2 == -1) {
				bindptr->bind2 = key;
			}
			else
			{
				trap_Key_SetBinding( bindptr->bind1, "" );
				trap_Key_SetBinding( bindptr->bind2, "" );
				bindptr->bind1 = key;
				bindptr->bind2 = -1;
			}						
			break;
		}
	}				
		
	s_controls.waitingforkey = qfalse;

	if (found)
	{	
		Controls_Update();
		return (menu_out_sound);
	}

ignorekey:
	return Menu_DefaultKey( &s_controls.menu, key );
}

/*
=================
Controls_ResetDefaults_Action
=================
*/
static void Controls_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}

	s_controls.changesmade = qtrue;
	Controls_SetDefaults();
	Controls_Update();
}

/*
=================
Controls_ResetDefaults_Draw
=================
*/
static void Controls_ResetDefaults_Draw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 0, "WARNING: This will reset all", UI_CENTER|UI_TINYFONT|UI_DROPSHADOW, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH/2, 356 + PROP_HEIGHT * 1, "controls to their default values.", UI_CENTER|UI_TINYFONT|UI_DROPSHADOW, color_yellow );
}

/*
=================
Controls_MenuEvent
=================
*/
static void Controls_MenuEvent( void* ptr, int event )
{
	switch (((menucommon_s*)ptr)->id)
	{
		case ID_SYSTEM:
			UI_SystemSettingsMenu();
			break;
			
		case ID_CONTROLS:
			UI_ControlsMenu();
			break;
			
		case ID_GENERAL:
			UI_PreferencesMenu();
			break;	

		case ID_DEFAULTS:
			if (event == QM_ACTIVATED)
			{
				UI_ConfirmMenu( "SET TO DEFAULTS?", Controls_ResetDefaults_Draw, Controls_ResetDefaults_Action );
			}
			break;			

		case ID_BACK:
			if (event == QM_ACTIVATED)
			{
				if (s_controls.changesmade)
					Controls_SetConfig();
				UI_PopMenu();
			}
			break;

		case ID_SAVEANDEXIT:
			if (event == QM_ACTIVATED)
			{
				Controls_SetConfig();
				UI_PopMenu();
			}
			break;

		case ID_EXIT:
			if (event == QM_ACTIVATED)
			{
				UI_PopMenu();
			}
			break;

//		case ID_INVERTMOUSE:
//		case ID_SMOOTHMOUSE:
		case ID_MOUSESPEED:
			if (event == QM_ACTIVATED)
			{
				s_controls.changesmade = qtrue;
			}
			break;		
	}
}

/*
=================
Controls_ActionEvent
=================
*/
static void Controls_ActionEvent( void* ptr, int event )
{
	if (event == QM_LOSTFOCUS)
	{
		Controls_UpdateModel( ANIM_IDLE );
	}
	else if (event == QM_GOTFOCUS)
	{
		Controls_UpdateModel( g_bindings[((menucommon_s*)ptr)->id].anim );
	}
	else if ((event == QM_ACTIVATED) && !s_controls.waitingforkey)
	{
		s_controls.waitingforkey = 1;
		Controls_Update();
	}
}

/*
=================
Controls_InitModel
=================
*/
static void Controls_InitModel( void )
{
	memset( &s_controls.playerinfo, 0, sizeof(playerInfo_t) );

	UI_PlayerInfo_SetModel( &s_controls.playerinfo, UI_Cvar_VariableString( "model" ) );

	Controls_UpdateModel( ANIM_IDLE );
}

/*
=================
Controls_InitWeapons
=================
*/
static void Controls_InitWeapons( void ) {
}

/*
=================
Controls_MenuInit
=================
*/
static void Controls_MenuInit( void )
{
	static char playername[32];

	// zero set all our globals	
	memset( &s_controls, 0 ,sizeof(controls_t) );

	Controls_Cache();
	
	s_controls.menu.key        = Controls_MenuKey;
	s_controls.menu.wrapAround = qtrue;
	s_controls.menu.fullscreen = qtrue;


	s_controls.system.generic.type		= MTYPE_PTEXT;
	s_controls.system.generic.flags		= QMF_LEFT_JUSTIFY;
	s_controls.system.generic.id		= ID_SYSTEM;
	s_controls.system.generic.callback	= Controls_MenuEvent;
	s_controls.system.generic.x			= 28;
	s_controls.system.generic.y			= 119;
	s_controls.system.string			= "SYSTEM";
	s_controls.system.style				= UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	s_controls.system.color				= color_white;

	s_controls.controls.generic.type		= MTYPE_PTEXT;
	s_controls.controls.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.controls.generic.id			= ID_CONTROLS;
	s_controls.controls.generic.callback	= Controls_MenuEvent;
	s_controls.controls.generic.x			= 28;
	s_controls.controls.generic.y			= 157;
	s_controls.controls.string				= "CONTROLS";
	s_controls.controls.style				= UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	s_controls.controls.color				= color_white;
	
	s_controls.general.generic.type		= MTYPE_PTEXT;
	s_controls.general.generic.flags	= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.general.generic.id		= ID_GENERAL;
	s_controls.general.generic.callback	= Controls_MenuEvent;
	s_controls.general.generic.x		= 28;
	s_controls.general.generic.y		= 195;
	s_controls.general.string			= "GENERAL";
	s_controls.general.style			= UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	s_controls.general.color			= color_white;

	s_controls.framel.generic.type  = MTYPE_BITMAP;
	s_controls.framel.generic.name  = ART_FRAMEL;
	s_controls.framel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_controls.framel.generic.x     = 0;
	s_controls.framel.generic.y     = 78;
	s_controls.framel.width  	    = 256;
	s_controls.framel.height  	    = 329;

	s_controls.framer.generic.type  = MTYPE_BITMAP;
	s_controls.framer.generic.name  = ART_FRAMER;
	s_controls.framer.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_controls.framer.generic.x     = 376;
	s_controls.framer.generic.y     = 76;
	s_controls.framer.width  	    = 256;
	s_controls.framer.height  	    = 334;


	s_controls.back.generic.type	 = MTYPE_BITMAP;
	s_controls.back.generic.name     = ART_BACK0;
	s_controls.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_controls.back.generic.x		 = 0;
	s_controls.back.generic.y		 = 480-64;
	s_controls.back.generic.id	     = ID_BACK;
	s_controls.back.generic.callback = Controls_MenuEvent;
	s_controls.back.width  		     = 128;
	s_controls.back.height  		 = 64;
	s_controls.back.focuspic         = ART_BACK1;

	s_controls.player.generic.type      = MTYPE_BITMAP;
	s_controls.player.generic.flags     = QMF_INACTIVE;
	s_controls.player.generic.ownerdraw = Controls_DrawPlayer;
	s_controls.player.generic.x	        = 400;
	s_controls.player.generic.y	        = -40;
	s_controls.player.width	            = 32*10;
	s_controls.player.height            = 56*10;

	s_controls.walkforward.generic.type	     = MTYPE_ACTION;
	s_controls.walkforward.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.walkforward.generic.callback  = Controls_ActionEvent;
	s_controls.walkforward.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.walkforward.generic.id 	     = ID_FORWARD;

	s_controls.backpedal.generic.type	   = MTYPE_ACTION;
	s_controls.backpedal.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.backpedal.generic.callback  = Controls_ActionEvent;
	s_controls.backpedal.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.backpedal.generic.id 	   = ID_BACKPEDAL;

	s_controls.stepleft.generic.type	  = MTYPE_ACTION;
	s_controls.stepleft.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepleft.generic.callback  = Controls_ActionEvent;
	s_controls.stepleft.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.stepleft.generic.id 		  = ID_MOVELEFT;

	s_controls.stepright.generic.type	   = MTYPE_ACTION;
	s_controls.stepright.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.stepright.generic.callback  = Controls_ActionEvent;
	s_controls.stepright.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.stepright.generic.id        = ID_MOVERIGHT;

	s_controls.moveup.generic.type	    = MTYPE_ACTION;
	s_controls.moveup.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.moveup.generic.callback  = Controls_ActionEvent;
	s_controls.moveup.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.moveup.generic.id        = ID_MOVEUP;

	s_controls.movedown.generic.type	  = MTYPE_ACTION;
	s_controls.movedown.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.movedown.generic.callback  = Controls_ActionEvent;
	s_controls.movedown.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.movedown.generic.id        = ID_MOVEDOWN;

	s_controls.turnleft.generic.type	  = MTYPE_ACTION;
	s_controls.turnleft.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.turnleft.generic.callback  = Controls_ActionEvent;
	s_controls.turnleft.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.turnleft.generic.id        = ID_LEFT;

	s_controls.turnright.generic.type	   = MTYPE_ACTION;
	s_controls.turnright.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.turnright.generic.callback  = Controls_ActionEvent;
	s_controls.turnright.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.turnright.generic.id        = ID_RIGHT;



	s_controls.run.generic.type	     = MTYPE_ACTION;
	s_controls.run.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.run.generic.callback  = Controls_ActionEvent;
	s_controls.run.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.run.generic.id        = ID_SPEED;

	s_controls.jump.generic.type	    = MTYPE_ACTION;
	s_controls.jump.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.jump.generic.callback	= Controls_ActionEvent;
	s_controls.jump.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.jump.generic.id			= ID_JUMP;

	s_controls.zanzoken.generic.type	    = MTYPE_ACTION;
	s_controls.zanzoken.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.zanzoken.generic.callback	= Controls_ActionEvent;
	s_controls.zanzoken.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.zanzoken.generic.id			= ID_ZANZOKEN;

	s_controls.rollleft.generic.type	    = MTYPE_ACTION;
	s_controls.rollleft.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.rollleft.generic.callback	= Controls_ActionEvent;
	s_controls.rollleft.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.rollleft.generic.id			= ID_ROLLLEFT;

	s_controls.rollright.generic.type	    = MTYPE_ACTION;
	s_controls.rollright.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.rollright.generic.callback	= Controls_ActionEvent;
	s_controls.rollright.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.rollright.generic.id			= ID_ROLLRIGHT;

	s_controls.boost.generic.type	    = MTYPE_ACTION;
	s_controls.boost.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.boost.generic.callback	= Controls_ActionEvent;
	s_controls.boost.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.boost.generic.id			= ID_BOOST;
	
	s_controls.attack.generic.type	    = MTYPE_ACTION;
	s_controls.attack.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.attack.generic.callback  = Controls_ActionEvent;
	s_controls.attack.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.attack.generic.id        = ID_ATTACK;

	s_controls.attack2.generic.type	    = MTYPE_ACTION;
	s_controls.attack2.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.attack2.generic.callback  = Controls_ActionEvent;
	s_controls.attack2.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.attack2.generic.id        = ID_ATTACK2;

	s_controls.prevweapon.generic.type	    = MTYPE_ACTION;
	s_controls.prevweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.prevweapon.generic.callback  = Controls_ActionEvent;
	s_controls.prevweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.prevweapon.generic.id        = ID_WEAPPREV;

	s_controls.nextweapon.generic.type	    = MTYPE_ACTION;
	s_controls.nextweapon.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.nextweapon.generic.callback  = Controls_ActionEvent;
	s_controls.nextweapon.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.nextweapon.generic.id        = ID_WEAPNEXT;

/*	s_controls.invertmouse.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.invertmouse.generic.flags	 = QMF_SMALLFONT;
	s_controls.invertmouse.generic.x	     = SCREEN_WIDTH/2;
	s_controls.invertmouse.generic.name	     = "invert mouse";
	s_controls.invertmouse.generic.id        = ID_INVERTMOUSE;
	s_controls.invertmouse.generic.callback  = Controls_MenuEvent;
	s_controls.invertmouse.generic.statusbar = Controls_StatusBar;

	s_controls.smoothmouse.generic.type      = MTYPE_RADIOBUTTON;
	s_controls.smoothmouse.generic.flags	 = QMF_SMALLFONT;
	s_controls.smoothmouse.generic.x	     = SCREEN_WIDTH/2;
	s_controls.smoothmouse.generic.name	     = "smooth mouse";
	s_controls.smoothmouse.generic.id        = ID_SMOOTHMOUSE;
	s_controls.smoothmouse.generic.callback  = Controls_MenuEvent;
	s_controls.smoothmouse.generic.statusbar = Controls_StatusBar;
*/

	s_controls.sensitivity.generic.type	     = MTYPE_SLIDER;
	s_controls.sensitivity.generic.x		 = SCREEN_WIDTH/2;
	s_controls.sensitivity.generic.flags	 = QMF_SMALLFONT;
	s_controls.sensitivity.generic.name	     = "mouse speed";
	s_controls.sensitivity.generic.id 	     = ID_MOUSESPEED;
	s_controls.sensitivity.generic.callback  = Controls_MenuEvent;
	s_controls.sensitivity.minvalue		     = 2;
	s_controls.sensitivity.maxvalue		     = 30;
	s_controls.sensitivity.generic.statusbar = Controls_StatusBar;

	s_controls.gesture.generic.type	     = MTYPE_ACTION;
	s_controls.gesture.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.gesture.generic.callback  = Controls_ActionEvent;
	s_controls.gesture.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.gesture.generic.id        = ID_GESTURE;

	s_controls.chargepl.generic.type	    = MTYPE_ACTION;
	s_controls.chargepl.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chargepl.generic.callback	= Controls_ActionEvent;
	s_controls.chargepl.generic.ownerdraw	= Controls_DrawKeyBinding;
	s_controls.chargepl.generic.id			= ID_CHARGEPL;

	s_controls.block.generic.type			= MTYPE_ACTION;
	s_controls.block.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.block.generic.callback		= Controls_ActionEvent;
	s_controls.block.generic.ownerdraw		= Controls_DrawKeyBinding;
	s_controls.block.generic.id				= ID_BLOCK;

	s_controls.chat.generic.type	  = MTYPE_ACTION;
	s_controls.chat.generic.flags     = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_GRAYED|QMF_HIDDEN;
	s_controls.chat.generic.callback  = Controls_ActionEvent;
	s_controls.chat.generic.ownerdraw = Controls_DrawKeyBinding;
	s_controls.chat.generic.id        = ID_CHAT;

	s_controls.name.generic.type	= MTYPE_PTEXT;
	s_controls.name.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_controls.name.generic.x		= 320;
	s_controls.name.generic.y		= 440;
	s_controls.name.string			= playername;
	s_controls.name.style			= UI_CENTER|UI_DROPSHADOW;
	s_controls.name.color			= text_color_normal;

	Menu_AddItem( &s_controls.menu, &s_controls.player );

	Menu_AddItem( &s_controls.menu, ( void * ) &s_controls.menu );
	Menu_AddItem( &s_controls.menu, ( void * ) &s_controls.menu );
	Menu_AddItem( &s_controls.menu, ( void * ) &s_controls.menu );

//	Menu_AddItem( &s_controls.menu, &s_controls.smoothmouse );
//	Menu_AddItem( &s_controls.menu, &s_controls.invertmouse );
	Menu_AddItem( &s_controls.menu, &s_controls.boost );
	Menu_AddItem( &s_controls.menu, &s_controls.zanzoken );
	Menu_AddItem( &s_controls.menu, &s_controls.walkforward );
	Menu_AddItem( &s_controls.menu, &s_controls.backpedal );
	Menu_AddItem( &s_controls.menu, &s_controls.stepleft );
	Menu_AddItem( &s_controls.menu, &s_controls.stepright );
	Menu_AddItem( &s_controls.menu, &s_controls.moveup );
	Menu_AddItem( &s_controls.menu, &s_controls.movedown );
	Menu_AddItem( &s_controls.menu, &s_controls.jump );
	Menu_AddItem( &s_controls.menu, &s_controls.turnleft );
	Menu_AddItem( &s_controls.menu, &s_controls.turnright );
	Menu_AddItem( &s_controls.menu, &s_controls.rollleft );
	Menu_AddItem( &s_controls.menu, &s_controls.rollright );
	Menu_AddItem( &s_controls.menu, &s_controls.run );	

	Menu_AddItem( &s_controls.menu, &s_controls.attack );
	Menu_AddItem( &s_controls.menu, &s_controls.attack2 );
	Menu_AddItem( &s_controls.menu, &s_controls.nextweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.prevweapon );
	Menu_AddItem( &s_controls.menu, &s_controls.run );
	Menu_AddItem( &s_controls.menu, &s_controls.chargepl );
	Menu_AddItem( &s_controls.menu, &s_controls.block );
	Menu_AddItem( &s_controls.menu, &s_controls.gesture );
	Menu_AddItem( &s_controls.menu, &s_controls.sensitivity );
	Menu_AddItem( &s_controls.menu, &s_controls.chat );

	Menu_AddItem( &s_controls.menu, &s_controls.back );

	trap_Cvar_VariableStringBuffer( "name", s_controls.name.string, 16 );
	Q_CleanStr( s_controls.name.string );

	// initialize the configurable cvars
	Controls_InitCvars();

	// initialize the current config
	Controls_GetConfig();

	// intialize the model
	Controls_InitModel();

	// intialize the weapons
	Controls_InitWeapons ();

	// update the ui
	Controls_Update();
}


/*
=================
Controls_Cache
=================
*/
void Controls_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
=================
UI_ControlsMenu
=================
*/
void UI_ControlsMenu( void ) {
	uis.menuamount = 3;
	uis.showFrame = qfalse;	
	uis.hideEarth = qtrue;
	Controls_MenuInit();
	UI_PushMenu( &s_controls.menu );
	Menu_SetCursorToItem(&s_controls.menu,&s_controls.controls);	
}
