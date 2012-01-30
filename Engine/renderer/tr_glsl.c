/*
 * tr_glsl.c
 * GLSL program support
 * Copyright (C) 2009-2010  Jens Loehr
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

#include "tr_local.h"

static glslProgram_t *programList;

/*
 * R_GenerateHash
 * Generate unique hash identifier to avoid duplicate programs
 */
static uint64_t R_GenerateHash(const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
	uint64_t	hash = 0;
	char		*str, c;
	int			i, j;

	for (i = 0, str = (char *)programVertexObjects; i < numVertexObjects; i++, str += MAX_QPATH) {
		for (j = 0, c = tolower(str[j]); c != '\0'; j++, c = tolower(str[j])) {
			if (c == '\\' || c == PATH_SEP)
				c = '/';

			hash += c * (j + 119);
		}
	}

	for (i = 0, str = (char *)programFragmentObjects; i < numFragmentObjects; i++, str += MAX_QPATH) {
		for (j = 0, c = tolower(str[j]); c != '\0'; j++, c = tolower(str[j])) {
			if (c == '\\' || c == PATH_SEP)
				c = '/';

			hash += c * (j + 119);
		}
	}

	return hash;
}

/*
 * R_AllocProgram
 * Allocate program memory
 */
static glslProgram_t *R_AllocProgram(void) {
	glslProgram_t	*program;

	program = ri.Hunk_Alloc(sizeof(*program), h_low);
	Com_Memset(program, 0, sizeof(*program));

	if (programList)
		program->next = programList;

	programList = program;

	return program;
}

/*
 * R_BindAttributes
 * Bind required vertex attributes
 */
static void R_BindAttributes(glslProgram_t *program, char *_text) {
	char	**text = &_text;
	char	*token;

	token = COM_ParseExt(text, qtrue);
	while (token[0]) {
		/* attribute */
		if (!Q_stricmp(token, "attribute")) {
			token = COM_ParseExt(text, qfalse);
			/* vec3 */
			if (!Q_stricmp(token, "vec3")) {
				token = COM_ParseExt(text, qfalse);
				/* a_Normal */
				if (!Q_stricmp(token, "a_Normal;")) {
					program->attributes |= ATTRIBUTE_NORMAL;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_NORMAL, "a_Normal");
				}
				/* a_Tangent */
				else if (!Q_stricmp(token, "a_Tangent;")) {
					program->attributes |= ATTRIBUTE_TANGENT;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_TANGENT, "a_Tangent");
				}
				/* a_Binormal */
				else if (!Q_stricmp(token, "a_Binormal;")) {
					program->attributes |= ATTRIBUTE_BINORMAL;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_BINORMAL, "a_Binormal");
				}
			/* vec4 */
			} else if (!Q_stricmp(token, "vec4")) {
				token = COM_ParseExt(text, qfalse);
				/* a_Position */
				if (!Q_stricmp(token, "a_Position;")) {
					program->attributes |= ATTRIBUTE_POSITION;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_POSITION, "a_Position");
				}
				/* a_Color */
				else if (!Q_stricmp(token, "a_Color;")) {
					program->attributes |= ATTRIBUTE_COLOR;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_COLOR, "a_Color");
				}
				/* a_TexCoord0 */
				else if (!Q_stricmp(token, "a_TexCoord0;")) {
					program->attributes |= ATTRIBUTE_TEXCOORD0;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_TEXCOORD0, "a_TexCoord0");
				}
				/* a_TexCoord1 */
				else if (!Q_stricmp(token, "a_TexCoord1;")) {
					program->attributes |= ATTRIBUTE_TEXCOORD1;
					qglBindAttribLocationARB(program->program, ATTRIBUTE_INDEX_TEXCOORD1, "a_TexCoord1");
				}
			}
		}

		token = COM_ParseExt(text, qtrue);
	}
}

/*
 * R_BindUniforms
 * Bind uniform locations
 */
