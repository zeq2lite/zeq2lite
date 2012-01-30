/*
 * tr_cg.c
 * Cg program support
 * Copyright (C) 2010  Jens Loehr
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef USE_CG

#include <Cg/cg.h>
#include <Cg/cgGL.h>
#include "tr_local.h"

#ifdef _MSC_VER
#pragma comment(lib, "cg.lib")
#pragma comment(lib, "cgGL.lib")
#endif

/* program structure */
typedef struct {
	/* index */
	qhandle_t		index;

	/* program object */
	CGprogram		program;

	/* program name */
	char			vertexProgram[MAX_QPATH];
	char			vertexEntry[MAX_QPATH];

	char			fragmentProgram[MAX_QPATH];
	char			fragmentEntry[MAX_QPATH];

	/* vertex attributes */
	byte			attributes;

	/* paramters */
	CGparameter		ambientLight;
	CGparameter		colorMap;
	CGparameter		cubeMap;
	CGparameter		deluxeMap;
	CGparameter		directedLight;
	CGparameter		entityColor;
	CGparameter		fogColor;
	CGparameter		groundDistance;
	CGparameter		groundPlane;
	CGparameter		lightDirection;
	CGparameter		lightMap;
	CGparameter		modelMatrix;
	CGparameter		modelViewMatrix;
	CGparameter		modelViewProjectionMatrix;
	CGparameter		normalMap;
	CGparameter		offsetMap;
	CGparameter		portalClipping;
	CGparameter		portalPlane;
	CGparameter		projectionMatrix;
	CGparameter		specularMap;
	CGparameter		time;
	CGparameter		viewOrigin;
} cgProgram_t;

/* cg state */
static CGcontext	context;
static CGprofile	vertexProfile;
static CGprofile	fragmentProfile;
static int			currentProgram;

/* programs */
#define MAX_PROGRAMS 32
static cgProgram_t	*programs[MAX_PROGRAMS];
static int			numPrograms;

/*
 * R_Cg_AllocProgram
 * Allocate memory for program
 */
static cgProgram_t *R_Cg_AllocProgram(void) {
	cgProgram_t *program;

	if (numPrograms == MAX_PROGRAMS)
		return NULL;

	program = ri.Hunk_Alloc(sizeof(*program), h_low);
	Com_Memset(program, 0, sizeof(*program));

	program->index = numPrograms;

	programs[numPrograms] = program;
	numPrograms++;

	return program;
}

/*
 * R_Cg_GetProgramByHandle
 * Return adress of program for supplied handle
 */
static cgProgram_t *R_Cg_GetProgramByHandle(qhandle_t index) {
	/* out of range gets the default program */
	if (index < 1 || index >= numPrograms)
		return programs[0];

	return programs[index];
}

/*
 * R_Cg_BindAttributes
 * Bind required vertex attributes
 */
static void R_Cg_BindAttributes(cgProgram_t *program, char *_text) {
	char	**text = &_text;
	char	*token;

	token = COM_ParseExt(text, qtrue);
	while (token[0]) {
		/* POSITION, ATTR0 */
		if (!Q_stricmp(token, "POSITION") || !Q_stricmp(token, "ATTR0"))
			program->attributes |= ATTRIBUTE_INDEX_POSITION;
		/* NORMAL, ATTR2 */
		else if (!Q_stricmp(token, "NORMAL") || !Q_stricmp(token, "ATTR2"))
			program->attributes |= ATTRIBUTE_INDEX_NORMAL;
		/* COLOR, COLOR0, ATTR3, DIFFUSE */
		else if (!Q_stricmp(token, "COLOR") || !Q_stricmp(token, "COLOR0") || !Q_stricmp(token, "ATTR3") || !Q_stricmp(token, "DIFFUSE"))
			program->attributes |= ATTRIBUTE_INDEX_COLOR;
		/* TEXCOORD0, ATTR8 */
		else if (!Q_stricmp(token, "TEXCOORD0") || !Q_stricmp(token, "ATTR8"))
			program->attributes |= ATTRIBUTE_INDEX_TEXCOORD0;
		/* TEXCOORD1, ATTR9 */
		else if (!Q_stricmp(token, "TEXCOORD1") || !Q_stricmp(token, "ATTR9"))
			program->attributes |= ATTRIBUTE_INDEX_TEXCOORD1;
		/* TEXCOORD6, ATTR14 */
		else if (!Q_stricmp(token, "TANGENT") || !Q_stricmp(token, "TEXCOORD6") || !Q_stricmp(token, "ATTR14"))
			program->attributes |= ATTRIBUTE_INDEX_TANGENT;
		/* TEXCOORD7, ATTR15 */
		else if (!Q_stricmp(token, "BINORMAL") || !Q_stricmp(token, "TEXCOORD7") || !Q_stricmp(token, "ATTR15"))
			program->attributes |= ATTRIBUTE_INDEX_BINORMAL;

		token = COM_ParseExt(text, qtrue);
	}
}

