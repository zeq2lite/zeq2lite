/*
 * tr_bsp.c
 * idTech 3 bsp map support
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

/* mapped buffer pointers */
static void	*vertexBuffer;
static void	*indexBuffer;
static int offsetVertexes;
static int offsetIndexes;

/* world file pointer */
static byte *fileBase;

/* world in-memory representation */
static world_t s_worldData;

/* cubemap probes */
static cubeProbe_t *cubeProbeList;
static int numCubeProbes;

/*
 * R_LoadShaders
 * Load surface shaders
 */
static void R_LoadShaders(lump_t *l) {
	dshader_t	*in, *out;
	int			i, count;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error(ERR_DROP, "R_LoadShaders: funny lump size in %s", s_worldData.name);

	count	= l->filelen / sizeof(*in);
	out		= ri.Hunk_Alloc(count * sizeof(*out), h_low);

	s_worldData.shaders		= out;
	s_worldData.numShaders	= count;

	Com_Memcpy(out, in, count * sizeof(*out));

	for (i = 0; i < count; i++) {
		out[i].surfaceFlags	= LittleLong(out[i].surfaceFlags);
		out[i].contentFlags	= LittleLong(out[i].contentFlags);
	}
}

/*
 * CheckDeluxeMapping
 * Check if deluxe mapping is used
 */
static qboolean CheckDeluxeMapping(lump_t *surfs) {
	dsurface_t	*in;
	int			count;
	int			i;

	if (tr.numLightmaps < 2 || tr.numLightmaps & 1)
		return qfalse;

	in		= (void *)(fileBase + surfs->fileofs);
	count	= surfs->filelen / sizeof(*in);

	/*
	 * since light and deluxemaps are interleaved
	 * a surface couldn't possibly have an odd lightmap
	 * if deluxemaps are used
	 */
	for (i = 0; i < count; i++, in++) {
		if ((LittleLong(in->lightmapNum) & 1) && LittleLong(in->lightmapNum) > 0) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
 * R_LoadLightmaps
 * Convert built-in light maps to single large one
 */
#define LIGHTMAP_SIZE	128
static void R_LoadLightmaps(lump_t *l, lump_t *surfs) {
	byte		*buffer, *buffer_p;
	int			length;
	byte		*lightmap;
	byte		*deluxemap = NULL;
	int			h, v;
	int			x, y;
	int			index;
	int			i;

	length = l->filelen;
	if (!length)
		return;

	buffer = fileBase + l->fileofs;

	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	/* get number of lightmaps */
	tr.numLightmaps = length / (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3);

	if (tr.numLightmaps >= MAX_LIGHTMAPS) {
		ri.Printf(PRINT_WARNING, "WARNING: number of lightmaps > MAX_LIGHTMAPS\n");
		tr.numLightmaps = MAX_LIGHTMAPS;
	}

	/* check for deluxe mapping */
	if (CheckDeluxeMapping(surfs)) {
		/* calculate lightmap size */
		for (tr.lightmapSize = 1; tr.lightmapSize < sqrt(tr.numLightmaps / 2) * LIGHTMAP_SIZE; tr.lightmapSize <<= 1);

		/* allocate lightmap buffer */
		lightmap = ri.Hunk_AllocateTempMemory(tr.lightmapSize * tr.lightmapSize * 4 * 2);

		/* clear lightmap buffer */
		Com_Memset(lightmap, 0, tr.lightmapSize * tr.lightmapSize * 4 * 2);

		/* offset deluxemap buffer */
		deluxemap = lightmap + sizeof(char) * tr.lightmapSize * tr.lightmapSize * 4;

		tr.deluxeMapping = qtrue;
	} else {
		/* calculate lightmap size */
		for (tr.lightmapSize = 1; tr.lightmapSize < sqrt(tr.numLightmaps) * LIGHTMAP_SIZE; tr.lightmapSize <<= 1);

		/* allocate lightmap buffer */
		lightmap = ri.Hunk_AllocateTempMemory(tr.lightmapSize * tr.lightmapSize * 4);

		/* clear lightmap buffer */
		Com_Memset(lightmap, 0, tr.lightmapSize * tr.lightmapSize * 4);
	}

	/* load maps */
	for (i = 0; i < tr.numLightmaps; i++) {
		buffer_p = buffer + i * LIGHTMAP_SIZE * LIGHTMAP_SIZE * 3;

		if (tr.deluxeMapping) {
			h = (i / 2) % (tr.lightmapSize / LIGHTMAP_SIZE);
			v = (i / 2) / (tr.lightmapSize / LIGHTMAP_SIZE);

			/* expand the 24 bit on-disk to 32 bit */
			for (y = 0; y < LIGHTMAP_SIZE; y++) {
				for (x = 0; x < LIGHTMAP_SIZE; x++) {
					index = ((x + (y * tr.lightmapSize)) + ((h * LIGHTMAP_SIZE) + (v * tr.lightmapSize * LIGHTMAP_SIZE))) * 4;

					/* deluxemap */
					if (i % 2) {
						deluxemap[index    ] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3)    ];
						deluxemap[index + 1] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3) + 1];
						deluxemap[index + 2] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3) + 2];
						deluxemap[index + 3] = 255;
					/* lightmap */
					} else {
						lightmap[index    ] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3)    ];
						lightmap[index + 1] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3) + 1];
						lightmap[index + 2] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3) + 2];
						lightmap[index + 3] = 255;
					}
				}
			}
		} else {
			h = i % (tr.lightmapSize / LIGHTMAP_SIZE);
			v = i / (tr.lightmapSize / LIGHTMAP_SIZE);

			/* expand the 24 bit on-disk to 32 bit */
			for (y = 0; y < LIGHTMAP_SIZE; y++) {
				for (x = 0; x < LIGHTMAP_SIZE; x++) {
					index = ((x + (y * tr.lightmapSize)) + ((h * LIGHTMAP_SIZE) + (v * tr.lightmapSize * LIGHTMAP_SIZE))) * 4;
					lightmap[index    ] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3)    ];
					lightmap[index + 1] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3) + 1];
					lightmap[index + 2] = buffer_p[((x + (y * LIGHTMAP_SIZE)) * 3) + 2];
					lightmap[index + 3] = 255;
				}
			}
		}
	}

	/* create the lightmap */
	tr.lightmaps[0] = R_CreateImage("*lightmap", lightmap, tr.lightmapSize, tr.lightmapSize, qfalse, qfalse, GL_CLAMP_TO_EDGE);

	/* create the deluxemap */
	if (tr.deluxeMapping)
		tr.deluxemaps[0] = R_CreateImage("*deluxemap", deluxemap, tr.lightmapSize, tr.lightmapSize, qfalse, qfalse, GL_CLAMP_TO_EDGE);

	/* free lightmap buffers */
	ri.Hunk_FreeTempMemory(lightmap);
}

/*
 * R_LoadPlanes
 * Load plane data
 */
