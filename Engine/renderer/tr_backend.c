/*
 * tr_backend.c
 * Render backend
 * Copyright (C) 1999-2005  Id Software, Inc.
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

backEndData_t	*backEndData[SMP_FRAMES];
backEndState_t	backEnd;

/*
 * GL_Bind
 * Bind image to active texture unit
 */
void GL_Bind(image_t *image) {
	int texnum;

	if (!image) {
		ri.Printf(PRINT_WARNING, "GL_Bind: NULL image\n");
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if (glState.currentTextures[glState.currentTMU] != texnum) {
		image->frameUsed = tr.frameCount;
		glState.currentTextures[glState.currentTMU] = texnum;

		qglBindTexture(image->type, texnum);
	}
}

/*
 * GL_BindAnimatedImage
 * Bind animated image file
 */
void GL_BindAnimatedImage(shaderStage_t *stage) {
	int	index;

	if (stage->isVideoMap) {
		ri.CIN_RunCinematic(stage->videoMapHandle);
		ri.CIN_UploadCinematic(stage->videoMapHandle);
		return;
	}

	if (stage->numImageAnimations <= 1) {
		GL_Bind(stage->image[0]);
		return;
	}

	/*
	 * it is necessary to do this messy calc to make sure animations line up
	 * exactly with waveforms of the same frequency
	 */
	index = myftol(tess.shaderTime * stage->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if (index < 0)
		index = 0;

	index %= stage->numImageAnimations;

	GL_Bind(stage->image[index]);
}

/*
 * GL_SelectTexture
 * Switch active texture unit
 */
void GL_SelectTexture(int unit) {
	if (glState.currentTMU == unit)
		return;

	if (unit < glConfig.numTextureUnits)
		qglActiveTextureARB(GL_TEXTURE0_ARB + unit);
	else
		ri.Error(ERR_DROP, "GL_SelectTexture: unit = %i", unit);

	glState.currentTMU = unit;
}


/*
 * GL_Cull
 * Switch culling order
 */
void GL_Cull(int cullType) {
	if (glState.faceCulling == cullType)
		return;

	glState.faceCulling = cullType;

	if (cullType == CT_TWO_SIDED) {
		qglDisable(GL_CULL_FACE);
	} else {
		qglEnable(GL_CULL_FACE);

		if (cullType == CT_BACK_SIDED) {
			if (backEnd.viewParms.isMirror) {
				qglCullFace(GL_FRONT);
			} else {
				qglCullFace(GL_BACK);
			}
		} else {
			if (backEnd.viewParms.isMirror) {
				qglCullFace(GL_BACK);
			} else {
				qglCullFace(GL_FRONT);
			}
		}
	}
}

/*
 * GL_State
 * Set most commonly changed states
 */
void GL_State(unsigned long stateBits) {
	unsigned long diff = stateBits ^ glState.glStateBits;

	/* no changes */
	if (!diff)
		return;

	/* check depthFunc bits */
	if (diff & GLS_DEPTHFUNC_EQUAL) {
		if (stateBits & GLS_DEPTHFUNC_EQUAL) {
			qglDepthFunc(GL_EQUAL);
		} else {
			qglDepthFunc(GL_LEQUAL);
		}
	}

	/* check blend bits */
	if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) {
		GLenum srcFactor, dstFactor;

		if (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) {
			switch (stateBits & GLS_SRCBLEND_BITS) {
				case GLS_SRCBLEND_ZERO:
					srcFactor = GL_ZERO;
					break;
				case GLS_SRCBLEND_ONE:
					srcFactor = GL_ONE;
					break;
				case GLS_SRCBLEND_DST_COLOR:
					srcFactor = GL_DST_COLOR;
					break;
				case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
					srcFactor = GL_ONE_MINUS_DST_COLOR;
					break;
				case GLS_SRCBLEND_SRC_ALPHA:
					srcFactor = GL_SRC_ALPHA;
					break;
				case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
					srcFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case GLS_SRCBLEND_DST_ALPHA:
					srcFactor = GL_DST_ALPHA;
					break;
				case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
					srcFactor = GL_ONE_MINUS_DST_ALPHA;
					break;
				case GLS_SRCBLEND_ALPHA_SATURATE:
					srcFactor = GL_SRC_ALPHA_SATURATE;
					break;
				default:
					srcFactor = GL_ONE;
					ri.Error(ERR_DROP, "GL_State: invalid src blend state bits\n");
			}

			switch (stateBits & GLS_DSTBLEND_BITS) {
				case GLS_DSTBLEND_ZERO:
					dstFactor = GL_ZERO;
					break;
				case GLS_DSTBLEND_ONE:
					dstFactor = GL_ONE;
					break;
				case GLS_DSTBLEND_SRC_COLOR:
					dstFactor = GL_SRC_COLOR;
					break;
				case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
					dstFactor = GL_ONE_MINUS_SRC_COLOR;
					break;
				case GLS_DSTBLEND_SRC_ALPHA:
					dstFactor = GL_SRC_ALPHA;
					break;
				case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
					dstFactor = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case GLS_DSTBLEND_DST_ALPHA:
					dstFactor = GL_DST_ALPHA;
					break;
				case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
					dstFactor = GL_ONE_MINUS_DST_ALPHA;
					break;
				default:
					dstFactor = GL_ONE;
					ri.Error(ERR_DROP, "GL_State: invalid dst blend state bits\n");
			}

			qglEnable(GL_BLEND);
			qglBlendFunc(srcFactor, dstFactor);
		} else {
			qglDisable(GL_BLEND);
		}
	}

	/* check depthmask */
	if (diff & GLS_DEPTHMASK_TRUE) {
		if (stateBits & GLS_DEPTHMASK_TRUE) {
			qglDepthMask(GL_TRUE);
		} else {
			qglDepthMask(GL_FALSE);
		}
	}

	/* fill/line mode */
	if (diff & GLS_POLYMODE_LINE) {
		if (stateBits & GLS_POLYMODE_LINE) {
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		} else {
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	/* depth test */
	if (diff & GLS_DEPTHTEST_DISABLE) {
		if (stateBits & GLS_DEPTHTEST_DISABLE) {
			qglDisable(GL_DEPTH_TEST);
		} else {
			qglEnable(GL_DEPTH_TEST);
		}
	}

	glState.glStateBits = stateBits;
}

/*
 * GL_VertexAttributeState
 * Set vertex attribute state
 */
void GL_VertexAttributeState(byte attributes) {
	byte diff = attributes ^ glState.attributeStateBits;

	/* vertex buffer attributes don't match program */
	if (attributes & ~glState.currentVBO->attributes)
		ri.Error(ERR_DROP, "GL_VertexAttributeState: Vertex Buffer Object attributes don't match program");

	/* no changes */
	if (!diff)
		return;

	/* vertex position */
	if (diff & ATTRIBUTE_POSITION) {
		if (attributes & ATTRIBUTE_POSITION)
			qglEnableVertexAttribArrayARB(ATTRIBUTE_INDEX_POSITION);
		else
			qglDisableVertexAttribArrayARB(ATTRIBUTE_INDEX_POSITION);
	}

	/* vertex normal */
	if (diff & ATTRIBUTE_NORMAL) {
		if (attributes & ATTRIBUTE_NORMAL)
			qglEnableVertexAttribArrayARB(ATTRIBUTE_INDEX_NORMAL);
		else
			qglDisableVertexAttribArrayARB(ATTRIBUTE_INDEX_NORMAL);
	}

	/* vertex color */
	if (diff & ATTRIBUTE_COLOR) {
		if (attributes & ATTRIBUTE_COLOR)
			qglEnableVertexAttribArrayARB(ATTRIBUTE_INDEX_COLOR);
		else
			qglDisableVertexAttribArrayARB(ATTRIBUTE_INDEX_COLOR);
	}

	/* vertex texture coordinate 0 */
	if (diff & ATTRIBUTE_TEXCOORD0) {
		if (attributes & ATTRIBUTE_TEXCOORD0)
			qglEnableVertexAttribArrayARB(ATTRIBUTE_INDEX_TEXCOORD0);
		else
			qglDisableVertexAttribArrayARB(ATTRIBUTE_INDEX_TEXCOORD0);
	}

	/* vertex texture coordinate 1 */
	if (diff & ATTRIBUTE_TEXCOORD1) {
		if (attributes & ATTRIBUTE_TEXCOORD1)
			qglEnableVertexAttribArrayARB(ATTRIBUTE_INDEX_TEXCOORD1);
		else
			qglDisableVertexAttribArrayARB(ATTRIBUTE_INDEX_TEXCOORD1);
	}

	/* vertex tangent */

	/* vertex binormal */

	glState.attributeStateBits = attributes;
}

/*
 * GL_VertexAttributePointers
 * Set vertex attribute pointers
 */
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
void GL_VertexAttributePointers(byte attributes) {
	/* vertex position */
	if (attributes & ATTRIBUTE_POSITION)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_POSITION, 4, GL_FLOAT, qfalse, 0, BUFFER_OFFSET(glState.currentVBO->offsetXYZ));

	/* vertex normal */
	if (attributes & ATTRIBUTE_NORMAL)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_NORMAL, 4, GL_FLOAT, qfalse, 0, BUFFER_OFFSET(glState.currentVBO->offsetNormals));

	/* vertex color */
	if (attributes & ATTRIBUTE_COLOR)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_COLOR, 4, GL_UNSIGNED_BYTE, qtrue, 0, BUFFER_OFFSET(glState.currentVBO->offsetColors));

	/* vertex texture coordinate 0 */
	if (attributes & ATTRIBUTE_TEXCOORD0)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_TEXCOORD0, 2, GL_FLOAT, qfalse, 0, BUFFER_OFFSET(glState.currentVBO->offsetTexCoords[0]));

	/* vertex texture coordinate 1 */
	if (attributes & ATTRIBUTE_TEXCOORD1)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_TEXCOORD1, 2, GL_FLOAT, qfalse, 0, BUFFER_OFFSET(glState.currentVBO->offsetTexCoords[1]));

	/* vertex tangent */
	if (attributes & ATTRIBUTE_TANGENT)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_TANGENT, 4, GL_FLOAT, qfalse, 0, BUFFER_OFFSET(glState.currentVBO->offsetTangents));

	/* vertex binormal */
	if (attributes & ATTRIBUTE_BINORMAL)
		qglVertexAttribPointerARB(ATTRIBUTE_INDEX_BINORMAL, 4, GL_FLOAT, qfalse, 0, BUFFER_OFFSET(glState.currentVBO->offsetBinormals));
}

