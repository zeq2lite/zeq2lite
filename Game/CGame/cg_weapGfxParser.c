// Copyright (C) 2003-2005 RiO
//
// cg_weapGfxParser.c -- token parser for ZEQ2's weapon graphics script language.

#include "cg_weapGfxParser.h" // <-- cg_local.h included in this


static int							cg_weapGfxRecursionDepth;
static cg_userWeaponParseBuffer_t	cg_weapGfxBuffer;
	// FIXME: Can this be a local variable instead, or would it give us
	//		  > 32k locals errors in the VM-bytecode compiler?

/*
=======================
CG_weapGfx_StoreBuffer
=======================
Copies the contents in the buffer to a client's weapon
configuration, converting filestrings into qhandle_t in
the process.
*/
void CG_weapGfx_StoreBuffer(int clientNum, int weaponNum) {
	cg_userWeapon_t				*dest;
	cg_userWeaponParseBuffer_t	*src;
	int							i;

	src = &cg_weapGfxBuffer;
	dest = CG_FindUserWeaponGraphics( clientNum, weaponNum + 1);
	memset( dest, 0, sizeof(cg_userWeapon_t));

	// --< Charge >--
	if ( *src->chargeModel ) dest->chargeModel = trap_R_RegisterModel( src->chargeModel );
	if ( *src->chargeSkin ) dest->chargeSkin = trap_R_RegisterSkin( src->chargeSkin );
	if ( *src->chargeShader ) dest->chargeShader = trap_R_RegisterShader( src->chargeShader );
	if ( *src->chargeLoopSound ) dest->chargeLoopSound = trap_S_RegisterSound( src->chargeLoopSound, qfalse );

	for ( i = 0; i < 6; i++ ) {
		if ( *(src->chargeVoice[i].voice) ) {
			dest->chargeVoice[i].voice = trap_S_RegisterSound( src->chargeVoice[i].voice, qfalse );
			dest->chargeVoice[i].startPct = src->chargeVoice[i].startPct;
		}
	}

	VectorCopy( src->chargeDlightColor, dest->chargeDlightColor );
	VectorCopy( src->chargeSpin, dest->chargeSpin );
	
	Q_strncpyz( dest->chargeTag[0], src->chargeTag[0], sizeof( dest->chargeTag[0] ));

	dest->chargeGrowth = ( src->chargeStartPct != src->chargeEndPct ); // <-- May become redundant...
	dest->chargeStartPct = src->chargeStartPct;
	dest->chargeEndPct = src->chargeEndPct;
	dest->chargeStartsize = src->chargeStartsize;
	dest->chargeEndsize = src->chargeEndsize;
	dest->chargeDlightStartRadius = src->chargeDlightStartRadius;
	dest->chargeDlightEndRadius = src->chargeDlightEndRadius;

	Q_strncpyz( dest->chargeParticleSystem, src->chargeParticleSystem, sizeof( dest->chargeParticleSystem ));

	
	// --< Flash >--
	if ( *src->flashModel ) dest->flashModel = trap_R_RegisterModel( src->flashModel );
	if ( *src->flashSkin ) dest->flashSkin = trap_R_RegisterSkin( src->flashSkin );
	if ( *src->flashShader ) dest->flashShader = trap_R_RegisterShader( src->flashShader );
	if ( *src->firingSound ) dest->firingSound = trap_S_RegisterSound( src->firingSound, qfalse );
	if ( *src->flashOnceSound ) dest->flashOnceSound = trap_S_RegisterSound( src->flashOnceSound, qfalse );

	for ( i = 0; i < 4; i++ ) {
		if ( *(src->flashSound[i]) ) {
			dest->flashSound[i] = trap_S_RegisterSound( src->flashSound[i], qfalse );
		}
	}

	for ( i = 0; i < 4; i++ ) {
		if ( *(src->voiceSound[i]) ) {
			dest->voiceSound[i] = trap_S_RegisterSound( src->voiceSound[i], qfalse );
		}
	}

	VectorCopy( src->flashDlightColor, dest->flashDlightColor );

	dest->flashDlightRadius = src->flashDlightRadius;
	dest->flashSize = src->flashSize;

	Q_strncpyz( dest->flashParticleSystem, src->flashParticleSystem, sizeof( dest->flashParticleSystem ));
	Q_strncpyz( dest->firingParticleSystem, src->firingParticleSystem, sizeof( dest->firingParticleSystem ));
	

	// --< Explosion >--
	if ( *src->explosionModel ) dest->explosionModel = trap_R_RegisterModel( src->explosionModel );
	if ( *src->explosionSkin ) dest->explosionSkin = trap_R_RegisterSkin( src->explosionSkin );
	if ( *src->explosionShader ) dest->explosionShader = trap_R_RegisterShader( src->explosionShader );
	if ( *src->shockwaveModel ) dest->shockwaveModel = trap_R_RegisterModel( src->shockwaveModel );
	if ( *src->shockwaveSkin ) dest->shockwaveSkin = trap_R_RegisterSkin( src->shockwaveSkin );
	if ( *src->markShader ) dest->markShader = trap_R_RegisterShader( src->markShader );
	
	for ( i = 0; i < 4; i++ ) {
		if ( *(src->explosionSound[i]) ) {
			dest->explosionSound[i] = trap_S_RegisterSound( src->explosionSound[i], qfalse );
		}
	}

	VectorCopy( src->explosionDlightColor, dest->explosionDlightColor );

	dest->explosionDlightRadius = src->explosionDlightRadius;
	dest->explosionSize = src->explosionSize;
	dest->explosionTime = src->explosionTime;
	dest->markSize = src->markSize;
	dest->noRockDebris = src->noRockDebris;
	Q_strncpyz( dest->smokeParticleSystem, src->smokeParticleSystem, sizeof( dest->smokeParticleSystem ));
	Q_strncpyz( dest->explosionParticleSystem, src->explosionParticleSystem, sizeof( dest->explosionParticleSystem ));
	
	// --< Missile >--
	if ( *src->missileModel ) dest->missileModel = trap_R_RegisterModel( src->missileModel );
	if ( *src->missileSkin ) dest->missileSkin = trap_R_RegisterSkin( src->missileSkin );
	if ( *src->missileShader ) dest->missileShader = trap_R_RegisterShader( src->missileShader );

	// --< Missile Struggle >--
	if ( *src->missileStruggleModel ) dest->missileStruggleModel = trap_R_RegisterModel( src->missileStruggleModel );
	if ( *src->missileStruggleSkin ) dest->missileStruggleSkin = trap_R_RegisterSkin( src->missileStruggleSkin );
	if ( *src->missileStruggleShader ) dest->missileStruggleShader = trap_R_RegisterShader( src->missileStruggleShader );

	if ( *src->missileSound ) dest->missileSound = trap_S_RegisterSound( src->missileSound, qfalse );
	
	VectorCopy( src->missileSpin, dest->missileSpin );
	VectorCopy( src->missileDlightColor, dest->missileDlightColor );

	dest->missileDlightRadius = src->missileDlightRadius;
	dest->missileSize = src->missileSize;
	Q_strncpyz( dest->missileParticleSystem, src->missileParticleSystem, sizeof( dest->missileParticleSystem ));


	// --< Trail >--
	if ( *src->missileTrailShader ) dest->missileTrailShader = trap_R_RegisterShader( src->missileTrailShader );
	if ( *src->missileTrailSpiralShader ) dest->missileTrailSpiralShader = trap_R_RegisterShader( src->missileTrailSpiralShader );

	dest->missileTrailRadius = src->missileTrailRadius;
	dest->missileTrailSpiralRadius = src->missileTrailSpiralRadius;
	dest->missileTrailSpiralOffset = src->missileTrailSpiralOffset;

	// --< Hud >--
	if ( *src->weaponIcon ) dest->weaponIcon = trap_R_RegisterShaderNoMip( src->weaponIcon );
	
	Q_strncpyz( dest->weaponName, src->weaponName, sizeof(dest->weaponName));
}



