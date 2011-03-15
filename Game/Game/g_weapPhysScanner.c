// Copyright (C) 2003-2005 RiO
//
// g_weapPhysScanner.c -- lexical scanner for ZEQ2's weapon physics script language.

#include "g_weapPhysParser.h" // <-- cg_local.h included in this

char *g_weapPhysCategories[] = {
	"Physics", "Costs", "Detonation", "Muzzle", "Trajectory", "Requirement", "Restrictions", ""
};

g_weapPhysField_t g_weapPhysFields[] = {
	{ "type",				G_weapPhys_ParseType			},	// Physics, Detonation, Trajectory
	{ "speed",				G_weapPhys_ParseSpeed			},	// Physics
	{ "acceleration",		G_weapPhys_ParseAcceleration	},	// Physics
	{ "radius",				G_weapPhys_ParseRadius			},	// Physics, Detonation
	{ "impede",				G_weapPhys_ParseImpede			},	// Detonation
	{ "range",				G_weapPhys_ParseRange			},	// Physics, Trajectory
	{ "duration",			G_weapPhys_ParseDuration		},	// Physics, Detonation
	{ "lifetime",			G_weapPhys_ParseLifetime		},	// Physics
	{ "swat",				G_weapPhys_ParseSwat			},	// Physics
	{ "deflect",			G_weapPhys_ParseSwat			},	// Physics
	{ "drain",				G_weapPhys_ParseDrain			},	// Physics
	{ "blind",				G_weapPhys_ParseBlind			},	// Physics
	{ "movement",			G_weapPhys_ParseMovement		},	// Restriction
	{ "minPowerLevel",		G_weapPhys_ParseMinPowerLevel	},	// Requirement
	{ "maxPowerLevel",		G_weapPhys_ParseMinPowerLevel	},	// Requirement
	{ "minFatigue",			G_weapPhys_ParseMinFatigue		},	// Requirement
	{ "maxFatigue",			G_weapPhys_ParseMaxFatigue		},	// Requirement
	{ "minHealth",			G_weapPhys_ParseMinHealth		},	// Requirement
	{ "maxHealth",			G_weapPhys_ParseMaxHealth		},	// Requirement
	{ "minTier",			G_weapPhys_ParseMinTier			},	// Requirement
	{ "maxTier",			G_weapPhys_ParseMaxTier			},	// Requirement
	{ "minTotalTier",		G_weapPhys_ParseMinTotalTier	},	// Requirement
	{ "maxTotalTier",		G_weapPhys_ParseMaxTotalTier	},	// Requirement
	{ "ground",				G_weapPhys_ParseGround			},	// Requirement
	{ "flight",				G_weapPhys_ParseFlight			},	// Requirement
	{ "water",				G_weapPhys_ParseWater			},	// Requirement
	{ "powerLevel",			G_weapPhys_ParsePowerLevel		},	// Costs
	{ "maximum",			G_weapPhys_ParseMaximum			},	// Costs
	{ "health",				G_weapPhys_ParseHealth			},	// Costs
	{ "fatigue",			G_weapPhys_ParseFatigue			},	// Costs
	{ "cooldownTime",		G_weapPhys_ParseCooldownTime	},	// Costs
	{ "chargeTime",			G_weapPhys_ParseChargeTime		},	// Costs
	{ "chargeReadyPct",		G_weapPhys_ParseChargeReadyPct	},	// Costs
	{ "damage",				G_weapPhys_ParseDamage			},	// Detonation
	{ "knockback",			G_weapPhys_ParseKnockBack		},	// Detonation
	{ "nrShots",			G_weapPhys_ParseNrShots			},	// Muzzle
	{ "offsetWidth",		G_weapPhys_ParseOffsetWidth		},	// Muzzle
	{ "offsetHeight",		G_weapPhys_ParseOffsetHeight	},	// Muzzle
	{ "angleWidth",			G_weapPhys_ParseAngleWidth		},	// Muzzle
	{ "angleHeight",		G_weapPhys_ParseAngleHeight		},	// Muzzle
	{ "flipInWidth",		G_weapPhys_ParseFlipInWidth		},	// Muzzle
	{ "flipInHeight",		G_weapPhys_ParseFlipInHeight	},	// Muzzle
	{ "fieldOfView",		G_weapPhys_ParseFOV				},	// Trajectory  --NOTE: Use for homing angles!
	{ "FOV",				G_weapPhys_ParseFOV				},	// Trajectory  --NOTE: Use for homing angles!
	{ "",					G_weapPhys_ParseDummy			}	// Terminator dummy function
};


