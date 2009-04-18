// Copyright (C) 2003-2004 RiO
//
// cg_weapGfxParser.c -- parser for weapon graphics script language.

#include "g_local.h"

#define MAX_SCRIPT_LENGTH		8000
#define MAX_SCRIPTITEM_LENGTH	  50
#define MAX_IMPORT_DEPTH		   5

typedef enum {
	ERROR_FILE_NOTFOUND,
	ERROR_FILE_TOOBIG,
	ERROR_PREMATURE_EOF,
	ERROR_SYMBOL_UNEXPECTED,
	ERROR_RESERVED_KEYWORD,
	ERROR_SECOND_DOT,
	ERROR_NO_ATTACKIDENT,
	ERROR_IMPORTSYMBOL_UNKNOWN,
	ERROR_ATTACKSYMBOL_UNKNOWN,
	ERROR_IMPORT_FAILED,
	ERROR_MAXIMPORT_EXCEEDED,
	ERROR_NONVALID_INHERITANCE,
	ERROR_NOT_SUBSECTION,
	ERROR_NOT_DAMAGETYPE,
	ERROR_NOT_HOMINGTYPE,
	ERROR_UNKNOWNPROPERTY
} weapPhysError_t;

static int	CG_weapGfxParser_importDepth;

/*
   ---------------------------------------------------
     E R R O R   H A N D L I N G   F U N C T I O N S
   ---------------------------------------------------
*/

/*
==============================
CG_weapGfxParser_ErrorHandle
==============================
Sends feedback on script errors to the console.
*/
void CG_weapGfxParser_ErrorHandle( weapPhysError_t errorNr, char *string1, char *string2 ) {
	switch ( errorNr ) {
	case ERROR_FILE_NOTFOUND:
		CG_Printf( "WEAPONSCRIPT ERROR: File '%s' not found.\n", string1 );
		break;

	case ERROR_FILE_TOOBIG:
		CG_Printf( "WEAPONSCRIPT ERROR: File '%s' exceeds maximum script length.\n", string1 );
		break;

	case ERROR_PREMATURE_EOF:
		CG_Printf( "WEAPONSCRIPT ERROR: Premature end of file.\n" );
		break;

	case ERROR_SYMBOL_UNEXPECTED:
		if ( !string2 ) {
			CG_Printf( "WEAPONSCRIPT ERROR: Unexpected symbol '%s' found.\n", string1 );
		} else {
			CG_Printf( "WEAPONSCRIPT ERROR: Unexpected symbol '%s' found, expected '%s'.\n", string1, string2 );
		}
		break;

	case ERROR_RESERVED_KEYWORD:
		CG_Printf( "WEAPONSCRIPT ERROR: Illegal use of reserved keyword '%s'.\n", string1 );
		break;

	case ERROR_SECOND_DOT:
		CG_Printf( "WEAPONSCRIPT ERROR: Trying to prepend second import identifier.\n" );
		break;

	case ERROR_NO_ATTACKIDENT:
		CG_Printf( "WEAPONSCRIPT ERROR: Incomplete attack identifier.\n" );
		break;

	case ERROR_IMPORTSYMBOL_UNKNOWN:
		CG_Printf( "WEAPONSCRIPT ERROR: Unresolved import identifier '%s'.\n", string1 );
		break;

	case ERROR_ATTACKSYMBOL_UNKNOWN:
		CG_Printf( "WEAPONSCRIPT ERROR: Unresolved attack identifier '%s'.\n", string1 );
		break;

	case ERROR_IMPORT_FAILED:
		CG_Printf( "WEAPONSCRIPT ERROR: Failed to import '%s'.\n", string1 );

	case ERROR_MAXIMPORT_EXCEEDED:
		CG_Printf( "WEAPONSCRIPT ERROR: Script exceeds maximum import depth.\n" );
		break;

	case ERROR_NONVALID_INHERITANCE:
		CG_Printf( "WEAPONSCRIPT ERROR: Can not inherit from '%s'.\n", string1 );
		break;

	case ERROR_NOT_SUBSECTION:
		CG_Printf( "WEAPONSCRIPT ERROR: '%s' is not a valid weapon declaration subsection.\n", string1 );
		break;

	case ERROR_NOT_DAMAGETYPE:
		CG_Printf( "WEAPONSCRIPT ERROR: '%s' is not a valid damage type.\n", string1 );
		break;

	case ERROR_NOT_HOMINGTYPE:
		CG_Printf( "WEAPONSCRIPT ERROR: '%s' is not a valid homing type.\n", string1 );
		break;

	case ERROR_UNKNOWNPROPERTY:
		CG_Printf( "WEAPONSCRIPT ERROR: '%s' is not a valid property or flag of '%s'.\n", string1, string2 );
		break;

	default:
		CG_Printf( "WEAPONSCRIPT ERROR: Unknown error occured.\n" );
		break;
	}
}

