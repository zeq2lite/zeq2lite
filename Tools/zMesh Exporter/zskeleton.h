/*
============================================================

structs used as temp storage to calculate final model.

============================================================
*/

#ifndef __ZSKELETON_H__
#define __ZSKELETON_H__

// zskeleton.h

#include "scriplib.h"
#include "md4file.h"

#define ZSKELETON_VERSION	1
#define SMD2MD4_VERSION	10
#define SMD_VERSION 	1
#define CFG_VERSION		3

#define MAX_BONES	128
#define MAX_FRAMES	2048
#define MAX_VERTS	4096
#define MAX_WEIGHTS	16
#define MAX_TRIS	8192
#define MAX_SURFS	32

typedef struct {
	float	origin[3];
	float	matrix[3][3];
} smdBoneFrame_t;

typedef struct {
	int		id;
	char	name[64];
	int		parent;
	int		flags;
} smdBoneName_t;

typedef struct {
	float	origin[3];
	float	bounds[2][3];
	float	radius;
	smdBoneFrame_t	*bones;
} smdFrame_t;

typedef struct {
	int		bone;
	float	weight;
	float	offset[3];
	float   normalOffset[3];
} smdWeight_t;

typedef struct {
	int		parent;
	float	origin[3];
	float	normal[3];
	float	texCoords[2];
	int		numWeights;
	smdWeight_t	weights[MAX_WEIGHTS];
} smdVertex_t;

typedef struct {
	int		verts[3];
	int		surf;
} smdTriangle_t;

typedef struct {
	int		id;
	char	name[64];
	int		numVerts;
	int		numTris;
	int		minLod;
	smdTriangle_t	*tris;
	smdVertex_t		*verts;
	int		*map;
} smdSurface_t;

typedef struct {
	char	name[64];
	int		numBones;
	int		numFrames;
	int		numSurfs;
	smdBoneName_t	*bones;
	smdFrame_t		*frames;
	smdSurface_t	*surfs;
} smdModel_t;

typedef struct {
	int	verts[3];
	vec3_t normal;
	int id;
	int valid;
} progMeshTri_t;

typedef struct {
	vec3_t origin;
	int id;
	float objdist;
	int numNeighbors;
	int numFaces;
	int collapse;
	int valid;
	int *neighbors;
	int *faces;
} progMeshVert_t;

extern qboolean verbose,debug;
extern int progress;
extern char indicator[];

void PrintUsage (void);
void LoadSMD(smdModel_t *smd, char modelName[32], char fileName[32]);
void LoadSMD_ParseNodes(smdModel_t *smd);
void LoadSMD_ParseSkeleton(smdModel_t *smd);
void LoadSMD_ParseTriangles(smdModel_t *smd);
void FixCoords(void);
void SaveMD4(void);
void SaveAnimation(smdModel_t *smd, char fileName[32]);
void SaveMesh(smdModel_t *smd, char fileName[32]);
void DoMeshes(void);
void DoAnims(void);
extern void OrderMesh(int input[][3], int output[][3], int numTris);
extern void ProgressiveMesh(smdSurface_t *surf);

#include "config.h"
#endif /*!__ZSKELETON_H__*/