/*
 * R_Cg_BindParameters
 * Bind parameter locations
 */
static void R_Cg_BindParameters(cgProgram_t *program) {
	program->ambientLight				= cgGetNamedParameter(program->program, "ambientLight");
	program->colorMap					= cgGetNamedParameter(program->program, "colorMap");
	program->cubeMap					= cgGetNamedParameter(program->program, "cubeMap");
	program->deluxeMap					= cgGetNamedParameter(program->program, "deluxeMap");
	program->directedLight				= cgGetNamedParameter(program->program, "directedLight");
	program->entityColor				= cgGetNamedParameter(program->program, "entityColor");
	program->fogColor					= cgGetNamedParameter(program->program, "fogColor");
	program->groundDistance				= cgGetNamedParameter(program->program, "groundDistance");
	program->groundPlane				= cgGetNamedParameter(program->program, "groundPlane");
	program->lightDirection				= cgGetNamedParameter(program->program, "lightDirection");
	program->lightMap					= cgGetNamedParameter(program->program, "lightMap");
	program->modelMatrix				= cgGetNamedParameter(program->program, "modelMatrix");
	program->modelViewMatrix			= cgGetNamedParameter(program->program, "modelViewMatrix");
	program->modelViewProjectionMatrix	= cgGetNamedParameter(program->program, "modelViewProjectionMatrix");
	program->normalMap					= cgGetNamedParameter(program->program, "normalMap");
	program->offsetMap					= cgGetNamedParameter(program->program, "offsetMap");
	program->offsetMap					= cgGetNamedParameter(program->program, "portalClipping");
	program->offsetMap					= cgGetNamedParameter(program->program, "portalPlane");
	program->projectionMatrix			= cgGetNamedParameter(program->program, "projectionMatrix");
	program->specularMap				= cgGetNamedParameter(program->program, "specularMap");
	program->time						= cgGetNamedParameter(program->program, "time");
	program->viewOrigin					= cgGetNamedParameter(program->program, "viewOrigin");
}

/*
 * R_Cg_InitPrograms
 * Create Cg context
 */
void R_Cg_InitPrograms(void) {
	ri.Printf(PRINT_ALL, "Initializing Cg Programs\n");

	numPrograms = 0;

	/* create context */
	context = cgCreateContext();
	if (!context)
		ri.Error(ERR_DROP, "R_Cg_InitPrograms: Failed to create Cg context");

	/* select vertex profile */
	vertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	if (vertexProfile == CG_PROFILE_UNKNOWN)
		ri.Error(ERR_DROP, "R_Cg_InitPrograms: Invalid vertex profile type");

	/* select fragment profile */
	fragmentProfile	= cgGLGetLatestProfile(CG_GL_FRAGMENT);
	if (fragmentProfile == CG_PROFILE_UNKNOWN)
		ri.Error(ERR_DROP, "R_Cg_InitPrograms: Invalid fragment profile type");

	ri.Printf(PRINT_ALL, "...using vertex profile %s\n", cgGetProfileString(vertexProfile));
	ri.Printf(PRINT_ALL, "...using fragment profile %s\n", cgGetProfileString(fragmentProfile));

	/* leave a space for null program */
	R_Cg_AllocProgram();
}

/*
 * R_Cg_LoadProgram
 * Load and compile Cg program
 */