/*
=======================
CG_weapGfx_ParseDummy
=======================
A special dummy function used in the terminator of
the cg_weapGfxFields list. Never actually used, and
if it _would_ be called, it would always hault
parsing.
*/
qboolean CG_weapGfx_ParseDummy( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	return qfalse;
}


/*
=======================
CG_weapGfx_ParseModel
=======================
Parses 'model' field.
Syntax:
'model' '=' ( "filename" | null )
*/
qboolean CG_weapGfx_ParseModel( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch ( category ) {
	case CAT_CHARGE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeModel, "", sizeof(cg_weapGfxBuffer.chargeModel) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeModel, token->stringval, sizeof(cg_weapGfxBuffer.chargeModel) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_EXPLOSION:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionModel, "", sizeof(cg_weapGfxBuffer.explosionModel) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionModel, token->stringval, sizeof(cg_weapGfxBuffer.explosionModel) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_STRUGGLE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileStruggleModel, "", sizeof(cg_weapGfxBuffer.missileStruggleModel) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileStruggleModel, token->stringval, sizeof(cg_weapGfxBuffer.missileStruggleModel) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_MISSILE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileModel, "", sizeof(cg_weapGfxBuffer.missileModel) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileModel, token->stringval, sizeof(cg_weapGfxBuffer.missileModel) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_FLASH:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.flashModel, "", sizeof(cg_weapGfxBuffer.flashModel) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.flashModel, token->stringval, sizeof(cg_weapGfxBuffer.flashModel) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}


	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=======================
CG_weapGfx_ParseSkin
=======================
Parses 'skin' field.
Syntax:
'skin' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseSkin( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch ( category ) {
	case CAT_CHARGE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeSkin, "", sizeof(cg_weapGfxBuffer.chargeSkin) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeSkin, token->stringval, sizeof(cg_weapGfxBuffer.chargeSkin) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_EXPLOSION:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionSkin, "", sizeof(cg_weapGfxBuffer.explosionSkin) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionSkin, token->stringval, sizeof(cg_weapGfxBuffer.explosionSkin) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_STRUGGLE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileStruggleSkin, "", sizeof(cg_weapGfxBuffer.missileStruggleSkin) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileStruggleSkin, token->stringval, sizeof(cg_weapGfxBuffer.missileStruggleSkin) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_MISSILE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileSkin, "", sizeof(cg_weapGfxBuffer.missileSkin) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileSkin, token->stringval, sizeof(cg_weapGfxBuffer.missileSkin) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_FLASH:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.flashSkin, "", sizeof(cg_weapGfxBuffer.flashSkin) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.flashSkin, token->stringval, sizeof(cg_weapGfxBuffer.flashSkin) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=======================
CG_weapGfx_ParseShader
=======================
Parses 'shader' field.
Syntax:
'shader' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseShader( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch ( category ) {
	case CAT_CHARGE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeShader, "", sizeof(cg_weapGfxBuffer.chargeShader) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeShader, token->stringval, sizeof(cg_weapGfxBuffer.chargeShader) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_EXPLOSION:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionShader, "", sizeof(cg_weapGfxBuffer.explosionShader) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionShader, token->stringval, sizeof(cg_weapGfxBuffer.explosionShader) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_STRUGGLE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileStruggleShader, "", sizeof(cg_weapGfxBuffer.missileStruggleShader) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileStruggleShader, token->stringval, sizeof(cg_weapGfxBuffer.missileStruggleShader) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_MISSILE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileShader, "", sizeof(cg_weapGfxBuffer.missileShader) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileShader, token->stringval, sizeof(cg_weapGfxBuffer.missileShader) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_FLASH:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.flashShader, "", sizeof(cg_weapGfxBuffer.flashShader) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.flashShader, token->stringval, sizeof(cg_weapGfxBuffer.flashShader) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_TRAIL:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileTrailShader, "", sizeof(cg_weapGfxBuffer.missileTrailShader) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileTrailShader, token->stringval, sizeof(cg_weapGfxBuffer.missileTrailShader) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
================================
CG_weapGfx_ParseAnimationRange
================================
Parses 'animationRange' field.
Syntax:
'animationRange' '=' ( '[' <int> <int> ']' | <int> )
*/
qboolean CG_weapGfx_ParseAnimationRange( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch( category ) {
	case CAT_CHARGE:
		switch ( token->tokenSym ) {
		case TOKEN_INTEGER:
			if ( token->intval > 100 ) {
				CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "100" );
				return qfalse;
			}
			if ( token->intval < 0 ) {
				CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0" );
				return qfalse;
			}
			cg_weapGfxBuffer.chargeStartPct = token->intval;
			cg_weapGfxBuffer.chargeEndPct = token->intval;
			break;		

		case TOKEN_OPENRANGE:
			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_INTEGER ) {
				CG_weapGfx_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				if ( token->intval > 100 ) {
					CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "100" );
					return qfalse;
				}
				if ( token->intval < 0 ) {
					CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0" );
					return qfalse;
				}
				cg_weapGfxBuffer.chargeStartPct = token->intval;
			}

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_INTEGER ) {
				CG_weapGfx_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				
				// Make sure this range is not inverted!
				// NOTE: This is the only range that must NOT be inverted.
				if ( token->intval < cg_weapGfxBuffer.chargeStartPct ) {
					CG_weapGfx_ErrorHandle( ERROR_INVERTED_RANGE, scanner, token->stringval, va("%s", cg_weapGfxBuffer.chargeStartPct) );
					return qfalse;
				} else {
					if ( token->intval > 100 ) {
						CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "100" );
						return qfalse;
					}
					if ( token->intval < 0 ) {
						CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0" );
						return qfalse;
					}
					cg_weapGfxBuffer.chargeEndPct = token->intval;
				}
			}			

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_CLOSERANGE ) {
				CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
				return qfalse;
			}

			break;		

		default:
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
			break;
		}

		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
======================
CG_weapGfx_ParseSize
======================
Parses 'animationRange' field.
Syntax:
'size' '=' ( <int> | <float> )  |  ( '[' ( <int> | float ) ( <int> | <float> ']' )
*/
qboolean CG_weapGfx_ParseSize( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch ( category ) {
	case CAT_CHARGE:
		if ( (token->tokenSym == TOKEN_INTEGER) || (token->tokenSym == TOKEN_FLOAT) ) {
			cg_weapGfxBuffer.chargeStartsize = token->floatval;  // <-- floatval also stores intval
			cg_weapGfxBuffer.chargeEndsize = token->floatval;

		} else if ( token->tokenSym == TOKEN_OPENRANGE ) {

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
				CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				cg_weapGfxBuffer.chargeStartsize = token->floatval;
			}

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
				CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				cg_weapGfxBuffer.chargeEndsize = token->floatval;
			}

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_CLOSERANGE ) {
				CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
				return qfalse;
			}

		} else {

			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}

		break;

	case CAT_EXPLOSION:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.explosionSize = token->floatval; // <-- floatval also stores intval
		break;

	case CAT_MISSILE:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.missileSize = token->floatval; // <-- floatval also stores intval
		break;

	case CAT_TRAIL:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.missileTrailRadius = token->floatval; // <-- floatval also stores intval
		break;

	case CAT_FLASH:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.flashSize = token->floatval; // <-- floatval also stores intval
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}	

	return qtrue;
}


