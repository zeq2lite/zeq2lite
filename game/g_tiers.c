#include "g_local.h"
void parseTier(char *path,tierConfig_g *tier);
void syncTier(gclient_t *client){
	tierConfig_g *tier;
	playerState_t *ps;
	ps = &client->ps;
	tier = &client->tiers[ps->stats[tierCurrent]];
	ps->powerLevelBreakLimitRate = tier->powerLevelBreakLimitRate * g_powerLevelBreakLimitRate.value;
	ps->drainPowerLevel = tier->powerLevelEffect;
	ps->drainPowerLevelTotal = tier->powerLevelTotalEffect;
	ps->drainPowerLevelMaximum = tier->powerLevelMaximumEffect;
	ps->speed = tier->speed;
	ps->meleeDefense = tier->meleeDefense;
	ps->meleeAttack = tier->meleeAttack;
	ps->energyDefense = tier->energyDefense;
	ps->energyAttack = tier->energyAttackDamage;
	ps->energyCost = tier->energyAttackCost;
}
void checkTier(gclient_t *client){
	int tier;
	playerState_t *ps;
	tierConfig_g *nextTier,*baseTier;
	ps = &client->ps;
	if(ps->powerups[PW_TRANSFORM]){return;}
	while(1){
		tier = ps->stats[tierCurrent];
		if(((tier+1) < 8) && (client->tiers[tier+1].exists)){
			nextTier = &client->tiers[tier+1];
			if(((nextTier->requirementPowerLevelButton && ps->stats[bitFlags] & keyTierUp) ||
			   (!nextTier->requirementPowerLevelButton && !(ps->stats[bitFlags] & keyTierUp))) &&
			   (ps->stats[powerLevel] >= nextTier->requirementPowerLevelCurrent) &&
			   (ps->stats[powerLevelTotal] >= nextTier->requirementPowerLevelTotal) &&
			   (ps->persistant[powerLevelMaximum] >= nextTier->requirementPowerLevelMaximum)){
				ps->powerups[PW_TRANSFORM] = 1;
				++ps->stats[tierCurrent];
				if(tier + 1 > ps->stats[tierTotal]){
					ps->stats[tierTotal] = ps->stats[tierCurrent];
					ps->powerups[PW_TRANSFORM] = client->tiers[tier+1].transformTime;
				}
				continue;
			}
		}
		if(tier > 0){
			baseTier = &client->tiers[tier];
			if(((baseTier->requirementPowerLevelButton && ps->stats[bitFlags] & keyTierDown) ||
			   (!baseTier->requirementPowerLevelButton && !(ps->stats[bitFlags] & keyTierDown))) ||
			   (ps->stats[powerLevel] < baseTier->sustainPowerLevelCurrent) ||
			   (ps->stats[powerLevelTotal] < baseTier->sustainPowerLevelTotal)){
				ps->powerups[PW_TRANSFORM] = -1;
				--ps->stats[tierCurrent];
				continue;
			}
		}
		break;
	}
}
void setupTiers(gclient_t *client){
	int	i;
	tierConfig_g *tier;
	char *modelName;
	char *tierPath;
	char tempPath[MAX_QPATH];
	modelName = client->modelName;
	for(i=0;i<8;i++){
		tier = &client->tiers[i];
		memset(tier,0,sizeof(tierConfig_g));
		tierPath = va("players/%s/tier%i/",modelName,i+1);
		parseTier("players/tierDefault.cfg",tier);
		parseTier(strcat(tierPath,"tier.cfg"),tier);
	}
	syncTier(client);
}
void parseTier(char *path,tierConfig_g *tier){
	fileHandle_t tierCFG;
	int i;
	char *token,*parse;
	int fileLength;
	char fileContents[32000];
	tier->exists = qfalse;
	if(trap_FS_FOpenFile(path,0,FS_READ)>0){
		tier->exists = qtrue;
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
			else if(!Q_stricmp(token,"percentMeleeAttack")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->meleeAttack = atof(token);
			}
			else if(!Q_stricmp(token,"percentMeleeDefense")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->meleeDefense = atoi(token);
			}
			else if(!Q_stricmp(token,"percentEnergyDefense")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyDefense = atoi(token);
			}
			else if(!Q_stricmp(token,"percentEnergyAttackDamage")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyAttackDamage = atof(token);
			}
			else if(!Q_stricmp(token,"percentEnergyAttackCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyAttackCost = atof(token);
			}
			else if(!Q_stricmp(token,"tierZanzokenCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenCost = atoi(token);
			}
			else if(!Q_stricmp(token,"tierZanzokenSpeed")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenSpeed = atoi(token);
			}
			else if(!Q_stricmp(token,"tierZanzokenDistance")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenDistance = atoi(token);
			}
			else if(!Q_stricmp(token,"powerLevelBreakLimitRate")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelBreakLimitRate = atof(token);
			}
			else if(!Q_stricmp(token,"powerLevelEffect")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelEffect = atoi(token);
			}
			else if(!Q_stricmp(token,"powerLevelTotalEffect")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelTotalEffect = atoi(token);
			}
			else if(!Q_stricmp(token,"powerLevelMaximumEffect")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->powerLevelMaximumEffect = atoi(token);
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
			else if(!Q_stricmp(token,"sustainPowerLevelCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainPowerLevelCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainPowerLevelTotal")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainPowerLevelTotal = atoi(token);
			}
			else if(!Q_stricmp(token,"transformTime")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformTime = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementPowerLevelButton")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementPowerLevelButton = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"tierPermanent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->permanent = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"tierCustomWeapons")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->customWeapons = strlen(token) == 4 ? qtrue : qfalse;
			}
		}
	}
}
