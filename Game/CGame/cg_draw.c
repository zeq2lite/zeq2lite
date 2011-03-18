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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../UI/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];

playerState_t lockedTargetPS;

#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
// FIXME: see ui_main.c, same problem
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
// TTimo: FIXME
//	const unsigned char *s = text;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h,qtrue);
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
// TTimo: FIXME
//		const unsigned char *s = text;
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(qfalse,x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic(qfalse, x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // !MISSIONPACK


/*
=================
CG_DrawHorGauge

=================
*/
void CG_DrawHorGauge( float x, float y, float w, float h, vec4_t color_bar, vec4_t color_empty, int value, int maxvalue, qboolean reversed) {
	float pct;
	float bar_w;

	pct = (float)value / (float)maxvalue;
	bar_w = w * pct;

	if (color_empty[3]) {
		trap_R_SetColor( color_empty );
		if (!reversed) {
			CG_DrawPic(qfalse, x + bar_w, y, w - bar_w, h, cgs.media.whiteShader );
		} else {
			CG_DrawPic(qfalse, x - bar_w, y, w + bar_w, h, cgs.media.whiteShader );
		}
	}
	trap_R_SetColor( color_bar );

	if (bar_w > w) {
		bar_w = w;
	}

	if (bar_w == 0) {
		trap_R_SetColor( NULL );
		return;
	}

	if (!reversed) {
		CG_DrawPic(qfalse, x, y, bar_w, h, cgs.media.whiteShader );
	} else {
		CG_DrawPic(qfalse, x + w - bar_w, y, bar_w, h, cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );
}
void CG_DrawDiffGauge(float x,float y,float width,float height,vec4_t color,vec4_t empty,int base,int value,int maxValue,int direction){
	float percent;
	int newWidth,newX,difference;
	difference = base - value;
	if(difference > 0 && direction <= 0){return;}
	if(difference < 0 && direction >= 0){return;}
	percent = ((float)value / (float)maxValue);
	newX = x + ((float)(width * percent));
	newWidth = ((float)difference / (float)maxValue) * width;
	CG_DrawHorGauge(newX,y,newWidth,height,color,empty,1,1,qfalse);
}
void CG_DrawRightGauge(float x,float y,float width,float height,vec4_t color,vec4_t empty,int value,int maxValue){
	float percent;
	int newWidth,newX,change;
	percent = ((float)value / (float)maxValue);
	change = width * percent;
	newX = x + change;
	newWidth = width - change;
	CG_DrawHorGauge(newX,y,newWidth,height,color,empty,1,1,qfalse);
}
void CG_DrawReverseGauge(float x,float y,float width,float height,vec4_t color,vec4_t empty,int value,int maxValue){
	float percent;
	int newWidth,newHeight,newX,newY;
	percent = (float)value / (float)maxValue;
	newWidth = width * percent;
	newX = (x + width) - newWidth;
	newHeight = height * percent;
	newY = (y + height) - newHeight;
	CG_DrawHorGauge(newX,newY,newWidth,newHeight,color,empty,1,1,qfalse);
}

/*
==================
CG_DrawVertGauge

==================
*/
void CG_DrawVertGauge( float x, float y, float w, float h, vec4_t color_bar, vec4_t color_empty, int value, int maxvalue, qboolean reversed) {
	float pct;
	float bar_h;

	pct = (float)value / (float)maxvalue;
	bar_h = h * pct;

	if (color_empty[3]) {
		trap_R_SetColor( color_empty );
		if (reversed) {
			CG_DrawPic(qfalse, x, y + bar_h, w, h - bar_h, cgs.media.whiteShader );
		} else {
			CG_DrawPic(qfalse, x, y, w, h - bar_h, cgs.media.whiteShader );
		}
	}
	trap_R_SetColor( color_bar );

	if (bar_h > h) {
		bar_h = h;
	}

	if (bar_h == 0) {
		trap_R_SetColor( NULL );
		return;
	}

	if (reversed) {
		CG_DrawPic(qfalse, x, y, w, bar_h, cgs.media.whiteShader );
	} else {
		CG_DrawPic(qfalse, x, y + h - bar_h, w, bar_h, cgs.media.whiteShader );
	}

	trap_R_SetColor( NULL );
}



/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h,qtrue);

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;
	ci = &cgs.clientinfo[ clientNum ];
	CG_DrawPic(qfalse, x, y, w, h, ci->tierConfig[ci->tierCurrent].icon);
/*
	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel[ci->tierCurrent];
		if ( !cm ) {
			return;
		}
		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModel( x, y, w, h, ci->headModel[ci->tierCurrent], ci->headSkin[ci->tierCurrent], origin, headAngles );
	} else if ( cg_drawIcons.integer ) {
		CG_DrawPic(qfalse, x, y, w, h, ci->modelIcon );
	}
*/
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, 480 - size, size, size, 
				cg.snap->ps.clientNum, angles );
}
#endif // MISSIONPACK

/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, 480 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK

/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	trap_R_SetColor( NULL );
}