static void R_LoadPlanes(lump_t *l) {
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error(ERR_DROP, "R_LoadPlanes: funny lump size in %s", s_worldData.name);

	count	= l->filelen / sizeof(*in);
	out		= ri.Hunk_Alloc (count * 2 * sizeof(*out), h_low);

	s_worldData.planes		= out;
	s_worldData.numplanes	= count;

	for (i = 0; i < count; i++, in++, out++) {
		bits = 0;

		for (j = 0; j < 3; j++) {
			out->normal[j] = LittleFloat(in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1 << j;
		}

		out->dist		= LittleFloat(in->dist);
		out->type		= PlaneTypeForNormal(out->normal);
		out->signbits	= bits;
	}
}

/*
=================
R_LoadFogs

=================
*/
static void R_LoadFogs(lump_t *l, lump_t *brushesLump, lump_t *sidesLump) {
	int				i;
	fog_t			*out;
	dfog_t			*fogs;
	dbrush_t 		*brushes, *brush;
	dbrushside_t	*sides;
	int				count, brushesCount, sidesCount;
	int				sideNum;
	int				planeNum;
	shader_t		*shader;
	float			d;
	int				firstSide;

	fogs = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*fogs))
		ri.Error(ERR_DROP, "R_LoadFogs: funny lump size in %s", s_worldData.name);

	count = l->filelen / sizeof(*fogs);

	/* create fog strucutres for them */
	s_worldData.numfogs = count + 1;
	s_worldData.fogs = ri.Hunk_Alloc(s_worldData.numfogs * sizeof(*out), h_low);
	out = s_worldData.fogs + 1;

	if (!count)
		return;

	brushes = (void *)(fileBase + brushesLump->fileofs);
	if (brushesLump->filelen % sizeof(*brushes))
		ri.Error(ERR_DROP, "R_LoadFogs: funny lump size in %s", s_worldData.name);

	brushesCount = brushesLump->filelen / sizeof(*brushes);

	sides = (void *)(fileBase + sidesLump->fileofs);
	if (sidesLump->filelen % sizeof(*sides))
		ri.Error(ERR_DROP, "R_LoadFogs: funny lump size in %s", s_worldData.name);

	sidesCount = sidesLump->filelen / sizeof(*sides);

	for (i = 0; i < count; i++, fogs++) {
		out->originalBrushNumber = LittleLong(fogs->brushNum);

		if ((unsigned)out->originalBrushNumber >= brushesCount)
			ri.Error(ERR_DROP, "fog brushNumber out of range");

		brush = brushes + out->originalBrushNumber;

		firstSide = LittleLong(brush->firstSide);

		if ((unsigned)firstSide > sidesCount - 6)
			ri.Error(ERR_DROP, "fog brush sideNumber out of range");

		/* brushes are always sorted with the axial sides first */
		sideNum				= firstSide + 0;
		planeNum			= LittleLong(sides[sideNum].planeNum);
		out->bounds[0][0]	= -s_worldData.planes[planeNum].dist;

		sideNum				= firstSide + 1;
		planeNum			= LittleLong(sides[sideNum].planeNum);
		out->bounds[1][0]	= s_worldData.planes[planeNum].dist;

		sideNum				= firstSide + 2;
		planeNum			= LittleLong(sides[sideNum].planeNum);
		out->bounds[0][1]	= -s_worldData.planes[planeNum].dist;

		sideNum				= firstSide + 3;
		planeNum			= LittleLong(sides[sideNum].planeNum);
		out->bounds[1][1]	= s_worldData.planes[planeNum].dist;

		sideNum				= firstSide + 4;
		planeNum			= LittleLong(sides[sideNum].planeNum);
		out->bounds[0][2]	= -s_worldData.planes[planeNum].dist;

		sideNum				= firstSide + 5;
		planeNum			= LittleLong(sides[sideNum].planeNum);
		out->bounds[1][2]	= s_worldData.planes[planeNum].dist;

		/* get information from the shader for fog parameters */
		shader = R_FindShader(fogs->shader, LIGHTMAP_NONE, qtrue);

		out->parms = shader->fogParms;

		out->colorInt = ColorBytes4(shader->fogParms.color[0] * tr.identityLight,
									shader->fogParms.color[1] * tr.identityLight,
									shader->fogParms.color[2] * tr.identityLight, 1.0);

		d = shader->fogParms.depthForOpaque < 1 ? 1 : shader->fogParms.depthForOpaque;
		out->tcScale = 1.0f / (d * 8);

		/* set the gradient vector */
		sideNum = LittleLong(fogs->visibleSide);

		if (sideNum == -1) {
			out->hasSurface = qfalse;
		} else {
			out->hasSurface = qtrue;
			planeNum = LittleLong(sides[firstSide + sideNum].planeNum);
			VectorSubtract(vec3_origin, s_worldData.planes[planeNum].normal, out->surface);
			out->surface[3] = -s_worldData.planes[planeNum].dist;
		}

		out++;
	}
}

/*
 * Transpose
 * Transpose patch mesh
 */
static void Transpose(int width, int height, vec3_t xyz[MAX_GRID_SIZE][MAX_GRID_SIZE]) {
	int		i, j;
	vec3_t	temp;

	if (width > height) {
		for (i = 0; i < height; i++) {
			for (j = i + 1; j < width; j++) {
				if (j < height) {
					/* swap the value */
					VectorCopy(xyz[j][i], temp);
					VectorCopy(xyz[i][j], xyz[j][i]);
					VectorCopy(temp, xyz[i][j]);
				} else {
					/* just copy */
					VectorCopy(xyz[i][j], xyz[j][i]);
				}
			}
		}
	} else {
		for (i = 0; i < width; i++) {
			for (j = i + 1; j < height; j++) {
				if (j < width) {
					/* swap the value */
					VectorCopy(xyz[i][j], temp);
					VectorCopy(xyz[j][i], xyz[i][j]);
					VectorCopy(temp, xyz[j][i]);
				} else {
					/* just copy */
					VectorCopy(xyz[j][i], xyz[i][j]);
				}
			}
		}
	}
}

/*
 * CalculateGridSize
 * Calculate final size of patch mesh after subdivisions
 */