static void R_BindUniforms(glslProgram_t *program) {
	const vec3_t	nullVector		= { 0, 0, 0 };
	const byte		nullColor[4]	= { 0, 0, 0, 0 };
	const matrix_t	nullMatrix		= { 0, 0, 0, 0,
										0, 0, 0, 0,
										0, 0, 0, 0,
										0, 0, 0, 0 };

	/* u_AmbientLight */
	if ((program->u_AmbientLight = qglGetUniformLocationARB(program->program, "u_AmbientLight")) > -1)
		R_SetUniform_AmbientLight(program, nullVector);

	/* u_ColorMap */
	if ((program->u_ColorMap = qglGetUniformLocationARB(program->program, "u_ColorMap")) > -1)
		R_SetUniform_ColorMap(program, 0);

	/* u_CubeMap */
	if ((program->u_CubeMap = qglGetUniformLocationARB(program->program, "u_CubeMap")) > -1)
		R_SetUniform_CubeMap(program, 0);

	/* u_DeluxeMap */
	if ((program->u_DeluxeMap = qglGetUniformLocationARB(program->program, "u_DeluxeMap")) > -1)
		R_SetUniform_DeluxeMap(program, 0);

	/* u_DirectedLight */
	if ((program->u_DirectedLight = qglGetUniformLocationARB(program->program, "u_DirectedLight")) > -1)
		R_SetUniform_DirectedLight(program, nullVector);

	/* u_EntityColor */
	if ((program->u_EntityColor = qglGetUniformLocationARB(program->program, "u_EntityColor")) > -1)
		R_SetUniform_EntityColor(program, (byte *)nullColor);

	/* u_FogColor */
	if ((program->u_FogColor = qglGetUniformLocationARB(program->program, "u_FogColor")) > -1)
		R_SetUniform_FogColor(program, *(unsigned *)nullColor);

	/* u_GroundDistance */
	if ((program->u_GroundDistance = qglGetUniformLocationARB(program->program, "u_GroundDistance")) > -1)
		R_SetUniform_GroundDistance(program, 0.0f);

	/* u_GroundPlane */
	if ((program->u_GroundPlane = qglGetUniformLocationARB(program->program, "u_GroundPlane")) > -1)
		R_SetUniform_GroundPlane(program, nullVector);

	/* u_LightDirection */
	if ((program->u_LightDirection = qglGetUniformLocationARB(program->program, "u_LightDirection")) > -1)
		R_SetUniform_LightDirection(program, nullVector);

	/* u_LightMap */
	if ((program->u_LightMap = qglGetUniformLocationARB(program->program, "u_LightMap")) > -1)
		R_SetUniform_LightMap(program, 0);

	/* u_ModelMatrix */
	if ((program->u_ModelMatrix = qglGetUniformLocationARB(program->program, "u_ModelMatrix")) > -1)
		R_SetUniform_ModelMatrix(program, nullMatrix);

	/* u_ModelViewMatrix */
	if ((program->u_ModelViewMatrix = qglGetUniformLocationARB(program->program, "u_ModelViewMatrix")) > -1)
		R_SetUniform_ModelViewMatrix(program, nullMatrix);

	/* u_ModelViewProjectionMatrix */
	if ((program->u_ModelViewProjectionMatrix = qglGetUniformLocationARB(program->program, "u_ModelViewProjectionMatrix")) > -1)
		R_SetUniform_ModelViewProjectionMatrix(program, nullMatrix);

	/* u_NormalMap */
	if ((program->u_NormalMap = qglGetUniformLocationARB(program->program, "u_NormalMap")) > -1)
		R_SetUniform_NormalMap(program, 0);

	/* u_OffsetMap */
	if ((program->u_OffsetMap = qglGetUniformLocationARB(program->program, "u_OffsetMap")) > -1)
		R_SetUniform_OffsetMap(program, 0);

	/* u_PortalClipping */
	if ((program->u_PortalClipping = qglGetUniformLocationARB(program->program, "u_PortalClipping")) > -1)
		R_SetUniform_PortalClipping(program, 0);

	/* u_PortalPlane */
	if ((program->u_PortalPlane = qglGetUniformLocationARB(program->program, "u_PortalPlane")) > -1)
		R_SetUniform_PortalPlane(program, nullVector);

	/* u_ProjectionMatrix */
	if ((program->u_ProjectionMatrix = qglGetUniformLocationARB(program->program, "u_ProjectionMatrix")) > -1)
		R_SetUniform_ProjectionMatrix(program, nullMatrix);

	/* u_SpecularMap */
	if ((program->u_SpecularMap = qglGetUniformLocationARB(program->program, "u_SpecularMap")) > -1)
		R_SetUniform_SpecularMap(program, 0);

	/* u_Time */
	if ((program->u_Time = qglGetUniformLocationARB(program->program, "u_Time")) > -1)
		R_SetUniform_Time(program, 0.0f);

	/* u_ViewOrigin */
	if ((program->u_ViewOrigin = qglGetUniformLocationARB(program->program, "u_ViewOrigin")) > -1)
		R_SetUniform_ViewOrigin(program, nullVector);
}