/*
========================
G_weapPhys_ErrorHandle
========================
Sends feedback on script errors to the console.
*/
void G_weapPhys_ErrorHandle( g_weapPhysError_t errorNr, g_weapPhysScanner_t *scanner, char *string1, char *string2 ) {
	char	*file;
	int		line;

	file = scanner->filename;
	line = scanner->line + 1; // <-- Internally we start from 0, for the user we start from 1

	switch ( errorNr ) {
	case ERROR_FILE_NOTFOUND:
		G_Printf( "%s: File not found.\n", file );
		break;

	case ERROR_FILE_TOOBIG:
		G_Printf( "%s: File exceeds maximum script length.\n", file );
		break;

	case ERROR_PREMATURE_EOF:
		G_Printf( "%s(%i): Premature end of file.\n", file, line );
		break;

	case ERROR_STRING_TOOBIG:
		G_Printf( "%s(%i): String exceeds limit of %i characters.\n", file, line, MAX_TOKENSTRING_LENGTH );
		break;

	case ERROR_TOKEN_TOOBIG:
		G_Printf( "%s(%i): Symbol exceeds limit of %i characters.\n", file, line, MAX_TOKENSTRING_LENGTH );
		break;

	case ERROR_UNKNOWN_SYMBOL:
		G_Printf( "%s(%i): Unknown symbol '%s' encountered.\n", file, line, string1 );
		break;

	case ERROR_UNEXPECTED_SYMBOL:
		if ( !string2 ) {
			G_Printf( "%s(%i): Unexpected symbol '%s' found.\n", file, line, string1 );
		} else {
			G_Printf( "%s(%i): Unexpected symbol '%s' found, expected '%s'.\n", file, line, string1, string2 );
		}
		break;

	case ERROR_STRING_EXPECTED:
		G_Printf( "%s(%i): String expected. '%s' is not a string or is missing quotes.\n", file, line, string1 );
		break;

	case ERROR_INTEGER_EXPECTED:
		G_Printf( "%s(%i): Integer expected, but '%s' found.\n", file, line, string1 );
		break;

	case ERROR_FLOAT_EXPECTED:
		G_Printf( "%s(%i): Float or integer expected, but '%s' found.\n", file, line, string1 );
		break;

	case ERROR_BOOLEAN_EXPECTED:
		G_Printf( "%s(%i): Boolean expected, but '%s' found.\n", file, line, string1 );
		break;

	case ERROR_IMPORTS_EXCEEDED:
		G_Printf( "%s(%i): Trying to exceed maximum number of %i imports.\n", file, line, MAX_IMPORTS );
		break;

	case ERROR_IMPORT_REDEFINED:
		G_Printf( "%s(%i): Trying to redefine previously defined import definition '%s'.\n", file, line, string1 );
		break;

	case ERROR_IMPORT_DOUBLED:
		G_Printf( "%s(%i): Trying to duplicate a previously imported definition under the new name '%s'.\n", file, line, string1 );
		break;

	case ERROR_IMPORT_UNDEFINED:
		G_Printf( "%s(%i): Undefined import '%s' being referenced.\n", file, line, string1 );
		break;

	case ERROR_DEFINITIONS_EXCEEDED:
		G_Printf( "%s(%i): Trying to exceed maximum number of %i weapon definitions.\n", file, line, MAX_DEFINES );
		break;

	case ERROR_DEFINITION_REDEFINED:
		G_Printf( "%s(%i): Trying to redefine previously defined weapon definition '%s'.\n", file, line, string1 );
		break;

	case ERROR_DEFINITION_UNDEFINED:
		G_Printf( "%s(%i): Undefined weapon definition '%s' being referenced.\n", file, line, string1 );
		break;

	case ERROR_REDEFINE_IMPORT_AS_DEFINITION:
		G_Printf( "%s(%i): Trying to redefine previously defined import definition '%s' as a local weapon definition.\n", file, line, string1 );
		break;

	case ERROR_LINK_BOUNDS:
		G_Printf( "%s(%i): Weapon link out of bounds. Must be in range [1..6].\n", file, line );
		break;

	case ERROR_LINK_REDEFINED:
		G_Printf( "%s(%i): Trying to redefine a previously defined weapon link number.\n", file, line );
		break;

	case ERROR_FIELD_NOT_IN_CATEGORY:
		G_Printf( "%s(%i): Field '%s' is not supported by category '%s'.\n", file, line, string1, string2 );
		break;

	case ERROR_INVERTED_RANGE:
		G_Printf( "%s(%i): This range doesn't allow end value '%s' to be larger than start value '%s'.\n", file, line, string1, string2 );
		break;

	case ERROR_OVER_MAXBOUND:
		G_Printf( "%s(%i): Value '%s' is larger than maximum bound of %s.\n", file, line, string1, string2 );
		break;

	case ERROR_UNDER_MINBOUND:
		G_Printf( "%s(%i): Value '%s' is smaller than minimum bound of %s.\n", file, line, string1, string2 );
		break;

	case ERROR_OVER_MAXVECTORELEMS:
		G_Printf( "%s(%i): Element '%s' exceeds maximum storage capacity of %s elements in vector.\n", file, line, string1, string2 );
		break;

	case ERROR_UNKNOWN_ENUMTYPE:
		G_Printf( "%s(%i): Identifier '%s' is not a valid option in this enumeration type.", file, line, string1 );
		break;

	case ERROR_MAX_RECURSION:
		G_Printf( "%s: Compiler is trying to read beyond maximum recursion depth %i for overriding.\n", file, MAX_RECURSION_DEPTH );
		break;

	case ERROR_INHERITING_PRIVATE:
		G_Printf( "%s: Private definition '%s' can not be inherited.\n", file, string1 );
		break;

	case ERROR_IMPORTING_NON_PUBLIC:
		G_Printf( "%s: Non-public definition '%s' can not be imported for local use.\n", file, string1 );
		break;

	case ERROR_OVERRIDING_WITH_HIGHER_ACCESS:
		G_Printf( "%s: Definition '%s' may not be declared with higher access than its superclass.\n", file, string1 );
		break;

	default:
		G_Printf( "WEAPONSCRIPT ERROR: Unknown error occured.\n" );
		break;
	}
}


