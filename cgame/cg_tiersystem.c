#include "cg_local.h"
qboolean CG_RegisterClientModelnameWithTiers(clientInfo_t *ci, const char *modelName, const char *skinName){
	int	i;
	char filename[MAX_QPATH * 2];
	char tierPath[MAX_QPATH];
	tier *tier;
	Com_sprintf(filename, sizeof(filename), "players/%s/icon_%s.tga", modelName, skinName);
	ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
	if(!ci->modelIcon){return qfalse;}
	Com_sprintf(filename,sizeof(filename),"players/%s/animation.cfg",modelName);
	if(!CG_ParseAnimationFile(filename,ci)){
		Com_sprintf(filename,sizeof(filename),"players/characters/%s/animation.cfg",modelName);
		if(!CG_ParseAnimationFile(filename,ci)){
			Com_Printf("Failed to load animation file %s\n",filename);
			return qfalse;
		}
	}
	for(i=0;i<8;i++){
		// ===================================
		// Structures
		// ===================================
		Com_sprintf(tierPath,MAX_QPATH,"players/%s/tier%s/",modelName,i);
		ci->tiers[i].index = i;	
		ci->tiers[i].exists = qfalse;
		//if(!(trap_FS_FOpenFile(strcat(tierPath,"tier.cfg"),0,FS_READ) > 0)){return qfalse;}
		ci->tiers[i].exists = qtrue;
		CG_Printf(tierPath);
		ci->tiers[i].soundTransform = trap_S_RegisterSound(strcat(tierPath,"transform.ogg"),qfalse);
		ci->tiers[i].soundTransformFirst = trap_S_RegisterSound(strcat(tierPath,"transformFirst.ogg"),qfalse);
		// ===================================
		// Models
		// ===================================
		Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/lower.md3", modelName, i+1);
		ci->legsModel[i] = trap_R_RegisterModel(filename);
		if(!ci->legsModel[i]){
			if(i == 0){
				Com_Printf("Failed to load model file %s\n", filename);
				return qfalse;
			}else {
				ci->legsModel[i] = ci->legsModel[i - 1];
			}
		}
		Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/upper.md3", modelName, i+1);
		ci->torsoModel[i] = trap_R_RegisterModel(filename);
		if(!ci->torsoModel[i]){
			if(i == 0){
				Com_Printf("Failed to load model file %s\n", filename);
				return qfalse;
			}else {
				ci->torsoModel[i] = ci->torsoModel[i - 1];
			}
		}
		Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/head.md3", modelName, i+1);
		ci->headModel[i] = trap_R_RegisterModel(filename);
		if(!ci->headModel[i]){
			if(i == 0){
				Com_Printf("Failed to load model file %s\n", filename);
				return qfalse;
			}else {
				ci->headModel[i] = ci->headModel[i - 1];
			}
		}
		// ===================================
		// Skins
		// ===================================
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/lower_%s.skin",modelName,i+1,skinName);
		ci->legsSkin[i] = trap_R_RegisterSkin(filename);
		if(!ci->legsSkin[i]){
			if(i == 0){
				Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/lower_default.skin", modelName, i+1);
				ci->legsSkin[i] = trap_R_RegisterSkin (filename);
				if(!ci->legsSkin[i]){
					Com_Printf("Failed to load skin file %s\n", filename);
					return qfalse;
				}
			}else{
				ci->legsSkin[i] = ci->legsSkin[i - 1];
			}
		}
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/upper_%s.skin",modelName,i+1,skinName);
		ci->torsoSkin[i] = trap_R_RegisterSkin(filename);
		if(!ci->torsoSkin[i]){
			if(i == 0){
				Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/upper_default.skin", modelName, i+1);
				ci->torsoSkin[i] = trap_R_RegisterSkin (filename);
				if(!ci->torsoSkin[i]){
					Com_Printf("Failed to load skin file %s\n", filename);
					return qfalse;
				}
			} else {
				ci->torsoSkin[i] = ci->torsoSkin[i - 1];
			}
		}
		Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/head_%s.skin", modelName, i+1, skinName);
		ci->headSkin[i] = trap_R_RegisterSkin(filename);
		if(!ci->headSkin[i]){
			if(i == 0){
				Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/head_default.skin", modelName, i+1);
				ci->headSkin[i] = trap_R_RegisterSkin (filename);
				if(!ci->headSkin[i]){
					Com_Printf("Failed to load skin file %s\n", filename);
					return qfalse;
				}
			} else {
				ci->headSkin[i] = ci->headSkin[i - 1];
			}
		}
	}

	return qtrue;
}