qhandle_t R_Cg_LoadProgram(const char *vertexProgram, const char *vertexEntry, const char *fragmentProgram, const char *fragmentEntry) {
	CGprogram	cgVertexProgram;
	CGprogram	cgFragmentProgram;
	cgProgram_t	*program;
	char		*buffer;
	int			i;

	/* search the currently loaded programs */
	for (i = 0; i < numPrograms; i++) {
		program = programs[i];

		if (!strcmp(program->vertexProgram, vertexProgram)
		&& !strcmp(program->vertexEntry, vertexEntry)
		&& !strcmp(program->fragmentProgram, fragmentProgram)
		&& !strcmp(program->fragmentEntry, fragmentEntry))
			return program->index;
	}

	/* allocate a new cgProgram_t */
	if ((program = R_Cg_AllocProgram()) == NULL) {
		ri.Printf(PRINT_WARNING, "R_Cg_LoadProgram: R_Cg_AllocProgram() failed\n");
		return 0;
	}

	/* vertex program */
	ri.FS_ReadFile(vertexProgram, (void **)&buffer);
	if (!buffer)
		ri.Error(ERR_DROP, "R_Cg_LoadProgram: Couldn't load %s", vertexProgram);

	/* compile vertex program */
	cgGLSetOptimalOptions(vertexProfile);
	cgVertexProgram = cgCreateProgram(context, CG_SOURCE, buffer, vertexProfile, vertexEntry, NULL);

	/* check for errors in vertex program */
	if (!cgVertexProgram) {
		/* print cg error message */
		ri.Printf(PRINT_ALL, "%s\n", cgGetErrorString(cgGetError()));

		/* exit */
		ri.Error(ERR_DROP, "R_Cg_LoadProgram: Couldn't compile vertex shader for program %s", vertexProgram);
	}

	/* bind vertex attributes */
	R_Cg_BindAttributes(program, buffer);

	/* clean up */
	ri.FS_FreeFile(buffer);

	/* fragment program */
	ri.FS_ReadFile(fragmentProgram, (void **)&buffer);
	if (!buffer)
		ri.Error(ERR_DROP, "R_Cg_LoadProgram: Couldn't load %s", fragmentProgram);

	/* compile fragment program */
	cgGLSetOptimalOptions(fragmentProfile);
	cgFragmentProgram = cgCreateProgram(context, CG_SOURCE, buffer, fragmentProfile, fragmentEntry, NULL);

	/* check for errors in fragment program */
	if (!cgFragmentProgram) {
		/* print cg error message */
		ri.Printf(PRINT_ALL, "%s\n", cgGetErrorString(cgGetError()));

		/* exit */
		ri.Error(ERR_DROP, "R_Cg_LoadProgram: Couldn't compile fragment shader for program %s", fragmentProgram);
	}

	/* link complete program */
	program->program = cgCombinePrograms2(cgVertexProgram, cgFragmentProgram);

	/* clean up */
	cgDestroyProgram(cgVertexProgram);
	cgDestroyProgram(cgFragmentProgram);

	/* check for linking errors */
	if (!program->program) {
		/* print cg error message */
		ri.Printf(PRINT_ALL, "%s\n", cgGetErrorString(cgGetError()));

		/* exit */
		ri.Error(ERR_DROP, "R_Cg_LoadProgram: Couldn't link programs %s and %s", vertexProgram, fragmentProgram);
	}

	/* get parameter locations */
	R_Cg_BindParameters(program);

	/* set name */
	Q_strncpyz(program->vertexProgram, vertexProgram, sizeof(program->vertexProgram));
	Q_strncpyz(program->vertexEntry, vertexEntry, sizeof(program->vertexEntry));

	Q_strncpyz(program->fragmentProgram, fragmentProgram, sizeof(program->fragmentProgram));
	Q_strncpyz(program->vertexEntry, vertexEntry, sizeof(program->vertexEntry));

	/* clean up */
	ri.FS_FreeFile(buffer);

	return program->index;
}

/*
 * R_Cg_GetAttributes
 * Get required vertex attributes
 */
byte R_Cg_GetAttributes(qhandle_t index) {
	cgProgram_t *program;

	/* get program */
	program = R_Cg_GetProgramByHandle(index);

	return program->attributes;
}

/*
 * R_Cg_SetParameters
 * Set Cg parameters
 */