/*
====================
G_weapPhys_NextSym
====================
Scans the next symbol in the scanner's
loaded scriptfile.
*/
qboolean G_weapPhys_NextSym( g_weapPhysScanner_t *scanner, g_weapPhysToken_t *token ) {
	
	// Skip non-relevant characters
	while (1) {

		// Skip leading whitespace
		while( (*scanner->pos) <= ' ') {
			if( !*scanner->pos ) {
				break;
			}
			if( *scanner->pos == '\n' ) {
				scanner->line++;
			}
			scanner->pos++;
		}
		
		// Skip double slash comments
		if ( scanner->pos[0] == '/' && scanner->pos[1] == '/' ) {

			scanner->pos += 2;
			while (*scanner->pos && *scanner->pos != '\n') {
				scanner->pos++;
			}
		}

		// Skip /* */ comments
		else if ( scanner->pos[0] == '/' && scanner->pos[1] == '*' ) {

			scanner->pos += 2;

			while ( *scanner->pos && ( scanner->pos[0] != '*' || scanner->pos[1] != '/' ) ) {
				scanner->pos++;
			}

			if ( *scanner->pos ) {
				scanner->pos += 2;
			}

		} else {

			break;
		}
	}


	// Handle quoted strings
	if ( *scanner->pos == '\"') {
		int len;

		len = 0;
		scanner->pos++;
		while (1) {

			if ( !(*scanner->pos) ) {
				G_weapPhys_ErrorHandle( ERROR_PREMATURE_EOF, scanner, NULL, NULL );
				return qfalse;
			}

			if ( *scanner->pos == '\n' ) {
				scanner->line++;
			}

			if ( *scanner->pos == '\"' ) {
				scanner->pos++;
				break;
			}

			if ( (len < (MAX_TOKENSTRING_LENGTH-1)) && ( *scanner->pos >= ' ' ) ) {
				token->stringval[len] = *scanner->pos;
				len++;
			} else {
				G_weapPhys_ErrorHandle( ERROR_STRING_TOOBIG, scanner, NULL, NULL );
				return qfalse;
			}

			scanner->pos++;
		}

		token->stringval[len] = '\0';
		token->tokenSym = TOKEN_STRING;
		return qtrue;
	}


	// Handle numbers (including support for negative ones)
	if ( (( *scanner->pos >= '0' ) && ( *scanner->pos <= '9' )) || (*scanner->pos == '-') ) {
		int len;
		qboolean dot;

		len = 0;
		dot = qfalse;

		do {

			if (len < (MAX_TOKENSTRING_LENGTH-1)) {
				token->stringval[len] = *scanner->pos;
				len++;
			} else {
				G_weapPhys_ErrorHandle( ERROR_TOKEN_TOOBIG, scanner, NULL, NULL );
				return qfalse;
			}

			scanner->pos++;
			if ( ( *scanner->pos == '.' ) && !dot ) {
				dot = qtrue;

				if ( len < (MAX_TOKENSTRING_LENGTH-1)) {
					token->stringval[len] = *scanner->pos;
					len++;
				} else {
					G_weapPhys_ErrorHandle( ERROR_TOKEN_TOOBIG, scanner, NULL, NULL );
					return qfalse;
				}

				scanner->pos++;
			}

		} while ( ( *scanner->pos >= '0' ) && ( *scanner->pos <= '9' ) );

		if ( dot ) {
			token->tokenSym = TOKEN_FLOAT;
		} else {
			token->tokenSym = TOKEN_INTEGER;
		}

		token->stringval[len] = '\0';
		token->floatval = atof( token->stringval );
		token->intval = ceil( token->floatval );

		return qtrue;
	}


	// Handle operators
	switch ( *scanner->pos ) {
	case '=':
		scanner->pos++;
		token->stringval[0] = '=';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_EQUALS;
		return qtrue;
		break;

	case '+':
		scanner->pos++;
		token->stringval[0] = '+';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_PLUS;
		return qtrue;
		break;

	case '|':
		scanner->pos++;
		token->stringval[0] = '|';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_COLON;
		return qtrue;
		break;

	case '{':
		scanner->pos++;
		token->stringval[0] = '{';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_OPENBLOCK;
		return qtrue;
		break;

	case '}':
		scanner->pos++;
		token->stringval[0] = '}';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_CLOSEBLOCK;
		return qtrue;
		break;

	case '(':
		scanner->pos++;
		token->stringval[0] = '(';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_OPENVECTOR;
		return qtrue;
		break;

	case ')':
		scanner->pos++;
		token->stringval[0] = ')';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_CLOSEVECTOR;
		return qtrue;
		break;

	case '[':
		scanner->pos++;
		token->stringval[0] = '[';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_OPENRANGE;
		return qtrue;
		break;

	case ']':
		scanner->pos++;
		token->stringval[0] = ']';
		token->stringval[1] = '\0';
		token->tokenSym = TOKEN_CLOSERANGE;
		return qtrue;
		break;

	default:
		break;
	}


	// Handle keywords
	{
		int len;
		int i;

		len = 0;
		while ( ((*scanner->pos >= 'a') && (*scanner->pos <= 'z')) ||
				((*scanner->pos >= 'A') && (*scanner->pos <= 'Z')) ) { // <-- Make sure we only have words
			if ( len < MAX_TOKENSTRING_LENGTH ) {
				token->stringval[len] = *scanner->pos;
				len++;
			} else {
				G_weapPhys_ErrorHandle( ERROR_TOKEN_TOOBIG, scanner, NULL, NULL );
				return qfalse;
			}

			scanner->pos++;
		}

		token->stringval[len] = '\0';

		// Check if the keyword is a category.
		for ( i = 0; strcmp( g_weapPhysCategories[i], "" ); i++ ) {
			if ( !Q_stricmp( g_weapPhysCategories[i], token->stringval ) ) {

				token->identifierIndex = i;
				token->tokenSym = TOKEN_CATEGORY;
				return qtrue;
			}
		}
		
		// Check if the keyword is a field.
		for ( i = 0; strcmp( g_weapPhysFields[i].fieldname, "" ); i++ ) {
			// NOTE:
			// Need to do this comparison case independantly. Q_stricmp is case
			// independant, so use this instead of standard library function strcmp.
			if ( !Q_stricmp( g_weapPhysFields[i].fieldname, token->stringval ) ) {

				token->identifierIndex = i;
				token->tokenSym = TOKEN_FIELD;
				return qtrue;
			}
		}

		// Check if the keyword is 'import', 'private', 'protected', 'public',
		// 'weapon', 'true', 'false', or 'null'.
		if ( !Q_stricmp( token->stringval, "import" ) ) {
			token->tokenSym = TOKEN_IMPORT;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "private" ) ) {
			token->tokenSym = TOKEN_PRIVATE;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "protected" ) ) {
			token->tokenSym = TOKEN_PROTECTED;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "public" ) ) {
			token->tokenSym = TOKEN_PUBLIC;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "weapon" ) ) {
			token->tokenSym = TOKEN_WEAPON;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "true" ) ) {
			token->tokenSym = TOKEN_TRUE;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "false" ) ) {
			token->tokenSym = TOKEN_FALSE;
			return qtrue;
		}

		if ( !Q_stricmp( token->stringval, "null" ) ) {
			token->tokenSym = TOKEN_NULL;
			return qtrue;
		}
	}

	// Handle end of file
	if ( *scanner->pos == '\0' ) {
		token->tokenSym = TOKEN_EOF;
		return qfalse;
	}

	// Return an error, because the scanned symbol is invalid
	G_weapPhys_ErrorHandle( ERROR_UNKNOWN_SYMBOL, scanner, token->stringval, NULL );
	return qfalse;
}


