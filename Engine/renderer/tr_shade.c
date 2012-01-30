/*
 * tr_shade.c
 * Surface rendering
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

shaderCommands_t tess;

/*
 * RB_BeginSurface
 * Initialize tesselator for shader
 */
void RB_BeginSurface(shader_t *shader, int fogNum) {
	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.isStatic					= qfalse;
	tess.offsetIndexes				= 0;
	tess.numIndexes					= 0;
	tess.numVertexes				= 0;
	tess.shader						= state;
	tess.fogNum						= fogNum;
	tess.xstages					= state->stages;
	tess.numPasses					= state->numUnfoggedPasses;
	tess.currentStageIteratorFunc	= state->optimalStageIteratorFunc;
	tess.shaderTime					= backEnd.refdef.floatTime - tess.shader->timeOffset;

	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime)
		tess.shaderTime					= tess.shader->clampTime;
}

/*
 * RB_StageIteratorGeneric
 * Prepare renderer for surface drawing
 */
void RB_StageIteratorGeneric(void) {
	shaderCommands_t	*input;
	int					stage;

	input = &tess;

	/* set face culling appropriately */
	GL_Cull(input->shader->cullType);

	/* set polygon offset if necessary */
	if (input->shader->polygonOffset) {
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}

	for(stage = 0; stage < MAX_SHADER_STAGES; stage++) {
		shaderStage_t	*pStage = tess.xstages[stage];
		glslProgram_t	*program;
		int				unit = 0;

		if (!pStage)
			break;

		/* set state */
		GL_State(pStage->stateBits);

#ifdef USE_CG
		if (pStage->cgProgram) {
			/* bind program */
			R_Cg_UseProgram(pStage->cgProgram);

			/* set parameters */
			R_Cg_SetParameters(pStage->cgProgram, pStage, input->shaderTime);
		} else {
#endif

		/* bind program */
		if (pStage->program)
			program = pStage->program;
		else
			program = tr.defaultProgram;

		R_UseProgram(program);

		/* color map */
		if (program->u_ColorMap > -1) {
			GL_SelectTexture(unit);

			if (!pStage->image)
				GL_Bind(tr.whiteImage);
			else
				GL_BindAnimatedImage(pStage);

			R_SetUniform_ColorMap(program, unit++);
		}

		/* normal map */
		if (program->u_NormalMap > -1) {
			GL_SelectTexture(unit);

			if (!pStage->normalMap)
				GL_Bind(tr.flatImage);
			else
				GL_Bind(pStage->normalMap);

			R_SetUniform_NormalMap(program, unit++);
		}

		/* offset map */
		if (program->u_OffsetMap > -1) {
			GL_SelectTexture(unit);

			if (!pStage->offsetMap)
				GL_Bind(tr.blackImage);
			else
				GL_Bind(pStage->offsetMap);

				R_SetUniform_OffsetMap(program, unit++);
		}

		/* specular map */
		if (program->u_SpecularMap > -1) {
			GL_SelectTexture(unit);

			if(!pStage->specularMap)
				GL_Bind(tr.blackImage);
			else
				GL_Bind(pStage->specularMap);

				R_SetUniform_SpecularMap(program, unit++);
		}

		/* light map */
		if (program->u_LightMap > -1) {
			GL_SelectTexture(unit);

			if(!pStage->lightMap)
				GL_Bind(tr.whiteImage);
			else
				GL_Bind(pStage->lightMap);

			R_SetUniform_LightMap(program, unit++);
		}

		/* deluxe map */
		if (program->u_DeluxeMap > -1) {
			GL_SelectTexture(unit);

			if(!pStage->deluxeMap)
				GL_Bind(tr.flatImage);
			else
				GL_Bind(pStage->deluxeMap);

			R_SetUniform_DeluxeMap(program, unit++);
		}

		/* cube map */
		if (program->u_CubeMap > -1) {
			GL_SelectTexture(unit);

			if (tr.refdef.rdflags & RDF_NOCUBEMAP) {
				GL_Bind(tr.defaultCubeImage);
			} else {
				cubeProbe_t *cubeProbe = R_FindNearestCubeProbe(backEnd.viewParms.or.origin);
				GL_Bind(cubeProbe->cubemap);
			}

			R_SetUniform_CubeMap(program, unit++);
		}

		/* ambient light */
		if (program->u_AmbientLight > -1)
			R_SetUniform_AmbientLight(program, backEnd.currentEntity->ambientLight);

		/* directed light */
		if (program->u_DirectedLight > -1)
			R_SetUniform_DirectedLight(program, backEnd.currentEntity->directedLight);

		/* entity color */
		if (program->u_EntityColor > -1)
			R_SetUniform_EntityColor(program, backEnd.currentEntity->e.shaderRGBA);

		/* fog color */
		if (program->u_FogColor > -1 && tess.fogNum)
			R_SetUniform_FogColor(program, (tr.world->fogs + tess.fogNum)->colorInt);

		/* ground distance */
		if (program->u_GroundDistance > -1)
			R_SetUniform_GroundDistance(program, backEnd.or.origin[2] - backEnd.currentEntity->e.shadowPlane);

		/* ground plane */
		if (program->u_GroundPlane > -1) {
			vec3_t	groundPlane = {  backEnd.or.axis[0][2], backEnd.or.axis[1][2], backEnd.or.axis[2][2] };
			R_SetUniform_GroundPlane(program, groundPlane);
		}

		/* light direction */
		if (program->u_LightDirection > -1)
			R_SetUniform_LightDirection(program, backEnd.currentEntity->lightDir);

		/* model matrix */
		if (program->u_ModelMatrix > -1)
			R_SetUniform_ModelMatrix(program, backEnd.or.modelMatrix);

		/* model view matrix */
		if (program->u_ModelViewMatrix > -1)
			R_SetUniform_ModelViewMatrix(program, glState.modelViewMatrix);

		/* model view projection matrix */
		if (program->u_ModelViewProjectionMatrix > -1)
			R_SetUniform_ModelViewProjectionMatrix(program, glState.modelViewProjectionMatrix);

		/* portal clipping */
		if (program->u_PortalClipping > -1)
			R_SetUniform_PortalClipping(program, backEnd.viewParms.isPortal);

		/* portal plane */
		if (program->u_PortalPlane > -1 && backEnd.viewParms.isPortal) {
			vec4_t	plane;

			plane[0] = backEnd.viewParms.portalPlane.normal[0];
			plane[1] = backEnd.viewParms.portalPlane.normal[1];
			plane[2] = backEnd.viewParms.portalPlane.normal[2];
			plane[3] = backEnd.viewParms.portalPlane.dist;

			R_SetUniform_PortalPlane(tr.defaultProgram, plane);
		}

		/* projection matrix */
		if (program->u_ProjectionMatrix > -1)
			R_SetUniform_ProjectionMatrix(program, glState.projectionMatrix);

		/* time */
		if (program->u_Time > -1)
			R_SetUniform_Time(program, input->shaderTime);

		/* view origin */
		if (program->u_ViewOrigin > -1)
			R_SetUniform_ViewOrigin(program, backEnd.viewParms.or.origin);
#ifdef USE_CG
		}
#endif

		/* non-static surface */
		if (!tess.isStatic) {
			/* bind vertex buffer object */
			R_BindIBO(tr.defaultIBO);
			R_BindVBO(tr.defaultVBO);

			/* upload vertex data */
			R_UpdateIBO(tess.indexes, tess.numIndexes);
			R_UpdateVBO(tess.xyz, tess.normal, tess.vertexColors, tess.texCoords[0], tess.texCoords[1], NULL, NULL, tess.numVertexes);
		}

		/* bind vertex attributes */
#ifdef USE_CG
		if (pStage->cgProgram) {
			GL_VertexAttributeState(R_Cg_GetAttributes(pStage->cgProgram));
			GL_VertexAttributePointers(R_Cg_GetAttributes(pStage->cgProgram));
		} else {
#endif
		GL_VertexAttributeState(program->attributes);
		GL_VertexAttributePointers(program->attributes);
#ifdef USE_CG
		}
#endif

		/* draw */
		GL_DrawElements(tess.offsetIndexes, tess.numIndexes);

		/* switch to texture unit 0 */
		GL_SelectTexture(0);
	}

	/* reset polygon offset */
	if (input->shader->polygonOffset)
		qglDisable(GL_POLYGON_OFFSET_FILL);
}

