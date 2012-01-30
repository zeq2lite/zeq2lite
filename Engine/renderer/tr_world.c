/*
 * tr_world.c
 * World surface preparation
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

/*
 * R_CullSurface
 * Tries to back face cull surfaces before they are lighted or added to the sorting list
 */
static qboolean	R_CullSurface(surfaceType_t *surface, shader_t *shader) {
	srfGeneric_t	*surf;
	int				cull;
	float			d;

	/* allow culling to be disabled */
	if (r_nocull->integer)
		return qfalse;

	switch (*surface) {
		case SF_TRIANGLES:
			break;
		case SF_GRID:
			if (r_nocurves->integer)
				return qtrue;

			break;
		case SF_FOLIAGE:
			if (!r_drawfoliage->value)
				return qtrue;

			break;
		default:
			return qtrue;
	}

	/* get generic surface */
	surf = (srfGeneric_t *)surface;

	/* plane cull */
	if (surf->plane.type != PLANE_NON_PLANAR && r_facePlaneCull->integer) {
		d = DotProduct(tr.or.viewOrigin, surf->plane.normal) - surf->plane.dist;

		/*
		 * don't cull exactly on the plane, because there are levels of rounding
		 * through the BSP, ICD and hardware that may cause pixel gaps if an
		 * epsilon isn't allowed here
		 */
		if (shader->cullType == CT_FRONT_SIDED) {
			if (d < -8.0f)
				return qtrue;
		} else if (shader->cullType == CT_BACK_SIDED) {
			if (d > 8.0f)
				return qtrue;
		}
	}

	/* try sphere cull */
	if (tr.currentEntityNum != ENTITYNUM_WORLD)
		cull = R_CullLocalPointAndRadius(surf->origin, surf->radius);
	else
		cull = R_CullPointAndRadius(surf->origin, surf->radius);

	if (cull == CULL_OUT)
		return qtrue;

	/* must be visible */
	return qfalse;
}

/*
 * R_AddWorldSurface
 * Add world surface to draw surface list
 */
static void R_AddWorldSurface(msurface_t *surf) {
	/* allready in this view */
	if (surf->viewCount == tr.viewCount)
		return;

	surf->viewCount = tr.viewCount;

	/* try to cull before dlighting or adding */
	if (R_CullSurface(surf->data, surf->shader))
		return;

	R_AddDrawSurf(surf->data, surf->shader, surf->fogIndex);
}

/*
 * R_AddBrushModelSurface
 * Add an entities brush model surfaces to draw surface list
 */
void R_AddBrushModelSurfaces(trRefEntity_t *ent) {
	bmodel_t	*bmodel;
	int			clip;
	model_t		*pModel;
	int			i;

	pModel	= R_GetModelByHandle(ent->e.hModel);
	bmodel	= pModel->model.bmodel;

	clip = R_CullLocalBox(bmodel->bounds);
	if (clip == CULL_OUT)
		return;

	R_SetupEntityLighting(&tr.refdef, ent);

	for (i = 0 ; i < bmodel->numSurfaces; i++)
		R_AddWorldSurface(bmodel->firstSurface + i);
}

/*
 * R_RecursiveWorldNode
 * Recurse through world nodes
 */
static void R_RecursiveWorldNode(mnode_t *node, int planeBits) {
	msurface_t	*surf, **mark;
	int			c;

	do {
		/* if the node wasn't marked as potentially visible, exit */
		if (node->visframe != tr.visCount)
			return;

		/*
		 * if the bounding volume is outside the frustum, nothing
		 * inside can be visible OPTIMIZE: don't do this all the way to leafs?
		 */

		if (!r_nocull->integer) {
			int	r;

			if (planeBits & 1) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[0]);
				if (r == 2)
					return;						/* culled */

				if (r == 1)
					planeBits &= ~1;			/* all descendants will also be in front */
			}

			if (planeBits & 2) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[1]);
				if (r == 2)
					return;						/* culled */

				if (r == 1)
					planeBits &= ~2;			/* all descendants will also be in front */
			}

			if (planeBits & 4) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[2]);
				if (r == 2)
					return;						/* culled */

				if (r == 1)
					planeBits &= ~4;			/* all descendants will also be in front */
			}

			if (planeBits & 8) {
				r = BoxOnPlaneSide(node->mins, node->maxs, &tr.viewParms.frustum[3]);
				if (r == 2)
					return;						/* culled */

				if (r == 1)
					planeBits &= ~8;			/* all descendants will also be in front */
			}
		}

		/* handle leaf nodes */
		if (node->contents != -1)
			break;

		/* recurse down the children, front side first */
		R_RecursiveWorldNode(node->children[0], planeBits);

		/* tail recurse */
		node = node->children[1];
	} while (qtrue);

	/* leaf node, so add mark surfaces */
	tr.pc.c_leafs++;

	/* add to z buffer bounds */
	if (node->mins[0] < tr.viewParms.visBounds[0][0])
		tr.viewParms.visBounds[0][0] = node->mins[0];

	if (node->mins[1] < tr.viewParms.visBounds[0][1])
		tr.viewParms.visBounds[0][1] = node->mins[1];

	if (node->mins[2] < tr.viewParms.visBounds[0][2])
		tr.viewParms.visBounds[0][2] = node->mins[2];

	if (node->maxs[0] > tr.viewParms.visBounds[1][0])
		tr.viewParms.visBounds[1][0] = node->maxs[0];

	if (node->maxs[1] > tr.viewParms.visBounds[1][1])
		tr.viewParms.visBounds[1][1] = node->maxs[1];

	if (node->maxs[2] > tr.viewParms.visBounds[1][2])
		tr.viewParms.visBounds[1][2] = node->maxs[2];

	/* add the individual surfaces */
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while (c--) {
		/* the surface may have already been added if it spans multiple leafs */
		surf = *mark;
		R_AddWorldSurface(surf);
		mark++;
	}
}

