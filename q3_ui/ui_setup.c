/*=======================================================================
SETUP MENU
=======================================================================*/
#include "ui_local.h"
#define ID_SYSTEMCONFIG			12
#define ID_GAME					13
#define ID_DEFAULTS				17
#define ID_BACK					18
#define	ID_CAMERA				19
typedef struct {
	menuframework_s	menu;
	menutext_s		setupsystem;
	menutext_s		game;
	menutext_s		camera;
	menutext_s		defaults;
	menutext_s		back;
} setupMenuInfo_t;
static setupMenuInfo_t	setupMenuInfo;
/*=================
Setup_ResetDefaults_Action
=================*/
static void Setup_ResetDefaults_Action( qboolean result ) {
	if( !result ) {
		return;
	}
	trap_Cmd_ExecuteText(EXEC_APPEND,"exec default.cfg\n");
	trap_Cmd_ExecuteText(EXEC_APPEND,"cvar_restart\n");
	trap_Cmd_ExecuteText(EXEC_APPEND,"vid_restart\n");
}
/*=================
Setup_ResetDefaults_Draw
=================*/
static void Setup_ResetDefaults_Draw( void ) {}
/*===============
UI_SetupMenu_Event
===============*/
static void UI_SetupMenu_Event( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}
	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SYSTEMCONFIG:
		UI_GraphicsOptionsMenu();
		break;
	case ID_GAME:
		UI_PreferencesMenu();
		break;
	case ID_CAMERA:
		UI_CameraMenu();
		break;
	case ID_DEFAULTS:
		UI_ConfirmMenu("RESET?",Setup_ResetDefaults_Draw,Setup_ResetDefaults_Action);
		break;
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}
/*===============
UI_SetupMenu_Init
===============*/
static void UI_SetupMenu_Init( void ) {
	int	y;
	int yOffset = 38;
	int	style = UI_LEFT | UI_DROPSHADOW | UI_SMALLFONT;
	UI_SetupMenu_Cache();
	memset( &setupMenuInfo, 0, sizeof(setupMenuInfo) );
	setupMenuInfo.menu.wrapAround = qtrue;
	setupMenuInfo.menu.fullscreen = qtrue;
	y = 116;
	setupMenuInfo.setupsystem.generic.type			= MTYPE_PTEXT;
	setupMenuInfo.setupsystem.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.setupsystem.generic.x				= 32;
	setupMenuInfo.setupsystem.generic.y				= y;
	setupMenuInfo.setupsystem.generic.id			= ID_SYSTEMCONFIG;
	setupMenuInfo.setupsystem.generic.callback		= UI_SetupMenu_Event; 
	setupMenuInfo.setupsystem.string				= "SYSTEM";
	setupMenuInfo.setupsystem.color					= color_white;
	setupMenuInfo.setupsystem.style					= style;
	y += yOffset;
	if(!trap_Cvar_VariableValue("cl_paused")){
		setupMenuInfo.defaults.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.defaults.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.defaults.generic.x				= 32;
		setupMenuInfo.defaults.generic.y				= y;
		setupMenuInfo.defaults.generic.id				= ID_DEFAULTS;
		setupMenuInfo.defaults.generic.callback			= UI_SetupMenu_Event; 
		setupMenuInfo.defaults.string					= "RESET";
		setupMenuInfo.defaults.color					= color_white;
		setupMenuInfo.defaults.style					= style;
	}
	else{
		setupMenuInfo.camera.generic.type				= MTYPE_PTEXT;
		setupMenuInfo.camera.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
		setupMenuInfo.camera.generic.x					= 32;
		setupMenuInfo.camera.generic.y					= y;
		setupMenuInfo.camera.generic.id					= ID_CAMERA;
		setupMenuInfo.camera.generic.callback			= UI_SetupMenu_Event;
		setupMenuInfo.camera.string						= "CAMERA";
		setupMenuInfo.camera.color						= color_white;
		setupMenuInfo.camera.style						= style;
	}
	y += yOffset;
	setupMenuInfo.back.generic.type				= MTYPE_PTEXT;
	setupMenuInfo.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	setupMenuInfo.back.generic.x				= 32;
	setupMenuInfo.back.generic.y				= y;
	setupMenuInfo.back.generic.id				= ID_BACK;
	setupMenuInfo.back.generic.callback			= UI_SetupMenu_Event;
	setupMenuInfo.back.string					= "BACK";
	setupMenuInfo.back.color					= color_white;
	setupMenuInfo.back.style					= style;
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.setupsystem );
	if(!trap_Cvar_VariableValue("cl_paused")){Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );}
	else{Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.camera );}
	Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back);
}
/*=================
UI_SetupMenu_Cache
=================*/
void UI_SetupMenu_Cache( void ) {}
/*===============
UI_SetupMenu
===============*/
void UI_SetupMenu( void ) {
	uis.menuamount = 4;
	UI_SetupMenu_Init();
	UI_PushMenu( &setupMenuInfo.menu );
}
