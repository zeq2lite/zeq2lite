/*=======================================================================
GAME OPTIONS MENU
=======================================================================*/
#include "ui_local.h"
#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"
#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"
#define ART_ACCEPT0				"menu/art/accept_0"
#define ART_ACCEPT1				"menu/art/accept_1"
#define PREFERENCES_X_POS		320
#define ID_CROSSHAIR			127
#define ID_CAMERASTYLE			128
#define	ID_BEAMCONTROL			129
#define	ID_BEAMDETAIL			130
#define ID_WALLMARKS			131
#define ID_DYNAMICLIGHTS		132
#define ID_IDENTIFYTARGET		133
#define ID_SYNCEVERYFRAME		134
#define	ID_MOTIONBLUR			135
#define ID_USERUNANIMATION		136
#define ID_ALLOWDOWNLOAD		137
#define ID_BACK					138
#define ID_CROSSHAIRSIZE		139
#define ID_PARTICLESOPTIMISE	140
#define ID_PARTICLESQUALITY		141
#define ID_BLOOMQUALITY			142
#define ID_BLOOMINTENSITY		143
#define ID_BLOOMDARKEN			144
#define ID_BLOOMALPHA			145
#define ID_OUTLINES				146
#define	NUM_CROSSHAIRS			10
typedef struct{

	menulist_s			crosshair;
	menulist_s			crosshairSize;
	menulist_s			camerastyle;
	menulist_s			beamcontrol;

	menubitmap_s		back;
	menubitmap_s		apply;
	qhandle_t			crosshairShader[NUM_CROSSHAIRS];
} preferences_t;
static int	initialBloomQuality;
static preferences_t s_preferences;