/*
 * GL_DrawElements
 * Draw vertex buffer object
 */
void GL_DrawElements(int offsetIndexes, int numIndexes) {
	qglDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(offsetIndexes));
}

/*
 * RB_Hyperspace
 * A player has predicted a teleport, but hasn't arrived yet
 */
static void RB_Hyperspace(void) {
	float c;

	c = (backEnd.refdef.time & 255) / 255.0f;
	qglClearColor(c, c, c, 1);
	qglClear(GL_COLOR_BUFFER_BIT);

	backEnd.isHyperspace = qtrue;
}

/*
 * SetViewportAndScissor
 * Set viewport and scissor
 */
static void SetViewportAndScissor(void) {
	R_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

	/* set the window clipping */
	qglViewport(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
	qglScissor(backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight);
}

/*
 * RB_BeginDrawingView
 * Prepare to render visible surfaces for this view
 */
void RB_BeginDrawingView(void) {
	int clearBits = 0;

	/* sync with gl if needed */
	if (r_finish->integer == 1 && !glState.finishCalled) {
		qglFinish();
		glState.finishCalled = qtrue;
	}

	if (r_finish->integer == 0) {
		glState.finishCalled = qtrue;
	}

	/* we will need to change the projection matrix before drawing 2D images again */
	backEnd.projection2D = qfalse;

	/* set the model view matrix for the viewer */
	SetViewportAndScissor();

	/* ensures that depth writes are enabled for the depth clear */
	GL_State(GLS_DEFAULT);

	/* clear relevant buffers */
	clearBits = GL_DEPTH_BUFFER_BIT;

	if (r_measureOverdraw->integer || r_shadows->integer == 2)
		clearBits |= GL_STENCIL_BUFFER_BIT;

	/* clear the backbuffer each target render */
	if (backEnd.refdef.pixelTarget) {
		clearBits |= GL_COLOR_BUFFER_BIT;
		qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	}

	qglClear(clearBits);

	if ((backEnd.refdef.rdflags & RDF_HYPERSPACE)) {
		RB_Hyperspace();
		return;
	}

	backEnd.isHyperspace = qfalse;

	/* force face culling to set next time */
	glState.faceCulling = -1;

	/* we will only draw a sun if there was sky rendered in this view */
	backEnd.skyRenderedThisView = qfalse;

	/* clip to the plane of the portal */
	if (backEnd.viewParms.isPortal) {
		matrix_t matrix;

		R_MatrixFlip(matrix);
		R_LoadModelViewMatrix(matrix);
	}
}

/*
 * RB_RenderDrawSurfList
 * Render all visible surfaces
 */
void RB_RenderDrawSurfList(drawSurf_t *drawSurfs, int numDrawSurfs) {
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	qboolean		depthRange, oldDepthRange, isCrosshair, wasCrosshair;
	int				i;
	drawSurf_t		*drawSurf;
	int				oldSort;
	float			originalTime;

	/* save original time for entity shader offsets */
	originalTime = backEnd.refdef.floatTime;

	/* clear the z buffer, set the model view, etc */
	RB_BeginDrawingView();

	/* draw everything */
	oldEntityNum			= -1;
	backEnd.currentEntity	= &tr.worldEntity;
	oldShader				= NULL;
	oldFogNum				= -1;
	oldDepthRange			= qfalse;
	wasCrosshair			= qfalse;
	oldSort					= -1;
	depthRange				= qfalse;

	backEnd.pc.c_surfaces	+= numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++) {
		if (drawSurf->sort == oldSort) {
			/* fast path, same as previous sort */
			rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
			continue;
		}

		oldSort = drawSurf->sort;
		R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum);

		/*
		 * change the tess parameters if needed
		 * an "entityMergable" shader is a shader that can have surfaces from separate
		 * entities merged into a single batch, like smoke and blood puff sprites
		 */
		if (shader != oldShader || fogNum != oldFogNum || (entityNum != oldEntityNum && !shader->entityMergable)) {
			if (oldShader != NULL)
				RB_EndSurface();

			RB_BeginSurface(shader, fogNum);

			oldShader	= shader;
			oldFogNum	= fogNum;
		}

		/* change the modelview matrix if needed */
		if (entityNum != oldEntityNum) {
			depthRange = isCrosshair = qfalse;

			if (entityNum != ENTITYNUM_WORLD) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

				/*
				 * we have to reset the shaderTime as well otherwise image animations start
				 * from the wrong frame
				 */
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				/* set up the transformation matrix */
				R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd.or);

				/* hack the depth range to prevent view model from poking into walls */
				if (backEnd.currentEntity->e.renderfx & RF_DEPTHHACK) {
					depthRange = qtrue;
					
					if(backEnd.currentEntity->e.renderfx & RF_CROSSHAIR)
						isCrosshair = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				/*
				 * we have to reset the shaderTime as well otherwise image animations on
				 * the world (like water) continue with the wrong frame
				 */
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
			}

			R_LoadModelViewMatrix(backEnd.or.modelMatrix);

			/*
			 * change depthrange. Also change projection matrix so first person weapon does not look like coming
			 * out of the screen.
			 */
			if (oldDepthRange != depthRange || wasCrosshair != isCrosshair) {
				if (depthRange) {
					if (backEnd.viewParms.stereoFrame != STEREO_CENTER) {
						if (isCrosshair) {
							if (oldDepthRange) {
								/* was not a crosshair but now is, change back proj matrix */
								R_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);
							}
						} else {
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, qfalse);
							R_LoadProjectionMatrix(temp.projectionMatrix);
						}
					}

					if (!oldDepthRange)
						qglDepthRange(0, 0.3);
				} else {
					if (!wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER)
						R_LoadProjectionMatrix(backEnd.viewParms.projectionMatrix);

					qglDepthRange(0, 1);
				}

				oldDepthRange = depthRange;
				wasCrosshair = isCrosshair;
			}

			oldEntityNum = entityNum;
		}

		/* add the triangles for this surface */
		rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
	}

	backEnd.refdef.floatTime = originalTime;

	/* draw the contents of the last shader batch */
	if (oldShader != NULL)
		RB_EndSurface();

	/* go back to the world modelview matrix */
	R_LoadModelViewMatrix(backEnd.viewParms.world.modelMatrix);

	if (depthRange)
		qglDepthRange(0, 1);

	/* darken down any stencil shadows */
	RB_ShadowFinish();

	if (tr.refdef.pixelTarget)
		qglReadPixels(glConfig.vidWidth / 2, glConfig.vidHeight / 2, tr.refdef.pixelTargetWidth, tr.refdef.pixelTargetHeight, GL_RGBA, GL_UNSIGNED_BYTE, tr.refdef.pixelTarget);
}

