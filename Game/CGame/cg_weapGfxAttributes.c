// Copyright (C) 2003-2005 RiO
//
// cg_weapGfxAttributes.c -- attribute evaluator for ZEQ2's weapon graphics script language.

#include "cg_weapGfxParser.h" // <-- cg_local.h included in this

/*
==========================
CG_weapGfx_FindImportRef
==========================
Finds out if an import reference was properly declared
or not. Raises an error if not. Returns index+1 if it
existed. (+1 since 0 is also an index, and is used as
false already.)
*/
int CG_weapGfx_FindImportRef( cg_weapGfxParser_t *parser, char *refname ) {
	cg_weapGfxImportRef_t	*importList;
	cg_weapGfxScanner_t		*scanner;
	int i;

	importList = parser->importRef;
	scanner = &parser->scanner;


	for ( i = 0; i < MAX_IMPORTS; i++ ) {
		if ( !importList[i].active ) {
			return qfalse;
		}

		if ( !Q_stricmp( refname, importList[i].refname ) ) {
			return i + 1;
		}
	}

	return qfalse;
}


/*
=========================
CG_weapGfx_AddImportRef
=========================
Adds an import reference to a reference list, if there is
room. Raises an error otherwise.
*/
qboolean CG_weapGfx_AddImportRef( cg_weapGfxParser_t *parser, char *refname, char *filename, char *defname ) {
	cg_weapGfxImportRef_t	*importList;
	cg_weapGfxScanner_t		*scanner;
	int i;

	importList = parser->importRef;
	scanner = &parser->scanner;

	i = 0;
	while ( i < MAX_IMPORTS ) {
		if ( importList[i].active ) {
			
			if ( !Q_stricmp( refname, importList[i].refname ) ) {
				CG_weapGfx_ErrorHandle( ERROR_IMPORT_REDEFINED, scanner, refname, NULL );
				return qfalse;
			}

			if ( !Q_stricmp( filename, importList[i].filename ) && !Q_stricmp( defname, importList[i].defname ) ) {
				CG_weapGfx_ErrorHandle( ERROR_IMPORT_DOUBLED, scanner, refname, NULL );
				return qfalse;
			}

			i++;

		} else {

			break;
		}
	}
	
	if ( i == MAX_IMPORTS ) {
		CG_weapGfx_ErrorHandle( ERROR_IMPORTS_EXCEEDED, scanner, NULL, NULL ); 
		return qfalse;
	}

	Q_strncpyz( importList[i].refname,  refname,  sizeof(importList[i].refname ) );
	Q_strncpyz( importList[i].filename, filename, sizeof(importList[i].filename) );
	Q_strncpyz( importList[i].defname,  defname,  sizeof(importList[i].defname ) );
	importList[i].active = qtrue;

	// Handle some verbose output
	if ( cg_verboseParse.integer ) {
		CG_Printf("Added import '%s' to attribute list.\n", importList[i].refname );
	}
	
	return qtrue;
}


/*
==============================
CG_weapGfx_FindDefinitionRef
==============================
Finds out if a definition reference was properly declared
or not. Raises an error if not. Returns index+1 if it
existed. (+1 since 0 is also an index, and is used as
false already.)
*/
int CG_weapGfx_FindDefinitionRef( cg_weapGfxParser_t *parser, char *refname ) {
	cg_weapGfxDefinitionRef_t	*defList;
	cg_weapGfxScanner_t			*scanner;
	int i;
	int retval;

	defList = parser->definitionRef;
	scanner = &parser->scanner;

	for ( i = 0; i < MAX_DEFINES; i++ ) {
		if ( !defList[i].active ) {
			break;
		}

		if ( !Q_stricmp( refname, defList[i].refname ) ) {
			return i + 1;
		}
	}

	retval = CG_weapGfx_FindImportRef( parser, refname );
	if ( !retval ) {
		CG_weapGfx_ErrorHandle( ERROR_DEFINITION_UNDEFINED, scanner, refname, NULL );
		return qfalse;
	} else {
		return retval + MAX_DEFINES; // So we can seperate a local ref from an import
	}

	
}

