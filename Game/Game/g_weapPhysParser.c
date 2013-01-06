// Copyright (C) 2003-2005 RiO
//
// g_weapGfxParser.c -- token parser for ZEQ2's weapon physics script language.

#include "g_weapPhysParser.h" // <-- cg_local.h included in this


static int							g_weapPhysRecursionDepth;
static g_userWeaponParseBuffer_t	g_weapPhysBuffer;
	// FIXME: Can this be a local variable instead, or would it give us
	//		  > 32k locals errors in the VM-bytecode compiler?

/*
=======================
G_weapPhys_StoreBuffer
=======================
Copies the contents in the buffer to a client's weapon
configuration, converting filestrings into qhandle_t in
the process.
*/
void G_weapPhys_StoreBuffer(int clientNum, int weaponNum) {
	g_userWeapon_t				*dest;
	g_userWeaponParseBuffer_t	*src;

	src = &g_weapPhysBuffer;
	dest = G_FindUserWeaponData( clientNum, weaponNum + 1);
	memset( dest, 0, sizeof(g_userWeapon_t));

	// Size and form of buffer and storage is equal, so just
	// copy the memory.
	memcpy( dest, src, sizeof(g_userWeapon_t));

	// Indicate the weapon as charged if there is a chargeReadyPct
	// or chargeTime set to something other than 0.
	if ( src->costs_chargeReady || src->costs_chargeTime ) {
		dest->general_bitflags |= WPF_NEEDSCHARGE;
	}

	// If nrShots is set to < 1, internally it must be represented by 1 instead.
	if ( dest->firing_nrShots < 1 ) dest->firing_nrShots = 1;

	// Handle the presence of alternate fire weapons through the
	// piggybacked WPF_ALTWEAPONPRESENT flag.
	
	// NOTE: The WPF_ALTWEAPONPRESENT flag will only be enabled in src if src is the
	//       buffer for a secondary fire configuration. In this case it is implicitly
	//       safe to subtract ALTWEAPON_OFFSET to obtain the primary fire index
	//       of the weapon!
	if ( src->general_bitflags & WPF_ALTWEAPONPRESENT ) {
		src->general_bitflags &= ~WPF_ALTWEAPONPRESENT;
		dest = G_FindUserWeaponData( clientNum, weaponNum - ALTWEAPON_OFFSET + 1);
		dest->general_bitflags |= WPF_ALTWEAPONPRESENT;
	}
}

