/*=======================================================================
SYSTEM CONFIGURATION MENU
=======================================================================*/
#include "ui_local.h"
#define ID_DISPLAY			11
#define ID_SOUND			12
#define ID_GENERAL			13
#define ID_BACK				14
typedef struct {
	menuframework_s	menu;
	menulist_s		crosshair;
	menulist_s		crosshairSize;
	menulist_s		camerastyle;
	menulist_s		beamcontrol;
	menutext_s		display;
	menutext_s		general;
	menutext_s		sound;
	menutext_s		back;
} optionsmenu_t;
static optionsmenu_t s_options;
static const char *beamcontrol_names[] = {"Beam Head Focus","Crosshair Focus",0};
static const char *camerastyle_names[] = {"Locked Behind Head","Locked Behind Character","Delay Behind Character",0};
static const char *crosshairSize_names[] = {"Tiny!","Very Small","Small","Normal","Large","Extra Large!","Extra Extra Large?!",0};
/*=================
Options_Event
=================*/
static void Options_Event( void* ptr, int event ) {
	if(event != QM_ACTIVATED){return;}
	switch( ((menucommon_s*)ptr)->id ) {
	case ID_DISPLAY:
		UI_PopMenu();
		UI_DisplayOptionsMenu();
		break;
	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;
	case ID_GENERAL:
		UI_PopMenu();
		UI_PreferencesMenu();
		break;
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}
void SystemConfig_Cache(void){}
/*===============
Main Options
===============*/
void Options_MenuInit( void ) {
	int x = 28;
	int	y = 119;
	int	offset = 38;
	int	style = UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	memset( &s_options, 0, sizeof(optionsmenu_t));
	s_options.menu.wrapAround = qtrue;
	s_options.menu.fullscreen = qtrue;
	s_options.display.generic.type		= MTYPE_PTEXT;
	s_options.display.generic.flags		= QMF_LEFT_JUSTIFY;
	s_options.display.generic.id		= ID_DISPLAY;
	s_options.display.generic.callback	= Options_Event;
	s_options.display.generic.x			= x;
	s_options.display.generic.y			= y;
	s_options.display.string			= "DISPLAY";
	s_options.display.style				= style;
	s_options.display.color				= color_white;
	y+=offset;
	s_options.general.generic.type		= MTYPE_PTEXT;
	s_options.general.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.general.generic.id		= ID_GENERAL;
	s_options.general.generic.callback	= Options_Event;
	s_options.general.generic.x			= x;
	s_options.general.generic.y			= y;
	s_options.general.string			= "GENERAL";
	s_options.general.style				= style;
	s_options.general.color				= color_white;
	y+=offset;
	s_options.sound.generic.type		= MTYPE_PTEXT;
	s_options.sound.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.sound.generic.id			= ID_SOUND;
	s_options.sound.generic.callback	= Options_Event;
	s_options.sound.generic.x			= x;
	s_options.sound.generic.y			= y;
	s_options.sound.string				= "SOUND";
	s_options.sound.style				= style;
	s_options.sound.color				= color_white;
	y+=offset;
	s_options.back.generic.type			= MTYPE_PTEXT;
	s_options.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_options.back.generic.id			= ID_BACK;
	s_options.back.generic.callback		= Options_Event;
	s_options.back.generic.x			= x;
	s_options.back.generic.y			= y;
	s_options.back.string				= "BACK";
	s_options.back.style				= style;
	s_options.back.color				= color_white;
	Menu_AddItem( &s_options.menu, ( void * ) &s_options.display );
	Menu_AddItem( &s_options.menu, ( void * ) &s_options.general );
	Menu_AddItem( &s_options.menu, ( void * ) &s_options.sound );
	Menu_AddItem( &s_options.menu, ( void * ) &s_options.back );
}
/*===============
UI_SystemConfigMenu
===============*/
void UI_SystemConfigMenu( void ) {
	uis.menuamount = 4;
	Options_MenuInit();
	UI_PushMenu ( &s_options.menu );
}
