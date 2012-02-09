#include "g_local.h"
void parseTier(char *path,tierConfig_g *tier);
void syncTier(gclient_t *client){
	tierConfig_g *tier;
	playerState_t *ps;
	ps = &client->ps;
	tier = &client->tiers[ps->powerLevel[plTierCurrent]];
	ps->breakLimitRate = (float)tier->breakLimitRate * g_breakLimitRate.value;
	ps->options = tier->capabilities | (ps->options & advancedMelee) | (ps->options & advancedFlight) | (ps->options & pointGravity);
	ps->stats[stTransformFirstDuration] = tier->transformFirstDuration;
	ps->stats[stTransformFirstEffectMaximum] = tier->transformFirstEffectMaximum;
	ps->stats[stTransformFirstHealth] = tier->transformFirstHealth;
	ps->stats[stTransformFirstFatigue] = tier->transformFirstFatigue;
	ps->stats[stTransformDuration] = tier->transformDuration;
	ps->stats[stTransformFatigue] = tier->transformFatigue;
	ps->stats[stTransformHealth] = tier->transformHealth;
	ps->stats[stTransformEffectMaximum] = tier->transformEffectMaximum;
	ps->baseStats[stSpeed] = tier->speed;
	ps->baseStats[stZanzokenDistance] = tier->zanzokenDistance;
	ps->baseStats[stZanzokenQuickDistance] = g_quickZanzokenDistance.value != -1.0 ? g_quickZanzokenDistance.value : tier->zanzokenQuickDistance;
	ps->baseStats[stZanzokenSpeed] = tier->zanzokenSpeed;
	ps->baseStats[stZanzokenCost] = tier->zanzokenCost;
	ps->baseStats[stZanzokenQuickCost] = g_quickZanzokenCost.value != -1.0 ? g_quickZanzokenCost.value : tier->zanzokenQuickCost;
	ps->baseStats[stBoostCost] = tier->boostCost;
	ps->baseStats[stFatigueRecovery] = tier->fatigueRecovery;
	ps->baseStats[stTransformSubsequentDuration] = tier->transformSubsequentDuration;
	ps->baseStats[stTransformSubsequentFatigueScale] = tier->transformSubsequentFatigueScale;
	ps->baseStats[stTransformSubsequentHealthScale] = tier->transformSubsequentHealthScale;
	ps->baseStats[stTransformSubsequentMaximumScale] = tier->transformSubsequentMaximumScale;
	ps->baseStats[stAirBrakeCost] = tier->airBrakeCost;
	ps->baseStats[stMeleeAttack] = tier->meleeAttack;
	ps->baseStats[stEnergyAttack] = tier->energyAttackDamage;
	ps->baseStats[stDefenseMelee] = tier->defenseMelee;
	ps->baseStats[stDefenseEnergy] = tier->defenseEnergy;
	ps->baseStats[stEnergyAttackCost] = tier->energyAttackCost;
	ps->powerLevel[plDrainCurrent] = tier->effectCurrent;
	ps->powerLevel[plDrainFatigue] = tier->effectFatigue;
	ps->powerLevel[plDrainHealth] = tier->effectHealth;
	ps->powerLevel[plDrainMaximum] = tier->effectMaximum;
}

