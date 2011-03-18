#include "cg_local.h"
void parseTier(char *path,tierConfig_cg *tier);
qboolean CG_RegisterClientModelnameWithTiers(clientInfo_t *ci, const char *modelName, const char *skinName){
	int	i,partIndex,damageIndex,lastSkinIndex,lastModelIndex;
	char filename[MAX_QPATH * 2];
	char tierPath[MAX_QPATH];
	char tempPath[MAX_QPATH];
	char legsPath[MAX_QPATH];
	char headPath[MAX_QPATH];
	char headPrefix[8],upperPrefix[8],lowerPrefix[8];
	strcpy(headPrefix,"head_");
	strcpy(lowerPrefix,"lower_");
	strcpy(upperPrefix,"upper_");
	Com_sprintf(legsPath,sizeof(legsPath),"%s",modelName);
	Com_sprintf(headPath,sizeof(headPath),"%s",modelName);
	Com_sprintf(tempPath,sizeof(tempPath),"players/%s/animation.cfg",ci->legsModelName);
	if(ci->legsModelName && trap_FS_FOpenFile(tempPath,0,FS_READ)>0){Com_sprintf(legsPath,sizeof(legsPath),"%s",ci->legsModelName);}
	Com_sprintf(tempPath,sizeof(tempPath),"players/%s/animation.cfg",ci->headModelName);
	if(ci->headModelName && trap_FS_FOpenFile(tempPath,0,FS_READ)>0){Com_sprintf(headPath,sizeof(headPath),"%s",ci->headModelName);}
	//Com_Printf("RegisterClientModelnameWithTiers: before animation load ci->usingMD4 = %i\n", ci->usingMD4);
	Com_sprintf(filename,sizeof(filename),"players/%s/animation.cfg",modelName);
	if(!CG_ParseAnimationFile(filename,ci)){
		Com_sprintf(filename,sizeof(filename),"players/characters/%s/animation.cfg",modelName);
		if(!CG_ParseAnimationFile(filename,ci)){
			//Com_Printf("Failed to load animation file %s\n",filename);
			return qfalse;
		}
	}
	//Com_Printf("RegisterClientModelnameWithTiers: after animation load ci->usingMD4 = %i\n", ci->usingMD4);
	if(ci->usingMD4 == qtrue) // Alex Adding
	{
		//Com_Printf("MD4 Part of RegisterClientModelnameWithTiers\n");
		// TODO: MD4 Specific set up later on if needed...
	}
	for(i=0;i<8;++i){
		// ===================================
		// Config
		// ===================================
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/icon.png",modelName,i+1);
		if(trap_FS_FOpenFile(tierPath,0,FS_READ)<=0){continue;}
		memset(&ci->tierConfig[i],0,sizeof(tierConfig_cg));
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		ci->tierConfig[i].icon = trap_R_RegisterShaderNoMip(strcat(tierPath,"icon.png"));
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		if(trap_FS_FOpenFile(strcat(tierPath,"transformFirst.ogg"),0,FS_READ)>0){
			Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
			ci->tierConfig[i].soundTransformFirst = trap_S_RegisterSound(strcat(tierPath,"transformFirst.ogg"),qfalse);
		}
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		if(trap_FS_FOpenFile(strcat(tierPath,"transformUp.ogg"),0,FS_READ)>0){
			Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
			ci->tierConfig[i].soundTransformUp = trap_S_RegisterSound(strcat(tierPath,"transformUp.ogg"),qfalse);
		}
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		if(trap_FS_FOpenFile(strcat(tierPath,"transformDown.ogg"),0,FS_READ)>0){
			Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
			ci->tierConfig[i].soundTransformDown = trap_S_RegisterSound(strcat(tierPath,"transformDown.ogg"),qfalse);
		}
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		if(trap_FS_FOpenFile(strcat(tierPath,"poweringUp.ogg"),0,FS_READ)>0){
			Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
			ci->tierConfig[i].soundPoweringUp = trap_S_RegisterSound(strcat(tierPath,"poweringUp.ogg"),qfalse);
		}
		Com_sprintf(filename,sizeof(filename),"players/tierDefault.cfg",modelName,i+1);
		parseTier(filename,&ci->tierConfig[i]);
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/tier.cfg",modelName,i+1);
		parseTier(filename,&ci->tierConfig[i]);
		// ===================================
		// Models
		// ===================================
		Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/body.zMesh", modelName, i+1);
		if(ci->usingMD4 && trap_FS_FOpenFile(filename,0,FS_READ)>0){
			ci->legsModel[i] = trap_R_RegisterModel(filename);
		}
		else{
			Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/lower.md3", legsPath, i+1);
			if(trap_FS_FOpenFile(filename,0,FS_READ)>0){ci->legsModel[i] = trap_R_RegisterModel(filename);}
			else{
				if(i == 0){return qfalse;}
				else{ci->legsModel[i] = ci->legsModel[i - 1];}
			}
			Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/upper.md3", modelName, i+1);
			if(trap_FS_FOpenFile(filename,0,FS_READ)>0){ci->torsoModel[i] = trap_R_RegisterModel(filename);}
			else{
				if(i == 0){return qfalse;}
				else{ci->torsoModel[i] = ci->torsoModel[i - 1];}
			}
			Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/head.md3", headPath, i+1);
			if(trap_FS_FOpenFile(filename,0,FS_READ)>0){ci->headModel[i] = trap_R_RegisterModel(filename);}
			else{
				if(i == 0){return qfalse;}
				else{ci->headModel[i] = ci->headModel[i - 1];}
			}
		}
		// ===================================
		// Skins
		// ===================================
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/%s.skin",legsPath,i+1,skinName);
		if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
			strcpy(headPrefix,"");
			strcpy(lowerPrefix,"");
			strcpy(upperPrefix,"");
		}
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/%s%s.skin",legsPath,i+1,skinName,lowerPrefix);
		if(trap_FS_FOpenFile(filename,0,FS_READ)>0){ci->legsSkin[i] = trap_R_RegisterSkin(filename);}
		else if(i!=0){ci->legsSkin[i] = ci->legsSkin[i - 1];}
		else{return qfalse;}
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/%s%s.skin",modelName,i+1,skinName,upperPrefix);
		if(trap_FS_FOpenFile(filename,0,FS_READ)>0){ci->torsoSkin[i] = trap_R_RegisterSkin(filename);}
		else if(i!=0){ci->torsoSkin[i] = ci->torsoSkin[i - 1];}	
		else{return qfalse;}
		Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/%s%s.skin", headPath, i+1,skinName,headPrefix);
		if(trap_FS_FOpenFile(filename,0,FS_READ)>0){ci->headSkin[i] = trap_R_RegisterSkin(filename);}
		else if(i!=0){ci->headSkin[i] = ci->headSkin[i - 1];}
		else{return qfalse;}
		// ===================================
		// Damage States
		// ===================================
		for(partIndex=0;partIndex<3;++partIndex){
			lastSkinIndex = -1;
			lastModelIndex = -1;
			for(damageIndex=9;damageIndex>=0;--damageIndex){
				if(partIndex==0){
					Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/head_percent%i.md3",headPath,i+1,(damageIndex+1)*10);
					if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
						ci->modelDamageState[i][partIndex][damageIndex] = trap_R_RegisterModel(filename);
						lastModelIndex = damageIndex;
					}
					else{ci->modelDamageState[i][partIndex][damageIndex] = lastModelIndex != -1? ci->modelDamageState[i][partIndex][lastModelIndex] : ci->headModel[i];}
					Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/%s%s_percent%i.skin",headPath,i+1,headPrefix,skinName,(damageIndex+1)*10);
					if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
						ci->skinDamageState[i][partIndex][damageIndex] = trap_R_RegisterSkin(filename);
						lastSkinIndex = damageIndex;
					}
					else{ci->skinDamageState[i][partIndex][damageIndex] = lastSkinIndex != -1 ? ci->skinDamageState[i][partIndex][lastSkinIndex] : ci->headSkin[i];}
				}
				else if(partIndex==1){
					Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/upper_percent%i.md3",modelName,i+1,(damageIndex+1)*10);
					if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
						ci->modelDamageState[i][partIndex][damageIndex] = trap_R_RegisterModel(filename);
						lastModelIndex = damageIndex;
					}
					else{ci->modelDamageState[i][partIndex][damageIndex] = lastModelIndex != -1? ci->modelDamageState[i][partIndex][lastModelIndex] : ci->torsoModel[i];}
					Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/%s%s_percent%i.skin",modelName,i+1,upperPrefix,skinName,(damageIndex+1)*10);
					if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
						ci->skinDamageState[i][partIndex][damageIndex] = trap_R_RegisterSkin(filename);
						lastSkinIndex = damageIndex;
					}
					else{ci->skinDamageState[i][partIndex][damageIndex] = lastSkinIndex != -1 ? ci->skinDamageState[i][partIndex][lastSkinIndex] : ci->torsoSkin[i];}
				}
				else if(partIndex==2){
					Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/lower_percent%i.md3",legsPath,i+1,(damageIndex+1)*10);
					if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
						ci->modelDamageState[i][partIndex][damageIndex] = trap_R_RegisterModel(filename);
						lastModelIndex = damageIndex;
					}
					else{ci->modelDamageState[i][partIndex][damageIndex] = lastModelIndex != -1? ci->modelDamageState[i][partIndex][lastModelIndex] : ci->legsModel[i];}
					Com_sprintf(filename, sizeof(filename), "players/%s/tier%i/%s%s_percent%i.skin",legsPath,i+1,lowerPrefix,skinName,(damageIndex+1)*10);
					if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
						ci->skinDamageState[i][partIndex][damageIndex] = trap_R_RegisterSkin(filename);
						lastSkinIndex = damageIndex;
					}
					else{ci->skinDamageState[i][partIndex][damageIndex] = lastSkinIndex != -1 ? ci->skinDamageState[i][partIndex][lastSkinIndex] : ci->legsSkin[i];}
				}
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
				tier->name = token;
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
			else if(!Q_stricmp(token,"sustainCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainMaximum = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainFatigue = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainHealth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainHealth = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementFatigue = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementMaximum = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementHealth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementHealth = atoi(token);
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
			else if(!Q_stricmp(token,"poweringUpSound")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->soundPoweringUp = trap_S_RegisterSound(token,qfalse);
			}
			else if(!Q_stricmp(token,"damageFeatures")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->damageFeatures = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"damageModelsRevertHealed")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->damageModelsRevertHealed = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"damageTexturesRevertHealed")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->damageTexturesRevertHealed = strlen(token) == 4 ? qtrue : qfalse;
			}
		}
	}
}
