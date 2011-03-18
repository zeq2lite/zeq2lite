// Copyright (C) 2003-2005 RiO
//
// cg_weapGfxParser.h -- Contains information for the parser, scanner and attribute evaluator
//                       of ZEQ2's weapon graphics script language.

#include "cg_local.h"

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
	
	ERROR_MAX_RECURSION,
	ERROR_INHERITING_PRIVATE,
	ERROR_IMPORTING_NON_PUBLIC,
	ERROR_OVERRIDING_WITH_HIGHER_ACCESS
} cg_weapGfxError_t;


// --< Storage structures >--

typedef struct {
	int		tokenSym;
	int		identifierIndex;
	int		intval;
	float	floatval;
	char	stringval[MAX_TOKENSTRING_LENGTH];
} cg_weapGfxToken_t;

typedef struct {
	char	script[MAX_SCRIPT_LENGTH];
	int		line;
	char	*pos;
	char	filename[MAX_QPATH];
} cg_weapGfxScanner_t;

typedef struct {
	char		refname[MAX_TOKENSTRING_LENGTH];
	char		filename[MAX_TOKENSTRING_LENGTH];
	char		defname[MAX_TOKENSTRING_LENGTH];
	qboolean	active;
} cg_weapGfxImportRef_t;

typedef enum {
	LVL_PUBLIC,
	LVL_PROTECTED,
	LVL_PRIVATE
} cg_weapGfxAccessLvls_t;

typedef struct {
	char					refname[MAX_TOKENSTRING_LENGTH];
	char					supername[MAX_TOKENSTRING_LENGTH];
	char					*scannerPos;
	int						scannerLine;
	cg_weapGfxAccessLvls_t	accessLvl;
	qboolean				hasSuper;
	qboolean				active;
} cg_weapGfxDefinitionRef_t;

typedef struct {
	char		pri_refname[MAX_TOKENSTRING_LENGTH];
	char		sec_refname[MAX_TOKENSTRING_LENGTH];
	qboolean	active;
} cg_weapGfxLinkRef_t;

typedef struct {
	cg_weapGfxScanner_t			scanner;
	cg_weapGfxToken_t			token;
	cg_weapGfxImportRef_t		importRef[MAX_IMPORTS];
	cg_weapGfxDefinitionRef_t	definitionRef[MAX_DEFINES];
	cg_weapGfxLinkRef_t			linkRef[MAX_LINKS];
} cg_weapGfxParser_t;

typedef enum {
	CAT_CHARGE,
	CAT_EXPLOSION,
	CAT_STRUGGLE,
	CAT_MISSILE,
	CAT_FLASH,
	CAT_TRAIL,
	CAT_HUD
} cg_weapGfxCategoryIndex_t;


// Prototype these so definition of cg_weapGfxFields doesn't complain
qboolean CG_weapGfx_ParseModel( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSkin( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseShader( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseAnimationRange( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSize( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseDlight( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSpin( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseTagTo( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSoundFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseVoiceFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseLoopFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseTimedFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseOnceFx( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseDuration( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseShockwave( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseMarkShader( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseMarkSize( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseRockDebris( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseParticles( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseLoopParticles( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSmokeParticles( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSpiralShader( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSpiralSize( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseSpiralOffset( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseIcon( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseDisplayName( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );
qboolean CG_weapGfx_ParseDummy( cg_weapGfxParser_t *parser, cg_weapGfxCategoryIndex_t category, int field );

typedef struct {
	char		*fieldname;
	qboolean	(*parseFunc)( cg_weapGfxParser_t*, cg_weapGfxCategoryIndex_t, int );
} cg_weapGfxField_t;

// --< Shared variables (located in cg_weapGfxScanner.c) >--
extern cg_weapGfxField_t cg_weapGfxFields[];
extern char *cg_weapGfxCategories[];


// --< Accesible functions >--

// -Lexical Scanner-
qboolean CG_weapGfx_NextSym( cg_weapGfxScanner_t *scanner, cg_weapGfxToken_t *token );
qboolean CG_weapGfx_LoadFile( cg_weapGfxScanner_t *scanner, char *filename );
// -Token Parser-
void CG_weapGfx_ErrorHandle( cg_weapGfxError_t errorNr, cg_weapGfxScanner_t *scanner, char *string1, char *string2 );
// -Attribute Evaluator-
int CG_weapGfx_FindImportRef( cg_weapGfxParser_t *parser, char *refname );
int CG_weapGfx_FindDefinitionRef( cg_weapGfxParser_t *parser, char *refname );
qboolean CG_weapGfx_AddImportRef( cg_weapGfxParser_t *parser, char *refname, char *filename, char *defname );
qboolean CG_weapGfx_AddDefinitionRef( cg_weapGfxParser_t *parser, char* refname, char* pos, int line, cg_weapGfxAccessLvls_t accessLvl, qboolean hasSuper, char *supername );
qboolean CG_weapGfx_AddLinkRef( cg_weapGfxParser_t *parser, int index, char* pri_refname, char* sec_refname );