/*
=============================
CG_weapGfx_AddDefinitionRef
=============================
Adds a definition reference to a reference list, if there is
room. Raises an error otherwise.
*/
qboolean CG_weapGfx_AddDefinitionRef( cg_weapGfxParser_t *parser, char* refname, char* pos, int line, cg_weapGfxAccessLvls_t accessLvl, qboolean hasSuper, char *supername ) {
	cg_weapGfxDefinitionRef_t	*defList;
	cg_weapGfxScanner_t			*scanner;
	int i;

	defList = parser->definitionRef;
	scanner = &parser->scanner;

	if ( CG_weapGfx_FindImportRef( parser, refname ) ) {
		CG_weapGfx_ErrorHandle( ERROR_REDEFINE_IMPORT_AS_DEFINITION, scanner, refname, NULL );
		return qfalse;
	}

	i = 0;
	while ( i < MAX_DEFINES ) {
		if ( defList[i].active ) {

			if ( !Q_stricmp( refname, defList[i].refname ) ) {
				CG_weapGfx_ErrorHandle( ERROR_DEFINITION_REDEFINED, scanner, refname, NULL );
				return qfalse;
			}
			
			i++;

		} else {
			
			break;
		}
	}
	
	if ( i == MAX_DEFINES ) {
		CG_weapGfx_ErrorHandle( ERROR_DEFINITIONS_EXCEEDED, scanner, NULL, NULL ); 
		return qfalse;
	}

	Q_strncpyz( defList[i].refname,   refname,   sizeof(defList[i].refname  ) );
	Q_strncpyz( defList[i].supername, supername, sizeof(defList[i].supername) );
	defList[i].accessLvl = accessLvl;
	defList[i].hasSuper = hasSuper;
	defList[i].active = qtrue;
	defList[i].scannerPos = pos;
	defList[i].scannerLine = line;

	// Handle some verbose output
	if ( cg_verboseParse.integer ) {
		CG_Printf("Added definition '%s' to attribute list.\n", defList[i].refname );
	}

	return qtrue;
}


/*
=======================
CG_weapGfx_AddLinkRef
=======================
Adds a link reference to a reference list, if there is
room. Raises an error otherwise.
*/
qboolean CG_weapGfx_AddLinkRef( cg_weapGfxParser_t *parser, int index, char* pri_refname, char* sec_refname ) {
	cg_weapGfxLinkRef_t	*linkList;
	cg_weapGfxScanner_t	*scanner;
	int true_index;

	linkList = parser->linkRef;
	scanner = &parser->scanner;

	true_index = index - 1; // The script parses 1 as the first weapon, but arrays start at 0.

	if ( ( true_index < 0 ) || ( true_index > MAX_LINKS ) ) {
		CG_weapGfx_ErrorHandle( ERROR_LINK_BOUNDS, scanner, NULL, NULL );
		return qfalse;
	}
	
	if ( linkList[true_index].active ) {
		CG_weapGfx_ErrorHandle( ERROR_LINK_REDEFINED, scanner, NULL, NULL );
		return qfalse;
	}

	Q_strncpyz( linkList[true_index].pri_refname,  pri_refname,  sizeof(linkList[true_index].pri_refname ) );
	Q_strncpyz( linkList[true_index].sec_refname,  sec_refname,  sizeof(linkList[true_index].sec_refname ) );
	linkList[true_index].active = qtrue;

		// Handle some verbose output
	if ( cg_verboseParse.integer ) {
		if ( !strcmp( linkList[true_index].sec_refname, "" ) ) {
			CG_Printf("Added link '%s' to attribute list.\n", linkList[true_index].pri_refname );
		} else {
			CG_Printf("Added link '%s | %s' to attribute list.\n", linkList[true_index].pri_refname, linkList[true_index].sec_refname );
		}
	}

	return qtrue;
}