/*
   -------------------------------------------------
     B L O C K   R E A D I N G   F U N C T I O N S
   -------------------------------------------------
*/

qboolean CG_weapGfxParser_ReadAttackBlock( char *script, g_userWeapon_t *weaponInfo ) {
	char *scriptPos;
	char *ungetPos;
	char *token;

	scriptPos = script;

	// Read the opening bracket.
	token = COM_Parse( &scriptPos );
	if ( Q_stricmp( token, "{" ) ) {
		CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "{" );
		return qfalse;
	}

	// Stay in this block until the closing bracket is met.
	token = COM_Parse( &scriptPos );
	if ( !(*token) ) {
		CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
		return qfalse;
	}
	while ( Q_stricmp( token, "}" ) ) {

		if ( !Q_stricmp( token, "Costs" ) ) {
			// Read costs section
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Processing Costs section.\n" );
			}

			// Read the opening bracket.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			if ( Q_stricmp( token, "{" ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "{" );
				return qfalse;
			}

			// Stay in this block until the closing bracket is met.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}
			while ( Q_stricmp( token, "}" ) ) {
				// Read items in costs section

				if ( !Q_stricmp( token, "hp" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->costs_hp = atoi(token);



				} else if ( !Q_stricmp( token, "ki" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->costs_ki = atoi(token);


				
				} else if ( !Q_stricmp( token, "stamina" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->costs_stamina = atoi(token);

			
				
				} else if ( !Q_stricmp( token, "chargeTime" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->costs_chargeTime = atoi(token);
				
				
				} else if ( !Q_stricmp( token, "chargeReady" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->costs_chargeReady = atoi(token);

				
				
				} else if ( !Q_stricmp( token, "cooldownTime" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->costs_cooldownTime = atoi(token);

				
				
				} else if ( !Q_stricmp( token, "+ki2hp" ) ) {
					weaponInfo->costs_transKi2HP = qtrue;

				} else if ( !Q_stricmp( token, "-ki2hp" ) ) {
					weaponInfo->costs_transKi2HP = qfalse;

				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_UNKNOWNPROPERTY, token, "Costs" );
					return qfalse;
				}


				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}


		} else if ( !Q_stricmp( token, "Damage" ) ) {
			// Read damage section
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Processing Damage section.\n" );
			}

			// Read the opening bracket.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			if ( !Q_stricmp( token, "=" ) ) {
				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}

				if ( !Q_stricmp( token, "Ki" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_KI;
				} else if ( !Q_stricmp( token, "Blunt" ) || !Q_stricmp( token, "Melee" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_MELEE;
				} else if ( !Q_stricmp( token, "Slice" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_SLICE;
				} else if ( !Q_stricmp( token, "Pierce" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_PIERCE;
				} else if ( !Q_stricmp( token, "Petrify" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_STONE;
				} else if ( !Q_stricmp( token, "Burn" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_BURN;
				} else if ( !Q_stricmp( token, "Candy" ) ) {
					weaponInfo->damage_meansOfDeath = UMOD_CANDY;
				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_NOT_DAMAGETYPE, token, NULL );
					return qfalse;
				}

				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}

			if ( Q_stricmp( token, "{" ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "{" );
				return qfalse;
			}


			// Stay in this block until the closing bracket is met.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}
			while ( Q_stricmp( token, "}" ) ) {
				// Read items in damage section

				if ( !Q_stricmp( token, "damage" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->damage_damage = atoi(token);

					ungetPos = scriptPos;
					token = COM_Parse( &scriptPos );
					if ( token[0] == '+' ) {
						if ( token[1] ) {
							weaponInfo->damage_multiplier = atoi(&token[1]);
						} else {
							CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						}
					} else {
						scriptPos = ungetPos;
					}



				} else if ( !Q_stricmp( token, "radius" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->damage_radius = atoi(token);

					ungetPos = scriptPos;
					token = COM_Parse( &scriptPos );
					if ( token[0] == '+' ) {
						if ( token[1] ) {
							weaponInfo->damage_radiusMultiplier = atoi(&token[1]);
						} else {
							CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						}
					} else {
						scriptPos = ungetPos;
					}



				} else if ( !Q_stricmp( token, "knockback" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->damage_extraKnockback = atoi(token);



				} else if ( !Q_stricmp( token, "+piercing" ) ) {
					weaponInfo->damage_piercing = qtrue;

				} else if ( !Q_stricmp( token, "-piercing" ) ) {
					weaponInfo->damage_piercing = qfalse;

				} else if ( !Q_stricmp( token, "+continuous" ) ) {
					weaponInfo->damage_continuous = qtrue;

				} else if ( !Q_stricmp( token, "-continuous" ) ) {
					weaponInfo->damage_continuous = qfalse;

				} else if ( !Q_stricmp( token, "+lethal" ) ) {
					weaponInfo->damage_lethal = qtrue;

				} else if ( !Q_stricmp( token, "-lethal" ) ) {
					weaponInfo->damage_lethal = qfalse;

				} else if ( !Q_stricmp( token, "+planetary" ) ) {
					weaponInfo->damage_planetary = qtrue;

				} else if ( !Q_stricmp( token, "-planetary" ) ) {
					weaponInfo->damage_planetary = qfalse;

				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_UNKNOWNPROPERTY, token, "Damage" );
					return qfalse;
				}



				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}


		} else if ( !Q_stricmp( token, "Firing" ) ) {
			// Read firing section
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Processing Firing section.\n" );
			}

			// Read the opening bracket.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			if ( Q_stricmp( token, "{" ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "{" );
				return qfalse;
			}

			// Stay in this block until the closing bracket is met.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}
			while ( Q_stricmp( token, "}" ) ) {
				// Read items in firing section

				if ( !Q_stricmp( token, "nrShots" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						G_weapPhysParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->firing_nrShots = atoi(token);



				} else if ( !Q_stricmp( token, "offsetWidth" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->firing_offsetW = atoi(token);


				
				} else if ( !Q_stricmp( token, "offsetHeight" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->firing_offsetH = atoi(token);

			
				
				} else if ( !Q_stricmp( token, "deviateWidth" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->firing_deviateW = atoi(token);

				
				
				} else if ( !Q_stricmp( token, "deviateHeight" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->firing_deviateH = atoi(token);
					


				} else if ( !Q_stricmp( token, "+flipWidth" ) ) {
					weaponInfo->firing_offsetWFlip = qtrue;

				} else if ( !Q_stricmp( token, "-flipWidth" ) ) {
					weaponInfo->firing_offsetWFlip = qfalse;

				} else {
					G_weapPhysParser_ErrorHandle( ERROR_UNKNOWNPROPERTY, token, "Firing" );
					return qfalse;
				}

				
				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}


		} else if ( !Q_stricmp( token, "Homing" ) ) {
			// Read homing section
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Processing Homing section.\n" );
			}

			// Read the opening bracket.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			if ( !Q_stricmp( token, "=" ) ) {
				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}

				if ( !Q_stricmp( token, "None" ) ) {
					weaponInfo->homing_type = HOM_NONE;
				} else if ( !Q_stricmp( token, "Prox" ) || !Q_stricmp( token, "Proximity" ) ) {
					weaponInfo->homing_type = HOM_PROX;
				} else if ( !Q_stricmp( token, "Guided" ) ) {
					weaponInfo->homing_type = HOM_GUIDED;
				} else if ( !Q_stricmp( token, "Arch" ) ) {
					weaponInfo->homing_type = HOM_ARCH;
				} else if ( !Q_stricmp( token, "Cylinder" ) ) {
					weaponInfo->homing_type = HOM_CYLINDER;
				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_NOT_HOMINGTYPE, token, NULL );
					return qfalse;
				}

				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}

			if ( Q_stricmp( token, "{" ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "{" );
				return qfalse;
			}


			// Stay in this block until the closing bracket is met.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}
			while ( Q_stricmp( token, "}" ) ) {
				// Read items in homing section
				
				if ( !Q_stricmp( token, "range" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->homing_range = atoi(token);



				} else if ( !Q_stricmp( token, "angleWidth" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->homing_angleW = atoi(token);



				} else if ( !Q_stricmp( token, "angleHeight" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->homing_angleH = atoi(token);



				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_UNKNOWNPROPERTY, token, "Homing" );
					return qfalse;
				}

				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}


		} else if ( !Q_stricmp( token, "Physics" ) ) {
			// Read physics section
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Processing Physics section.\n" );
			}

			// Read the opening bracket.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			if ( Q_stricmp( token, "{" ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "{" );
				return qfalse;
			}

			// Stay in this block until the closing bracket is met.
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}
			while ( Q_stricmp( token, "}" ) ) {
				// Read items in physics section

				if ( !Q_stricmp( token, "speed" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_speed = atof(token);



				} else if ( !Q_stricmp( token, "acceleration" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_acceleration = atof(token);



				} else if ( !Q_stricmp( token, "gravity" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_gravity = atof(token);



				} else if ( !Q_stricmp( token, "radius" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_radius = atoi(token);

					ungetPos = scriptPos;
					token = COM_Parse( &scriptPos );
					if ( token[0] == '+' ) {
						if ( token[1] ) {
							weaponInfo->physics_radiusMultiplier = atoi(&token[1]);
						} else {
							CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						}
					} else {
						scriptPos = ungetPos;
					}



				} else if ( !Q_stricmp( token, "nrBounces" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_maxBounces = atoi(token);



				} else if ( !Q_stricmp( token, "bounceTransfer" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_bounceFrac = atof(token);



				} else if ( !Q_stricmp( token, "range" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_range = atoi(token);



				} else if ( !Q_stricmp( token, "lifetime" ) ) {
					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					if ( Q_stricmp( token, "=" ) ) {
						CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
						return qfalse;
					}

					token = COM_Parse( &scriptPos );
					if ( !(*token) ) {
						C,G_weapPhysParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
						return qfalse;
					}

					weaponInfo->physics_lifetime = atoi(token);



				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_UNKNOWNPROPERTY, token, "Physics" );
					return qfalse;
				}


				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}
			}


		} else {
			CG_weapGfxParser_ErrorHandle( ERROR_NOT_SUBSECTION, token, NULL );
			return qfalse;
		}


		token = COM_Parse( &scriptPos );
		if ( !(*token) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
			return qfalse;
		}
	}
	return qtrue;
}



/*
   -------------------------------------------------------------------------------
     B L O C K   L O C A T I O N   &   I N H E R I T A N C E   F U N C T I O N S
   -------------------------------------------------------------------------------
*/

/*
======================================
CG_weapGfxParser_DecomposeIdentifier
======================================
Splits a combined '<import>.<attack>' identifier
into its components <import> and <attack>.
*/
qboolean CG_weapGfxParser_DecomposeIdentifier( char *input, char **importIdent, char **attackIdent ) {
	int i;
	qboolean dotSet;

	Q_strncpyz( *importIdent, input, sizeof(char) * MAX_SCRIPTITEM_LENGTH );
	*attackIdent = *importIdent;

	dotSet = qfalse;
	
	i = 0;
	// As long as there is string left to parse
	while ( (*importIdent)[i] ) {
		
		// If we encounter a dot
		if ( (*importIdent)[i] == '.' ) {

			// We can only have one dot. More dots means an error
			if ( dotSet ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SECOND_DOT, NULL, NULL );
				return qfalse;
			}
			
			// Mark the dot being used and seperate the importIdent and attackIdent.
			dotSet = qtrue;
			(*importIdent)[i] = '\0';
			*attackIdent = &((*importIdent)[i + 1]);

			// If the dot was the last symbol, we would end up with a blank attackIdent,
			// which would spell trouble.
			if ( !(*attackIdent) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_NO_ATTACKIDENT, NULL, NULL );
				return qfalse;
			}
		}

		// Jump to the next symbol in the string.
		i++;
	}

	// If we didn't find a dot, there is no import identifier, so make sure
	// we return the null string for it.
	if ( !dotSet ) {
		*importIdent = &(*importIdent)[i];
	}

	return qtrue;
}

/*
=========================================
G_weapPhysParser_ParseImportIdentifiers
=========================================
Searches through the list of import statements
to find a matching import identifier and retrieve
the associated filename.
*/
qboolean CG_weapGfxParser_ParseImportIdentifiers( char *script, char *importIdent, char *filename ) {
	char *scriptPos;
	char *token;
	int braceCount;
	
	braceCount = 0;
	scriptPos = script;
	while ( 1 ) {
		token = COM_Parse( &scriptPos );

		// No more import declarations left to parse.
		if ( !(*token) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_IMPORTSYMBOL_UNKNOWN, importIdent, NULL );
			return qfalse;
		}

		if ( !Q_stricmp( token, "{" ) ) {
			braceCount++;
		}

		if ( !Q_stricmp( token, "}" ) ) {
			braceCount--;
		}

		if ( braceCount < 0 ) {
			CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, "}", NULL );
			return qfalse;
		}

		
		// Did we find the start of an import declaration and are we
		// not inside a block? (Which would mean someone used the
		// reserved keyword somewhere they shouldn't have.)
		if ( !Q_stricmp( token, "import" ) ) {

			if ( braceCount > 0 ) {
				CG_weapGfxParser_ErrorHandle( ERROR_RESERVED_KEYWORD, "import", NULL );
				return qfalse;
			}

			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			// If the token matches the entry we're looking for
			if ( !Q_stricmp( token, importIdent ) ) {


				// Read the equivalence sign of the import identifier's declaration
				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}				
				if ( Q_stricmp( token, "=" ) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
					return qfalse;
				}

				token = COM_Parse( &scriptPos );
				if ( !(*token) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
					return qfalse;
				}

				// We've found the filename, exit the function succesfully.
				Q_strncpyz( filename, token, sizeof(char) * MAX_QPATH );

				// Handle some verbose output
				if ( g_verboseWeaponParser.integer ) {
					CG_Printf( "Import identifier '%s' resolved to file '%s'.\n", importIdent, filename );
				}

				return qtrue;
			}
		}
	}

	return qfalse;
}


/*
=========================================
G_weapPhysParser_ParseAttackIdentifiers
=========================================
Searches through the list of actual weapon scripts
for a matching identifier and sets up the attack.
*/

// Need this prototyped
qboolean G_weapPhysParser_SubParse( char *filename, char *attackIdent, g_userWeapon_t *weaponInfo );

qboolean G_weapPhysParser_ParseAttackIdentifiers( char *script, char *searchIdent, char **blockStart, g_userWeapon_t *weaponInfo ) {
	char		*scriptPos;
	char		*token;
	int			braceCount;
	qboolean	exported;

	char	buffer[MAX_SCRIPTITEM_LENGTH];
	char	*importIdent = buffer;		// assign this to surpress compiler warning
	char	*attackIdent = importIdent; // assign this to surpress compiler warning
	char	newFilename[MAX_QPATH];

	braceCount = 0;
	scriptPos = script;
	exported = qfalse;
	while ( 1 ) {
		token = COM_Parse( &scriptPos );

		// No more attack declarations left to parse.
		if ( !(*token) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_ATTACKSYMBOL_UNKNOWN, attackIdent, NULL );
			return qfalse;
		}

		if ( !Q_stricmp( token, "{" ) ) {
			braceCount++;
		}

		if ( !Q_stricmp( token, "}" ) ) {
			braceCount--;
		}

		if ( braceCount < 0 ) {
			CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, "}", NULL );
			return qfalse;
		}

		
		// Did we find the start of an export declaration and are we
		// not inside a block? (Which would mean someone used the
		// reserved keyword somewhere they shouldn't have.)
		if ( !Q_stricmp( token, "export" ) ) {
			if ( braceCount > 0 ) {
				CG_weapGfxParser_ErrorHandle( ERROR_RESERVED_KEYWORD, "export", NULL );
				return qfalse;
			}

			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
			}

			exported = qtrue;
		}
		

		// We've found a match
		if ( !Q_stricmp( token, searchIdent ) && ( exported || ( G_weapPhysParser_importDepth == 0 ) ) ) {

			// Handle some verbose output
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Attack identifier '%s' found. Parsing this weapon.\n", searchIdent );
			}


			// Read the equivalence sign of the export / attack identifier's declaration
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}				
			if ( Q_stricmp( token, "=" ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
				return qfalse;
			}

			// Read the name to be used, parse out the import part if not a default type
			token = COM_Parse( &scriptPos );
			if ( !(*token) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
				return qfalse;
			}

			if (!CG_weapGfxParser_DecomposeIdentifier( token, &importIdent, &attackIdent ) ) {
				return qfalse;
			}

			// Handle some verbose output
			if ( g_verboseWeaponParser.integer ) {
				CG_Printf( "Attack declaration read:\n   Import: '%s'\n   Attack: '%s'\n", importIdent, attackIdent );
			}

			if ( *importIdent ) {
				// We have found an attack that needs to be imported from another file.

				if ( !Q_stricmp(attackIdent, "Missile") || !Q_stricmp(attackIdent, "Beam") ||
					 !Q_stricmp(attackIdent, "Hitscan") || !Q_stricmp(attackIdent, "Skim") ) {
					CG_weapGfxParser_ErrorHandle( ERROR_RESERVED_KEYWORD, attackIdent, NULL );
					return qfalse;
				}
						
				// Scan the import identifiers list for the filename to be used.
				if (!CG_weapGfxParser_ParseImportIdentifiers( script, importIdent, newFilename ) ) {
					return qfalse;
				}

				// Parse the new weapon file for the attack identifier's information
				if (!CG_weapGfxParser_SubParse( newFilename, attackIdent, weaponInfo ) ) {
					CG_weapGfxParser_ErrorHandle( ERROR_IMPORT_FAILED, attackIdent, NULL );
					return qfalse;
				}

				// We've arrived back at this level, so signal that we start overriding
				// previously set properties now.
				if ( g_verboseWeaponParser.integer ) {
					CG_Printf( "   Overriding previous settings.\n" );
				}
			
			} else {

				if ( !Q_stricmp( attackIdent, "Missile" ) ) {
					// Initialize as a missile
					weaponInfo->general_type = WPT_MISSILE;

				} else if ( !Q_stricmp( attackIdent, "Beam" ) ) {
					// Initialize as a beam
					weaponInfo->general_type = WPT_BEAM;

				} else if ( !Q_stricmp( attackIdent, "Hitscan" ) ) {
					// Initialize as a hitscan
					weaponInfo->general_type = WPT_HITSCAN;

				} else if ( !Q_stricmp( attackIdent, "Skim" ) ) {
					// Initialize as a groundskimmer
					weaponInfo->general_type = WPT_GROUNDSKIM;

				} else {
					CG_weapGfxParser_ErrorHandle( ERROR_NONVALID_INHERITANCE, attackIdent, NULL );
					return qfalse;
				}
			}

			// Be sure to return the correct start of this block!
			*blockStart = scriptPos;
			return qtrue;
		
		} else {
			exported = qfalse;
		}

	}

	return qfalse;
}


