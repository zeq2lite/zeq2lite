// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

SAGA MENU

=======================================================================
*/
#include "ui_local.h"

#define ART_BACK			"interface/art/arrows_horz_left"
#define ART_ENTER			"interface/art/arrows_horz_right"
#define ART_FRAME			"interface/menu/menuframe"

#define MAX_SAGAS			64
#define NAMEBUFSIZE			( MAX_SAGAS * 48 )
#define DIRBUFSIZE			( MAX_SAGAS * 16 )

#define ID_BACK				10
#define ID_ENTER			11
#define ID_LIST				12


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	frame;

	menulist_s		list;

	menubitmap_s	back;
	menubitmap_s	enter;

	char			description[NAMEBUFSIZE];
	char			fs_dir[DIRBUFSIZE];

	char			*descriptionPtr;
	char			*fs_dirPtr;

	char			*descriptionList[MAX_SAGAS];
	char			*fs_dirList[MAX_SAGAS];
} sagas_t;

static sagas_t	s_sagas;


/*
===============
UI_Sagas_MenuEvent
===============
*/
static void UI_Sagas_MenuEvent( void *ptr, int event ) {
	if(event != QM_ACTIVATED){return;}
	
	switch( ((menucommon_s*)ptr)->id ){
	case ID_ENTER:
		trap_Cvar_Set( "fs_dir", s_sagas.fs_dirList[s_sagas.list.curvalue] );
		trap_Cmd_ExecuteText( EXEC_APPEND, "snd_restart; wait 3; cinematic intro.roq;" );
		UI_PopMenu();
		break;
	case ID_BACK: UI_PopMenu(); break;
	}
}


/*
===============
UI_Sagas_ParseInfos
===============
*/
static void UI_Sagas_ParseInfos( char *sagaDir, char *sagaDesc ) {
	s_sagas.fs_dirList[s_sagas.list.numitems] = s_sagas.fs_dirPtr;
	Q_strncpyz( s_sagas.fs_dirPtr, sagaDir, 16 );

	s_sagas.descriptionList[s_sagas.list.numitems] = s_sagas.descriptionPtr;
	Q_strncpyz( s_sagas.descriptionPtr, sagaDesc, 48 );

	s_sagas.list.itemnames[s_sagas.list.numitems] = s_sagas.descriptionPtr;
	s_sagas.descriptionPtr += strlen( s_sagas.descriptionPtr ) + 1;
	s_sagas.fs_dirPtr += strlen( s_sagas.fs_dirPtr ) + 1;
	s_sagas.list.numitems++;
}


#if 0 // bk001204 - unused
/*
===============
UI_Sagas_LoadSagasFromFile
===============
*/
static void UI_Sagas_LoadSagasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[1024];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if(!f){
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if(len >= sizeof(buf)){
		trap_Print(va(S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, sizeof(buf)));
		trap_FS_FCloseFile(f);
		return;
	}
	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);
	
	len = strlen(filename);
	if(!Q_stricmp(filename + len - 5,".saga")){filename[len-5] = '\0';}
	UI_Sagas_ParseInfos(filename, buf);
}
#endif


/*
===============
UI_Sagas_LoadSagas
===============
*/
static void UI_Sagas_LoadSagas( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
	char	*descptr;
	int		i;
	int		dirlen;

	s_sagas.list.itemnames = (const char **)s_sagas.descriptionList;
	s_sagas.descriptionPtr = s_sagas.description;
	s_sagas.fs_dirPtr = s_sagas.fs_dir;
	s_sagas.fs_dirList[0] = "";

	numdirs = trap_FS_GetFileList("$modlist", "", dirlist, sizeof(dirlist));
	dirptr  = dirlist;
	for(i=0;i<numdirs;i++){
		dirlen = strlen(dirptr) + 1;
		descptr = dirptr + dirlen;
		UI_Sagas_ParseInfos(dirptr, descptr);
		dirptr += dirlen + strlen(descptr) + 1;
	}

	trap_Print(va("%i sagas parsed\n", s_sagas.list.numitems));
	if(s_sagas.list.numitems > MAX_SAGAS){ s_sagas.list.numitems = MAX_SAGAS;}
}

/*
=================
UI_Sagas_Cache
=================
*/
void UI_SagasMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip(ART_BACK);
	trap_R_RegisterShaderNoMip(ART_ENTER);
	trap_R_RegisterShaderNoMip(ART_FRAME);
}

/*
===============
UI_Sagas_MenuInit
===============
*/
static void UI_Sagas_MenuInit(void) {
	UI_SagasMenu_Cache();
	memset(&s_sagas, 0 ,sizeof(sagas_t));
	s_sagas.menu.wrapAround = qtrue;
	s_sagas.menu.fullscreen = qtrue;

	s_sagas.banner.generic.type		= MTYPE_BTEXT;
	s_sagas.banner.generic.x		= 320;
	s_sagas.banner.generic.y		= 16;
	s_sagas.banner.string			= "SAGAS";
	s_sagas.banner.color			= color_white;
	s_sagas.banner.style			= UI_CENTER|UI_DROPSHADOW;

	s_sagas.frame.generic.type		= MTYPE_BITMAP;
	s_sagas.frame.generic.name		= ART_FRAME;
	s_sagas.frame.generic.flags		= QMF_INACTIVE;
	s_sagas.frame.generic.x			= 160;  
	s_sagas.frame.generic.y			= 78;
	s_sagas.frame.width				= 320;
	s_sagas.frame.height			= 329;
	
	s_sagas.back.generic.type		= MTYPE_BITMAP;
	s_sagas.back.generic.name		= ART_BACK;
	s_sagas.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_sagas.back.generic.id			= ID_BACK;
	s_sagas.back.generic.callback	= UI_Sagas_MenuEvent;
	s_sagas.back.generic.x			= 0;
	s_sagas.back.generic.y			= 480-64;
	s_sagas.back.width				= 128;
	s_sagas.back.height				= 64;
	s_sagas.back.focuspic			= ART_BACK;

	s_sagas.enter.generic.type		= MTYPE_BITMAP;
	s_sagas.enter.generic.name		= ART_ENTER;
	s_sagas.enter.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_sagas.enter.generic.id		= ID_ENTER;
	s_sagas.enter.generic.callback	= UI_Sagas_MenuEvent;
	s_sagas.enter.generic.x			= 640;
	s_sagas.enter.generic.y			= 480-64;
	s_sagas.enter.width				= 128;
	s_sagas.enter.height			= 64;
	s_sagas.enter.focuspic			= ART_ENTER;

	// scan for sagas
	s_sagas.list.generic.type		= MTYPE_SCROLLLIST;
	s_sagas.list.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_sagas.list.generic.callback	= UI_Sagas_MenuEvent;
	s_sagas.list.generic.id			= ID_LIST;
	s_sagas.list.generic.x			= 300;
	s_sagas.list.generic.y			= 130;
	s_sagas.list.width				= 48;
	s_sagas.list.height				= 14;

	UI_Sagas_LoadSagas();

	Menu_AddItem(&s_sagas.menu, &s_sagas.banner);
	Menu_AddItem(&s_sagas.menu, &s_sagas.frame);
	Menu_AddItem(&s_sagas.menu, &s_sagas.list);
	Menu_AddItem(&s_sagas.menu, &s_sagas.back);
	Menu_AddItem(&s_sagas.menu, &s_sagas.enter);
}

/*
===============
UI_SagasMenu
===============
*/
void UI_SagasMenu( void ) {
	UI_Sagas_MenuInit();
	UI_PushMenu( &s_sagas.menu );
}
