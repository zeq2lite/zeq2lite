// Copyright (C) 2003-2005 RiO
//
// g_weapPhysParser.h -- Contains information for the parser, scanner and attribute evaluator
//                       of ZEQ2's weapon physics script language.

#include "g_local.h"

// -< General settings >--

#define MAX_IMPORTS	 16 // 2^4: Won't really need much of these
#define MAX_DEFINES  64	// 2^6: Made this quite large to facilitate 'library' files
#define MAX_LINKS	  6 // We only have 6 weapon definitions, after all.

#define MAX_RECURSION_DEPTH		   5
#define MAX_SCRIPT_LENGTH		8192		// 2^13
#define MAX_TOKENSTRING_LENGTH	MAX_QPATH	// Equal to MAX_QPATH, to prevent problems
											// with reading filenames.

// --< Tokens >--

#define TOKEN_EOF				   0	// End of File

#define TOKEN_OPENBLOCK			   1	// '{'
#define TOKEN_CLOSEBLOCK		   2	// '}'
#define TOKEN_OPENVECTOR		   3	// '('
#define TOKEN_CLOSEVECTOR		   4	// ')'
#define TOKEN_OPENRANGE			   5	// '['
#define TOKEN_CLOSERANGE		   6	// ']'

#define TOKEN_IMPORT			   7	// 'import'
#define TOKEN_PUBLIC			   8	// 'public'
#define TOKEN_PROTECTED			   9	// 'protected'
#define TOKEN_PRIVATE			  10	// 'private'

#define TOKEN_EQUALS			  11	// '='
#define TOKEN_PLUS				  12	// '+'
#define TOKEN_COLON				  13	// '|'

#define TOKEN_TRUE				  14	// 'true'
#define TOKEN_FALSE				  15	// 'false'
#define TOKEN_NULL				  16	// 'null'

#define TOKEN_INTEGER			  17	// <integer number>
#define TOKEN_FLOAT				  18	// <real number>
#define TOKEN_STRING			  19	// "<alphanumeric characters>"

#define TOKEN_FIELD				  20	// <field name>
#define TOKEN_CATEGORY			  21	// <category name>

#define TOKEN_WEAPON			  22	// 'weapon'

// --< Error IDs >--

typedef enum {
	ERROR_FILE_NOTFOUND,
	ERROR_FILE_TOOBIG,
	ERROR_PREMATURE_EOF,

	ERROR_STRING_TOOBIG,
	ERROR_TOKEN_TOOBIG,
	
	ERROR_UNKNOWN_SYMBOL,
	ERROR_UNEXPECTED_SYMBOL,
	ERROR_STRING_EXPECTED,
	ERROR_INTEGER_EXPECTED,
	ERROR_FLOAT_EXPECTED,
	ERROR_BOOLEAN_EXPECTED,
	
	ERROR_IMPORTS_EXCEEDED,
	ERROR_IMPORT_REDEFINED,
	ERROR_IMPORT_DOUBLED,
	ERROR_IMPORT_UNDEFINED,
	ERROR_REDEFINE_IMPORT_AS_DEFINITION,
	
	ERROR_DEFINITIONS_EXCEEDED,
	ERROR_DEFINITION_REDEFINED,
	ERROR_DEFINITION_UNDEFINED,

	ERROR_LINK_BOUNDS,
	ERROR_LINK_REDEFINED,

	ERROR_FIELD_NOT_IN_CATEGORY,
	ERROR_INVERTED_RANGE,
	ERROR_OVER_MAXBOUND,
	ERROR_UNDER_MINBOUND,
	ERROR_OVER_MAXVECTORELEMS,

	ERROR_UNKNOWN_ENUMTYPE,
	
	ERROR_MAX_RECURSION,
	ERROR_INHERITING_PRIVATE,
	ERROR_IMPORTING_NON_PUBLIC,
	ERROR_OVERRIDING_WITH_HIGHER_ACCESS
} g_weapPhysError_t;


// --< Storage structures >--

typedef struct {
	int		tokenSym;
	int		identifierIndex;
	int		intval;
	float	floatval;
	char	stringval[MAX_TOKENSTRING_LENGTH];
} g_weapPhysToken_t;

typedef struct {
	char	script[MAX_SCRIPT_LENGTH];
	int		line;
	char	*pos;
	char	filename[MAX_QPATH];
} g_weapPhysScanner_t;

