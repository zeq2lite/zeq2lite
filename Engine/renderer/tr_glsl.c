/*
 * tr_glsl.c
 * GLSL shader program handling
 * Copyright (C) 2009  Jens Loehr <jens.loehr@gmx.de>
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

/*
 * R_GLSL_GetProgramByHandle
 * Return adress of program for supplied handle
 */
static glslProgram_t *R_GLSL_GetProgramByHandle(qhandle_t index) {
	glslProgram_t *program;

	/* out of range gets the default program */
	if (index < 1 || index >= tr.numPrograms)
		return tr.programs[0];

	program = tr.programs[index];

	return program;
}

/*
 * R_GLSL_AllocProgram
 * Reserve memory for program
 */
static glslProgram_t *R_GLSL_AllocProgram(void) {
	glslProgram_t	*program;

	if (tr.numPrograms == MAX_PROGRAMS)
		return NULL;

	program = ri.Hunk_Alloc(sizeof(*tr.programs[tr.numPrograms]), h_low);
	Com_Memset(program, 0, sizeof(program));

	program->index							= tr.numPrograms;
	program->u_AlphaGen						= -1;
	program->u_AmbientLight					= -1;
	program->u_DynamicLight					= -1;
	program->u_LightDistance				= -1;
	program->u_ColorGen						= -1;
	program->u_ConstantColor				= -1;
	program->u_DirectedLight				= -1;
	program->u_EntityColor					= -1;
	program->u_Greyscale					= -1;
	program->u_FogColor						= -1;
	program->u_IdentityLight				= -1;
	program->u_LightDirection				= -1;
	program->u_ModelViewMatrix				= -1;
	program->u_ModelViewProjectionMatrix	= -1;
	program->u_ProjectionMatrix				= -1;
	program->u_TCGen0						= -1;
	program->u_TCGen1						= -1;
	program->u_TexEnv						= -1;
	program->u_Texture0						= -1;
	program->u_Texture1						= -1;
	program->u_Texture2						= -1;
	program->u_Texture3						= -1;
	program->u_Texture4						= -1;
	program->u_Texture5						= -1;
	program->u_Texture6						= -1;
	program->u_Texture7						= -1;
	program->u_Time							= -1;
	program->u_ViewOrigin					= -1;

	tr.programs[tr.numPrograms] = program;
	tr.numPrograms++;

	return program;
}

/*
 * R_GLSL_ParseProgram
 * Parse program for uniform locations
 */