static void CalculateGridSize(drawVert_t *dv, int width, int height, int *outWidth, int *outHeight) {
	int		i, j, l, t;
	float	len, maxLen;
	int		dir;
	vec3_t	xyz[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float	errorTable[2][MAX_GRID_SIZE];

	for (i = 0; i < width; i++) {
		for (j = 0; j < height; j++) {
			xyz[j][i][0] =  LittleFloat(dv[j * width + i].xyz[0]);
			xyz[j][i][1] =  LittleFloat(dv[j * width + i].xyz[1]);
			xyz[j][i][2] =  LittleFloat(dv[j * width + i].xyz[2]);
		}
	}

	for (dir = 0; dir < 2; dir++) {
		for (j = 0; j < MAX_GRID_SIZE; j++)
			errorTable[dir][j] = 0;

		/* horizontal subdivisions */
		for (j = 0; j + 2 < width; j += 2) {
			/* check subdivided midpoints against control points */
			maxLen = 0;
			for (i = 0; i < height; i++) {
				vec3_t		midxyz;
				vec3_t		midxyz2;
				vec3_t		dir;
				vec3_t		projected;
				float		d;

				/* calculate the point on the curve */
				for ( l = 0; l < 3 ; l++)
					midxyz[l] = (xyz[i][j][l] + xyz[i][j + 1][l] * 2 + xyz[i][j + 2][l]) * 0.25f;

				/*
				 * see how far off the line it is
				 * using dist-from-line will not account for internal
				 * texture warping, but it gives a lot less polygons than
				 * dist-from-midpoint
				 */
				VectorSubtract(midxyz, xyz[i][j], midxyz);
				VectorSubtract(xyz[i][j + 2], xyz[i][j], dir);
				VectorNormalize(dir);

				d = DotProduct(midxyz, dir);
				VectorScale(dir, d, projected);
				VectorSubtract(midxyz, projected, midxyz2);
				len = VectorLengthSquared(midxyz2);
				if (len > maxLen)
					maxLen = len;
			}

			maxLen = sqrt(maxLen);

			/* if all the points are on the lines, remove the entire columns */
			if (maxLen < 0.1f) {
				errorTable[dir][j + 1] = 999;
				continue;
			}

			/* can't subdivide any more */
			if (width + 2 > MAX_GRID_SIZE) {
				errorTable[dir][j + 1] = 1.0f / maxLen;
				continue;
			}

			/* doesn't need subdivision */
			if (maxLen <= 1.0) {
				errorTable[dir][j + 1] = 1.0f / maxLen;
				continue;
			}

			errorTable[dir][j + 2] = 1.0f / maxLen;

			/* insert two columns and replace the peak */
			width += 2;

			/* back up and recheck this set again, it may need more subdivision */
			j -= 2;
		}

		Transpose(width, height, xyz);
		t = width;
		width = height;
		height = t;
	}

	/* cull out any columns that are colinear */
	for (i = 1; i < width - 1; i++) {
		if (errorTable[0][i] != 999)
			continue;

		for (j = i + 1; j < width; j++)
			errorTable[0][j - 1] = errorTable[0][j];

		width--;
	}

	/* cull out any rows that are colinear */
	for (i = 1; i < height - 1; i++) {
		if (errorTable[1][i] != 999)
			continue;

		for (j = i + 1; j < height; j++)
			errorTable[1][j-1] = errorTable[1][j];

		height--;
	}

	*outWidth = width;
	*outHeight = height;
}

/*
 * ShaderForShaderNum
 * Get Shader for surface
 */
static shader_t *ShaderForShaderNum(int shaderNum, int lightmapNum) {
	shader_t	*shader;
	dshader_t	*dsh;

	shaderNum = LittleLong(shaderNum);
	if (shaderNum < 0 || shaderNum >= s_worldData.numShaders)
		ri.Error(ERR_DROP, "ShaderForShaderNum: bad num %i", shaderNum);

	dsh = &s_worldData.shaders[shaderNum];

	if (r_fullbright->integer)
		lightmapNum = LIGHTMAP_WHITEIMAGE;

	shader = R_FindShader(dsh->shader, lightmapNum, qtrue);

	/* if the shader had errors, just use default shader */
	if (shader->defaultShader)
		return tr.defaultShader;

	return shader;
}

/*
 * R_LightmapU
 * Get u coordinate for combined lightmap
 */
static float LightmapU(float u, int lightmapNum) {
	if (!tr.lightmapSize)
		return u;

	if (tr.deluxeMapping)
		lightmapNum /= 2;

	return (u / ((float)(tr.lightmapSize / LIGHTMAP_SIZE))) + ((1.0f / ((float)(tr.lightmapSize / LIGHTMAP_SIZE))) * ((float)(lightmapNum % (tr.lightmapSize / LIGHTMAP_SIZE))));
}

/*
 * R_LightmapV
 * Get v coordinate for combined lightmap
 */
static float LightmapV(float v, int lightmapNum) {
	if (!tr.lightmapSize)
		return v;

	if (tr.deluxeMapping)
		lightmapNum /= 2;

	return (v / ((float)(tr.lightmapSize / LIGHTMAP_SIZE))) + ((1.0f / ((float)(tr.lightmapSize / LIGHTMAP_SIZE))) * ((float)(lightmapNum / (tr.lightmapSize / LIGHTMAP_SIZE))));
}

/*
 * SphereFromBounds
 * Creates a bounding sphere from a bounding box
 */
static void SphereFromBounds(vec3_t mins, vec3_t maxs, vec3_t origin, float *radius) {
	vec3_t temp;

	VectorAdd(mins, maxs, origin);
	VectorScale(origin, 0.5, origin);
	VectorSubtract(maxs, origin, temp);
	*radius = VectorLength(temp);
}

/*
 * FinishGenericSurface
 * Handles final surface classification
 */
static void FinishGenericSurface(dsurface_t *ds, srfGeneric_t *surf, vec3_t point) {
	/* set bounding sphere */
	SphereFromBounds(surf->bounds[0], surf->bounds[1], surf->origin, &surf->radius);

	/* take the plane normal from the lightmap vector and classify it */
	surf->plane.normal[0] = LittleFloat(ds->lightmapVecs[2][0]);
	surf->plane.normal[1] = LittleFloat(ds->lightmapVecs[2][1]);
	surf->plane.normal[2] = LittleFloat(ds->lightmapVecs[2][2]);

	surf->plane.dist = DotProduct(point, surf->plane.normal);

	SetPlaneSignbits(&surf->plane);

	surf->plane.type = PlaneTypeForNormal(surf->plane.normal);
}

/*
 * R_ParseMesh
 * Parse curved surface
 */
static void ParseMesh(dsurface_t *ds, drawVert_t *dv, msurface_t *surf) {
	srfGridMesh_t	*grid;
	glIndex_t		*index;
	vec4_t			*position;
	vec4_t			*normal;
	color4ub_t		*color;
	vec2_t			*texCoords;
	vec2_t			*lightCoords;
	vec4_t			*tangent;
	vec4_t			*binormal;
	vec3_t			bounds[2];
	vec3_t			tmp;
	int				v1, v2, v3, v4;
	int				lightmapNum;
	int				numVertexes;
	int				numIndexes;
	int				i;

	lightmapNum = LittleLong(ds->lightmapNum);

	/* get fog volume */
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	/* get shader value */
	surf->shader = ShaderForShaderNum(ds->shaderNum, lightmapNum);
	if (r_singleShader->integer && !surf->shader->isSky)
		surf->shader = tr.defaultShader;

	/* we may have a nodraw surface, because they might still need to be around for movement clipping */
	if (s_worldData.shaders[LittleLong(ds->shaderNum)].surfaceFlags & SURF_NODRAW) {
		static surfaceType_t skip = SF_SKIP;
		surf->data = &skip;
		return;
	}

	/* number of vertexes */
	numVertexes = LittleLong(ds->numVerts);

	/* number of indexes */
	numIndexes = ((LittleLong(ds->patchWidth) - 1) * (LittleLong(ds->patchHeight) - 1) + 1) * 6;

	/* create srfGridMesh_t */
	grid = ri.Hunk_Alloc(sizeof(*grid), h_low);
	grid->surfaceType	= SF_GRID;
	grid->numVertexes	= numVertexes;
	grid->numIndexes	= numIndexes;
	grid->offsetIndexes	= offsetIndexes * sizeof(*index);

	/* current buffer position */
	index		= indexBuffer + offsetIndexes * sizeof(*index);
	position	= vertexBuffer + glState.currentVBO->offsetXYZ + offsetVertexes * sizeof(*position);
	normal		= vertexBuffer + glState.currentVBO->offsetNormals + offsetVertexes * sizeof(*normal);
	color		= vertexBuffer + glState.currentVBO->offsetColors + offsetVertexes * sizeof(*color);
	texCoords	= vertexBuffer + glState.currentVBO->offsetTexCoords[0] + offsetVertexes * sizeof(*texCoords);
	lightCoords	= vertexBuffer + glState.currentVBO->offsetTexCoords[1] + offsetVertexes * sizeof(*lightCoords);
	tangent		= vertexBuffer + glState.currentVBO->offsetTangents + offsetVertexes * sizeof(*tangent);
	binormal	= vertexBuffer + glState.currentVBO->offsetBinormals + offsetVertexes * sizeof(*binormal);

	/* vertexes */
	dv += LittleLong(ds->firstVert);
	for (i = 0; i < numVertexes; i++) {
		/* position */
		position[i][0] = LittleFloat(dv[i].xyz[0]);
		position[i][1] = LittleFloat(dv[i].xyz[1]);
		position[i][2] = LittleFloat(dv[i].xyz[2]);
		position[i][3] = 1.0f;

		/* normal */
		normal[i][0] = LittleFloat(dv[i].normal[0]);
		normal[i][1] = LittleFloat(dv[i].normal[1]);
		normal[i][2] = LittleFloat(dv[i].normal[2]);
		normal[i][3] = 1.0f;

		/* texture coordinates */
		texCoords[i][0] = LittleFloat(dv[i].st[0]);
		texCoords[i][1] = LittleFloat(dv[i].st[1]);

		/* lightmap */
		lightCoords[i][0] = LightmapU(LittleFloat(dv[i].lightmap[0]), lightmapNum);
		lightCoords[i][1] = LightmapV(LittleFloat(dv[i].lightmap[1]), lightmapNum);

		/* color */
		color[i][0] = dv[i].color[0];
		color[i][1] = dv[i].color[1];
		color[i][2] = dv[i].color[2];
		color[i][3] = dv[i].color[3];

		/* tangent */
		tangent[i][0] = LittleFloat(dv[i].normal[0]);
		tangent[i][1] = LittleFloat(dv[i].normal[1]);
		tangent[i][2] = LittleFloat(dv[i].normal[2]);
		tangent[i][3] = 1.0f;

		/* binormal */
		binormal[i][0] = LittleFloat(dv[i].normal[0]);
		binormal[i][1] = LittleFloat(dv[i].normal[1]);
		binormal[i][2] = LittleFloat(dv[i].normal[2]);
		binormal[i][3] = 1.0f;
	}

	/* indexes */
	for (i = 0; i < numIndexes; i++) {
		v1 = i + 1;
		v2 = i;
		v3 = v2 + LittleLong(ds->patchWidth);
		v4 = v3 + 1;

		index[i * 6    ] = v2 + offsetVertexes;
		index[i * 6 + 1] = v3 + offsetVertexes;
		index[i * 6 + 2] = v1 + offsetVertexes;

		index[i * 6 + 3] = v1 + offsetVertexes;
		index[i * 6 + 4] = v3 + offsetVertexes;
		index[i * 6 + 5] = v4 + offsetVertexes;
	}

	/* surface */
	surf->data = (surfaceType_t *)grid;

	/*
	 * copy the level of detail origin, which is the center
	 * of the group of all curves that must subdivide the same
	 * to avoid cracking
	 */
	for (i = 0; i < 3; i++) {
		bounds[0][i] = LittleFloat(ds->lightmapVecs[0][i]);
		bounds[1][i] = LittleFloat(ds->lightmapVecs[1][i]);
	}

	VectorAdd(bounds[0], bounds[1], bounds[1]);
	VectorScale(bounds[1], 0.5f, grid->lodOrigin);
	VectorSubtract(bounds[0], grid->lodOrigin, tmp);
	grid->lodRadius = VectorLength(tmp);

	/* finish surface */
	tmp[0] = position[0][0];
	tmp[1] = position[0][1];
	tmp[2] = position[0][2];
	FinishGenericSurface(ds, (srfGeneric_t*)grid, tmp);

	/* advance buffer offsets */
	offsetVertexes	+= numVertexes;
	offsetIndexes	+= numIndexes;
}

/*
 * R_ParseTriSurf
 * Parse triangle soup
 */
static void ParseTriSurf(dsurface_t *ds, drawVert_t *dv, msurface_t *surf, int *indexes) {
	srfTriangles_t	*tri;
	glIndex_t		*index;
	vec4_t			*position;
	vec4_t			*normal;
	color4ub_t		*color;
	vec2_t			*texCoords;
	vec2_t			*lightCoords;
	vec4_t			*tangent;
	vec4_t			*binormal;
	vec3_t			tmp;
	int				lightmapNum;
	int				numVertexes;
	int				numIndexes;
	int				i;

	lightmapNum = LittleLong(ds->lightmapNum);

	/* get fog volume */
	surf->fogIndex = LittleLong(ds->fogNum) + 1;

	/* get shader value */
	surf->shader = ShaderForShaderNum(ds->shaderNum, lightmapNum);
	if (r_singleShader->integer && !surf->shader->isSky)
		surf->shader = tr.defaultShader;

	/* number of vertexes */
	numVertexes = LittleLong(ds->numVerts);

	/* number of indexes */
	numIndexes = LittleLong(ds->numIndexes);

	/* create srfTriangles_t */
	tri = ri.Hunk_Alloc(sizeof(*tri), h_low);
	tri->surfaceType	= SF_TRIANGLES;
	tri->numVertexes	= numVertexes;
	tri->numIndexes		= numIndexes;
	tri->offsetIndexes	= offsetIndexes * sizeof(*index);
	ClearBounds(tri->bounds[0], tri->bounds[1]);

	/* current buffer position */
	index		= indexBuffer + offsetIndexes * sizeof(*index);
	position	= vertexBuffer + glState.currentVBO->offsetXYZ + offsetVertexes * sizeof(*position);
	normal		= vertexBuffer + glState.currentVBO->offsetNormals + offsetVertexes * sizeof(*normal);
	color		= vertexBuffer + glState.currentVBO->offsetColors + offsetVertexes * sizeof(*color);
	texCoords	= vertexBuffer + glState.currentVBO->offsetTexCoords[0] + offsetVertexes * sizeof(*texCoords);
	lightCoords	= vertexBuffer + glState.currentVBO->offsetTexCoords[1] + offsetVertexes * sizeof(*lightCoords);
	tangent		= vertexBuffer + glState.currentVBO->offsetTangents + offsetVertexes * sizeof(*tangent);
	binormal	= vertexBuffer + glState.currentVBO->offsetBinormals + offsetVertexes * sizeof(*binormal);

	/* vertexes */
	dv += LittleLong(ds->firstVert);
	for (i = 0; i < numVertexes; i++) {
		/* position */
		position[i][0] = tmp[0] = LittleFloat(dv[i].xyz[0]);
		position[i][1] = tmp[1] = LittleFloat(dv[i].xyz[1]);
		position[i][2] = tmp[2] = LittleFloat(dv[i].xyz[2]);
		position[i][3] = 1.0f;

		/* normal */
		normal[i][0] = LittleFloat(dv[i].normal[0]);
		normal[i][1] = LittleFloat(dv[i].normal[1]);
		normal[i][2] = LittleFloat(dv[i].normal[2]);
		normal[i][3] = 1.0f;

		/* texture coordinates */
		texCoords[i][0] = LittleFloat(dv[i].st[0]);
		texCoords[i][1] = LittleFloat(dv[i].st[1]);

		/* lightmap */
		lightCoords[i][0] = LightmapU(LittleFloat(dv[i].lightmap[0]), lightmapNum);
		lightCoords[i][1] = LightmapV(LittleFloat(dv[i].lightmap[1]), lightmapNum);

		/* color */
		color[i][0] = dv[i].color[0];
		color[i][1] = dv[i].color[1];
		color[i][2] = dv[i].color[2];
		color[i][3] = dv[i].color[3];

		/* tangent */
		tangent[i][0] = LittleFloat(dv[i].normal[0]);
		tangent[i][1] = LittleFloat(dv[i].normal[1]);
		tangent[i][2] = LittleFloat(dv[i].normal[2]);
		tangent[i][3] = 1.0f;

		/* binormal */
		binormal[i][0] = LittleFloat(dv[i].normal[0]);
		binormal[i][1] = LittleFloat(dv[i].normal[1]);
		binormal[i][2] = LittleFloat(dv[i].normal[2]);
		binormal[i][3] = 1.0f;

		/* boundaries */
		AddPointToBounds(tmp, tri->bounds[0], tri->bounds[1]);
	}

	/* indexes */
	indexes += LittleLong(ds->firstIndex);
	for (i = 0; i < numIndexes; i++) {
		index[i] = LittleLong(indexes[i]) + offsetVertexes;
		if (LittleLong(indexes[i]) < 0 || LittleLong(indexes[i]) >= numVertexes)
			ri.Error(ERR_DROP, "Bad index in triangle surface");
	}

	/* surface */
	surf->data = (surfaceType_t *)tri;

	/* finish surface */
	tmp[0] = position[0][0];
	tmp[1] = position[0][1];
	tmp[2] = position[0][2];
	FinishGenericSurface(ds, (srfGeneric_t *)tri, tmp);

	/* advance buffer offsets */
	offsetVertexes	+= numVertexes;
	offsetIndexes	+= numIndexes;
}

/*
 * R_ParseFoliage
 * Parse triangle soup
 */
static void ParseFoliage(dsurface_t *ds, drawVert_t *dv, msurface_t *surf, int *indexes) {
	ri.Printf(PRINT_ALL, "Foliage rendering not supported yet.\n");
	// stub
}

/*
 * R_LoadSurfaces
 * Load static map surfaces
 */
static void R_LoadSurfaces(lump_t *surfs, lump_t *verts, lump_t *indexLump) {
	dsurface_t	*in;
	msurface_t	*out;
	drawVert_t	*dv;
	int			*indexes;
	int			count;
	int			numMeshes, numTriSurfs, numFoliage;
	int			numVertexes, numIndexes;
	int			width, height;
	int			i;

	numMeshes	= 0;
	numTriSurfs	= 0;
	numFoliage	= 0;

	numVertexes	= 0;
	numIndexes	= 0;

	in = (void *)(fileBase + surfs->fileofs);
	if (surfs->filelen % sizeof(*in))
		ri.Error(ERR_DROP, "R_LoadSurfaces: funny lump size in %s", s_worldData.name);

	count = surfs->filelen / sizeof(*in);

	dv = (void *)(fileBase + verts->fileofs);
	if (verts->filelen % sizeof(*dv))
		ri.Error(ERR_DROP, "R_LoadSurfaces: funny lump size in %s", s_worldData.name);

	indexes = (void *)(fileBase + indexLump->fileofs);
	if (indexLump->filelen % sizeof(*indexes))
		ri.Error(ERR_DROP, "R_LoadSurfaces: funny lump size in %s", s_worldData.name);

	out = ri.Hunk_Alloc(count * sizeof(*out), h_low);

	s_worldData.surfaces	= out;
	s_worldData.numsurfaces	= count;

	/* calculate buffer size */
	for (i = 0; i < count; i++, in++) {
		switch (LittleLong(in->surfaceType)) {
			case MST_PATCH:
				/* we may have a nodraw surface, because they might still need to be around for movement clipping */
				if (s_worldData.shaders[LittleLong(in->shaderNum)].surfaceFlags & SURF_NODRAW)
					continue;

				width	= LittleLong(in->patchWidth);
				height	= LittleLong(in->patchHeight);

				CalculateGridSize(dv + LittleLong(in->firstVert), width, height, &width, &height);

				numIndexes	+= ((width - 1) * (height - 1) + 1) * 6;
				numVertexes	+= width * height;
				break;
			case MST_TRIANGLE_SOUP:
			case MST_PLANAR:
				numIndexes	+= LittleLong(in->numIndexes);
				numVertexes	+= LittleLong(in->numVerts);
				break;
			case MST_FOLIAGE:
				numIndexes	+= LittleLong(in->numIndexes);
				numVertexes	+= LittleLong(in->patchHeight);
				break;
		}
	}

	/* create buffer objects */
	tr.worldIBO = R_CreateIBO(numIndexes, GL_STATIC_DRAW_ARB);
	tr.worldVBO = R_CreateVBO(numVertexes, ATTRIBUTE_POSITION | ATTRIBUTE_NORMAL | ATTRIBUTE_COLOR | ATTRIBUTE_TEXCOORD0 | ATTRIBUTE_TEXCOORD1 | ATTRIBUTE_TANGENT | ATTRIBUTE_BINORMAL, GL_STATIC_DRAW_ARB);

	/* bind vertex buffer object */
	R_BindIBO(tr.worldIBO);
	R_BindVBO(tr.worldVBO);

	/* map buffer objects */
	indexBuffer		= qglMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
	vertexBuffer	= qglMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB);

	/* parse surfaces */
	for (i = 0, in = (void *)(fileBase + surfs->fileofs); i < count; i++, in++, out++) {
		switch (LittleLong(in->surfaceType)) {
		case MST_PATCH:
			ParseMesh(in, dv, out);
			numMeshes++;
			break;
		case MST_TRIANGLE_SOUP:
		case MST_PLANAR:
			ParseTriSurf(in, dv, out, indexes);
			numTriSurfs++;
			break;
		case MST_FOLIAGE:
			ParseFoliage(in, dv, out, indexes);
			break;
		case MST_FLARE:
			break;
		default:
			ri.Error(ERR_DROP, "Bad surfaceType");
		}
	}

	/* unmap buffer objects */
	qglUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
	qglUnmapBufferARB(GL_ARRAY_BUFFER_ARB);

	ri.Printf(PRINT_ALL, "...loaded %i meshes, %i trisurfs, %i foliage\n", numMeshes, numTriSurfs, numFoliage);
	ri.Printf(PRINT_ALL, "...made up of %i vertices and %i triangles\n", numVertexes, numIndexes / 3);
	ri.Printf(PRINT_ALL, "...total buffer size: %.2f MB\n", (tr.worldIBO->size + tr.worldVBO->size) / 1024.0f / 1024.0f);
}

