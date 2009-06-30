// Copyright (C) 1999-2000 Id Software, Inc.
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate
#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "bg_userweapons.h"
#include "g_tiers.h"
pmove_t		*pm;
pml_t		pml;
// movement parameters
float	pm_stopspeed = 100.0f;
float	pm_swimScale = 0.80f;
float	pm_accelerate = 10.0f;
float	pm_airaccelerate = 15.0f;
float	pm_wateraccelerate = 4.0f;
float	pm_flyaccelerate = 15.0f;
float	pm_dashaccelerate = 15.0f;
float	pm_friction = 15.0f;
float	pm_waterfriction = 1.0f;
float	pm_flightfriction = 15.0f;
float	pm_spectatorfriction = 7.0f;
int		c_pmove = 0;
/*===============
DECLARATIONS
===============*/
void PM_StopBoost(void);
void PM_StopZanzoken(void);
void PM_StopDash(void);
void PM_StopJump(void);
void PM_StopFlight(void);
void PM_StopMovement(void);
void PM_StopDirections(void);
void PM_StopLockon(void);
void PM_ContinueLegsAnim(int anim);
void PM_ContinueTorsoAnim(int anim);
void PM_Accelerate(vec3_t wishdir,float wishspeed,float accel);
void PM_WeaponRelease(void);
float PM_CmdScale(usercmd_t *cmd);
/*===================
PM_StartTorsoAnim
===================*/
void PM_StartTorsoAnim(int anim){
	pm->ps->torsoAnim = ((pm->ps->torsoAnim & ANIM_TOGGLEBIT)^ ANIM_TOGGLEBIT) | anim;
}
void PM_StartLegsAnim(int anim){
	if((pm->ps->bitFlags & isGuiding) && (pm->cmd.buttons & BUTTON_ANY) && !(VectorLength(pm->ps->velocity)== 0.0f)){return;}
	pm->ps->legsAnim = ((pm->ps->legsAnim & ANIM_TOGGLEBIT)^ ANIM_TOGGLEBIT ) | anim;
}

void PM_ContinueLegsAnim(int anim){
	if((pm->ps->bitFlags & isGuiding) && (pm->cmd.buttons & BUTTON_ANY) && !(VectorLength(pm->ps->velocity)== 0.0f)){return;}
	if((pm->ps->legsAnim & ~ANIM_TOGGLEBIT)== anim){
		return;
	}
	PM_StartLegsAnim(anim );
}

