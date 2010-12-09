
/*
=======================================================================

MUSIC MENU

=======================================================================
*/

#include "ui_local.h"

#define ART_BACK0			"interface/art/back_0"
#define ART_BACK1			"interface/art/back_1"	
#define ART_GO0				"interface/art/play_0"
#define ART_GO1				"interface/art/play_1"
#define ART_FRAMEL			"interface/art/frame2_l"
#define ART_FRAMER			"interface/art/frame1_r"
#define ART_ARROWS			"interface/art/arrows_horz_0"
#define ART_ARROWLEFT		"interface/art/arrows_horz_left"
#define ART_ARROWRIGHT		"interface/art/arrows_horz_right"

#define MAX_MUSIC			128
#define NAMEBUFSIZE			( MAX_MUSIC * 16 )

#define ID_BACK				29
#define ID_GO				30
#define ID_LIST				31
#define ID_RIGHT			32
#define ID_LEFT				33

#define ARROWS_WIDTH		128
#define ARROWS_HEIGHT		48

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menulist_s		list;

	menubitmap_s	arrows;
	menubitmap_s	left;
	menubitmap_s	right;
	menubitmap_s	back;
	menubitmap_s	go;

	int				numMusic;
	char			names[NAMEBUFSIZE];
	char			*musiclist[MAX_MUSIC];
} music_t;

static music_t	s_music;


/*
===============
Music_MenuEvent
===============
*/
static void Music_MenuEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GO:
//		UI_ForceMenuOff ();
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "music music/%s\n",
								s_music.list.itemnames[s_music.list.curvalue]) );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;

	case ID_LEFT:
		ScrollList_Key( &s_music.list, K_LEFTARROW );
		break;

	case ID_RIGHT:
		ScrollList_Key( &s_music.list, K_RIGHTARROW );
		break;
	}
}


/*
=================
UI_MusicMenu_Key
=================
*/
static sfxHandle_t UI_MusicMenu_Key( int key ) {
	menucommon_s	*item;

	item = Menu_ItemAtCursor( &s_music.menu );

	return Menu_DefaultKey( &s_music.menu, key );
}

/*
=================
Music_Cache
=================
*/
void Music_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_GO0 );
	trap_R_RegisterShaderNoMip( ART_GO1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_ARROWS );
	trap_R_RegisterShaderNoMip( ART_ARROWLEFT );
	trap_R_RegisterShaderNoMip( ART_ARROWRIGHT );
}