void R_Cg_SetParameters(qhandle_t index, shaderStage_t *pStage, float time) {
	cgProgram_t *program;
	int			unit = 0;

	/* get program */
	program = R_Cg_GetProgramByHandle(index);

	/* ambient light */
	if (program->ambientLight)
		cgSetParameter3fv(program->ambientLight, backEnd.currentEntity->ambientLight);

	/* color map */
	if (program->colorMap) {
		GL_SelectTexture(unit);

		if (!pStage->image)
			GL_Bind(tr.whiteImage);
		else
			GL_BindAnimatedImage(pStage);

		cgSetParameter1i(program->colorMap, unit++);
	}

	/* cube map */
	if (program->cubeMap) {
		GL_SelectTexture(unit);

		if(tr.refdef.rdflags & RDF_NOCUBEMAP) {
			GL_Bind(tr.defaultCubeImage);
		} else {
			cubeProbe_t *cubeProbe = R_FindNearestCubeProbe(backEnd.currentEntity->e.origin);
			GL_Bind(cubeProbe->cubemap);
		}

		cgSetParameter1i(program->normalMap, unit++);
	}

	/* deluxe map */
	if (program->deluxeMap) {
		GL_SelectTexture(unit);

		if(!pStage->deluxeMap)
			GL_Bind(tr.flatImage);
		else
			GL_Bind(pStage->deluxeMap);

		cgSetParameter1i(program->deluxeMap, unit++);
	}

	/* directed light */
	if (program->directedLight)
		cgSetParameter3f(program->directedLight, backEnd.currentEntity->directedLight[0] / 255.0f, backEnd.currentEntity->directedLight[1] / 255.0f, backEnd.currentEntity->directedLight[2] / 255.0f);

	/* entity color */
	if (program->entityColor)
		cgSetParameter4f(program->entityColor, backEnd.currentEntity->e.shaderRGBA[0] / 255.0f, backEnd.currentEntity->e.shaderRGBA[1] / 255.0f, backEnd.currentEntity->e.shaderRGBA[2] / 255.0f, backEnd.currentEntity->e.shaderRGBA[3] / 255.0f);

	/* fog color */
	if (program->fogColor) {
		byte temp[4];

		*(unsigned *)temp = (tr.world->fogs + tess.fogNum)->colorInt;
		cgSetParameter4f(program->fogColor, temp[0] / 255.0f, temp[1] / 255.0f, temp[2] / 255.0f, temp[3] / 255.0f);
	}

	/* ground distance */
	if (program->groundDistance)
		cgSetParameter1f(program->groundDistance, backEnd.or.origin[2] - backEnd.currentEntity->e.shadowPlane);

	/* ground plane */
	if (program->groundPlane) {
		vec3_t	groundPlane = {  backEnd.or.axis[0][2], backEnd.or.axis[1][2], backEnd.or.axis[2][2] };
		cgSetParameter3fv(program->groundPlane, groundPlane);
	}

	/* light direction */
	if (program->lightDirection)
		cgSetParameter3fv(program->lightDirection, backEnd.currentEntity->lightDir);

	/* light map */
	if (program->lightMap) {
		GL_SelectTexture(unit);

		if(!pStage->lightMap)
			GL_Bind(tr.whiteImage);
		else
			GL_Bind(pStage->lightMap);

		cgSetParameter1i(program->lightMap, unit++);
	}

	/* model matrix */
	if (program->modelMatrix)
		cgSetMatrixParameterfc(program->modelMatrix, backEnd.or.modelMatrix);

	/* model view matrix */
	if (program->modelViewMatrix)
		cgSetMatrixParameterfc(program->modelViewMatrix, glState.modelViewMatrix);

	/* model view projection matrix */
	if (program->modelViewProjectionMatrix)
		cgSetMatrixParameterfc(program->modelViewProjectionMatrix, glState.modelViewProjectionMatrix);

	/* normal map */
	if (program->normalMap) {
		GL_SelectTexture(unit);

		if(!pStage->normalMap)
			GL_Bind(tr.flatImage);
		else
			GL_Bind(pStage->normalMap);

		cgSetParameter1i(program->normalMap, unit++);
	}

	/* offset map */
	if (program->offsetMap) {
		GL_SelectTexture(unit);

		if(!pStage->offsetMap)
			GL_Bind(tr.blackImage);
		else
			GL_Bind(pStage->offsetMap);

		cgSetParameter1i(program->offsetMap, unit++);
	}

	/* portal clipping */
	if (program->portalClipping)
		cgSetParameter1i(program->portalClipping, backEnd.viewParms.isPortal);

	/* portal plane */
	if (program->portalPlane && backEnd.viewParms.isPortal) {
		vec4_t plane;

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		cgSetParameter4fv(program->portalPlane, plane);
	}

	/* projection matrix */
	if (program->projectionMatrix)
		cgSetMatrixParameterfc(program->projectionMatrix, glState.projectionMatrix);

	/* specular map */
	if (program->specularMap) {
		GL_SelectTexture(unit);

		if(!pStage->specularMap)
			GL_Bind(tr.defaultSpecularMap);
		else
			GL_Bind(pStage->specularMap);

		cgSetParameter1i(program->specularMap, unit++);
	}

	/* time */
	if (program->time)
		cgSetParameter1f(program->time, time);

	/* view origin */
	if (program->viewOrigin)
		cgSetParameter3fv(program->viewOrigin, backEnd.viewParms.or.origin);
}

/*
 * R_Cg_UseProgram
 * Switch programs
 */
void R_Cg_UseProgram(qhandle_t index) {
	cgProgram_t *program;

	/* prevent redundant calls */
	if (index == currentProgram)
		return;

	/* get program */
	program = R_Cg_GetProgramByHandle(index);

	/* activate program */
	cgGLBindProgram(program->program);
	currentProgram = program->index;
}

/*
 * R_Cg_DeletePrograms
 * Clean up programs
 */
void R_Cg_DeletePrograms(void) {
	int i;

	currentProgram	= 0;
	numPrograms		= 0;

	for (i = 0; i < numPrograms; i++)
		Com_Memset(programs[i], 0, sizeof(*programs[0]));

	Com_Memset(programs, 0, sizeof(programs));
	cgDestroyContext(context);
}

#endif /* USE_CG */
