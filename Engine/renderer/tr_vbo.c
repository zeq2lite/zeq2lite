/*
 * tr_vbo.c
 * vertex buffer objects
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

#include "tr_local.h"

static ibo_t *iboList;
static vbo_t *vboList;

/*
 * R_AllocIBO
 * Allocate index buffer memory
 */
static ibo_t *R_AllocIBO(void) {
	ibo_t	*ibo;

	ibo = ri.Hunk_Alloc(sizeof(*ibo), h_low);
	Com_Memset(ibo, 0, sizeof(*ibo));

	if (iboList)
		ibo->next = iboList;

	iboList = ibo;

	return ibo;
}

/*
 * R_AllocVBO
 * Allocate vertex buffer memory
 */
static vbo_t *R_AllocVBO(void) {
	vbo_t	*vbo;

	vbo = ri.Hunk_Alloc(sizeof(*vbo), h_low);
	Com_Memset(vbo, 0, sizeof(*vbo));

	if (vboList)
		vbo->next = vboList;

	vboList = vbo;

	return vbo;
}

/*
 * R_CreateIBO
 * Create index buffer object
 */
ibo_t *R_CreateIBO(int numIndexes, int usage) {
	ibo_t	*ibo;

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	/* allocate a new ibo_t */
	if ((ibo = R_AllocIBO()) == NULL)
		ri.Error(ERR_DROP, "R_CreateIBO: R_AllocIBO() failed\n");

	/* set size */
	ibo->maxIndexes	= numIndexes;
	ibo->size = numIndexes * sizeof(glIndex_t);

	/* generate vertex buffer object */
	qglGenBuffersARB(1, &ibo->ibo);

	/* allocate vertex buffer object */
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->ibo);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->size, NULL, usage);
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	/* check for errors */
	GL_CheckErrors();

	return ibo;
}

/*
 * R_CreateVBO
 * Create vertex buffer object
 */
vbo_t *R_CreateVBO(int numVertexes, byte attributes, int usage) {
	vbo_t	*vbo;

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	/* allocate a new vbo_t */
	if ((vbo = R_AllocVBO()) == NULL)
		ri.Error(ERR_DROP, "R_CreateVBO: R_AllocVBO() failed\n");

	/* set attributes */
	vbo->attributes = attributes;

	/* set size and offsets */
	vbo->maxVertexes = numVertexes;
	vbo->size = 0;

	/* vertex position */
	if (vbo->attributes & ATTRIBUTE_POSITION) {
		vbo->offsetXYZ = vbo->size;
		vbo->size += numVertexes * sizeof(vec4_t);
	}

	/* vertex normal */
	if (vbo->attributes & ATTRIBUTE_NORMAL) {
		vbo->offsetNormals = vbo->size;
		vbo->size += numVertexes * sizeof(vec4_t);
	}

	/* vertex color */
	if (vbo->attributes & ATTRIBUTE_COLOR) {
		vbo->offsetColors = vbo->size;
		vbo->size += numVertexes * sizeof(color4ub_t);
	}

	/* vertex texture coordinates */
	if (vbo->attributes & ATTRIBUTE_TEXCOORD0) {
		vbo->offsetTexCoords[0] = vbo->size;
		vbo->size += numVertexes * sizeof(vec2_t);
	}

	/* vertex texture coordinates */
	if (vbo->attributes & ATTRIBUTE_TEXCOORD1) {
		vbo->offsetTexCoords[1] = vbo->size;
		vbo->size += numVertexes * sizeof(vec2_t);
	}

	/* vertex tangent */
	if (vbo->attributes & ATTRIBUTE_TANGENT) {
		vbo->offsetTangents = vbo->size;
		vbo->size += numVertexes * sizeof(vec4_t);
	}

	/* vertex binormal */
	if (vbo->attributes & ATTRIBUTE_BINORMAL) {
		vbo->offsetBinormals = vbo->size;
		vbo->size += numVertexes * sizeof(vec4_t);
	}

	/* generate vertex buffer object */
	qglGenBuffersARB(1, &vbo->vbo);

	/* allocate vertex buffer object */
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo->vbo);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vbo->size, NULL, usage);
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);

	/* check for errors */
	GL_CheckErrors();

	return vbo;
}

/*
 * R_UpdateIBO
 * Upload index data to index buffer object
 */
void R_UpdateIBO(glIndex_t *indexes, int numIndexes) {
	/* indexes */
	qglBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, numIndexes * sizeof(glIndex_t), indexes);

	/* check for errors */
	GL_CheckErrors();
}