static void R_GLSL_ParseProgram(glslProgram_t *program, char *_text) {
	char	**text = &_text;
	char	*token;

	token = COM_ParseExt(text, qtrue);
	while (token[0]) {
		if (!Q_stricmp(token, "uniform")) {
			token = COM_ParseExt(text, qfalse);
			if (!Q_stricmp(token, "int")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_AlphaGen;")) {
					program->u_AlphaGen = qglGetUniformLocationARB(program->program, "u_AlphaGen");
				} else if (!Q_stricmp(token, "u_ColorGen;")) {
					program->u_ColorGen = qglGetUniformLocationARB(program->program, "u_ColorGen");
				} else if (!Q_stricmp(token, "u_Greyscale;")) {
					program->u_Greyscale = qglGetUniformLocationARB(program->program, "u_Greyscale");
				} else if (!Q_stricmp(token, "u_TCGen0;")) {
					program->u_TCGen0 = qglGetUniformLocationARB(program->program, "u_TCGen0");
				} else if (!Q_stricmp(token, "u_TCGen1;")) {
					program->u_TCGen1 = qglGetUniformLocationARB(program->program, "u_TCGen1");
				} else if (!Q_stricmp(token, "u_TexEnv;")) {
					program->u_TexEnv = qglGetUniformLocationARB(program->program, "u_TexEnv");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform int %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "float")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_IdentityLight;")) {
					program->u_IdentityLight = qglGetUniformLocationARB(program->program, "u_IdentityLight");
				} else if (!Q_stricmp(token, "u_LightDistance;")) {
					program->u_LightDistance = qglGetUniformLocationARB(program->program, "u_LightDistance");
				} else if (!Q_stricmp(token, "u_Time;")) {
					program->u_Time = qglGetUniformLocationARB(program->program, "u_Time");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform float %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "mat4")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_ModelViewMatrix;")) {
					program->u_ModelViewMatrix = qglGetUniformLocationARB(program->program, "u_ModelViewMatrix");
				} else if (!Q_stricmp(token, "u_ModelViewProjectionMatrix;")) {
					program->u_ModelViewProjectionMatrix = qglGetUniformLocationARB(program->program, "u_ModelViewProjectionMatrix");
				} else if (!Q_stricmp(token, "u_ProjectionMatrix;")) {
					program->u_ProjectionMatrix = qglGetUniformLocationARB(program->program, "u_ProjectionMatrix");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform mat4 %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "sampler2D")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_Texture0;")) {
					program->u_Texture0 = qglGetUniformLocationARB(program->program, "u_Texture0");
					R_GLSL_SetUniform_Texture0(program, 0);
				} else if (!Q_stricmp(token, "u_Texture1;")) {
					program->u_Texture1 = qglGetUniformLocationARB(program->program, "u_Texture1");
					R_GLSL_SetUniform_Texture1(program, 1);
				} else if (!Q_stricmp(token, "u_Texture2;")) {
					program->u_Texture2 = qglGetUniformLocationARB(program->program, "u_Texture2");
					R_GLSL_SetUniform_Texture2(program, 2);
				} else if (!Q_stricmp(token, "u_Texture3;")) {
					program->u_Texture3 = qglGetUniformLocationARB(program->program, "u_Texture3");
					R_GLSL_SetUniform_Texture3(program, 3);
				} else if (!Q_stricmp(token, "u_Texture4;")) {
					program->u_Texture4 = qglGetUniformLocationARB(program->program, "u_Texture4");
					R_GLSL_SetUniform_Texture4(program, 4);
				} else if (!Q_stricmp(token, "u_Texture5;")) {
					program->u_Texture5 = qglGetUniformLocationARB(program->program, "u_Texture5");
					R_GLSL_SetUniform_Texture5(program, 5);
				} else if (!Q_stricmp(token, "u_Texture6;")) {
					program->u_Texture6 = qglGetUniformLocationARB(program->program, "u_Texture6");
					R_GLSL_SetUniform_Texture6(program, 6);
				} else if (!Q_stricmp(token, "u_Texture7;")) {
					program->u_Texture7 = qglGetUniformLocationARB(program->program, "u_Texture7");
					R_GLSL_SetUniform_Texture7(program, 7);
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform sampler2D %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "vec3")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_AmbientLight;")) {
					program->u_AmbientLight = qglGetUniformLocationARB(program->program, "u_AmbientLight");
				} else if (!Q_stricmp(token, "u_DynamicLight;")) {
					program->u_DynamicLight = qglGetUniformLocationARB(program->program, "u_DynamicLight");
				} else if (!Q_stricmp(token, "u_DirectedLight;")) {
					program->u_DirectedLight = qglGetUniformLocationARB(program->program, "u_DirectedLight");
				} else if (!Q_stricmp(token, "u_LightDirection;")) {
					program->u_LightDirection = qglGetUniformLocationARB(program->program, "u_LightDirection");
				} else if (!Q_stricmp(token, "u_ViewOrigin;")) {
					program->u_ViewOrigin = qglGetUniformLocationARB(program->program, "u_ViewOrigin");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform vec3 %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "vec4")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_ConstantColor;")) {
					program->u_ConstantColor = qglGetUniformLocationARB(program->program, "u_ConstantColor");
				} else if (!Q_stricmp(token, "u_EntityColor;")) {
					program->u_EntityColor = qglGetUniformLocationARB(program->program, "u_EntityColor");
				} else if (!Q_stricmp(token, "u_FogColor;")) {
					program->u_FogColor = qglGetUniformLocationARB(program->program, "u_FogColor");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform vec4 %s unrecognized in program %s\n", token, program->name);
				}
			} else {
				ri.Printf(PRINT_WARNING, "WARNING: uniform %s unrecognized in program %s\n", token, program->name);
			}
		}

		token = COM_ParseExt(text, qtrue);
	}
}

/*
 * R_GLSL_LoadProgram
 * Load, compile and link program
 */
static qboolean R_GLSL_LoadProgram(glslProgram_t *program, const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
	GLcharARB		*buffer_vp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer_fp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer;
	GLhandleARB		shader_vp;
	GLhandleARB		shader_fp;
	GLint			status;
	char			*str;
	int				size = 0;
	int				i;

	/* create program */
	program->program = qglCreateProgramObjectARB();

	/* vertex program */
	for (i = 0, str = (char *)programVertexObjects; i < numVertexObjects; i++, str += MAX_QPATH) {
		ri.Printf(PRINT_DEVELOPER, "... loading '%s'\n", str);

		size += ri.FS_ReadFile(str, (void **)&buffer_vp[i]);
		if (!buffer_vp[i]) {
			ri.Error(ERR_DROP, "Couldn't load %s", str);
			return qfalse;
		}

		/* compile vertex shader */
		shader_vp = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		qglShaderSourceARB(shader_vp, 1, (const GLcharARB **)&buffer_vp, NULL);
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
			ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			ri.Error(ERR_DROP, "Couldn't compile vertex shader for program %s", name);
			return qfalse;
		}

		/* attach vertex shader to program */
		qglAttachObjectARB(program->program, shader_vp);
		qglDeleteObjectARB(shader_vp);
	}

	/* fragment program */
	for (i = 0, str = (char *)programFragmentObjects; i < numFragmentObjects; i++, str += MAX_QPATH) {
		ri.Printf(PRINT_DEVELOPER, "... loading '%s'\n", str);

		size += ri.FS_ReadFile(str, (void **)&buffer_fp[i]);
		if (!buffer_fp[i]) {
			ri.Error(ERR_DROP, "Couldn't load %s", str);
			return qfalse;
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
			ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			ri.Error(ERR_DROP, "Couldn't compile fragment shader for program %s", name);
			return qfalse;
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
		ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
		ri.Hunk_FreeTempMemory(msg);

		/* exit */
		ri.Error(ERR_DROP, "Couldn't link shaders for program %s", name);
		return qfalse;
	}

	/* build single large program file for parsing */
	buffer = ri.Hunk_AllocateTempMemory(++size);

	Q_strncpyz(buffer, buffer_vp[0], size);

	for (i = 1; i < numVertexObjects; i++)
		strncat(buffer, buffer_vp[i], size);

	for (i = 0; i < numFragmentObjects; i++)
		strncat(buffer, buffer_fp[i], size);

	/* get uniform locations */
	qglUseProgramObjectARB(program->program);
	R_GLSL_ParseProgram(program, buffer);
	qglUseProgramObjectARB(0);

	/* clean up */
	ri.Hunk_FreeTempMemory(buffer);

	for (i = numFragmentObjects - 1; i >= 0; i--)
		ri.FS_FreeFile(buffer_fp[i]);

	for (i = numVertexObjects - 1; i >= 0; i--)
		ri.FS_FreeFile(buffer_vp[i]);

	return qtrue;
}

