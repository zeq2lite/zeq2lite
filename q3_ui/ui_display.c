/*=======================================================================
DISPLAY OPTIONS MENU
=======================================================================*/
#include "ui_local.h"
#define ID_DISPLAY			11
#define ID_SOUND			12
#define ID_GENERAL			13
#define ID_BACK				16
typedef struct {
	menuframework_s	menu;
	menutext_s			display;
	menutext_s			sound;
	menutext_s			general;
	menutext_s			back;
	menuslider_s		brightness;
	menuslider_s		screensize;
	menulist_s			mode;
	menuslider_s		tq;
	menulist_s  		fs;
	menulist_s			beamdetail;
	menulist_s			particlesOptimise;
	menulist_s			particlesQuality;
	menulist_s			bloomQuality;
	menuslider_s		bloomIntensity;
	menulist_s			bloomDarken;
	menuslider_s		bloomAlpha;
	menuradiobutton_s	wallmarks;
	menuradiobutton_s	dynamiclights;
	menuradiobutton_s	motionblur;
	menuradiobutton_s	outlines;
	menulist_s  		lighting;
	menulist_s  		texturebits;
	menulist_s  		colordepth;
	menulist_s  		filter;
	menulist_s			anisotropy;
	menubitmap_s		apply;
}displayOptionsInfo_t;
static displayOptionsInfo_t	displayOptionsInfo;
static const char *particlesQuality_names[] = {"Off","Sprites","Models",0};
static const char *particlesOptimise_names[] = {"Remove After Awhile","Remove On Stop",	0};
static const char *beamdetail_names[] = {"Very Low","Low","Medium","High","Very High", 0};
static const char *bloomQuality_names[] = {"Off","Low","Medium","High","Very High", 0};
static const char *filter_names[] = {"Bilinear","Trilinear",0};
static const char *anisotropic_names[] = {"Off","2x","4x","8x","16x",0};
static const char *quality_names[] = {"Low","Medium","High",0};
static const char *enabled_names[] = {"Off","On",0};
static const char *resolutions[] = {"320 x 240","400 x 300","512 x 384","640 x 480","800 x 600","960 x 720","1024 x 768","1152 x 864","1280 x 1024","1600 x 1200","2048 x 1536","856 x 480 16:9","1280 x 720 16:9","1365 x 768 16:9","1600 x 900 16:9","1920 x 1080 16:9","1440 x 900 16:10","1680 x 1050 16:10","1920 x 1200 16:10",0};
/*=================
UI_DisplayOptionsMenu_Event
=================*/
static void UI_DisplayOptionsMenu_Setup(void){
	displayOptionsInfo.beamdetail.curvalue = Com_Clamp( 0, 4, trap_Cvar_VariableValue( "r_beamDetail" ) );
	displayOptionsInfo.particlesQuality.curvalue = Com_Clamp( 0, 2, trap_Cvar_VariableValue( "cg_particlesQuality" ) );
	displayOptionsInfo.particlesOptimise.curvalue = Com_Clamp( 0, 1, trap_Cvar_VariableValue( "cg_particlesStop" ) );
	displayOptionsInfo.wallmarks.curvalue = trap_Cvar_VariableValue( "cg_marks" ) != 0;
	displayOptionsInfo.dynamiclights.curvalue = trap_Cvar_VariableValue( "r_dynamiclight" ) != 0;
	displayOptionsInfo.motionblur.curvalue = trap_Cvar_VariableValue( "r_motionBlur" ) != 0;
	displayOptionsInfo.outlines.curvalue = trap_Cvar_VariableValue( "r_outlines" ) != 0;
	switch ( ( int ) trap_Cvar_VariableValue( "r_bloom_sample_size" ) ){
		default:
		case 0:
			displayOptionsInfo.bloomQuality.curvalue = 0;
			break;
		case 64:
			displayOptionsInfo.bloomQuality.curvalue = 1;
			break;
		case 128:
			displayOptionsInfo.bloomQuality.curvalue = 2;
			break;
		case 256:
			displayOptionsInfo.bloomQuality.curvalue = 3;
			break;
		case 512:
			displayOptionsInfo.bloomQuality.curvalue = 4;
			break;
	}
	displayOptionsInfo.bloomIntensity.curvalue = trap_Cvar_VariableValue( "r_bloom_intensity" );
	switch((int)trap_Cvar_VariableValue("r_bloom_darken")){
		default:
		case 16:
			displayOptionsInfo.bloomDarken.curvalue = 0;
			break;
		case 32:
			displayOptionsInfo.bloomDarken.curvalue = 1;
			break;
		case 64:
			displayOptionsInfo.bloomDarken.curvalue = 2;
			break;
	}
	displayOptionsInfo.bloomAlpha.curvalue = trap_Cvar_VariableValue( "r_bloom_alpha" );
	displayOptionsInfo.brightness.curvalue = trap_Cvar_VariableValue("r_gamma") * 10;
	displayOptionsInfo.screensize.curvalue = trap_Cvar_VariableValue( "cg_viewsize")/10;
	displayOptionsInfo.mode.curvalue = trap_Cvar_VariableValue("r_mode");
	displayOptionsInfo.fs.curvalue = trap_Cvar_VariableValue("r_fullscreen");
	displayOptionsInfo.tq.curvalue = 3-trap_Cvar_VariableValue("r_picmip");
	switch((int)trap_Cvar_VariableValue( "r_texturebits" ) ){
		default:
		case 0:
			displayOptionsInfo.texturebits.curvalue = 0;
			break;
		case 16:
			displayOptionsInfo.texturebits.curvalue = 1;
			break;
		case 32:
			displayOptionsInfo.texturebits.curvalue = 2;
			break;
	}
	if(!Q_stricmp( UI_Cvar_VariableString( "r_textureMode" ), "GL_LINEAR_MIPMAP_NEAREST")){displayOptionsInfo.filter.curvalue = 0;}
	else{displayOptionsInfo.filter.curvalue = 1;}
	switch((int)trap_Cvar_VariableValue( "r_ext_max_anisotropy")){
		default:
		case 0:
			displayOptionsInfo.anisotropy.curvalue = 0;
			break;
		case 2:
			displayOptionsInfo.anisotropy.curvalue = 1;
			break;
		case 4:
			displayOptionsInfo.anisotropy.curvalue = 2;
			break;
		case 8:
			displayOptionsInfo.anisotropy.curvalue = 3;
			break;
		case 16:
			displayOptionsInfo.anisotropy.curvalue = 4;
			break;
	}
}
static void UI_DisplayOptionsMenu_Apply(void *unused,int notification){
	if(notification != QM_ACTIVATED ) {return;}
	switch(displayOptionsInfo.texturebits.curvalue){
		case 0:
			trap_Cvar_SetValue("r_texturebits", 0);
			break;
		case 1:
			trap_Cvar_SetValue( "r_texturebits", 16 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_texturebits", 32 );
			break;
	}
	switch(displayOptionsInfo.colordepth.curvalue){
		case 0:
			trap_Cvar_SetValue( "r_colorbits", 0 );
			trap_Cvar_SetValue( "r_depthbits", 0 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_colorbits", 16 );
			trap_Cvar_SetValue( "r_depthbits", 16 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_colorbits", 32 );
			trap_Cvar_SetValue( "r_depthbits", 24 );
			break;
	}
	switch(displayOptionsInfo.anisotropy.curvalue){
		case 4:
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "16" );
			trap_Cvar_Set( "r_ext_max_anisotropy", "16" );
			break;
		case 3:
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "8");
			trap_Cvar_Set( "r_ext_max_anisotropy", "8" );
			break;
		case 2:
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "4" );
			trap_Cvar_Set( "r_ext_max_anisotropy", "4" );
			break;
		case 1:
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "2" );
			trap_Cvar_Set( "r_ext_max_anisotropy", "2" );
			break;
		default:
			trap_Cvar_Set( "r_ext_texture_filter_anisotropic", "0" );
			trap_Cvar_Set( "r_ext_max_anisotropy", "0" );
			break;
	}
	switch ( displayOptionsInfo.bloomQuality.curvalue  ){
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
	switch ( displayOptionsInfo.bloomDarken.curvalue  ){
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
	displayOptionsInfo.tq.curvalue = (int)(displayOptionsInfo.tq.curvalue + 0.5);
	trap_Cvar_SetValue( "r_bloom_alpha", displayOptionsInfo.bloomAlpha.curvalue);
	trap_Cvar_SetValue( "r_outlines", displayOptionsInfo.outlines.curvalue );
	trap_Cvar_SetValue("r_gamma",displayOptionsInfo.brightness.curvalue / 10.0f);
	trap_Cvar_SetValue( "r_bloom_intensity", displayOptionsInfo.bloomIntensity.curvalue);
	trap_Cvar_SetValue("cg_viewsize", displayOptionsInfo.screensize.curvalue * 10);		
	trap_Cvar_SetValue( "cg_particlesQuality", displayOptionsInfo.particlesQuality.curvalue );
	trap_Cvar_SetValue( "cg_particlesStop", displayOptionsInfo.particlesOptimise.curvalue );
	trap_Cvar_SetValue( "r_motionBlur", displayOptionsInfo.motionblur.curvalue );
	trap_Cvar_SetValue( "cg_marks", displayOptionsInfo.wallmarks.curvalue );
	trap_Cvar_SetValue( "r_dynamiclight", displayOptionsInfo.dynamiclights.curvalue );
	trap_Cvar_SetValue( "r_picmip", 3 - displayOptionsInfo.tq.curvalue );
	trap_Cvar_SetValue( "r_mode", displayOptionsInfo.mode.curvalue );
	trap_Cvar_SetValue( "r_fullscreen", displayOptionsInfo.fs.curvalue );
	trap_Cvar_SetValue( "r_beamDetail", displayOptionsInfo.beamdetail.curvalue);
	if(displayOptionsInfo.filter.curvalue ){trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR" );}
	else{trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );}
	trap_Cmd_ExecuteText(EXEC_APPEND,"vid_restart\n");
}
static void UI_DisplayOptionsMenu_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED){return;}
	switch(((menucommon_s*)ptr)->id){
		case ID_DISPLAY:
			UI_DisplayOptionsMenu();
			break;
		case ID_SOUND:
			UI_SoundOptionsMenu();
			break;
		case ID_GENERAL:
			UI_PreferencesMenu();
			break;
		case ID_BACK:
			UI_PopMenu();
			break;
	}
}
/*===============
UI_DisplayOptionsMenu_Init
===============*/
static void UI_DisplayOptionsMenu_Init( void ) {
	int x = 28;
	int	y = 119;
	int	offset = 38;
	int	style = UI_LEFT | UI_DROPSHADOW | UI_TINYFONT;
	memset(&displayOptionsInfo,0,sizeof(displayOptionsInfo));
	displayOptionsInfo.menu.wrapAround = qtrue;
	displayOptionsInfo.menu.fullscreen = qtrue;
	displayOptionsInfo.display.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.display.generic.flags		= QMF_LEFT_JUSTIFY;
	displayOptionsInfo.display.generic.id			= ID_DISPLAY;
	displayOptionsInfo.display.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.display.generic.x			= x;
	displayOptionsInfo.display.generic.y			= y;
	displayOptionsInfo.display.string				= "DISPLAY";
	displayOptionsInfo.display.style				= style;
	displayOptionsInfo.display.color				= color_white;
	y+=offset;
	displayOptionsInfo.general.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.general.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.general.generic.id			= ID_GENERAL;
	displayOptionsInfo.general.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.general.generic.x			= x;
	displayOptionsInfo.general.generic.y			= y;
	displayOptionsInfo.general.string				= "GENERAL";
	displayOptionsInfo.general.style				= style;
	displayOptionsInfo.general.color				= color_white;
	y+=offset;
	displayOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.sound.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.sound.generic.id				= ID_SOUND;
	displayOptionsInfo.sound.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.sound.generic.x				= x;
	displayOptionsInfo.sound.generic.y				= y;
	displayOptionsInfo.sound.string					= "SOUND";
	displayOptionsInfo.sound.style					= style;
	displayOptionsInfo.sound.color					= color_white;
	y+=offset;
	displayOptionsInfo.back.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.back.generic.id				= ID_BACK;
	displayOptionsInfo.back.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.back.generic.x				= x;
	displayOptionsInfo.back.generic.y				= y;
	displayOptionsInfo.back.string					= "BACK";
	displayOptionsInfo.back.style					= style;
	displayOptionsInfo.back.color					= color_white;
	x = 285;
	y = 135;
	offset = 15;
	displayOptionsInfo.mode.generic.type			= MTYPE_SPINCONTROL;
	displayOptionsInfo.mode.generic.name     		= "Resolution:";
	displayOptionsInfo.mode.generic.x				= x;
	displayOptionsInfo.mode.generic.y				= y;
	displayOptionsInfo.mode.itemnames				= resolutions;
	displayOptionsInfo.mode.generic.callback		= UI_DisplayOptionsMenu_Event;
	y += offset;
	displayOptionsInfo.fs.generic.type				= MTYPE_SPINCONTROL;
	displayOptionsInfo.fs.generic.name				= "Fullscreen:";
	displayOptionsInfo.fs.generic.x					= x;
	displayOptionsInfo.fs.generic.y	      			= y;
	displayOptionsInfo.fs.itemnames	      			= enabled_names;
	y += offset;
	displayOptionsInfo.brightness.generic.type		= MTYPE_SLIDER;
	displayOptionsInfo.brightness.generic.name		= "Brightness:";
	displayOptionsInfo.brightness.generic.x			= x-8;
	displayOptionsInfo.brightness.generic.y			= y;
	displayOptionsInfo.brightness.minvalue			= 5;
	displayOptionsInfo.brightness.maxvalue			= 20;
	y+=offset;
	if( !uis.glconfig.deviceSupportsGamma ){displayOptionsInfo.brightness.generic.flags |= QMF_GRAYED;}
	displayOptionsInfo.screensize.generic.type		= MTYPE_SLIDER;
	displayOptionsInfo.screensize.generic.name		= "Screen Scale:";
	displayOptionsInfo.screensize.generic.x			= x-8;
	displayOptionsInfo.screensize.generic.y			= y;
	displayOptionsInfo.screensize.minvalue			= 3;
    displayOptionsInfo.screensize.maxvalue			= 10;
	y += offset;
	displayOptionsInfo.tq.generic.type				= MTYPE_SLIDER;
	displayOptionsInfo.tq.generic.name				= "Texture Detail:";
	displayOptionsInfo.tq.generic.x		 			= x-8;
	displayOptionsInfo.tq.generic.y		 			= y;
	displayOptionsInfo.tq.minvalue     				= 0;
	displayOptionsInfo.tq.maxvalue     				= 3;
	displayOptionsInfo.tq.generic.callback			= UI_DisplayOptionsMenu_Event;
	y += offset;
	displayOptionsInfo.texturebits.generic.type		= MTYPE_SPINCONTROL;
	displayOptionsInfo.texturebits.generic.name		= "Texture Quality:";;
	displayOptionsInfo.texturebits.generic.x	    = x;
	displayOptionsInfo.texturebits.generic.y	    = y;
	displayOptionsInfo.texturebits.itemnames		= quality_names;
	y += offset;
	displayOptionsInfo.filter.generic.type			= MTYPE_SPINCONTROL;
	displayOptionsInfo.filter.generic.name			= "Texture Filter:";
	displayOptionsInfo.filter.generic.x				= x;
	displayOptionsInfo.filter.generic.y				= y;
	displayOptionsInfo.filter.itemnames				= filter_names;
	y += offset;
	displayOptionsInfo.anisotropy.generic.type		= MTYPE_SPINCONTROL;
	displayOptionsInfo.anisotropy.generic.name		= "Texture Anisotropy:";
	displayOptionsInfo.anisotropy.generic.x			= x;
	displayOptionsInfo.anisotropy.generic.y			= y;
	displayOptionsInfo.anisotropy.itemnames 		= anisotropic_names;
	y += offset;
	displayOptionsInfo.beamdetail.generic.type		= MTYPE_SPINCONTROL;
	displayOptionsInfo.beamdetail.generic.name		= "Beam Detail:";
	displayOptionsInfo.beamdetail.generic.x			= x;
	displayOptionsInfo.beamdetail.generic.y			= y;
	displayOptionsInfo.beamdetail.itemnames			= beamdetail_names;
	y += offset;
	displayOptionsInfo.particlesQuality.generic.type= MTYPE_SPINCONTROL;
	displayOptionsInfo.particlesQuality.generic.name= "Particle Quality:";
	displayOptionsInfo.particlesQuality.generic.x	= x;
	displayOptionsInfo.particlesQuality.generic.y	= y;
	displayOptionsInfo.particlesQuality.itemnames	= particlesQuality_names;
	y += offset;
	displayOptionsInfo.particlesOptimise.generic.type= MTYPE_SPINCONTROL;
	displayOptionsInfo.particlesOptimise.generic.name= "Particle Physics:";
	displayOptionsInfo.particlesOptimise.generic.x	 = x;
	displayOptionsInfo.particlesOptimise.generic.y	 = y;
	displayOptionsInfo.particlesOptimise.itemnames	 = particlesOptimise_names;
	y += offset;
	displayOptionsInfo.dynamiclights.generic.type	= MTYPE_RADIOBUTTON;
	displayOptionsInfo.dynamiclights.generic.name	= "Dynamic Lights:";
	displayOptionsInfo.dynamiclights.generic.x		= x;
	displayOptionsInfo.dynamiclights.generic.y		= y;
	y += BIGCHAR_HEIGHT;
	displayOptionsInfo.wallmarks.generic.type		= MTYPE_RADIOBUTTON;
	displayOptionsInfo.wallmarks.generic.name		= "Decals:";
	displayOptionsInfo.wallmarks.generic.x			= x;
	displayOptionsInfo.wallmarks.generic.y			= y;
	y += offset;
	displayOptionsInfo.outlines.generic.type		= MTYPE_RADIOBUTTON;
	displayOptionsInfo.outlines.generic.name		= "Outlines:";
	displayOptionsInfo.outlines.generic.x			= x;
	displayOptionsInfo.outlines.generic.y			= y;
	y += offset;
	displayOptionsInfo.bloomQuality.generic.type	= MTYPE_SPINCONTROL;
	displayOptionsInfo.bloomQuality.generic.name	= "Bloom Quality:";
	displayOptionsInfo.bloomQuality.generic.x		= x;
	displayOptionsInfo.bloomQuality.generic.y		= y;
	displayOptionsInfo.bloomQuality.itemnames		= quality_names;
	y += offset;
	displayOptionsInfo.bloomDarken.generic.type		= MTYPE_SPINCONTROL;
	displayOptionsInfo.bloomDarken.generic.name		= "Bloom Darken:";
	displayOptionsInfo.bloomDarken.generic.x		= x;
	displayOptionsInfo.bloomDarken.generic.y		= y;
	displayOptionsInfo.bloomDarken.itemnames		= quality_names;
	y += offset;
	displayOptionsInfo.bloomIntensity.generic.type	= MTYPE_SLIDER;
	displayOptionsInfo.bloomIntensity.generic.name	= "Bloom Intensity:";
	displayOptionsInfo.bloomIntensity.generic.x		= x-8;
	displayOptionsInfo.bloomIntensity.generic.y		= y;
	displayOptionsInfo.bloomIntensity.minvalue		= 0;
	displayOptionsInfo.bloomIntensity.maxvalue		= 1;
	y += offset;
	displayOptionsInfo.bloomAlpha.generic.type		= MTYPE_SLIDER;
	displayOptionsInfo.bloomAlpha.generic.name		= "Bloom Alpha:";
	displayOptionsInfo.bloomAlpha.generic.x			= x-8;
	displayOptionsInfo.bloomAlpha.generic.y			= y;
	displayOptionsInfo.bloomAlpha.minvalue			= 0;
	displayOptionsInfo.bloomAlpha.maxvalue			= 1;

	displayOptionsInfo.apply.generic.type     		= MTYPE_BITMAP;
	displayOptionsInfo.apply.generic.name     		= "applyButton";
	displayOptionsInfo.apply.generic.callback		= UI_DisplayOptionsMenu_Apply;
	displayOptionsInfo.apply.generic.x       		= 395;
	displayOptionsInfo.apply.generic.y       		= 380;
	displayOptionsInfo.apply.width  			    = 117;
	displayOptionsInfo.apply.height  		 		= 55;
	displayOptionsInfo.apply.focuspic         		= "applyButton";

	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.display );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.general );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.sound );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.back );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.brightness );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.screensize );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.mode );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.fs );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.tq );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.texturebits );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.filter );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.anisotropy );

	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.beamdetail );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.particlesQuality );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.particlesOptimise );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.wallmarks );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.dynamiclights );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.outlines );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.bloomQuality );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.bloomIntensity );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.bloomDarken );
	Menu_AddItem( &displayOptionsInfo.menu, &displayOptionsInfo.bloomAlpha );

	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.apply);
	UI_DisplayOptionsMenu_Setup();
}
void UI_DisplayOptionsMenu_Cache(void){}
/*===============
UI_DisplayOptionsMenu
===============*/
void UI_DisplayOptionsMenu( void ) {
	uis.menuamount = 4;
	uis.hideEarth = qtrue;
	uis.showFrame = qtrue;
	UI_DisplayOptionsMenu_Init();
	UI_PushMenu(&displayOptionsInfo.menu);
	Menu_SetCursorToItem(&displayOptionsInfo.menu,&displayOptionsInfo.display);
}