/*
========================
CG_weapGfx_ParseDlight
========================
Parses 'light' field.
Syntax:
'light' '=' '(' <i>|<f>  <i>|<f>  <i>|<f> ')' (   ( <i>|<f> )  |  ( '[' <i>|<f>  <i>|<f> ']' )   )
*/
qboolean CG_weapGfx_ParseDlight( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;
	vec3_t	RGB;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( (category != CAT_CHARGE) && (category != CAT_EXPLOSION) &&
		 (category != CAT_MISSILE) && (category != CAT_FLASH) ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}


	if ( token->tokenSym != TOKEN_OPENVECTOR ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "(" );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		if ( token->floatval > 1.0f ) {
			CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "1.0" );
			return qfalse;
		} else if ( token->floatval < 0.0f ) {
			CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0.0" );
		} else {
			RGB[0] = token->floatval;
		}
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		if ( token->floatval > 1.0f ) {
			CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "1.0" );
			return qfalse;
		} else if ( token->floatval < 0.0f ) {
			CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0.0" );
		} else {
			RGB[1] = token->floatval;
		}
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		if ( token->floatval > 1.0f ) {
			CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "1.0" );
			return qfalse;
		} else if ( token->floatval < 0.0f ) {
			CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0.0" );
		} else {
			RGB[2] = token->floatval;
		}
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_CLOSEVECTOR ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, ")" );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	switch ( category ) {
	case CAT_CHARGE:
		if ( (token->tokenSym == TOKEN_INTEGER) || (token->tokenSym == TOKEN_FLOAT) ) {
			cg_weapGfxBuffer.chargeDlightStartRadius = token->floatval;
			cg_weapGfxBuffer.chargeDlightEndRadius = token->floatval;

		} else if ( token->tokenSym == TOKEN_OPENRANGE ) {

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
				CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				cg_weapGfxBuffer.chargeDlightStartRadius = token->floatval;
			}

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
				CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				cg_weapGfxBuffer.chargeDlightEndRadius = token->floatval;
			}

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_CLOSERANGE ) {
				CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
				return qfalse;
			}

		} else {

			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}

		VectorCopy( RGB, cg_weapGfxBuffer.chargeDlightColor );

		break;

	case CAT_EXPLOSION:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.explosionDlightRadius = token->floatval;
		VectorCopy( RGB, cg_weapGfxBuffer.explosionDlightColor );
		break;

	case CAT_MISSILE:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.missileDlightRadius = token->floatval;
		VectorCopy( RGB, cg_weapGfxBuffer.missileDlightColor );
		break;

	case CAT_FLASH:
		if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
			CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		cg_weapGfxBuffer.flashDlightRadius = token->floatval;
		VectorCopy( RGB, cg_weapGfxBuffer.flashDlightColor );
		break;

	default:
		// Unable to happen
		break;
	}
	
	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
======================
CG_weapGfx_ParseSpin
======================
Parses 'spin' field.
Syntax:
'spin' '=' '(' <i>|<f>  <i>|<f>  <i>|<f> ')'
*/
qboolean CG_weapGfx_ParseSpin( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;
	vec3_t	spin;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( (category != CAT_CHARGE) && (category != CAT_MISSILE) ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_OPENVECTOR ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "(" );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		spin[0] = token->floatval;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		spin[1] = token->floatval;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		spin[2] = token->floatval;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_CLOSEVECTOR ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, ")" );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	switch ( category ) {
	case CAT_CHARGE:
		VectorCopy( spin, cg_weapGfxBuffer.chargeSpin );
		break;
	case CAT_MISSILE:
		VectorCopy( spin, cg_weapGfxBuffer.missileSpin );
		break;
	default:
		// Unable to happen
		break;
	}
	return qtrue;
}