/*
 * RB_SetGL2D
 * Switch to 2d rendering mode
 */
void RB_SetGL2D(void) {
	matrix_t matrix;

	backEnd.projection2D = qtrue;

	/* set 2D virtual screen size */
	qglViewport(0, 0, glConfig.vidWidth, glConfig.vidHeight);
	qglScissor(0, 0, glConfig.vidWidth, glConfig.vidHeight);

	R_MatrixIdentity(matrix);
	R_LoadModelViewMatrix(matrix);
	R_MatrixOrthogonalProjection(matrix, 0.0f, glConfig.vidWidth, glConfig.vidHeight, 0.0f, 0.0f, 1.0f);
	R_LoadProjectionMatrix(matrix);

	GL_State(GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA);

	qglDisable(GL_CULL_FACE);

	/* set time for 2D shaders */
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}

/*
 * RE_UploadCinematic
 * Upload scratch image
 */
void RE_UploadCinematic(int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	GL_Bind(tr.scratchImage[client]);

	/* if the scratchImage isn't in the format we want, specify it as a new texture */
	if (cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		if (dirty) {
			/*
			 * otherwise, just subimage upload it so that drivers can tell we are going to be changing
			 * it and don't try and do a texture compression
			 */
			qglTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
}

/*
 * RE_StretchRaw
 * Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle
 */
void RE_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	glIndex_t	indexes[6];
	vec4_t		xyz[4];
	vec2_t		texCoords[4];
	int			numIndexes;
	int			numVertexes;
	int			i, j;
	int			start, end;

	if (!tr.registered)
		return;

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	/* we definately want to sync every frame for the cinematics */
	qglFinish();

	start = end = 0;
	if (r_speeds->integer)
		start = ri.Milliseconds();

	/* make sure rows and cols are powers of 2 */
	for (i = 0; (1 << i) < cols; i++);
	for (j = 0; (1 << j) < rows; j++);

	if ((1 << i) != cols || (1 << j) != rows)
		ri.Error(ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);

	if (r_speeds->integer) {
		end = ri.Milliseconds();
		ri.Printf(PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start);
	}

	/* switch to 2d rendering mode */
	RB_SetGL2D();

	/* texture upload */
	RE_UploadCinematic(w, h, cols, rows, data, client, dirty);
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
	R_SetUniform_PortalClipping(tr.defaultProgram, qfalse);

	/* create full-screen quad */
	numIndexes = 0;
	numVertexes = 0;

	/* vertex 1 */
	xyz[numVertexes][0] = x;
	xyz[numVertexes][1] = y;
	xyz[numVertexes][2] = 0;
	xyz[numVertexes][3] = 1;

	texCoords[numVertexes][0] = 0.5f / cols;
	texCoords[numVertexes][1] = 0.5f / rows;

	numVertexes++;

	/* vertex 2 */
	xyz[numVertexes][0] = x + w;
	xyz[numVertexes][1] = y;
	xyz[numVertexes][2] = 0;
	xyz[numVertexes][3] = 1;

	texCoords[numVertexes][0] = (cols - 0.5f) / cols;
	texCoords[numVertexes][1] = 0.5f / rows;

	numVertexes++;

	/* vertex 3 */
	xyz[numVertexes][0] = x + w;
	xyz[numVertexes][1] = y + h;
	xyz[numVertexes][2] = 0;
	xyz[numVertexes][3] = 1;

	texCoords[numVertexes][0] = (cols - 0.5f) / cols;
	texCoords[numVertexes][1] = (rows - 0.5f) / rows;

	numVertexes++;

	/* vertex 4 */
	xyz[numVertexes][0] = x;
	xyz[numVertexes][1] = y + h;
	xyz[numVertexes][2] = 0;
	xyz[numVertexes][3] = 1;

	texCoords[numVertexes][0] = 0.5f / cols;
	texCoords[numVertexes][1] = (rows - 0.5f) / rows;

	numVertexes++;

	/* indexes */
	indexes[numIndexes++] = 0;
	indexes[numIndexes++] = 1;
	indexes[numIndexes++] = 2;
	indexes[numIndexes++] = 0;
	indexes[numIndexes++] = 2;
	indexes[numIndexes++] = 3;

	/* bind vertex buffer object */
	R_BindIBO(tr.defaultIBO);
	R_BindVBO(tr.defaultVBO);

	/* bind vertex attributes */
	GL_VertexAttributeState(tr.defaultProgram->attributes);
	GL_VertexAttributePointers(tr.defaultProgram->attributes);

	/* upload vertex data */
	R_UpdateIBO(indexes, numIndexes);
	R_UpdateVBO(xyz, NULL, NULL, texCoords, texCoords, NULL, NULL, numVertexes);

	/* draw */
	GL_DrawElements(0, numIndexes);
}

/*
 * RB_SetColor
 * Set color
 */
const void *RB_SetColor(const void *data) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
 * RB_StretchPic
 * Draw 2d image
 */
const void *RB_StretchPic(const void *data) {
	const stretchPicCommand_t	*cmd;
	shader_t					*shader;
	int							numVertexes;

	cmd = (const stretchPicCommand_t *)data;

	if (!backEnd.projection2D)
		RB_SetGL2D();

	shader = cmd->shader;
	if (shader != tess.shader) {
		if (tess.numIndexes)
			RB_EndSurface();

		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface(shader, 0);
	}

	RB_CHECKOVERFLOW(4, 6);

	numVertexes	= tess.numVertexes;

	/* vertex 1 */
	tess.xyz[tess.numVertexes][0] = cmd->x;
	tess.xyz[tess.numVertexes][1] = cmd->y;
	tess.xyz[tess.numVertexes][2] = 0.0f;
	tess.xyz[tess.numVertexes][3] = 1.0f;

	*(int *)tess.vertexColors[tess.numVertexes] = *(int *)backEnd.color2D;

	tess.texCoords[0][tess.numVertexes][0] = cmd->s1;
	tess.texCoords[0][tess.numVertexes][1] = cmd->t1;

	tess.numVertexes++;

	/* vertex 2 */
	tess.xyz[tess.numVertexes][0] = cmd->x + cmd->w;
	tess.xyz[tess.numVertexes][1] = cmd->y;
	tess.xyz[tess.numVertexes][2] = 0.0f;
	tess.xyz[tess.numVertexes][3] = 1.0f;

	*(int *)tess.vertexColors[tess.numVertexes] = *(int *)backEnd.color2D;

	tess.texCoords[0][tess.numVertexes][0] = cmd->s2;
	tess.texCoords[0][tess.numVertexes][1] = cmd->t1;

	tess.numVertexes++;

	/* vertex 3 */
	tess.xyz[tess.numVertexes][0] = cmd->x + cmd->w;
	tess.xyz[tess.numVertexes][1] = cmd->y + cmd->h;
	tess.xyz[tess.numVertexes][2] = 0.0f;
	tess.xyz[tess.numVertexes][3] = 1.0f;

	*(int *)tess.vertexColors[tess.numVertexes] = *(int *)backEnd.color2D;

	tess.texCoords[0][tess.numVertexes][0] = cmd->s2;
	tess.texCoords[0][tess.numVertexes][1] = cmd->t2;

	tess.numVertexes++;

	/* vertex 4 */
	tess.xyz[tess.numVertexes][0] = cmd->x;
	tess.xyz[tess.numVertexes][1] = cmd->y + cmd->h;
	tess.xyz[tess.numVertexes][2] = 0.0f;
	tess.xyz[tess.numVertexes][3] = 1.0f;

	*(int *)tess.vertexColors[tess.numVertexes] = *(int *)backEnd.color2D;

	tess.texCoords[0][tess.numVertexes][0] = cmd->s1;
	tess.texCoords[0][tess.numVertexes][1] = cmd->t2;

	tess.numVertexes++;

	/* indexes */
	tess.indexes[tess.numIndexes++]	= numVertexes + 3;
	tess.indexes[tess.numIndexes++]	= numVertexes;
	tess.indexes[tess.numIndexes++]	= numVertexes + 2;
	tess.indexes[tess.numIndexes++]	= numVertexes + 2;
	tess.indexes[tess.numIndexes++]	= numVertexes;
	tess.indexes[tess.numIndexes++]	= numVertexes + 1;

	return (const void *)(cmd + 1);
}


/*
 * RB_DrawSurfs
 * Draw 3d surfaces
*/
const void *RB_DrawSurfs(const void *data) {
	const drawSurfsCommand_t *cmd;

	/* finish any 2D drawing if needed */
	if (tess.numIndexes)
		RB_EndSurface();

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList(cmd->drawSurfs, cmd->numDrawSurfs);

	return (const void *)(cmd + 1);
}


/*
 * RB_DrawBuffer
 * Switch draw buffer
 */
const void *RB_DrawBuffer(const void *data) {
	const drawBufferCommand_t *cmd;

	cmd = (const drawBufferCommand_t *)data;

	qglDrawBuffer(cmd->buffer);

	/* clear screen for debugging */
	if (r_clear->integer) {
		qglClearColor(1.0f, 0.0f, 0.5f, 1.0f);
		qglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	return (const void *)(cmd + 1);
}

/*
 * RB_ShowImages
 * Draw all images to the screen, on top of whatever
 * was there. This is used to test for texture thrashing.
 */
void RB_ShowImages(void) {
	glIndex_t	indexes[6];
	vec4_t		xyz[4];
	vec2_t		texCoords[4];
	int			numIndexes;
	int			numVertexes;
	image_t		*image;
	float		x, y, w, h;
	int			start, end;
	int			i;

	if (!backEnd.projection2D)
		RB_SetGL2D();

	qglClear(GL_COLOR_BUFFER_BIT);

	qglFinish();

	start = ri.Milliseconds();

	for (i = 0; i < tr.numImages; i++) {
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		/* show in proportional size in mode 2 */
		if (r_showImages->integer == 2) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		/* texture upload */
		GL_Bind(image);
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
		R_SetUniform_PortalClipping(tr.defaultProgram, qfalse);

		/* create full-screen quad */
		numIndexes = 0;
		numVertexes = 0;

		/* vertex 1 */
		xyz[numVertexes][0] = x;
		xyz[numVertexes][1] = y;
		xyz[numVertexes][2] = 0;
		xyz[numVertexes][3] = 1;

		texCoords[numVertexes][0] = 0.0f;
		texCoords[numVertexes][1] = 0.0f;

		numVertexes++;

		/* vertex 2 */
		xyz[numVertexes][0] = x + w;
		xyz[numVertexes][1] = y;
		xyz[numVertexes][2] = 0;
		xyz[numVertexes][3] = 1;

		texCoords[numVertexes][0] = 1.0f;
		texCoords[numVertexes][1] = 0.0f;

		numVertexes++;

		/* vertex 3 */
		xyz[numVertexes][0] = x + w;
		xyz[numVertexes][1] = y + h;
		xyz[numVertexes][2] = 0;
		xyz[numVertexes][3] = 1;

		texCoords[numVertexes][0] = 1.0f;
		texCoords[numVertexes][1] = 1.0f;

		numVertexes++;

		/* vertex 4 */
		xyz[numVertexes][0] = x;
		xyz[numVertexes][1] = y + h;
		xyz[numVertexes][2] = 0;
		xyz[numVertexes][3] = 1;

		texCoords[numVertexes][0] = 0.0f;
		texCoords[numVertexes][1] = 1.0f;

		numVertexes++;

		/* indexes */
		indexes[numIndexes++] = 0;
		indexes[numIndexes++] = 1;
		indexes[numIndexes++] = 2;
		indexes[numIndexes++] = 0;
		indexes[numIndexes++] = 2;
		indexes[numIndexes++] = 3;

		/* bind vertex buffer object */
		R_BindIBO(tr.defaultIBO);
		R_BindVBO(tr.defaultVBO);

		/* bind vertex attributes */
		GL_VertexAttributeState(tr.defaultProgram->attributes);
		GL_VertexAttributePointers(tr.defaultProgram->attributes);

		/* upload vertex data */
		R_UpdateIBO(indexes, numIndexes);
		R_UpdateVBO(xyz, NULL, NULL, texCoords, texCoords, NULL, NULL, numVertexes);

		/* draw */
		GL_DrawElements(0, numIndexes);
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf(PRINT_ALL, "%i msec to draw all images\n", end - start);

}

/*
 * RB_ColorMask
 * Set color mask
 */
const void *RB_ColorMask(const void *data) {
	const colorMaskCommand_t *cmd = data;
	
	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}

/*
 * RB_ClearDepth
 * Clear depth buffer
 */
const void *RB_ClearDepth(const void *data) {
	const clearDepthCommand_t *cmd = data;
	
	if(tess.numIndexes)
		RB_EndSurface();

	/* texture swapping test */
	if (r_showImages->integer)
		RB_ShowImages();

	qglClear(GL_DEPTH_BUFFER_BIT);
	
	return (const void *)(cmd + 1);
}

/*
 * RB_SwapBuffers
 * Finish frame and swap buffers
 */
const void *RB_SwapBuffers(const void *data) {
	const swapBuffersCommand_t	*cmd;

	/* finish any 2D drawing if needed */
	if (tess.numIndexes)
		RB_EndSurface();

	/* texture swapping test */
	if (r_showImages->integer)
		RB_ShowImages();

	cmd = (const swapBuffersCommand_t *)data;

	/*
	 * we measure overdraw by reading back the stencil buffer and
	 * counting up the number of increments that have happened
	 */
	if (r_measureOverdraw->integer) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory(glConfig.vidWidth * glConfig.vidHeight);
		qglReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback);

		for (i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++)
			sum += stencilReadback[i];

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory(stencilReadback);
	}


	if (!glState.finishCalled)
		qglFinish();

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

/*
 * RB_ExecuteRenderCommands
 * This function will be called synchronously if running without
 * smp extensions, or asynchronously by another thread.
 */
void RB_ExecuteRenderCommands(const void *data) {
	int		t1, t2;

	t1 = ri.Milliseconds();

	if (!r_smp->integer || data == backEndData[0]->commands.cmds)
		backEnd.smpFrame = 0;
	else
		backEnd.smpFrame = 1;

	while (1) {
		switch (*(const int *)data) {
			case RC_SET_COLOR:
				data = RB_SetColor(data);
				break;
			case RC_STRETCH_PIC:
				data = RB_StretchPic(data);
				break;
			case RC_DRAW_SURFS:
				data = RB_DrawSurfs(data);
				break;
			case RC_DRAW_BUFFER:
				data = RB_DrawBuffer(data);
				break;
			case RC_SWAP_BUFFERS:
				data = RB_SwapBuffers(data);
				break;
			case RC_SCREENSHOT:
				data = RB_TakeScreenshotCmd(data);
				break;
			case RC_VIDEOFRAME:
				data = RB_TakeVideoFrameCmd(data);
				break;
			case RC_COLORMASK:
				data = RB_ColorMask(data);
				break;
			case RC_CLEARDEPTH:
				data = RB_ClearDepth(data);
				break;
			case RC_END_OF_LIST:
			default:
				/* stop rendering on this thread */
				t2 = ri.Milliseconds();
				backEnd.pc.msec = t2 - t1;
				return;
		}
	}
}

/*
 * RB_RenderThread
 * Run render thread
 */
void RB_RenderThread(void) {
	const void	*data;

	/* wait for either a rendering command or a quit command */
	while (1) {
		/* sleep until we have work to do */
		data = GLimp_RendererSleep();

		/* all done, renderer is shutting down */
		if (!data)
			return;

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands(data);

		renderThreadActive = qfalse;
	}
}
