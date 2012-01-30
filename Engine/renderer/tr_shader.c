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
#include "tr_local.h"

// tr_shader.c -- this file deals with the parsing and definition of shaders

// SKYBOX FIX START !!

#ifndef CL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// SKYBOX FIX END !!

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];
static	shader_t		shader;

#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

/*
================
return a hash value for the filename
================
*/
#ifdef __GNUCC__
  #warning TODO: check if long is ok here
#endif
static long generateHashValue( const char *fname, const int size ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);
	return hash;
}

void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh, *sh2;
	qhandle_t	h;

	sh = R_FindShaderByName( shaderName );
	if (sh == NULL || sh == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(shaderName, 0);
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(newShaderName, 0);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		if (Q_stricmp(sh->name, strippedName) == 0) {
			if (sh != sh2) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset) {
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
 * NameToSrcBlendMode
 * Convert shader command to OpenGL source blend mode
 */
static int NameToSrcBlendMode(const char *name) {
	/* GL_ONE */
	if (!Q_stricmp(name, "GL_ONE"))
		return GLS_SRCBLEND_ONE;

	/* GL_ZERO */
	else if (!Q_stricmp(name, "GL_ZERO"))
		return GLS_SRCBLEND_ZERO;

	/* GL_DST_COLOR */
	else if (!Q_stricmp(name, "GL_DST_COLOR"))
		return GLS_SRCBLEND_DST_COLOR;

	/* GL_ONE_MINUS_DST_COLOR */
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_COLOR"))
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;

	/* GL_SRC_ALPHA */
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
		return GLS_SRCBLEND_SRC_ALPHA;

	/* GL_ONE_MINUS_SRC_ALPHA */
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;

	/* GL_DST_ALPHA */
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
		return GLS_SRCBLEND_DST_ALPHA;

	/* GL_ONE_MINUS_DST_ALPHA */
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;

	/* GL_SRC_ALPHA_SATURATE */
	else if (!Q_stricmp(name, "GL_SRC_ALPHA_SATURATE"))
		return GLS_SRCBLEND_ALPHA_SATURATE;

	/* unknown blend mode */
	ri.Printf(PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_SRCBLEND_ONE;
}

/*
 * NameToDstBlendMode
 * Convert shader command to OpenGL destination blend mode
 */
static int NameToDstBlendMode(const char *name) {
	/* GL_ONE */
	if (!Q_stricmp(name, "GL_ONE"))
		return GLS_DSTBLEND_ONE;

	/* GL_ZERO */
	else if (!Q_stricmp( name, "GL_ZERO"))
		return GLS_DSTBLEND_ZERO;

	/* GL_SRC_ALPHA */
	else if (!Q_stricmp(name, "GL_SRC_ALPHA"))
		return GLS_DSTBLEND_SRC_ALPHA;

	/* GL_ONE_MINUS_SRC_ALPHA */
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;

	/* GL_DST_ALPHA */
	else if (!Q_stricmp(name, "GL_DST_ALPHA"))
		return GLS_DSTBLEND_DST_ALPHA;

	/* GL_ONE_MINUS_DST_ALPHA */
	else if (!Q_stricmp(name, "GL_ONE_MINUS_DST_ALPHA"))
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;

	/* GL_SRC_COLOR */
	else if (!Q_stricmp(name, "GL_SRC_COLOR"))
		return GLS_DSTBLEND_SRC_COLOR;

	/* GL_ONE_MINUS_SRC_COLOR */
	else if (!Q_stricmp(name, "GL_ONE_MINUS_SRC_COLOR"))
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;

	/* unknown blend mode */
	ri.Printf(PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_DSTBLEND_ONE;
}

/*
 * ParseStage
 * Parse a shader stage, which is the equivalent of one rendering pass
 */
static qboolean ParseStage(shaderStage_t *stage, char **text) {
	char *token;
	char programVertexObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	char programFragmentObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	int numVertexObjects = 0;
	int numFragmentObjects = 0;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	for(;;) {
		token = COM_ParseExt(text, qtrue);
		if (!token[0]) {
			ri.Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		/* finish stage */
		if (token[0] == '}') {
#ifdef USE_CG
			/* use Cg program */
			if (numVertexObjects == 2 && !Q_stricmp(COM_GetExtension(programVertexObjects[1]), "")
				&& numFragmentObjects == 2 && !Q_stricmp(COM_GetExtension(programFragmentObjects[1]), "")) {
				stage->cgProgram = R_Cg_LoadProgram((const char *)programVertexObjects[0], (const char *)programVertexObjects[1], (const char *)programFragmentObjects[0], (const char *)programFragmentObjects[1]);
				if (!stage->cgProgram) {
					ri.Printf(PRINT_WARNING, "WARNING: R_Cg_LoadProgram failed for shader '%s'\n", shader.name);
					return qfalse;
				}
			} else
#endif
			/* use GLSL program */
			if (numVertexObjects && numFragmentObjects) {
				stage->program = R_LoadProgram((const char *)programVertexObjects, numVertexObjects, (const char *)programFragmentObjects, numFragmentObjects);
				if (!stage->program) {
					ri.Printf(PRINT_WARNING, "WARNING: R_LoadProgram failed for shader '%s'\n", shader.name);
					return qfalse;
				}
			} else if (numVertexObjects) {
				ri.Printf(PRINT_WARNING, "WARNING: no fragment program specified for shader '%s'\n", shader.name);
			} else if (numFragmentObjects) {
				ri.Printf(PRINT_WARNING, "WARNING: no vertex program specified for shader '%s'\n", shader.name);
			}

			break;
		}

		/* vertexProgram <path1> .... <pathN> */
		else if (!Q_stricmp(token, "vertexProgram")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter(s) for 'vertexProgram' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			/* parse up to MAX_PROGRAM_OBJECTS files */
			for(;;) {
				if (numVertexObjects < MAX_PROGRAM_OBJECTS) {
					Q_strncpyz(programVertexObjects[numVertexObjects], token, sizeof(programVertexObjects[numVertexObjects]));
					numVertexObjects++;
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: Too many parameters for 'vertexProgram' keyword in shader '%s'\n", shader.name);
					return qfalse;
				}

				token = COM_ParseExt(text, qfalse);
				if (!token[0])
					break;
			}
		}

		/* fragmentProgram <path1> .... <pathN> */
		else if (!Q_stricmp(token, "fragmentProgram")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter(s) for 'fragmentProgram' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			/* parse up to MAX_PROGRAM_OBJECTS files */
			for(;;) {
				if (numFragmentObjects < MAX_PROGRAM_OBJECTS) {
					Q_strncpyz(programFragmentObjects[numFragmentObjects], token, sizeof(programFragmentObjects[numFragmentObjects]));
					numFragmentObjects++;
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: Too many parameters for 'fragmentProgram' keyword in shader '%s'\n", shader.name);
					return qfalse;
				}

				token = COM_ParseExt(text, qfalse);
				if (!token[0])
					break;
			}
		}

		/* map <name> */
		else if (!Q_stricmp(token, "map")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "$whiteimage")) {
				stage->image[0] = tr.whiteImage;
				continue;
			} else {
				stage->image[0] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
				if (!stage->image[0]) {
					ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
					return qfalse;
				}
			}
		}

		/* normalmap <name> */
		else if (!Q_stricmp(token, "normalmap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'normalmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->normalMap = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
			if (!stage->normalMap) {
				ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}

		/* offsetmap <name> */
		else if (!Q_stricmp(token, "offsetmap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'offsetmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->offsetMap = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
			if (!stage->offsetMap) {
				ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}

		/* specularmap <name> */
		else if (!Q_stricmp(token, "specularmap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'specularmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->specularMap = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
			if (!stage->specularMap) {
				ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}

		/* lightmap <name> */
		else if (!Q_stricmp(token, "lightmap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'lightmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "$whiteimage")) {
				stage->lightMap = tr.whiteImage;
				continue;
			} else if (!Q_stricmp( token, "$lightmap")) {
				if (shader.lightmapIndex < 0)
					stage->lightMap = tr.whiteImage;
				else if (!tr.lightmapSize)
					stage->lightMap = tr.lightmaps[shader.lightmapIndex];
				else
					stage->lightMap = tr.lightmaps[0];

				continue;
			} else {
				stage->lightMap = R_FindImageFile(token, qfalse, qfalse, GL_CLAMP_TO_EDGE);
				if (!stage->lightMap) {
					ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
					return qfalse;
				}
			}
		}

		/* deluxemap <name> */
		else if (!Q_stricmp(token, "deluxemap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'deluxemap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp( token, "$deluxemap")) {
				if (shader.lightmapIndex < 0)
					stage->deluxeMap = tr.flatImage;
				else if (!tr.lightmapSize)
					stage->deluxeMap = tr.deluxemaps[0];
				else
					stage->deluxeMap = tr.deluxemaps[shader.lightmapIndex];

				continue;
			} else {
				stage->deluxeMap = R_FindImageFile(token, qfalse, qfalse, GL_CLAMP_TO_EDGE);
				if (!stage->deluxeMap) {
					ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
					return qfalse;
				}
			}
		}

		/* clampmap <name> */
		else if (!Q_stricmp(token, "clampmap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->image[0] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_CLAMP_TO_EDGE);
			if (!stage->image[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return qfalse;
			}
		}

		/* animmap <frequency> <image1> .... <imageN> */
		else if (!Q_stricmp(token, "animmap")) {
			token = COM_ParseExt(text, qfalse);

			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'animMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->imageAnimationSpeed = atof(token);

			/* parse up to MAX_IMAGE_ANIMATIONS animations */
			for(;;) {
				int num;

				token = COM_ParseExt(text, qfalse);
				if (!token[0])
					break;

				num = stage->numImageAnimations;
				if (num < MAX_IMAGE_ANIMATIONS) {
					stage->image[num] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
					if (!stage->image[num]) {
						ri.Printf(PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
						return qfalse;
					}

					stage->numImageAnimations++;
				}
			}
		}

		/* videomap <name> */
		else if (!Q_stricmp(token, "videomap")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'videoMap' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			stage->videoMapHandle = ri.CIN_PlayCinematic(token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->videoMapHandle != -1) {
				stage->isVideoMap = qtrue;
				stage->image[0] = tr.scratchImage[stage->videoMapHandle];
			}
		}

		/* blendfunc <srcFactor> <dstFactor> */
		else if (!Q_stricmp(token, "blendfunc")) {
			token = COM_ParseExt(text, qfalse);
			if (token[0] == 0) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
				continue;
			}

			/* check for "simple" blends first */
			if (!Q_stricmp(token, "add")) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if (!Q_stricmp(token, "filter")) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if (!Q_stricmp( token, "blend")) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			/* complex double blends */
			} else {
				blendSrcBits = NameToSrcBlendMode(token);

				token = COM_ParseExt(text, qfalse);
				if (token[0] == 0) {
					ri.Printf(PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
					continue;
				}

				blendDstBits = NameToDstBlendMode(token);
			}

			/* clear depth mask for blended surfaces */
			if (!depthMaskExplicit)
				depthMaskBits = 0;
		}

		/* depthfunc <func> */
		else if (!Q_stricmp(token, "depthfunc")) {
			token = COM_ParseExt(text, qfalse);
			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			if (!Q_stricmp(token, "lequal")) {
				depthFuncBits = 0;
			} else if (!Q_stricmp(token, "equal")) {
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			} else {
				ri.Printf(PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}

		/* depthmask */
		else if (!Q_stricmp(token, "depthwrite")) {
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}

		/* unknown parameter */
		else {
			ri.Printf(PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name);
			return qfalse;
		}
	}

	/* implicitly assume that a GL_ONE GL_ZERO blend mask disables blending */
	if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ZERO)) {
		blendDstBits	= blendSrcBits = 0;
		depthMaskBits	= GLS_DEPTHMASK_TRUE;
	}

	/* compute state bits */
	stage->stateBits = depthMaskBits | blendSrcBits | blendDstBits | depthFuncBits;

	return qtrue;
}

/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( char **text ) {
	char		*token;
	static char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	char		pathname[MAX_QPATH];
	int			i;

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s.tga"
				, token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, GL_CLAMP_TO_EDGE );

			if ( !shader.sky.outerbox[i] ) {
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );


	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s.tga"
				, token, suf[i] );
			shader.sky.innerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, GL_REPEAT );
			if ( !shader.sky.innerbox[i] ) {
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( char **text ) {
	char	*token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	}else if ( !Q_stricmp( token, "decal" ) ) {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else {
		shader.sort = atof( token );
	}
}



// this table is also present in q3map

typedef struct {
	char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// server relevant contents
	{"water",		1,	0,	CONTENTS_WATER },
	{"slime",		1,	0,	CONTENTS_SLIME },		// mildly damaging
	{"lava",		1,	0,	CONTENTS_LAVA },		// very damaging
	{"playerclip",	1,	0,	CONTENTS_PLAYERCLIP },
	{"monsterclip",	1,	0,	CONTENTS_MONSTERCLIP },
	{"nodrop",		1,	0,	CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid",	1,	SURF_NONSOLID,	0},						// clears the solid flag

	// utility relevant attributes
	{"origin",		1,	0,	CONTENTS_ORIGIN },		// center of rotating brushes
	{"trans",		0,	0,	CONTENTS_TRANSLUCENT },	// don't eat contained surfaces
	{"detail",		0,	0,	CONTENTS_DETAIL },		// don't include in structural bsp
	{"structural",	0,	0,	CONTENTS_STRUCTURAL },	// force into structural bsp even if trnas
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"clusterportal", 1,0,  CONTENTS_CLUSTERPORTAL },	// for bots
	{"donotenter",  1,  0,  CONTENTS_DONOTENTER },		// for bots

	{"fog",			1,	0,	CONTENTS_FOG},			// carves surfaces entering
	{"sky",			0,	SURF_SKY,		0 },		// emit light from an environment map
	{"lightfilter",	0,	SURF_LIGHTFILTER, 0 },		// filter light going through it
	{"alphashadow",	0,	SURF_ALPHASHADOW, 0 },		// test light on a per-pixel basis
	{"hint",		0,	SURF_HINT,		0 },		// use as a primary splitter

	// server attributes
	{"slick",		0,	SURF_SLICK,		0 },
	{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nomarks",		0,	SURF_NOMARKS,	0 },		// don't make impact marks, but still explode
	{"ladder",		0,	SURF_LADDER,	0 },
	{"nodamage",	0,	SURF_NODAMAGE,	0 },
	{"metalsteps",	0,	SURF_METALSTEPS,0 },
	{"flesh",		0,	SURF_FLESH,		0 },
	{"nosteps",		0,	SURF_NOSTEPS,	0 },

	// drawsurf attributes
	{"nodraw",		0,	SURF_NODRAW,	0 },	// don't generate a drawsurface (or a lightmap)
	{"pointlight",	0,	SURF_POINTLIGHT, 0 },	// sample lighting at vertexes
	{"nolightmap",	0,	SURF_NOLIGHTMAP,0 },	// don't generate a lightmap
	{"nodlight",	0,	SURF_NODLIGHT, 0 },		// don't ever add dynamic lights
	{"dust",		0,	SURF_DUST, 0}			// leave a dust trail when walking on this surface
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( char **text ) {
	char	*token;
	int		numInfoParms = ARRAY_LEN( infoParms );
	int		i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if ( infoParms[i].clearSolid ) {
				si->contents &= ~CONTENTS_SOLID;
			}
#endif
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( char **text )
{
	char *token;
	int s;

	s = 0;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		ri.Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri.Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' )
		{
			break;
		}
		// stage definition
		else if ( token[0] == '{' )
		{
			if ( s >= MAX_SHADER_STAGES ) {
				ri.Printf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
				return qfalse;
			}

			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;

			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// sun parms
		else if ( !Q_stricmp( token, "q3map_sun" ) ) {
			float	a, b;

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );
			
			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "clampTime" ) ) {
			token = COM_ParseExt( text, qfalse );
      if (token[0]) {
        shader.clampTime = atof(token);
      }
    }
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) )
		{
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = qtrue;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) ) 
		{
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) 
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp(token, "portal") )
		{
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) )
		{
			ParseSkyParms( text );
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp(token, "light") ) 
		{
			token = COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull") ) 
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) )
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) )
		{
			ParseSort( text );
			continue;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
 * ComputeStageIteratorFunc
 * Assign most appropiate stage iterator
 */
static void ComputeStageIteratorFunc(void) {
	if (shader.isSky)
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
	else
		shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList( int newShader ) {
	renderCommandList_t	*cmdList = &backEndData[tr.smpFrame]->commands;

	if( cmdList ) {
		const void *curCmd = cmdList->cmds;

		while ( 1 ) {
			curCmd = PADP(curCmd, sizeof(void *));

			switch ( *(const int *)curCmd ) {
			case RC_SET_COLOR:
				{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
				curCmd = (const void *)(sc_cmd + 1);
				break;
				}
			case RC_STRETCH_PIC:
				{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
				curCmd = (const void *)(sp_cmd + 1);
				break;
				}
			case RC_DRAW_SURFS:
				{
				int i;
				drawSurf_t	*drawSurf;
				shader_t	*shader;
				int			fogNum;
				int			entityNum;
				int			sortedIndex;
				const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

				for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
					R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum );
                    sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
					if( sortedIndex >= newShader ) {
						sortedIndex++;
						drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | entityNum | ( fogNum << QSORT_FOGNUM_SHIFT );
					}
				}
				curCmd = (const void *)(ds_cmd + 1);
				break;
				}
			case RC_DRAW_BUFFER:
				{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
				curCmd = (const void *)(db_cmd + 1);
				break;
				}
			case RC_SWAP_BUFFERS:
				{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
				curCmd = (const void *)(sb_cmd + 1);
				break;
				}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int		i;
	float	sort;
	shader_t	*newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
		tr.sortedShaders[i+1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i+1 );

	newShader->sortedIndex = i+1;
	tr.sortedShaders[i+1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t	*newShader;
	int			i;
	int			hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = ri.Hunk_Alloc( sizeof( shader_t ), h_low );

	*newShader = shader;

	if ( shader.sort >= SS_SEE_THROUGH ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = ri.Hunk_Alloc( sizeof( stages[i] ), h_low );
		*newShader->stages[i] = stages[i];
	}

	SortNewShader();

	hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

    // check for a missing texture
		if ( !pStage->image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			stage++;
			continue;
		}

		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
					shader.sort = SS_BLEND0;
				}
			}
		}
		
		stage++;
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.isSky)
		shader.sort = SS_FOG;

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static char *FindShaderInShaderText( const char *shadername ) {

	char *token, *p;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if(shaderTextHashTable[hash])
	{
		for (i = 0; shaderTextHashTable[hash][i]; i++)
		{
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);
		
			if(!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

	// look for label
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
		else {
			// skip the definition
			SkipBracedSection( &p );
		}
	}

	return NULL;
}


/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	if ( (name==NULL) || (name[0] == 0) ) {
		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

/*
===============
R_FindLightmap - ydnar
given a (potentially erroneous) lightmap index, attempts to load
an external lightmap image and/or sets the index to a valid number
===============
*/

#define EXTERNAL_LIGHTMAP   "lm_%04d.tga"    // THIS MUST BE IN SYNC WITH Q3MAP2

void R_FindLightmap( int *lightmapIndex ) {
	image_t     *image;
	char fileName[ MAX_QPATH ];

	// using internal lightmaps
	if (tr.lightmapSize)
		return;

	// don't fool with bogus lightmap indexes
	if ( *lightmapIndex < 0 ) {
		return;
	}

	// does this lightmap already exist?
	if ( *lightmapIndex < tr.numLightmaps && tr.lightmaps[ *lightmapIndex ] != NULL ) {
		return;
	}

	// bail if no world dir
	if ( tr.worldDir == NULL ) {
		*lightmapIndex = LIGHTMAP_BY_VERTEX;
		return;
	}

	// sync up render thread, because we're going to have to load an image
	R_SyncRenderThread();

	// attempt to load an external lightmap
	sprintf( fileName, "%s/" EXTERNAL_LIGHTMAP, tr.worldDir, *lightmapIndex );
	image = R_FindImageFile( fileName, qfalse, qfalse, GL_CLAMP );
	if ( image == NULL ) {
		*lightmapIndex = LIGHTMAP_BY_VERTEX;
		return;
	}

	// add it to the lightmap list
	if ( *lightmapIndex >= tr.numLightmaps ) {
		tr.numLightmaps = *lightmapIndex + 1;
	}
	tr.lightmaps[ *lightmapIndex ] = image;

	// attempt to load an external deluxemap
	sprintf( fileName, "%s/" EXTERNAL_LIGHTMAP, tr.worldDir, *lightmapIndex + 1 );
	image = R_FindImageFile( fileName, qfalse, qfalse, GL_CLAMP );
	if ( image == NULL ) {
		return;
	}

	// add it to the deluxemap list
	tr.deluxemaps[ *lightmapIndex ] = image;
}

/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
shader_t *R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage ) {
	char		strippedName[MAX_QPATH];
	int			hash;
	char		*shaderText;
	image_t		*image;
	shader_t	*sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	R_FindLightmap( &lightmapIndex );

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( (sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
		     !Q_stricmp(sh->name, strippedName)) {
			// match found
			return sh;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if (r_smp->integer) {
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}

		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
		sh = FinishShader();
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	//
	image = R_FindImageFile( name, mipRawImage, mipRawImage, mipRawImage ? GL_REPEAT : GL_CLAMP_TO_EDGE );
	if ( !image ) {
		ri.Printf( PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name );
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if (shader.lightmapIndex == LIGHTMAP_BY_VERTEX) {
		stages[0].image[0]	= image;
		stages[0].active	= qtrue;
		stages[0].program	= tr.passthroughProgram;
		stages[0].stateBits	= GLS_DEFAULT;
	} else if (shader.lightmapIndex == LIGHTMAP_2D) {
		stages[0].image[0]	= image;
		stages[0].active	= qtrue;
		stages[0].program	= tr.passthroughProgram;
		stages[0].stateBits	= GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else {
		stages[0].image[0]	= image;
		stages[0].active	= qtrue;
		stages[0].program	= tr.defaultProgram;
		stages[0].stateBits	= GLS_DEFAULT;
	}

	return FinishShader();
}


qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage) {
	int			hash;
	shader_t	*sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	if ( lightmapIndex >= tr.numLightmaps ) {
		lightmapIndex = LIGHTMAP_WHITEIMAGE;
	}

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( (sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
			// index by name
			!Q_stricmp(sh->name, name)) {
			// match found
			return sh->index;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if (r_smp->integer) {
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;

	//
	// create the default shading commands
	//
	if (shader.lightmapIndex == LIGHTMAP_BY_VERTEX) {
		stages[0].image[0]	= image;
		stages[0].active	= qtrue;
		stages[0].program	= tr.passthroughProgram;
		stages[0].stateBits	= GLS_DEFAULT;
	} else if (shader.lightmapIndex == LIGHTMAP_2D) {
		stages[0].image[0]	= image;
		stages[0].active	= qtrue;
		stages[0].program	= tr.passthroughProgram;
		stages[0].stateBits	= GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else {
		stages[0].image[0]	= image;
		stages[0].active	= qtrue;
		stages[0].program	= tr.defaultProgram;
		stages[0].stateBits	= GLS_DEFAULT;
	}

	sh = FinishShader();
  return sh->index; 
}


/* 
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, int lightmapIndex ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndex, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/* 
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
	  ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void	R_ShaderList_f (void) {
	int			i;
	int			count;
	shader_t	*shader;

	ri.Printf (PRINT_ALL, "-----------------------\n");

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri.Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if (shader->lightmapIndex >= 0 ) {
			ri.Printf (PRINT_ALL, "L ");
		} else {
			ri.Printf (PRINT_ALL, "  ");
		}
		if ( shader->explicitlyDefined ) {
			ri.Printf( PRINT_ALL, "E " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri.Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri.Printf( PRINT_ALL, "sky " );
		} else {
			ri.Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name);
		} else {
			ri.Printf (PRINT_ALL,  ": %s\n", shader->name);
		}
		count++;
	}
	ri.Printf (PRINT_ALL, "%i total shaders\n", count);
	ri.Printf (PRINT_ALL, "------------------\n");
}


/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	4096
static void ScanAndLoadShaderFiles( void )
{
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	char *p;
	int numShaderFiles;
	int i;
	char *oldp, *token, *hashMem, *textEnd;
	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;

	long sum = 0, summand;
	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "scripts", ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "scripts/%s", shaderFiles[i] );
		ri.Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		summand = ri.FS_ReadFile( filename, (void **)&buffers[i] );
		
		if ( !buffers[i] )
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		
		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		while(1)
		{
			token = COM_ParseExt(&p, qtrue);
			
			if(!*token)
				break;
			
			oldp = p;
			
			token = COM_ParseExt(&p, qtrue);
			if(token[0] != '{' && token[1] != '\0')
			{
				ri.Printf(PRINT_WARNING, "WARNING: Bad shader file %s has incorrect syntax.\n", filename);
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			SkipBracedSection(&oldp);
			p = oldp;
		}
			
		
		if (buffers[i])
			sum += summand;		
	}

	// build single large buffer
	s_shaderText = ri.Hunk_Alloc( sum + numShaderFiles*2, h_low );
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;
 
	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0 ; i-- )
	{
		if ( !buffers[i] )
			continue;

		strcat( textEnd, buffers[i] );
		strcat( textEnd, "\n" );
		textEnd += strlen( textEnd );
		ri.FS_FreeFile( buffers[i] );
	}

	COM_Compress( s_shaderText );

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection(&p);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = ri.Hunk_Alloc( size * sizeof(char *), h_low );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		oldp = p;
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		SkipBracedSection(&p);
	}

	return;

}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void ) {
	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	Com_Memset(hashTable, 0, sizeof(hashTable));

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();
}