/*
=======================
CG_weapGfx_ParseTagTo
=======================
Parses 'tagTo' field.
Syntax:
'tagTo' '=' "tagname"
FIXME: Should become later:
'tagTo' '=' ( "tagname" | 'null' ) ( "tagname" | 'null' )
*/
qboolean CG_weapGfx_ParseTagTo( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_CHARGE ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	// FIXME: When later on we can specify two tags, we will have to be able to pass
	//        'null' for the 2nd one, to disable the 2nd tag, or to the 1st to change
	//        the 2nd tag into the first.
	if ( token->tokenSym != TOKEN_STRING ) {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		Q_strncpyz( cg_weapGfxBuffer.chargeTag[0], token->stringval, sizeof(cg_weapGfxBuffer.chargeTag[0]) );
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=========================
CG_weapGfx_ParseSoundFx
=========================
Parses 'soundFx' field.
Syntax:
'soundFx' '=' ( 'null' | "filename" | '(' "filename"* ')' )
*/
qboolean CG_weapGfx_ParseSoundFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;
	int i;

	scanner = &parser->scanner;
	token = &parser->token;
	
	switch ( category ) {
	case CAT_EXPLOSION:
		i = 0;
		if ( token->tokenSym == TOKEN_NULL ) {
			memset(cg_weapGfxBuffer.explosionSound, 0, sizeof(char)*4*MAX_QPATH ); // <-- Maximum of 4 samples
		} else if ( token->tokenSym == TOKEN_STRING ) {
			memset(cg_weapGfxBuffer.explosionSound, 0, sizeof(char)*4*MAX_QPATH );
			Q_strncpyz( cg_weapGfxBuffer.explosionSound[0], token->stringval, sizeof(cg_weapGfxBuffer.explosionSound[0]) );

		} else if ( token->tokenSym == TOKEN_OPENVECTOR ) {
			// Intialize everything blank
			memset(cg_weapGfxBuffer.explosionSound, 0, sizeof(char)*4*MAX_QPATH );

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			while ( token->tokenSym == TOKEN_STRING ) {
				if ( i >= 4 ) {
					CG_weapGfx_ErrorHandle( ERROR_OVER_MAXVECTORELEMS, scanner, token->stringval, "4" );
					return qfalse;
				}

				Q_strncpyz( cg_weapGfxBuffer.explosionSound[i], token->stringval, sizeof(cg_weapGfxBuffer.explosionSound[i]) );

				i++;

				if ( !CG_weapGfx_NextSym( scanner, token ) ) {
					if ( token->tokenSym == TOKEN_EOF ) {
						CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
					}
					return qfalse;
				}
			}

			if ( token->tokenSym != TOKEN_CLOSEVECTOR ) {
				CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, ")" );
				return qfalse;
			}
		} else {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}
		break;

	case CAT_FLASH:
		i = 0;
		if ( token->tokenSym == TOKEN_NULL ) {
			memset(cg_weapGfxBuffer.flashSound, 0, sizeof(char)*4*MAX_QPATH );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			memset(cg_weapGfxBuffer.flashSound, 0, sizeof(char)*4*MAX_QPATH );
			Q_strncpyz( cg_weapGfxBuffer.flashSound[0], token->stringval, sizeof(cg_weapGfxBuffer.flashSound[0]) );

		} else if ( token->tokenSym == TOKEN_OPENVECTOR ) {
			// Intialize everything blank
			memset( cg_weapGfxBuffer.flashSound, 0, sizeof(char)*4*MAX_QPATH );

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			while ( token->tokenSym == TOKEN_STRING ) {
				if ( i >= 4 ) {
					CG_weapGfx_ErrorHandle( ERROR_OVER_MAXVECTORELEMS, scanner, token->stringval, "4" );
					return qfalse;
				}

				Q_strncpyz( cg_weapGfxBuffer.flashSound[i], token->stringval, sizeof(cg_weapGfxBuffer.flashSound[i]) );

				i++;

				if ( !CG_weapGfx_NextSym( scanner, token ) ) {
					if ( token->tokenSym == TOKEN_EOF ) {
						CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
					}
					return qfalse;
				}
			}

			if ( token->tokenSym != TOKEN_CLOSEVECTOR ) {
				CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, ")" );
				return qfalse;
			}
		} else {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*
=========================
CG_weapGfx_ParseVoiceFx
=========================
Parses 'voiceFx' field.
Syntax:
'voiceFx' '=' ( 'null' | "filename" | '(' "filename"* ')' )
*/
qboolean CG_weapGfx_ParseVoiceFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;
	int i;
	scanner = &parser->scanner;
	token = &parser->token;
	switch ( category ) {
	case CAT_FLASH:
		i = 0;
		if ( token->tokenSym == TOKEN_NULL ) {
			memset(cg_weapGfxBuffer.voiceSound, 0, sizeof(char)*4*MAX_QPATH );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			memset(cg_weapGfxBuffer.voiceSound, 0, sizeof(char)*4*MAX_QPATH );
			Q_strncpyz( cg_weapGfxBuffer.voiceSound[0], token->stringval, sizeof(cg_weapGfxBuffer.voiceSound[0]) );

		} else if ( token->tokenSym == TOKEN_OPENVECTOR ) {
			// Intialize everything blank
			memset( cg_weapGfxBuffer.voiceSound, 0, sizeof(char)*4*MAX_QPATH );

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			while ( token->tokenSym == TOKEN_STRING ) {
				if ( i >= 4 ) {
					CG_weapGfx_ErrorHandle( ERROR_OVER_MAXVECTORELEMS, scanner, token->stringval, "4" );
					return qfalse;
				}

				Q_strncpyz( cg_weapGfxBuffer.voiceSound[i], token->stringval, sizeof(cg_weapGfxBuffer.voiceSound[i]) );

				i++;

				if ( !CG_weapGfx_NextSym( scanner, token ) ) {
					if ( token->tokenSym == TOKEN_EOF ) {
						CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
					}
					return qfalse;
				}
			}

			if ( token->tokenSym != TOKEN_CLOSEVECTOR ) {
				CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, ")" );
				return qfalse;
			}
		} else {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}
	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}
	return qtrue;
}