/*
===============
Music_MenuInit
===============
*/
static void Music_MenuInit( void ) {
	int		i;
	int		len;
	char	*musicname, extension[32];

	memset( &s_music, 0 ,sizeof(music_t) );
	s_music.menu.key = UI_MusicMenu_Key;

	Music_Cache();

	s_music.menu.fullscreen = qfalse;
	s_music.menu.wrapAround = qtrue;

	s_music.banner.generic.type		= MTYPE_BTEXT;
	s_music.banner.generic.x		= 320;
	s_music.banner.generic.y		= 16;
	s_music.banner.string			= "MUSIC";
	s_music.banner.color			= color_white;
	s_music.banner.style			= UI_CENTER|UI_DROPSHADOW;

	s_music.framel.generic.type		= MTYPE_BITMAP;
	s_music.framel.generic.name		= ART_FRAMEL;
	s_music.framel.generic.flags	= QMF_INACTIVE;
	s_music.framel.generic.x		= 0;  
	s_music.framel.generic.y		= 78;
	s_music.framel.width			= 256;
	s_music.framel.height			= 329;

	s_music.framer.generic.type		= MTYPE_BITMAP;
	s_music.framer.generic.name		= ART_FRAMER;
	s_music.framer.generic.flags	= QMF_INACTIVE;
	s_music.framer.generic.x		= 376;
	s_music.framer.generic.y		= 46;
	s_music.framer.width			= 256;
	s_music.framer.height			= 334;

	s_music.arrows.generic.type		= MTYPE_BITMAP;
	s_music.arrows.generic.name		= ART_ARROWS;
	s_music.arrows.generic.flags	= QMF_INACTIVE;
	s_music.arrows.generic.x		= 315-ARROWS_WIDTH/2;
	s_music.arrows.generic.y		= 343;
	s_music.arrows.width			= ARROWS_WIDTH;
	s_music.arrows.height			= ARROWS_HEIGHT;

	s_music.left.generic.type		= MTYPE_BITMAP;
	s_music.left.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_music.left.generic.x			= 315-ARROWS_WIDTH/2;
	s_music.left.generic.y			= 343;
	s_music.left.generic.id			= ID_LEFT;
	s_music.left.generic.callback	= Music_MenuEvent;
	s_music.left.width				= ARROWS_WIDTH/2;
	s_music.left.height				= ARROWS_HEIGHT;
	s_music.left.focuspic			= ART_ARROWLEFT;

	s_music.right.generic.type		= MTYPE_BITMAP;
	s_music.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_MOUSEONLY;
	s_music.right.generic.x			= 315;
	s_music.right.generic.y			= 343;
	s_music.right.generic.id		= ID_RIGHT;
	s_music.right.generic.callback	= Music_MenuEvent;
	s_music.right.width				= ARROWS_WIDTH/2;
	s_music.right.height			= ARROWS_HEIGHT;
	s_music.right.focuspic			= ART_ARROWRIGHT;

	s_music.back.generic.type		= MTYPE_BITMAP;
	s_music.back.generic.name		= ART_BACK0;
	s_music.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_music.back.generic.id			= ID_BACK;
	s_music.back.generic.callback	= Music_MenuEvent;
	s_music.back.generic.x			= 103;
	s_music.back.generic.y			= 330;
	s_music.back.width				= 128;
	s_music.back.height				= 64;
	s_music.back.focuspic			= ART_BACK1;

	s_music.go.generic.type			= MTYPE_BITMAP;
	s_music.go.generic.name			= ART_GO0;
	s_music.go.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_music.go.generic.id			= ID_GO;
	s_music.go.generic.callback		= Music_MenuEvent;
	s_music.go.generic.x			= 541;
	s_music.go.generic.y			= 330;
	s_music.go.width				= 128;
	s_music.go.height				= 64;
	s_music.go.focuspic				= ART_GO1;

	s_music.list.generic.type		= MTYPE_SCROLLLIST;
	s_music.list.generic.flags		= QMF_PULSEIFFOCUS;
	s_music.list.generic.callback	= Music_MenuEvent;
	s_music.list.generic.id			= ID_LIST;
	s_music.list.generic.x			= 10;//118;
	s_music.list.generic.y			= 100;
	s_music.list.width				= 16;
	s_music.list.height				= 14;
	s_music.list.numitems			= trap_FS_GetFileList( "music", "ogg", s_music.names, NAMEBUFSIZE );
	s_music.list.itemnames			= (const char **)s_music.musiclist;
	s_music.list.columns			= 4;//3;

	if (!s_music.list.numitems) {
		strcpy( s_music.names, "No Music Found." );
		s_music.list.numitems = 1;

		//degenerate case, not selectable
		s_music.go.generic.flags |= (QMF_INACTIVE|QMF_HIDDEN);
	}
	else if (s_music.list.numitems > MAX_MUSIC)
		s_music.list.numitems = MAX_MUSIC;

	musicname = s_music.names;
	for ( i = 0; i < s_music.list.numitems; i++ ) {
		s_music.list.itemnames[i] = musicname;
		
		// strip extension
		len = strlen( musicname );
		if (!Q_stricmp(musicname +  len - 4,".ogg"))
			musicname[len-4] = '\0';

		Q_strupr(musicname);

		musicname += len + 1;
	}

	Menu_AddItem( &s_music.menu, &s_music.banner );
	Menu_AddItem( &s_music.menu, &s_music.framel );
	Menu_AddItem( &s_music.menu, &s_music.framer );
	Menu_AddItem( &s_music.menu, &s_music.list );
	Menu_AddItem( &s_music.menu, &s_music.arrows );
	Menu_AddItem( &s_music.menu, &s_music.left );
	Menu_AddItem( &s_music.menu, &s_music.right );
	Menu_AddItem( &s_music.menu, &s_music.back );
	Menu_AddItem( &s_music.menu, &s_music.go );
}


/*
===============
UI_MusicMenu
===============
*/
void UI_MusicMenu( void ) {
	Music_MenuInit();
	UI_PushMenu( &s_music.menu );
}