/*
===========================
G_weapPhysParser_SubParse
===========================
Parses an imported file. This function is slightly different
from the main function to suppport maximum import depth and to
ignore weapon declarations. It only has to load a file and
search for a relevant attack identifier.
*/
qboolean CG_weapGfxParser_SubParse( char *filename, char *attackIdent, g_userWeapon_t *weaponInfo ) {
	char		script[MAX_SCRIPT_LENGTH];
	char		*scriptPos;
	char		*blockPos;
	int			len;
	qhandle_t	file;
	

	CG_weapGfxParser_importDepth++;

	if ( CG_weapGfxParser_importDepth > MAX_IMPORT_DEPTH ) {
		CG_weapGfxParser_ErrorHandle( ERROR_MAXIMPORT_EXCEEDED, NULL, NULL );
		return qfalse;
	}

	// Grab file handle
	len = trap_FS_FOpenFile( filename, &file, FS_READ );
	if ( !file ) {
		CG_weapGfxParser_ErrorHandle( ERROR_FILE_NOTFOUND, filename, NULL );
		return qfalse;
	}
	if ( len >= (sizeof(script) - 1)) {
		CG_weapGfxParser_ErrorHandle( ERROR_FILE_TOOBIG, filename, NULL );
		trap_FS_FCloseFile( file );
		return qfalse;
	}

	// Read file
	trap_FS_Read( script, len, file );
	trap_FS_FCloseFile( file );
	script[len] = '\0'; // ensure null termination
	

	// Handle some verbose output
	if ( g_verboseWeaponParser.integer ) {
		CG_Printf( "Scriptfile '%s' has been read into memory.\n", filename );
	}


	// Find the attack information we're looking for in this file
	scriptPos = script;
	if (!CG_weapGfxParser_ParseAttackIdentifiers( scriptPos, attackIdent, &blockPos, weaponInfo ) ) {
		return qfalse;
	}

	if (!CG_weapGfxParser_ReadAttackBlock( blockPos, weaponInfo ) ) {
		return qfalse;
	}

	CG_weapGfxParser_importDepth--;
	return qtrue;
}