/*
 * R_LoadMarksurfaces
 * Load mark surfaces
 */
static void R_LoadMarksurfaces(lump_t *l) {
	int			i, j, count;
	int			*in;
	msurface_t	**out;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error(ERR_DROP, "R_LoadMarksurfaces: funny lump size in %s", s_worldData.name);

	count	= l->filelen / sizeof(*in);
	out		= ri.Hunk_Alloc(count * sizeof(*out), h_low);

	s_worldData.marksurfaces	= out;
	s_worldData.nummarksurfaces	= count;

	for (i = 0; i < count; i++) {
		j		= LittleLong(in[i]);
		out[i]	= s_worldData.surfaces + j;
	}
}

/*
 * R_SetParent
 * Set parent node for node
 */
static void R_SetParent(mnode_t *node, mnode_t *parent) {
	node->parent = parent;

	if (node->contents != -1)
		return;

	R_SetParent(node->children[0], node);
	R_SetParent(node->children[1], node);
}

/*
 * R_LoadNodesAndLeafs
 * Load nodes and leafs
 */
static void R_LoadNodesAndLeafs(lump_t *nodeLump, lump_t *leafLump) {
	int			i, j, p;
	dnode_t		*in;
	dleaf_t		*inLeaf;
	mnode_t 	*out;
	int			numNodes, numLeafs;

	in = (void *)(fileBase + nodeLump->fileofs);
	if (nodeLump->filelen % sizeof(*in) || leafLump->filelen % sizeof(*inLeaf))
		ri.Error(ERR_DROP, "R_LoadNodesAndLeafs: funny lump size in %s", s_worldData.name);

	numNodes = nodeLump->filelen / sizeof(*in);
	numLeafs = leafLump->filelen / sizeof(*inLeaf);

	out = ri.Hunk_Alloc((numNodes + numLeafs) * sizeof(*out), h_low);

	s_worldData.nodes				= out;
	s_worldData.numnodes			= numNodes + numLeafs;
	s_worldData.numDecisionNodes	= numNodes;

	// load nodes
	for (i = 0; i < numNodes; i++, in++, out++) {
		for (j = 0; j < 3; j++) {
			out->mins[j] = LittleLong(in->mins[j]);
			out->maxs[j] = LittleLong(in->maxs[j]);
		}

		p = LittleLong(in->planeNum);
		out->plane = s_worldData.planes + p;

		/* differentiate from leafs */
		out->contents = CONTENTS_NODE;

		for (j = 0; j < 2; j++) {
			p = LittleLong (in->children[j]);
			if (p >= 0)
				out->children[j] = s_worldData.nodes + p;
			else
				out->children[j] = s_worldData.nodes + numNodes + (-1 - p);
		}
	}

	/* load leafs */
	inLeaf = (void *)(fileBase + leafLump->fileofs);
	for (i = 0; i < numLeafs; i++, inLeaf++, out++) {
		for (j = 0 ; j < 3; j++) {
			out->mins[j] = LittleLong(inLeaf->mins[j]);
			out->maxs[j] = LittleLong(inLeaf->maxs[j]);
		}

		out->cluster	= LittleLong(inLeaf->cluster);
		out->area		= LittleLong(inLeaf->area);

		if (out->cluster >= s_worldData.numClusters)
			s_worldData.numClusters = out->cluster + 1;


		out->firstmarksurface	= s_worldData.marksurfaces + LittleLong(inLeaf->firstLeafSurface);
		out->nummarksurfaces	= LittleLong(inLeaf->numLeafSurfaces);
	}

	/* chain decendants */
	R_SetParent(s_worldData.nodes, NULL);
}