/*
 * R_PointInLeaf
 * Check for leaf at point p
 */
static mnode_t *R_PointInLeaf(const vec3_t p) {
	mnode_t		*node;
	float		d;
	cplane_t	*plane;
	
	if (!tr.world)
		ri.Error(ERR_DROP, "R_PointInLeaf: bad model");

	node = tr.world->nodes;

	while (qtrue) {
		if (node->contents != -1)
			break;

		plane = node->plane;

		d = DotProduct(p, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return node;
}

/*
 * R_ClusterPVS
 * Get vis data for cluster
 */
const byte *R_ClusterPVS(int cluster) {
	if (!tr.world || !tr.world->vis || cluster < 0 || cluster >= tr.world->numClusters)
		return tr.world->novis;

	return tr.world->vis + cluster * tr.world->clusterBytes;
}

/*
 * R_inPVS
 * Check if bounding box is in potentially visible set
 */
qboolean R_inPVS(const vec3_t p1, const vec3_t p2) {
	mnode_t *leaf;
	byte	*vis;

	leaf	= R_PointInLeaf(p1);
	vis		= CM_ClusterPVS(leaf->cluster);
	leaf	= R_PointInLeaf(p2);

	if (!(vis[leaf->cluster >> 3] & (1 << (leaf->cluster & 7))))
		return qfalse;

	return qtrue;
}

/*
 * R_MarkLeaves
 * Mark the leaves and nodes that are in the PVS for the current cluster
 */
static void R_MarkLeaves(void) {
	const byte	*vis;
	mnode_t		*leaf, *parent;
	int			cluster;
	int			i;

	/* lockpvs lets designers walk around to determine the extent of the current pvs */
	if (r_lockpvs->integer)
		return;

	/* current viewcluster */
	leaf = R_PointInLeaf(tr.viewParms.pvsOrigin);
	cluster = leaf->cluster;

	/*
	 * if the cluster is the same and the area visibility matrix
	 * hasn't changed, we don't need to mark everything again
	 */

	/* if r_showcluster was just turned on, remark everything */
	if (tr.viewCluster == cluster && !tr.refdef.areamaskModified && !r_showcluster->modified)
		return;

	if (r_showcluster->modified || r_showcluster->integer) {
		r_showcluster->modified = qfalse;

		if (r_showcluster->integer)
			ri.Printf(PRINT_ALL, "cluster:%i  area:%i\n", cluster, leaf->area);
	}

	tr.visCount++;
	tr.viewCluster = cluster;

	if (r_novis->integer || tr.viewCluster == -1) {
		for (i = 0; i < tr.world->numnodes; i++)
			if (tr.world->nodes[i].contents != CONTENTS_SOLID)
				tr.world->nodes[i].visframe = tr.visCount;

		return;
	}

	vis = R_ClusterPVS(tr.viewCluster);
	
	for (i = 0, leaf = tr.world->nodes; i < tr.world->numnodes; i++, leaf++) {
		cluster = leaf->cluster;
		if (cluster < 0 || cluster >= tr.world->numClusters)
			continue;

		/* check general pvs */
		if (!(vis[cluster >> 3] & (1 << (cluster & 7))))
			continue;

		/* check for door connection */
		if ((tr.refdef.areamask[leaf->area >> 3] & (1 << (leaf->area & 7))))
			continue;

		parent = leaf;
		do {
			if (parent->visframe == tr.visCount)
				break;

			parent->visframe = tr.visCount;
			parent = parent->parent;
		} while (parent);
	}
}

/*
 * R_AddWorldSurfaces
 * Add all world surfaces
 */
void R_AddWorldSurfaces(void) {
	if (!r_drawworld->integer)
		return;

	if (tr.refdef.rdflags & RDF_NOWORLDMODEL)
		return;

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	/* determine which leaves are in the PVS / areamask */
	R_MarkLeaves();

	/* clear out the visible min/max */
	ClearBounds(tr.viewParms.visBounds[0], tr.viewParms.visBounds[1]);

	/* perform frustum culling and add all the potentially visible surfaces */
	if (tr.refdef.num_dlights > 32)
		tr.refdef.num_dlights = 32 ;

	R_RecursiveWorldNode(tr.world->nodes, 15);
}