/*
=========================
CG_weapGfx_ParseLoopFx
=========================
Parses 'loopFx' field.
Syntax:
'loopFx' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseLoopFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;
	
	switch ( category ) {
	case CAT_CHARGE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeLoopSound, "", sizeof(cg_weapGfxBuffer.chargeLoopSound) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeLoopSound, token->stringval, sizeof(cg_weapGfxBuffer.chargeLoopSound) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_MISSILE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileSound, "", sizeof(cg_weapGfxBuffer.missileSound) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileSound, token->stringval, sizeof(cg_weapGfxBuffer.missileSound) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_FLASH:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.firingSound, "", sizeof(cg_weapGfxBuffer.firingSound) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.firingSound, token->stringval, sizeof(cg_weapGfxBuffer.firingSound) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=========================
CG_weapGfx_ParseTimedFx
=========================
Parses 'timedFx' field.
Syntax:
'timedFx' '=' ( ( '(' ( <i> "filename" )* ')' ) | 'null' )
*/
qboolean CG_weapGfx_ParseTimedFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;
	int i;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_CHARGE ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	memset( cg_weapGfxBuffer.chargeVoice, 0, sizeof(chargeVoiceParseBuffer_t)*6 ); // <-- Maximum of 6 timed samples

	// Don't have to do anything but blank it out and advance the token, if 'null' was passed.
	if ( token->tokenSym == TOKEN_NULL ) {
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		return qtrue;
	}

	if ( token->tokenSym != TOKEN_OPENVECTOR ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "(" );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	i = 0;
	while ( token->tokenSym == TOKEN_INTEGER ) {
		if ( i >= 6 ) { // <-- Maximum of 6 timed samples
			CG_weapGfx_ErrorHandle( ERROR_OVER_MAXVECTORELEMS, scanner, token->stringval, "8" );
			return qfalse;
		}

		if ( token->intval > 100 ) {
			CG_weapGfx_ErrorHandle( ERROR_OVER_MAXBOUND, scanner, token->stringval, "100" );
			return qfalse;
		} else 	if ( token->intval < 0 ) {
			CG_weapGfx_ErrorHandle( ERROR_UNDER_MINBOUND, scanner, token->stringval, "0" );
			return qfalse;
		} else {
			cg_weapGfxBuffer.chargeVoice[i].startPct = token->intval;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( cg_weapGfxBuffer.chargeVoice[i].voice, token->stringval, sizeof(cg_weapGfxBuffer.chargeVoice[i].voice) );
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		i++;
	}

	if ( token->tokenSym != TOKEN_CLOSEVECTOR ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=========================
CG_weapGfx_ParseOnceFx
=========================
Parses 'onceFx' field.
Syntax:
'onceFx' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseOnceFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_FLASH ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.flashOnceSound, "", sizeof(cg_weapGfxBuffer.flashOnceSound) );
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.flashOnceSound, token->stringval, sizeof(cg_weapGfxBuffer.flashOnceSound) );
	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}		

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==========================
CG_weapGfx_ParseDuration
==========================
Parses 'duration' field.
Syntax:
'duration' '=' <int>
*/
qboolean CG_weapGfx_ParseDuration( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_EXPLOSION ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		CG_weapGfx_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	} else {
		cg_weapGfxBuffer.explosionTime = token->intval;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==========================
CG_weapGfx_ParseShockwave
==========================
Parses 'shockwave' field.
Syntax:
'shockwave' '=' ( ("filename" "filename") | null )
*/
qboolean CG_weapGfx_ParseShockwave( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_EXPLOSION ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.shockwaveModel, "", sizeof(cg_weapGfxBuffer.shockwaveModel));
		Q_strncpyz( cg_weapGfxBuffer.shockwaveSkin, "", sizeof(cg_weapGfxBuffer.shockwaveSkin));
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.shockwaveModel, token->stringval, sizeof(cg_weapGfxBuffer.shockwaveModel));

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.shockwaveSkin, token->stringval, sizeof(cg_weapGfxBuffer.shockwaveSkin));
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
============================
CG_weapGfx_ParseMarkShader
============================
Parses 'markShader' field.
Syntax:
'markShader' '=' ( "filename" | null )
*/
qboolean CG_weapGfx_ParseMarkShader( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_EXPLOSION ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.markShader, "", sizeof(cg_weapGfxBuffer.markShader));
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.markShader, token->stringval, sizeof(cg_weapGfxBuffer.markShader));
	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==========================
