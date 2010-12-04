/*
 *	progressive mesh algorithm
 *		used to generate collapse map for model
 *		original code in c++ by Stan Melax
 *		adapted for use in smd2md4
 */

#include "zskeleton.h"

static int min_resolution;
static progMeshTri_t *triangles[MAX_TRIS];
static progMeshVert_t *vertices[MAX_VERTS];
static int numProgMeshTris,numProgMeshVerts;


void UpdateNeighbors(progMeshTri_t *tri) {
	int i,j,k;

	// make sure all vertices for this triangle are neighbors
	for(i = 0; i < 3; i++) {
		for(k = 1; k < 3; k++) {
			int found = 0;
			for(j = 0; j < vertices[tri->verts[i]]->numNeighbors; j++) {
				if(vertices[tri->verts[i]]->neighbors[j] == tri->verts[(i + k) % 3]) {
					found = 1;
					break;
				}
			}

			// add neighbor if it is unique
			if(!found && vertices[tri->verts[(i + k) % 3]]->valid) {
				vertices[tri->verts[i]]->numNeighbors++;
				vertices[tri->verts[i]]->neighbors = (int *)realloc(vertices[tri->verts[i]]->neighbors, sizeof(int) * vertices[tri->verts[i]]->numNeighbors);
				vertices[tri->verts[i]]->neighbors[vertices[tri->verts[i]]->numNeighbors - 1] = tri->verts[(i + k) % 3];
			}
		}
	}
}

void RebuildNeighbors(progMeshVert_t *v) {
	int i;

	// clear neighbor list
	v->numNeighbors = 0;
	v->neighbors = (int *)realloc(v->neighbors,sizeof(int));
	for(i = 0; i < v->numFaces; i++) {
		// rebuild from triangle list
		UpdateNeighbors(triangles[v->faces[i]]);
	}
}

void RemoveTriangle(progMeshTri_t *tri) {
	int i,j;

	// remove triangle from each referenced vertex
	for(i = 0; i < 3; i++) {
		int numFaces = 0;
		int *faces;

		// update the triangle list
		faces = (int *)calloc(vertices[tri->verts[i]]->numFaces,sizeof(int));
		for(j = 0; j < vertices[tri->verts[i]]->numFaces; j++) {
			if( (vertices[tri->verts[i]]->faces[j] != tri->id) &&
				(triangles[vertices[tri->verts[i]]->faces[j]]->valid) ) {
				faces[numFaces] = vertices[tri->verts[i]]->faces[j];
				numFaces++;
			}
		}
		vertices[tri->verts[i]]->numFaces = numFaces;
		for(j = 0; j < numFaces; j++) {
			vertices[tri->verts[i]]->faces[j] = faces[j];
		}
		RebuildNeighbors(vertices[tri->verts[i]]);
	}
	tri->valid = 0;
	numProgMeshTris--;
}

void RemoveVertex(progMeshVert_t *v) {
	int i,j;

	// remove this vertex from all of its neighbors' neighbors list
	for(i = 0; i < v->numNeighbors; i++) {
		int numNeighbors = 0;
		int *neighbors;

		neighbors = (int *)calloc(vertices[v->neighbors[i]]->numNeighbors, sizeof(int));
		for(j = 0; j < vertices[v->neighbors[i]]->numNeighbors; j++) {
			if( (vertices[v->neighbors[i]]->neighbors[j] != v->id) &&
				(vertices[vertices[v->neighbors[i]]->neighbors[j]]->valid) ) {
				neighbors[numNeighbors] = vertices[v->neighbors[i]]->neighbors[j];
				numNeighbors++;
			}
		}
		vertices[v->neighbors[i]]->numNeighbors = numNeighbors;
		for(j = 0; j < numNeighbors; j++) {
			vertices[v->neighbors[i]]->neighbors[j] = neighbors[j];
		}
	}
	v->valid = 0;
	numProgMeshVerts--;
}

void ComputeNormal(progMeshTri_t *tri) {
	vec3_t temp1,temp2,temp3;

	VectorSubtract(vertices[tri->verts[1]]->origin,vertices[tri->verts[0]]->origin,temp1);
	VectorSubtract(vertices[tri->verts[2]]->origin,vertices[tri->verts[1]]->origin,temp2);
	CrossProduct(temp1,temp2,temp3);
	VectorNormalize(temp3,tri->normal);
}