/*
======================
G_weapPhys_ParseType
======================
Parses 'type' field.
Syntax:
'type' '=' "enumstring"
*/
qboolean G_weapPhys_ParseType( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	
	// NOTE: The sequences of the strings in these stringtables are dependant on the
	//       sequences of enumeration types in g_userweapons.h !!!
	char *Physics_strings[] = { "Missile", "Beam", "Hitscan", "Trigger", "None", "" };
	char *Detonation_strings[] = { "Ki", "" };
	char *Trajectory_strings[] = { "LineOfSight", "ProxBomb", "Guided", "Homing", "Arch", "Drunken", "Cylinder", "" };

	int i;

	if ( token->tokenSym != TOKEN_STRING ) {
		G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	switch ( category ) {

	case CAT_PHYSICS:
		for ( i = 0; Q_stricmp( Physics_strings[i], token->stringval ); i++ ) {
			if ( !strcmp( Physics_strings[i], "" )) {
				G_weapPhys_ErrorHandle( ERROR_UNKNOWN_ENUMTYPE, scanner, token->stringval, NULL );
				return qfalse;
			}
		}
		g_weapPhysBuffer.general_type = i;
		break;

	case CAT_DETONATION:
		for ( i = 0; Q_stricmp( Detonation_strings[i], token->stringval ); i++ ) {
			if ( !strcmp( Detonation_strings[i], "" )) {
				G_weapPhys_ErrorHandle( ERROR_UNKNOWN_ENUMTYPE, scanner, token->stringval, NULL );
				return qfalse;
			}
		}
		break;

	case CAT_TRAJECTORY:
		for ( i = 0; Q_stricmp( Trajectory_strings[i], token->stringval ); i++ ) {
			if ( !strcmp( Trajectory_strings[i], "" )) {
				G_weapPhys_ErrorHandle( ERROR_UNKNOWN_ENUMTYPE, scanner, token->stringval, NULL );
				return qfalse;
			}
		}
		g_weapPhysBuffer.homing_type = i;
		break;

	default:
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
		break;
	}

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
======================
G_weapPhys_ParseSpeed
======================
Parses 'speed' field.
Syntax:
'speed' '=' ( <int> | <float> )
*/
qboolean G_weapPhys_ParseSpeed( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_PHYSICS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		G_weapPhys_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.physics_speed = token->floatval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==============================
G_weapPhys_ParseAcceleration
==============================
Parses 'acceleration' field.
Syntax:
'acceleration' '=' ( <int> | <float> )
*/
qboolean G_weapPhys_ParseAcceleration( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_PHYSICS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( (token->tokenSym != TOKEN_INTEGER) && (token->tokenSym != TOKEN_FLOAT) ) {
		G_weapPhys_ErrorHandle( ERROR_FLOAT_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.physics_acceleration = token->floatval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}
qboolean G_weapPhys_ParseImpede(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_DETONATION){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.damage_impede = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
/*
========================
G_weapPhys_ParseRadius
========================
Parses 'radius' field.
Syntax:
'radius' '=' ( <int> | <float> ) ( e | ('+' ( <int> | <float> )) )
*/
qboolean G_weapPhys_ParseRadius( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( (category != CAT_PHYSICS) && (category != CAT_DETONATION) ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	switch ( category ) {
	case CAT_PHYSICS:
		g_weapPhysBuffer.physics_radius = token->intval;
		break;
	case CAT_DETONATION:
		g_weapPhysBuffer.damage_radius = token->intval;
		break;
	default:
		// shouldn't be able to happen
		break;
	}

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_PLUS ) {

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		switch ( category ) {
		case CAT_PHYSICS:
			g_weapPhysBuffer.physics_radiusMultiplier = token->intval;
			break;
		case CAT_DETONATION:
			g_weapPhysBuffer.damage_radiusMultiplier = token->intval;
			break;
		default:
			// shouldn't be able to happen
			break;
		}		

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}
	} else {
		// No + multiplier means we reset any old multiplier, to keep it from remaining
		// as a ghost.

		switch ( category ) {
		case CAT_PHYSICS:
			g_weapPhysBuffer.physics_radiusMultiplier = 0;
			break;
		case CAT_DETONATION:
			g_weapPhysBuffer.damage_radiusMultiplier = 0;
			break;
		default:
			// shouldn't be able to happen
			break;
		}	
	}

	return qtrue;
}


/*
=======================
G_weapPhys_ParseRange
=======================
Parses 'range' field.
Syntax (Physics):
  'range' '=' ( <int> | ( '[' <int> <int> ']'))
Syntax (Trajectory):
  'range' '=' <int>
*/
qboolean G_weapPhys_ParseRange( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	int rng_start, rng_end;

	if ( (category != CAT_PHYSICS) && (category != CAT_TRAJECTORY) ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	

	switch ( category ) {
	case CAT_PHYSICS:
		switch ( token->tokenSym ) {
		case TOKEN_INTEGER:
			rng_start = rng_end = token->intval;
			break;

		case TOKEN_OPENRANGE:

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_INTEGER ) {
				G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				rng_start = token->intval;
			}

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_INTEGER ) {
				G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				rng_end = token->intval;
			}

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_CLOSERANGE ) {
				G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
				return qfalse;
			}
			break;

		default:
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
			break;
		}

		g_weapPhysBuffer.physics_range_min = rng_start;
		g_weapPhysBuffer.physics_range_max = rng_end;
		break;

	case CAT_TRAJECTORY:
		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		g_weapPhysBuffer.homing_range = token->intval;
		break;

	default:
		// shouldn't be able to happen
		break;
	}
	
	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}
/*
==========================
G_weapPhys_ParseDuration
==========================
Parses 'duration' field.
Syntax:
'duration' '=' <int>
*/
qboolean G_weapPhys_ParseDuration(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if ( category != CAT_DETONATION ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}
	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}
	g_weapPhysBuffer.damage_radiusDuration = token->intval;
	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*
==========================
G_weapPhys_ParseLifetime
==========================
Parses 'lifetime' field.
Syntax:
'lifetime' '=' <int>
*/
qboolean G_weapPhys_ParseLifetime( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_PHYSICS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.physics_lifetime = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*
=========================
G_weapPhys_ParseSwat
=========================
Parses 'swat' field.
Syntax:
'swat' '=' <int>
*/
qboolean G_weapPhys_ParseSwat( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_PHYSICS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.physics_swat = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*=========================
G_weapPhys_ParseMovement
=========================*/
qboolean G_weapPhys_ParseMovement(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category != CAT_RESTRICT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym != TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.restrict_movement = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){
			G_weapPhys_ErrorHandle(ERROR_PREMATURE_EOF,scanner,NULL,NULL);
		}
		return qfalse;
	}
	return qtrue;
}

/*
=========================
G_weapPhys_ParseDrain
=========================
Parses 'drain' field.
Syntax:
'drain' '=' <int>
*/
qboolean G_weapPhys_ParseDrain( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_PHYSICS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.physics_drain = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*
=========================
G_weapPhys_ParseBlind
=========================
Parses 'blind' field.
Syntax:
'blind' '=' <int>
*/
qboolean G_weapPhys_ParseBlind( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_PHYSICS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.physics_blind = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}
/*====================
G_weapPhys_ParseMinPowerLevel
====================*/
qboolean G_weapPhys_ParseMinPowerLevel(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_minPowerLevel = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMaxPowerLevel(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_maxPowerLevel = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMinHealth(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_minHealth = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMaxHealth(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_maxHealth = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMinFatigue(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_minFatigue = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMaxFatigue(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_maxFatigue = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMinTier(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_minTier = token->intval+1;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMaxTier(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_maxTier = token->intval+1;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMinTotalTier(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_minTotalTier = token->intval+1;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMaxTotalTier(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_maxTotalTier = token->intval+1;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseGround(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_ground = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseFlight(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_flight = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseWater(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.require_water = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
/*
====================
G_weapPhys_ParsePowerLevel
====================
Parses 'powerLevel' field.
Syntax:
( 'powerLevel' ) '=' <int>
*/
qboolean G_weapPhys_ParsePowerLevel( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_COSTS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.costs_powerLevel = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}
qboolean G_weapPhys_ParseFatigue(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_COSTS){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.costs_fatigue = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseHealth(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_COSTS){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	g_weapPhysBuffer.costs_health = token->intval;
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}
qboolean G_weapPhys_ParseMaximum(g_weapPhysParser_t *parser,g_weapPhysCategoryIndex_t category,int field){
	g_weapPhysToken_t *token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;
	if(category!=CAT_COSTS && category!=CAT_REQUIREMENT){
		G_weapPhys_ErrorHandle(ERROR_FIELD_NOT_IN_CATEGORY,scanner,g_weapPhysFields[field].fieldname,g_weapPhysCategories[category]);
		return qfalse;
	}
	if(token->tokenSym!=TOKEN_INTEGER){
		G_weapPhys_ErrorHandle(ERROR_INTEGER_EXPECTED,scanner,token->stringval,NULL);
		return qfalse;
	}
	if(category==CAT_COSTS){g_weapPhysBuffer.costs_maximum = token->intval;}
	if(category==CAT_REQUIREMENT){g_weapPhysBuffer.require_maximum = token->intval;}
	if(!G_weapPhys_NextSym(scanner,token)){
		if(token->tokenSym == TOKEN_EOF){G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );}
		return qfalse;
	}
	return qtrue;
}

/*
==============================
G_weapPhys_ParseCooldownTime
==============================
Parses 'cooldownTime' field.
Syntax:
'cooldownTime' '=' <int>
*/
qboolean G_weapPhys_ParseCooldownTime( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_COSTS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.costs_cooldownTime = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
============================
G_weapPhys_ParseChargeTime
============================
Parses 'chargeTime' field.
Syntax:
'chargeTime' '=' <int>
*/
qboolean G_weapPhys_ParseChargeTime( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_COSTS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.costs_chargeTime = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
================================
G_weapPhys_ParseChargeReadyPct
================================
Parses 'chargeReadyPct' field.
Syntax:
'chargeReadyPct' '=' <int>
*/
qboolean G_weapPhys_ParseChargeReadyPct( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_COSTS ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.costs_chargeReady = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}

/*
========================
G_weapPhys_ParseDamage
========================
Parses 'damage' field.
Syntax:
'damage' '=' <int>
*/
qboolean G_weapPhys_ParseDamage( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_DETONATION ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.damage_damage = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	if ( token->tokenSym == TOKEN_PLUS ) {

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		g_weapPhysBuffer.damage_multiplier = token->intval;

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}
	} else {
		// No + multiplier means we reset any old multiplier, to keep it from remaining
		// as a ghost.

		g_weapPhysBuffer.damage_multiplier = 0;
	}

	return qtrue;
}


/*
===========================
G_weapPhys_ParseKnockBack
===========================
Parses 'knockback' field.
Syntax:
'knockback' '=' <int>
*/
qboolean G_weapPhys_ParseKnockBack( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_DETONATION ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.damage_extraKnockback = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=========================
G_weapPhys_ParseNrShots
=========================
Parses 'nrShots' field.
Syntax:
'nrShots' '=' <int>
*/
qboolean G_weapPhys_ParseNrShots( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.firing_nrShots = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=============================
G_weapPhys_ParseOffsetWidth
=============================
Parses 'offsetWidth' field.
Syntax:
'offsetWidth' '=' ( <int> | ( '[' <int> <int> ']' ) )
*/
qboolean G_weapPhys_ParseOffsetWidth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	int rng_start, rng_end;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	switch ( token->tokenSym ) {
	case TOKEN_INTEGER:
		rng_start = rng_end = token->intval;
		break;
	case TOKEN_OPENRANGE:

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_start = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_end = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_CLOSERANGE ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
			return qfalse;
		}
		break;

	default:
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.firing_offsetW_min = rng_start;
	g_weapPhysBuffer.firing_offsetW_max = rng_end;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==============================
G_weapPhys_ParseOffsetHeight
==============================
Parses 'offsetHeight' field.
Syntax:
'offsetHeight' '=' ( <int> | ( '[' <int> <int> ']' ) )
*/
qboolean G_weapPhys_ParseOffsetHeight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	int rng_start, rng_end;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	switch ( token->tokenSym ) {
	case TOKEN_INTEGER:
		rng_start = rng_end = token->intval;
		break;
	case TOKEN_OPENRANGE:

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_start = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_end = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_CLOSERANGE ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
			return qfalse;
		}
		break;

	default:
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.firing_offsetH_min = rng_start;
	g_weapPhysBuffer.firing_offsetH_max = rng_end;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
============================
G_weapPhys_ParseAngleWidth
============================
Parses 'angleWidth' field.
Syntax:
'angleWidth' '=' ( <int> | ( '[' <int> <int> ']' ) )
*/
qboolean G_weapPhys_ParseAngleWidth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	int rng_start, rng_end;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	switch ( token->tokenSym ) {
	case TOKEN_INTEGER:
		rng_start = rng_end = token->intval;
		break;
	case TOKEN_OPENRANGE:

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_start = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_end = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_CLOSERANGE ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
			return qfalse;
		}
		break;

	default:
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
		break;
	}

	g_weapPhysBuffer.firing_angleW_min = rng_start;
	g_weapPhysBuffer.firing_angleW_max = rng_end;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}


	return qtrue;
}


/*
=============================
G_weapPhys_ParseAngleHeight
=============================
Parses 'angleHeight' field.
Syntax:
'angleHeight' '=' ( <int> | ( '[' <int> <int> ']' ) )
*/
qboolean G_weapPhys_ParseAngleHeight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	int rng_start, rng_end;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	switch ( token->tokenSym ) {
	case TOKEN_INTEGER:
		rng_start = rng_end = token->intval;
		break;
		case TOKEN_OPENRANGE:

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_start = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			rng_end = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_CLOSERANGE ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "]" );
			return qfalse;
		}
		break;

	default:
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
		break;
	}

	g_weapPhysBuffer.firing_angleH_min = rng_start;
	g_weapPhysBuffer.firing_angleH_max = rng_end;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}


	return qtrue;
}


/*
=============================
G_weapPhys_ParseFlipInWidth
=============================
Parses 'flipInWidth' field.
Syntax:
'flipInWidth' '=' <int>
*/
qboolean G_weapPhys_ParseFlipInWidth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	switch ( token->tokenSym ) {
	case TOKEN_TRUE:
		g_weapPhysBuffer.firing_offsetWFlip = qtrue;
		break;
	case TOKEN_FALSE:
		g_weapPhysBuffer.firing_offsetWFlip = qfalse;
		break;
	default:
		G_weapPhys_ErrorHandle( ERROR_BOOLEAN_EXPECTED, scanner, token->stringval, NULL );
		break;
	}

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=============================
G_weapPhys_ParseFlipInWidth
=============================
Parses 'flipInHeight' field.
Syntax:
'flipInHeight' '=' <int>
*/
qboolean G_weapPhys_ParseFlipInHeight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	switch ( token->tokenSym ) {
	case TOKEN_TRUE:
		g_weapPhysBuffer.firing_offsetHFlip = qtrue;
		break;
	case TOKEN_FALSE:
		g_weapPhysBuffer.firing_offsetHFlip = qfalse;
		break;
	default:
		G_weapPhys_ErrorHandle( ERROR_BOOLEAN_EXPECTED, scanner, token->stringval, NULL );
		break;
	}

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
==============================
G_weapPhys_ParseFOV
==============================
Parses 'fieldOfView' / 'FOV' field.
Syntax:
( 'fieldOfView' | 'FOV' ) '=' <int>
*/
qboolean G_weapPhys_ParseFOV( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	g_weapPhysToken_t	*token = &parser->token;
	g_weapPhysScanner_t	*scanner = &parser->scanner;

	if ( category != CAT_MUZZLE ) {
		G_weapPhys_ErrorHandle( ERROR_FIELD_NOT_IN_CATEGORY, scanner, g_weapPhysFields[field].fieldname, g_weapPhysCategories[category] );
		return qfalse;
	}

	if ( token->tokenSym != TOKEN_INTEGER ) {
		G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
		return qfalse;
	}

	g_weapPhysBuffer.homing_FOV = token->intval;

	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym == TOKEN_EOF ) {
			G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
		}
		return qfalse;
	}

	return qtrue;
}


/*
=======================
G_weapPhys_ParseDummy
=======================
A special dummy function used in the terminator of
the g_weapPhysFields list. Never actually used, and
if it _would_ be called, it would always hault
parsing.
*/
qboolean G_weapPhys_ParseDummy( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field ) {
	return qfalse;
}




// ==================================================================================
//
//    F  I  X  E  D     F  U  N  C  T  I  O  N  S  - Do not change the below.
//
// ==================================================================================

/*
=========================
G_weapPhys_ParseImports
=========================
Parses import definitions and stores them
in a reference list.
*/
static qboolean G_weapPhys_ParseImports( g_weapPhysParser_t *parser ) {
	g_weapPhysToken_t		*token;
	g_weapPhysScanner_t		*scanner;
	char					refname[MAX_TOKENSTRING_LENGTH];
	char					filename[MAX_TOKENSTRING_LENGTH];

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   'import' "<refname>" '=' "<filename>" "<defname>"

	while ( token->tokenSym == TOKEN_IMPORT ) {

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( refname, token->stringval, sizeof(refname) );
		}
		
		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_EQUALS ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "=" );
			return qfalse;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( filename, token->stringval, sizeof(filename) );
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		}

		if ( !G_weapPhys_AddImportRef( parser, refname, filename, token->stringval ) ) {
			return qfalse;
		}

		// NOTE: Do it like this to prevent errors if a file happens to only contain
		//       import lines. While it is actually useless to have such a file, it
		//       still is syntacticly correct.
		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym != TOKEN_EOF ) {
				return qfalse;
			}
		}
	}

	return qtrue;
}


/*
========================
G_weapPhys_ParseFields
========================
Parses the fields of one category
within a weapon definition.
*/
static qboolean G_weapPhys_ParseFields( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category ) {
	g_weapPhysToken_t	*token;
	g_weapPhysScanner_t	*scanner;

	scanner = &parser->scanner;
	token = &parser->token;

	while ( token->tokenSym == TOKEN_FIELD ) {
		int field;

		field = token->identifierIndex;

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_EQUALS ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "=" );
			return qfalse;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if (!g_weapPhysFields[field].parseFunc( parser, category, field ) ) {
			return qfalse;
		}		
	}
	return qtrue;
}


/*
============================
G_weapPhys_ParseCategories
============================
Parses the categories of a weapon
definition.
*/
static qboolean G_weapPhys_ParseCategories( g_weapPhysParser_t *parser ) {
	g_weapPhysToken_t		*token;
	g_weapPhysScanner_t		*scanner;
	
	int		currentCategory;

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   <categoryname> '{' <HANDLE FIELDS> '}'

	while ( token->tokenSym == TOKEN_CATEGORY ) {
		currentCategory = token->identifierIndex;

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_OPENBLOCK ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
			return qfalse;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		// Handles fields
		if (!G_weapPhys_ParseFields( parser, currentCategory ) ) {
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_CLOSEBLOCK ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "}" );
			return qfalse;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}
	}

	if ( token->tokenSym != TOKEN_CLOSEBLOCK ) {
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "}" );
		return qfalse;
	}

	if ( g_verboseParse.integer ) {
		G_Printf("Processed categories succesfully.\n");
	}

	return qtrue;
}


/*
================================
G_weapPhys_PreParseDefinitions
================================
Pre-parses the definitions of the weapons in the
scriptfile. The contents of a definition block
are checked for validity by the lexical scanner
and parser to make sure syntax is correct.
The parsed values however, are not yet stored,
because we don't have a link to an actual
weapon yet. Entry points into the definitions
are cached into a table for a quick jump once
we find out the links in G_weapPhys_ParseLinks.
*/
static qboolean G_weapPhys_PreParseDefinitions( g_weapPhysParser_t *parser ) {
	g_weapPhysToken_t		*token;
	g_weapPhysScanner_t		*scanner;

	int						defline;
	char					*defpos;
	g_weapPhysAccessLvls_t	accessLvl;
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

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( refname, token->stringval, sizeof(refname) );
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}
		
		// Are we deriving?
		if ( token->tokenSym == TOKEN_EQUALS ) {
			hasSuper = qtrue;

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_STRING ) {
				G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			}

			if ( !G_weapPhys_FindDefinitionRef( parser, token->stringval ) ) {
				return qfalse;
			} else {
				Q_strncpyz( supername, token->stringval, sizeof(supername) );
			}

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				}
				return qfalse;
			}
		}

		if ( token->tokenSym != TOKEN_OPENBLOCK ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "{" );
			return qfalse;
		} else {
			blockCount = 1;
			defline = scanner->line;
			defpos = scanner->pos;
			if (!G_weapPhys_AddDefinitionRef( parser, refname, defpos, defline, accessLvl, hasSuper, supername ) ) {
				return qfalse;
			}
		}

		while ( blockCount > 0 ) {

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym == TOKEN_EOF ) {
					G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
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
		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym != TOKEN_EOF ) {
				return qfalse;
			}
		}
	}

	return qtrue;
}