/*
 * R_LoadProgram
 * Load and compile program
 */
glslProgram_t *R_LoadProgram(const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
	uint64_t		hash;
	glslProgram_t	*program;
	GLcharARB		*buffer_vp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer_fp[MAX_PROGRAM_OBJECTS];
	GLhandleARB		shader_vp;
	GLhandleARB		shader_fp;
	GLint			status;
	char			*str;
	int				size = 0;
	int				i;

	/* generate unique hash identifier */
	hash = R_GenerateHash(programVertexObjects, numVertexObjects, programFragmentObjects, numFragmentObjects);

	/* search the currently loaded programs */
	for (program = programList; program; program = program->next)
		if (program->hash == hash)
			return program;

	/* allocate a new glslProgram_t */
	if (!(program = R_AllocProgram()))
		ri.Error(ERR_DROP, "R_LoadProgram: R_AllocProgram() failed\n");

	/* only copy the hash after the program has successfully loaded */
	program->hash = hash;

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	/* create program */
	program->program = qglCreateProgramObjectARB();

	/* vertex program */
	for (i = 0, str = (char *)programVertexObjects; i < numVertexObjects; i++, str += MAX_QPATH) {
		ri.Printf(PRINT_DEVELOPER, "... loading '%s'\n", str);

		size += ri.FS_ReadFile(str, (void **)&buffer_vp[i]);
		if (!buffer_vp[i]) {
			qglDeleteObjectARB(program->program);
			ri.Error(ERR_DROP, "R_LoadProgram: Couldn't load %s", str);
		}

		/* compile vertex shader */
		shader_vp = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		qglShaderSourceARB(shader_vp, 1, (const GLcharARB **)&buffer_vp[i], NULL);
		qglCompileShaderARB(shader_vp);

		/* check for errors in vertex shader */
		qglGetObjectParameterivARB(shader_vp, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) {
			int		length;
			char	*msg;

			/* print glsl error message */
			qglGetObjectParameterivARB(shader_vp, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
			msg = ri.Hunk_AllocateTempMemory(length);
			qglGetInfoLogARB(shader_vp, length, &length, msg);
			ri.Printf(PRINT_ALL, "%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			qglDeleteObjectARB(program->program);
			ri.Error(ERR_DROP, "R_LoadProgram: Couldn't compile vertex shader for shader %s", str);
		}

		/* attach vertex shader to program */
		qglAttachObjectARB(program->program, shader_vp);
		qglDeleteObjectARB(shader_vp);

		/* bind vertex attributes */
		R_BindAttributes(program, buffer_vp[i]);
	}

	/* fragment program */
	for (i = 0, str = (char *)programFragmentObjects; i < numFragmentObjects; i++, str += MAX_QPATH) {
		ri.Printf(PRINT_DEVELOPER, "... loading '%s'\n", str);

		size += ri.FS_ReadFile(str, (void **)&buffer_fp[i]);
		if (!buffer_fp[i]) {
			qglDeleteObjectARB(program->program);
			ri.Error(ERR_DROP, "R_LoadProgram: Couldn't load %s", str);
		}

		/* compile fragment shader */
		shader_fp = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		qglShaderSourceARB(shader_fp, 1, (const GLcharARB **)&buffer_fp[i], NULL);
		qglCompileShaderARB(shader_fp);

		/* check for errors in fragment shader */
		qglGetObjectParameterivARB(shader_fp, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) {
			int		length;
			char	*msg;

			/* print glsl error message */
			qglGetObjectParameterivARB(shader_fp, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
			msg = ri.Hunk_AllocateTempMemory(length);
			qglGetInfoLogARB(shader_fp, length, &length, msg);
			ri.Printf(PRINT_ALL, "%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			qglDeleteObjectARB(program->program);
			ri.Error(ERR_DROP, "R_LoadProgram: Couldn't compile fragment shader for shader %s", str);
		}

		/* attach fragment shader to program */
		qglAttachObjectARB(program->program, shader_fp);
		qglDeleteObjectARB(shader_fp);
	}

	/* link complete program */
	qglLinkProgramARB(program->program);

	/* check for linking errors */
	qglGetObjectParameterivARB(program->program, GL_OBJECT_LINK_STATUS_ARB, &status);
	if (!status) {
		int		length;
		char	*msg;

		/* print glsl error message */
		qglGetObjectParameterivARB(program->program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		msg = ri.Hunk_AllocateTempMemory(length);
		qglGetInfoLogARB(program->program, length, &length, msg);
		ri.Printf(PRINT_ALL, "%s\n", msg);
		ri.Hunk_FreeTempMemory(msg);

		/* exit */
		qglDeleteObjectARB(program->program);
		ri.Error(ERR_DROP, "R_LoadProgram: Couldn't link shaders");
	}

	/* get uniform locations */
	qglUseProgramObjectARB(program->program);
	R_BindUniforms(program);
	qglUseProgramObjectARB(0);

	/* clean up */
	for (i = numFragmentObjects - 1; i >= 0; i--)
		ri.FS_FreeFile(buffer_fp[i]);

	for (i = numVertexObjects - 1; i >= 0; i--)
		ri.FS_FreeFile(buffer_vp[i]);

	return program;
}

/*
 * R_InitPrograms
 * Initialize GLSL subsystem
 */
void R_InitPrograms(void) {
	char			programVertexObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	char			programFragmentObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	int				i, j;

	ri.Printf(PRINT_ALL, "Initializing Programs\n");

	/* clear program list */
	programList = NULL;

	/* load default program */
	i = j = 0;
	Q_strncpyz(programVertexObjects[i++], "glsl/default.vert", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/default.frag", sizeof(programFragmentObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/lib/clipplane.frag", sizeof(programFragmentObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/lib/texture.frag", sizeof(programFragmentObjects[0]));
	tr.defaultProgram = R_LoadProgram((const char *)programVertexObjects, i, (const char *)programFragmentObjects, j);

	/* load passthrough program */
	i = j = 0;
	Q_strncpyz(programVertexObjects[i++], "glsl/passthrough.vert", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/passthrough.frag", sizeof(programFragmentObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/lib/clipplane.frag", sizeof(programFragmentObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/lib/texture.frag", sizeof(programFragmentObjects[0]));
	tr.passthroughProgram = R_LoadProgram((const char *)programVertexObjects, i, (const char *)programFragmentObjects, j);

	/* load volumeshadow program */
	i = j = 0;
	Q_strncpyz(programVertexObjects[i++], "glsl/volumeshadow.vert", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[j++], "glsl/volumeshadow.frag", sizeof(programFragmentObjects[0]));
	tr.shadowProgram = R_LoadProgram((const char *)programVertexObjects, i, (const char *)programFragmentObjects, j);
}

/*
 * R_UseProgram
 * Switch programs
 */
void R_UseProgram(glslProgram_t *program) {
	/* prevent redundant calls */
	if (program == glState.currentProgram)
		return;

	/* activate program */
	qglUseProgramObjectARB(program->program);
	glState.currentProgram = program;
}

/*
 * R_DeletePrograms
 * Clean up programs
 */
void R_DeletePrograms(void) {
	glslProgram_t	*program;

	qglUseProgramObjectARB(0);
	glState.currentProgram = 0;

	for (program = programList; program; program = program->next)
		qglDeleteObjectARB(program->program);
}