void ReplaceVertex(progMeshVert_t *vold, progMeshVert_t *vnew) {
	int i;

	for(i = 0; i < vold->numFaces; i++) {
		// remove degenerate triangles
		if( (triangles[vold->faces[i]]->verts[0] == vnew->id) ||
			(triangles[vold->faces[i]]->verts[1] == vnew->id) ||
			(triangles[vold->faces[i]]->verts[2] == vnew->id) ) {
			RemoveTriangle(triangles[vold->faces[i]]);
			continue;
		}

		// update the remaining triangles
		if(triangles[vold->faces[i]]->verts[0] == vold->id) {
			triangles[vold->faces[i]]->verts[0] = vnew->id;
		} else if(triangles[vold->faces[i]]->verts[1] == vold->id) {
			triangles[vold->faces[i]]->verts[1] = vnew->id;
		} else if(triangles[vold->faces[i]]->verts[2] == vold->id) {
			triangles[vold->faces[i]]->verts[2] = vnew->id;
		}
		vnew->numFaces++;
		vnew->faces = (int *)realloc(vnew->faces,sizeof(int) * vnew->numFaces);
		vnew->faces[vnew->numFaces - 1] = vold->faces[i];
		ComputeNormal(triangles[vold->faces[i]]);
		UpdateNeighbors(triangles[vold->faces[i]]);
	}
	RemoveVertex(vold);
}

int CountTrisOnEdge(progMeshVert_t *u, progMeshVert_t *v) {
	int i, num;

	num = 0;
	for(i = 0; i < u->numFaces; i++) {
		if( (triangles[u->faces[i]]->verts[0] == v->id) ||
			(triangles[u->faces[i]]->verts[1] == v->id) ||
			(triangles[u->faces[i]]->verts[2] == v->id) ) {
			num++;
		}
	}

	return num;
}

int TriHasSilhouetteEdge(progMeshTri_t *tri, progMeshVert_t *u) {
	int i;

	for(i = 0; i < 3; i++) {
		if ( (tri->verts[i] != u->id) && (CountTrisOnEdge(u, vertices[tri->verts[i]]) == 1) ) {
			return 1;
		}
	}

	return 0;
}

int HasSilhouetteEdge(progMeshVert_t *u) {
	int i;

	for(i = 0; i < u->numFaces; i++) {
		if ( TriHasSilhouetteEdge( triangles[u->faces[i]], u ) ) {
			return 1;
		}
	}

	return 0;
}

float ComputeEdgeCollapseCost(progMeshVert_t *u, progMeshVert_t *v) {
	int i, j;
	int *sides, numSides;
	vec3_t temp;
	float edgelength;
	float curvature = 0;

	VectorSubtract(v->origin,u->origin,temp);
	edgelength = VectorLength(temp);

	numSides = 0;
	sides = (int *)calloc(1, sizeof(int));
	for(i = 0; i < u->numFaces; i++) {
		if( (triangles[u->faces[i]]->verts[0] == v->id) ||
			(triangles[u->faces[i]]->verts[1] == v->id) ||
			(triangles[u->faces[i]]->verts[2] == v->id) ) {
			numSides++;
			sides = (int *)realloc(sides,sizeof(int) * numSides);
			sides[numSides - 1] = u->faces[i];
		}
	}

	for(i = 0; i < u->numFaces; i++) {
		float mincurv = 1;

		for(j = 0; j < numSides; j++) {
			float dotprod;

			dotprod = DotProduct(triangles[u->faces[i]]->normal, triangles[sides[j]]->normal);
			mincurv = min(mincurv,(1 - dotprod)/2.0f);
		}

		curvature = max(curvature,mincurv);
	}

   return edgelength * curvature + ( HasSilhouetteEdge(u) ? 100000 : 0);
}