/*
 * R_LoadSubmodels
 * Load brush models
 */
static void R_LoadSubmodels(lump_t *l) {
	dmodel_t	*in;
	bmodel_t	*out;
	int			i, j, count;

	in = (void *)(fileBase + l->fileofs);
	if (l->filelen % sizeof(*in))
		ri.Error(ERR_DROP, "R_LoadSubmodels: funny lump size in %s", s_worldData.name);

	count = l->filelen / sizeof(*in);

	s_worldData.bmodels = out = ri.Hunk_Alloc(count * sizeof(*out), h_low);

	for (i = 0; i < count; i++, in++, out++) {
		model_t *model;

		/* allocate model */
		model = R_AllocModel();
		if (!model)
			ri.Error(ERR_DROP, "R_LoadSubmodels: R_AllocModel() failed");

		model->type			= MOD_BRUSH;
		model->model.bmodel	= out;

		Com_sprintf(model->name, sizeof(model->name), "*%d", i);

		for (j = 0; j < 3; j++) {
			out->bounds[0][j] = LittleFloat(in->mins[j]);
			out->bounds[1][j] = LittleFloat(in->maxs[j]);
		}

		out->firstSurface	= s_worldData.surfaces + LittleLong(in->firstSurface);
		out->numSurfaces	= LittleLong(in->numSurfaces);
	}
}