/*
=======================
G_weapPhys_ParseLinks
=======================
Parses weapon links to definitions and continues to
parse the actual weapon definitions.
*/
static qboolean G_weapPhys_ParseLinks( g_weapPhysParser_t *parser ) {
	g_weapPhysToken_t		*token;
	g_weapPhysScanner_t		*scanner;

	int						weaponNum;
	char					pri_refname[MAX_TOKENSTRING_LENGTH];
	char					sec_refname[MAX_TOKENSTRING_LENGTH];

	scanner = &parser->scanner;
	token = &parser->token;

	// Syntax:
	//   'weapon' <int> '=' "<refname>" ( e | '|' "<refname>" )

	while ( token->tokenSym == TOKEN_WEAPON ) {
		
		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_INTEGER ) {
			G_weapPhys_ErrorHandle( ERROR_INTEGER_EXPECTED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			weaponNum = token->intval;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_EQUALS ) {
			G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, "=" );
			return qfalse;
		}

		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		if ( token->tokenSym != TOKEN_STRING ) {
			G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner,  token->stringval, NULL );
			return qfalse;
		}

		if ( !G_weapPhys_FindDefinitionRef( parser, token->stringval ) ) {
			G_weapPhys_ErrorHandle( ERROR_DEFINITION_UNDEFINED, scanner, token->stringval, NULL );
			return qfalse;
		} else {
			Q_strncpyz( pri_refname, token->stringval, sizeof(pri_refname) );
		}

		// NOTE: This makes sure we don't get an error if the last link in the file contains
		//       no secondary definition, but instead terminates after the primary one.
		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym != TOKEN_EOF ) {
				return qfalse;
			}
		}

		if ( token->tokenSym == TOKEN_COLON ) {

			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				return qfalse;
			}

			if ( token->tokenSym != TOKEN_STRING ) {
				G_weapPhys_ErrorHandle( ERROR_STRING_EXPECTED, scanner, token->stringval, NULL );
				return qfalse;
			}

			if ( !G_weapPhys_FindDefinitionRef( parser, token->stringval ) ) {
				G_weapPhys_ErrorHandle( ERROR_DEFINITION_UNDEFINED, scanner, token->stringval, NULL );
				return qfalse;
			} else {
				Q_strncpyz( sec_refname, token->stringval, sizeof(sec_refname) );
			}

			// NOTE: This makes sure we don't get an error if this is the last link in the file.
			if ( !G_weapPhys_NextSym( scanner, token ) ) {
				if ( token->tokenSym != TOKEN_EOF ) {
					return qfalse;
				}
			}

		} else {
			Q_strncpyz( sec_refname, "", sizeof(sec_refname) );
		}

		if ( !G_weapPhys_AddLinkRef( parser, weaponNum, pri_refname, sec_refname ) ) {
			return qfalse;
		}
	}

	return qtrue;
}



