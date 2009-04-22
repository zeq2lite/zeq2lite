#include "g_local.h"
static tierList tierInfos[MAX_CLIENTS];
void parseTier(char *path,tierConfig_g *tier);
void setupTiers(int clientNum,char *modelName){
	int	i;
	tierList *list;
	tierConfig_g *tier;
	char filename[MAX_QPATH * 2];
	char tierPath[MAX_QPATH];
	char tempPath[MAX_QPATH];
	list = &tierInfos[clientNum];
	for(i=0;i<8;i++){
		tier = &list->tiers[i];
		memset(tier,0,sizeof(tierConfig_g));
		Com_sprintf(tierPath,sizeof(tierPath),"players/%s/tier%i/",modelName,i+1);
		Com_sprintf(filename,sizeof(filename),"players/tierDefault.cfg",modelName,i+1);
		parseTier(filename,tier);
		Com_sprintf(filename,sizeof(filename),"players/%s/tier%i/tier.cfg",modelName,i+1);
		parseTier(filename,tier);
	}
}
void parseTier(char *path,tierConfig_g *tier){
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
			else if(!Q_stricmp(token,"tierSpeed")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->speed = atoi(token);
			}
			else if(!Q_stricmp(token,"tierMeleeAttack")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->meleeAttack = atof(token);
			}
			else if(!Q_stricmp(token,"tierMeleeDefense")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->meleeDefense = atoi(token);
			}
			else if(!Q_stricmp(token,"tierEnergyDefense")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyDefense = atoi(token);
			}
			else if(!Q_stricmp(token,"tierEnergyAttackDamage")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyAttackDamage = atof(token);
			}
			else if(!Q_stricmp(token,"tierEnergyAttackCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyAttackCost = atof(token);
			}
			else if(!Q_stricmp(token,"tierZanzokenCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenCost = atoi(token);
			}
			else if(!Q_stricmp(token,"tierZanzokenDistance")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenDistance = atoi(token);
			}
			else if(!Q_stricmp(token,"powerLevelChargeRate")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelChargeRate = atof(token);
			}
			else if(!Q_stricmp(token,"powerLevelBreakLimitRate")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelBreakLimitRate = atof(token);
			}
			else if(!Q_stricmp(token,"powerLevelDrain")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelDrain = atoi(token);
			}
			else if(!Q_stricmp(token,"powerLevelTotalDrain")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelTotalDrain = atoi(token);
			}
			else if(!Q_stricmp(token,"powerLevelMaximumDrain")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelMaximumDrain = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementPowerLevelCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementPowerLevelCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementPowerLevelTotal")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementPowerLevelTotal = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementPowerLevelMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementPowerLevelMaximum = atoi(token);
			}
			else if(!Q_stricmp(token,"transformTime")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformTime = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementPowerLevelButton")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementPowerLevelButton = token == "True" ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"tierPermanent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->permanent = token == "True" ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"tierCustomWeapons")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->customWeapons = token == "True" ? qtrue : qfalse;
			}
		}
	}
}