/*
 * R_LoadVisibility
 * Load PVS data
 */
static void R_LoadVisibility(lump_t *l) {
	int		length;
	byte	*buffer;

	length = (s_worldData.numClusters + 63) & ~63;
	s_worldData.novis = ri.Hunk_Alloc(length, h_low);
	Com_Memset(s_worldData.novis, 0xff, length);

    length = l->filelen;
	if (!length)
		return;

	buffer = fileBase + l->fileofs;

	s_worldData.numClusters		= LittleLong(((int *)buffer)[0]);
	s_worldData.clusterBytes	= LittleLong(((int *)buffer)[1]);

	/*
	 * CM_Load should have given us the vis data to share, so
	 * we don't need to allocate another copy
	 */
	if (tr.externalVisData) {
		s_worldData.vis = tr.externalVisData;
	} else {
		byte *dest;

		dest = ri.Hunk_Alloc(length - 8, h_low);
		Com_Memcpy(dest, buffer + 8, length - 8);
		s_worldData.vis = dest;
	}
}

/*
 * R_LoadLightGrid
 * Load light grid data
 */
void R_LoadLightGrid(lump_t *l) {
	int		i;
	vec3_t	maxs;
	int		numGridPoints;
	world_t	*w;
	float	*wMins, *wMaxs;

	w = &s_worldData;

	w->lightGridInverseSize[0] = 1.0f / w->lightGridSize[0];
	w->lightGridInverseSize[1] = 1.0f / w->lightGridSize[1];
	w->lightGridInverseSize[2] = 1.0f / w->lightGridSize[2];

	wMins = w->bmodels[0].bounds[0];
	wMaxs = w->bmodels[0].bounds[1];

	for (i = 0; i < 3; i++) {
		w->lightGridOrigin[i] = w->lightGridSize[i] * ceil(wMins[i] / w->lightGridSize[i]);
		maxs[i] = w->lightGridSize[i] * floor(wMaxs[i] / w->lightGridSize[i]);
		w->lightGridBounds[i] = (maxs[i] - w->lightGridOrigin[i]) /w->lightGridSize[i] + 1;
	}

	numGridPoints = w->lightGridBounds[0] * w->lightGridBounds[1] * w->lightGridBounds[2];

	if (l->filelen != numGridPoints * 8) {
		ri.Printf(PRINT_WARNING, "WARNING: light grid mismatch\n");
		w->lightGridData = NULL;
		return;
	}

	w->lightGridData = ri.Hunk_Alloc(l->filelen, h_low);
	Com_Memcpy(w->lightGridData, (void *)(fileBase + l->fileofs), l->filelen);
}

/*
 * R_LoadEntities
 * Load entities
 */