static void Preferences_SetMenuItems( void ) {
	s_preferences.crosshair.curvalue		= (int)trap_Cvar_VariableValue( "cg_drawCrosshair" ) % NUM_CROSSHAIRS;
	s_preferences.camerastyle.curvalue		= Com_Clamp( 0, 2, trap_Cvar_VariableValue( "cg_thirdPersonCamera" ) );
	s_preferences.crosshairSize.curvalue	= Com_Clamp( 0, 6, trap_Cvar_VariableValue( "cg_crosshairSize" ) );
	s_preferences.beamdetail.curvalue		= Com_Clamp( 0, 4, trap_Cvar_VariableValue( "r_beamDetail" ) );
	s_preferences.particlesQuality.curvalue	= Com_Clamp( 0, 2, trap_Cvar_VariableValue( "cg_particlesQuality" ) );
	s_preferences.particlesOptimise.curvalue= Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_particlesStop" ) );
	s_preferences.wallmarks.curvalue		= trap_Cvar_VariableValue( "cg_marks" ) != 0;
	s_preferences.identifytarget.curvalue	= trap_Cvar_VariableValue( "cg_drawCrosshairNames" ) != 0;
	s_preferences.dynamiclights.curvalue	= trap_Cvar_VariableValue( "r_dynamiclight" ) != 0;
	s_preferences.beamcontrol.curvalue		= Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_beamControl" ) );
	s_preferences.synceveryframe.curvalue	= trap_Cvar_VariableValue( "r_finish" ) != 0;
	s_preferences.motionblur.curvalue		= trap_Cvar_VariableValue( "r_motionBlur" ) != 0;
	s_preferences.useRunAnimation.curvalue	= trap_Cvar_VariableValue( "g_running" ) != 0;
	s_preferences.allowdownload.curvalue	= trap_Cvar_VariableValue( "cl_allowDownload" ) != 0;
	s_preferences.outlines.curvalue			= trap_Cvar_VariableValue( "r_outlines" ) != 0;
	// Bloom Quality
	switch ( ( int ) trap_Cvar_VariableValue( "r_bloom_sample_size" ) )
	{
	default:
	case 0:
		s_preferences.bloomQuality.curvalue = 0;
		break;
	case 64:
		s_preferences.bloomQuality.curvalue = 1;
		break;
	case 128:
		s_preferences.bloomQuality.curvalue = 2;
		break;
	case 256:
		s_preferences.bloomQuality.curvalue = 3;
		break;
	case 512:
		s_preferences.bloomQuality.curvalue = 4;
		break;
	}
	// Bloom Intensity
	s_preferences.bloomIntensity.curvalue = trap_Cvar_VariableValue( "r_bloom_intensity" );
	// Bloom Darken
	switch ( ( int ) trap_Cvar_VariableValue( "r_bloom_darken" ) )
	{
	default:
	case 16:
		s_preferences.bloomDarken.curvalue = 0;
		break;
	case 32:
		s_preferences.bloomDarken.curvalue = 1;
		break;
	case 64:
		s_preferences.bloomDarken.curvalue = 2;
		break;
	}
	// Bloom Alpha
	s_preferences.bloomAlpha.curvalue = trap_Cvar_VariableValue( "r_bloom_alpha" );
}
static void Preferences_Event( void* ptr, int notification ) {
	if(notification != QM_ACTIVATED){
		return;
	}
	switch(((menucommon_s*)ptr)->id){
	case ID_CROSSHAIR:
		s_preferences.crosshair.curvalue++;
		if( s_preferences.crosshair.curvalue == NUM_CROSSHAIRS ) {
			s_preferences.crosshair.curvalue = 0;
		}
		trap_Cvar_SetValue( "cg_drawCrosshair", s_preferences.crosshair.curvalue );
		break;
	case ID_CROSSHAIRSIZE:
		trap_Cvar_SetValue( "cg_crosshairSize", s_preferences.crosshairSize.curvalue);
		break;
	case ID_CAMERASTYLE:
		trap_Cvar_SetValue( "cg_thirdPersonCamera", s_preferences.camerastyle.curvalue );
		break;
	case ID_BEAMCONTROL:
		trap_Cvar_SetValue( "cg_beamControl", s_preferences.beamcontrol.curvalue );
		break;
	case ID_BEAMDETAIL:
		trap_Cvar_SetValue( "r_beamDetail", s_preferences.beamdetail.curvalue);
		break;
	case ID_PARTICLESQUALITY:
		trap_Cvar_SetValue( "cg_particlesQuality", s_preferences.particlesQuality.curvalue );
		break;
	case ID_PARTICLESOPTIMISE:
		trap_Cvar_SetValue( "cg_particlesStop", s_preferences.particlesOptimise.curvalue );
		break;
	case ID_WALLMARKS:
		trap_Cvar_SetValue( "cg_marks", s_preferences.wallmarks.curvalue );
		break;
	case ID_DYNAMICLIGHTS:
		trap_Cvar_SetValue( "r_dynamiclight", s_preferences.dynamiclights.curvalue );
		break;		
	case ID_IDENTIFYTARGET:
		trap_Cvar_SetValue( "cg_drawCrosshairNames", s_preferences.identifytarget.curvalue );
		break;
	case ID_SYNCEVERYFRAME:
		trap_Cvar_SetValue( "r_finish", s_preferences.synceveryframe.curvalue );
		break;
	case ID_MOTIONBLUR:
		trap_Cvar_SetValue( "r_motionBlur", s_preferences.motionblur.curvalue );
		break;
	case ID_USERUNANIMATION:
		trap_Cvar_SetValue( "g_running", s_preferences.useRunAnimation.curvalue );
		break;
	case ID_ALLOWDOWNLOAD:
		trap_Cvar_SetValue( "cl_allowDownload", s_preferences.allowdownload.curvalue );
		break;
	case ID_OUTLINES:
		trap_Cvar_SetValue( "r_outlines", s_preferences.outlines.curvalue );
		break;
	case ID_BLOOMQUALITY:
		switch ( s_preferences.bloomQuality.curvalue  )
		{
		case 0:
			trap_Cvar_SetValue( "r_bloom_sample_size", 0 );
			trap_Cvar_SetValue( "r_bloom", 0 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_bloom_sample_size", 64 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_bloom_sample_size", 128 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		case 3:
			trap_Cvar_SetValue( "r_bloom_sample_size", 256 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		case 4:
			trap_Cvar_SetValue( "r_bloom_sample_size", 512 );
			trap_Cvar_SetValue( "r_bloom", 1 );
			break;
		}
		break;
	case ID_BLOOMINTENSITY:
		trap_Cvar_SetValue( "r_bloom_intensity", s_preferences.bloomIntensity.curvalue);
		break;
	case ID_BLOOMDARKEN:
		switch ( s_preferences.bloomDarken.curvalue  )
		{
		case 0:
			trap_Cvar_SetValue( "r_bloom_darken", 16 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_bloom_darken", 32 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_bloom_darken", 64 );
			break;
		}
		break;
	case ID_BLOOMALPHA:
		trap_Cvar_SetValue( "r_bloom_alpha", s_preferences.bloomAlpha.curvalue);
		break;
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}
/*=================
Crosshair_Draw
=================*/
static void Crosshair_Draw( void *self ) {
	menulist_s	*s;
	float		*color;
	int			x, y;
	int			style;
	int			crosshairSizeImage;
	qboolean	focus;
	crosshairSizeImage = trap_Cvar_VariableValue( "cg_crosshairSize" ) * 8 + 8;
	s = (menulist_s *)self;
	x = s->generic.x;
	y =	s->generic.y;
	style = UI_SMALLFONT|UI_DROPSHADOW;
	focus = (s->generic.parent->cursor == s->generic.menuPosition);
	if(s->generic.flags & QMF_GRAYED){
		color = text_color_disabled;
	}
	else if(focus){
		color = text_color_highlight;
		style |= UI_PULSE;
	}
	else if(s->generic.flags & QMF_BLINK){
		color = text_color_highlight;
		style |= UI_BLINK;
	}
	else{
		color = text_color_normal;
	}
	if(focus){
		// draw cursor
		UI_FillRect( s->generic.left, s->generic.top, s->generic.right-s->generic.left+1, s->generic.bottom-s->generic.top+1, listbar_color ); 
		UI_DrawChar( x, y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT|UI_DROPSHADOW, color);
	}
	UI_DrawString( x - SMALLCHAR_WIDTH, y, s->generic.name, style|UI_RIGHT, color );
	if(!s->curvalue){return;}
	UI_DrawHandlePic( x + SMALLCHAR_WIDTH + 64 - 0.5f * crosshairSizeImage, y + 2 - 0.5f * crosshairSizeImage, crosshairSizeImage, crosshairSizeImage, s_preferences.crosshairShader[s->curvalue] );
}

/*
=================
Bloom_ApplyChanges
=================
*/
static void Bloom_ApplyChanges( void *unused, int notification )
{
	if (notification != QM_ACTIVATED)
		return;

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}
/*
================
Preferences_MenuDraw
================
*/
void Preferences_MenuDraw (void)
{
	s_preferences.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if ( initialBloomQuality != s_preferences.bloomQuality.curvalue )
	{
		s_preferences.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	Menu_Draw( &s_preferences.menu );
}

static void Preferences_MenuInit( void ) {
	int	y;
	memset( &s_preferences, 0 ,sizeof(preferences_t) );
	Preferences_Cache();
	s_preferences.menu.wrapAround = qtrue;
	s_preferences.menu.fullscreen = qtrue;
	s_preferences.menu.draw = Preferences_MenuDraw;
	s_preferences.banner.generic.type  = MTYPE_BTEXT;
	s_preferences.banner.generic.x	   = 320;
	s_preferences.banner.generic.y	   = 16;
	s_preferences.banner.string		   = "GAME OPTIONS";
	s_preferences.banner.color         = color_white;
	s_preferences.banner.style         = UI_CENTER|UI_DROPSHADOW;
	s_preferences.framel.generic.type  = MTYPE_BITMAP;
	s_preferences.framel.generic.name  = ART_FRAMEL;
	s_preferences.framel.generic.flags = QMF_INACTIVE;
	s_preferences.framel.generic.x	   = 0;
	s_preferences.framel.generic.y	   = 78;
	s_preferences.framel.width  	   = 256;
	s_preferences.framel.height  	   = 329;
	s_preferences.framer.generic.type  = MTYPE_BITMAP;
	s_preferences.framer.generic.name  = ART_FRAMER;
	s_preferences.framer.generic.flags = QMF_INACTIVE;
	s_preferences.framer.generic.x	   = 376;
	s_preferences.framer.generic.y	   = 76;
	s_preferences.framer.width  	   = 256;
	s_preferences.framer.height  	   = 334;

	y = 88;
	s_preferences.crosshair.generic.type		= MTYPE_TEXT;
	s_preferences.crosshair.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_NODEFAULTINIT|QMF_OWNERDRAW;
	s_preferences.crosshair.generic.x			= PREFERENCES_X_POS - 32;
	s_preferences.crosshair.generic.y			= y;
	s_preferences.crosshair.generic.name		= "Crosshair:";
	s_preferences.crosshair.generic.callback	= Preferences_Event;
	s_preferences.crosshair.generic.ownerdraw	= Crosshair_Draw;
	s_preferences.crosshair.generic.id			= ID_CROSSHAIR;
	s_preferences.crosshair.generic.top			= y - 4;
	s_preferences.crosshair.generic.bottom		= y + 20;
	s_preferences.crosshair.generic.left		= PREFERENCES_X_POS - ( ( strlen(s_preferences.crosshair.generic.name) + 1 ) * SMALLCHAR_WIDTH ) - 32;
	s_preferences.crosshair.generic.right		= PREFERENCES_X_POS + 48 - 32;

	y += BIGCHAR_HEIGHT+26;
	s_preferences.crosshairSize.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.crosshairSize.generic.name		= "Crosshair Size:";
	s_preferences.crosshairSize.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.crosshairSize.generic.callback	= Preferences_Event;
	s_preferences.crosshairSize.generic.id			= ID_CROSSHAIRSIZE;
	s_preferences.crosshairSize.generic.x			= PREFERENCES_X_POS;
	s_preferences.crosshairSize.generic.y			= y;
	s_preferences.crosshairSize.itemnames			= crosshairSize_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.camerastyle.generic.type        = MTYPE_SPINCONTROL;
	s_preferences.camerastyle.generic.name	      = "Camera style:";
	s_preferences.camerastyle.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.camerastyle.generic.callback    = Preferences_Event;
	s_preferences.camerastyle.generic.id          = ID_CAMERASTYLE;
	s_preferences.camerastyle.generic.x	          = PREFERENCES_X_POS;
	s_preferences.camerastyle.generic.y	          = y;
	s_preferences.camerastyle.itemnames			  = camerastyle_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.beamcontrol.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.beamcontrol.generic.name		= "Beam Control Style:";
	s_preferences.beamcontrol.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.beamcontrol.generic.callback	= Preferences_Event;
	s_preferences.beamcontrol.generic.id		= ID_BEAMCONTROL;
	s_preferences.beamcontrol.generic.x			= PREFERENCES_X_POS;
	s_preferences.beamcontrol.generic.y			= y;
	s_preferences.beamcontrol.itemnames			= beamcontrol_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.beamdetail.generic.type			  = MTYPE_SPINCONTROL;
	s_preferences.beamdetail.generic.name			  = "Beam Detail:";
	s_preferences.beamdetail.generic.flags	          = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.beamdetail.generic.callback         = Preferences_Event;
	s_preferences.beamdetail.generic.id               = ID_BEAMDETAIL;
	s_preferences.beamdetail.generic.x	              = PREFERENCES_X_POS;
	s_preferences.beamdetail.generic.y	              = y;
	s_preferences.beamdetail.itemnames				  = beamdetail_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.particlesQuality.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.particlesQuality.generic.name			= "Particle Quality:";
	s_preferences.particlesQuality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.particlesQuality.generic.callback		= Preferences_Event;
	s_preferences.particlesQuality.generic.id			= ID_PARTICLESQUALITY;
	s_preferences.particlesQuality.generic.x			= PREFERENCES_X_POS;
	s_preferences.particlesQuality.generic.y			= y;
	s_preferences.particlesQuality.itemnames			= particlesQuality_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.particlesOptimise.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.particlesOptimise.generic.name		= "Particle Physics:";
	s_preferences.particlesOptimise.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.particlesOptimise.generic.callback	= Preferences_Event;
	s_preferences.particlesOptimise.generic.id			= ID_PARTICLESOPTIMISE;
	s_preferences.particlesOptimise.generic.x			= PREFERENCES_X_POS;
	s_preferences.particlesOptimise.generic.y			= y;
	s_preferences.particlesOptimise.itemnames			= particlesOptimise_names;
/*
	y += BIGCHAR_HEIGHT+2;
	s_preferences.motionblur.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.motionblur.generic.name	  = "Motion Blur Effect:";
	s_preferences.motionblur.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.motionblur.generic.callback = Preferences_Event;
	s_preferences.motionblur.generic.id       = ID_MOTIONBLUR;
	s_preferences.motionblur.generic.x	      = PREFERENCES_X_POS;
	s_preferences.motionblur.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.useRunAnimation.generic.type		= MTYPE_RADIOBUTTON;
	s_preferences.useRunAnimation.generic.name		= "Use Run Animations:";
	s_preferences.useRunAnimation.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.useRunAnimation.generic.callback	= Preferences_Event;
	s_preferences.useRunAnimation.generic.id		= ID_USERUNANIMATION;
	s_preferences.useRunAnimation.generic.x			= PREFERENCES_X_POS;
	s_preferences.useRunAnimation.generic.y			= y;
*/
	y += BIGCHAR_HEIGHT+2;
	y += BIGCHAR_HEIGHT+2;
	s_preferences.dynamiclights.generic.type      = MTYPE_RADIOBUTTON;
	s_preferences.dynamiclights.generic.name	  = "Dynamic Lights:";
	s_preferences.dynamiclights.generic.flags     = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.dynamiclights.generic.callback  = Preferences_Event;
	s_preferences.dynamiclights.generic.id        = ID_DYNAMICLIGHTS;
	s_preferences.dynamiclights.generic.x	      = PREFERENCES_X_POS;
	s_preferences.dynamiclights.generic.y	      = y;

	y += BIGCHAR_HEIGHT;
	s_preferences.wallmarks.generic.type          = MTYPE_RADIOBUTTON;
	s_preferences.wallmarks.generic.name	      = "Marks on Walls:";
	s_preferences.wallmarks.generic.flags	      = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.wallmarks.generic.callback      = Preferences_Event;
	s_preferences.wallmarks.generic.id            = ID_WALLMARKS;
	s_preferences.wallmarks.generic.x	          = PREFERENCES_X_POS;
	s_preferences.wallmarks.generic.y	          = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.identifytarget.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.identifytarget.generic.name	  = "Identify Target:";
	s_preferences.identifytarget.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.identifytarget.generic.callback = Preferences_Event;
	s_preferences.identifytarget.generic.id       = ID_IDENTIFYTARGET;
	s_preferences.identifytarget.generic.x	      = PREFERENCES_X_POS;
	s_preferences.identifytarget.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.synceveryframe.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.synceveryframe.generic.name	  = "Sync Every Frame:";
	s_preferences.synceveryframe.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.synceveryframe.generic.callback = Preferences_Event;
	s_preferences.synceveryframe.generic.id       = ID_SYNCEVERYFRAME;
	s_preferences.synceveryframe.generic.x	      = PREFERENCES_X_POS;
	s_preferences.synceveryframe.generic.y	      = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.allowdownload.generic.type     = MTYPE_RADIOBUTTON;
	s_preferences.allowdownload.generic.name	   = "Automatic Downloading:";
	s_preferences.allowdownload.generic.flags	   = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.allowdownload.generic.callback = Preferences_Event;
	s_preferences.allowdownload.generic.id       = ID_ALLOWDOWNLOAD;
	s_preferences.allowdownload.generic.x	       = PREFERENCES_X_POS;
	s_preferences.allowdownload.generic.y	       = y;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.outlines.generic.type			= MTYPE_RADIOBUTTON;
	s_preferences.outlines.generic.name			= "Character Outlines:";
	s_preferences.outlines.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.outlines.generic.callback		= Preferences_Event;
	s_preferences.outlines.generic.id			= ID_OUTLINES;
	s_preferences.outlines.generic.x			= PREFERENCES_X_POS;
	s_preferences.outlines.generic.y			= y;

	y += BIGCHAR_HEIGHT+2;
	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomQuality.generic.type			= MTYPE_SPINCONTROL;
	s_preferences.bloomQuality.generic.name			= "Bloom Quality:";
	s_preferences.bloomQuality.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomQuality.generic.callback		= Preferences_Event;
	s_preferences.bloomQuality.generic.id			= ID_BLOOMQUALITY;
	s_preferences.bloomQuality.generic.x			= PREFERENCES_X_POS;
	s_preferences.bloomQuality.generic.y			= y;
	s_preferences.bloomQuality.itemnames			= bloomQuality_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomDarken.generic.type		= MTYPE_SPINCONTROL;
	s_preferences.bloomDarken.generic.name		= "Bloom Darken:";
	s_preferences.bloomDarken.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomDarken.generic.callback	= Preferences_Event;
	s_preferences.bloomDarken.generic.id		= ID_BLOOMDARKEN;
	s_preferences.bloomDarken.generic.x			= PREFERENCES_X_POS;
	s_preferences.bloomDarken.generic.y			= y;
	s_preferences.bloomDarken.itemnames			= bloomDarken_names;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomIntensity.generic.type		= MTYPE_SLIDER;
	s_preferences.bloomIntensity.generic.name		= "Bloom Intensity:";
	s_preferences.bloomIntensity.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomIntensity.generic.callback	= Preferences_Event;
	s_preferences.bloomIntensity.generic.id			= ID_BLOOMINTENSITY;
	s_preferences.bloomIntensity.generic.x			= PREFERENCES_X_POS;
	s_preferences.bloomIntensity.generic.y			= y;
	s_preferences.bloomIntensity.minvalue			= 0;
	s_preferences.bloomIntensity.maxvalue			= 1;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.bloomAlpha.generic.type		= MTYPE_SLIDER;
	s_preferences.bloomAlpha.generic.name		= "Bloom Alpha:";
	s_preferences.bloomAlpha.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_preferences.bloomAlpha.generic.callback	= Preferences_Event;
	s_preferences.bloomAlpha.generic.id			= ID_BLOOMALPHA;
	s_preferences.bloomAlpha.generic.x			= PREFERENCES_X_POS;
	s_preferences.bloomAlpha.generic.y			= y;
	s_preferences.bloomAlpha.minvalue			= 0;
	s_preferences.bloomAlpha.maxvalue			= 1;

	y += BIGCHAR_HEIGHT+2;
	s_preferences.back.generic.type	    = MTYPE_BITMAP;
	s_preferences.back.generic.name     = ART_BACK0;
	s_preferences.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_preferences.back.generic.callback = Preferences_Event;
	s_preferences.back.generic.id	    = ID_BACK;
	s_preferences.back.generic.x		= 0;
	s_preferences.back.generic.y		= 480-64;
	s_preferences.back.width  		    = 128;
	s_preferences.back.height  		    = 64;
	s_preferences.back.focuspic         = ART_BACK1;

	s_preferences.apply.generic.type     = MTYPE_BITMAP;
	s_preferences.apply.generic.name     = ART_ACCEPT0;
	s_preferences.apply.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_preferences.apply.generic.callback = Bloom_ApplyChanges;
	s_preferences.apply.generic.x        = 640;
	s_preferences.apply.generic.y        = 480-64;
	s_preferences.apply.width  		     = 128;
	s_preferences.apply.height  		 = 64;
	s_preferences.apply.focuspic         = ART_ACCEPT1;

	Menu_AddItem( &s_preferences.menu, &s_preferences.crosshair );
	Menu_AddItem( &s_preferences.menu, &s_preferences.crosshairSize );
	Menu_AddItem( &s_preferences.menu, &s_preferences.camerastyle );
	Menu_AddItem( &s_preferences.menu, &s_preferences.beamcontrol );
	Menu_AddItem( &s_preferences.menu, &s_preferences.beamdetail );
	Menu_AddItem( &s_preferences.menu, &s_preferences.particlesQuality );
	Menu_AddItem( &s_preferences.menu, &s_preferences.particlesOptimise );
//	Menu_AddItem( &s_preferences.menu, &s_preferences.motionblur );
	Menu_AddItem( &s_preferences.menu, &s_preferences.wallmarks );
	Menu_AddItem( &s_preferences.menu, &s_preferences.dynamiclights );
	Menu_AddItem( &s_preferences.menu, &s_preferences.identifytarget );
	Menu_AddItem( &s_preferences.menu, &s_preferences.synceveryframe );
//	Menu_AddItem( &s_preferences.menu, &s_preferences.useRunAnimation );
	Menu_AddItem( &s_preferences.menu, &s_preferences.allowdownload );
	Menu_AddItem( &s_preferences.menu, &s_preferences.outlines );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomQuality );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomIntensity );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomDarken );
	Menu_AddItem( &s_preferences.menu, &s_preferences.bloomAlpha );
	Menu_AddItem( &s_preferences.menu, &s_preferences.back );
	Menu_AddItem( &s_preferences.menu, &s_preferences.apply );

	Preferences_SetMenuItems();
	initialBloomQuality = s_preferences.bloomQuality.curvalue;
}
/*===============
Preferences_Cache
===============*/
void Preferences_Cache(void){
	int	n;
	trap_R_RegisterShaderNoMip(ART_FRAMEL);
	trap_R_RegisterShaderNoMip(ART_FRAMER);
	trap_R_RegisterShaderNoMip(ART_BACK0);
	trap_R_RegisterShaderNoMip(ART_BACK1);
	trap_R_RegisterShaderNoMip(ART_ACCEPT0);
	trap_R_RegisterShaderNoMip(ART_ACCEPT1);
	for(n=0;n<NUM_CROSSHAIRS;n++){
		s_preferences.crosshairShader[n] = trap_R_RegisterShaderNoMip( va("interface/hud/crosshair%c", 'a' + n ) );
	}
}
/*===============
UI_PreferencesMenu
===============*/
void UI_PreferencesMenu( void ) {
	uis.menuamount = 0;
	Preferences_MenuInit();
	UI_PushMenu( &s_preferences.menu );
}