void ComputeEdgeCostAtVertex(progMeshVert_t *v) {
	int i;

	if(!v->numNeighbors) {
		v->collapse = -1;
		v->objdist = -0.01f;
		return;
	}
	v->objdist = 1000000;
	v->collapse = -1;
	for(i = 0; i < v->numNeighbors; i++) {
		float dist;

		dist = ComputeEdgeCollapseCost(v,vertices[v->neighbors[i]]);
		if(dist < v->objdist) {
			v->collapse = v->neighbors[i];
			v->objdist = dist;
		}
	}
}

void ComputeAllEdgeCollapseCosts(void) {
	int i,j;

	for(i = 0, j = 0; i < numProgMeshVerts; i++, j++) {
		while(!vertices[j]->valid) {
			j++;
		}
		ComputeEdgeCostAtVertex(vertices[j]);
	}
}

void Collapse(progMeshVert_t *u, progMeshVert_t *v) {
	int i;

	if(!v) {
		RemoveVertex(u);
		return;
	}

	ReplaceVertex(u,v);

	for(i = 0; i < u->numNeighbors; i++) {
		ComputeEdgeCostAtVertex(vertices[u->neighbors[i]]);
	}
	for(i = 0; i < v->numNeighbors; i++) {
		ComputeEdgeCostAtVertex(vertices[v->neighbors[i]]);
	}
}

void AddVertices(smdSurface_t *surf) {
	int i,j,k,m;
	int numNeighbors,numFaces;
	int *neighbors,*faces;

	numProgMeshVerts = 0;

	for(i = 0; i < surf->numVerts; i++) {
		// find all faces that reference this vertex
		numFaces = 0;
		faces = (int *)calloc(1,sizeof(int));
		for(j = 0; j < numProgMeshTris; j++) {
			if( (triangles[j]->verts[0] == i) ||
				(triangles[j]->verts[1] == i) ||
				(triangles[j]->verts[2] == i) ) {
				numFaces++;
				faces = (int *)realloc(faces, sizeof(int) * numFaces);
				faces[numFaces - 1] = j;
			}
		}

		// find all neighbors to this vertex
		numNeighbors = 0;
		neighbors = (int *)calloc(1,sizeof(int));
		for(j = 0; j < numFaces; j++) {
			for(k = 0; k < 3; k++) {
				if(triangles[faces[j]]->verts[k] != i) {
					int found = 0;
					for(m = 0; m < numNeighbors; m++) {
						if(triangles[faces[j]]->verts[k] == neighbors[m]) {
							found = 1;
							break;
						}
					}

					// add neighbor if unique
					if(!found) {
						numNeighbors++;
						neighbors = (int *)realloc(neighbors, sizeof(int) * numNeighbors);
						neighbors[numNeighbors - 1] = triangles[faces[j]]->verts[k];
					}
				}
			}
		}

		// allocate space and copy data
		vertices[i] = (progMeshVert_t *)calloc(1, sizeof(progMeshVert_t) + (sizeof(int) * numNeighbors) + (sizeof(int) * numFaces));
		vertices[i]->neighbors = (int *)realloc(vertices[i]->neighbors, sizeof(int) * numNeighbors);
		for(j = 0; j < numNeighbors; j++) {
			vertices[i]->neighbors[j] = neighbors[j];
		}
		vertices[i]->faces = (int *)realloc(vertices[i]->faces, sizeof(int) * numFaces);
		for(j = 0; j < numFaces; j++) {
			vertices[i]->faces[j] = faces[j];
		}
		//vertices[i]->origin[0] = -vertices[i]->origin[1];
		//vertices[i]->origin[1] = vertices[i]->origin[0];
		vertices[i]->origin[0] = vertices[i]->origin[0];
		vertices[i]->origin[1] = vertices[i]->origin[1];
		vertices[i]->origin[2] = vertices[i]->origin[2];
		vertices[i]->id = i;
		vertices[i]->numNeighbors = numNeighbors;
		vertices[i]->numFaces = numFaces;
		vertices[i]->collapse = -1;
		vertices[i]->valid = 1;

		numProgMeshVerts++;
	}

	// compute face normals now that vertex data is loaded
	for(i = 0; i < numProgMeshTris; i++) {
		ComputeNormal(triangles[i]);
	}
}