/*
 * DrawTris
 * Draw triangle outlines for debugging
 */
static void DrawTris(void) {
	GL_State(GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE);
	qglDepthRange(0, 0);

	/* texture upload */
	GL_Bind(tr.whiteImage);
	GL_SelectTexture(1);
	GL_Bind(tr.whiteImage);
	GL_SelectTexture(0);

	/* bind program */
	R_UseProgram(tr.defaultProgram);

	/* color map */
	R_SetUniform_ColorMap(tr.defaultProgram, 0);

	/* light map */
	R_SetUniform_LightMap(tr.defaultProgram, 1);

	/* model matrix */
	R_SetUniform_ModelMatrix(tr.defaultProgram, backEnd.or.modelMatrix);

	/* model view projection matrix */
	R_SetUniform_ModelViewProjectionMatrix(tr.defaultProgram, glState.modelViewProjectionMatrix);

	/* portal clipping */
	R_SetUniform_PortalClipping(tr.defaultProgram, backEnd.viewParms.isPortal);

	/* portal plane */
	if (backEnd.viewParms.isPortal) {
		vec4_t	plane;

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		R_SetUniform_PortalPlane(tr.defaultProgram, plane);
	}

	/* bind vertex attributes */
	GL_VertexAttributeState(tr.defaultProgram->attributes);
	GL_VertexAttributePointers(tr.defaultProgram->attributes);

	/* draw */
	GL_DrawElements(tess.offsetIndexes, tess.numIndexes);

	qglDepthRange(0, 1);
}

/*
 * RB_EndSurface
 * Finish surface drawing
 */
void RB_EndSurface(void) {
	if (tess.numIndexes == 0)
		return;

	if (tess.indexes[SHADER_MAX_INDEXES-1] != 0)
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");

	if (tess.xyz[SHADER_MAX_VERTEXES-1][0] != 0)
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");

	if (tess.shader == tr.shadowShader) {
		RB_ShadowTessEnd();
		return;
	}

	/* for debugging of sort order issues, stop rendering after a given sort value */
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort)
		return;

	/* update performance counters */
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes		+= tess.numVertexes;
	backEnd.pc.c_indexes		+= tess.numIndexes;
	backEnd.pc.c_totalIndexes	+= tess.numIndexes * tess.numPasses;

	/* call off to shader specific tess end function */
	tess.currentStageIteratorFunc();

	/* draw debugging stuff */
	if (r_showtris->integer)
		DrawTris();

	/* clear shader so we can tell we don't have any unclosed surfaces */
	tess.numIndexes = 0;
}