qboolean checkTierUpTransformation(gclient_t *client, int nextTierIndex, int currentTierIndex, int tierChangeMode){
	qboolean hasRequirementsToTransform = qfalse;
	playerState_t *ps;
	tierConfig_g *nextTier, *currentTier;
	int newPowerLevel;
	ps = &client->ps;
	if(tierChangeMode > 1 )	{
		if(nextTierIndex > -1 && ((nextTierIndex) < 8) && (client->tiers[nextTierIndex].exists)){
			nextTier = &client->tiers[nextTierIndex];
			currentTier = &client->tiers[currentTierIndex];
			hasRequirementsToTransform =
					   (((ps->powerLevel[plCurrent] >= nextTier->requirementCurrent)  &&
					   (ps->powerLevel[plCurrent] >= nextTier->requirementCurrentPercent / 100.0f * ps->powerLevel[plMaximum])) ||
					   ((ps->powerLevel[plMaximum] >= nextTier->requirementCurrent)  &&
					   (ps->powerLevel[plMaximum] >= nextTier->requirementCurrentPercent / 100.0f * ps->powerLevel[plMaximum]) &&
					   tierChangeMode == 3))  &&
					   (ps->powerLevel[plFatigue] >= nextTier->requirementFatigue) &&
					   (ps->powerLevel[plHealth]  >= nextTier->requirementHealth) &&
					   (ps->powerLevel[plHealth]  <= nextTier->requirementHealthMaximum / 100.0f * ps->powerLevel[plMaximum]) &&
					   (ps->powerLevel[plMaximum] >= nextTier->requirementMaximum) &&
					   (((ps->powerLevel[plCurrent] >= nextTier->sustainCurrent) &&
					   (ps->powerLevel[plCurrent] >= nextTier->sustainCurrentPercent / 100.0f * ps->powerLevel[plMaximum])) ||
					   ((ps->powerLevel[plMaximum] >= nextTier->sustainCurrent) &&
					   (ps->powerLevel[plMaximum] >= nextTier->sustainCurrentPercent / 100.0f * ps->powerLevel[plMaximum]) &&
					   tierChangeMode == 3)) &&
					   (ps->powerLevel[plHealth]  >= nextTier->sustainHealth) &&
					   (ps->powerLevel[plFatigue] >= nextTier->sustainFatigue) &&
					   (ps->powerLevel[plMaximum] >= nextTier->sustainMaximum);

				if((((nextTier->requirementButtonUp && (tierChangeMode > 0)) || !nextTier->requirementButtonUp) &&
						hasRequirementsToTransform == 1) ){
					ps->timers[tmTransform] = 1;
					ps->powerLevel[plTierCurrent] = nextTierIndex;
					ps->powerLevel[plTierDesired] = nextTierIndex;
					ps->powerLevel[plTierChanged] = 1;
					newPowerLevel = (int)(nextTier->requirementCurrent * 1.1 );
					if(nextTierIndex > ps->powerLevel[plTierTotal]){
						ps->powerLevel[plTierTotal] = nextTierIndex;
						ps->timers[tmTransform] = client->tiers[ps->powerLevel[plTierCurrent]].transformTime;
						ps->stats[stTransformState] = 2;
						if(ps->powerLevel[plCurrent] < nextTier->requirementCurrent) {
							ps->powerLevel[plCurrent] = newPowerLevel ;
						}
					}
					else {
						ps->stats[stTransformState] = 1;
						if(tierChangeMode==3) {
							if(ps->powerLevel[plCurrent] < nextTier->requirementCurrent) {
								ps->powerLevel[plFatigue] -= (newPowerLevel - ps->powerLevel[plCurrent]) *(g_quickTransformCost.value +( (nextTierIndex - currentTierIndex) * g_quickTransformCostPerTier.value)) ;
								ps->powerLevel[plCurrent] = newPowerLevel ;
							}
						}
					}
					ps->powerLevel[plTierSelectionMode]=0;
					return qtrue;
				}
				else if (!hasRequirementsToTransform && tierChangeMode==3) {
					ps->powerLevel[plTierDesired] = -1;
					ps->powerLevel[plTierCurrent] = currentTierIndex;
					ps->powerLevel[plTierChanged] = 1;
				}
				if(tierChangeMode>0) {
					ps->powerLevel[plTierChanged] = 1;
					ps->powerLevel[plTierSelectionMode]=0;
				}
			}
	}
	return qfalse;
}

qboolean checkTierDownTransformation(gclient_t *client, int nextTierIndex, int currentTierIndex, int tierChangeMode){
	playerState_t *ps;
	tierConfig_g *baseTier, *currentTier;
	int newPowerLevel;
	ps = &client->ps;
	if(tierChangeMode == 1 || tierChangeMode == 3){
		if(nextTierIndex > -1 ){
			currentTier = &client->tiers[currentTierIndex];
			baseTier = &client->tiers[(nextTierIndex)];
			if(!baseTier->permanent && !currentTier->permanent && (((baseTier->requirementButtonDown && tierChangeMode > 0 ) || !baseTier->requirementButtonDown) ||
			   (ps->powerLevel[plCurrent] < baseTier->sustainCurrent) ||
			   (ps->powerLevel[plCurrent] < baseTier->sustainCurrentPercent / 100.0f * ps->powerLevel[plMaximum]) ||
			   (ps->powerLevel[plHealth] < baseTier->sustainHealth) ||
			   (ps->powerLevel[plFatigue] < baseTier->sustainFatigue) ||
			   (ps->powerLevel[plMaximum] < baseTier->sustainMaximum))){
				if( !currentTier->permanent) {
					ps->timers[tmTransform] = -1;
					ps->stats[stTransformState] = -1;
					ps->powerLevel[plTierChanged] = 1;
					if(tierChangeMode==3) {
						newPowerLevel = (int)((currentTier->requirementCurrent + baseTier->requirementCurrent) * 0.5);
						if( ps->powerLevel[plCurrent] > newPowerLevel) {
							ps->powerLevel[plCurrent] = newPowerLevel ;
						}
					}
					ps->powerLevel[plTierCurrent] = nextTierIndex;
				}
				ps->powerLevel[plTierDesired] = ps->powerLevel[plTierCurrent];
				ps->powerLevel[plTierSelectionMode]=0;
				return qtrue;
			}
		}
	}
	return qfalse;
}