CG_weapGfx_ParseMarkSize
==========================
Parses 'markSize' field.
Syntax:
'markSize' '=' <int>
*/
qboolean CG_weapGfx_ParseMarkSize( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_EXPLOSION ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	cg_weapGfxBuffer.markSize = token->floatval; // <-- floatval also stores intval

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==========================
CG_weapGfx_ParseRockDeris
==========================
Parses 'noRockDebris' field.
Syntax:
'noRockDebris' '=' <int>
*/
qboolean CG_weapGfx_ParseRockDebris( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_EXPLOSION ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		CG_weapGfx_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	cg_weapGfxBuffer.noRockDebris = token->intval;

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*
================================
CG_weapGfx_ParseSmokeParticles
================================
Parses 'smokeParticles' field.
Syntax:
'smokeParticles' '=' "system name" | 'null'
*/
qboolean CG_weapGfx_ParseSmokeParticles( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_EXPLOSION ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.smokeParticleSystem, "", sizeof(cg_weapGfxBuffer.smokeParticleSystem) );
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.smokeParticleSystem, token->stringval, sizeof(cg_weapGfxBuffer.smokeParticleSystem) );
	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}		

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
===============================
CG_weapGfx_ParseLoopParticles
===============================
Parses 'loopParticles' field.
Syntax:
'loopParticles' '=' "system name" | 'null'
*/
qboolean CG_weapGfx_ParseLoopParticles( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch ( category ) {
	case CAT_CHARGE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeParticleSystem, "", sizeof(cg_weapGfxBuffer.chargeParticleSystem) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.chargeParticleSystem, token->stringval, sizeof(cg_weapGfxBuffer.chargeParticleSystem) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_FLASH:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.firingParticleSystem, "", sizeof(cg_weapGfxBuffer.firingParticleSystem) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.firingParticleSystem, token->stringval, sizeof(cg_weapGfxBuffer.firingParticleSystem) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
===========================
CG_weapGfx_ParseParticles
===========================
Parses 'particles' field.
Syntax:
'particles' '=' "system name" | 'null'
*/
qboolean CG_weapGfx_ParseParticles( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	switch ( category ) {
	case CAT_EXPLOSION:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionParticleSystem, "", sizeof(cg_weapGfxBuffer.explosionParticleSystem) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.explosionParticleSystem, token->stringval, sizeof(cg_weapGfxBuffer.explosionParticleSystem) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_MISSILE:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.missileParticleSystem, "", sizeof(cg_weapGfxBuffer.missileParticleSystem) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.missileParticleSystem, token->stringval, sizeof(cg_weapGfxBuffer.missileParticleSystem) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	case CAT_FLASH:
		if ( token->tokenSym == TOKEN_NULL ) {
			Q_strncpyz( cg_weapGfxBuffer.flashParticleSystem, "", sizeof(cg_weapGfxBuffer.flashParticleSystem) );
		} else if ( token->tokenSym == TOKEN_STRING ) {
			Q_strncpyz( cg_weapGfxBuffer.flashParticleSystem, token->stringval, sizeof(cg_weapGfxBuffer.flashParticleSystem) );
		} else {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}		
		break;

	default:
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
		break;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==============================
CG_weapGfx_ParseSpiralShader
==============================
Parses 'spiralShader' field.
Syntax:
'spiralShader' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseSpiralShader( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_TRAIL ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.missileTrailSpiralShader, "", sizeof(cg_weapGfxBuffer.missileTrailSpiralShader) );
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.missileTrailSpiralShader, token->stringval, sizeof(cg_weapGfxBuffer.missileTrailSpiralShader) );
	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
============================
CG_weapGfx_ParseSpiralSize
============================
Parses 'spiralSize' field.
Syntax:
'spiralSize' '=' (<int> | <float>)
*/
qboolean CG_weapGfx_ParseSpiralSize( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_TRAIL ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( (token->tokenSym == TOKEN_INTEGER) || (token->tokenSym == TOKEN_FLOAT) ) {
		cg_weapGfxBuffer.missileTrailSpiralRadius = token->floatval;
	} else {
		CG_weapGfx_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==============================
CG_weapGfx_ParseSpiralOffset
==============================
Parses 'spiralOffset' field.
Syntax:
'spiralOffset' '=' (<int> | <float>)
*/
qboolean CG_weapGfx_ParseSpiralOffset( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_TRAIL ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( (token->tokenSym == TOKEN_INTEGER) || (token->tokenSym == TOKEN_FLOAT) ) {
		cg_weapGfxBuffer.missileTrailSpiralOffset = token->floatval;
	} else {
		CG_weapGfx_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
======================
CG_weapGfx_ParseIcon
======================
Parses 'icon' field.
Syntax:
'icon' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseIcon( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_HUD ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.weaponIcon, "", sizeof(cg_weapGfxBuffer.missileTrailSpiralShader) );
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.weaponIcon, token->stringval, sizeof(cg_weapGfxBuffer.missileTrailSpiralShader) );
	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=============================
CG_weapGfx_ParseDisplayName
=============================
Parses 'displayName' field.
Syntax:
'displayName' '=' ( "filename" | 'null' )
*/
qboolean CG_weapGfx_ParseDisplayName( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	if ( category != CAT_HUD ) {
		CG_weapGfx_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, cg_weapGfxFields[field].fieldname, cg_weapGfxCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_NULL ) {
		Q_strncpyz( cg_weapGfxBuffer.weaponName, "", sizeof(cg_weapGfxBuffer.missileTrailSpiralShader) );
	} else if ( token->tokenSym == TOKEN_STRING ) {
		Q_strncpyz( cg_weapGfxBuffer.weaponName, token->stringval, sizeof(cg_weapGfxBuffer.missileTrailSpiralShader) );
	} else {
		CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


// ==================================================================================
//
//    F  I  X  E  D     F  U  N  C  T  I  O  N  S  - Do not change the below.
//
// ==================================================================================

/*
=========================
CG_weapGfx_ParseImports
=========================
Parses import definitions and stores them
in a reference list.
*/
static qboolean CG_weapGfx_ParseImports( cg_weapGfxParser_t *parser ) {
	cg_weapGfxToken_t		*token;
	cg_weapGfxScanner_t		*scanner;
	char					refname[MAX_TOKENSTRING_LENGTH];
	char					filename[MAX_TOKENSTRING_LENGTH];

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   'import' "<refname>" '=' "<filename>" "<defname>"

	while ( token->tokenSym == TOKEN_IMPORT ) {

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( refname, token->stringval, sizeof(refname) );
		}
		
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_EQUALS ) {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "=" );
			return qfalse;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( filename, token->stringval, sizeof(filename) );
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		if ( !CG_weapGfx_AddImportRef( parser, refname, filename, token->stringval ) ) {
			return qfalse;
		}

		// NOTE: Do it like this to prevent errors if a file happens to only contain
		//       import lines. While it is actually useless to have such a file, it
		//       still is syntacticly correct.
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym != TOKEN_EOF ) {
				return qfalse;
			}
		}
	}

	return qtrue;
}


/*
========================
CG_weapGfx_ParseFields
========================
Parses the fields of one category
within a weapon definition.
*/
static qboolean CG_weapGfx_ParseFields( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category ) {
	cg_weapGfxToken_t	*token;
	cg_weapGfxScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	while ( token->tokenSym == TOKEN_FIELD ) {
		int field;

		field = token->identifierIndex;

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_EQUALS ) {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "=" );
			return qfalse;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if (!cg_weapGfxFields[field].parseFunc( parser, category, field ) ) {
			return qfalse;
		}		
	}
	return qtrue;
}


/*
============================
CG_weapGfx_ParseCategories
============================
Parses the categories of a weapon
definition.
*/
static qboolean CG_weapGfx_ParseCategories( cg_weapGfxParser_t *parser ) {
	cg_weapGfxToken_t		*token;
	cg_weapGfxScanner_t		*scanner;
	
	int		currentCategory;

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   <categoryname> '{' <HANDLE FIELDS> '}'

	while ( token->tokenSym == TOKEN_CATEGORY ) {
		currentCategory = token->identifierIndex;

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_OPENBLOCK ) {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		// Handles fields
		if (!CG_weapGfx_ParseFields( parser, currentCategory ) ) {
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_CLOSEBLOCK ) {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "}" );
			return qfalse;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}
	}

	if ( token->tokenSym != TOKEN_CLOSEBLOCK ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "}" );
		return qfalse;
	}

	if ( cg_verboseParse.integer ) {
		CG_Printf("Processed categories succesfully.\n");
	}

	return qtrue;
}


/*
================================
CG_weapGfx_PreParseDefinitions
================================
Pre-parses the definitions of the weapons in the
scriptfile. The contents of a definition block
are checked for validity by the lexical scanner
and parser to make sure syntax is correct.
The parsed values however, are not yet stored,
because we don't have a link to an actual
weapon yet. Entry points into the definitions
are cached into a table for a quick jump once
we find out the links in CG_weapGfx_ParseLinks.
*/
static qboolean CG_weapGfx_PreParseDefinitions( cg_weapGfxParser_t *parser ) {
	cg_weapGfxToken_t		*token;
	cg_weapGfxScanner_t		*scanner;

	int						defline;
	char					*defpos;
	cg_weapGfxAccessLvls_t	accessLvl;
	qboolean				hasSuper;
	char					supername[MAX_TOKENSTRING_LENGTH];
	char					refname[MAX_TOKENSTRING_LENGTH];


	int						blockCount;

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   ( 'public' | 'protected' | 'private' ) "<refname>" ( e | '=' ( "<importref>" | "<definitionref>" ) ) '{' <HANDLE CATEGORIES> '}'
	while ( ( token->tokenSym == TOKEN_PRIVATE ) || ( token->tokenSym == TOKEN_PUBLIC ) || ( token->tokenSym == TOKEN_PROTECTED ) ) {
		
		if ( token->tokenSym == TOKEN_PUBLIC ) {
			accessLvl = LVL_PUBLIC;
		} else if ( token->tokenSym == TOKEN_PROTECTED ) {
			accessLvl = LVL_PROTECTED;
		} else {
			accessLvl = LVL_PRIVATE;
		}

		hasSuper = qfalse;

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( refname, token->stringval, sizeof(refname) );
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}
		
		// Are we deriving?
		if ( token->tokenSym == TOKEN_EQUALS ) {
			hasSuper = qtrue;

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_STRING ) {
				CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			}

			if ( !CG_weapGfx_FindDefinitionRef( parser, token->stringval ) ) {
				return qfalse;
			} else {
				Q_strncpyz( supername, token->stringval, sizeof(supername) );
			}

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}
		}

		if ( token->tokenSym != TOKEN_OPENBLOCK ) {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "{" );
			return qfalse;
		} else {
			blockCount = 1;
			defline = scanner->line;
			defpos = scanner->pos;
			if (!CG_weapGfx_AddDefinitionRef( parser, refname, defpos, defline, accessLvl, hasSuper, supername ) ) {
				return qfalse;
			}
		}

		while ( blockCount > 0 ) {

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym == TOKEN_OPENBLOCK ) {
				blockCount++;
			}

			if ( token->tokenSym == TOKEN_CLOSEBLOCK ) {
				blockCount--;
			}
		}

		// NOTE: This makes sure we don't get an error if a file contains no weapon link
		//       lines, but instead terminates after the definitions. This is what one
		//       would expect of 'library' files.
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym != TOKEN_EOF ) {
				return qfalse;
			}
		}
	}

	return qtrue;
}


