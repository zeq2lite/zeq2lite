/*
============================================================

this stuff goes in qfiles.h.

============================================================
*/
#ifndef __MD4FILE_H__
#define __MD4FILE_H__

// md4file.h

#include "mathlib.h"

// the maximum size of game relative pathnames
#define	MAX_QPATH		64

/*
============================================================

MD4 file format

============================================================

revision history:

  v1	id software version
		incomplete implementation in q3a pr 1.32b

  v2	first gongo version
		internal development release
		max bones 128->256
		max frames 1024->2048
		added name[32] to bones
		frame names name[16]->name[32]
		added offset to weights

  v3	first public release
		removed md4LOD system (it was broken anyway)
		numSurfs and ofsSurfs in md4Header
		added collapseMap to md4Surface_t
		added lodBias to md4Surface_t
		added minLOD to md4Surface_t
		max tris 8192->5632
		max verts 4096->2816
		changed md4Bone_t -> md4BoneFrame_t
		added md4Bone_t to header from ofsBones
		stored bone name[32], parent, and flags in md4Bone_t
		created new bone flags for future use (H,L,U,T)

  v4	removed frame names
		renamed md4Bone_t -> md4BoneName_t
		renamed BoneReferences -> BoneRefs
		renamed Triangles -> Tris
		renamed minLOD -> minLod
		increased stack size in skl2md4
		increased MD4_MAX_TRIANGLES 5632 -> 8192
		increased MD4_MAX_VERTS 2816 -> 4096

  v5	MD4_MAX_BONES 256 -> 128
		changed bone flags to be more generic (NONE,A,B,C)
		moved bone flags from #defines to md4BoneFlag_t
		MD4_MAX_WEIGHTS 8 -> 16
		removed BoneRefs, was unused anyway
		removed md4Vertex.vertex, was unused

  v6	ZEQ2Lite additions
		Added weight->normalOffset
*/

#define MD4_IDENT			(('4'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD4_VERSION			6
#define	MD4_MAX_BONES		128
#define MD4_MAX_WEIGHTS		16		// per vertex
#define	MD4_MAX_TRIANGLES	8192	// per surface
#define MD4_MAX_VERTS		4096	// per surface
#define MD4_MAX_FRAMES		2048	// per model
#define	MD4_MAX_SURFACES	32		// per model

typedef struct {
	int			boneIndex;
	float		boneWeight;
	vec3_t		offset;
	vec3_t		normalOffset;
} md4Weight_t;

typedef struct {
	vec3_t		normal;
	vec2_t		texCoords;
	int			numWeights;
	md4Weight_t	weights[1];		// numWeights in size
} md4Vertex_t;

typedef struct {
	int			indexes[3];
} md4Triangle_t;

typedef struct {
	int			ident;

	char		name[MAX_QPATH];	// polyset name
	char		shader[MAX_QPATH];
	int			shaderIndex;		// for in-game use

	int			lodBias;			// in-game lod setting
	int			minLod;				// don't collapse past this point

	int			ofsHeader;			// this will be a negative number

	int			numVerts;
	int			ofsVerts;

	int			numTris;
	int			ofsTris;

	int			ofsCollapseMap;		// collapse map for dynamic lod

	int			ofsEnd;				// next surface follows
} md4Surface_t;

typedef struct {
	float		matrix[3][4];
} md4BoneFrame_t;

typedef struct {
	vec3_t			bounds[2];		// bounds of all surfaces for this frame
	vec3_t			localOrigin;	// midpoint of bounds, used for sphere cull
	float			radius;			// dist from localOrigin to corner
	md4BoneFrame_t	bones[1];		// numBones in size
} md4Frame_t;

typedef enum {
	MD4_BONE_FLAG_NONE,
	MD4_BONE_FLAG_A,
	MD4_BONE_FLAG_B,
	MD4_BONE_FLAG_C
} md4BoneFlag_t;

typedef struct {
	char	name[32];			// bone name
	int		parent;				// parent index
	int		flags;				// flags (NONE,A,B,C)
} md4BoneName_t;

typedef struct {
	int		ident;
	int		version;

	char	name[MAX_QPATH];	// model name

	int		numFrames;
	int		numBones;
	int		numSurfs;
	int		ofsFrames;			// md4Frame_t[numFrames]
	int		ofsBones;			// md4BoneName_t[numBones]
	int		ofsSurfs;			// first surface, others follow

	int		ofsEnd;				// end of file
} md4Header_t;

