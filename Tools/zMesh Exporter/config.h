/*/
============================================================

structs used as temp storage to calculate final model.

============================================================
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define ZSKEL_VERSION		1
#define ZSKEL_MAXBONES		128
#define ZSKEL_MAXBONESETS	3
#define ZSKEL_MAXMESHES		32
#define ZSKEL_MAXANIMS		256


typedef char zSkel_Config_BoneName_t[32];

typedef struct {
	char name[32];
	char input[32];
	char output[32];
	float meshOffset[3];
	smdModel_t smd;
	md4bHeader_t zMesh;
} zSkel_Config_Mesh_t;

typedef struct {
	char name[32];
	char input[32];
	char output[32];
	smdModel_t smd;
	md4aHeader_t zAnimation;
	float animOffset[3];

} zSkel_Config_Anim_t;

typedef struct {
	char name[32];
	int numBones;
	zSkel_Config_BoneName_t bones[ZSKEL_MAXBONES];
} zSkel_Config_BoneSets_t;

typedef struct {
	float originOffset[3];
	char animExtension[32];
	char meshExtension[32];
	qboolean verbose;
	qboolean debug;
	qboolean forceCompile;

	int numBoneSets;
	zSkel_Config_BoneSets_t boneSets[ZSKEL_MAXBONESETS];

	int numMeshes;
	zSkel_Config_Mesh_t meshes[ZSKEL_MAXMESHES];
	
	int numAnims;
	zSkel_Config_Anim_t anims[ZSKEL_MAXANIMS];

} zSkel_Config_t;

typedef enum {
	ZSC_GENERAL,
	ZSC_SKELETON,
	ZSC_BONESET,
	ZSC_MESH,
	ZSC_ANIM
} zSkel_Config_Mode_t;

extern zSkel_Config_t config;

void LoadCFG(const char srcFileName[512]);
void LoadCFG_Print(void);
void LoadCFG_General(void);
void LoadCFG_Skeleton(void);
void LoadCFG_BoneSet(void);
void LoadCFG_Mesh(void);
void LoadCFG_Anim(void);

#endif __CONFIG_H__