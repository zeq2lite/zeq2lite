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

/*
=================
RB_ShadowTessEnd

triangleFromEdge[ v1 ][ v2 ]


  set triangle from edge( v1, v2, tri )
  if ( facing[ triangleFromEdge[ v1 ][ v2 ] ] && !facing[ triangleFromEdge[ v2 ][ v1 ] ) {
  }
=================
*/
void RB_ShadowTessEnd(void) {
	GLboolean rgba[4];

	/* check stencil buffer */
	if (glConfig.stencilBits < 4)
		return;

	/* enable face culling */
	qglEnable(GL_CULL_FACE);

	/* set state */
	GL_State(GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO);

	/* don't write to the color buffer */
	qglGetBooleanv(GL_COLOR_WRITEMASK, rgba);
	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	/* bind program */
	R_UseProgram(tr.shadowProgram);

	/* light direction */
	R_SetUniform_LightDirection(tr.shadowProgram, backEnd.currentEntity->lightDir);

	/* model view projection matrix */
	R_SetUniform_ModelViewProjectionMatrix(tr.shadowProgram, glState.modelViewProjectionMatrix);

	/* non-static surface */
	if (!tess.isStatic) {
		/* bind vertex buffer object */
		R_BindIBO(tr.defaultIBO);
		R_BindVBO(tr.defaultVBO);

		/* upload vertex data */
		R_UpdateIBO(tess.indexes, tess.numIndexes);
		R_UpdateVBO(tess.xyz, tess.normal, NULL, NULL, NULL, NULL, NULL, tess.numVertexes);
	}

	/* bind vertex attributes */
	GL_VertexAttributeState(tr.shadowProgram->attributes);
	GL_VertexAttributePointers(tr.shadowProgram->attributes);

	/* enable stencil test */
	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_ALWAYS, 1, 255);

	/* mirrors have the culling order reversed */
	if (backEnd.viewParms.isMirror) {
		qglCullFace(GL_FRONT);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		/* draw */
		GL_DrawElements(tess.offsetIndexes, tess.numIndexes);

		qglCullFace(GL_BACK);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		/* draw */
		GL_DrawElements(tess.offsetIndexes, tess.numIndexes);
	} else {
		qglCullFace(GL_BACK);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

		/* draw */
		GL_DrawElements(tess.offsetIndexes, tess.numIndexes);

		qglCullFace(GL_FRONT);
		qglStencilOp(GL_KEEP, GL_KEEP, GL_DECR);

		/* draw */
		GL_DrawElements(tess.offsetIndexes, tess.numIndexes);
	}

	/* reenable writing to the color buffer */
	qglColorMask(rgba[0], rgba[1], rgba[2], rgba[3]);
}

/*
=================
RB_ShadowFinish

Darken everything that is is a shadow volume.
We have to delay this until everything has been shadowed,
because otherwise shadows from different body parts would
overlap and double darken.
=================
*/
void RB_ShadowFinish(void) {
	matrix_t	matrix;
	glIndex_t	indexes[6];
	vec4_t		xyz[4];
	color4ub_t	colors[4];
	vec2_t		texCoords[4];
	int			numIndexes;
	int			numVertexes;

	/* stencil shadows are disabled */
	if (r_shadows->integer != 2)
		return;

	/* stencil buffer too small */
	if (glConfig.stencilBits < 4)
		return;

	/* enable stencil test */
	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc(GL_NOTEQUAL, 0, 255);

	/* disable face culling */
	qglDisable(GL_CULL_FACE);

	/* texture upload */
	GL_Bind(tr.whiteImage);

	/* set model view matrix */
	R_MatrixIdentity(matrix);
	R_LoadModelViewMatrix(matrix);

	/* set state */
	GL_State(GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);

	/* bind program */
	R_UseProgram(tr.passthroughProgram);

	/* color map */
	R_SetUniform_ColorMap(tr.passthroughProgram, 0);

	/* model view projection matrix */
	R_SetUniform_ModelViewProjectionMatrix(tr.passthroughProgram, glState.modelViewProjectionMatrix);

	/* portal clipping */
	R_SetUniform_PortalClipping(tr.passthroughProgram, qfalse);


	/* create full-screen quad */
	numIndexes = 0;
	numVertexes = 0;

	/* vertex 1 */
	xyz[numVertexes][0] = -100.0f;
	xyz[numVertexes][1] = 100.0f;
	xyz[numVertexes][2] = -10.0f;
	xyz[numVertexes][3] = 1.0f;

	colors[numVertexes][0] = 153;
	colors[numVertexes][1] = 153;
	colors[numVertexes][2] = 153;
	colors[numVertexes][3] = 255;

	texCoords[numVertexes][0] = 0.0f;
	texCoords[numVertexes][1] = 0.0f;

	numVertexes++;

		/* vertex 2 */
	xyz[numVertexes][0] = 100.0f;
	xyz[numVertexes][1] = 100.0f;
	xyz[numVertexes][2] = -10.0f;
	xyz[numVertexes][3] = 1.0f;

	colors[numVertexes][0] = 153;
	colors[numVertexes][1] = 153;
	colors[numVertexes][2] = 153;
	colors[numVertexes][3] = 255;

	texCoords[numVertexes][0] = 1.0f;
	texCoords[numVertexes][1] = 1.0f;

	numVertexes++;

	/* vertex 3 */
	xyz[numVertexes][0] = 100.0f;
	xyz[numVertexes][1] = -100.0f;
	xyz[numVertexes][2] = -10.0f;
	xyz[numVertexes][3] = 1.0f;

	colors[numVertexes][0] = 153;
	colors[numVertexes][1] = 153;
	colors[numVertexes][2] = 153;
	colors[numVertexes][3] = 255;

	texCoords[numVertexes][0] = 0.0f;
	texCoords[numVertexes][1] = 0.0f;

	numVertexes++;

	/* vertex 4 */
	xyz[numVertexes][0] = -100.0f;
	xyz[numVertexes][1] = -100.0f;
	xyz[numVertexes][2] = -10.0f;
	xyz[numVertexes][3] = 1.0f;

	colors[numVertexes][0] = 153;
	colors[numVertexes][1] = 153;
	colors[numVertexes][2] = 153;
	colors[numVertexes][3] = 255;

	texCoords[numVertexes][0] = 1.0f;
	texCoords[numVertexes][1] = 1.0f;

	numVertexes++;

	/* indexes */
	indexes[numIndexes++] = 0;
	indexes[numIndexes++] = 1;
	indexes[numIndexes++] = 2;
	indexes[numIndexes++] = 2;
	indexes[numIndexes++] = 1;
	indexes[numIndexes++] = 3;

	/* bind vertex buffer object */
	R_BindIBO(tr.defaultIBO);
	R_BindVBO(tr.defaultVBO);

	/* bind vertex attributes */
	GL_VertexAttributeState(tr.passthroughProgram->attributes);
	GL_VertexAttributePointers(tr.passthroughProgram->attributes);

	/* upload vertex data */
	R_UpdateIBO(indexes, numIndexes);
	R_UpdateVBO(xyz, NULL, colors, texCoords, NULL, NULL, NULL, numVertexes);

	/* draw */
	GL_DrawElements(tess.offsetIndexes, tess.numIndexes);

	/* disable stencil test */
	qglDisable(GL_STENCIL_TEST);
}