/*
============================================================

basic framework for new MD4a (anim) and MD4b (base) system

============================================================
*/

#define MD4A_IDENT			(('A'<<24)+('4'<<16)+('D'<<8)+'M')
#define MD4A_VERSION		2
#define MD4A_MAX_FRAMES		512	// per animation file

typedef struct {
	char	name[32];			// bone name
	int		parent;				// parent index
	int		flags;				// flags (NONE,A,B,C)
} md4aBoneName_t;

typedef struct {
	//vec3_t	origin;				// origin offset from parent bone
	float	matrix[3][4];			// axis orientation
} md4aBoneFrame_t;

typedef struct {
	vec3_t			bounds[2];		// bounds of all surfaces for this frame
	vec3_t			localOrigin;	// midpoint of bounds, used for sphere cull
	float			radius;			// dist from localOrigin to corner
	md4BoneFrame_t	bones[1];		// numBones in size
} md4aFrame_t;

typedef struct {
	int		ident;				// 'MD4A'
	int		version;			// MD4A_VERSION
	char	name[MAX_QPATH];	// animation name
	int		numFrames;			// total frames in this animation
	int		numBones;			// to compute frame size
	int		ofsFrames;			// md4aFrame_t[numFrames]
	int		ofsBones;			// md4aBoneName_t[numBones]
	int		ofsEnd;				// EOF
} md4aHeader_t;

#define MD4B_IDENT			(('B'<<24)+('4'<<16)+('D'<<8)+'M')
#define MD4B_VERSION		2
#define MD4B_MAX_WEIGHTS	16		// per vertex
#define	MD4B_MAX_BONES		128		// per model
#define	MD4B_MAX_TRIANGLES	8192	// per surface
#define MD4B_MAX_VERTS		4096	// per surface
#define	MD4B_MAX_SURFACES	32		// per model

typedef enum {
	MD4B_BONE_FLAG_NONE,
	MD4B_BONE_FLAG_A,
	MD4B_BONE_FLAG_B,
	MD4B_BONE_FLAG_C
} md4bBoneFlag_t;

typedef struct {
	char	name[MAX_QPATH];	// bone name
	int		parent;				// parent index
	int		flags;				// md4bBoneFlag_t
	float	length;				// bone length
} md4bBoneName_t;

typedef struct {
	int		boneIndex;				// bone index
	float	boneWeight;				// bone weight
	vec3_t	offset;				// offset to vertex
	vec3_t	normalOffset;				// offset to vertex
} md4bWeight_t;

typedef struct {
	vec3_t			normal;		// vertex normal
	vec2_t			texCoords;	// texture coordinates
	int				numWeights;	// number of bone influences
	md4bWeight_t	weights[1];	// md4Weight_t[numWeights]
} md4bVertex_t;

typedef struct {
	int		indexes[3];			// vertex indexes
} md4bTriangle_t;

typedef struct {
	int		ident;				// SF_MD4B
	char	name[MAX_QPATH];	// surface name
	char	shader[MAX_QPATH];	// shader name
	int		shaderIndex;		// shader index
	int		lodBias;			// engine detail setting
	int		minLod;				// don't collapse past this point
	int		numVerts;			// total verts for this surf
	int		numTris;			// total faces for this surf
	int		ofsHeader;			// offset to md4bHeader_t
	int		ofsVerts;			// md4bVertex_t[numVerts]
	int		ofsTris;			// md4bTriangle_t[numTris]
	int		ofsCollapseMap;		// int[numVerts]
	int		ofsEnd;				// next md4bSurface_t
} md4bSurface_t;

typedef struct {
	int		ident;				// 'MD4B'
	int		version;			// MD4B_VERSION
	char	name[MAX_QPATH];	// model name
	int		numBones;			// total bones in skeleton
	int		numSurfs;			// total surfaces for model
	int		ofsBones;			// md4bBone_t[numBones]
	int		ofsSurfs;			// md4bSurface_t[numSurfs]
	int		ofsEnd;				// EOF
} md4bHeader_t;

#endif /*!__MD4FILE_H__*/
