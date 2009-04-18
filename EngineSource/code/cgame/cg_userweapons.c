// Copyright (C) 2003-2004 RiO
//
// cg_userweapons.c -- Clientside custom weapons database and parser functions.
//                     Contains the actual loading of the clientside files, and
//                     of the both games files. Passes the pointer to the loaded
//                     string of the latter to bg_userweapons.c. 

#include "cg_local.h"


static cg_userWeapon_t		weaponGraphicsDatabase[MAX_CLIENTS][ALTSPAWN_OFFSET + MAX_PLAYERWEAPONS];
							// * 2 because we also have alternate fires possible for each
							// weapon!

cg_userWeapon_t *CG_FindUserWeaponGraphics( int clientNum, int index ) {
	return &weaponGraphicsDatabase[clientNum][index - 1];
}

void CG_ParseWeaponList( void ) {
	int i, j;
	cg_userWeapon_t		*weaponSettings;
	cg_userWeapon_t		*altWeaponSettings;

	// Initialize the graphics database to 0
	memset (weaponGraphicsDatabase, 0, sizeof(weaponGraphicsDatabase));


	// OBSOLETE CODE
/*
	BG_ParseWeaponList(); // FIXME: Later on, should load the files and then pass
	                      //        the loaded string(s).
*/
	// END OBSOLETE CODE

	for (i=0; i < MAX_CLIENTS; i++) {
		
		// --< 1: Kikou >--
		j = 0;
		weaponSettings = &weaponGraphicsDatabase[i][j];
		altWeaponSettings = &weaponGraphicsDatabase[i][ALTWEAPON_OFFSET + j];

		// CHARGE
		weaponSettings->chargeShader = trap_R_RegisterShader( "Weapons_KikouCharge" );
		weaponSettings->chargeGrowth = qtrue;
		weaponSettings->chargeStartPct = 10;
		weaponSettings->chargeEndPct = 60;
		weaponSettings->chargeStartsize = 1;
		weaponSettings->chargeEndsize = 2;
		MAKERGB(weaponSettings->chargeDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->chargeDlightStartRadius = 0.25f;
		weaponSettings->chargeDlightEndRadius = 1.25f;
		Q_strncpyz( weaponSettings->chargeTag[0], "tag_weapon", sizeof( weaponSettings->chargeTag[0] ) );
		
		// EXPLOSION
		weaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		weaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/Kikou_explosion.skin" );
		MAKERGB(weaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->explosionDlightRadius = 2;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->explosionSize = 4;
		weaponSettings->explosionTime = 750;
		weaponSettings->smokeAmount = 8;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 10;
		weaponSettings->smokeRadius = 12;
		weaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		weaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->flashDlightRadius = 1.5f;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Kikou_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;
		
		// MISSILE
		weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/Kikou.md3" );
		weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/Kikou.skin" );
		MAKERGB(weaponSettings->missileDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->missileDlightRadius = 1;
		weaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponSettings->missileSize = 0.75f;

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_Kikou.tga");
		Q_strncpyz( weaponSettings->weaponName , "Kikou", sizeof( weaponSettings->weaponName ) );


		// --< 1, Altfire: Renzoku Energy Dan >--

		// EXPLOSION
		altWeaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		altWeaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/Kikou_explosion.skin" );
		MAKERGB(altWeaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->explosionDlightRadius = 2;
		altWeaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		altWeaponSettings->explosionSize = 3;
		altWeaponSettings->explosionTime = 600;
		altWeaponSettings->smokeAmount = 6;
		altWeaponSettings->smokeDuration = 1000;
		altWeaponSettings->smokeSize = 10;
		altWeaponSettings->smokeRadius = 10;
		altWeaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		altWeaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(altWeaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->flashDlightRadius = 1.5;
		altWeaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Kikou_flash.wav", qfalse );
		altWeaponSettings->flashSound[1] = 0;
		altWeaponSettings->flashSound[2] = 0;
		altWeaponSettings->flashSound[3] = 0;
		altWeaponSettings->flashVoice[0] = 0;
		altWeaponSettings->flashVoice[1] = 0;
		altWeaponSettings->repeatFlashVoice = qtrue;
		altWeaponSettings->flashSize = 1;
		
		// MISSILE
		altWeaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/Kikou.md3" );
		altWeaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/Kikou.skin" );
		MAKERGB(altWeaponSettings->missileDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->missileDlightRadius = 1;
		altWeaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		altWeaponSettings->missileSize = 0.75f;

		altWeaponSettings->missileTrailFunc = CG_TrailFunc_FadeTail;
		altWeaponSettings->missileTrailShader = trap_R_RegisterShader( "Trail_Yellow" );
		altWeaponSettings->missileTrailRadius = 5.0f;

		// HUD
		Q_strncpyz( altWeaponSettings->weaponName , "Renzoku Energy Dan", sizeof( altWeaponSettings->weaponName ) );


		
		//  --< 2: Shyogeki Ha >--
		j++;
		weaponSettings = &weaponGraphicsDatabase[i][j];
		altWeaponSettings = &weaponGraphicsDatabase[i][ALTWEAPON_OFFSET + j];

		// EXPLOSION
		MAKERGB(weaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->explosionDlightRadius = 3;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->explosionSize = 1;
		weaponSettings->explosionTime = 1000;
		weaponSettings->smokeAmount = 3;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 6;
		weaponSettings->smokeRadius = 5;
		weaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		weaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->flashDlightRadius = 1.5;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Kikou_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_Shyogeki.tga");
		Q_strncpyz( weaponSettings->weaponName , "Shyogeki Ha", sizeof( weaponSettings->weaponName ) );



		// --< 3: Big Bang Attack  ( Ball ) >--
		// NOTE: Now turned into --< 3: Freeza Beam >--
		j++;
		weaponSettings = &weaponGraphicsDatabase[i][j];
		altWeaponSettings = &weaponGraphicsDatabase[i][ALTWEAPON_OFFSET + j];

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_Fzbeam.tga");
		Q_strncpyz( weaponSettings->weaponName , "Freeza Beam", sizeof( weaponSettings->weaponName ) );

		// EXPLOSION
		weaponSettings->explosionSize = 0.5f;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->smokeAmount = 12;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 10;
		weaponSettings->smokeRadius = 15;
		
		// RAILTRAIL
		MAKERGB( weaponSettings->railTrailColor, 1.0f, 0, 0 );
		weaponSettings->hasRailTrail = qtrue;

		// FLASH
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Fzbeam_flash.wav", qfalse );

/*
		// CHARGE
		weaponSettings->chargeShader = trap_R_RegisterShader( "Weapons_BBACharge" );
		weaponSettings->chargeGrowth = qtrue;
		weaponSettings->chargeStartPct = 15;
		weaponSettings->chargeEndPct = 80;
		weaponSettings->chargeStartsize = 1;
		weaponSettings->chargeEndsize = 3;
		MAKERGB(weaponSettings->chargeDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->chargeDlightStartRadius = 0.25;
		weaponSettings->chargeDlightEndRadius = 1.25;
		weaponSettings->chargeSound = 0;
		Q_strncpyz( weaponSettings->chargeTag[0], "tag_weapon", sizeof( weaponSettings->chargeTag[0] ) );
		
		// EXPLOSION
		weaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		weaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/BBA_explosion.skin" );
		MAKERGB(weaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->explosionDlightRadius = 3;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->explosionSize = 8;
		weaponSettings->explosionTime = 1000;
		weaponSettings->smokeAmount = 12;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 10;
		weaponSettings->smokeRadius = 15;
		weaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		weaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->flashDlightRadius = 1.5;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Kikou_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;
		
		// MISSILE
		weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/EnergyBall.md3" );
		weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/BBA.skin" );
		MAKERGB(weaponSettings->missileDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->missileDlightRadius = 1;
		weaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponSettings->missileSize = 0.75f;
		weaponSettings->missileTrailFunc = CG_TrailFunc_FadeTail;
		weaponSettings->missileTrailShader = trap_R_RegisterShader( "Beam_Blue" );

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_BBA.tga");
		Q_strncpyz( weaponSettings->weaponName , "Big Bang Attack (Ball", sizeof( weaponSettings->weaponName ) );


		// --< 3, Altfire: Big Bang Attack  ( Beam ) >--

		// CHARGE
		altWeaponSettings->chargeShader = trap_R_RegisterShader( "Weapons_BBACharge" );
		altWeaponSettings->chargeGrowth = qtrue;
		altWeaponSettings->chargeStartPct = 15;
		altWeaponSettings->chargeEndPct = 80;
		altWeaponSettings->chargeStartsize = 1;
		altWeaponSettings->chargeEndsize = 3;
		MAKERGB(altWeaponSettings->chargeDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->chargeDlightStartRadius = 0.25;
		altWeaponSettings->chargeDlightEndRadius = 1.25;
		altWeaponSettings->chargeSound = 0;
		Q_strncpyz( altWeaponSettings->chargeTag[0], "tag_weapon", sizeof( altWeaponSettings->chargeTag[0] ) );
		
		// EXPLOSION
		altWeaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		altWeaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/BBA_explosion.skin" );
		MAKERGB(altWeaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->explosionDlightRadius = 3;
		altWeaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		altWeaponSettings->explosionSize = 8;
		altWeaponSettings->explosionTime = 1000;
		altWeaponSettings->smokeAmount = 12;
		altWeaponSettings->smokeDuration = 1000;
		altWeaponSettings->smokeSize = 10;
		altWeaponSettings->smokeRadius = 15;
		altWeaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		altWeaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(altWeaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->flashDlightRadius = 1.5;
		altWeaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Kikou_flash.wav", qfalse );
		altWeaponSettings->flashSound[1] = 0;
		altWeaponSettings->flashSound[2] = 0;
		altWeaponSettings->flashSound[3] = 0;
		altWeaponSettings->flashVoice[0] = 0;
		altWeaponSettings->flashVoice[1] = 0;
		altWeaponSettings->repeatFlashVoice = qtrue;
		altWeaponSettings->flashSize = 1;
		
		// MISSILE
		altWeaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/EnergyBall.md3" );
		altWeaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/BBA.skin" );
		MAKERGB(altWeaponSettings->missileDlightColor, 0.85f, 0.97f, 0);
		altWeaponSettings->missileDlightRadius = 1;
		altWeaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		altWeaponSettings->missileSize = 0.75f;
		altWeaponSettings->missileTrailFunc = CG_TrailFunc_BendyBeam;
		altWeaponSettings->missileTrailShader = trap_R_RegisterShader( "Beam_Blue" );
		altWeaponSettings->missileTrailRadius = 20;

		// HUD
		Q_strncpyz( altWeaponSettings->weaponName , "Beam)", sizeof( altWeaponSettings->weaponName ) );
*/

		



		// --< 4: Daichiretsuzan >--
		j++;
		weaponSettings = &weaponGraphicsDatabase[i][j];
		altWeaponSettings = &weaponGraphicsDatabase[i][ALTWEAPON_OFFSET + j];

		// CHARGE
		weaponSettings->chargeShader = trap_R_RegisterShader( "Weapons_DaichiCharge" );
		weaponSettings->chargeGrowth = qtrue;
		weaponSettings->chargeStartPct = 10;
		weaponSettings->chargeEndPct = 50;
		weaponSettings->chargeStartsize = 1;
		weaponSettings->chargeEndsize = 2;
		MAKERGB(weaponSettings->chargeDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->chargeDlightStartRadius = 0.25;
		weaponSettings->chargeDlightEndRadius = 1.25;
		Q_strncpyz( weaponSettings->chargeTag[0], "tag_weapon", sizeof( weaponSettings->chargeTag[0] ) );
		
		// EXPLOSION
		weaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		weaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/Daichi_explosion.skin" );
		MAKERGB(weaponSettings->explosionDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->explosionDlightRadius = 2;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->explosionSize = 5;
		weaponSettings->explosionTime = 750;
		weaponSettings->smokeAmount = 8;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 10;
		weaponSettings->smokeRadius = 12;
		weaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		weaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->flashDlightRadius = 1.5;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Bigweapon_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;
		
		// MISSILE
		weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/Daichi.md3" );
		weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/Daichi.skin" );
		MAKERGB(weaponSettings->missileDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->missileDlightRadius = 1;
		weaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponSettings->missileSize = 1.75f;

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_Daichi.tga");
		Q_strncpyz( weaponSettings->weaponName , "Daichiretsuzan", sizeof( weaponSettings->weaponName ) );


			
		// --< 5: Final Flash >--
		// NOTE: Now turned into --< 5: Deathball >--
		j++;
		weaponSettings = &weaponGraphicsDatabase[i][j];
		altWeaponSettings = &weaponGraphicsDatabase[i][ALTWEAPON_OFFSET + j];

		// CHARGE

		// -- Changed lines --
		//weaponSettings->chargeShader = trap_R_RegisterShader( "Weapons_KikouCharge" );		
		weaponSettings->chargeModel = trap_R_RegisterModel( "models/weapons/Deathball_charge.md3" );
		weaponSettings->chargeSkin = trap_R_RegisterSkin( "models/weapons/Deathball.skin" );
		// -- End changed lines --
		weaponSettings->chargeGrowth = qtrue;
		//weaponSettings->chargeStartPct = 15;
		weaponSettings->chargeStartPct = 5;
		weaponSettings->chargeEndPct = 100;
		//weaponSettings->chargeStartsize = 1;
		weaponSettings->chargeStartsize = 0.1f;
		weaponSettings->chargeEndsize = 2.5f;
		MAKERGB(weaponSettings->chargeDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->chargeDlightStartRadius = 0.25;
		weaponSettings->chargeDlightEndRadius = 1.25;
		weaponSettings->chargeLoopSound = trap_S_RegisterSound( "sound/weapons/Deathball_charging.wav", qfalse );
		weaponSettings->chargeVoice[0].startPct = 5;
		weaponSettings->chargeVoice[0].voice = trap_S_RegisterSound( "sound/weapons/Deathball_charge_pct1.wav", qfalse );
		Q_strncpyz( weaponSettings->chargeTag[0], "tag_weapon", sizeof( weaponSettings->chargeTag[0] ) );
		
		// EXPLOSION
		weaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		//weaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/Kikou_explosion.skin" );
		weaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/Deathball_explosion.skin" );
		MAKERGB(weaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->explosionDlightRadius = 3;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->explosionSize = 8;
		weaponSettings->explosionTime = 1000;
		weaponSettings->smokeAmount = 12;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 10;
		weaponSettings->smokeRadius = 15;
		weaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		weaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->flashDlightRadius = 1.5;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Bigweapon_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;
		
		// MISSILE
		//weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/Kikou.md3" );
		weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/Deathball.md3" );
		//weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/Kikou.skin" );
		weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/Deathball.skin" );
		MAKERGB(weaponSettings->missileDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->missileDlightRadius = 1;
		weaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		//weaponSettings->missileSize = 0.75f;
		weaponSettings->missileSize = 1.0f;
		//weaponSettings->missileTrailFunc = CG_TrailFunc_StraightBeam;
		//weaponSettings->missileTrailShader = trap_R_RegisterShader( "Beam_Yellow" );
		//weaponSettings->missileTrailRadius = 35;

		// HUD
		//weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_FF.tga");
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_Deathball.tga");
		//Q_strncpyz( weaponSettings->weaponName , "Final Flash", sizeof( weaponSettings->weaponName ) );
		Q_strncpyz( weaponSettings->weaponName , "Deathball", sizeof( weaponSettings->weaponName ) );



		//  --< 6: Makankosappo >--
		// NOTE: Now turned into --< 6: Tsuibi Kienzan >--
		j++;
		weaponSettings = &weaponGraphicsDatabase[i][j];
		altWeaponSettings = &weaponGraphicsDatabase[i][ALTWEAPON_OFFSET + j];

		// CHARGE
		weaponSettings->chargeModel = trap_R_RegisterModel( "models/weapons/disk.md3" );
		weaponSettings->chargeSkin = trap_R_RegisterSkin( "models/weapons/purple_disk.skin" );
		weaponSettings->chargeSpinYaw = qtrue;
		weaponSettings->chargeGrowth = qtrue;
		weaponSettings->chargeStartPct = 2;
		weaponSettings->chargeEndPct = 20;
		weaponSettings->chargeStartsize = 0.1f;
		weaponSettings->chargeEndsize = 1;
		MAKERGB(weaponSettings->chargeDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->chargeDlightStartRadius = 0.25;
		weaponSettings->chargeDlightEndRadius = 1.25;
		Q_strncpyz( weaponSettings->chargeTag[0], "tag_weapon", sizeof( weaponSettings->chargeTag[0] ) );
		
		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->flashDlightRadius = 1.5;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Bigweapon_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;
		
		// MISSILE
		weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/disk.md3" );
		weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/purple_disk.skin" );
		MAKERGB(weaponSettings->missileDlightColor, 0.82f, 0.18f, 0.60f);
		weaponSettings->missileDlightRadius = 1;
		weaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponSettings->missileSize = 1;
		weaponSettings->missileSpinYaw = qtrue;

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_TsKienzan.tga");
		Q_strncpyz( weaponSettings->weaponName , "Tsuibi Kienzan", sizeof( weaponSettings->weaponName ) );

		/*
		// CHARGE
		weaponSettings->chargeShader = trap_R_RegisterShader( "Weapons_KikouCharge" );
		weaponSettings->chargeGrowth = qtrue;
		weaponSettings->chargeStartPct = 15;
		weaponSettings->chargeEndPct = 80;
		weaponSettings->chargeStartsize = 1;
		weaponSettings->chargeEndsize = 3;
		MAKERGB(weaponSettings->chargeDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->chargeDlightStartRadius = 0.25;
		weaponSettings->chargeDlightEndRadius = 1.25;
		weaponSettings->chargeSound = 0;
		Q_strncpyz( weaponSettings->chargeTag[0], "tag_weapon", sizeof( weaponSettings->chargeTag[0] ) );
		
		// EXPLOSION
		weaponSettings->explosionModel = trap_R_RegisterModel( "models/weapons/Explosion.md3" );
		weaponSettings->explosionSkin = trap_R_RegisterSkin( "models/weapons/Kikou_explosion.skin" );
		MAKERGB(weaponSettings->explosionDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->explosionDlightRadius = 3;
		weaponSettings->explosionSound[0] = trap_S_RegisterSound ("sound/weapons/rocket/rocklx1a.wav", qfalse);
		weaponSettings->explosionSize = 6;
		weaponSettings->explosionTime = 1000;
		weaponSettings->smokeAmount = 8;
		weaponSettings->smokeDuration = 1000;
		weaponSettings->smokeSize = 10;
		weaponSettings->smokeRadius = 10;
		weaponSettings->shockwaveModel = trap_R_RegisterModel( "models/weapons/Shockwaves.md3" );
		weaponSettings->shockwaveSkin = trap_R_RegisterSkin( "models/weapons/Shockwaves.skin" );

		// FLASH
		MAKERGB(weaponSettings->flashDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->flashDlightRadius = 1.5;
		weaponSettings->flashSound[0] = trap_S_RegisterSound( "sound/weapons/Kikou_flash.wav", qfalse );
		weaponSettings->flashSound[1] = 0;
		weaponSettings->flashSound[2] = 0;
		weaponSettings->flashSound[3] = 0;
		weaponSettings->flashVoice[0] = 0;
		weaponSettings->flashVoice[1] = 0;
		weaponSettings->repeatFlashVoice = qtrue;
		weaponSettings->flashSize = 1;
		
		// MISSILE
		weaponSettings->missileModel = trap_R_RegisterModel( "models/weapons/Kikou.md3" );
		weaponSettings->missileSkin = trap_R_RegisterSkin( "models/weapons/Kikou.skin" );
		MAKERGB(weaponSettings->missileDlightColor, 0.85f, 0.97f, 0);
		weaponSettings->missileDlightRadius = 1;
		weaponSettings->missileSound = trap_S_RegisterSound( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponSettings->missileSize = 0.75f;
		weaponSettings->missileTrailFunc = CG_TrailFunc_SpiralBeam;
		weaponSettings->missileTrailShader = trap_R_RegisterShader( "Beam_Yellow" );
		weaponSettings->missileTrailRadius = 5;
		weaponSettings->missileTrailSpiralShader = trap_R_RegisterShader( "Beam_Yellow" );
		weaponSettings->missileTrailSpiralRadius = 12;

		// HUD
		weaponSettings->weaponIcon = trap_R_RegisterShaderNoMip("icons/HUD_Makan.tga");
		Q_strncpyz( weaponSettings->weaponName , "Makankosappo", sizeof( weaponSettings->weaponName ) );
		*/
	}
}