/*
=====================
G_weapPhys_LoadFile
=====================
Loads a new scriptfile into a scanner's memory.
*/
qboolean G_weapPhys_LoadFile( g_weapPhysScanner_t *scanner, char *filename ) {
	int			len;
	qhandle_t	file;

	Q_strncpyz( scanner->filename, filename, sizeof(scanner->filename) );

	// Grab filehandle
	len = trap_FS_FOpenFile( filename, &file, FS_READ );

	// File must exist, else report error
	if ( !file ) {
		G_weapPhys_ErrorHandle( ERROR_FILE_NOTFOUND, scanner, filename, NULL );
		return qfalse;
	}

	// File must not be too big, else report error
	if ( len >= ( sizeof(char) * MAX_SCRIPT_LENGTH - 1 ) ) {
		G_weapPhys_ErrorHandle( ERROR_FILE_TOOBIG, scanner, NULL, NULL );
		trap_FS_FCloseFile( file );
		return qfalse;
	}

	// Read file
	trap_FS_Read( scanner->script, len, file );
	trap_FS_FCloseFile( file );

	// Ensure null termination
	scanner->script[len] = '\0';

	// Set starting position
	scanner->line = 0;
	scanner->pos = scanner->script;

		
	// Handle some verbose output
	if ( g_verboseParse.integer ) {
		G_Printf( "Scriptfile '%s' has been read into memory.\n", filename );
	}

	return qtrue;
}