typedef struct {
	char		refname[MAX_TOKENSTRING_LENGTH];
	char		filename[MAX_TOKENSTRING_LENGTH];
	char		defname[MAX_TOKENSTRING_LENGTH];
	qboolean	active;
} g_weapPhysImportRef_t;

typedef enum {
	LVL_PUBLIC,
	LVL_PROTECTED,
	LVL_PRIVATE
} g_weapPhysAccessLvls_t;

typedef struct {
	char					refname[MAX_TOKENSTRING_LENGTH];
	char					supername[MAX_TOKENSTRING_LENGTH];
	char					*scannerPos;
	int						scannerLine;
	g_weapPhysAccessLvls_t	accessLvl;
	qboolean				hasSuper;
	qboolean				active;
} g_weapPhysDefinitionRef_t;

typedef struct {
	char		pri_refname[MAX_TOKENSTRING_LENGTH];
	char		sec_refname[MAX_TOKENSTRING_LENGTH];
	qboolean	active;
} g_weapPhysLinkRef_t;

typedef struct {
	g_weapPhysScanner_t			scanner;
	g_weapPhysToken_t			token;
	g_weapPhysImportRef_t		importRef[MAX_IMPORTS];
	g_weapPhysDefinitionRef_t	definitionRef[MAX_DEFINES];
	g_weapPhysLinkRef_t			linkRef[MAX_LINKS];
} g_weapPhysParser_t;

typedef enum {
	CAT_PHYSICS,
	CAT_COSTS,
	CAT_DETONATION,
	CAT_MUZZLE,
	CAT_TRAJECTORY,
	CAT_REQUIREMENT,
	CAT_RESTRICT
} g_weapPhysCategoryIndex_t;


// Prototype these, so definition of G_weapPhysFields doesn'tAlr complain
qboolean G_weapPhys_ParseMovement( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseType( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseRequire( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseImpede( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseReaction( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseSpeed( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseAcceleration( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseRadius( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseDuration( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseRange( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseLifetime( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseSwat( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseDrain( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseBlind( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMinPowerLevel( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMaxPowerLevel( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMinHealth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMaxHealth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMinFatigue( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMaxFatigue( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMinTier( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMaxTier( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMinTotalTier( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMaxTotalTier( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseGround( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseFlight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseWater( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParsePowerLevel( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseMaximum( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseHealth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseFatigue( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseCooldownTime( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseChargeTime( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseChargeReadyPct( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseDamage( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseKnockBack( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseNrShots( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseOffsetWidth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseOffsetHeight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseAngleWidth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseAngleHeight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseFlipInWidth( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseFlipInHeight( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseFOV( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );
qboolean G_weapPhys_ParseDummy( g_weapPhysParser_t *parser, g_weapPhysCategoryIndex_t category, int field );

typedef struct {
	char		*fieldname;
	qboolean	(*parseFunc)( g_weapPhysParser_t*, g_weapPhysCategoryIndex_t, int );
} g_weapPhysField_t;

// --< Shared variables (located in G_weapPhysScanner.c) >--
extern g_weapPhysField_t g_weapPhysFields[];
extern char *g_weapPhysCategories[];


// --< Accesible functions >--

// -Lexical Scanner-
qboolean G_weapPhys_NextSym( g_weapPhysScanner_t *scanner, g_weapPhysToken_t *token );
qboolean G_weapPhys_LoadFile( g_weapPhysScanner_t *scanner, char *filename );
// -Token Parser-
void G_weapPhys_ErrorHandle( g_weapPhysError_t errorNr, g_weapPhysScanner_t *scanner, char *string1, char *string2 );
// -Attribute Evaluator-
int G_weapPhys_FindImportRef( g_weapPhysParser_t *parser, char *refname );
int G_weapPhys_FindDefinitionRef( g_weapPhysParser_t *parser, char *refname );
qboolean G_weapPhys_AddImportRef( g_weapPhysParser_t *parser, char *refname, char *filename, char *defname );
qboolean G_weapPhys_AddDefinitionRef( g_weapPhysParser_t *parser, char* refname, char* pos, int line, g_weapPhysAccessLvls_t accessLvl, qboolean hasSuper, char *supername );
qboolean G_weapPhys_AddLinkRef( g_weapPhysParser_t *parser, int index, char* pri_refname, char* sec_refname );