/*
===================================
G_weapPhys_IncreaseRecursionDepth
===================================
Increases inheritance recursion depth.
Checks if the maximum level is exceeded and returns
an error if so.
*/
static qboolean G_weapPhys_IncreaseRecursionDepth( void ) {
	if ( g_weapPhysRecursionDepth == MAX_RECURSION_DEPTH ) {
		return qfalse;
	}

	g_weapPhysRecursionDepth++;
	return qtrue;
}


/*
===================================
G_weapPhys_DecreaseRecursionDepth
===================================
Decreases inheritance recursion depth.
*/
static void G_weapPhys_DecreaseRecursionDepth( void ) {
	g_weapPhysRecursionDepth--;
}

// Need this prototyped
qboolean G_weapPhys_ParseDefinition( g_weapPhysParser_t *parser, char* refname, g_weapPhysAccessLvls_t *accessLvl );

/*
==================================
G_weapPhys_ParseRemoteDefinition
==================================
Instantiates a new parser and scanner
to parse a remote definition
*/
qboolean G_weapPhys_ParseRemoteDefinition( char *filename, char *refname ) {
	g_weapPhysParser_t		parser;
	g_weapPhysScanner_t		*scanner;
	g_weapPhysToken_t		*token;
	int						i;

	// Initialize the parser
	memset( &parser, 0, sizeof(parser) );
	scanner = &parser.scanner;
	token = &parser.token;	

	// Initialize the scanner by loading the file
	G_weapPhys_LoadFile( scanner, filename );

	// Get the very first token initialized. If
	// it is an end of file token, we will not parse
	// the empty file but will instead exit with true.
	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym != TOKEN_EOF ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	// Parse the imports
	if ( !G_weapPhys_ParseImports( &parser ) ) {
		return qfalse;
	}

	// Pre-Parse the definitions
	if ( !G_weapPhys_PreParseDefinitions( &parser ) ) {
		return qfalse;
	}

	// Parse the link to the weapons
	// NOTE: We don't really need to do this, but it does
	//       ensure file structure.
	if ( !G_weapPhys_ParseLinks( &parser ) ) {
		return qfalse;
	}

	// Respond with an error if something is trailing the
	// link definitions.
	// NOTE: We don't really need to do this, but it does
	//       ensure file structure.
	if ( token->tokenSym != TOKEN_EOF ) {
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	// If we're dealing with a local definition in this file, then that definition
	// MUST be public, since we're importing it to another file.
	i = G_weapPhys_FindDefinitionRef( &parser, refname ) - 1;
	if ( i < MAX_DEFINES ) {
		if ( parser.definitionRef[i].accessLvl != LVL_PUBLIC ) {
			scanner->line = parser.definitionRef[i].scannerLine; // <-- Make sure the error reports the correct line
			G_weapPhys_ErrorHandle( ERROR_IMPORTING_NON_PUBLIC, scanner, parser.definitionRef[i].refname, NULL );
			return qfalse;
		}
	}

	if ( !G_weapPhys_ParseDefinition( &parser, refname, NULL ) ) {
		return qfalse;
	}
	
	return qtrue;
}


/*
============================
G_weapPhys_ParseDefinition
============================
Parses a definition, taking inheritance into account.
*/
qboolean G_weapPhys_ParseDefinition( g_weapPhysParser_t *parser, char* refname, g_weapPhysAccessLvls_t *accessLvl ) {
	int i;
	g_weapPhysScanner_t	*scanner;
	g_weapPhysToken_t	*token;
	g_weapPhysAccessLvls_t lastAccessLvl;

	lastAccessLvl = LVL_PUBLIC; // <-- Incase there IS no last access level from a super class
	scanner = &parser->scanner;
	token = &parser->token;

	i = G_weapPhys_FindDefinitionRef( parser, refname ) - 1; // <-- Must subtract one to get proper index!
	if ( i < MAX_DEFINES ) {
		// local declaration
		if ( parser->definitionRef[i].hasSuper ) {
			
			if (!G_weapPhys_IncreaseRecursionDepth() ) {
				G_weapPhys_ErrorHandle( ERROR_MAX_RECURSION, scanner, NULL, NULL );
				return qfalse;
			}

			if ( g_verboseParse.integer ) {
				G_Printf("Inheriting superclass '%s'\n", parser->definitionRef[i].supername );
			}
			if ( !G_weapPhys_ParseDefinition( parser, parser->definitionRef[i].supername, &lastAccessLvl ) ) {
				return qfalse;
			}
			
			G_weapPhys_DecreaseRecursionDepth();
		}

		// Check if the super class was private, instead of public or protected
		if ( lastAccessLvl == LVL_PRIVATE ) {
			G_weapPhys_ErrorHandle( ERROR_INHERITING_PRIVATE, scanner, parser->definitionRef[i].supername, NULL );
			return qfalse;
		}

		// Reposition the lexical scanner
		scanner->pos = parser->definitionRef[i].scannerPos;
		scanner->line = parser->definitionRef[i].scannerLine;

		// Check if we're not breaking access level hierarchy
		if ( accessLvl ) {
			*accessLvl = parser->definitionRef[i].accessLvl;
			if ( *accessLvl < lastAccessLvl ) {
				G_weapPhys_ErrorHandle( ERROR_OVERRIDING_WITH_HIGHER_ACCESS, scanner, parser->definitionRef[i].refname, NULL );
				return qfalse;
			}
		}

		// Skip the '{' opening brace of the definition block, and align to the first real
		// symbol in the block.
		if ( !G_weapPhys_NextSym( scanner, token ) ) {
			if ( token->tokenSym == TOKEN_EOF ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
			}
			return qfalse;
		}

		// Parse the block's categories.
		if ( !G_weapPhys_ParseCategories( parser ) ) {
			return qfalse;
		}

	} else {
		// imported declaration
		
		// First subtract the offset we added to detect difference between
		// an imported and a local definition.
		i -= MAX_DEFINES;

		if (!G_weapPhys_IncreaseRecursionDepth() ) {
				G_weapPhys_ErrorHandle( ERROR_MAX_RECURSION, scanner, NULL, NULL );
				return qfalse;
		}

		if ( g_verboseParse.integer ) {
			G_Printf("Importing '%s'\n", refname );
		}		

		if ( !G_weapPhys_ParseRemoteDefinition( parser->importRef[i].filename, parser->importRef[i].defname ) ) {
			return qfalse;
		}

		G_weapPhys_DecreaseRecursionDepth();
	}

	return qtrue;

}

/*
==================
G_weapPhys_Parse
==================
Main parsing function for a scriptfile.
This is the parser's 'entrypoint',
*/
qboolean G_weapPhys_Parse( char *filename, int clientNum ) {
	g_weapPhysParser_t		parser;
	g_weapPhysScanner_t		*scanner;
	g_weapPhysToken_t		*token;
	int						i;
	int						*weaponMask;

	// Initialize the parser
	memset( &parser, 0, sizeof(parser) );
	scanner = &parser.scanner;
	token = &parser.token;
	g_weapPhysRecursionDepth = 0;

	// Clear the weapons here, so we are never stuck with 'ghost' weapons
	// if an error occurs in the parse.
	weaponMask = G_FindUserWeaponMask( clientNum );
	*weaponMask = 0;
	
	// Initialize the scanner by loading the file
	G_weapPhys_LoadFile( scanner, filename );

	// Get the very first token initialized. If
	// it is an end of file token, we will not parse
	// the empty file but will instead exit with true.
	if ( !G_weapPhys_NextSym( scanner, token ) ) {
		if ( token->tokenSym != TOKEN_EOF ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	// Parse the imports
	if ( !G_weapPhys_ParseImports( &parser ) ) {
		return qfalse;
	}

	// Pre-Parse the definitions
	if ( !G_weapPhys_PreParseDefinitions( &parser ) ) {
		return qfalse;
	}

	// Parse the link to the weapons
	if ( !G_weapPhys_ParseLinks( &parser ) ) {
		return qfalse;
	}

	// Respond with an error if something is trailing the
	// link definitions.
	if ( token->tokenSym != TOKEN_EOF ) {
		G_weapPhys_ErrorHandle( ERROR_UNEXPECTED_SYMBOL, scanner, token->stringval, NULL );
		return qfalse;
	}

	// Work through the link table, parsing and assigning the definitions
	// as we go.
	for ( i = 0; i < MAX_LINKS; i++ ) {

		if ( !parser.linkRef[i].active ) {
			continue;
		}

		// Empty the buffer.
		memset( &g_weapPhysBuffer, 0, sizeof(g_weapPhysBuffer) );

		if ( g_verboseParse.integer ) {
			G_Printf( "Processing weapon nr %i, primary '%s'.\n", i+1, parser.linkRef[i].pri_refname );
		}

		if ( !G_weapPhys_ParseDefinition( &parser, parser.linkRef[i].pri_refname, NULL ) ) {
			return qfalse;
		}

		G_weapPhys_StoreBuffer( clientNum, i );

		// Empty the buffer.
		memset( &g_weapPhysBuffer, 0, sizeof(g_weapPhysBuffer) );

		if ( strcmp( parser.linkRef[i].sec_refname, "" ) ) {
			if ( g_verboseParse.integer ) {
				G_Printf("Processing weapon nr %i, secondary '%s'.\n", i+1, parser.linkRef[i].sec_refname );
			}
			if ( !G_weapPhys_ParseDefinition( &parser, parser.linkRef[i].sec_refname, NULL ) ) {
				return qfalse;
			}

			// Piggyback the alternate fire present flag in the buffer of the alternate fire
			// to let G_weapPhys_StoreBuffer enable this flag on the primary fire.
			g_weapPhysBuffer.general_bitflags |= WPF_ALTWEAPONPRESENT;
		}

		G_weapPhys_StoreBuffer( clientNum, i + ALTWEAPON_OFFSET );



		// If everything went okay, we add the weapon to the availability mask
		*weaponMask |= ( 1 << (i+1) );

		if ( g_verboseParse.integer ) {
			G_Printf("Added weapon nr %i to availabilty mask. New mask reads: %i\n", i+1, *weaponMask );
		}
		
	}

	if ( g_verboseParse.integer ) {
		G_Printf("Parse completed succesfully.\n");
	}

	return qtrue;
}