/*
============================
G_weapPhysParser_MainParse
============================
Main parsing function. This is called to parse the required files and
set up a player's weapons.
*/
qboolean CG_weapGfxParser_MainParse( char *filename, int clientNum, int *weaponMask ) {
	char	script[MAX_SCRIPT_LENGTH];
	char	*scriptPos;
	char	*blockPos;
	char	*token;

	char	buffer[MAX_SCRIPTITEM_LENGTH];
	char	*importIdent = buffer; // assign this to surpress compiler warning
	char	*attackIdent = importIdent; // assign this to surpress compiler warning
	char	newFilename[MAX_QPATH];

	int			len;
	qhandle_t	file;

	int				weaponNr;
	g_userWeapon_t	*weaponInfo;
	g_userWeapon_t	*alt_weaponInfo;

	// Handle some verbose output
	if ( g_verboseWeaponParser.integer ) {
		CG_Printf( "\n=============================\n  W E A P O N   P A R S E R\n=============================\n\n");
	}	

	// Grab file handle
	len = trap_FS_FOpenFile( filename, &file, FS_READ );
	if ( !file ) {
		CG_weapGfxParser_ErrorHandle( ERROR_FILE_NOTFOUND, filename, NULL );
		return qfalse;
	}
	if ( len >= (sizeof(script) - 1)) {
		CG_weapGfxParser_ErrorHandle( ERROR_FILE_TOOBIG, filename, NULL );
		trap_FS_FCloseFile( file );
		return qfalse;
	}

	// Read file
	trap_FS_Read( script, len, file );
	trap_FS_FCloseFile( file );
	script[len] = '\0'; // ensure null termination

	// Handle some verbose output
	if ( g_verboseWeaponParser.integer ) {
		CG_Printf( "Scriptfile '%s' has been read into memory.\n", filename );
	}

	// Reset the import depth and weaponMask
	CG_weapGfxParser_importDepth = 0;
	*weaponMask = 0;

	
	// Find a weapon declaration to work on
	for ( weaponNr = 1; weaponNr <= MAX_PLAYERWEAPONS; weaponNr++ ) {
		char	weaponIdent[10]; // Give it a bit of leeway, incase we choose bigger MAX_PLAYERWEAPONS
		int		braceCount;

		// Set which weapon declaration we are currently looking for.
		Com_sprintf( weaponIdent, sizeof(weaponIdent), "Wp%i", weaponNr );
		weaponInfo = CG_FindUserWeaponData( clientNum, weaponNr );
		alt_weaponInfo = CG_FindUserAltWeaponData( clientNum, weaponNr );
			
		// Start looking
		braceCount = 0;
		scriptPos = script;
		while ( 1 ) {
			token = COM_Parse( &scriptPos );

			if ( !(*token) ) {
				// We didn't find this weapon declaration, which means there
				// are no more weapon declarations left to parse.
				
				// Handle some verbose output
				if ( g_verboseWeaponParser.integer ) {
					CG_Printf( "Weapon parsing complete.\n", weaponIdent );
				}
				return qtrue;
			}

			if ( !Q_stricmp( token, "{" ) ) {
				braceCount++;
			}

			if ( !Q_stricmp( token, "}" ) ) {
				braceCount--;
			}

			if ( braceCount < 0 ) {
				CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, "}", NULL );
				return qfalse;
			}

			// Did we find the start of the declaration and are we
			// not inside a block? (Which would mean someone used
			// the reserved keyword somewhere they shouldn't have.)
			if ( !Q_stricmp( token, weaponIdent ) ) {
				if ( braceCount ) {
					CG_weapGfxParser_ErrorHandle( ERROR_RESERVED_KEYWORD, weaponIdent, NULL );
					return qfalse;
				}
				break;
			}
		}

		// Handle some verbose output
		if ( g_verboseWeaponParser.integer ) {
			CG_Printf( "Weapon '%s' found. Parsing this declaration.\n", weaponIdent );
		}

		// Read the equivalence sign of the weapon declaration
		token = COM_Parse( &scriptPos );
		if ( !(*token) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
			return qfalse;
		}
		if ( Q_stricmp( token, "=" ) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_SYMBOL_UNEXPECTED, token, "=" );
			return qfalse;
		}

		// Read the name to be used, parse out the import identifier
		token = COM_Parse( &scriptPos );
		if ( !(*token) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
			return qfalse;
		}
		if (!CG_weapGfxParser_DecomposeIdentifier( token, &importIdent, &attackIdent ) ) {
			return qfalse;
		}

		// Handle some verbose output
		if ( g_verboseWeaponParser.integer ) {
			CG_Printf( "Declaration read:\n   Import: '%s'\n   Attack: '%s'\n", importIdent, attackIdent );
		}

		if ( *importIdent ) {
			// We have found an attack that needs to be imported from another file.
			
			
			// Scan the import identifiers list for the filename to be used.
			if (!CG_weapGfxParser_ParseImportIdentifiers( script, importIdent, newFilename ) ) {
				return qfalse;
			}

			// Parse the new weapon file for the attack identifier's information
			if (!CG_weapGfxParser_SubParse( newFilename, attackIdent, weaponInfo ) ) {
				return qfalse;
			}			

		} else {
			// We have found an attack that has a local declaration.

			// Find the attack information we're looking for in this file
			if (!CG_weapGfxParser_ParseAttackIdentifiers( script, attackIdent, &blockPos, weaponInfo ) ) {
				return qfalse;
			}

			if (!CG_weapGfxParser_ReadAttackBlock( blockPos, weaponInfo ) ) {
				return qfalse;
			}
		}

		// Mark needscharge flag based on chargetime and chargeready parameters
		if ( weaponInfo->costs_chargeReady && weaponInfo->costs_chargeTime ) {
			weaponInfo->general_bitflags |= WPF_NEEDSCHARGE;
		}
		
		// See if there's an alternate fire
		token = COM_Parse( &scriptPos );
		if ( !(*token) ) {
			// Reached the end of the file and thus the declaration.
			// Proceed with the next weapon number.
			*weaponMask |= ( 1 << weaponNr );
			continue;
		}
		if ( Q_stricmp( token, "|" ) ) {
			// We have not found an alternate fire to process on this number,
			// thus we've reached the end of the declaration.
			// Proceed with the next weapon number.
			*weaponMask |= ( 1 << weaponNr );
			continue;
		}

		// Handle some verbose output
		if ( g_verboseWeaponParser.integer ) {
			CG_Printf( "Continuing parse to find alternate fire declaration.\n" );
		}

		// Read the name to be used, parse out the import identifier
		token = COM_Parse( &scriptPos );
		if ( !(*token) ) {
			CG_weapGfxParser_ErrorHandle( ERROR_PREMATURE_EOF, NULL, NULL );
			return qfalse;
		}
		if (!CG_weapGfxParser_DecomposeIdentifier( token, &importIdent, &attackIdent ) ) {
			return qfalse;
		}

		// Handle some verbose output
		if ( g_verboseWeaponParser.integer ) {
			CG_Printf( "Declaration read:\n   Import: '%s'\n   Attack: '%s'\n", importIdent, attackIdent );
		}
		
		if ( *importIdent ) {
			// We have found an attack that needs to be imported from another file.
			
			
			// Scan the import identifiers list for the filename to be used.
			if (!CG_weapGfxParser_ParseImportIdentifiers( script, importIdent, newFilename ) ) {
				return qfalse;
			}

			// Parse the new weapon file for the attack identifier's information
			if (!CG_weapGfxParser_SubParse( newFilename, attackIdent, alt_weaponInfo ) ) {
				CG_weapGfxParser_ErrorHandle( ERROR_IMPORT_FAILED, attackIdent, NULL );
				return qfalse;
			}
			
		} else {
			// We have found an attack that has a local declaration.

			// Scan the attack identifiers for a match.
			if (!CG_weapGfxParser_ParseAttackIdentifiers( script, attackIdent, &blockPos, alt_weaponInfo ) ) {
				return qfalse;
			}

			if (!CG_weapGfxParser_ReadAttackBlock( blockPos, alt_weaponInfo ) ) {
				return qfalse;
			}
		}

		// Mark alternate weapon present
		weaponInfo->general_bitflags |= WPF_ALTWEAPONPRESENT;

		// Mark needscharge flag based on chargetime and chargeready parameters
		if ( alt_weaponInfo->costs_chargeReady && alt_weaponInfo->costs_chargeTime ) {
			alt_weaponInfo->general_bitflags |= WPF_NEEDSCHARGE;
		}

		*weaponMask |= ( 1 << weaponNr );
	}

	return qtrue;
}