/*
=======================
CG_weapGfx_ParseLinks
=======================
Parses weapon links to definitions and continues to
parse the actual weapon definitions.
*/
static qboolean CG_weapGfx_ParseLinks( cg_weapGfxParser_t *parser ) {
	cg_weapGfxToken_t		*token;
	cg_weapGfxScanner_t		*scanner;

	int						weaponNum;
	char					pri_refname[MAX_TOKENSTRING_LENGTH];
	char					sec_refname[MAX_TOKENSTRING_LENGTH];

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   'weapon' <int> '=' "<refname>" ( e | '|' "<refname>" )

	while ( token->tokenSym == TOKEN_WEAPON ) {
		
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			CG_weapGfx_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			weaponNum = token->intval;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_EQUALS ) {
			CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "=" );
			return qfalse;
		}

		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner,  token->stringval, NULL );
			return qfalse;
		}

		if ( !CG_weapGfx_FindDefinitionRef( parser, token->stringval ) ) {
			CG_weapGfx_ErrorHandle( ERROR_DEFINITION_UNDEFINED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( pri_refname, token->stringval, sizeof(pri_refname) );
		}

		// NOTE: This makes sure we don't get an error if the last link in the file contains
		//       no secondary definition, but instead terminates after the primary one.
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym != TOKEN_EOF ) {
				return qfalse;
			}
		}

		if ( token->tokenSym == TOKEN_COLON ) {

			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_STRING ) {
				CG_weapGfx_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			}

			if ( !CG_weapGfx_FindDefinitionRef( parser, token->stringval ) ) {
				CG_weapGfx_ErrorHandle( ERROR_DEFINITION_UNDEFINED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				Q_strncpyz( sec_refname, token->stringval, sizeof(sec_refname) );
			}

			// NOTE: This makes sure we don't get an error if this is the last link in the file.
			if ( !CG_weapGfx_NextSym( scanner, token ) ) {
				if ( token->tokenSym != TOKEN_EOF ) {
					return qfalse;
				}
			}

		} else {
			Q_strncpyz( sec_refname, "", sizeof(sec_refname) );
		}

		if ( !CG_weapGfx_AddLinkRef( parser, weaponNum, pri_refname, sec_refname ) ) {
			return qfalse;
		}
	}

	return qtrue;
}



/*
===================================
CG_weapGfx_IncreaseRecursionDepth
===================================
Increases inheritance recursion depth.
Checks if the maximum level is exceeded and returns
an error if so.
*/
static qboolean CG_weapGfx_IncreaseRecursionDepth( void ) {
	if ( cg_weapGfxRecursionDepth == MAX_RECURSION_DEPTH ) {
		return qfalse;
	}

	cg_weapGfxRecursionDepth++;
	return qtrue;
}


/*
===================================
CG_weapGfx_DecreaseRecursionDepth
===================================
Decreases inheritance recursion depth.
*/
static void CG_weapGfx_DecreaseRecursionDepth( void ) {
	cg_weapGfxRecursionDepth--;
}

// Need this prototyped
qboolean CG_weapGfx_ParseDefinition( cg_weapGfxParser_t *parser, char* refname, cg_weapGfxAccessLvls_t *accessLvl );