void R_LoadEntities(lump_t *l) {
	char	*p, *token, *s;
	char	keyname[MAX_TOKEN_CHARS];
	char	value[MAX_TOKEN_CHARS];
	world_t	*w;

	w = &s_worldData;
	w->lightGridSize[0] = 64;
	w->lightGridSize[1] = 64;
	w->lightGridSize[2] = 128;

	p = (char *)(fileBase + l->fileofs);

	/* store for reference by the cgame */
	w->entityString = ri.Hunk_Alloc(l->filelen + 1, h_low);
	strcpy(w->entityString, p);
	w->entityParsePoint = w->entityString;

	token = COM_ParseExt(&p, qtrue);
	if (!*token || *token != '{')
		return;

	/* only parse the world spawn */
	for(;;) {
		/* parse key */
		token = COM_ParseExt(&p, qtrue);

		if (!*token || *token == '}')
			break;

		Q_strncpyz(keyname, token, sizeof(keyname));

		/* parse value */
		token = COM_ParseExt(&p, qtrue);

		if (!*token || *token == '}')
			break;

		Q_strncpyz(value, token, sizeof(value));

		/* check for remapping of shaders for vertex lighting */
		s = "vertexremapshader";
		if (!Q_strncmp(keyname, s, strlen(s))) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf(PRINT_WARNING, "WARNING: no semi colon in vertexshaderremap '%s'\n", value);
				break;
			}

			*s++ = 0;
			continue;
		}

		/* check for remapping of shaders */
		s = "remapshader";
		if (!Q_strncmp(keyname, s, strlen(s))) {
			s = strchr(value, ';');
			if (!s) {
				ri.Printf(PRINT_WARNING, "WARNING: no semi colon in shaderremap '%s'\n", value);
				break;
			}

			*s++ = 0;
			R_RemapShader(value, s, "0");
			continue;
		}

		/* check for a different grid size */
		if (!Q_stricmp(keyname, "gridsize")) {
			sscanf(value, "%f %f %f", &w->lightGridSize[0], &w->lightGridSize[1], &w->lightGridSize[2]);
			continue;
		}
	}
}

/*
 * R_AllocCubeProbe
 * Allocate cubemap probe memory
 */
static cubeProbe_t *R_AllocCubeProbe(void) {
	cubeProbe_t	*cubeProbe;

	cubeProbe = ri.Hunk_Alloc(sizeof(*cubeProbe), h_low);
	Com_Memset(cubeProbe, 0, sizeof(*cubeProbe));

	if (cubeProbeList)
		cubeProbe->next = cubeProbeList;

	cubeProbeList = cubeProbe;

	return cubeProbe;
}

/*
 * R_CreateCubeProbe
 * Render and store a new cubemap
 */
static void R_CreateCubeProbe(vec3_t origin, int size) {
	cubeProbe_t	*cubeProbe;
	byte		*images;
	refdef_t	refdef;
	int			i, j;

	/* allocate a new cubeProbe_t */
	if (!(cubeProbe = R_AllocCubeProbe()))
		ri.Error(ERR_DROP, "R_CreateCubeProbe: R_AllocCubeProbe() failed\n");

	/* clear refdef */
	Com_Memset(&refdef, 0, sizeof(refdef));

	/* allocate image buffer */
	images = ri.Hunk_AllocateTempMemory(6 * size * size * 4);
	Com_Memset(images, 0, 6 * size * size * 4);

	/* position camera at origin */
	VectorCopy(origin, refdef.vieworg);
	AxisClear(refdef.viewaxis);

	/* field of view needs to be 90 degree */
	refdef.fov_x = refdef.fov_y = 90.0f;

	/* start reading at center screen */
	refdef.x = glConfig.vidWidth / 2;
	refdef.y = glConfig.vidHeight / 2;

	/* set resolution */
	refdef.width	= size;
	refdef.height	= size;

	refdef.time = 0;

	refdef.rdflags = RDF_NOEFFECTS | RDF_NOCUBEMAP | RDF_NODISPLAY;

	/* FIXME: render scene to fix missing side bug on first cubemap */
	RE_BeginFrame(STEREO_CENTER);
	RE_RenderScene(&refdef);
	RE_EndFrame(NULL, NULL);

	/* set camera angle and render each side of the cube */
	for (i = 0; i < 6; i++) {
		switch (i) {
			case 0:
				refdef.viewaxis[0][0] = 1;
				refdef.viewaxis[0][1] = 0;
				refdef.viewaxis[0][2] = 0;

				refdef.viewaxis[1][0] = 0;
				refdef.viewaxis[1][1] = 0;
				refdef.viewaxis[1][2] = 1;

				CrossProduct(refdef.viewaxis[0], refdef.viewaxis[1], refdef.viewaxis[2]);
				break;

			case 1:
				refdef.viewaxis[0][0] = -1;
				refdef.viewaxis[0][1] = 0;
				refdef.viewaxis[0][2] = 0;

				refdef.viewaxis[1][0] = 0;
				refdef.viewaxis[1][1] = 0;
				refdef.viewaxis[1][2] = -1;

				CrossProduct(refdef.viewaxis[0], refdef.viewaxis[1], refdef.viewaxis[2]);
				break;

			case 2:
				refdef.viewaxis[0][0] = 0;
				refdef.viewaxis[0][1] = 1;
				refdef.viewaxis[0][2] = 0;

				refdef.viewaxis[1][0] = -1;
				refdef.viewaxis[1][1] = 0;
				refdef.viewaxis[1][2] = 0;

				CrossProduct(refdef.viewaxis[0], refdef.viewaxis[1], refdef.viewaxis[2]);
				break;

			case 3:
				refdef.viewaxis[0][0] = 0;
				refdef.viewaxis[0][1] = -1;
				refdef.viewaxis[0][2] = 0;

				refdef.viewaxis[1][0] = -1;
				refdef.viewaxis[1][1] = 0;
				refdef.viewaxis[1][2] = 0;

				CrossProduct(refdef.viewaxis[0], refdef.viewaxis[1], refdef.viewaxis[2]);
				break;

			case 4:
				refdef.viewaxis[0][0] = 0;
				refdef.viewaxis[0][1] = 0;
				refdef.viewaxis[0][2] = 1;

				refdef.viewaxis[1][0] = -1;
				refdef.viewaxis[1][1] = 0;
				refdef.viewaxis[1][2] = 0;

				CrossProduct(refdef.viewaxis[0], refdef.viewaxis[1], refdef.viewaxis[2]);
				break;

			case 5:
				refdef.viewaxis[0][0] = 0;
				refdef.viewaxis[0][1] = 0;
				refdef.viewaxis[0][2] = -1;

				refdef.viewaxis[1][0] = 1;
				refdef.viewaxis[1][1] = 0;
				refdef.viewaxis[1][2] = 0;

				CrossProduct(refdef.viewaxis[0], refdef.viewaxis[1], refdef.viewaxis[2]);
				break;
		}

		tr.refdef.pixelTarget		= (byte *)images + i * size * size * 4;
		tr.refdef.pixelTargetWidth	= size;
		tr.refdef.pixelTargetHeight	= size;

		/* render scene */
		RE_BeginFrame(STEREO_CENTER);
		RE_RenderScene(&refdef);
		RE_EndFrame(NULL, NULL);

		/* encode pixel intensity into alpha channel */
		for (j = 0; j < size * size; j++) {
			/* a = r */
			tr.refdef.pixelTarget[j * 4 + 3] = tr.refdef.pixelTarget[j * 4];

			/* a = g */
			if (tr.refdef.pixelTarget[j * 4 + 1] > tr.refdef.pixelTarget[j * 4 + 3])
				tr.refdef.pixelTarget[j * 4 + 3] = tr.refdef.pixelTarget[j * 4 + 1];

			/* a = b */
			if (tr.refdef.pixelTarget[j * 4 + 2] > tr.refdef.pixelTarget[j * 4 + 3])
				tr.refdef.pixelTarget[j * 4 + 3] = tr.refdef.pixelTarget[j * 4 + 2];
		}
	}

	/* create cube probe */
	VectorCopy(origin, cubeProbe->origin);
	cubeProbe->cubemap = R_CreateCubeImage(va("*cubeProbe%i", numCubeProbes++), (const byte **)&images, size, size, qfalse, qfalse, GL_CLAMP_TO_EDGE);

	/* clean up */
	tr.refdef.pixelTarget = NULL;
	tr.refdef.rdflags &= ~(RDF_NOCUBEMAP | RDF_NOEFFECTS | RDF_NODISPLAY);

	ri.Hunk_FreeTempMemory(images);
}