/*
 * RE_GLSL_RegisterProgram
 * Loads in a program of given name
 */
qhandle_t RE_GLSL_RegisterProgram(const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
	glslProgram_t	*program;
	qhandle_t		hProgram;

	if (!vertexShaders)
			return 0;

	if (!name || !name[0]) {
		ri.Printf(PRINT_ALL, "RE_GLSL_RegisterProgram: NULL name\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Printf("Program name exceeds MAX_QPATH\n");
		return 0;
	}

	/* search the currently loaded programs */
	for (hProgram = 0; hProgram < tr.numPrograms; hProgram++) {
		program = tr.programs[hProgram];
		if (!strcmp(program->name, name)) {
			if (!program->valid)
				return 0;

			return hProgram;
		}
	}

	/* allocate a new glslProgram_t */
	if ((program = R_GLSL_AllocProgram()) == NULL) {
		ri.Printf(PRINT_WARNING, "RE_GLSL_RegisterProgram: R_GLSL_AllocProgram() failed for '%s'\n", name);
		return 0;
	}

	/* only set the name after the program has successfully loaded */
	Q_strncpyz(program->name, name, sizeof(program->name));

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	/* load the files */
	if (!R_GLSL_LoadProgram(program, name, programVertexObjects, numVertexObjects, programFragmentObjects, numFragmentObjects)) {
		qglDeleteObjectARB(program->program);
		program->valid = qfalse;
		return 0;
	}

	program->valid = qtrue;
	return program->index;
}

/*
 * R_GLSL_UseProgram
 * Use specified program or switch back to standard rendering pipeline
 */
void R_GLSL_UseProgram(qhandle_t index) {
	glslProgram_t	*program;

	if (!vertexShaders)
			return;

	/* are we allready running the program? */
	if (index == glState.currentProgram)
		return;

	/* disable the glsl rendering path */
	if (!index) {
		qglUseProgramObjectARB(0);
		glState.currentProgram = 0;
		return;
	}

	/* bad program specified */
	program = R_GLSL_GetProgramByHandle(index);
	if (!program->valid)
		program = R_GLSL_GetProgramByHandle(tr.defaultProgram);

	qglUseProgramObjectARB(program->program);
	glState.currentProgram = program->index;
}

/*
 * R_GLSL_Init
 * Load all default GLSL programs which are not loaded via the q3 shader system
 */
void R_GLSL_Init(void) {
	glslProgram_t	*program;
	char			programVertexObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	char			programFragmentObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];

	if (!vertexShaders)
		return;

	ri.Printf(PRINT_ALL, "Initializing Programs\n");

	tr.numPrograms = 0;

	/* leave a space for NULL program */
	program = R_GLSL_AllocProgram();
	program->valid = qfalse;

	/* skip program */
	program = R_GLSL_AllocProgram();
	Q_strncpyz(program->name, "skip", sizeof(program->name));
	program->valid = qtrue;

	/* load default programs */
	tr.skipProgram = RE_GLSL_RegisterProgram("skip", (const char *)NULL, 0, (const char *)NULL, 0);

	Q_strncpyz(programVertexObjects[0], "glsl/generic_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/generic_fp.glsl", sizeof(programFragmentObjects[0]));
	Q_strncpyz(programFragmentObjects[1], "glsl/texturing.glsl", sizeof(programFragmentObjects[1]));
	tr.defaultProgram = RE_GLSL_RegisterProgram("generic", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 2);

	Q_strncpyz(programVertexObjects[0], "glsl/vertexLitTexture_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/vertexLitTexture_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.vertexLitProgram = RE_GLSL_RegisterProgram("vertexLitTexture", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/lightmappedMultitexture_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/lightmappedMultitexture_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.lightmappedMultitextureProgram = RE_GLSL_RegisterProgram("lightmappedMultitexture", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/sky_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/sky_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.skyProgram = RE_GLSL_RegisterProgram("sky", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);
}