qboolean checkDefaultTierConfiguration(playerState_t *ps){
	//The value of 2 means the player was spawned, and the default tier configuration must be sent to the client
	if(ps->powerLevel[plTierChanged] == 2) {
		if(ps->powerLevel[plTierCurrent] == 0 && ps->powerLevel[plTierDesired] == 0) {
			ps->powerLevel[plTierChanged] = 1;
		}
		ps->powerLevel[plTierDesired] = 0;
		ps->powerLevel[plTierCurrent] = 0;
		ps->powerLevel[plTierSelectionMode]=-1;
		return qtrue;
	}
	return qfalse;
}

void checkTier(gclient_t *client){
	int trigger;
	int tier = 1;
	int tierUp = -1;
	int tierDown = -1;
	int desiredTier = -1;
	int tierChangeMode = 0;
	playerState_t *ps;
	ps = &client->ps;
	if(ps->timers[tmTransform]){return;}
	while(client->tiers[tier].exists){
		if(client->tiers[tier].requirementUseSkill && (ps->weapon == client->tiers[tier].requirementUseSkill)){
			trigger = tier;
			break;
		}
		++tier;
	}

	desiredTier = ps->powerLevel[plTierDesired];
	tier = ps->powerLevel[plTierCurrent];
	if(checkDefaultTierConfiguration(ps)) { return; }
	ps->powerLevel[plTierChanged] = 0;
	if(ps->stats[stTransformState] != 0) {
		ps->powerLevel[plTierChanged] = 1;
	}
	else if(desiredTier == tier || ps->powerLevel[plTierSelectionMode] < 3) {
		tierChangeMode = ps->powerLevel[plTierSelectionMode];
		if(tierChangeMode == 3) { tierChangeMode = 0; }
		if(ps->bitFlags & keyTierUp) { tierChangeMode = 2; }
		if(ps->bitFlags & keyTierDown) { tierChangeMode = 1; }
		tierUp = tier + 1;
		tierDown = tier - 1;
	}
	else if(!client->tiers[desiredTier].exists) {
		 ps->powerLevel[plTierDesired] = -1;
		 ps->powerLevel[plTierSelectionMode]=0;
		 ps->powerLevel[plTierChanged] = 1;
		return;
	}
	else if(ps->powerLevel[plTierSelectionMode] == 3) {
		ps->powerLevel[plTierSelectionMode] = 0;
		if( desiredTier > tier ) {
			if(desiredTier > ps->powerLevel[plTierTotal]+1) {
				ps->powerLevel[plTierDesired] = tier;
				return;
			}
			tierUp = desiredTier;
		}
		else if (desiredTier < tier) { tierDown = desiredTier; }
		tierChangeMode = 3;
	}
	if(!checkTierUpTransformation(client,tierUp,tier,tierChangeMode)) { checkTierDownTransformation(client,tierDown,tier,tierChangeMode); }
	ps->powerLevel[plTierSelectionMode]=0;
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
			else if(!Q_stricmp(token,"speed")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->speed = atof(token);
			}
			else if(!Q_stricmp(token,"meleeAttack")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->meleeAttack = atof(token);
			}
			else if(!Q_stricmp(token,"energyAttackDamage")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyAttackDamage = atof(token);
			}
			else if(!Q_stricmp(token,"energyAttackCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->energyAttackCost = atof(token);
			}
			else if(!Q_stricmp(token,"defenseMelee")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->defenseMelee = atof(token);
			}
			else if(!Q_stricmp(token,"defenseEnergy")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->defenseEnergy = atof(token);
			}
			else if(!Q_stricmp(token,"defenseCapacity")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->defenseCapacity = atof(token);
			}
			else if(!Q_stricmp(token,"defenseRecovery")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->defenseRecovery = atof(token);
			}
			else if(!Q_stricmp(token,"defenseRecoveryDelay")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->defenseRecoveryDelay = atof(token);
			}
			else if(!Q_stricmp(token,"knockbackPower")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->knockbackPower = atof(token);
			}
			else if(!Q_stricmp(token,"knockbackIntensity")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->knockbackIntensity = atof(token);
			}
			else if(!Q_stricmp(token,"airBrakeCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->airBrakeCost = atof(token);
			}
			else if(!Q_stricmp(token,"fatigueRecovery")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->fatigueRecovery = atof(token);
			}
			else if(!Q_stricmp(token,"boostCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->boostCost = atof(token);
			}
			else if(!Q_stricmp(token,"zanzokenCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenCost = atof(token);
			}
			else if(!Q_stricmp(token,"zanzokenQuickCost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenQuickCost = atof(token);
			}
			else if(!Q_stricmp(token,"zanzokenSpeed")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenSpeed = atof(token);
			}
			else if(!Q_stricmp(token,"zanzokenDistance")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenDistance = atof(token);
			}
			else if(!Q_stricmp(token,"zanzokenQuickDistance")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->zanzokenQuickDistance = atof(token);
			}
			else if(!Q_stricmp(token,"breakLimitRate")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->breakLimitRate = atof(token);
			}
			else if(!Q_stricmp(token,"effectCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->effectCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"effectMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->effectMaximum = atoi(token);
			}
			else if(!Q_stricmp(token,"effectFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->effectFatigue = atoi(token);
			}
			else if(!Q_stricmp(token,"effectHealth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->effectHealth = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementCurrentPercent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementCurrentPercent = atoi(token);
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
			//Begin Add
			else if(!Q_stricmp(token,"requirementHealthMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementHealthMaximum = atoi(token);
			}
			//End Add
			else if(!Q_stricmp(token,"requirementFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementFatigue = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainCurrent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainCurrent = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainCurrentPercent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainCurrentPercent = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainHealth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainHealth = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainFatigue = atoi(token);
			}
			else if(!Q_stricmp(token,"sustainMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->sustainMaximum = atoi(token);
			}
			else if(!Q_stricmp(token,"transformFirstDuration")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformFirstDuration = atof(token);
			}
			else if(!Q_stricmp(token,"transformFirstHealth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformFirstHealth = atof(token);
			}
			else if(!Q_stricmp(token,"transformFirstFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformFirstFatigue = atof(token);
			}
			else if(!Q_stricmp(token,"transformFirstEffectMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformFirstEffectMaximum = atof(token);
			}
			else if(!Q_stricmp(token,"transformDuration")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformDuration = atof(token);
			}
			else if(!Q_stricmp(token,"transformHealth")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformHealth = atof(token);
			}
			else if(!Q_stricmp(token,"transformFatigue")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformFatigue = atof(token);
			}
			else if(!Q_stricmp(token,"transformEffectMaximum")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformEffectMaximum = atof(token);
			}
			else if(!Q_stricmp(token,"transformSubsequentDuration")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformSubsequentDuration = atof(token);
			}
			else if(!Q_stricmp(token,"transformEffectSubsequentHealthScale")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformSubsequentHealthScale = atof(token);
			}
			else if(!Q_stricmp(token,"transformEffectSubsequentFatigueScale")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformSubsequentFatigueScale = atof(token);
			}
			else if(!Q_stricmp(token,"transformEffectSubsequentMaximumScale")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformSubsequentMaximumScale = atof(token);
			}
			else if(!Q_stricmp(token,"transformTime")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->transformTime = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementUseSkill")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementUseSkill = atoi(token);
			}
			else if(!Q_stricmp(token,"requirementButtonUp")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementButtonUp = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"requirementButtonDown")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->requirementButtonDown = strlen(token) == 4 ? qtrue : qfalse;
				if(tier->requirementButtonDown == 0);
			}
			else if(!Q_stricmp(token,"tierPermanent")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				tier->permanent = strlen(token) == 4 ? qtrue : qfalse;
			}
			else if(!Q_stricmp(token,"canBlock")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canBlock;}
			}
			else if(!Q_stricmp(token,"canMelee")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canMelee;}
			}
			else if(!Q_stricmp(token,"canLockon")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canLockon;}
			}
			else if(!Q_stricmp(token,"canHandspring")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canHandspring;}
			}
			else if(!Q_stricmp(token,"canGrab")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canGrab;}
			}
			else if(!Q_stricmp(token,"canSlam")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canSlam;}
			}
			else if(!Q_stricmp(token,"canClench")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canClench;}
			}
			else if(!Q_stricmp(token,"canExpulse")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canExpulse;}
			}
			else if(!Q_stricmp(token,"canDischarge")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canDischarge;}
			}
			else if(!Q_stricmp(token,"canBoost")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canBoost;}
			}
			else if(!Q_stricmp(token,"canSwim")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canSwim;}
			}
			else if(!Q_stricmp(token,"canFly")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canFly;}
			}
			else if(!Q_stricmp(token,"canJump")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canJump;}
			}
			else if(!Q_stricmp(token,"canZanzoken")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canZanzoken;}
			}
			else if(!Q_stricmp(token,"canBallFlip")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canBallFlip;}
			}
			else if(!Q_stricmp(token,"canOverheal")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canOverheal;}
			}
			else if(!Q_stricmp(token,"canBreakLimit")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canBreakLimit;}
			}
			else if(!Q_stricmp(token,"canTransform")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canTransform;}
			}
			else if(!Q_stricmp(token,"canSoar")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canSoar;}
			}
			else if(!Q_stricmp(token,"canAlter")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(strlen(token) == 4){tier->capabilities |= canAlter;}
			}
		}
	}
}