/*
==================================
CG_weapGfx_ParseRemoteDefinition
==================================
Instantiates a new parser and scanner
to parse a remote definition
*/
qboolean CG_weapGfx_ParseRemoteDefinition( char *filename, char *refname ) {
	cg_weapGfxParser_t		parser;
	cg_weapGfxScanner_t		*scanner;
	cg_weapGfxToken_t		*token;
	int						i;

	// Initialize the parser
	memset( &parser, 0, sizeof(parser) );
	scanner = &parser.scanner;
	token = &parser.token;	

	// Initialize the scanner by loading the file
	CG_weapGfx_LoadFile( scanner, filename );

	// Get the very first token initialized. If
	// it is an end of file token, we will not parse
	// the empty file but will instead exit with true.
	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym != TOKEN_EOF ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	// Parse the imports
	if ( !CG_weapGfx_ParseImports( &parser ) ) {
		return qfalse;
	}

	// Pre-Parse the definitions
	if ( !CG_weapGfx_PreParseDefinitions( &parser ) ) {
		return qfalse;
	}

	// Parse the link to the weapons
	// NOTE: We don't really need to do this, but it does
	//       ensure file structure.
	if ( !CG_weapGfx_ParseLinks( &parser ) ) {
		return qfalse;
	}

	// Respond with an error if something is trailing the
	// link definitions.
	// NOTE: We don't really need to do this, but it does
	//       ensure file structure.
	if ( token->tokenSym != TOKEN_EOF ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	// If we're dealing with a local definition in this file, then that definition
	// MUST be public, since we're importing it to another file.
	i = CG_weapGfx_FindDefinitionRef( &parser, refname ) - 1;
	if ( i < MAX_DEFINES ) {
		if ( parser.definitionRef[i].accessLvl != LVL_PUBLIC ) {
			scanner->line = parser.definitionRef[i].scannerLine; // <-- Make sure the error reports the correct line
			CG_weapGfx_ErrorHandle( ERROR_IMPORTING_NON_PUBLIC, scanner, parser.definitionRef[i].refname, NULL );
			return qfalse;
		}
	}

	if ( !CG_weapGfx_ParseDefinition( &parser, refname, NULL ) ) {
		return qfalse;
	}
	
	return qtrue;
}


/*
============================
CG_weapGfx_ParseDefinition
============================
Parses a definition, taking inheritance into account.
*/
qboolean CG_weapGfx_ParseDefinition( cg_weapGfxParser_t *parser, char* refname, cg_weapGfxAccessLvls_t *accessLvl ) {
	int i;
	cg_weapGfxScanner_t	*scanner;
	cg_weapGfxToken_t	*token;
	cg_weapGfxAccessLvls_t lastAccessLvl;

	lastAccessLvl = LVL_PUBLIC; // <-- Incase there IS no last access level from a super class
	scanner = &parser->scanner;
	token = &parser->token;

	i = CG_weapGfx_FindDefinitionRef( parser, refname ) - 1; // <-- Must subtract one to get proper index!
	if ( i < MAX_DEFINES ) {
		// local declaration
		if ( parser->definitionRef[i].hasSuper ) {
			
			if (!CG_weapGfx_IncreaseRecursionDepth() ) {
				CG_weapGfx_ErrorHandle( ERROR_MAX_RECURSION, scanner, NULL, NULL );
				return qfalse;
			}

			if ( cg_verboseParse.integer ) {
				CG_Printf("Inheriting superclass '%s'\n", parser->definitionRef[i].supername );
			}
			if ( !CG_weapGfx_ParseDefinition( parser, parser->definitionRef[i].supername, &lastAccessLvl ) ) {
				return qfalse;
			}
			
			CG_weapGfx_DecreaseRecursionDepth();
		}

		// Check if the super class was private, instead of public or protected
		if ( lastAccessLvl == LVL_PRIVATE ) {
			CG_weapGfx_ErrorHandle( ERROR_INHERITING_PRIVATE, scanner, parser->definitionRef[i].supername, NULL );
			return qfalse;
		}

		// Reposition the lexical scanner
		scanner->pos = parser->definitionRef[i].scannerPos;
		scanner->line = parser->definitionRef[i].scannerLine;

		// Check if we're not breaking access level hierarchy
		if ( accessLvl ) {
			*accessLvl = parser->definitionRef[i].accessLvl;
			if ( *accessLvl < lastAccessLvl ) {
				CG_weapGfx_ErrorHandle( ERROR_OVERRIDING_WITH_HIGHER_ACCESS, scanner, parser->definitionRef[i].refname, NULL );
				return qfalse;
			}
		}

		// Skip the '{' opening brace of the definition block, and align to the first real
		// symbol in the block.
		if ( !CG_weapGfx_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				CG_weapGfx_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		// Parse the block's categories.
		if ( !CG_weapGfx_ParseCategories( parser ) ) {
			return qfalse;
		}

	} else {
		// imported declaration
		
		// First subtract the offset we added to detect difference between
		// an imported and a local definition.
		i -= MAX_DEFINES;

		if (!CG_weapGfx_IncreaseRecursionDepth() ) {
				CG_weapGfx_ErrorHandle( ERROR_MAX_RECURSION, scanner, NULL, NULL );
				return qfalse;
		}

		if ( cg_verboseParse.integer ) {
			CG_Printf("Importing '%s'\n", refname );
		}		

		if ( !CG_weapGfx_ParseRemoteDefinition( parser->importRef[i].filename, parser->importRef[i].defname ) ) {
			return qfalse;
		}

		CG_weapGfx_DecreaseRecursionDepth();
	}

	return qtrue;

}

/*
==================
CG_weapGfx_Parse
==================
Main parsing function for a scriptfile.
This is the parser's 'entrypoint',
*/
qboolean CG_weapGfx_Parse( char *filename, int clientNum ) {
	cg_weapGfxParser_t		parser;
	cg_weapGfxScanner_t		*scanner;
	cg_weapGfxToken_t		*token;
	int						i;

	// Initialize the parser
	memset( &parser, 0, sizeof(parser) );
	scanner = &parser.scanner;
	token = &parser.token;
	cg_weapGfxRecursionDepth = 0;
	
	// Initialize the scanner by loading the file
	CG_weapGfx_LoadFile( scanner, filename );

	// Get the very first token initialized. If
	// it is an end of file token, we will not parse
	// the empty file but will instead exit with true.
	if ( !CG_weapGfx_NextSym( scanner, token ) ) {
		if ( token->tokenSym != TOKEN_EOF ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	// Parse the imports
	if ( !CG_weapGfx_ParseImports( &parser ) ) {
		return qfalse;
	}

	// Pre-Parse the definitions
	if ( !CG_weapGfx_PreParseDefinitions( &parser ) ) {
		return qfalse;
	}

	// Parse the link to the weapons
	if ( !CG_weapGfx_ParseLinks( &parser ) ) {
		return qfalse;
	}

	// Respond with an error if something is trailing the
	// link definitions.
	if ( token->tokenSym != TOKEN_EOF ) {
		CG_weapGfx_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	// Work through the link table, parsing and assigning the definitions
	// as we go.
	for ( i = 0; i < MAX_LINKS; i++ ) {

		if (!parser.linkRef[i].active) {
			continue;
		}

		// Empty the buffer.
		memset( &cg_weapGfxBuffer, 0, sizeof(cg_weapGfxBuffer) );

		if ( cg_verboseParse.integer ) {
			CG_Printf("Processing weapon nr %i, primary '%s'.\n", i+1, parser.linkRef[i].pri_refname );
		}

		if ( !CG_weapGfx_ParseDefinition( &parser, parser.linkRef[i].pri_refname, NULL ) ) {
			return qfalse;
		}

		CG_weapGfx_StoreBuffer( clientNum, i );

		// Empty the buffer.
		memset( &cg_weapGfxBuffer, 0, sizeof(cg_weapGfxBuffer) );

		if ( strcmp( parser.linkRef[i].sec_refname, "" ) ) {
			if ( cg_verboseParse.integer ) {
				CG_Printf("Processing weapon nr %i, secondary '%s'.\n", i+1, parser.linkRef[i].sec_refname );
			}
			if ( !CG_weapGfx_ParseDefinition( &parser, parser.linkRef[i].sec_refname, NULL ) ) {
				return qfalse;
			}
		}

		CG_weapGfx_StoreBuffer( clientNum, i + ALTWEAPON_OFFSET );

	}

	if ( cg_verboseParse.integer ) {
		CG_Printf("Parse completed succesfully.\n");
	}

	return qtrue;
}