/*
 * R_ParseCubeProbes
 * Create a cubemap for each misc_cubeprobe entity
 */
static void R_ParseCubeProbes(void) {
	char		*p, *token, *s;
	qboolean	isCubeProbe;
	vec3_t		origin;
	int			size;

	p = s_worldData.entityString;

	for (;;) {
		token = COM_ParseExt(&p, qtrue);

		/* end of entity definitions */
		if (!*token)
			break;

		/* start of class definition */
		if (*token != '{')
			continue;

		/* set default values */
		isCubeProbe	= qfalse;
		size		= 32;
		VectorClear(origin);

		/* get origin and size */
		for (;;) {
			token = COM_ParseExt(&p, qtrue);

			/* end of class definition */
			if (*token == '}')
				break;

			/* cubeprobe */
			s = "classname";
			if (!Q_strncmp(token, s, strlen(s))) {
				token = COM_ParseExt(&p, qfalse);
				s = "misc_cubeprobe";
				if (!Q_strncmp(token, s, strlen(s)))
					isCubeProbe = qtrue;
				else
					break;
			}

			/* origin */
			s = "origin";
			if (!Q_strncmp(token, s, strlen(s))) {
				token = COM_ParseExt(&p, qfalse);
				sscanf(token, "%f %f %f", &origin[0], &origin[1], &origin[2]);
			}

			/* size */
			s = "size";
			if (!Q_strncmp(token, s, strlen(s))) {
				token = COM_ParseExt(&p, qfalse);
				sscanf(token, "%i", &size);
			}
		}

		/* create cubeprobe */
		if (isCubeProbe)
			R_CreateCubeProbe(origin, size);
	}
}

/*
 * RE_LoadWorldMap
 * Load bsp map into memory
 */
void RE_LoadWorldMap(const char *name) {
	int			i;
	dheader_t	*header;
	union {
		byte *b;
		void *v;
	} buffer;
	byte		*startMarker;

	if (tr.worldMapLoaded)
		ri.Error(ERR_DROP, "ERROR: attempted to redundantly load world map");

	/* set default sun direction to be used if it isn't overridden by a shader */
	tr.sunDirection[0] = 0.45f;
	tr.sunDirection[1] = 0.3f;
	tr.sunDirection[2] = 0.9f;

	VectorNormalize(tr.sunDirection);

	tr.worldMapLoaded	= qtrue;
	tr.worldDir			= NULL;

	/* load it */
    ri.FS_ReadFile(name, &buffer.v);
	if (!buffer.b)
		ri.Error(ERR_DROP, "RE_LoadWorldMap: %s not found", name);

	/* set map meta directory */
	tr.worldDir = CopyString(name);
	COM_StripExtension(tr.worldDir, tr.worldDir, strlen(tr.worldDir) - 3);

	/* clear static buffer */
	vertexBuffer	= NULL;
	indexBuffer		= NULL;
	offsetVertexes	= 0;
	offsetIndexes	= 0;

	/* clear cubemap probe list */
	cubeProbeList	= NULL;
	numCubeProbes	= 0;

	/*
	 * clear tr.world so if the level fails to load, the next
	 * try will not look at the partially loaded version
	 */
	tr.world = NULL;

	Com_Memset(&s_worldData, 0, sizeof(s_worldData));
	Q_strncpyz(s_worldData.name, name, sizeof(s_worldData.name));

	Q_strncpyz(s_worldData.baseName, COM_SkipPath(s_worldData.name), sizeof(s_worldData.name));
	COM_StripExtension(s_worldData.baseName, s_worldData.baseName, sizeof(s_worldData.baseName));

	startMarker = ri.Hunk_Alloc(0, h_low);

	header = (dheader_t *)buffer.b;
	fileBase = (byte *)header;

	i = LittleLong(header->version);
	if (i != Q3_BSP_VERSION && i != ET_BSP_VERSION)
		ri.Error(ERR_DROP, "RE_LoadWorldMap: %s has wrong version number (%i should be %i or %i)", name, i, Q3_BSP_VERSION, ET_BSP_VERSION);

	/* swap all the lumps */
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong(((int *)header)[i]);

	/* load into heap */
	R_LoadShaders(&header->lumps[LUMP_SHADERS]);
	R_LoadLightmaps(&header->lumps[LUMP_LIGHTMAPS], &header->lumps[LUMP_SURFACES]);
	R_LoadPlanes(&header->lumps[LUMP_PLANES]);
	R_LoadFogs(&header->lumps[LUMP_FOGS], &header->lumps[LUMP_BRUSHES], &header->lumps[LUMP_BRUSHSIDES]);
	R_LoadSurfaces(&header->lumps[LUMP_SURFACES], &header->lumps[LUMP_DRAWVERTS], &header->lumps[LUMP_DRAWINDEXES]);
	R_LoadMarksurfaces(&header->lumps[LUMP_LEAFSURFACES]);
	R_LoadNodesAndLeafs(&header->lumps[LUMP_NODES], &header->lumps[LUMP_LEAFS]);
	R_LoadSubmodels(&header->lumps[LUMP_MODELS]);
	R_LoadVisibility(&header->lumps[LUMP_VISIBILITY]);
	R_LoadEntities(&header->lumps[LUMP_ENTITIES]);
	R_LoadLightGrid(&header->lumps[LUMP_LIGHTGRID]);

	s_worldData.dataSize = (byte *)ri.Hunk_Alloc(0, h_low) - startMarker;

	/* only set tr.world now that we know the entire level has loaded properly */
	tr.world = &s_worldData;

    ri.FS_FreeFile(buffer.v);

    /* clear texture bindings */
	Com_Memset(glState.currentTextures, 0, sizeof(glState.currentTextures));

	/* build cubemap probes */
	R_ParseCubeProbes();
}

/*
 * RE_SetWorldVisData
 * Share vis data with clipmodel subsystem
 */
void RE_SetWorldVisData(const byte *vis) {
	tr.externalVisData = vis;
}

/*
 * R_GetEntityToken
 * Get entity
 */
qboolean R_GetEntityToken(char *buffer, int size) {
	const char	*s;

	s = COM_Parse(&s_worldData.entityParsePoint);
	Q_strncpyz(buffer, s, size);
	if (!s_worldData.entityParsePoint || !s[0]) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return qfalse;
	} else {
		return qtrue;
	}
}

/*
 * R_FindNearestCubeProbe
 * Find cube probe closest to origin
 */
cubeProbe_t *R_FindNearestCubeProbe(vec3_t origin) {
	cubeProbe_t	*cubeProbe;
	cubeProbe_t	*p;

	if (!numCubeProbes)
		return NULL;

	cubeProbe = cubeProbeList;
	for (p = cubeProbe->next; p; p = p->next)
		if (Distance(origin, p->origin) < Distance(origin, cubeProbe->origin))
			cubeProbe = p;

	return cubeProbe;
}