void PM_ContinueTorsoAnim(int anim){
	if((pm->ps->torsoAnim & ~ANIM_TOGGLEBIT)== anim){
		return;
	}
	PM_StartTorsoAnim(anim );
}
void PM_ForceTorsoAnim(int anim){
	pm->ps->torsoTimer = 0;
	PM_StartTorsoAnim(anim);
}
void PM_ForceLegsAnim(int anim){
	if((pm->ps->bitFlags & isGuiding) && (pm->cmd.buttons & BUTTON_ANY) && !(VectorLength(pm->ps->velocity)== 0.0f)){return;}
	pm->ps->legsTimer = 0;
	PM_StartLegsAnim(anim );
}
/*===================
FREEZE
===================*/
void PM_Freeze(void){
	if(pm->ps->timers[tmFreeze] > 0){
		pm->ps->timers[tmFreeze] -= pml.msec;
		VectorClear(pm->ps->velocity);
		//pm->cmd.forwardmove = 0;
		if(pm->ps->timers[tmFreeze]<0){pm->ps->timers[tmFreeze] = 0;}
	}
}
/*===================
CHECK LOOPING SOUND
===================*/
void PM_CheckLoopingSound(void){
	if(pm->ps->timers[tmKnockback] ||
		pm->ps->timers[tmMelee1] < 0 ||
		pm->ps->timers[tmMelee2] < 0 ||
		pm->ps->bitFlags & isUnconcious || 
		pm->ps->bitFlags & isDead){
		PM_AddEvent(EV_STOPLOOPINGSOUND);
	}
	if(pm->ps->lockedTarget > 0){
		if (pm->ps->lockedPlayer->powerLevel[plCurrent] <= 0 || 
			pm->ps->lockedPlayer->powerLevel[plFatigue] <= 0){
			PM_AddEvent(EV_STOPLOOPINGSOUND);
		}
	}
}
/*===================
KNOCKBACK
===================*/
void PM_StopKnockback(void){
	pm->ps->timers[tmKnockback] = 0;
	pm->ps->knockBackDirection = 0;
}
void PM_StopDirections(void){
	pm->cmd.forwardmove = 0;
	pm->cmd.upmove = 0;
	pm->cmd.rightmove = 0;
}
void PM_CheckKnockback(void){
	vec3_t pre_vel,post_vel,wishvel,wishdir;	
	float scale,wishspeed;
	int i,speed;
	if(pm->ps->pm_flags & PMF_TIME_KNOCKBACK){
		pm->ps->timers[tmKnockback] = 5000; 
		pm->ps->knockBackDirection = 5;
	}
	if(pm->ps->timers[tmKnockback] < 0){
		pm->ps->timers[tmKnockback] += pml.msec;
		PM_ContinueLegsAnim(LEGS_KNOCKBACK_RECOVER_2);
		if(pm->ps->timers[tmKnockback] >= 0){
			pm->ps->timers[tmKnockback] = 0;
		}
	}
	if(pm->ps->timers[tmKnockback] > 0){
		PM_ContinueLegsAnim(LEGS_KNOCKBACK);
		pm->ps->timers[tmKnockback] -= pml.msec;
		PM_StopDirections();
		if(pm->ps->timers[tmKnockback] > 0){
			if(pm->ps->knockBackDirection == 1){pm->cmd.upmove = 127;}
			else if(pm->ps->knockBackDirection == 2){pm->cmd.upmove = -127;}
			else if(pm->ps->knockBackDirection == 3){pm->cmd.rightmove = -127;}
			else if(pm->ps->knockBackDirection == 4){pm->cmd.rightmove = 127;}
			else if(pm->ps->knockBackDirection == 0){pm->cmd.forwardmove = -127;}
		}
		else{
			PM_StopKnockback();
		}
		scale = PM_CmdScale(&pm->cmd);
		for(i=0;i<3;i++){
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove + scale * pml.up[i] * pm->cmd.upmove;
		}
		VectorCopy(wishvel,wishdir);
		wishspeed = VectorNormalize(wishdir);
		PM_Accelerate(wishdir,wishspeed,50);
		VectorNormalize(pm->ps->velocity);
		VectorCopy(pm->ps->velocity,pre_vel);
		speed = pm->ps->powerups[PW_KNOCKBACK_SPEED];
		if((pm->cmd.buttons & BUTTON_ALT_ATTACK) && (pm->ps->timers[tmKnockback] < 4500)){
			PM_AddEvent(EV_AIRBRAKE);
			speed = 0;
		}
		else if(pm->cmd.buttons & BUTTON_ATTACK){
			pm->ps->timers[tmKnockback] -= pml.msec;
			PM_ContinueLegsAnim(LEGS_KNOCKBACK_RECOVER_1);
			speed /= 2;
		}
		VectorScale(pm->ps->velocity,speed,pm->ps->velocity);
		PM_StepSlideMove(qfalse);
		VectorNormalize2(pm->ps->velocity,post_vel);
		if((DotProduct(pre_vel,post_vel)< 0.5f) || (VectorLength(pm->ps->velocity)== 0.0f)){
			//PM_KnockbackCrash();
		}
	}
}
/*===================
TALK
===================*/
void PM_CheckTalk(void){
	if(pm->cmd.buttons & BUTTON_TALK){pm->ps->eFlags |= EF_TALK;}
	else{pm->ps->eFlags &= ~EF_TALK;}
	if(pm->cmd.buttons & BUTTON_TALK){
		pm->cmd.buttons = BUTTON_TALK;
	}
}
/*===============
ZANZOKEN
===============*/
void PM_StopZanzoken(void){
	if(pm->ps->bitFlags & usingZanzoken){
		pm->ps->bitFlags &= ~usingZanzoken;
		PM_AddEvent(EV_ZANZOKEN_END);
		VectorClear(pm->ps->velocity);
	}
	pm->ps->timers[tmZanzoken] = 0;
}
void PM_CheckZanzoken(void){
	vec3_t pre_vel, post_vel,minSize,maxSize,forward,up,end;
	int speed,cost;
	trace_t trace;
	if(!(pm->cmd.buttons & BUTTON_LIGHTSPEED)){
		pm->ps->timers[tmZanzoken] = -1;
	}
	if((pm->ps->bitFlags & usingZanzoken) && (pm->ps->timers[tmZanzoken] <= 0)){
		minSize[0] = minSize[1] = minSize[2] = -1;
		maxSize[0] = maxSize[1] = maxSize[2] = 1;
		AngleVectors(pm->ps->viewangles,forward,NULL,NULL);
		VectorMA(pm->ps->origin,10,forward,end);
		pm->trace(&trace,pm->ps->origin,minSize,maxSize,end,pm->ps->clientNum,CONTENTS_BODY); 
		if(trace.fraction == 1.0){
			PM_StopZanzoken();
		} 
	}
	if(pm->ps->timers[tmZanzoken] > 0){
		speed = (pm->ps->powerLevel[plCurrent] / 13.1) + pm->ps->stats[stZanzokenSpeed];
		pm->ps->timers[tmZanzoken] -= pml.msec;
		VectorNormalize(pm->ps->velocity);
		VectorCopy(pm->ps->velocity,pre_vel);
		if(pm->ps->lockedTarget > 0){speed *= 1.5;}
		VectorScale(pm->ps->velocity,speed,pm->ps->velocity);
		VectorNormalize2(pm->ps->velocity,post_vel);
		if(pm->ps->timers[tmZanzoken] < 0){
			pm->ps->timers[tmZanzoken] = 0;
		}
	}
	if((pm->cmd.buttons & BUTTON_LIGHTSPEED) && !(pm->ps->bitFlags & usingZanzoken) && (pm->ps->timers[tmZanzoken] == -1)){
		PM_StopDash();
		pm->ps->bitFlags |= usingZanzoken;
		pm->ps->timers[tmZanzoken] = (pm->ps->powerLevel[plFatigue] / 93.62) + pm->ps->stats[stZanzokenDistance];
		cost = (pm->ps->powerLevel[plMaximum] / 12) * pm->ps->stats[stZanzokenCost];
		if(pm->ps->lockedTarget > 0){cost *= 0.8;}
		pm->ps->powerLevel[plUseFatigue] += cost;
		PM_AddEvent(EV_ZANZOKEN_START);
	}	
}
/*===============
POWER LEVEL
===============*/
void PM_UsePowerLevel(qboolean useTotal){
	int newValue,amount,stat;
	stat = useTotal ? pm->ps->powerLevel[plFatigue] : pm->ps->powerLevel[plCurrent];
	amount = useTotal ? pm->ps->powerLevel[plUseFatigue] : pm->ps->powerLevel[plUseCurrent];
	newValue = (stat - amount) > 32767 ? 32767 : stat - amount;
	if(newValue <= 0 && !useTotal && pm->ps->powerLevel[plFatigue] > 0){
		pm->ps->powerLevel[plFatigue] -= (amount * 1.5);
		pm->ps->powerLevel[plCurrent] = 1;
		pm->ps->powerLevel[plUseCurrent] = 0;
		return;
	}
	newValue = newValue < -32767 ? -32767 : newValue;
	if(useTotal){
		pm->ps->powerLevel[plFatigue] = newValue;
		pm->ps->powerLevel[plUseFatigue] = 0;
	}
	else{
		pm->ps->powerLevel[plCurrent] = newValue;
		pm->ps->powerLevel[plUseCurrent] = 0;
	}
}
void PM_BurnPowerLevel(qboolean melee){
	float percent;
	int defense;
	int burn;
	int newValue;
	burn = melee ? pm->ps->powerLevel[plDamageFromMelee] : pm->ps->powerLevel[plDamageFromEnergy];
	if(!burn){return;}
	defense = melee ? pm->ps->stats[stMeleeDefense] : pm->ps->stats[stEnergyDefense];
	defense = pm->ps->bitFlags & usingBlock ? defense * 2.5 : defense;
	percent = 1.0 - ((float)pm->ps->powerLevel[plCurrent] / (float)pm->ps->powerLevel[plMaximum]);
	burn -= (int)(((float)pm->ps->powerLevel[plFatigue] * 0.01) * defense);
	newValue = pm->ps->powerLevel[plHealth] - burn;
	if(burn > 0){
		newValue = newValue > 32767 ? 32767 : newValue;
		pm->ps->powerLevel[plHealth] = newValue < 0 ? 0 : newValue;
		newValue = pm->ps->powerLevel[plMaximum] + ((float)burn * percent) * 1.5;
		newValue = newValue < 0 ? 0 : newValue;
		pm->ps->powerLevel[plMaximum] = newValue > 32767 ? 32767 : newValue;
	}
	if(melee){pm->ps->powerLevel[plDamageFromMelee] = 0;}
	else{pm->ps->powerLevel[plDamageFromEnergy] = 0;}
}
void PM_CheckStatus(void){
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR){return;}
	if(pm->ps->powerLevel[plHealth] <= 0){
		if(pm->ps->lockedPlayer->powerups[PW_MELEE_STATE] == 2){
			pm->ps->powerLevel[plHealth] = 1;
		}
		else if(pm->ps->powerups[PW_STATE] != -2){
			pm->ps->bitFlags |= isDead;
			pm->ps->powerups[PW_STATE] = -2;
			PM_StopMovement();
			PM_AddEvent(EV_DEATH);
		}
		else{
			PM_ContinueTorsoAnim(BOTH_DEATH2);
			PM_ContinueLegsAnim(BOTH_DEATH2);
		}
	}
	else if(pm->ps->powerLevel[plFatigue]<= 0){
		if(pm->ps->powerups[PW_STATE] != -1){
			pm->ps->bitFlags |= isUnconcious;
			pm->ps->powerups[PW_STATE] = -1;
			PM_StopMovement();
			pm->ps->powerLevel[plCurrent] = 1;
			PM_AddEvent(EV_UNCONCIOUS);
			if(pm->ps->powerLevel[plFatigue] > -2500){
				pm->ps->powerLevel[plFatigue] = -2500;
			}
		}
		else{
			pm->ps->powerLevel[plFatigue] += pml.msec / 2;
			PM_ContinueTorsoAnim(BOTH_DEATH3);
			PM_ContinueLegsAnim(BOTH_DEATH3);
			if(pm->ps->powerLevel[plFatigue] > 0){
				pm->ps->bitFlags &= ~isUnconcious;
				pm->ps->powerups[PW_STATE] = 0;
			}
		}
	}
	else{
		pm->ps->powerups[PW_STATE] = 0;
	}
}
void PM_CheckTransform(void){
	pm->ps->timers[tmUpdateTier] += pml.msec;
	if(pm->ps->timers[tmTransform] == 1){
		pm->ps->timers[tmTransform] = -1000;
		PM_AddEvent(EV_TIERUP);
	}
	else if(pm->ps->timers[tmTransform] == -1){
		pm->ps->timers[tmTransform] = -1000;
		PM_AddEvent(EV_TIERDOWN);
	}
	else if(pm->ps->timers[tmTransform] > 0){
		if(pm->ps->bitFlags & isTransforming){
			PM_ContinueLegsAnim(LEGS_TRANS_UP);
			pm->ps->eFlags |= EF_AURA;
			pm->ps->timers[tmTransform] -= pml.msec;
			if(pm->ps->timers[tmTransform] <= 0){pm->ps->timers[tmTransform] = 0;}
			PM_StopMovement();
		}
		else{
			pm->ps->bitFlags |= isTransforming;
			PM_AddEvent(EV_TIERUP_FIRST);
		}
	}
	else if(pm->ps->timers[tmTransform] < 0){
		pm->ps->timers[tmTransform] += pml.msec;
		if(pm->ps->timers[tmTransform] >= -1){
			pm->ps->timers[tmTransform] = 0;
		}
	}
	else{
		if(pm->ps->timers[tmUpdateTier] > 300){
			pm->ps->timers[tmUpdateTier] = 0;
			PM_AddEvent(EV_TIERCHECK);
		}
	}
	if(pm->ps->timers[tmTransform] == 0){
		pm->ps->bitFlags &= ~isTransforming;
	}
}
void PM_CheckPowerLevel(void){
	int plSpeed,amount;
	int *timers,*powerLevel;
	float lower,raise,recovery;
	float check;
	float pushLimit;
	int newValue;
	int idleScale;
	timers = pm->ps->timers;
	powerLevel = pm->ps->powerLevel;
	timers[tmPowerAuto] += pml.msec;
	pm->ps->bitFlags &= ~usingAlter;
	if(pm->ps->powerLevel[plFatigue] <= 0 && pm->ps->powerLevel[plCurrent] > 0){
		pm->ps->powerLevel[plCurrent] = 1;
	}
	while(timers[tmPowerAuto] >= 100){
		timers[tmPowerAuto] -= 100;
		idleScale = (!pm->cmd.forwardmove && !pm->cmd.rightmove && !pm->cmd.upmove) ? 2 : 1;
		recovery = (float)pm->ps->powerLevel[plMaximum] * 0.002 * idleScale;
		recovery *= (1.0 - ((float)pm->ps->powerLevel[plCurrent] / (float)pm->ps->powerLevel[plMaximum]));
		if(pm->ps->bitFlags & usingBoost || pm->ps->bitFlags & usingAlter || pm->ps->powerLevel[plCurrent] > pm->ps->powerLevel[plFatigue]){recovery = 0;}
		if(powerLevel[plCurrent] > powerLevel[plFatigue]){
			newValue = powerLevel[plCurrent] - (pm->ps->powerLevel[plMaximum] * 0.005);
			powerLevel[plCurrent] = (powerLevel[plCurrent] - newValue >= 0) ? newValue : 0;
		}
		newValue = powerLevel[plCurrent] + pm->ps->powerLevel[plDrainCurrent];
		if(newValue < powerLevel[plMaximum] && newValue > 0){powerLevel[plCurrent] = newValue;}
		newValue = powerLevel[plFatigue] + pm->ps->powerLevel[plDrainFatigue];
		if(newValue < pm->ps->powerLevel[plMaximum]){powerLevel[plFatigue] = newValue;}
		newValue = pm->ps->powerLevel[plHealth] + pm->ps->powerLevel[plDrainHealth];
		if(newValue < pm->ps->powerLevel[plMaximum]){pm->ps->powerLevel[plHealth] = newValue;}
		newValue = pm->ps->powerLevel[plMaximum] + pm->ps->powerLevel[plDrainMaximum];
		if(newValue < 32767 &&  newValue > 0){pm->ps->powerLevel[plMaximum] = newValue;}
		if(powerLevel[plFatigue] + recovery < pm->ps->powerLevel[plMaximum]){
			powerLevel[plFatigue] += recovery;
		}
		else{
			powerLevel[plFatigue] = pm->ps->powerLevel[plMaximum];
		}
	}
	if(pm->cmd.buttons & BUTTON_POWERLEVEL){
		pm->ps->bitFlags &= ~keyTierDown;
		pm->ps->bitFlags &= ~keyTierUp;
		pm->ps->bitFlags |= usingAlter;
		if(pm->ps->bitFlags & usingBoost || pm->ps->timers[tmTransform] < 0){return;}
		if(pm->cmd.upmove < 0){
			PM_StopFlight();
			PM_ForceLegsAnim(LEGS_FLY_DOWN);
		}
		if(pm->cmd.forwardmove > 0){
			pm->ps->bitFlags |= keyTierUp;
		}
		else if(pm->cmd.forwardmove < 0){
			pm->ps->bitFlags |= keyTierDown;
		}
		else if(pm->cmd.rightmove < 0){
			pm->ps->eFlags &= ~EF_AURA;
			lower = powerLevel[plFatigue] * 0.01;
			powerLevel[plCurrent] = (powerLevel[plCurrent] - lower > 1) ? powerLevel[plCurrent] - lower : 1;
		}
		else if((pm->cmd.rightmove > 0)){
			pm->ps->eFlags |= EF_AURA;
			timers[tmPowerRaise] += pml.msec;
			while(timers[tmPowerRaise] >= 25){
				timers[tmPowerRaise] -= 25;
				raise = powerLevel[plFatigue] * 0.009;
				if(raise < 1){raise = 1;}
				if(powerLevel[plCurrent] > powerLevel[plFatigue]){raise *= 0.6;}
				newValue = powerLevel[plCurrent] + raise;
				if(newValue > pm->ps->powerLevel[plMaximum]){newValue = pm->ps->powerLevel[plMaximum];}
				if(newValue > 32767){newValue = 32767;}
				powerLevel[plCurrent] = newValue;
				if(powerLevel[plCurrent] == pm->ps->powerLevel[plMaximum]){
					pm->ps->bitFlags |= isBreakingLimit;
					pushLimit = powerLevel[plCurrent] + pm->ps->breakLimitRate;
					if(powerLevel[plHealth] + raise * 0.3 < pushLimit){
						powerLevel[plHealth] += raise * 0.3;
						pm->ps->powerLevel[plUseFatigue] += raise * 0.25;
					}
					if(powerLevel[plHealth] > 32767){powerLevel[plHealth] = 32767;}
					if(pushLimit < 32767){
						powerLevel[plCurrent] = powerLevel[plMaximum] = pushLimit;
					}
				}
				if(pm->ps->bitFlags & isBreakingLimit){
					if(powerLevel[plCurrent] < ((float)pm->ps->powerLevel[plMaximum] * 0.95)){
						pm->ps->bitFlags &= ~isBreakingLimit;
					}
				}
			}
		}
		else{
			pm->ps->bitFlags &= ~usingAlter;
			pm->ps->bitFlags &= ~isBreakingLimit;
		}
	}
	else{
		pm->ps->bitFlags &= ~keyTierDown;
		pm->ps->bitFlags &= ~keyTierUp;
		pm->ps->bitFlags &= ~isBreakingLimit;
	}
}
/*===============
BOOST TYPES
===============*/
void PM_StopBoost(void){
	pm->ps->bitFlags &= ~usingBoost;
	pm->ps->powerups[PW_BOOST] = 0;
}
void PM_StopDash(void){
	VectorClear(pm->ps->dashDir);
}
void PM_StopMovementTypes(void){
	PM_StopDash();
	PM_StopJump();
	PM_StopZanzoken();
	PM_StopKnockback();
}
void PM_StopMovement(void){
	PM_StopMovementTypes();
	VectorClear(pm->ps->velocity);
}
void PM_CheckBoost(void){
	if(!(pm->cmd.buttons & BUTTON_BOOST)){
		PM_StopBoost();
	}
	else if(pm->ps->powerups[PW_BOOST]){
		pm->ps->eFlags |= EF_AURA;
		pm->ps->powerups[PW_BOOST] += pml.msec;
		if(pm->ps->powerups[PW_BOOST] > 150){
			pm->ps->powerLevel[plUseFatigue] += pm->ps->powerLevel[plMaximum] * 0.003;
			pm->ps->powerups[PW_BOOST] -= 150;
		}
	}
	else{
		pm->ps->powerups[PW_BOOST] = 1;
		pm->ps->bitFlags |= usingBoost;
	}
}
/*===============
PM_AddEvent
===============*/
void PM_AddEvent(int newEvent){
	BG_AddPredictableEventToPlayerstate(newEvent,0,pm->ps);
}
/*===============
PM_AddTouchEnt
===============*/
void PM_AddTouchEnt(int entityNum){
	int		i;
	if(entityNum == ENTITYNUM_WORLD){
		return;
	}
	if(pm->numtouch == MAXTOUCH){
		return;
	}
	// see ifit is already added
	for (i = 0 ; i < pm->numtouch ; i++){
		if(pm->touchents[ i ] == entityNum){
			return;
		}
	}
	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity(vec3_t in, vec3_t normal, vec3_t out, float overbounce){
	float	backoff;
	float	change;
	int		i;
	backoff = DotProduct (in, normal);
	if(backoff < 0){
		backoff *= overbounce;
	} else{
		backoff /= overbounce;
	}
	for (i=0 ; i<3 ; i++){
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*==================
PM_Friction
==================*/
void PM_Friction(void){
	float speed,newspeed,control,drop;
	speed = VectorLength(pm->ps->velocity);
	if(speed < 5){
		PM_StopDash();
		VectorClear(pm->ps->velocity);
		return;
	}
	if(pm->waterlevel){
		drop += speed*pm_waterfriction*pm->waterlevel*pml.frametime;
	}
	if(pm->ps->bitFlags & usingFlight || pml.onGround){
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*pm_friction*pml.frametime;
	}
	if(pm->ps->timers[tmKnockback] > 0){
		if(speed < 100){
			drop = speed*15*pml.frametime;
		} else {
			drop = speed*0*pml.frametime;
		}
	}
	newspeed = speed - drop;
	if(newspeed < 0) {newspeed = 0;}
	newspeed /= speed;
	pm->ps->velocity[0] *= newspeed;
	pm->ps->velocity[1] *= newspeed;
	pm->ps->velocity[2] *= newspeed;
}
/*==============
PM_Accelerate
==============*/
void PM_Accelerate(vec3_t wishdir, float wishspeed, float accel){
#if 1
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	// clamp the speed lower if wading or moving underwater.
	if(pm->waterlevel){
		float waterScale;

		waterScale = 1.0f - (1.0f - pm_swimScale)* (float)pm->waterlevel / 3.0f;
		wishspeed = wishspeed * waterScale;
	}

	currentspeed = DotProduct (pm->ps->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if(addspeed <= 0) {
		return;
	}
	accelspeed = accel*pml.frametime*wishspeed;
	if(accelspeed > addspeed) {
		accelspeed = addspeed;
	}

	for (i=0 ; i<3 ; i++) {
		pm->ps->velocity[i] += accelspeed*wishdir[i];
	}

#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t		wishVelocity;
	vec3_t		pushDir;
	float		pushLen;
	float		canPush;

	VectorScale(wishdir, wishspeed, wishVelocity );
	VectorSubtract(wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize(pushDir );

	canPush = accel*pml.frametime*wishspeed;
	if(canPush > pushLen) {
		canPush = pushLen;
	}

	VectorMA(pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
float PM_CmdScale(usercmd_t *cmd){
	int		max;
	int		totalSpeed;
	float	total;
	float	scale;

	max = abs(cmd->forwardmove );
	if(abs(cmd->rightmove)> max){
		max = abs(cmd->rightmove );
	}
	if(abs(cmd->upmove)> max){
		max = abs(cmd->upmove );
	}
	if(!max){
		return 0;
	}

	total = sqrt(cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );

	totalSpeed = (pm->ps->powerLevel[plFatigue] / 72.81) + pm->ps->stats[stSpeed];
	if(pm->ps->bitFlags & isAutoClosing && pm->ps->lockedPlayer->timers[tmKnockback]){totalSpeed *= 3;}
	scale = (float)totalSpeed * max / (127.0 * total );

	return scale;
}
/*=============
PM_CheckJump
=============*/
void PM_NotOnGround(void){
	pml.groundPlane = qfalse;
	pml.onGround = qfalse;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
}
void PM_StopJump(void){
	pm->ps->bitFlags &= ~usingJump;
}
void PM_CheckJump(void){
	int jumpPower;
	float jumpScale;
	if(pm->ps->bitFlags & usingJump || pm->ps->bitFlags & usingAlter || pm->ps->bitFlags & usingFlight){return;}
	if(!(pm->cmd.buttons & BUTTON_JUMP)){return;}
	PM_NotOnGround();
	PM_StopDash();
	pm->ps->bitFlags |= usingJump;
	jumpScale = pm->ps->bitFlags & usingBoost ? 1.5 : 1.0;
	jumpPower = pm->ps->stats[stSpeed] * jumpScale;
	pm->ps->gravity = 12000 * jumpScale;
	pm->ps->velocity[0] *= fabs(pm->ps->velocity[0]) < jumpPower ? 4.0 : 1.0;
	pm->ps->velocity[1] *= fabs(pm->ps->velocity[1]) < jumpPower ? 4.0 : 1.0;
	if((pm->cmd.forwardmove != 0) || (pm->cmd.rightmove != 0)){
		pm->ps->velocity[2] = jumpPower * 6;
		pm->ps->powerLevel[plUseCurrent] += pm->ps->powerLevel[plMaximum] * 0.01;
		PM_AddEvent(EV_HIGHJUMP);
	} else{
		pm->ps->gravity = 6000;
		pm->ps->velocity[2] = jumpPower * 8;
		pm->ps->powerLevel[plUseCurrent] += pm->ps->powerLevel[plMaximum] * 0.06;
		PM_AddEvent(EV_JUMP);
	}
	if(pm->cmd.forwardmove >= 0){
		PM_ForceLegsAnim(LEGS_JUMP);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else{
		PM_ForceLegsAnim(LEGS_JUMPB);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
}

/*=============
PM_CheckBlock
=============*/
void PM_CheckBlock(void){
	pm->ps->bitFlags &= ~usingBlock;
	if((pm->cmd.buttons & BUTTON_BLOCK)){
		pm->ps->bitFlags |= usingBlock;
		if(pm->ps->bitFlags & isStruggling){
			pm->ps->timers[tmFreeze] = 100;
			pm->ps->powerLevel[plUseCurrent] += 1;
		}
	}
}
/*===================
PM_FlyMove
===================*/
void PM_StopFlight(void){
	pm->ps->bitFlags &= ~usingFlight;
}
void PM_FlyMove(void){
	int		i;
	vec3_t	wishvel;
	vec3_t	wishdir;
	float	wishspeed;
	float	scale;
	float	boostFactor;
	if(pm->cmd.upmove > 0){
		pm->ps->bitFlags |= usingFlight;
		PM_StopDash();
		PM_StopJump();
	}
	if(!(pm->ps->bitFlags & usingFlight)){return;}
	boostFactor = 1.8;
	if(pm->ps->bitFlags & usingBoost){
		boostFactor = 3.9;
	}
	else if(pm->cmd.buttons & BUTTON_WALKING){
		boostFactor = 1.0;	
	}
	scale = PM_CmdScale(&pm->cmd) * boostFactor;
	if(!scale){
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}
	else{
		for(i=0 ;i<3;i++){
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove + scale * pml.up[i] * pm->cmd.upmove;
		}
	}
	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	PM_Accelerate(wishdir,wishspeed,pm_flyaccelerate);
}


/*===================
PM_AirMove
===================*/
void PM_AirMove(void){
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	if(pml.onGround){return;}
	if(pm->ps->bitFlags & isGuiding){return;}
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd);
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);
	for(i = 0 ; i < 2 ; i++){
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	wishvel[2] = 0;
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;
	PM_Accelerate (wishdir, wishspeed, pm_airaccelerate);
	if(pml.groundPlane){
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,pm->ps->velocity, OVERCLIP);
	}
	if(pm->waterlevel == 3){
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity * pml.frametime;
	}
	if(pm->ps->bitFlags & usingZanzoken){pm->tracemask = CONTENTS_SOLID;}
	PM_StepSlideMove(pm->ps->bitFlags & usingFlight ? qfalse : qtrue); 
	if(pm->ps->bitFlags & usingZanzoken){pm->tracemask = MASK_PLAYERSOLID;}
}
/*===================
PM_WalkMove
===================*/
void PM_WalkMove(void){
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	float		accelerate;
	float		vel;
	if(!pml.onGround || VectorLength(pm->ps->dashDir)){return;}
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd) * 2.5;
	// set the movementDir so clients can rotate the legs for strafing
	pml.forward[2] = 0;
	pml.right[2] = 0;
	PM_ClipVelocity (pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity (pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);
	for (i = 0 ; i < 3 ; i++){
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	}
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale * 0.60f;
	accelerate = pm_accelerate;
	PM_Accelerate (wishdir, wishspeed,accelerate);
	vel = VectorLength(pm->ps->velocity);
	PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,pm->ps->velocity, OVERCLIP );
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	if(!pm->ps->velocity[0] && !pm->ps->velocity[1]) {
		return;
	}
	if(pm->waterlevel == 3){
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity * pml.frametime;
	}
	PM_StepSlideMove(qfalse);
}

/*=============
PM_DashMove
=============*/
void PM_DashMove(void){
	int			i;
	vec3_t		wishvel;
	vec3_t		wishdir;
	float		fmove,smove;
	float		wishspeed;
	float		scale;
	float		dashAmount;
	float		boostFactor;
	float		accelerate;
	usercmd_t	cmd;
	dashAmount = VectorLength(pm->ps->dashDir);
	if((pml.onGround && !(pm->cmd.buttons & BUTTON_WALKING) && (pm->cmd.forwardmove || pm->cmd.rightmove))){
		pm->cmd.upmove = 0;
		boostFactor = (pm->ps->bitFlags & usingBoost) ? 4.4 : 1.1;
		if(dashAmount == 0){
			pm->ps->dashDir[0] = 0;
			pm->ps->dashDir[1] = 0;
			if(pm->cmd.forwardmove > 0){
				pm->ps->dashDir[0] = 1;
				pm->cmd.forwardmove = 120;
			}else if(pm->cmd.forwardmove < 0){
				pm->ps->dashDir[0] = -1;
				pm->cmd.forwardmove = -120;
			}
			if(pm->cmd.rightmove > 0){
				pm->ps->dashDir[1] = 1;
				pm->cmd.rightmove = 120;
			}else if(pm->cmd.rightmove < 0){
				pm->ps->dashDir[1] = -1;
				pm->cmd.rightmove = -120;
			}
		}
		cmd = pm->cmd;
		fmove = pm->cmd.forwardmove;
		smove = pm->cmd.rightmove;
		scale = PM_CmdScale(&cmd) * boostFactor;
		pml.forward[2] = 0;
		pml.right[2] = 0;
		VectorNormalize(pml.forward);
		VectorNormalize(pml.right);
		for(i=0;i<2;i++){
			wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
		}
		wishvel[2] = 0;
		VectorCopy (wishvel, wishdir);
		wishspeed = VectorNormalize(wishdir);
		wishspeed *= scale;
		accelerate = pm_dashaccelerate;
		PM_Accelerate(wishdir,wishspeed,accelerate);
		if(pml.groundPlane){
			PM_ClipVelocity(pm->ps->velocity,pml.groundTrace.plane.normal,pm->ps->velocity,OVERCLIP);
		}
		if(pm->waterlevel == 3){
			pm->ps->velocity[2] += 0.7f * pm->ps->gravity * pml.frametime;
		}
		PM_StepSlideMove(qtrue);
	}
	else{	
		PM_StopDash();
	}
}
/*===============
PM_NoclipMove
===============*/
void PM_NoclipMove(void){
	float	speed, drop, friction, control, newspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength (pm->ps->velocity);
	if(speed < 1){
		VectorCopy (vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction*1.5;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control*friction*pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if(newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		VectorScale (pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale(&pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i=0 ; i<3 ; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);
	wishspeed *= scale;

	PM_Accelerate(wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA (pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}
/*================
PM_FootstepForSurface
================*/
int PM_FootstepForSurface(void){
	if(pml.groundTrace.surfaceFlags & SURF_NOSTEPS){
		return 0;
	}
	if(pml.groundTrace.surfaceFlags & SURF_METALSTEPS){
		return EV_FOOTSTEP_METAL;
	}
	return EV_FOOTSTEP;
}
/*=================
PM_Land
=================*/
void PM_Land(void){
	float		delta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;

	// decide which landing animation to use
	if(pm->ps->pm_flags & PMF_BACKWARDS_JUMP){
		PM_ForceLegsAnim(LEGS_LANDB );
	} else{
		PM_ForceLegsAnim(LEGS_LAND );
	}

	pm->ps->legsTimer = TIMER_LAND;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if(den < 0){
		return;
	}
	t = (-b - sqrt(den)) / (2 * a );

	delta = vel + t * acc;
	delta = delta*delta * 0.0001;
	// never take falling damage ifcompletely underwater
	if(pm->waterlevel == 3){
		return;
	}

	// reduce falling damage ifthere is standing water
	if(pm->waterlevel == 2){
		delta *= 0.25;
	}
	if(pm->waterlevel == 1){
		delta *= 0.5;
	}

	if(delta < 1){
		return;
	}
	// start footstep cycle over
	pm->ps->bobCycle = 0;
}
/*
=============
PM_CorrectAllSolid
=============
*/
int PM_CorrectAllSolid(trace_t *trace){
	int			i, j, k;
	vec3_t		point;
	// jitter around
	for (i = -1; i <= 1; i++) {
		for (j = -1; j <= 1; j++) {
			for (k = -1; k <= 1; k++) {
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace (trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
				if(!trace->allsolid){
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace (trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}
	PM_NotOnGround();
	return qfalse;
}
/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
void PM_GroundTraceMissed(void){
	trace_t		trace;
	vec3_t		point;
	if(pm->ps->groundEntityNum != ENTITYNUM_NONE){
		VectorCopy(pm->ps->origin, point );
		point[2] -= 64;
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if(trace.fraction == 1.0){
			if(pm->cmd.forwardmove >= 0){
				PM_ContinueLegsAnim(LEGS_JUMP);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else{
				PM_ContinueLegsAnim(LEGS_JUMPB);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
			PM_StopDash();
		}
	}
	PM_NotOnGround();
}
/*
=============
PM_GroundTrace
=============
*/
void PM_GroundTrace(void){
	vec3_t		point;
	trace_t		trace;
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.75f;
	pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
	pml.groundTrace = trace;
	if(trace.allsolid && (!PM_CorrectAllSolid(&trace))){return;}
	// if the trace didn't hit anything, we are in free fall
	if(trace.fraction == 1.0){
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.onGround = qfalse;
		return;
	}
	if(pm->ps->velocity[2] > 0 && DotProduct(pm->ps->velocity, trace.plane.normal)> 10){
		if(pm->cmd.forwardmove >= 0){
			PM_ForceLegsAnim(LEGS_JUMP);
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}else{
			PM_ForceLegsAnim(LEGS_JUMPB );
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}
		PM_NotOnGround();
		return;
	}
	if(pm->ps->bitFlags & usingFlight){
		vec3_t testVec;
		VectorNormalize2(pml.up, testVec);
		if((testVec[2] < 0.9f) || (pm->cmd.upmove >= 0)){
			PM_NotOnGround();
			return;
		} else{
			PM_StopFlight();
			pm->ps->timers[tmZanzoken] = 0;
		}
	}
	// slopes that are too steep will not be considered onground
	if(trace.plane.normal[2] < MIN_WALK_NORMAL){
		PM_NotOnGround();
		pml.groundPlane = qtrue;
		return;
	}
	pml.groundPlane = qtrue;
	pml.onGround = qtrue;
	if(pm->ps->groundEntityNum == ENTITYNUM_NONE && (pm->ps->bitFlags & usingJump || pm->ps->bitFlags & usingFlight)){
		PM_StopJump();
		PM_StopFlight();
		PM_Land();
	}
	pm->ps->groundEntityNum = trace.entityNum;
	// don't reset the z velocity for slopes
	pm->ps->velocity[2] = 0;
	PM_AddTouchEnt(trace.entityNum );
}


/*=============
PM_SetWaterLevel
=============*/
void PM_SetWaterLevel(void){
	vec3_t		point;
	int			cont;
	int			sample1;
	int			sample2;
	pm->waterlevel = 0;
	pm->watertype = 0;
	if(pm->ps->pm_flags & PMF_TIME_WATERJUMP){
		return;
	}

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + MINS_Z + 1;
	cont = pm->pointcontents(point, pm->ps->clientNum );

	if(cont & MASK_WATER){
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + MINS_Z + sample1;
		cont = pm->pointcontents (point, pm->ps->clientNum );
		if(cont & MASK_WATER){
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + MINS_Z + sample2;
			cont = pm->pointcontents (point, pm->ps->clientNum );
			if(cont & MASK_WATER ){
				pm->waterlevel = 3;
			}
		}
	}

}
/*==============
PM_SetView
==============*/
void PM_SetView(void){
	pm->mins[0] = -15;
	pm->mins[1] = -15;
	pm->mins[2] = MINS_Z;
	pm->maxs[0] = 15;
	pm->maxs[1] = 15;
	pm->maxs[2] = 32;
	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;
}
/*===============
PM_Footsteps
===============*/
void PM_Footsteps(void){
	float bobmove;
	int	old;
	qboolean footstep;
	if((pm->ps->bitFlags & usingZanzoken)){return;}
	if((pm->ps->bitFlags & usingAlter) && (VectorLength(pm->ps->velocity) <= 0)){
		if(!(pm->cmd.rightmove > 0)){return;}
		if(pm->ps->powerLevel[plCurrent] > 31000){PM_ContinueLegsAnim(LEGS_KI_CHARGE);}
		else{pm->ps->powerLevel[plCurrent] > pm->ps->powerLevel[plFatigue] ? PM_ContinueLegsAnim(LEGS_KI_CHARGE) : PM_ContinueLegsAnim(LEGS_PL_UP);}
		return;
	}
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] +  pm->ps->velocity[1] * pm->ps->velocity[1]);
	if(pm->ps->bitFlags & usingFlight){
		if((!pm->cmd.forwardmove && !pm->cmd.rightmove) || (pm->cmd.buttons & BUTTON_WALKING )) {
			int	tempAnimIndex;
			pm->ps->bobCycle = 0;
			tempAnimIndex = pm->ps->torsoAnim & ~ANIM_TOGGLEBIT;
			if((tempAnimIndex >= TORSO_KI_ATTACK1_PREPARE) && (tempAnimIndex <= TORSO_KI_ATTACK6_ALT_FIRE)) {
				tempAnimIndex = tempAnimIndex - TORSO_KI_ATTACK1_PREPARE;
				tempAnimIndex = LEGS_AIR_KI_ATTACK1_PREPARE + tempAnimIndex;
				PM_ContinueLegsAnim(tempAnimIndex);
			} else{
				if(pm->cmd.upmove > 0){PM_ContinueLegsAnim(LEGS_FLY_UP);} 
				else if(pm->cmd.upmove < 0){PM_ContinueLegsAnim(LEGS_FLY_DOWN);}
				else if(pm->ps->clientLockedTarget>0){PM_ContinueLegsAnim(LEGS_IDLE_LOCKED);}
				else{PM_ContinueLegsAnim(LEGS_FLY_IDLE);}
			}
			return;
		}
		if(pm->cmd.forwardmove > 0){
			PM_ContinueLegsAnim(LEGS_DASH_FORWARD);
		} else if(pm->cmd.forwardmove < 0){
			PM_ContinueLegsAnim(LEGS_DASH_BACKWARD);
		} else if(pm->cmd.rightmove > 0){
			PM_ContinueLegsAnim(LEGS_DASH_RIGHT);
		} else if(pm->cmd.rightmove < 0){
			PM_ContinueLegsAnim(LEGS_DASH_LEFT);
		}
		return;
	}
	if(pm->ps->groundEntityNum == ENTITYNUM_NONE){
		//if(pm->ps->velocity[2] <= 0){PM_ForceLegsAnim(LEGS_FLY_DOWN);}
		if(pm->waterlevel > 1){
			PM_ContinueLegsAnim(LEGS_SWIM);
		}
		return;
	}
	if(!pm->cmd.forwardmove && !pm->cmd.rightmove){
		if(pm->xyspeed < 5){
			int	tempAnimIndex;
			pm->ps->bobCycle = 0;	// start at beginning of cycle again
			tempAnimIndex = pm->ps->torsoAnim & ~ANIM_TOGGLEBIT;
			if((tempAnimIndex >= TORSO_KI_ATTACK1_PREPARE) && (tempAnimIndex <= TORSO_KI_ATTACK6_ALT_FIRE)) {
				tempAnimIndex = tempAnimIndex - TORSO_KI_ATTACK1_PREPARE;
				tempAnimIndex = LEGS_KI_ATTACK1_PREPARE + tempAnimIndex;
				PM_ContinueLegsAnim(tempAnimIndex);
			} else{
				if(pm->ps->clientLockedTarget>0){PM_ContinueLegsAnim(LEGS_IDLE_LOCKED);}
				else{PM_ContinueLegsAnim(LEGS_IDLE);}
			}
		}
		return;
	}
	footstep = qfalse;
	if(!(pm->cmd.buttons & BUTTON_WALKING)){
		bobmove = 0.4f;
		if(!pm->ps->running){
			if(pm->cmd.forwardmove < 0) {
				PM_ContinueLegsAnim(LEGS_DASH_BACKWARD);
			} else if(pm->cmd.forwardmove > 0){
				PM_ContinueLegsAnim(LEGS_DASH_FORWARD);
			} else if(pm->cmd.rightmove > 0){
				PM_ContinueLegsAnim(LEGS_DASH_RIGHT);
			} else if(pm->cmd.rightmove < 0){
				PM_ContinueLegsAnim(LEGS_DASH_LEFT);
			}
		}else{
			if(pm->cmd.forwardmove < 0){
				PM_ContinueLegsAnim(LEGS_BACK);
			} else if(pm->cmd.forwardmove > 0){
				PM_ContinueLegsAnim(LEGS_RUN);
			} else if(pm->cmd.rightmove > 0){
				PM_ContinueLegsAnim(LEGS_DASH_RIGHT );
			} else if(pm->cmd.rightmove < 0){
				PM_ContinueLegsAnim(LEGS_DASH_LEFT);
			}
		}
		footstep = qtrue;
	}else{
		bobmove = 0.3f;
		if(pm->cmd.forwardmove < 0){
			PM_ContinueLegsAnim(LEGS_BACKWALK );
		}
		else{
			PM_ContinueLegsAnim(LEGS_WALK);
		}
	}
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec)& 255;
	if(((old + 64)^ (pm->ps->bobCycle + 64)) & 128){
		if(pm->waterlevel == 0){
			if(footstep && !pm->noFootsteps){
				PM_AddEvent(PM_FootstepForSurface());
			}
		} else if(pm->waterlevel == 1){
			PM_AddEvent(EV_FOOTSPLASH );
		} else if(pm->waterlevel == 2){
			PM_AddEvent(EV_SWIM );
		} else if(pm->waterlevel == 3){
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
void PM_WaterEvents(void){		// FIXME?
	//
	// ifjust entered a water volume, play a sound
	//
	if(!pml.previous_waterlevel && pm->waterlevel) {
		PM_AddEvent(EV_WATER_TOUCH );
	}

	//
	// ifjust completely exited a water volume, play a sound
	//
	if(pml.previous_waterlevel && !pm->waterlevel) {
		PM_AddEvent(EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if(pml.previous_waterlevel != 3 && pm->waterlevel == 3) {
		PM_AddEvent(EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if(pml.previous_waterlevel == 3 && pm->waterlevel != 3) {
		PM_AddEvent(EV_WATER_CLEAR );
	}
}


/*
===============
PM_BeginWeaponChange
===============
*/
void PM_BeginWeaponChange(int weapon){
	qboolean charging;
	charging = (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING) ? qtrue : qfalse;
	if(pm->ps->weapon == pm->cmd.weapon){
		return;
	}
	if(weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS){
		return;
	}
	if(pm->ps->powerups[PW_MELEE_STATE] || pm->ps->timers[tmTransform] || charging){
		return;
	}
	if(!(pm->ps->stats[stSkills] & (1 << weapon))){
		return;
	}

	if(pm->ps->weaponstate == WEAPON_DROPPING){
		return;
	}
	PM_AddEvent(EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
}


/*
===============
PM_FinishWeaponChange
===============
*/
void PM_FinishWeaponChange(void){
	int	weapon;
	weapon = pm->cmd.weapon;
	if(weapon < WP_NONE || weapon >= WP_NUM_WEAPONS){
		weapon = WP_NONE;
	}
	if(!(pm->ps->stats[stSkills] & (1 << weapon))){
		weapon = WP_NONE;
	}
	pm->ps->weapon = weapon;
	pm->ps->timers[tmAttack1] = 0;
	pm->ps->timers[tmAttack2] = 0;
}


/*==============
PM_TorsoAnimation
==============*/
void PM_TorsoAnimation(void){
	if(pm->ps->weaponstate != WEAPON_READY && !(pm->ps->bitFlags & usingMelee)){return;}
	if(pm->ps->bitFlags & usingBlock && !(pm->ps->bitFlags & usingMelee)){
		if(pm->ps->bitFlags & isStruggling){
			PM_ContinueTorsoAnim(TORSO_PUSH);
			PM_ContinueLegsAnim(LEGS_PUSH);
			return;
		}else{
			PM_ContinueTorsoAnim(TORSO_BLOCK);
			if(!pm->cmd.forwardmove && !pm->cmd.rightmove){
				PM_ContinueLegsAnim(LEGS_BLOCK);
			}
			return;
		}
	}
	switch(pm->ps->legsAnim & ~ANIM_TOGGLEBIT){
	case LEGS_KNOCKBACK:
		PM_ContinueTorsoAnim(TORSO_KNOCKBACK);
		break;
	case LEGS_KNOCKBACK_RECOVER_1:
		PM_ContinueTorsoAnim(TORSO_KNOCKBACK_RECOVER_1);
		break;
	case LEGS_KNOCKBACK_RECOVER_2:
		PM_ContinueTorsoAnim(TORSO_KNOCKBACK_RECOVER_2);
		break;
	case LEGS_POWER_MELEE_1_CHARGE:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_1_CHARGE);
		break;
	case LEGS_POWER_MELEE_2_CHARGE:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_2_CHARGE);
		break;
	case LEGS_POWER_MELEE_3_CHARGE:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_3_CHARGE);
		break;
	case LEGS_POWER_MELEE_4_CHARGE:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_4_CHARGE);
		break;
	case LEGS_POWER_MELEE_5_CHARGE:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_5_CHARGE);
		break;
	case LEGS_POWER_MELEE_6_CHARGE:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_6_CHARGE);
		break;
	case LEGS_POWER_MELEE_1_HIT:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_1_HIT);
		break;
	case LEGS_POWER_MELEE_2_HIT:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_2_HIT);
		break;
	case LEGS_POWER_MELEE_3_HIT:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_3_HIT);
		break;
	case LEGS_POWER_MELEE_4_HIT:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_4_HIT);
		break;
	case LEGS_POWER_MELEE_5_HIT:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_5_HIT);
		break;
	case LEGS_POWER_MELEE_6_HIT:
		PM_ContinueTorsoAnim(TORSO_POWER_MELEE_6_HIT);
		break;
	case LEGS_SPEED_MELEE_DODGE:
		PM_ContinueTorsoAnim(TORSO_SPEED_MELEE_DODGE);
		break;
	case LEGS_SPEED_MELEE_ATTACK:
		PM_ContinueTorsoAnim(TORSO_SPEED_MELEE_ATTACK);
		break;
	case LEGS_SPEED_MELEE_BLOCK:
		PM_ContinueTorsoAnim(TORSO_SPEED_MELEE_BLOCK);
		break;
	case LEGS_SPEED_MELEE_HIT:
		PM_ContinueTorsoAnim(TORSO_SPEED_MELEE_HIT);
		break;
	case LEGS_RUN:
		PM_ContinueTorsoAnim(TORSO_RUN);
		break;
	case LEGS_BACK:
		PM_ContinueTorsoAnim(TORSO_BACK);
		break;
	case LEGS_DASH_FORWARD:
		PM_ContinueTorsoAnim(TORSO_DASH_FORWARD );
		break;
	case LEGS_DASH_BACKWARD:
		PM_ContinueTorsoAnim(TORSO_DASH_BACKWARD );
		break;
	case LEGS_DASH_RIGHT:
		PM_ContinueTorsoAnim(TORSO_DASH_RIGHT );
		break;
	case LEGS_DASH_LEFT:
		PM_ContinueTorsoAnim(TORSO_DASH_LEFT );
		break;
	case LEGS_WALK:
		PM_ContinueTorsoAnim(TORSO_WALK );
		break;
	case LEGS_BACKWALK:
		PM_ContinueTorsoAnim(TORSO_BACKWALK );
		break;
	case LEGS_WALKCR:
		PM_ContinueTorsoAnim(TORSO_WALKCR );
		break;
	case LEGS_SWIM:
		PM_ContinueTorsoAnim(TORSO_SWIM );
		break;
	case LEGS_JUMP:
		PM_ContinueTorsoAnim(TORSO_JUMP );
		break;
	case LEGS_LAND:
		PM_ContinueTorsoAnim(TORSO_LAND );
		break;
	case LEGS_STUNNED:
		PM_ContinueTorsoAnim(TORSO_STUNNED);
		break;
	case LEGS_PUSH:
		PM_ContinueTorsoAnim(TORSO_PUSH);
		break;
	case LEGS_DEFLECT:
		PM_ContinueTorsoAnim(TORSO_DEFLECT);
		break;
	case LEGS_BLOCK:
		PM_ContinueTorsoAnim(TORSO_BLOCK);
		break;
	case LEGS_JUMPB:
		PM_ContinueTorsoAnim(TORSO_JUMPB );
		break;
	case LEGS_LANDB:
		PM_ContinueTorsoAnim(TORSO_LANDB );
		break;
	case LEGS_KI_CHARGE:
		PM_ContinueTorsoAnim(TORSO_KI_CHARGE );
		break;
	case LEGS_PL_UP:
		PM_ContinueTorsoAnim(TORSO_PL_UP );
		break;
	case LEGS_TRANS_UP:
		PM_ContinueTorsoAnim(TORSO_TRANS_UP);
		break;
	case LEGS_TRANS_BACK:
		PM_ContinueTorsoAnim(TORSO_TRANS_BACK );
		break;
	case LEGS_FLY_IDLE:
		PM_ContinueTorsoAnim(TORSO_FLY_IDLE);
		break;
	case LEGS_FLY_FORWARD:
		PM_ContinueTorsoAnim(TORSO_FLY_FORWARD );
		break;
	case LEGS_FLY_BACKWARD:
		PM_ContinueTorsoAnim(TORSO_FLY_BACKWARD);
		break;
	case LEGS_FLY_UP:
		PM_ContinueTorsoAnim(TORSO_FLY_UP );
		break;
	case LEGS_FLY_DOWN:
		PM_ContinueTorsoAnim(TORSO_FLY_DOWN);
		break;
	case LEGS_IDLE:
		PM_ContinueTorsoAnim(TORSO_STAND);
		break;
	default:
		(pm->ps->clientLockedTarget>0) ? PM_ContinueTorsoAnim(TORSO_STAND_LOCKED) : PM_ContinueTorsoAnim(TORSO_STAND);
		break;
	}
}
/*=============================================*\
    Amazin' Melee (now with 60% more flavor!)
\*=============================================*/
// MELEE STATE 0 - Not Ready
// MELEE STATE 1 - Idle
// MELEE STATE 2 - Speed Melee Held
// MELEE STATE 3 - Power Melee Charging
// MELEE STATE 4 - Power Melee Using
// MELEE STATE 5 - Block Held
// MELEE STATE 6 - Zanzoken Held
void PM_StopMelee(void){
	qboolean charging;
	pm->ps->powerups[PW_MELEE_STATE] = 0;	
	pm->ps->timers[tmMelee1] = 0;
	pm->ps->timers[tmMelee2] = 0;
	charging = (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING) ? qtrue : qfalse;
	if((!(pm->cmd.buttons & BUTTON_ALT_ATTACK) && !(pm->cmd.buttons & BUTTON_ATTACK)) || charging){
		pm->ps->bitFlags &= ~usingMelee;
	}
}
void PM_Melee(void){
	int melee1,melee2,enemyState,state,option,damage,distance;
	qboolean knockback,inRange,charging,movingForward;
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR){return;}
	knockback = qfalse;
	inRange = qfalse;
	state = pm->ps->powerups[PW_MELEE_STATE];
	charging = (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING) ? qtrue : qfalse;
	pm->ps->timers[tmUpdateMelee] += pml.msec;
	pm->ps->bitFlags &= ~isAutoClosing;
	if(pm->ps->lockedTarget > 0){
		/*
		if(pm->ps->lockedPlayer->bitFlags & isDead || pm->ps->lockedPlayer->bitFlags & isUnconcious){
			PM_StopLockon();
			return;
		}
		*/
		if(pm->ps->timers[tmUpdateMelee] > 300){
			pm->ps->timers[tmUpdateMelee] = 0;
			PM_AddEvent(EV_MELEE_CHECK);
		}
		distance = Distance(pm->ps->origin,*(pm->ps->lockedPosition));
		inRange = distance <= 64 ? qtrue : qfalse;
		if(charging || !state || distance > 512){PM_StopMelee();}
		if(!charging && !state && distance <= 512){pm->ps->powerups[PW_MELEE_STATE] = 1;}
	}
	else if(!pm->ps->lockedTarget){
		PM_StopMelee();
	}
	state = pm->ps->powerups[PW_MELEE_STATE];
	if((state > 0) && !charging && (pm->ps->lockedTarget > 0)){
		enemyState = pm->ps->lockedPlayer->powerups[PW_MELEE_STATE];
		melee1 = pm->ps->timers[tmMelee1];
		melee2 = pm->ps->timers[tmMelee2];
		damage = 0;		if(pm->ps->lockedPlayer->bitFlags & isDead || pm->ps->lockedPlayer->bitFlags & isUnconcious){
			PM_StopLockon();
			return;
		}
		// Knockback Juggle Helper
		if(pm->ps->lockedPlayer->timers[tmKnockback] > 500 && distance <= 128 && melee2 >= 0 && !(pm->ps->bitFlags & usingZanzoken)){
			pm->ps->lockedPlayer->timers[tmKnockback] = 250;
			if(melee2 < 500){melee2 = 500;}
		}
		// Auto Close-in
		movingForward = pm->cmd.forwardmove == 127 ? qtrue : qfalse;
		if(pm->cmd.forwardmove > 0){
			if((state != 4 && distance > 32 && distance < 512) || (pm->ps->bitFlags & usingZanzoken)){
				pm->ps->bitFlags |= isAutoClosing;
				if(!pm->ps->lockedPlayer->timers[tmKnockback] && (pm->cmd.buttons & BUTTON_ALT_ATTACK || pm->cmd.buttons & BUTTON_ATTACK)){
					pm->cmd.forwardmove = 127;
				}
			}
		}
		else if(state == 3){
			pm->cmd.forwardmove = 0;
		}
		// Release Power Melee
		if(state == 3){
			if(inRange){PM_StopMovement();}
			else{pm->cmd.rightmove = 0;}
		}
		if(state == 3 && (!(pm->cmd.buttons & BUTTON_ALT_ATTACK) || melee2 >= 750) && distance <= 96){
			damage = ((float)melee2 / 1500.0) * (pm->ps->powerLevel[plCurrent] * 0.25) * pm->ps->stats[stMeleeAttack];
			if(melee2 >= 750){
				knockback = qtrue;
				PM_AddEvent(EV_MELEE_KNOCKBACK);
			}
			pm->ps->timers[tmFreeze] = 1000;
			//PM_AddEvent(EV_MELEE_SPEED);
			melee2 = -1000;
			state = 4;
		}
		// Cooldown Speed Melee
		if(melee1 < 0){
			melee1 += pml.msec;
			if(melee1 >= 0){
				melee1 = 0;
				state = 1;
			}
		}
		// Cooldown Power Melee
		else if(melee2 < 0){
			melee2 += pml.msec;
			if(melee2 >= 0){melee2 = 0;}
		}
		else if(melee1 >=0 || melee2 >= 0){
			// Charging Power Melee
			if(!(pm->cmd.buttons & BUTTON_ATTACK) && pm->cmd.buttons & BUTTON_ALT_ATTACK && !(enemyState == 2 && (state == 6 || state <= 1))){
				state = 3;
				melee2 += pml.msec;
				if(melee2 >= 750){melee2 = 750;}
			}
			// Using Speed Melee
			else if(pm->cmd.buttons & BUTTON_ATTACK && inRange){
				damage = (pm->ps->powerLevel[plFatigue] * 0.013) * pm->ps->stats[stMeleeAttack];
				pm->cmd.forwardmove = 126;
				melee1 += pml.msec;
				state = 2;
			}
			else if(state == 2){
				melee1 = -400;
			}
			// Using Block
			else if(pm->cmd.buttons & BUTTON_BLOCK){
				int direction,blockDirection;
				direction = pm->ps->knockBackDirection;
				state = 5;
				if(enemyState == 4){
					if(movingForward && direction == 1){}
					else if(pm->cmd.forwardmove < 0 && direction == 2){}
					else if(pm->cmd.rightmove > 0 && direction == 3){}
					else if(pm->cmd.rightmove < 0 && direction == 4){}
					else if(pm->cmd.forwardmove == 0 && pm->cmd.rightmove == 0 && direction == 0){}
					else{
						state = 1;
					}
				}
			}
			// Using Zanzoken
			else if(pm->cmd.buttons & BUTTON_LIGHTSPEED){state = 6;}
			else{
				state = 1;
			}
		}
		// Reaction Events
		if(!inRange && (state > 1 && state != 3 && state != 4)){
			state = 1;
		}
		// Speed Melee Hit
		if(enemyState == 2 && (state == 6 || state <= 1)){
			pm->ps->timers[tmFreeze] = 300;
			PM_ContinueLegsAnim(LEGS_SPEED_MELEE_HIT);
		}
		// Block Power Melee
		else if(enemyState == 4 && state == 5){
			PM_ContinueLegsAnim(LEGS_BLOCK);
		}
		// Block / Dodge Speed Melee
		else if(enemyState == 2 && state == 5){
			if(pm->ps->bitFlags & usingBoost){
				PM_ContinueLegsAnim(LEGS_SPEED_MELEE_BLOCK);
				PM_AddEvent(EV_MELEE_SPEED);
			}
			else{
				pm->ps->timers[tmFreeze] = 300;
				PM_ContinueLegsAnim(LEGS_SPEED_MELEE_DODGE);
				PM_AddEvent(EV_MELEE_MISS);
			}
		}
		// Speed Melee Using
		else if(state == 2){
			PM_ContinueLegsAnim(LEGS_SPEED_MELEE_ATTACK);
			if(pm->ps->bitFlags & usingBoost){
				damage *= 2.2;
			}
			else{
				PM_AddEvent(EV_MELEE_SPEED);
			}
			while(melee1 >= 100){
				melee1 -= 100;
				if(enemyState == 5){
					if(!(pm->ps->lockedPlayer->bitFlags & usingBoost)){
						pm->ps->lockedPlayer->powerLevel[plUseCurrent] += damage;
					}
					else{
						pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] += damage * 0.2;
					}
					pm->ps->powerLevel[plUseFatigue] += damage * 1.5;
				}
				else{pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] += damage;}
			}
		}
		else if(state == 3){
			int desired = 0;
			if(movingForward && inRange){desired = 1;}
			else if(pm->cmd.forwardmove < 0){desired = 2;}
			else if(pm->cmd.rightmove > 0){desired = 3;} 
			else if(pm->cmd.rightmove < 0){desired = 4;}
			if(!pm->ps->lockedPlayer->knockBackDirection && !pm->ps->lockedPlayer->timers[tmKnockback]){
				pm->ps->lockedPlayer->knockBackDirection = desired;
			}
		}
		else if(state == 4){
			if(knockback && enemyState < 5){
				pm->ps->lockedPlayer->timers[tmKnockback] = 5000;
				pm->ps->lockedPlayer->powerups[PW_KNOCKBACK_SPEED] = (pm->ps->powerLevel[plCurrent] / 21.84) + pm->ps->stats[stKnockbackPower];
				pm->ps->lockedPlayer->powerups[PW_MELEE_STATE] = 0;
			}
			if(enemyState == 5){damage *= 0.6;}
			if(enemyState != 6){
				pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] += damage;
			}
			if(pm->ps->persistant[PERS_TEAM] != pm->ps->lockedPlayer->persistant[PERS_TEAM] && pm->ps->persistant[PERS_TEAM] != TEAM_FREE){
				if(pm->ps->lockedPlayer->powerLevel[plHealth] <= damage && pm->ps->lockedPlayer->powerLevel[plHealth] > 0){
					PM_AddEvent(EV_MELEE_KNOCKOUT);
				}
			} else if (pm->ps->persistant[PERS_TEAM] == TEAM_FREE) {
				if(pm->ps->lockedPlayer->powerLevel[plHealth] <= damage && pm->ps->lockedPlayer->powerLevel[plHealth] > 0){
					PM_AddEvent(EV_MELEE_KNOCKOUT);
				}
			}
		}
		else if(state == 1 && inRange){
			if(pm->ps->bitFlags & usingFlight){PM_ContinueLegsAnim(LEGS_FLY_IDLE);}
			else{PM_ContinueLegsAnim(LEGS_IDLE);}
		}
		if(state == 3 || state == 4){
			int direction;
			direction = pm->ps->lockedPlayer->knockBackDirection;
			if(direction == 1){state == 4 ? PM_ContinueLegsAnim(LEGS_POWER_MELEE_4_HIT) : PM_ContinueLegsAnim(LEGS_POWER_MELEE_4_CHARGE);}
			else if(direction == 2){state == 4 ? PM_ContinueLegsAnim(LEGS_POWER_MELEE_2_HIT) : PM_ContinueLegsAnim(LEGS_POWER_MELEE_2_CHARGE);}
			else if(direction == 3){state == 4 ? PM_ContinueLegsAnim(LEGS_POWER_MELEE_6_HIT) : PM_ContinueLegsAnim(LEGS_POWER_MELEE_6_CHARGE);}
			else if(direction == 4){state == 4 ? PM_ContinueLegsAnim(LEGS_POWER_MELEE_5_HIT) : PM_ContinueLegsAnim(LEGS_POWER_MELEE_5_CHARGE);}
			else if(pm->cmd.buttons & BUTTON_BOOST){
				state == 4 ? PM_ContinueLegsAnim(LEGS_POWER_MELEE_3_HIT) : PM_ContinueLegsAnim(LEGS_POWER_MELEE_3_CHARGE);
			}
			else{
				state == 4 ? PM_ContinueLegsAnim(LEGS_POWER_MELEE_1_HIT) : PM_ContinueLegsAnim(LEGS_POWER_MELEE_1_CHARGE);
			}
		}
		// Set Melee Position Mode
		if((state > 1 || enemyState > 1)){
			if(inRange){
				pm->cmd.forwardmove *= 0.5;
				pm->cmd.rightmove *= 0.5;
				pm->cmd.upmove *= 0.5;
				if(enemyState == 0){
					pm->ps->lockedPlayer->lockedTarget = pm->ps->clientNum + 1;
					pm->ps->lockedPlayer->clientLockedTarget = pm->ps->clientNum + 1;
				}
			}
			pm->ps->bitFlags |= usingMelee;
		}
		pm->ps->powerups[PW_MELEE_STATE] = state;
		pm->ps->timers[tmMelee1] = melee1;
		pm->ps->timers[tmMelee2] = melee2;
	}
}
/*==============
PM_Weapon
Generates weapon events and modifes the weapon counter
==============*/
void PM_WeaponRelease(void){
	if(pm->ps->bitFlags & isGuiding){
		PM_AddEvent(EV_DETONATE_WEAPON);
		pm->ps->bitFlags &= ~isGuiding;
	}
	pm->ps->weaponstate = WEAPON_READY;
	pm->ps->stats[stChargePercentPrimary] = 0;
	pm->ps->stats[stChargePercentSecondary] = 0;
}
void PM_Weapon(void){
	int	*weaponInfo;
	int	*alt_weaponInfo;
	int chargeRate;
	int costPrimary,costSecondary;	
	if(pm->ps->weaponstate != WEAPON_GUIDING){pm->ps->bitFlags &= ~isGuiding;}
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR){return;}
	if(pm->ps->bitFlags & isGuiding){
		PM_StopMovement();
		PM_StopDash();
	}
	if(pm->cmd.buttons & BUTTON_POWERLEVEL || pm->ps->timers[tmTransform] > 0){
		PM_WeaponRelease();
		return;
	}
	if(pm->ps->weapon == WP_NONE || pm->ps->bitFlags & usingMelee){
		PM_WeaponRelease();
		return;
	}
	// Retrieve our weapon's settings
	weaponInfo = pm->ps->ammo;
	if(weaponInfo[WPbitFlags] & WPF_ALTWEAPONPRESENT){
		alt_weaponInfo = &weaponInfo[WPSTAT_ALT_KICOST];
	} else{
		alt_weaponInfo = weaponInfo;	 // Keep both sets the same.
		if(pm->cmd.buttons & BUTTON_ALT_ATTACK) {	// ifwe have no altfire,
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;	// override ALT_ATTACK to ATTACK.
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
	}
	/*
	We're not allowed to do anything with the weapon until the
	client has received the relevant changed data about the weapon's
	configuration from the server. This will cause a bit of stuttering
	clientside, as it 'disables' client's weapon prediction for the own
	player temporarily.
	Even then, it shouldn't happen unless you run into a bad
	network transfer rate right when you immmediately fire a weapon
	after switching. Most unlikely to occur, but a 'jump' in graphics
	is still better than a possible glitch in graphics or game physics.
	*/
	if(weaponInfo[WPSTAT_NUMCHECK] != pm->ps->weapon){
		return;
	}
	costPrimary = weaponInfo[WPSTAT_KICOST] * ((float)pm->ps->powerLevel[plMaximum] * 0.0001);
	costSecondary = alt_weaponInfo[WPSTAT_KICOST] * ((float)pm->ps->powerLevel[plMaximum] * 0.0001);;
	switch(pm->ps->weaponstate){
	case WEAPON_READY:
	case WEAPON_DROPPING:
	case WEAPON_RAISING:
	case WEAPON_COOLING:
		{
			if(pm->ps->weaponTime > 0){
				pm->ps->weaponTime -= pml.msec;
			}

			if(pm->ps->weaponTime > 0){
				break;
			} else{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
			}
			PM_BeginWeaponChange(pm->cmd.weapon );
			if(pm->ps->weaponstate == WEAPON_DROPPING){
				PM_FinishWeaponChange();
				break;
			}

			if(pm->ps->weaponstate == WEAPON_RAISING){
				pm->ps->weaponstate = WEAPON_READY;
				PM_StartTorsoAnim(TORSO_STAND);
				break;
			}

			// Starting a primary attack
			if(pm->cmd.buttons & BUTTON_ATTACK) {
				if(weaponInfo[WPbitFlags] & WPF_NEEDSCHARGE){
					pm->ps->weaponstate = WEAPON_CHARGING;
					PM_StartTorsoAnim(TORSO_KI_ATTACK1_PREPARE + (pm->ps->weapon - 1) * 2 );
					break;
				}
				pm->ps->powerLevel[plUseCurrent] += costPrimary;
				if(weaponInfo[WPbitFlags] & WPF_CONTINUOUS){
					PM_AddEvent(EV_FIRE_WEAPON );
					pm->ps->weaponstate = WEAPON_FIRING;
				} else{
					PM_AddEvent(EV_FIRE_WEAPON);
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				}
				PM_StartTorsoAnim(TORSO_KI_ATTACK1_FIRE + (pm->ps->weapon - 1) * 2 );
				break;
			}
			if(pm->cmd.buttons & BUTTON_ALT_ATTACK){
				if(alt_weaponInfo[WPbitFlags] & WPF_NEEDSCHARGE){
					pm->ps->weaponstate = WEAPON_ALTCHARGING;
					PM_StartTorsoAnim(TORSO_KI_ATTACK1_ALT_PREPARE + (pm->ps->weapon - 1) * 2 );
					break;
				}
				pm->ps->powerLevel[plUseCurrent] += costSecondary;
				if(alt_weaponInfo[WPbitFlags] & WPF_CONTINUOUS){
					pm->ps->weaponstate = WEAPON_ALTFIRING;
					PM_AddEvent(EV_ALTFIRE_WEAPON );
				} else{
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
					PM_AddEvent(EV_ALTFIRE_WEAPON );
				}
				PM_StartTorsoAnim(TORSO_KI_ATTACK1_ALT_FIRE + (pm->ps->weapon - 1) * 2 );
				break;
			}
			PM_ContinueTorsoAnim(TORSO_STAND);
		}
		break;
	case WEAPON_GUIDING:
		{
			if(pm->cmd.buttons & BUTTON_ATTACK){
				PM_AddEvent(EV_DETONATE_WEAPON );
				pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim(TORSO_STAND);
			}
			pm->ps->velocity[0] = 0.0f;
			pm->ps->velocity[1] = 0.0f;
			pm->ps->velocity[2] = 0.0f;
		}
		break;
	case WEAPON_ALTGUIDING:
		{
			if(pm->cmd.buttons & BUTTON_ALT_ATTACK){
				PM_AddEvent(EV_DETONATE_WEAPON );
				pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim(TORSO_STAND );
			}
			pm->ps->velocity[0] = 0.0f;
			pm->ps->velocity[1] = 0.0f;
			pm->ps->velocity[2] = 0.0f;
		}
		break;
	case WEAPON_CHARGING:
		{
			chargeRate = (pm->ps->bitFlags & usingBoost) ? 6 : 1;
			costPrimary *= (pm->ps->bitFlags & usingBoost) ? 5 : 1;
			pm->ps->timers[tmAttack1] += pml.msec;
			if(pm->ps->timers[tmAttack1] >= weaponInfo[WPSTAT_CHRGTIME]){
				pm->ps->timers[tmAttack1] -= weaponInfo[WPSTAT_CHRGTIME];
				if(pm->ps->stats[stChargePercentPrimary] < 100){
					pm->ps->stats[stChargePercentPrimary] += chargeRate;
					if(pm->ps->stats[stChargePercentPrimary] > 100){
						pm->ps->stats[stChargePercentPrimary] = 100;
					}
					pm->ps->powerLevel[plUseCurrent] += costPrimary;
				}
			}
			if(!(pm->cmd.buttons & BUTTON_ATTACK)) {
				pm->ps->timers[tmAttack1] = 0;
				if(weaponInfo[WPbitFlags] & WPF_READY){
					weaponInfo[WPbitFlags] &= ~WPF_READY;
					if(weaponInfo[WPbitFlags] & WPF_GUIDED){
						pm->ps->weaponstate = WEAPON_GUIDING;
						pm->ps->bitFlags |= isGuiding;
					} else{
						pm->ps->weaponstate = WEAPON_COOLING;
						pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
					}
					PM_AddEvent(EV_FIRE_WEAPON);
					PM_StartTorsoAnim(TORSO_KI_ATTACK1_FIRE + (pm->ps->weapon - 1) * 2 );
				} else{
					pm->ps->weaponTime = 0;
					pm->ps->weaponstate = WEAPON_READY;
					pm->ps->stats[stChargePercentPrimary] = 0;
					PM_StartTorsoAnim(TORSO_STAND);
				}
			}
		}
		break;
	case WEAPON_ALTCHARGING:
		{
			pm->ps->timers[tmAttack2] += pml.msec;
			if(pm->ps->timers[tmAttack2] >= weaponInfo[WPSTAT_ALT_CHRGTIME]){
				pm->ps->timers[tmAttack2] -= weaponInfo[WPSTAT_ALT_CHRGTIME];
				if(pm->ps->stats[stChargePercentSecondary] < 100){
					pm->ps->stats[stChargePercentSecondary] += 1;
					if(pm->ps->stats[stChargePercentSecondary] > 100){
						pm->ps->stats[stChargePercentSecondary] = 100;
					}
					pm->ps->powerLevel[plUseCurrent] += costPrimary;
				}
			}
			if(!(pm->cmd.buttons & BUTTON_ALT_ATTACK)) {
				if(alt_weaponInfo[WPbitFlags] & WPF_READY){
					alt_weaponInfo[WPbitFlags] &= ~WPF_READY;
					if(alt_weaponInfo[WPbitFlags] & WPF_GUIDED){
						pm->ps->weaponstate = WEAPON_ALTGUIDING;
						pm->ps->bitFlags |= isGuiding;
					} else{
						pm->ps->weaponstate = WEAPON_COOLING;
						pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
					}
					PM_AddEvent(EV_ALTFIRE_WEAPON );
					PM_StartTorsoAnim(TORSO_KI_ATTACK1_ALT_FIRE + (pm->ps->weapon - 1) * 2 );
				} else{
					pm->ps->weaponTime = 0;
					pm->ps->weaponstate = WEAPON_READY;
					pm->ps->stats[stChargePercentSecondary] = 0;
					PM_StartTorsoAnim(TORSO_STAND );
				}
			}
		}
		break;
	case WEAPON_FIRING:
		{
			if(!(pm->cmd.buttons & BUTTON_ATTACK)){
				pm->ps->weaponstate = WEAPON_COOLING;
				pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim(TORSO_STAND );
				break;
			}
			pm->ps->weaponTime += pml.msec;
			while(pm->ps->weaponTime > 100){
				pm->ps->powerLevel[plUseCurrent] += costPrimary;
			}

		}
		break;
	case WEAPON_ALTFIRING:
		{
			if(!(pm->cmd.buttons & BUTTON_ALT_ATTACK)){
				pm->ps->weaponstate = WEAPON_COOLING;
				pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
				PM_StartTorsoAnim(TORSO_STAND );
				break;
			}
			pm->ps->weaponTime += pml.msec;
			while(pm->ps->weaponTime > 100){
				pm->ps->powerLevel[plUseCurrent] += costSecondary;
			}
		}
		break;
	default:
		break;
	}
}
/*================
PM_CheckLockon
================*/
void PM_StopLockon(void){
	if(pm->ps->lockedTarget>0){
		PM_AddEvent(EV_LOCKON_END);
	}
	pm->ps->lockedPosition = NULL;
	pm->ps->lockedTarget = 0;
	pm->ps->clientLockedTarget = 0;
}
void PM_CheckLockon(void){
	int	lockBox;
	trace_t	trace;
	vec3_t minSize,maxSize,forward,up,end;
	if(pm->ps->lockedTarget && (pm->ps->bitFlags & usingFlight) && !(pm->ps->lockedPlayer->bitFlags & usingFlight)){
		PM_StopLockon();
	}
	if(pm->cmd.buttons & BUTTON_GESTURE && pm->ps->powerups[PW_MELEE_STATE] == 0){
		if(pm->ps->pm_flags & PMF_LOCK_HELD){return;}
		pm->ps->pm_flags |= PMF_LOCK_HELD;
		if(pm->ps->lockedTarget>0){
			PM_StopLockon();
			return;
		}
		AngleVectors(pm->ps->viewangles,forward,NULL,NULL);
		VectorMA(pm->ps->origin,131072,forward,end);
		lockBox = 250;
		minSize[0] = -lockBox;
		minSize[1] = -lockBox;
		minSize[2] = -lockBox;
		maxSize[0] = -minSize[0];
		maxSize[1] = -minSize[1];
		maxSize[2] = -minSize[2];
		pm->trace(&trace,pm->ps->origin,minSize,maxSize,end,pm->ps->clientNum,CONTENTS_BODY);
		if((trace.entityNum >= MAX_CLIENTS)){return;}
		PM_AddEvent(EV_LOCKON_START);
		pm->ps->lockedTarget = trace.entityNum+1;
		pm->ps->clientLockedTarget = trace.entityNum+1;
	}
	else{
		pm->ps->pm_flags &= ~PMF_LOCK_HELD;
	}
}
/*
================
PM_DropTimers
================
*/
void PM_DropTimers(void){
	if(pm->ps->pm_time){
		if(pml.msec >= pm->ps->pm_time){
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else{
			pm->ps->pm_time -= pml.msec;
		}
	}
	if(pm->ps->legsTimer > 0){
		pm->ps->legsTimer -= pml.msec;
		if(pm->ps->legsTimer < 0){
			pm->ps->legsTimer = 0;
		}
	}
	if(pm->ps->torsoTimer > 0){
		pm->ps->torsoTimer -= pml.msec;
		if(pm->ps->torsoTimer < 0){
			pm->ps->torsoTimer = 0;
		}
	}
	if(pm->ps->lockTimer > 0){
		pm->ps->lockTimer -= pml.msec;
		if(pm->ps->lockTimer < 0){
			pm->ps->lockTimer = 0;
		}
	}
}

/*================
PM_UpdateViewAngles
================*/
void PM_UpdateViewAngles(playerState_t *ps, const usercmd_t *cmd){
	short		temp;
	int			i;
	int			roll = 0;
	float		oldCmdAngle;
	vec3_t		offset_angles;
	vec4_t		quatOrient;
	vec4_t		quatRot;
	vec4_t		quatResult;
	vec3_t		new_angles;
	float		pitchNorm;
	if(ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION) {
		return;		// no view changes at all
	}
	if((pm->ps->lockedTarget > 0) && !(pm->ps->bitFlags & usingAlter) && *ps->lockedPosition){
		vec3_t dir;
		vec3_t angles;
		VectorSubtract(*(ps->lockedPosition),ps->origin,dir);
		vectoangles(dir, angles);
		if(angles[PITCH] > 180) { 
			angles[PITCH] -= 360;
		}
		else if(angles[PITCH] < -180) {
			angles[PITCH] += 360;
		}
		for (i = 0; i < 3; i++) {
			ps->delta_angles[i] = ANGLE2SHORT(angles[i]) - cmd->angles[i];
		}
	}
	else if(pm->ps->bitFlags & usingFlight) {
		if(cmd->buttons & BUTTON_ROLL_LEFT){
			roll -= 28 * (pml.msec / 200.0f);
		}
		if(cmd->buttons & BUTTON_ROLL_RIGHT) {
			roll += 28 * (pml.msec / 200.0f);
		}
		for (i=0; i<3; i++) {
			// Get the offsets for the angles
			oldCmdAngle = ps->viewangles[i] - SHORT2ANGLE(ps->delta_angles[i]);
			offset_angles[i] = AngleNormalize180(SHORT2ANGLE(cmd->angles[i]) - oldCmdAngle);
		}
		// Don't forget to add our customized roll function if
		// we're not guiding a weapon!
		if(!(pm->ps->bitFlags & isGuiding)){
			offset_angles[ROLL] = AngleNormalize180(offset_angles[ROLL] + roll);
		}

		// There's a drifting problem with the quaternions' rounding, so when we DON'T change
		// our viewangles at all, we don't run through the quaternions to prevent drifting.
		// (As a bonus; this also means we won't be doing needless matrix manipulations.)

		if(((offset_angles[0] == 0) && (offset_angles[1] == 0)) && (offset_angles[2] == 0)) {
			return;
		}

		AnglesToQuat(ps->viewangles, quatOrient);
		AnglesToQuat(offset_angles, quatRot);
		QuatMul(quatOrient, quatRot, quatResult);
		QuatToAngles(quatResult, new_angles);

		// HACK: Because an exact pitch of -90 degrees STILL causes trouble troughout the
		//       code due to 'bad mathematics', we give a slight nudge ifthis threatens
		//       to happen. 2 degrees is not really noticeable, and is big enough to
		//       compensate for round off errors leading to -90 degrees of pitch.
		// NOTE: For some reason the program 'auto-magically' prevents angles of 90 degree
		//       pitch, by forcing you into a bit more yaw. Strange...
		AngleNormalize180(new_angles[PITCH]);
		if((new_angles[PITCH] < -89.0f)&& (new_angles[PITCH] > -91.0f)) {
			if(ps->viewangles[PITCH] < new_angles[PITCH]){
				new_angles[PITCH] += 2;
			} else{
				new_angles[PITCH] -= 2;
			}
		}

		// Correct the 3 delta_angles;
		for (i=0; i<3; i++) {
			ps->viewangles[i] = AngleNormalize180(new_angles[i]);
			ps->delta_angles[i] = ANGLE2SHORT(ps->viewangles[i]) - cmd->angles[i];
		}
		return;
	}

	// We've been flying and we've inverted our pitch to a good amount upside down.
	// Cheat the view, by adjusting the yaw to 'swing around' and adjusting the pitch to match.
	pitchNorm = AngleNormalize180(ps->viewangles[PITCH]);
	if(pitchNorm > 100){
		ps->delta_angles[YAW] += ANGLE2SHORT(180);
		ps->delta_angles[PITCH] -= ANGLE2SHORT(pitchNorm);
		ps->delta_angles[PITCH] += ANGLE2SHORT(180 - pitchNorm);
	}
	if(pitchNorm < -100){
		ps->delta_angles[YAW] += ANGLE2SHORT(180);
		ps->delta_angles[PITCH] -= ANGLE2SHORT(pitchNorm);
		ps->delta_angles[PITCH] += ANGLE2SHORT(-180 - pitchNorm);
	}
	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if(i == PITCH){
			// don't let the player look up or down more than 90 degrees
			if(temp > 16000){
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
				pm->cmd.rightmove *= 0.1;
				pm->cmd.upmove *= 0.1;
			} else if(temp < -16000){
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
				pm->cmd.rightmove *= 0.1;
				pm->cmd.upmove *= 0.1;
			}
			// Slow down side movement speed the more pitch we have + or - during lockon.
			if(pm->ps->lockedTarget > 0){
				if(temp > 2000 || temp < -2000){
					pm->cmd.rightmove *= 0.5;
					pm->cmd.upmove *= 0.5;
				} else if(temp > 4000 || temp < -4000){
					pm->cmd.rightmove *= 0.25;
					pm->cmd.upmove *= 0.25;
				} else if(temp > 8000 || temp < -8000){
					pm->cmd.rightmove *= 0.1;
					pm->cmd.upmove *= 0.1;
				} else if(temp > 9000){
					ps->delta_angles[i] = 9000 - cmd->angles[i];
					temp = 9000;
					pm->cmd.rightmove *= 0;
					pm->cmd.upmove *= 0;
				} else if(temp < -9000){
					ps->delta_angles[i] = -9000 - cmd->angles[i];
					temp = -9000;
					pm->cmd.rightmove *= 0;
					pm->cmd.upmove *= 0;
				}
			}
		}
		// ADDING FOR ZEQ2
		// Incase we've been flying, our roll might've been changed.
		if(i == ROLL && ps->viewangles[i] != 0) {
			ps->viewangles[i] = 0;
			ps->delta_angles[i] = 0 - cmd->angles[i];
		}
		// END ADDING
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}
}

/*================
PmoveSingle
================*/
void trap_SnapVector(float *v );
void PmoveSingle(pmove_t *pmove){
	int state;
	qboolean meleeRange;
	pm = pmove;
	c_pmove++;
	meleeRange = qfalse;
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;
	pm->ps->gravity = pm->ps->bitFlags & usingJump ? pm->ps->gravity : 800;
	pm->ps->eFlags &= ~EF_AURA;
	memset (&pml, 0, sizeof(pml));
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if(pml.msec < 1){pml.msec = 1;}
	else if(pml.msec > 200){pml.msec = 200;}
	if(abs(pm->cmd.forwardmove)> 64 || abs(pm->cmd.rightmove)> 64){pm->cmd.buttons &= ~BUTTON_WALKING;}
	PM_CheckKnockback();
	PM_CheckTransform();
	PM_CheckLoopingSound();
	if(pm->ps->lockedTarget > 0){
		meleeRange = Distance(pm->ps->origin,*(pm->ps->lockedPosition)) <= 32 ? qtrue : qfalse;
	}
	if(!(pm->ps->bitFlags & isTransforming)){
		PM_UsePowerLevel(qtrue);
		PM_UsePowerLevel(qfalse);
		PM_BurnPowerLevel(qtrue);
		PM_BurnPowerLevel(qfalse);
		PM_CheckStatus();
		if(!pm->ps->timers[tmKnockback] && !(pm->ps->bitFlags & isUnconcious) && !(pm->ps->bitFlags & isDead)){
			PM_CheckBoost();
			PM_CheckPowerLevel();
			PM_CheckLockon();
			PM_CheckZanzoken();
			if(!meleeRange){
				if(!(pm->ps->bitFlags & usingAlter)){
					PM_CheckJump();
					PM_CheckBlock();
				}
				PM_Footsteps();
			}
		}
	}
	PM_CheckTalk();
	pm->ps->commandTime = pmove->cmd.serverTime;
	VectorCopy(pm->ps->origin, pml.previous_origin);
	VectorCopy(pm->ps->velocity, pml.previous_velocity);
	pml.frametime = pml.msec * 0.001;
	PM_DropTimers();
	AngleVectors(pm->ps->viewangles,pml.forward,pml.right,pml.up);
	PM_SetView();
	if(pm->ps->bitFlags & isUnconcious || pm->ps->bitFlags & isDead){
		PM_StopMovementTypes();
		PM_StopFlight();
		PM_StopDirections();
		PM_StopLockon();
		PM_GroundTrace();
		PM_AirMove();
		PM_UpdateViewAngles(pm->ps, &pm->cmd);
		return;
	}
	if(!pm->ps->timers[tmKnockback]){
		PM_Melee();
		if(pm->ps->powerups[PW_MELEE_STATE] != 4){
			PM_UpdateViewAngles(pm->ps, &pm->cmd);
			PM_Weapon();
		}
	}
	if(pm->ps->pm_type == PM_SPECTATOR){
		PM_FlyMove();
	}
	pml.previous_waterlevel = pmove->waterlevel;
	PM_SetWaterLevel();
	PM_TorsoAnimation();
	PM_WaterEvents();
	PM_Freeze();
	PM_GroundTrace();
	if(!pm->ps->timers[tmFreeze]){
		if(!(pm->ps->bitFlags & usingAlter)){
			PM_FlyMove();
			PM_DashMove();
			PM_WalkMove();
			PM_AirMove();
		}
	}
	PM_Friction();
	PM_GroundTrace();
	trap_SnapVector(pm->ps->velocity);
}

/*================
Pmove
================*/
void Pmove(pmove_t *pmove){
	int	finalTime;
	finalTime = pmove->cmd.serverTime;
	if(finalTime < pmove->ps->commandTime){return;}
	if(finalTime > pmove->ps->commandTime + 1000){
		pmove->ps->commandTime = finalTime - 1000;
	}
	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount+1) & ((1<<PS_PMOVEFRAMECOUNTBITS)-1);
	while(pmove->ps->commandTime != finalTime){
		int	msec;
		msec = finalTime - pmove->ps->commandTime;
		if(pmove->pmove_fixed){
			if(msec > pmove->pmove_msec){
				msec = pmove->pmove_msec;
			}
		}
		else if(msec > 66){
			msec = 66;
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle(pmove);
	}
}