void AddFaces(smdSurface_t *surf) {
	int i;

	numProgMeshTris = 0;

	for(i = 0; i < surf->numTris; i++) {
		triangles[i] = (progMeshTri_t *)calloc(1, sizeof(progMeshTri_t));
		triangles[i]->verts[0] = surf->tris[i].verts[0];
		triangles[i]->verts[1] = surf->tris[i].verts[1];
		triangles[i]->verts[2] = surf->tris[i].verts[2];
		triangles[i]->id = i;
		triangles[i]->valid = 1;

		numProgMeshTris++;
	}
}

progMeshVert_t *MinimumCostEdge(void) {
	progMeshVert_t *mn = NULL;
	int i,j;

	for(i = 0, j = 0; i < numProgMeshVerts; i++, j++) {
		while(!vertices[j]->valid) {
			j++;
		}
		if(vertices[j]->collapse < 0) {
			return vertices[j];
		}
	}

	for(i = 0, j = 0; i < numProgMeshVerts; i++, j++) {
		while(!vertices[j]->valid) {
			j++;
		}
		if(!mn || (vertices[j]->objdist < mn->objdist) ) {
			if (HasSilhouetteEdge(vertices[j])) {
				continue;
			}
			mn = vertices[j];
		}
	}

	if (mn) {
		return mn;
	}

	if (min_resolution == -1) {
		min_resolution = numProgMeshVerts;
	}

	mn = vertices[0];
	for(i = 0, j = 0; i < numProgMeshVerts; i++, j++) {
		while(!vertices[j]->valid) {
			j++;
		}
		if( (!mn->valid) || (vertices[j]->objdist < mn->objdist) ) {
			mn = vertices[j];
		}
	}
	return mn;
}

void ProgressiveMesh(smdSurface_t *surf) {
	int i,num;
	int *permutation;
	progMeshVert_t *mn;
	smdVertex_t *verts;

	AddFaces(surf);
	AddVertices(surf);  // put input data into our data structures
	ComputeAllEdgeCollapseCosts(); // cache all edge collapse costs

	min_resolution = -1;
	permutation = (int *)calloc(surf->numVerts,sizeof(int));

	// reduce the object down to nothing:
	for( num = surf->numVerts; num > 0; num-- ) {

		// get the next vertex to collapse
		mn = MinimumCostEdge();

		// keep track of this vertex, i.e. the collapse ordering
		permutation[mn->id] = num - 1;
		// keep track of vertex to which we collapse to
		surf->map[num - 1] = mn->collapse;

		// Collapse this edge
		if(mn->collapse > -1) {
			Collapse(mn,vertices[mn->collapse]);
		} else {
			Collapse(mn,NULL);
		}
	}

	// reorder the map list based on the collapse ordering
	for(i = 0 ; i < surf->numVerts; i++) {
		surf->map[i] = (surf->map[i] == -1) ? 0 : permutation[surf->map[i]];
	}

	// reorder vertices according to permutation
	verts = (smdVertex_t *)calloc(surf->numVerts,sizeof(smdVertex_t));
	for(i = 0; i < surf->numVerts; i++) {
		verts[permutation[i]] = surf->verts[i];
	}
	for(i = 0; i < surf->numVerts; i++) {
		surf->verts[i] = verts[i];
	}

	// reorder vertex indices for triangles
	for(i = 0; i < surf->numTris; i++) {
		surf->tris[i].verts[0] = permutation[surf->tris[i].verts[0]];
		surf->tris[i].verts[1] = permutation[surf->tris[i].verts[1]];
		surf->tris[i].verts[2] = permutation[surf->tris[i].verts[2]];
	}

	if ( min_resolution == -1 ) {
		min_resolution = 0;
	}
	surf->minLod = min_resolution;

	if(debug) {
		printf("  surface %s has minimum resolution of %i vertices\r",surf->name,surf->minLod);
	}
	// clean up
	for(i = 0; i < surf->numVerts; i++) {
		free(vertices[i]->faces);
		free(vertices[i]->neighbors);
		free(vertices[i]);
	}
	numProgMeshVerts = 0;
	for(i = 0; i < surf->numTris; i++) {
		free(triangles[i]);
	}
	numProgMeshTris = 0;
}
