#include "cg_local.h"
void parseTier(char *path,tierConfig_cg *tier);
qboolean CG_RegisterClientModelnameWithTiers(clientInfo_t *ci, const char *modelName, const char *skinName){
	int	i;
	char filename[MAX_QPATH * 2];
	char tierPath[MAX_QPATH];
	char tempPath[MAX_QPATH];
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
		// Config
		// ===================================
		memset(&ci->tierConfig[i],0,sizeof(tierConfig_cg));
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		ci->tierConfig[i].icon = trap_R_RegisterShaderNoMip(strcat(tierPath,"icon.png"));
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		ci->tierConfig[i].soundTransformFirst = trap_S_RegisterSound(strcat(tierPath,"transformFirst.ogg"),qfalse);
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		ci->tierConfig[i].soundTransformUp = trap_S_RegisterSound(strcat(tierPath,"transformUp.ogg"),qfalse);
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		ci->tierConfig[i].soundTransformDown = trap_S_RegisterSound(strcat(tierPath,"transformDown.ogg"),qfalse);
		Com_sprintf(filename,sizeof(filename),"players/tierDefault.cfg",modelName,i+1);
		parseTier(filename,&ci->tierConfig[i]);
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/tier.cfg",modelName,i+1);
		parseTier(filename,&ci->tierConfig[i]);
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
void parseTier(char *path,tierConfig_cg *tier){
	fileHandle_t tierCFG;
	int i;
	char *token,*parse;
	int fileLength;
	char fileContents[32000];
	if(trap_FS_FOpenFile(path,0,FS_READ)>0){
		fileLength = trap_FS_FOpenFile(path,&tierCFG,FS_READ);
		trap_FS_Read(fileContents,fileLength,tierCFG);
		fileContents[fileLength] = 0;
		trap_FS_FCloseFile(tierCFG);
		parse = fileContents;
		while(1){
			token = COM_Parse(&parse);
			if(!token[0]){break;}
			else if(!Q_stricmp(token,"tierName")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				//tier->name = token;
			}
			else if(!Q_stricmp(token,"transformSoundFirst")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->soundTransformFirst = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp(token,"tierIcon")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->icon = trap_R_RegisterShaderNoMip(token);
			}
			else if(!Q_stricmp(token,"powerLevelHudMultiplier")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->hudMultiplier = atof(token);
			}
			else if(!Q_stricmp(token,"transformSoundFirst")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->soundTransformFirst = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp(token,"transformSoundUp")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->soundTransformUp = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp(token,"transformSoundDown")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->soundTransformDown = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp(token,"transformCameraDefault")){
				for(i=0;i<3;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraDefault[i] = atoi(token);
				}
			}
			else if(!Q_stricmp(token,"transformCameraOrbit")){
				for(i=0;i<2;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraOrbit[i] = atoi(token);
				}
			}
			else if(!Q_stricmp(token,"transformCameraZoom")){
				for(i=0;i<2;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraZoom[i] = atoi(token);
				}
			}
			else if(!Q_stricmp(token,"transformCameraPan")){
				for(i=0;i<2;i++){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraPan[i] = atoi(token);
				}
			}
		}
	}
}