/*================
CG_DRAWCHAT
================*/
void strrep(char *str, char old, char new)  {
    char *pos;
    while (1)  {
        pos = strchr(str, old);
        if (pos == NULL)  {
            break;
        }
        *pos = new;
    }
}
void CG_CheckChat(void){
	int index,offset;
	vec3_t angles;
	int yStart = cg.predictedPlayerState.lockedTarget ? 330 : 0;
	int yOffset = cg.predictedPlayerState.lockedTarget ? -40 : 40;
	if(cg.time < cgs.chatTimer){
		for(index=0;index<3;++index){
			if(cgs.messageClient[index] >= 0){
				CG_DrawPic(qfalse,-15,-15+yStart+(yOffset*index),369,75,cgs.media.chatBackgroundShader);
				CG_DrawHead(7,7+yStart+(yOffset*index),32,32,cgs.messageClient[index],angles);
				CG_DrawSmallStringCustom(45,16+yStart+(yOffset*index),8,8,cgs.messages[index],1.0,4);
			}
		}
	}
	else{
		for(index=0;index<3;++index){
			cgs.messageClient[index] = -1;
			strcpy(cgs.messages[index],"");
		}
	}
}
void CG_DrawChat(char *text){
	int clientNum,index,safeIndex;
	char cleaned[256];
	char name[14];
	char *safe;
	char find = ':';
	char find2[] = "^7";
	char replace = ' ';
	safe = text;
	strrep(safe, find, replace);
	strcpy(cleaned, text);
	strrep(safe, *find2, replace);
	cgs.chatTimer = cg.time + cg_chatTime.integer;
	strcpy(name,COM_Parse(&safe));
	for(safeIndex=0; safeIndex<3; ++safeIndex){if(!strcmp(cgs.messages[safeIndex],"")){break;}}
	if(safeIndex>=2 && cgs.messageClient[2]>= 0){
		safeIndex = 2;
		cgs.messageClient[0] = cgs.messageClient[1];
		cgs.messageClient[1] = cgs.messageClient[2];
		strcpy(cgs.messages[0],cgs.messages[1]);
		strcpy(cgs.messages[1],cgs.messages[2]);
	}
	for(clientNum=0;clientNum<MAX_CLIENTS;++clientNum){
		if(!strcmp(name,cgs.clientinfo[clientNum].name)){
			cgs.messageClient[safeIndex] = clientNum;
			break;
		}
	}
	strcpy(cgs.messages[safeIndex],cleaned);
}
/*================
CG_HUD
================*/
void CG_DrawHUD(playerState_t *ps,int clientNum,int x,int y,qboolean flipped){
	const char	*powerLevelString;
	int 		powerLevelOffset;
	long	 	powerLevelDisplay;
	float		multiplier;
	vec4_t	powerColor = {0.0f,0.588f,1.0f,1.0f};
	vec4_t	dullColor = {0.188f,0.278f,0.345f,1.0f};
	vec4_t	limitColor = {1.0f,0.1f,0.0f,1.0f};
	vec4_t	beyondFatigueColor = {0.9f,0.5f,0.0f,1.0f};
	vec4_t	beyondHealthColor = {0.8f,0.2f,0.2f,1.0f};
	vec4_t	healthFatigueColor = {1.0f,0.4f,0.2f,1.0f};
	vec4_t	plFatigueHealthColor = {0.5f,0.16f,0.16f,1.0f};
	vec4_t	plFatigueColor = {0.4f,0.4f,0.5f,1.0f};
	vec4_t	clearColor = {0.0f,0.0f,0.0f,0.0f};
	vec3_t	angles;
	CG_DrawHorGauge(x+60,y+41,200,16,powerColor,dullColor,ps->powerLevel[plCurrent],ps->powerLevel[plMaximum],qfalse);	
	CG_DrawRightGauge(x+60,y+41,200,16,plFatigueColor,plFatigueColor,ps->powerLevel[plFatigue],ps->powerLevel[plMaximum]);
	CG_DrawRightGauge(x+60,y+41,200,16,limitColor,limitColor,ps->powerLevel[plHealth],ps->powerLevel[plMaximum]);
	CG_DrawDiffGauge(x+60,y+41,200,16,beyondFatigueColor,beyondFatigueColor,ps->powerLevel[plCurrent],ps->powerLevel[plFatigue],ps->powerLevel[plMaximum],1);
	CG_DrawDiffGauge(x+60,y+41,200,16,plFatigueHealthColor,plFatigueHealthColor,ps->powerLevel[plFatigue],ps->powerLevel[plHealth],ps->powerLevel[plMaximum],1);
	CG_DrawDiffGauge(x+60,y+41,200,16,beyondHealthColor,beyondHealthColor,ps->powerLevel[plCurrent],ps->powerLevel[plHealth],ps->powerLevel[plMaximum],1);
	if((ps->powerLevel[plCurrent] > ps->powerLevel[plFatigue]) && (ps->powerLevel[plFatigue] > ps->powerLevel[plHealth])){
		CG_DrawDiffGauge(x+60,y+41,200,16,healthFatigueColor,healthFatigueColor,ps->powerLevel[plCurrent],ps->powerLevel[plFatigue],ps->powerLevel[plMaximum],1);
	}
	CG_DrawPic(qfalse,x,y,288,72,cgs.media.hudShader);
	CG_DrawHead(x+6,y+22,50,50,clientNum,angles);
	if(ps->powerLevel[plCurrent] == ps->powerLevel[plMaximum] && ps->bitFlags & usingAlter){
		CG_DrawPic(qfalse,x+243,y+25,40,44,cgs.media.breakLimitShader);
	}
	if(ps->powerLevel[plCurrent] == 9001){
		powerLevelString = "Over ^3NINE-THOUSAND!!!";
	}
	multiplier = cgs.clientinfo[clientNum].tierConfig[cgs.clientinfo[clientNum].tierCurrent].hudMultiplier;
	if(multiplier <= 0){
		multiplier = 1.0;
	}
	powerLevelDisplay = (float)ps->powerLevel[plCurrent] * multiplier;
	powerLevelString = powerLevelDisplay >= 1000000 ? va("%.1f ^3mil",(float)powerLevelDisplay / 1000000.0) : va("%i",powerLevelDisplay);
	powerLevelOffset = (Q_PrintStrlen(powerLevelString)-2)*8;
	CG_DrawSmallStringHalfHeight(x+239-powerLevelOffset,y+44,powerLevelString,1.0F);
}
static void CG_DrawStatusBar( void ) {
	centity_t		*cent;
	playerState_t	*ps;
	float			tierLast,tierNext,tier;
	int				base;
	clientInfo_t	*ci;
	cg_userWeapon_t	*weaponGraphics;
	tierConfig_cg	*activeTier;

	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;
	if((ci->lockStartTimer > cg.time) /*(&& cg.time > ci->lockStartTimer - 500)*/){
		CG_DrawPic(qfalse,0,0,640,480,cgs.media.speedLineSpinShader);
	}
	if(ps->bitFlags & usingBoost){
		CG_DrawPic(qfalse,0,0,640,480,cgs.media.speedLineShader);
	}
	if(cg_drawStatus.integer == 0){return;}
	cent = &cg_entities[cg.snap->ps.clientNum];
	tier = (float)ps->powerLevel[plTierCurrent];
	CG_CheckChat();
	if(ps->lockedTarget > 0 && cgs.clientinfo[ps->lockedTarget-1].infoValid){
		lockedTargetPS.clientNum = ps->lockedTarget-1;
		lockedTargetPS.powerLevel[plCurrent] = ps->lockedPlayerData[lkPowerCurrent];
		lockedTargetPS.powerLevel[plHealth] = ps->lockedPlayerData[lkPowerHealth];
		lockedTargetPS.powerLevel[plMaximum] = ps->lockedPlayerData[lkPowerMaximum];
		lockedTargetPS.powerLevel[plFatigue] = lockedTargetPS.powerLevel[plMaximum];
		lockedTargetPS.powerLevel[plTierCurrent] = cgs.clientinfo[lockedTargetPS.clientNum].tierCurrent;
		CG_DrawHUD(ps,ps->clientNum,0,0,qfalse);
		CG_DrawHUD(&lockedTargetPS,lockedTargetPS.clientNum,320,0,qtrue);
	}
	else{
		CG_DrawHUD(ps,ps->clientNum,0,408,qfalse);
		if(tier){
			activeTier = &ci->tierConfig[ci->tierCurrent];
			tierLast = 32767;
			if(activeTier->sustainCurrent && activeTier->sustainCurrent < tierLast){tierLast = (float)activeTier->sustainCurrent;}
			if(activeTier->sustainFatigue && activeTier->sustainFatigue < tierLast){tierLast = (float)activeTier->sustainFatigue;}
			if(activeTier->sustainHealth && activeTier->sustainHealth < tierLast){tierLast = (float)activeTier->sustainHealth;}
			if(activeTier->sustainMaximum && activeTier->sustainMaximum < tierLast){tierLast = (float)activeTier->sustainMaximum;}
			if(tierLast < 32767){
				tierLast = tierLast / (float)ps->powerLevel[plMaximum];
				CG_DrawPic(qfalse,(187*tierLast)+60,428,13,38,cgs.media.markerDescendShader);
			}
		}
		if(tier < ps->powerLevel[plTierTotal]){
			activeTier = &ci->tierConfig[ci->tierCurrent+1];
			tierNext = 0;
			if(activeTier->requirementCurrent && activeTier->requirementCurrent > tierNext){tierNext = (float)activeTier->requirementCurrent;}
			if(activeTier->requirementFatigue && activeTier->requirementFatigue > tierNext){tierNext = (float)activeTier->requirementFatigue;}
			if(activeTier->requirementMaximum && activeTier->requirementMaximum > tierNext){tierNext = (float)activeTier->requirementMaximum;}
			if(activeTier->requirementHealth && activeTier->requirementHealth > tierNext){tierNext = (float)activeTier->requirementHealth;}
			if(tierNext){
				tierNext = tierNext / (float)ps->powerLevel[plMaximum];
				if(tierNext < 1.0){
					CG_DrawPic(qfalse,(187*tierNext)+60,428,13,38,cgs.media.markerAscendShader);
				}
			}
		}
	}
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define	FPS_FRAMES	16
static float CG_DrawFPS( float y ) {
	char		*s;
	int			w;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int			i, total;
	int			fps;
	static	int	previous, lastupdate;
	int			t, frameTime;
	const int	xOffset = 0;

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;
	if (t - lastupdate > 50)	//don't sample faster than this
	{
		lastupdate = t;
		previousTimes[index % FPS_FRAMES] = frameTime;
		index++;
	}
	// average multiple frames together to smooth changes out a bit
	total = 0;
	for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
		total += previousTimes[i];
	}
	if ( !total ) {
		total = 1;
	}
	fps = 1000 * FPS_FRAMES / total;

	s = va( "%ifps", fps );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w + xOffset, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=================
CG_DrawTeamOverlay
=================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer){
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt(-1, xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
				len = CG_DrawStrlen(p);
				if (len > lwidth)
					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt(-1, xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->powerLevel, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->powerLevel,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt(-1, xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

//			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
//				CG_DrawPic(qfalse, xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
//					cg_weapons[ci->curWeapon].weaponIcon );
//			} else {
//				CG_DrawPic(qfalse, xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
//					cgs.media.deferShader );
//			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight( void ) {
	float	y;

	y = 0;

#if MAPLENSFLARES	// JUHOX: draw lens flare editor title
	if (cgs.editMode == EM_mlf) {
		CG_DrawBigString(640 - 17 * BIGCHAR_WIDTH, y, "lens flare editor", 1);
		y += BIGCHAR_HEIGHT;
	}
#endif

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if ( cg_drawFPS.integer ) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	int			x, w;
	int			v;
	vec4_t		color;
	float		y1;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM ) {
		x = 640;
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic(qfalse, x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic(qfalse, x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawBigString( x + 4, y, s, 1.0F);
		if ( cgs.gametype >= GT_CTF ) {
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}
		if ( v ) {
			s = va( "%2i", v );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	} else {
		qboolean	spectator;

		x = 640;
		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s2 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic(qfalse, x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			s = va( "%2i", s1 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.0f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic(qfalse, x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			CG_DrawBigString( x + 4, y, s, 1.0F);
		}

	}

	return y1 - 8;
}
#endif // ! MISSIONPACK

/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	int		x;
	int		color;
	float	size;
	float	f;
	static float colors[2][4] = { 
    { 0.2f, 1.0f, 0.2f, 1.0f } , 
    { 1.0f, 0.2f, 0.2f, 1.0f } 
  };

	ps = &cg.snap->ps;

	if ( ps->powerLevel[plCurrent] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = 640 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK

/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	// JUHOX: don't draw scores in lens flare editor
#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) return;
#endif

	y = 480 - ICON_SIZE;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

	//y = CG_DrawScores( y );
	//y = CG_DrawPowerups( y );
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	int w, h;
	int i, len;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt(-1,CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}
#endif // !MISSIONPACK

/*==============
CG_DrawDisconnect
Should we draw something differnet for long lag vs no packets?
==============*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

	x = 640 - 48;
	y = 480 - 48;

	CG_DrawPic(qfalse, x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		x, y, w;
#ifdef MISSIONPACK
	int h;
#endif
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value, 200 );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		w = cg.centerPrintCharWidth * CG_DrawStrlen( linebuffer );

		x = ( SCREEN_WIDTH - w ) / 2;

		CG_DrawStringExt(-1, x, y, linebuffer, color, qfalse, qtrue,
			cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0 );

		y += cg.centerPrintCharWidth * 1.5;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/

// NOTE: Prototype so it's known
static void CG_DrawCrosshairChargeBars( float x_cross, float y_cross );

/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float			w, h;
	qhandle_t		hShader;
	float			f;
	float			x, y;
	int				ca;
	int				i;
	trace_t			trace;
	playerState_t	*ps;
	vec3_t			muzzle, forward, up;
	vec3_t			start, end;
	vec4_t			lockOnEnemyColor	= {1.0f,0.0f,0.0f,1.0f};
	vec4_t			lockOnAllyColor		= {0.0f,1.0f,0.0f,1.0f};
	vec4_t			chargeColor			= {0.5f,0.5f,1.0f,1.0f};

	radar_t			cg_playerOrigins[MAX_CLIENTS];

	if ( !cg_drawCrosshair.integer || cg.snap->ps.lockedTarget > 0 ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	ps = &cg.predictedPlayerState;

	if(ps->bitFlags & usingMelee){return;}

	AngleVectors( ps->viewangles, forward, NULL, up );
	VectorCopy( ps->origin, muzzle );
	VectorMA( muzzle, ps->viewheight, up, muzzle );
	VectorMA( muzzle, 14, forward, muzzle );
		
	VectorCopy( muzzle, start );
	VectorMA( start, 131072, forward, end );

	CG_Trace( &trace, start, NULL, NULL, end, cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );	
	if ( !CG_WorldCoordToScreenCoordFloat( trace.endpos, &x, &y ) ) {
		return;
	}

	w = h = (cg_crosshairSize.value * 8 + 8);

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	ca = cg_drawCrosshair.integer;

	if (ca < 0) {
		ca = 0;
	}

	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	if ( cg.snap->ps.currentSkill[WPSTAT_BITFLAGS] & WPF_READY || cg.snap->ps.currentSkill[WPSTAT_ALT_BITFLAGS] & WPF_READY) {
		trap_R_SetColor( chargeColor );
	}
	else if (cg.crosshairClientNum > 0 && cg.crosshairClientNum <= MAX_CLIENTS || ps->lockedTarget > 0) {
		if( cgs.clientinfo[cg.crosshairClientNum].team == cg.snap->ps.persistant[PERS_TEAM] && cgs.clientinfo[cg.crosshairClientNum].team != TEAM_FREE  ) {
			trap_R_SetColor( lockOnAllyColor );
		}
		else{
			trap_R_SetColor( lockOnEnemyColor );
		}
	}
	else{
		trap_R_SetColor( NULL );
	}
	CG_DrawPic(qfalse, x - 0.5f * w, y - 0.5f * h, w, h, hShader );
	trap_R_SetColor( NULL );
	CG_DrawCrosshairChargeBars( x, y );	
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity(void) {
	trace_t			trace,trace2;
	vec3_t			start,end,muzzle,forward,up,minSize,maxSize;
	playerState_t	*ps;

	ps = &cg.predictedPlayerState;

	AngleVectors(ps->viewangles,forward,NULL,up);
	VectorCopy(ps->origin, muzzle );
	VectorMA(muzzle,ps->viewheight,up,muzzle);
	VectorMA(muzzle,14,forward,muzzle);
	VectorCopy(muzzle,start);
	VectorMA(start,131072,forward,end);

	minSize[0] = -(float)cg_lockonDistance.value;
	minSize[1] = -(float)cg_lockonDistance.value;
	minSize[2] = -(float)cg_lockonDistance.value;
	maxSize[0] = -minSize[0];
	maxSize[1] = -minSize[1];
	maxSize[2] = -minSize[2];
	
	CG_Trace(&trace,start,minSize,maxSize,end,cg.snap->ps.clientNum,CONTENTS_BODY);

	if (trace.entityNum>=MAX_CLIENTS){cg.crosshairClientNum= -1;return;}

	cg.crosshairClientNum=trace.entityNum;
	cg.crosshairClientTime=cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000, 200 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;

#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);

#else
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );
#endif

	trap_R_SetColor( NULL );
}



/*
==========================
CG_DrawCrosshairChargeBars
==========================
*/
static void CG_DrawCrosshairChargeBars( float x_cross, float y_cross ) {}

//==============================================================================


/*
=================
JUHOX: CG_DrawLensFlareEffectList
=================
*/
#if MAPLENSFLARES
static void CG_DrawLensFlareEffectList(void) {
	int firstEffect;
	int y;
	int i;

	y = 480 - 12 * TINYCHAR_HEIGHT;

	firstEffect = cg.lfEditor.selectedEffect - 5;
	for (i = 0; i < 12; i++) {
		int effectNum;

		effectNum = firstEffect + i;
		if (effectNum >= 0 && effectNum < cgs.numLensFlareEffects) {
			lensFlareEffect_t* lfeff;
			int width;
			const float* color;

			lfeff = &cgs.lensFlareEffects[effectNum];
			width = CG_DrawStrlen(lfeff->name) * TINYCHAR_WIDTH;
			color = i == 5? colorWhite : colorMdGrey;
			CG_DrawStringExt(-1,640 - width, y, lfeff->name, color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		y += TINYCHAR_HEIGHT;
	}
}
#endif

/*
=================
JUHOX: CG_DrawCopyOptions
=================
*/
#if MAPLENSFLARES
static void CG_DrawCopyOptions(void) {
	int y;
	char buf[256];

	y = 480;

	y -= TINYCHAR_HEIGHT;	// 9
	y -= TINYCHAR_HEIGHT;	// 8
	y -= TINYCHAR_HEIGHT;	// 7

	y -= TINYCHAR_HEIGHT;
	Com_sprintf(buf, sizeof(buf), "[6] paste entity angle = %s", cg.lfEditor.copyOptions & LFECO_SPOT_ANGLE? "on" : "off");
	CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	y -= TINYCHAR_HEIGHT;
	Com_sprintf(buf, sizeof(buf), "[5] paste direction    = %s", cg.lfEditor.copyOptions & LFECO_SPOT_DIR? "on" : "off");
	CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	y -= TINYCHAR_HEIGHT;
	Com_sprintf(buf, sizeof(buf), "[4] paste light radius = %s", cg.lfEditor.copyOptions & LFECO_LIGHTRADIUS? "on" : "off");
	CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	y -= TINYCHAR_HEIGHT;
	Com_sprintf(buf, sizeof(buf), "[3] paste vis radius   = %s", cg.lfEditor.copyOptions & LFECO_VISRADIUS? "on" : "off");
	CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	y -= TINYCHAR_HEIGHT;
	Com_sprintf(buf, sizeof(buf), "[2] paste effect       = %s", cg.lfEditor.copyOptions & LFECO_EFFECT? "on" : "off");
	CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

	y -= TINYCHAR_HEIGHT;
	Com_sprintf(buf, sizeof(buf), "[1] done");
	CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
}
#endif

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {

#if MAPLENSFLARES
	if (cgs.editMode == EM_mlf) {
		static const vec4_t backFillColor = {
			0.0, 0.0, 0.0, 0.6
		};
		static const vec4_t colorDkGreen = {
			0.0, 0.5, 0.0, 1.0
		};
		static const vec4_t colorLtGreen = {
			0.5, 1.0, 0.5, 1.0
		};
		static const char* const drawModes[] = {
			"normal", "marks", "none"
		};
		static const char* const cursorSize[] = {
			"small", "light radius", "vis radius"
		};
		static const char* const moveModes[] = {
			"coarse", "fine"
		};
		char buf[256];
		int y;

		// crosshair
		if (!cg.lfEditor.selectedLFEnt || cg.lfEditor.editMode != LFEEM_pos) {
			CG_DrawPic(qfalse,320 - 12, 240 - 12, 24, 24, cgs.media.crosshairShader[0]);
		}

		CG_FillRect(0, 480 - 12 * TINYCHAR_HEIGHT, 640, 12 * TINYCHAR_HEIGHT, backFillColor);

		CG_DrawLensFlareEffectList();

		if (cg.lfEditor.cmdMode == LFECM_copyOptions) {
			CG_DrawCopyOptions();
			return;
		}

		y = 480;

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[9] cursor size = %s", cursorSize[cg.lfEditor.cursorSize]);
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorLtGreen : colorDkGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[9] draw mode = %s", drawModes[cg.lfEditor.drawMode]);
			CG_DrawStringExt(-1,0, y, buf, colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[8] copy entity data");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorLtGreen : colorDkGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			const char* name;

			name = "";
			if (cg.lfEditor.selectedLFEnt && cg.lfEditor.selectedLFEnt->lfeff) {
				name = cg.lfEditor.selectedLFEnt->lfeff->name;
			}
			Com_sprintf(buf, sizeof(buf), "[8] note effect %s", name);
			CG_DrawStringExt(-1,0, y, buf, name[0]? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[7] paste entity data");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorLtGreen : colorDkGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[7] assign effect %s", cgs.lensFlareEffects[cg.lfEditor.selectedEffect].name);
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[6] paste options");
			CG_DrawStringExt(-1,0, y, buf, colorLtGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[6] %sedit light size f+b / vis radius l+r", cg.lfEditor.editMode == LFEEM_radius? "^3" : "");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[5] find entity using %s", cgs.lensFlareEffects[cg.lfEditor.selectedEffect].name);
			CG_DrawStringExt(-1,0, y, buf, colorLtGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[5] %sedit spotlight target", cg.lfEditor.editMode == LFEEM_target? "^3" : "");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			if (cg.lfEditor.selectedLFEnt) {
				if (cg.lfEditor.selectedLFEnt->lock) {
					CG_DrawStringExt(-1,0, y, "[4] unlock from mover", colorLtGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
				}
				else {
					CG_DrawStringExt(-1,0, y, "[4] lock to selected mover", cg.lfEditor.selectedMover? colorLtGreen : colorDkGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
				}
			}
			else {
				CG_DrawStringExt(-1,0, y, "[4] lock to selected mover", colorDkGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
			}
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[4] %sedit position & vis radius", cg.lfEditor.editMode == LFEEM_pos? "^3" : "");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[3] find mover %s", cg.lfEditor.moversStopped? "" : "(need to be stopped)");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.moversStopped? colorLtGreen : colorDkGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[3] %sdelete flare entity", cg.lfEditor.delAck? "^1really^7 " : "");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		if (cg.lfEditor.oldButtons & BUTTON_WALKING) {
			Com_sprintf(buf, sizeof(buf), "[2] %s movers", cg.lfEditor.moversStopped? "release" : "stop");
			CG_DrawStringExt(-1,0, y, buf, colorLtGreen, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
		else {
			Com_sprintf(buf, sizeof(buf), "[2] %s flare entity", cg.lfEditor.selectedLFEnt? "duplicate" : "create");
			CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.editMode == LFEEM_none? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}

		y -= TINYCHAR_HEIGHT;
		Com_sprintf(buf, sizeof(buf), "[1] cancel");
		CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt? NULL : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

		y -= TINYCHAR_HEIGHT;
		Com_sprintf(buf, sizeof(buf), "[TAB] move mode = %s", moveModes[cg.lfEditor.moveMode]);
		CG_DrawStringExt(-1,0, y, buf, cg.lfEditor.selectedLFEnt && cg.lfEditor.editMode > LFEEM_none? colorWhite : colorMdGrey, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

		y -= TINYCHAR_HEIGHT;
		Com_sprintf(buf, sizeof(buf), "[WALK] alternate command set");
		CG_DrawStringExt(-1,0, y, buf, (cg.lfEditor.oldButtons & BUTTON_WALKING)? colorLtGreen : colorWhite, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

		y -= TINYCHAR_HEIGHT;
		if (!cg.lfEditor.selectedLFEnt) {
			if (cg.lfEditor.markedLFEnt >= 0) {
				Com_sprintf(buf, sizeof(buf), "[ATTACK] select flare entity");
				CG_DrawStringExt(-1,0, y, buf, NULL, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
			}
		}
		else {
			switch (cg.lfEditor.editMode) {
			case LFEEM_none:
				Com_sprintf(buf, sizeof(buf), "[ATTACK] accept changes");
				break;
			case LFEEM_pos:
				if (cg.lfEditor.moveMode == LFEMM_coarse) {
					Com_sprintf(buf, sizeof(buf), "[ATTACK] switch to tune mode");
				}
				else {
					Com_sprintf(buf, sizeof(buf), "[ATTACK] modify view dist (f+b) or vis radius (l+r)");
				}
				break;
			case LFEEM_target:
				if (cg.lfEditor.editTarget) {
					Com_sprintf(buf, sizeof(buf), "[ATTACK] set target");
				}
				else if (DistanceSquared(cg.refdef.vieworg, cg.lfEditor.targetPosition) < 1) {
					Com_sprintf(buf, sizeof(buf), "[ATTACK] remove target & leave editing mode");
				}
				else {
					Com_sprintf(buf, sizeof(buf), "[ATTACK] set angle & leave editing mode");
				}
				break;
			case LFEEM_radius:
				Com_sprintf(buf, sizeof(buf), "[ATTACK] modify view distance (f+b)");
				break;
			default:
				buf[0] = 0;
				break;
			}
			CG_DrawStringExt(-1,0, y, buf, NULL, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
	}
	else
#endif

	CG_DrawBigString(320 - 9 * 8, 440, "SPECTATOR", 1.0F);
	if ( cgs.gametype == GT_TOURNAMENT ) {
		CG_DrawBigString(320 - 15 * 8, 460, "waiting to play", 1.0F);
	}
	else if ( cgs.gametype >= GT_TEAM ) {
		CG_DrawBigString(320 - 39 * 8, 460, "press ESC and use the JOIN menu to play", 1.0F);
	}
}

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
#ifdef MISSIONPACK
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
#else
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_DrawSmallString( 0, 58, s, 1.0F );
#endif
}

/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo->team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo->team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}


/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	float		x;
	vec4_t		color;
	const char	*name;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;


	CG_DrawBigString( 320 - 9 * 8, 24, "following", 1.0F );

	name = cgs.clientinfo[ cg.snap->ps.clientNum ].name;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( name ) );

	CG_DrawStringExt(-1, x, 40, name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

	return qtrue;
}



/*
=================
CG_DrawAmmoWarning
=================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;
	int			w;

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
	} else {
		s = "LOW POWER";
	}
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 64, s, 1.0F);
}


#ifdef MISSIONPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	int			w;
  static int proxTime;
  static int proxCounter;
  static int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

  if (proxTime == 0) {
    proxTime = cg.time + 5000;
    proxCounter = 5;
    proxTick = 0;
  }

  if (cg.time > proxTime) {
    proxTick = proxCounter--;
    proxTime = cg.time + 1000;
  }

  if (proxTick != 0) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
#endif


/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void ) {
	int			w;
	int			sec;
	int			i;
	float scale;
	clientInfo_t	*ci1, *ci2;
	int			cw;
	const char	*s;

	sec = cg.warmup;
	if ( !sec ) {
		return;
	}

	if ( sec < 0 ) {
		s = "Waiting for players";		
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		CG_DrawBigString(320 - w / 2, 24, s, 1.0F);
		cg.warmupCount = 0;
		return;
	}

	if (cgs.gametype == GT_TOURNAMENT) {
		// find the two active players
		ci1 = NULL;
		ci2 = NULL;
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_FREE ) {
				if ( !ci1 ) {
					ci1 = &cgs.clientinfo[i];
				} else {
					ci2 = &cgs.clientinfo[i];
				}
			}
		}

		if ( ci1 && ci2 ) {
			s = va( "%s vs %s", ci1->name, ci2->name );
			w = CG_DrawStrlen( s );
			if ( w > 640 / GIANT_WIDTH ) {
				cw = 640 / w;
			} else {
				cw = GIANT_WIDTH;
			}
			CG_DrawStringExt(-1, 320 - w * cw/2, 20,s, colorWhite, 
					qfalse, qtrue, cw, (int)(cw * 1.5f), 0 );
		}
	} else {
		if ( cgs.gametype == GT_FFA ) {
			s = "Free For All";
		} else if ( cgs.gametype == GT_STRUGGLE ) {
			s = "Struggle";
		} else if ( cgs.gametype == GT_TEAM ) {
			s = "Team Deathmatch";
		} else if ( cgs.gametype == GT_CTF ) {
			s = "Capture the Flag";
		} else {
			s = "";
		}
		w = CG_DrawStrlen( s );
		if ( w > 640 / GIANT_WIDTH ) {
			cw = 640 / w;
		} else {
			cw = GIANT_WIDTH;
		}
		CG_DrawStringExt(-1, 320 - w * cw/2, 25,s, colorWhite, 
				qfalse, qtrue, cw, (int)(cw * 1.1f), 0 );
	}

	sec = ( sec - cg.time ) / 1000;
	if ( sec < 0 ) {
		cg.warmup = 0;
		sec = 0;
	}
	s = va( "Starts in: %i", sec + 1 );
	if ( sec != cg.warmupCount ) {
		cg.warmupCount = sec;
	}
	scale = 0.45f;
	switch ( cg.warmupCount ) {
	case 0:
		cw = 28;
		scale = 0.54f;
		break;
	case 1:
		cw = 24;
		scale = 0.51f;
		break;
	case 2:
		cw = 20;
		scale = 0.48f;
		break;
	default:
		cw = 16;
		scale = 0.45f;
		break;
	}

#ifdef MISSIONPACK
		w = CG_Text_Width(s, scale, 0);
		CG_Text_Paint(320 - w / 2, 125, scale, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
#else
	w = CG_DrawStrlen( s );
	CG_DrawStringExt(-1, 320 - w * cw/2, 70, s, colorWhite, 
			qfalse, qtrue, cw, (int)(cw * 1.5), 0 );
#endif
}

//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif
/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void ) {
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}
	if ( cg_draw2D.integer == 0 ) {
		return;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		CG_DrawSpectator();
		CG_DrawCrosshair();
		CG_DrawCrosshairNames();
		CG_DrawRadar();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if (!(cg.snap->ps.timers[tmTransform] > 1) && !(cg.snap->ps.powerups[PW_STATE] < 0)){
		

			playerState_t	*ps;
			clientInfo_t *ci;
			ci = &cgs.clientinfo[cg.snap->ps.clientNum];
			ps = &cg.snap->ps;
			CG_DrawStatusBar();
			CG_DrawAmmoWarning();  
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawRadar();
			if(!(cg.snap->ps.bitFlags & usingMelee)){
				CG_DrawWeaponSelect();
			}
		}
		if ( cgs.gametype >= GT_TEAM ) {
			CG_DrawTeamInfo();
		}
	}
	CG_DrawVote();
	CG_DrawTeamVote();
	CG_DrawUpperRight();
	CG_DrawLowerRight();
	if (!CG_DrawFollow()){
		CG_DrawWarmup();
	}
}



void CG_DrawScreenFlash ( void ) {
	float		*color;
	vec4_t		white = {1.0f,1.0f,1.0f,0.5f};
	vec4_t		black = {0.0f,0.0f,0.0f,0.5f};

	color = CG_FadeColor( cg.screenFlashTime, cg.screenFlastTimeTotal, cg.screenFlashFadeTime );

	if ( !color ) {
		return;
	}

	if ( cg.snap->ps.timers[tmBlind] > 0 ){

		trap_R_SetColor( color );

		white[3] = color[3] * cg.screenFlashFadeAmount * 0.5f;
		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, white);

		trap_R_SetColor( NULL );

	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	vec4_t		water = {0.25f,0.5f,1.0f,0.1f};
	vec4_t		slime = {0.25f,1.0f,0.25f,0.1f};
	vec4_t		lava = {1.0f,0.5f,0.0f,0.1f};
	int			contents;
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}
	// clear around the rendered view if sized down
	CG_TileClear();

#if MAPLENSFLARES	// JUHOX: add lens flare editor cursor
	if (
		cgs.editMode == EM_mlf &&
		!(
			cg.lfEditor.moversStopped &&
			cg.lfEditor.selectedMover
		)
	) {
		CG_AddLFEditorCursor();
	}
#endif

	CG_MotionBlur();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

#if MAPLENSFLARES	// JUHOX: mark selected mover for lens flare editor
	if (
		cgs.editMode == EM_mlf &&
		cg.lfEditor.moversStopped &&
		cg.lfEditor.selectedMover
	) {
		static const vec4_t darkening = { 0.2, 0, 0, 0.7 };

		CG_FillRect(0, 0, 640, 480, darkening);
		trap_R_ClearScene();
		CG_Mover(cg.lfEditor.selectedMover);
		CG_AddLFEditorCursor();
		cg.refdef.rdflags |= RDF_NOWORLDMODEL;
		trap_R_RenderScene(&cg.refdef);
	}
#endif
	// draw screen tinting and flashing

	contents = CG_PointContents( cg.refdef.vieworg, -1 );

	if ( contents & CONTENTS_WATER ){
		float phase = cg.time / 1000.0 * 0.2f * M_PI * 2;
		water[3] = 0.1f + (0.02f*sin( phase ));
		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, water);
		trap_R_AddFogToScene(0,5000, 0,0,0,1,2,2);
	} else if ( contents & CONTENTS_SLIME ){
		float phase = cg.time / 1000.0 * 0.2f * M_PI * 2;
		slime[3] = 0.1f + (0.02f*sin( phase ));
		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, slime);
		trap_R_AddFogToScene(0,3000, 0.5,0.5,0.5,1,2,2);
	} else if ( contents & CONTENTS_LAVA ){
		float phase = cg.time / 1000.0 * 0.2f * M_PI * 2;
		lava[3] = 0.1f + (0.02f*sin( phase ));
		CG_DrawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH*SCREEN_HEIGHT, lava);
		trap_R_AddFogToScene(0,2000, 1,1,1,1,2,2);
	} else {
		trap_R_AddFogToScene(0,0, 0,0,0,0,2,2);
	}

	CG_DrawScreenFlash();

	// draw status bar and other floating elements
 	CG_Draw2D();

	// ADDING FOR ZEQ2
	// HACK: We don't support deferring, so until it can REALLY be removed,
	// always issue a CG_LoadDeferredPlayers() call.
	CG_LoadDeferredPlayers();
	// END ADDING
}