/*
 * R_UpdateVBO
 * Upload vertex data to vertex buffer object
 */
void R_UpdateVBO(vec4_t *xyz, vec4_t *normals, color4ub_t *colors, vec2_t *texCoords0, vec2_t *texCoords1, vec4_t *tangents, vec4_t *binormals, int numVertexes) {
	/* vertex position */
	if (glState.currentVBO->attributes & ATTRIBUTE_POSITION)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetXYZ, numVertexes * sizeof(vec4_t), xyz);

	/* vertex normal */
	if (glState.currentVBO->attributes & ATTRIBUTE_NORMAL)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetNormals, numVertexes * sizeof(vec4_t), normals);

	/* vertex color */
	if (glState.currentVBO->attributes & ATTRIBUTE_COLOR)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetColors, numVertexes * sizeof(color4ub_t), colors);

	/* vertex texture coordinate 0 */
	if (glState.currentVBO->attributes & ATTRIBUTE_TEXCOORD0)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetTexCoords[0], numVertexes * sizeof(vec2_t), texCoords0);

	/* vertex texture coordinates */
	if (glState.currentVBO->attributes & ATTRIBUTE_TEXCOORD1)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetTexCoords[1], numVertexes * sizeof(vec2_t), texCoords1);

	/* vertex tangent */
	if (glState.currentVBO->attributes & ATTRIBUTE_TANGENT)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetTangents, numVertexes * sizeof(vec4_t), tangents);

	/* vertex binormal */
	if (glState.currentVBO->attributes & ATTRIBUTE_BINORMAL)
		qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, glState.currentVBO->offsetNormals, numVertexes * sizeof(vec4_t), binormals);

	/* check for errors */
	GL_CheckErrors();
}

/*
 * R_BindIBO
 * Bind index buffer object
 */
void R_BindIBO(ibo_t *ibo) {
	/* prevent redundant calls */
	if (ibo == glState.currentIBO)
		return;

	/* activate vertex buffer object */
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ibo->ibo);
	glState.currentIBO = ibo;

	/* check for errors */
	GL_CheckErrors();
}

/*
 * R_BindVBO
 * Bind vertex buffer object
 */
void R_BindVBO(vbo_t *vbo) {
	/* prevent redundant calls */
	if (vbo == glState.currentVBO)
		return;

	/* bind vertex buffer object */
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vbo->vbo);
	glState.currentVBO = vbo;

	/* check for errors */
	GL_CheckErrors();
}

/*
 * R_UnbindIBO
 * Unbind index buffer object
 */
void R_UnbindIBO(void) {
	/* prevent redundant calls */
	if (!glState.currentIBO)
		return;

	/* unbind index buffer object */
	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	glState.currentIBO = NULL;

	/* check for errors */
	GL_CheckErrors();
}

/*
 * R_UnbindVBO
 * Unbind vertex buffer object
 */
void R_UnbindVBO(void) {
	/* prevent redundant calls */
	if (!glState.currentVBO)
		return;

	/* unbind vertex buffer object */
	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glState.currentVBO = NULL;

	/* check for errors */
	GL_CheckErrors();
}

/*
 * R_InitVBOs
 * Create vertex buffer objects
 */
void R_InitVBOs(void) {
	ri.Printf(PRINT_ALL, "Initializing Vertex Buffer Objects\n");

	/* clear buffer lists */
	iboList = NULL;
	vboList = NULL;

	/* create default buffers */
	tr.defaultIBO = R_CreateIBO(SHADER_MAX_INDEXES, GL_DYNAMIC_DRAW_ARB);
	tr.defaultVBO = R_CreateVBO(SHADER_MAX_VERTEXES, ATTRIBUTE_POSITION | ATTRIBUTE_NORMAL | ATTRIBUTE_COLOR | ATTRIBUTE_TEXCOORD0 | ATTRIBUTE_TEXCOORD1, GL_DYNAMIC_DRAW_ARB);
}

/*
 * R_DeleteVBOs
 * Clean up vertex buffer objects
 */
void R_DeleteVBOs(void) {
	ibo_t	*ibo;
	vbo_t	*vbo;

	R_UnbindIBO();
	R_UnbindVBO();

	for (ibo = iboList; ibo; ibo = ibo->next)
		qglDeleteBuffersARB(1, (const GLuint *)&ibo->ibo);

	for (vbo = vboList; vbo; vbo = vbo->next)
		qglDeleteBuffersARB(1, (const GLuint *)&vbo->vbo);
}
