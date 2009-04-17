#include "cg_local.h"
qboolean CG_RegisterClientModelnameWithTiers(clientInfo_t *ci, const char *modelName, const char *skinName){
	int	i;
	tierConfig_t *tier;
	char filename[MAX_QPATH * 2];
	char tierPath[MAX_QPATH];
	Com_sprintf(filename, sizeof(filename), "players/%s/icon_%s.tga", modelName, skinName);
	ci->modelIcon = trap_R_RegisterShaderNoMip(filename);
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
		fileHandle_t tierCFG;
		char fileContents[32000];
		char *parse,*token;
		int fileLength;
		Com_sprintf(tierPath,MAX_QPATH,"players/%s/tier%s/",modelName,i);
		Com_sprintf(filename,sizeof(filename),"players//%s/tier%i/tier.cfg",modelName,i+1);
		if(trap_FS_FOpenFile(filename,0,FS_READ)>0){
			tier = &ci->tierConfig[i];
			fileLength = trap_FS_FOpenFile(filename,&tierCFG,FS_READ);
			trap_FS_Read(fileContents,fileLength,tierCFG);
			fileContents[fileLength] = 0;
			trap_FS_FCloseFile(tierCFG);
			memset(tier,0,sizeof(tierConfig_t));
			//tier->name = "Normal";
			tier->icon = trap_R_RegisterShaderNoMip(strcat(tierPath,"icon.png"));
			parse = fileContents;
			while(1){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				else if(!Q_stricmp(token,"tierName")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					//tier->name = token;
				}
				else if(!Q_stricmp(token,"tierIcon")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->icon = trap_R_RegisterShaderNoMip(token);
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
				else if(!Q_stricmp(token,"transformCameraType")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					//tier->transformCameraType = token;
				}
				else if(!Q_stricmp(token,"transformCameraAngle")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraAngle = atoi(token);
				}
				else if(!Q_stricmp(token,"transformCameraHeight")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraHeight = atoi(token);
				}
				else if(!Q_stricmp(token,"transformCameraRange")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformCameraAngle = atoi(token);
				}
				else if(!Q_stricmp(token,"transformTime")){
					token = COM_Parse(&parse);
					if(!token[0]){break;}
					tier->transformTime = atoi(token);
				}
			}
		}
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
