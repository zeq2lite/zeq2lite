/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../../Shared/q_shared.h"
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
int PM_CheckDirection(vec3_t direction,qboolean player);
void PM_StopBoost(void);
void PM_StopZanzoken(void);
void PM_StopDash(void);
void PM_StopJump(void);
void PM_StopSoar(void);
void PM_StopFlight(void);
void PM_StopMovement(void);
void PM_StopMelee(void);
void PM_StopDirections(void);
void PM_StopLockon(void);
void PM_EndDrift(void);
void PM_Crash(qboolean vertical);
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
		if(pm->ps->timers[tmFreeze]<0){pm->ps->timers[tmFreeze] = 0;}
	}
}
/*================
IMPEDE
================*/
void PM_Impede(void){
	if(pm->ps->timers[tmImpede] > 0){
		pm->ps->timers[tmImpede] -= pml.msec;
		if(pm->ps->timers[tmImpede] <= 0){pm->ps->timers[tmImpede] = 0;}
		if(usingJump && (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING)){pm->ps->bitFlags = usingFlight;}
		VectorClear(pm->ps->velocity);
		PM_StopDirections();
		PM_StopZanzoken();
	}
}
/*================
RIDE
================*/
void PM_Ride(void){
	if(pm->ps->states & isRiding){
		if(usingJump && (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING)){pm->ps->bitFlags = usingFlight;}
		VectorClear(pm->ps->velocity);
		//PM_StopDirections();
		PM_StopZanzoken();
		pm->cmd.forwardmove = -1;
	}
}
/*================
Burn
================*/
void PM_Burn(void){
	if(pm->ps->states & isBurning){
		if(usingJump && (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING)){pm->ps->bitFlags = usingFlight;}
		VectorClear(pm->ps->velocity);
		PM_StopDirections();
		PM_StopZanzoken();
	}
}
/*===================
BLIND
===================*/
void PM_Blind(void){
	if(pm->ps->timers[tmBlind] > 0){
		pm->ps->timers[tmBlind] -= pml.msec;
		if(pm->ps->timers[tmBlind]<0){pm->ps->timers[tmBlind] = 0;}
	}
}
/*===================
CHECK LOOPING SOUND
===================*/
void PM_CheckLoopingSound(void){
	if(pm->ps->timers[tmKnockback] ||
		pm->ps->timers[tmMeleeCharge] != 0 ||
		pm->ps->timers[tmMeleeIdle]  ||
		pm->ps->bitFlags & usingMelee ||
		pm->ps->bitFlags & isUnconcious ||
		pm->ps->bitFlags & isCrashed ||
		pm->ps->bitFlags & isDead){
		PM_AddEvent(EV_STOPLOOPINGSOUND);
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
	vec3_t pre_vel,post_vel,wishvel,wishdir,direction;
	qboolean vertical;
	float scale,wishspeed;
	int i,speed;
	vertical = qfalse;
	if(pm->ps->timers[tmFreeze]){return;}
	if(pm->ps->timers[tmKnockback] < 0){
		pm->ps->timers[tmKnockback] += pml.msec;
		PM_ContinueLegsAnim(ANIM_KNOCKBACK_RECOVER_2);
		if(pm->ps->timers[tmKnockback] >= 0){
			pm->ps->timers[tmKnockback] = 0;
		}
	}
	if(pm->ps->timers[tmKnockback] > 0){
		PM_ContinueLegsAnim(ANIM_KNOCKBACK);
		pm->ps->timers[tmKnockback] -= pml.msec;
		PM_StopDirections();
		VectorScale(pml.forward,-1,direction);
		if(pm->ps->timers[tmKnockback] > 0){
			PM_WeaponRelease();
			if(pm->ps->knockBackDirection == 1){
				VectorCopy(pml.up,direction);
				vertical = qtrue;
				pm->cmd.upmove = 127;
			}
			else if(pm->ps->knockBackDirection == 2){
				VectorScale(pml.up,-1.0,direction);
				vertical = qtrue;
				pm->cmd.upmove = -127;
			}
			else if(pm->ps->knockBackDirection == 3){
				VectorScale(pml.right,-1.0,direction);
				pm->cmd.rightmove = -127;
			}
			else if(pm->ps->knockBackDirection == 4){
				VectorCopy(pml.right,direction);
				pm->cmd.rightmove = 127;
			}
			else if(pm->ps->knockBackDirection == 5){
				VectorScale(pml.forward,-1.0,direction);
				pm->cmd.forwardmove = -127;
			}
			else if(pm->ps->knockBackDirection == 6){
				VectorCopy(pml.forward,direction);
				pm->cmd.forwardmove = 127;
			}
		}
		else{
			pm->ps->timers[tmKnockback] = 0;
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
		if((pm->cmd.buttons & BUTTON_ALT_ATTACK) && (pm->ps->timers[tmKnockback] < 4000)){
			pm->ps->powerLevel[plUseFatigue] += ((float)pm->ps->timers[tmKnockback] / 5000.0) * (0.2 * pm->ps->powerLevel[plFatigue]);
			pm->ps->timers[tmKnockback] = -500;
			speed = 0;
		}
		else if(pm->cmd.buttons & BUTTON_ATTACK){
			pm->ps->timers[tmKnockback] -= pml.msec;
			//PM_ContinueLegsAnim(ANIM_KNOCKBACK_RECOVER_1);
			speed /= 2;
		}
		VectorScale(pm->ps->velocity,speed,pm->ps->velocity);
		PM_StepSlideMove(qfalse);
		VectorNormalize2(pm->ps->velocity,post_vel);
		if(PM_CheckDirection(direction,qfalse)){PM_Crash(vertical);}
	}
}
void PM_Crash(qboolean vertical){
	pm->ps->powerLevel[plDamageGeneric] = pm->ps->bitFlags & usingSoar ? VectorLength(pm->ps->velocity) * 0.03 : VectorLength(pm->ps->velocity) * 0.3;
	pm->ps->timers[tmCrash] = vertical ? -2500 : 2500;
	pm->ps->bitFlags |= isCrashed;
	PM_AddEvent(EV_CRASH);
	PM_StopSoar();
	PM_StopKnockback();
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
	if(pm->ps->powerLevel[plFatigue] <= 1){
		PM_StopZanzoken();
		return;
	}
	if(!(pm->ps->states & canZanzoken)){return;}
	if(pm->ps->bitFlags & usingSoar || pm->ps->bitFlags & isPreparing || pm->ps->bitFlags & usingMelee){return;}
	if(!(pm->cmd.buttons & BUTTON_TELEPORT)){
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
		if(pm->ps->timers[tmZanzoken] < 0){
			pm->ps->timers[tmZanzoken] = 0;
		}
		VectorNormalize(pm->ps->velocity);
		VectorCopy(pm->ps->velocity,pre_vel);
		if(pm->ps->lockedTarget > 0){speed *= 1.5;}
		VectorScale(pm->ps->velocity,speed,pm->ps->velocity);
		VectorNormalize2(pm->ps->velocity,post_vel);
	}
	if((pm->cmd.buttons & BUTTON_TELEPORT) && !(pm->ps->bitFlags & usingZanzoken) && (pm->ps->timers[tmZanzoken] == -1)){
		PM_StopDash();
		pm->ps->bitFlags |= usingZanzoken;
		pm->ps->timers[tmZanzoken] = (pm->ps->powerLevel[plFatigue] / 93.62) + pm->ps->stats[stZanzokenDistance];
		cost = (pm->ps->powerLevel[plMaximum] * 0.12) * pm->ps->baseStats[stZanzokenCost];
		if(pm->ps->lockedTarget > 0){cost *= 0.8;}
		if(pm->ps->bitFlags & usingJump){cost *= 0.4;}
		pm->ps->powerLevel[plUseFatigue] += cost;
		PM_AddEvent(EV_ZANZOKEN_START);
	}	
}
/*===============
POWER LEVEL
===============*/
void PM_UsePowerLevel(){
	int newValue,amount,stat,useType,limit;
	useType = 0;
	limit = pm->ps->powerLevel[plLimit];
	while(useType < 4){
		stat = useType == 1 ? pm->ps->powerLevel[plFatigue] : pm->ps->powerLevel[plCurrent];
		stat = useType == 2 ? pm->ps->powerLevel[plHealth] : stat;
		stat = useType == 3 ? pm->ps->powerLevel[plMaximum] : stat;
		amount = useType == 1 ? pm->ps->powerLevel[plUseFatigue] : pm->ps->powerLevel[plUseCurrent];
		amount = useType == 2 ? pm->ps->powerLevel[plUseHealth] : amount;
		amount = useType == 3 ? pm->ps->powerLevel[plUseMaximum] : amount;
		newValue = (stat - amount) > limit ? limit : stat - amount;
		if(newValue <= 0 && useType == 0 && pm->ps->powerLevel[plFatigue] > 0){
			pm->ps->powerLevel[plFatigue] -= (amount * 1.5);
			pm->ps->powerLevel[plCurrent] = 1;
			pm->ps->powerLevel[plUseCurrent] = 0;
			return;
		}
		newValue = newValue < -limit ? -limit : newValue;
		if(useType == 1){
			pm->ps->powerLevel[plFatigue] = newValue;
			if(pm->ps->powerLevel[plFatigue] < 0){
				amount = (pm->ps->powerLevel[plFatigue]*-1)*0.75;
				if(pm->ps->powerLevel[plHealth] - amount <= 0){
					amount = pm->ps->powerLevel[plHealth] - 1;
				}
				pm->ps->powerLevel[plUseHealth] += amount;
				pm->ps->powerLevel[plFatigue] = 0;
			}
			pm->ps->powerLevel[plUseFatigue] = 0;
		}
		else if(useType == 2){
			pm->ps->powerLevel[plHealth] = newValue;
			pm->ps->powerLevel[plUseHealth] = 0;
		}
		else if(useType == 3){
			pm->ps->powerLevel[plMaximum] = newValue;
			pm->ps->powerLevel[plUseMaximum] = 0;
		}
		else{
			pm->ps->powerLevel[plCurrent] = newValue;
			pm->ps->powerLevel[plUseCurrent] = 0;
		}
		useType += 1;
	}
}
void PM_BurnPowerLevel(){
	float percent;
	int defense;
	int burn,initial;
	int newValue;
	int burnType;
	int limit;
	limit = pm->ps->powerLevel[plLimit];
	burnType = 0;
	if(pm->ps->powerLevel[plDamageFromMelee] || pm->ps->powerLevel[plDamageFromEnergy] || pm->ps->powerLevel[plDamageGeneric] || pm->ps->states & causedDamage){
		pm->ps->bitFlags |= isUnsafe;
		pm->ps->states &= ~causedDamage;
		pm->ps->timers[tmSafe] = 0;
	}
	else if(pm->ps->timers[tmSafe] > 10000){
		pm->ps->bitFlags &= ~isUnsafe;
	}
	pm->ps->timers[tmSafe] += pml.msec;
	while(burnType < 3){
		burn = burnType == 1 ? pm->ps->powerLevel[plDamageFromMelee] : pm->ps->powerLevel[plDamageFromEnergy];
		burn = burnType == 2 ? pm->ps->powerLevel[plDamageGeneric] : burn;
		if(!burn){
			burnType += 1;
			continue;
		}
		defense = burnType == 1 ? pm->ps->stats[stMeleeDefense] : pm->ps->stats[stEnergyDefense];
		defense = pm->ps->bitFlags & usingBlock ? defense * 2.0 : defense;
		defense = pm->ps->bitFlags & usingBallFlip ? defense * 1.5 : defense;
		defense = pm->ps->bitFlags & atopGround ? defense * 1.1 : defense;
		defense = (pm->cmd.buttons & BUTTON_WALKING) && pm->ps->bitFlags & atopGround ? defense * 1.5 : defense;
		initial = burn;
		percent = 1.0 - ((float)pm->ps->powerLevel[plCurrent] / (float)pm->ps->powerLevel[plMaximum]);
		burn -= (int)(((float)pm->ps->powerLevel[plFatigue] * 0.01) * defense);
		if(burnType != 2){
			pm->ps->powerLevel[plHealthPool] += burn * 0.5;
			pm->ps->powerLevel[plMaximumPool] += burn * 0.7;
		}
		if(pm->ps->powerLevel[plHealthPool] > limit){pm->ps->powerLevel[plHealthPool] = limit;}
		if(pm->ps->powerLevel[plMaximumPool] > limit){pm->ps->powerLevel[plMaximumPool] = limit;}
		newValue = pm->ps->powerLevel[plHealth] - burn;
		if(burn > 0){
			newValue = newValue > limit ? limit : newValue;
			pm->ps->powerLevel[plHealth] = newValue < 0 ? 0 : newValue;
			if(burnType != 2){
				newValue = pm->ps->powerLevel[plMaximum] + ((initial*0.7) * percent);
			}
			newValue = newValue < 0 ? 0 : newValue;
			if(burnType != 2){pm->ps->powerLevel[plMaximum] = newValue > limit ? limit : newValue;}
		}
		if(burnType == 0){pm->ps->powerLevel[plDamageFromEnergy] = 0;}
		else if(burnType == 1){pm->ps->powerLevel[plDamageFromMelee] = 0;}
		else if(burnType == 2){pm->ps->powerLevel[plDamageGeneric] = 0;}
		burnType += 1;
	}
}
void PM_CheckCrash(void){
	if(pm->ps->timers[tmCrash] > 0){
		pm->ps->timers[tmCrash] -= pml.msec;
		PM_ContinueTorsoAnim(ANIM_KNOCKBACK_HIT_WALL);
		PM_ContinueLegsAnim(ANIM_KNOCKBACK_HIT_WALL);
		if(pm->ps->timers[tmCrash] < 0){
			pm->ps->powerups[PW_STATE] = -1;
			pm->ps->timers[tmCrash] = 0;
		}
	}
	else if(pm->ps->timers[tmCrash] < 0){
		pm->ps->timers[tmCrash] += pml.msec;
		PM_ContinueTorsoAnim(ANIM_DEATH_AIR_LAND);
		PM_ContinueLegsAnim(ANIM_DEATH_AIR_LAND);
		if(pm->ps->timers[tmCrash] > 0){
			pm->ps->powerups[PW_STATE] = -1;
			pm->ps->timers[tmCrash] = 0;
		}
	}
}
void PM_CheckStatus(void){
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR){return;}
	if(pm->ps->powerLevel[plHealth] <= 0){
		if(pm->ps->powerups[PW_STATE] != -2){
			pm->ps->bitFlags |= isDead;
			pm->ps->bitFlags &= ~isStruggling;
			pm->ps->powerups[PW_STATE] = -2;
			pm->ps->timers[tmFreeze] = 0;
			PM_StopMovement();
			PM_StopFlight();
			PM_WeaponRelease();
			PM_AddEvent(EV_DEATH);
		}
		else{
			if(pm->ps->bitFlags & atopGround){
				if(!(pm->ps->bitFlags & nearGround)){
					PM_ContinueTorsoAnim(ANIM_DEATH_AIR_LAND);
					PM_ContinueLegsAnim(ANIM_DEATH_AIR_LAND);
				}
				else{
					PM_ContinueTorsoAnim(ANIM_DEATH_GROUND);
					PM_ContinueLegsAnim(ANIM_DEATH_GROUND);
				}
			}
			else{
				PM_ContinueTorsoAnim(ANIM_DEATH_AIR);
				PM_ContinueLegsAnim(ANIM_DEATH_AIR);
			}
		}
	}
	/*else if(pm->ps->powerLevel[plFatigue] <= 0){
		if(pm->ps->powerups[PW_STATE] != -1){
			pm->ps->bitFlags |= isUnconcious;
			pm->ps->bitFlags &= ~isStruggling;
			pm->ps->powerups[PW_STATE] = -1;
			pm->ps->timers[tmFreeze] = 0;
			pm->ps->powerLevel[plCurrent] = 1;
			PM_StopMovement();
			PM_StopFlight();
			PM_StopBoost();
			PM_WeaponRelease();
			PM_AddEvent(EV_UNCONCIOUS);
			if(pm->ps->powerLevel[plFatigue] > -2500){
				pm->ps->powerLevel[plFatigue] = -2500;
			}
		}
		else{
			pm->ps->powerLevel[plFatigue] += pml.msec / 2;
			if(pm->ps->bitFlags & atopGround){
				if(!(pm->ps->bitFlags & nearGround)){
					PM_ContinueTorsoAnim(ANIM_DEATH_AIR_LAND);
					PM_ContinueLegsAnim(ANIM_DEATH_AIR_LAND);
				}
				else{
					PM_ContinueTorsoAnim(ANIM_DEATH_GROUND);
					PM_ContinueLegsAnim(ANIM_DEATH_GROUND);
				}
			}
			else{
				PM_ContinueTorsoAnim(ANIM_DEATH_AIR);
				PM_ContinueLegsAnim(ANIM_DEATH_AIR);
			}
		}
	}*/
	else{
		if(pm->ps->powerups[PW_STATE] == -1){
			if(pm->ps->timers[tmRecover] <= 0){pm->ps->timers[tmRecover] = 2000;}
			pm->ps->timers[tmRecover] -= pml.msec;
			if(pm->ps->timers[tmRecover] <= 0){
				pm->ps->bitFlags &= ~isUnconcious;
				pm->ps->bitFlags &= ~isCrashed;
				pm->ps->powerups[PW_STATE] = 0;
			}
			else if(pm->ps->bitFlags & isCrashed && !(pm->ps->bitFlags & atopGround)){
				PM_StopMovement();
				pm->ps->bitFlags |= usingFlight;
				PM_ContinueTorsoAnim(ANIM_KNOCKBACK_HIT_WALL);
				PM_ContinueLegsAnim(ANIM_KNOCKBACK_HIT_WALL);
				return;
			}
			else{
				PM_ContinueTorsoAnim(ANIM_FLOOR_RECOVER);
				PM_ContinueLegsAnim(ANIM_FLOOR_RECOVER);
				return;
			}
		}
		pm->ps->powerups[PW_STATE] = 0;
	}
}
qboolean PM_CheckTransform(void){
	pm->ps->timers[tmUpdateTier] += pml.msec;
	if(pm->ps->timers[tmUpdateTier] > 300){
		pm->ps->timers[tmUpdateTier] = 0;
		if(pm->ps->stats[stTransformState] == -1){PM_AddEvent(EV_TIERDOWN);}
		else if(pm->ps->stats[stTransformState] == 1){PM_AddEvent(EV_TIERUP);}
		else if(pm->ps->stats[stTransformState] == 2){PM_AddEvent(EV_TIERUP_FIRST);}
		pm->ps->stats[stTransformState] = 0;
		PM_AddEvent(EV_TIERCHECK);
	}
	if(pm->ps->timers[tmTransform] == 1){
		pm->ps->timers[tmTransform] = -100;
		PM_AddEvent(EV_SYNCTIER);
		return qtrue;
	}
	else if(pm->ps->timers[tmTransform] == -1){
		pm->ps->timers[tmTransform] = -100;
		PM_AddEvent(EV_SYNCTIER);
		return qtrue;
	}
	else if(pm->ps->timers[tmTransform] > 0){
		PM_StopDirections();
		if(pm->ps->bitFlags & isTransforming){
			PM_ContinueLegsAnim(ANIM_TRANS_UP);
			pm->ps->eFlags |= EF_AURA;
			pm->ps->timers[tmTransform] -= pml.msec;
			if(pm->ps->timers[tmTransform] <= 0){pm->ps->timers[tmTransform] = 0;}
			PM_StopMovement();
		}
		else{
			pm->ps->bitFlags |= isTransforming;
			PM_AddEvent(EV_SYNCTIER);
			return qtrue;
		}
	}
	else if(pm->ps->timers[tmTransform] < 0){
		if(pm->ps->timers[tmTransform] > 100 || pm->ps->timers[tmTransform] < -100){PM_StopDirections();}
		pm->ps->timers[tmTransform] += pml.msec;
		if(pm->ps->timers[tmTransform] >= -1){
			pm->ps->timers[tmTransform] = 0;
		}
	}
	if(pm->ps->timers[tmTransform] == 0){
		pm->ps->bitFlags &= ~isTransforming;
	}
	return qfalse;
}
void PM_CheckPowerLevel(void){
	int plSpeed,amount,limit;
	int *timers,*powerLevel;
	float lower,raise,recovery;
	float statScale;
	float fatigueScale;
	float check;
	float pushLimit;
	int newValue;
	int idleScale;
	timers = pm->ps->timers;
	powerLevel = pm->ps->powerLevel;
	timers[tmPowerAuto] += pml.msec;
	limit = powerLevel[plLimit];
	/*if(powerLevel[plFatigue] <= 0 && powerLevel[plCurrent] > 0){
		powerLevel[plCurrent] = 1;
	}*/
	while(timers[tmPowerAuto] >= 100){
		timers[tmPowerAuto] -= 100;
		idleScale = (!pm->cmd.forwardmove && !pm->cmd.rightmove && !pm->cmd.upmove) ? 2.8 : 1;
		statScale = 1.0 - ((float)powerLevel[plCurrent] / (float)powerLevel[plMaximum]);
		if(statScale > 0.75){statScale = 0.75;}
		if(statScale < 0.25){statScale = 0.25;}
		fatigueScale = (float)powerLevel[plFatigue] / (float)powerLevel[plMaximum];
		recovery = (float)powerLevel[plMaximum] * 0.01 * idleScale;
		recovery *=  statScale;
		recovery *=  fatigueScale < 0.15 ? 0.15 : fatigueScale;
		recovery *= pm->ps->baseStats[stFatigueRecovery];
		if(pm->ps->bitFlags & usingAlter || pm->ps->bitFlags & isStruggling || pm->ps->bitFlags & usingSoar
		|| pm->ps->bitFlags & isBreakingLimit || pm->ps->weaponstate >= WEAPON_GUIDING){recovery = 0;}
		/*if(powerLevel[plCurrent] > powerLevel[plFatigue]){
			newValue = powerLevel[plCurrent] - (powerLevel[plMaximum] * 0.005);
			powerLevel[plCurrent] = (powerLevel[plCurrent] - newValue >= 0) ? newValue : 0;
		}*/
		newValue = powerLevel[plCurrent] + powerLevel[plDrainCurrent];
		if(newValue < powerLevel[plMaximum] && newValue > 0){powerLevel[plCurrent] = newValue;}
		newValue = powerLevel[plFatigue] + powerLevel[plDrainFatigue];
		if(newValue < powerLevel[plMaximum]){powerLevel[plFatigue] = newValue;}
		newValue = powerLevel[plHealth] + powerLevel[plDrainHealth];
		if(newValue < powerLevel[plMaximum]){powerLevel[plHealth] = newValue;}
		newValue = powerLevel[plMaximum] + powerLevel[plDrainMaximum];
		if(newValue < limit &&  newValue > 0){powerLevel[plMaximum] = newValue;}
		if(powerLevel[plFatigue] + recovery < powerLevel[plMaximum]){
			powerLevel[plFatigue] += recovery;
		}
		else{
			powerLevel[plFatigue] = powerLevel[plMaximum];
		}
	}
	if(pm->cmd.buttons & BUTTON_POWERLEVEL){
		qboolean canAlter;
		pm->ps->bitFlags &= ~keyTierDown;
		pm->ps->bitFlags &= ~keyTierUp;
		canAlter = qtrue;
		if(pm->ps->bitFlags & usingJump){
			pm->ps->bitFlags |= usingFlight;
			PM_StopJump();
		}
		if(pm->ps->bitFlags & usingBoost || pm->ps->timers[tmTransform] < 0 || pm->ps->bitFlags & usingSoar ||
		   pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING){
			canAlter = qfalse;
		}
		if(canAlter && pm->cmd.upmove < 0){
			PM_StopFlight();
			PM_ForceLegsAnim(ANIM_FLY_DOWN);
		}
		if(pm->cmd.forwardmove > 0){
			pm->ps->bitFlags |= keyTierUp;
			pm->ps->bitFlags &= ~usingAlter;
		}
		else if(pm->cmd.forwardmove < 0){
			pm->ps->bitFlags |= keyTierDown;
			pm->ps->bitFlags &= ~usingAlter;
		}
		else if(canAlter && pm->cmd.rightmove < 0){
			pm->ps->eFlags &= ~EF_AURA;
			lower = powerLevel[plMaximum] * 0.01;
			powerLevel[plCurrent] = (powerLevel[plCurrent] - lower > 1) ? powerLevel[plCurrent] - lower : 1;
			if(!(pm->ps->bitFlags & usingAlter)){PM_AddEvent(EV_ALTERDOWN_START);}
			pm->ps->bitFlags |= usingAlter;
		}
		else if(canAlter && (pm->cmd.rightmove > 0)){
			pm->ps->eFlags |= EF_AURA;
			timers[tmPowerRaise] += pml.msec;
			while(timers[tmPowerRaise] >= 25){
				timers[tmPowerRaise] -= 25;
				raise = powerLevel[plMaximum] * 0.009;
				if(raise < 1){raise = 1;}
				if(powerLevel[plCurrent] > powerLevel[plFatigue]){raise *= 0.6;}
				newValue = powerLevel[plCurrent] + raise;
				if(newValue > powerLevel[plMaximum]){newValue = powerLevel[plMaximum];}
				if(newValue > limit){newValue = limit;}
				powerLevel[plCurrent] = newValue;
				if(powerLevel[plCurrent] == powerLevel[plMaximum]){
					if(!(pm->ps->bitFlags & isBreakingLimit)){PM_AddEvent(EV_POWERINGUP_START);}
					pm->ps->bitFlags |= isBreakingLimit;
					pushLimit = powerLevel[plCurrent] + pm->ps->breakLimitRate;
					if(powerLevel[plHealthPool] > 0){
						if(powerLevel[plHealth] + raise * 0.3 < pushLimit){
							powerLevel[plHealth] += raise * 0.3;
							powerLevel[plUseFatigue] += raise * 0.25;
						}
					}
					powerLevel[plHealthPool] -= raise * 0.3;
					if(powerLevel[plHealthPool] < 0){powerLevel[plHealthPool] = 0;}
					if(powerLevel[plHealth] > limit){powerLevel[plHealth] = limit;}
					if(powerLevel[plMaximumPool] > 0){
						if(pushLimit < limit){
							powerLevel[plCurrent] = powerLevel[plMaximum] = pushLimit;
						}
					}
					else{
						powerLevel[plUseFatigue] += raise;
					}
					powerLevel[plMaximumPool] -= raise * 0.3;
					if(powerLevel[plMaximumPool] < 0){powerLevel[plMaximumPool] = 0;}
				}
				if(pm->ps->bitFlags & isBreakingLimit){
					if(powerLevel[plCurrent] < ((float)powerLevel[plMaximum] * 0.95)){
						pm->ps->bitFlags &= ~isBreakingLimit;
					}
				}
			}
			if(!(pm->ps->bitFlags & usingAlter)){PM_AddEvent(EV_ALTERUP_START);}
			pm->ps->bitFlags |= usingAlter;
		}
		else{
			pm->ps->bitFlags &= ~usingAlter;
			pm->ps->bitFlags &= ~isBreakingLimit;
		}
	}
	else{
		pm->ps->bitFlags &= ~usingAlter;
		pm->ps->bitFlags &= ~keyTierDown;
		pm->ps->bitFlags &= ~keyTierUp;
		pm->ps->bitFlags &= ~isBreakingLimit;
	}
}
void PM_CheckHover(void){
	if(VectorLength(pm->ps->velocity) && pm->ps->bitFlags & usingFlight){
		if(pm->cmd.buttons & BUTTON_WALKING){
			if(!(pm->ps->states & isHovering)){
				pm->ps->states |= isHovering;
				PM_AddEvent(EV_HOVER);
			}
		}else{
			if(!(pm->ps->states & isDashing)){
				pm->ps->states |= isDashing;
				//PM_AddEvent(EV_HOVER_FAST);
			}
		}
	}
	else{
		pm->ps->states &= ~isDashing;
		pm->ps->states &= ~isHovering;
	}
}
/*===============
BOOST TYPES
===============*/
void PM_StopBoost(void){
	pm->ps->bitFlags &= ~usingBoost;
	pm->ps->powerups[PW_BOOST] = 0;
	pm->ps->timers[tmBoost] = 0;
}
void PM_StopDash(void){
	VectorClear(pm->ps->dashDir);
}
void PM_StopMovementTypes(void){
	PM_StopDash();
	PM_StopJump();
	PM_StopSoar();
	PM_StopZanzoken();
	PM_StopKnockback();
	//PM_EndDrift();
}
void PM_StopMovement(void){
	PM_StopMovementTypes();
	VectorClear(pm->ps->velocity);
}
void PM_CheckBoost(void){
	int limit;
	limit = pm->ps->powerLevel[plLimit];
	if(!(pm->ps->states & canBoost && !(pm->ps->bitFlags & isStruggling))){return;}
	if(!(pm->cmd.buttons & BUTTON_BOOST)){
		PM_StopBoost();
	}
	else if(pm->ps->powerups[PW_BOOST]){
		pm->ps->eFlags |= EF_AURA;
		pm->ps->powerups[PW_BOOST] += pml.msec;
		pm->ps->timers[tmBoost] += pml.msec;
		if(pm->ps->timers[tmBoost] > limit){pm->ps->timers[tmBoost] = limit;}
		if(pm->ps->powerups[PW_BOOST] > 150){
			pm->ps->powerLevel[plUseFatigue] += (pm->ps->powerLevel[plMaximum] * 0.003) * pm->ps->baseStats[stBoostCost];
			pm->ps->powerups[PW_BOOST] -= 150;
		}
	}
	else{
		pm->ps->timers[tmBoost] = 0;
		pm->ps->powerups[PW_BOOST] = 1;
		if(!(pm->ps->bitFlags & usingBoost)){PM_AddEvent(EV_BOOST_START);}
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
	/*
	if(pm->ps->timers[tmKnockback] > 0){
		if(!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK)){
			drop = speed*20*pml.frametime;
		} else {
			drop = speed*pml.frametime;
		}
	}*/
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
		pm->ps->bitFlags |= underWater;
		waterScale = 1.0f - (1.0f - pm_swimScale)* (float)pm->waterlevel / 3.0f;
		wishspeed = wishspeed * waterScale;
	}
	else{
		pm->ps->bitFlags &= ~underWater;
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
	if(pm->ps->powerups[PW_DRIFTING] > 0){
		totalSpeed = 500;
		if(pm->ps->lockedPlayer->timers[tmMeleeIdle] > 1500 && pm->ps->timers[tmMeleeIdle] > 1500){totalSpeed = 1500;}
	}
	if(pm->ps->timers[tmMeleeIdle] < 0){totalSpeed = 4000;}
	scale = (float)totalSpeed * max / (127.0 * total );
	return scale;
}
/*=============
PM_CheckJump
=============*/
void PM_NotOnGround(void){
	pml.groundPlane = qfalse;
	pml.onGround = qfalse;
	pm->ps->bitFlags &= ~atopGround;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;
}
void PM_StopJump(void){
	if(pm->ps->bitFlags & usingJump){
		pm->ps->bitFlags &= ~usingJump;
		VectorScale(pm->ps->velocity,0.01,pm->ps->velocity);
	}
	if(pm->ps->bitFlags & usingBallFlip){
		pm->ps->bitFlags &= ~usingBallFlip;
		PM_AddEvent(EV_STOPLOOPINGSOUND);
	}
}
void PM_CheckJump(void){
	int jumpPower;
	float jumpScale,jumpEmphasis;
	vec3_t pre_vel,post_vel;
	if(!(pm->ps->states & canJump)){return;}
	if(pm->ps->bitFlags & usingBallFlip){return;}
	if(pm->ps->bitFlags & usingJump){
		if(pm->ps->bitFlags & nearGround && pm->ps->velocity[2] < 0){
			PM_ContinueLegsAnim(ANIM_FLY_DOWN);
		}
		else if(pm->ps->velocity[2] < 200){
			if(pm->ps->pm_flags & PMF_BACKWARDS_JUMP){PM_ForceLegsAnim(ANIM_LAND_BACK);}
			else if(pm->ps->pm_flags & PMF_FORWARDS_JUMP){PM_ForceLegsAnim(ANIM_LAND_UP);}
			else{PM_ForceLegsAnim(ANIM_LAND_UP);}
		}
		return;
	}	
	if(pm->ps->bitFlags & usingAlter || pm->ps->bitFlags & usingFlight
		|| pm->ps->bitFlags & isStruggling || (!(pm->ps->bitFlags & atopGround && !(pm->waterlevel)))){return;}
	if(!(pm->cmd.buttons & BUTTON_JUMP) || pm->cmd.upmove < 0 || pm->ps->weaponstate == WEAPON_GUIDING){return;}
	PM_NotOnGround();
	PM_StopDash();
	pm->ps->bitFlags |= usingJump;
	jumpPower = pm->ps->stats[stSpeed];
	jumpEmphasis = 2500.0;
	pm->ps->gravity[2] = 12000;
	pm->ps->velocity[2] = 0;
	VectorNormalize(pm->ps->velocity);
	VectorCopy(pm->ps->velocity,pre_vel);
	VectorScale(pm->ps->velocity,2500.0,pm->ps->velocity);
	VectorNormalize2(pm->ps->velocity,post_vel);
	if((pm->cmd.forwardmove != 0) || (pm->cmd.rightmove != 0)){
		pm->ps->velocity[2] = jumpPower * 6;
		pm->ps->powerLevel[plUseFatigue] += pm->ps->powerLevel[plMaximum] * 0.01;
		PM_AddEvent(EV_HIGHJUMP);
	} else{
		pm->ps->gravity[2] = 6000;
		pm->ps->velocity[2] = jumpPower * 8;
		pm->ps->powerLevel[plUseFatigue] += pm->ps->powerLevel[plMaximum] * 0.06;
		PM_AddEvent(EV_JUMP);
	}
	if(pm->cmd.forwardmove > 0){
		PM_ForceLegsAnim(ANIM_JUMP_UP);
		pm->ps->pm_flags |= PMF_FORWARDS_JUMP;
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}else if(pm->cmd.forwardmove < 0 || pm->ps->pm_flags & PMF_BACKWARDS_JUMP){
		PM_ForceLegsAnim(ANIM_JUMP_BACK);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		pm->ps->pm_flags &= ~PMF_FORWARDS_JUMP;
	}
	else{
		PM_ForceLegsAnim(ANIM_JUMP_UP);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		pm->ps->pm_flags &= ~PMF_FORWARDS_JUMP;
	}
}

/*==================
PM_CheckStruggling
==================*/
void PM_CheckStruggling(void){
	if(pm->ps->bitFlags & isStruggling){
		PM_StopMovement();
		pm->ps->timers[tmFreeze] = 100;
		if(pm->ps->bitFlags & usingBoost){
			pm->ps->powerLevel[plUseFatigue] += pm->ps->powerLevel[plMaximum] * 0.002;
		}else{
			pm->ps->powerLevel[plUseFatigue] += pm->ps->powerLevel[plMaximum] * 0.001;
		}
	}
}

/*=============
PM_CheckBlock
=============*/
void PM_CheckBlock(void){
	pm->ps->bitFlags &= ~usingBlock;
	if((pm->cmd.buttons & BUTTON_BLOCK)
		&& !(pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING)
		&& !(pm->ps->bitFlags & usingAlter)
		&& !(pm->ps->bitFlags & usingSoar)
		&& !(pm->ps->bitFlags & usingZanzoken)
		&& !(pm->ps->bitFlags & usingWeapon)
		&& !(pm->ps->bitFlags & isStruggling)
		&& !(pm->ps->bitFlags & isGuiding)
		&& !(pm->ps->bitFlags & isPreparing)){
		pm->ps->bitFlags |= usingBlock;
	}
}
/*===================
PM_FlyMove
===================*/
void PM_StopSoar(void){
	pm->ps->bitFlags &= ~isPreparing;
	pm->ps->bitFlags &= ~usingSoar;
	pm->ps->bitFlags &= ~lockedRoll;
	pm->ps->bitFlags &= ~lockedPitch;
	pm->ps->bitFlags &= ~lockedYaw;
	pm->ps->bitFlags &= ~locked360;
}
void PM_StopFlight(void){
	pm->ps->bitFlags &= ~usingFlight;
}
void PM_FlyMove(void){
	int		i;
	vec3_t	wishvel;
	vec3_t	wishdir;
	float	soarCost;
	float	wishspeed;
	float	fadeSpeed;
	float	scale;
	float	boostFactor;
	if(!(pm->ps->states & canFly)){return;}
	if(pm->cmd.upmove > 0){
		pm->ps->bitFlags |= usingFlight;
		PM_StopDash();
		PM_StopJump();
	}
	if(pm->cmd.buttons & BUTTON_POWERLEVEL && !VectorLength(pm->ps->velocity)){return;}
	if(!(pm->ps->bitFlags & usingFlight) ||(pm->cmd.buttons & BUTTON_POWERLEVEL && !VectorLength(pm->ps->velocity))){return;}
	boostFactor = pm->ps->bitFlags & usingBoost ? 4.4 : 1.1;
	if(pm->cmd.buttons & BUTTON_WALKING){
		boostFactor = 1.0;
	}
	if((pm->cmd.buttons & BUTTON_JUMP &&
		!(pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING) && 
		!(pm->ps->weaponstate == WEAPON_GUIDING || pm->ps->weaponstate == WEAPON_ALTGUIDING) && 
		!(pm->ps->bitFlags & usingZanzoken)) ||
		(pm->ps->bitFlags & lockedPitch || pm->ps->bitFlags & lockedYaw || pm->ps->bitFlags & lockedRoll)){
		if(pm->ps->bitFlags & usingJump){return;}
		//if(PM_CheckDirection(pml.forward,qfalse)){PM_Crash(qtrue);}
		pm->ps->timers[tmSoar] += pml.msec;
		pm->ps->eFlags |= EF_AURA;
		if(pm->cmd.buttons & BUTTON_POWERLEVEL){pm->cmd.forwardmove = 0;}
		if(!(pm->ps->bitFlags & usingSoar)){
			pm->cmd.upmove = pm->cmd.rightmove = pm->cmd.forwardmove = 0;
			pm->ps->bitFlags |= isPreparing;
			pm->ps->soarBase[YAW] = pm->ps->soarLimit[YAW] = pm->ps->viewangles[YAW];
			pm->ps->soarBase[ROLL] = pm->ps->soarLimit[ROLL] = 0.0;
			pm->ps->soarBase[PITCH] = pm->ps->soarLimit[PITCH] = 0.0;
			pm->ps->viewangles[PITCH] = 0.0;
			if(pm->ps->timers[tmSoar] < 700){return;}
			pm->ps->bitFlags |= usingSoar;
			pm->ps->bitFlags &= ~isPreparing;
			pm->ps->timers[tmSoar] = 0;
		}
		soarCost = 0;
		if(pm->ps->powerLevel[plMaximum] * 0.8 > pm->ps->powerLevel[plFatigue]){
			soarCost = pm->ps->powerLevel[plMaximum] * 0.001;
		}
		boostFactor = 5.8;
		pm->ps->viewangles[PITCH] = pm->ps->soarBase[PITCH];
		pm->ps->viewangles[YAW] = pm->ps->soarBase[YAW];
		pm->ps->viewangles[ROLL] = pm->ps->soarBase[ROLL];
		if(pm->cmd.upmove != 0){
			if(pm->cmd.upmove < 0){pm->ps->soarLimit[PITCH] = 360.0;}
			else if(pm->cmd.upmove > 0){pm->ps->soarLimit[PITCH] = -360.0;}
			pm->ps->bitFlags |= locked360;
		}
		if(pm->cmd.forwardmove != 0){
			if(pm->cmd.forwardmove < 0){pm->ps->soarLimit[PITCH] += 2.0;}
			else if(pm->cmd.forwardmove > 0){pm->ps->soarLimit[PITCH] += -2.0;}
			pm->ps->bitFlags |= lockedPitch;
		}
		if(pm->cmd.rightmove != 0){
			if(pm->cmd.rightmove < 0){
				if(!(pm->ps->bitFlags & lockedRoll) && !(pm->ps->bitFlags & lockedSpin) && pm->ps->soarLimit[ROLL] > -20){
					pm->ps->soarLimit[ROLL] -= 2;
				}
				pm->ps->soarLimit[YAW] += 2.0;
			}
			else if(pm->cmd.rightmove > 0){
				if(!(pm->ps->bitFlags & lockedRoll) && !(pm->ps->bitFlags & lockedSpin) && pm->ps->soarLimit[ROLL] < 20){
					pm->ps->soarLimit[ROLL] += 2;
				}
				pm->ps->soarLimit[YAW] -= 2.0;
			}
			pm->ps->bitFlags |= lockedYaw;
			pm->ps->bitFlags |= lockedRoll;
		}
		if(pm->cmd.buttons & BUTTON_BOOST){
			if(soarCost){soarCost = pm->ps->powerLevel[plMaximum] * 0.002;}
			boostFactor *= 1.5;
		}
		if(pm->cmd.buttons & BUTTON_ROLL_RIGHT || pm->cmd.buttons & BUTTON_ROLL_LEFT){
			if(pm->cmd.buttons & BUTTON_ROLL_RIGHT){pm->ps->soarLimit[ROLL] = 360;}
			else if(pm->cmd.buttons & BUTTON_ROLL_LEFT){pm->ps->soarLimit[ROLL] = -360;}
			pm->ps->bitFlags |= lockedSpin;
		}
		fadeSpeed = 2.0;
		while(pm->ps->timers[tmSoar] > 10){
			if(pm->ps->bitFlags & lockedPitch || pm->ps->bitFlags & locked360){
				if(pm->ps->bitFlags & locked360){fadeSpeed = 4;}
				if(pm->ps->soarBase[PITCH] > 180){
					pm->ps->soarBase[PITCH] = pm->ps->bitFlags & locked360 ? -184 : -182;
					pm->ps->soarLimit[PITCH] = 0;
				}
				else if(pm->ps->soarBase[PITCH] < -180){
					pm->ps->soarBase[PITCH] = pm->ps->bitFlags & locked360 ? 184 : 182;
					pm->ps->soarLimit[PITCH] = 0;
				}
				if(pm->ps->soarBase[PITCH] > pm->ps->soarLimit[PITCH]){pm->ps->soarBase[PITCH] -= fadeSpeed;}
				else if(pm->ps->soarBase[PITCH] < pm->ps->soarLimit[PITCH]){pm->ps->soarBase[PITCH] += fadeSpeed;}
				else{
					pm->ps->bitFlags &= ~lockedPitch;
					pm->ps->bitFlags &= ~locked360;
				}
			}
			if(pm->ps->bitFlags & lockedYaw){
				if(pm->ps->soarBase[YAW] > pm->ps->soarLimit[YAW]){pm->ps->soarBase[YAW] -= fadeSpeed;}
				else if(pm->ps->soarBase[YAW] < pm->ps->soarLimit[YAW]){pm->ps->soarBase[YAW] += fadeSpeed;}
				else{pm->ps->bitFlags &= ~lockedYaw;}
			}
			if(pm->ps->bitFlags & lockedRoll || pm->ps->bitFlags & lockedSpin){
				boostFactor *= 1.2;
				if(pm->ps->bitFlags & lockedSpin){fadeSpeed = 4;}
				if(pm->ps->soarBase[ROLL] > 180){
					pm->ps->soarBase[ROLL] = pm->ps->bitFlags & lockedSpin ? -184 : -182;
					pm->ps->soarLimit[ROLL] = 0;
				}
				else if(pm->ps->soarBase[ROLL] < -180){
					pm->ps->soarBase[ROLL] = pm->ps->bitFlags & lockedSpin ? 184 : 182;
					pm->ps->soarLimit[ROLL] = 0;
				}
				if(pm->ps->soarBase[ROLL] > pm->ps->soarLimit[ROLL]){pm->ps->soarBase[ROLL] -= fadeSpeed;}
				else if(pm->ps->soarBase[ROLL] < pm->ps->soarLimit[ROLL]){pm->ps->soarBase[ROLL] += fadeSpeed;}
				else{
					pm->ps->bitFlags &= ~lockedRoll;
					pm->ps->bitFlags &= ~lockedSpin;
				}
			}
			pm->ps->timers[tmSoar] -= 10;
			pm->ps->powerLevel[plUseFatigue] +=	soarCost;
		}
		pm->cmd.upmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.forwardmove = 127;
	}
	else if(pm->ps->bitFlags & usingSoar || pm->ps->bitFlags & isPreparing){
		pm->ps->timers[tmFreeze] = 1000;
		pm->ps->bitFlags &= ~usingSoar;
		pm->ps->bitFlags &= ~isPreparing;
		pm->ps->eFlags &= ~EF_AURA;
	}
	else{
		/*
		pm->ps->viewangles[ROLL] = pm->ps->soarBase[ROLL];
		if(pm->cmd.buttons & BUTTON_ROLL_LEFT){pm->ps->soarBase[ROLL] -= 3;}
		if(pm->cmd.buttons & BUTTON_ROLL_RIGHT){pm->ps->soarBase[ROLL] += 3;}
		*/
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
	int			i,backupDrift;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	qboolean	gravity;
	float		wishspeed;
	float		scale;
	usercmd_t	cmd;
	backupDrift = pm->cmd.forwardmove;
	if((!pm->ps->powerups[PW_DRIFTING] && pm->ps->timers[tmMeleeIdle] >= 0) && (pml.onGround
		|| (pm->cmd.buttons & BUTTON_POWERLEVEL && !VectorLength(pm->ps->velocity))
		&& !(pm->ps->bitFlags & isUnconcious) && !(pm->ps->bitFlags & isDead) && !(pm->ps->bitFlags & isCrashed))){return;}
	if(pm->ps->bitFlags & isGuiding){return;}
	if(!(pm->ps->bitFlags & usingFlight)){
		if((pm->ps->states & canBallFlip)){
			if(pm->ps->bitFlags & usingJump && pm->cmd.upmove < 0){
				pm->ps->gravity[2] = 3000;
				pm->ps->bitFlags |= usingBallFlip;
				PM_AddEvent(EV_BALLFLIP);
				if(pm->ps->bitFlags & nearGround && pm->ps->velocity[2] < 0){PM_ContinueLegsAnim(ANIM_FLY_DOWN);}
				else{PM_ContinueLegsAnim(ANIM_JUMP_FORWARD);}
			}
			else if(pm->ps->bitFlags & usingJump && pm->ps->bitFlags & usingBallFlip && pm->cmd.upmove == 0){
				pm->ps->gravity[2] = 6000;
				pm->ps->bitFlags &= ~usingBallFlip;
				PM_AddEvent(EV_STOPLOOPINGSOUND);
			}
			if(pm->ps->bitFlags & usingBallFlip){PM_StopDirections();}
		}
	}
	if(pm->ps->timers[tmMeleeIdle] < 0){
		PM_StopDirections();
		pm->cmd.forwardmove = -127;
		pm->ps->timers[tmMeleeIdle] += pml.msec;
		if(pm->ps->timers[tmMeleeIdle] > 0){pm->ps->timers[tmMeleeIdle] = 0;}
	}
	if(pm->ps->bitFlags & usingMelee){
		PM_StopDirections();
		if(pm->ps->powerups[PW_DRIFTING] == 1){pm->cmd.forwardmove = 127;}
		else if(pm->ps->powerups[PW_DRIFTING] == 2){pm->cmd.forwardmove = -127;}
	}
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;
	cmd = pm->cmd;
	scale = PM_CmdScale(&cmd);
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);
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
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity[2] * pml.frametime;
	}
	gravity = pm->ps->bitFlags & usingFlight ? qfalse : qtrue;
	if(pm->ps->bitFlags & usingZanzoken){
		if(pm->ps->bitFlags & usingJump){gravity = qtrue;}
		pm->tracemask = CONTENTS_SOLID;
	}
	PM_StepSlideMove(gravity); 
	if(pm->ps->bitFlags & usingZanzoken){pm->tracemask = MASK_PLAYERSOLID;}
	if(pm->ps->bitFlags & usingMelee){
		PM_StopDirections();
	}
	pm->cmd.forwardmove = backupDrift;
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
	if(pm->ps->bitFlags & usingFlight){return;}
	if(pm->cmd.buttons & BUTTON_POWERLEVEL && !VectorLength(pm->ps->velocity)){return;}
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
		pm->ps->velocity[2] += 0.7f * pm->ps->gravity[2] * pml.frametime;
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
	if(pm->cmd.buttons & BUTTON_POWERLEVEL && !VectorLength(pm->ps->velocity)){return;}
	if((pml.onGround && !(pm->cmd.buttons & BUTTON_WALKING) && (pm->cmd.forwardmove || pm->cmd.rightmove))){
		pm->cmd.upmove = 0;
		boostFactor = pm->ps->bitFlags & usingBoost ? 4.4 : 1.1;
		if(dashAmount == 0){
			pm->ps->dashDir[0] = 0;
			pm->ps->dashDir[1] = 0;
			if(pm->cmd.forwardmove > 0){
				pm->ps->dashDir[0] = 1;
				pm->cmd.forwardmove = 127;
			}else if(pm->cmd.forwardmove < 0){
				pm->ps->dashDir[0] = -1;
				pm->cmd.forwardmove = -127;
			}
			if(pm->cmd.rightmove > 0){
				pm->ps->dashDir[1] = 1;
				pm->cmd.rightmove = 127;
			}else if(pm->cmd.rightmove < 0){
				pm->ps->dashDir[1] = -1;
				pm->cmd.rightmove = -127;
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
			pm->ps->velocity[2] += 0.7f * pm->ps->gravity[2] * pml.frametime;
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
	pm->ps->legsTimer = TIMER_LAND;
	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity[2];

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
int PM_CheckDirection(vec3_t direction,qboolean player){
	vec3_t	point;
	trace_t	trace;
	int flags = pm->tracemask &~ CONTENTS_BODY;
	VectorMA(pm->ps->origin,64,direction,point);
	if(player){flags = CONTENTS_BODY;}
	pm->trace(&trace,pm->ps->origin,pm->mins,pm->maxs,point,pm->ps->clientNum,flags);
	if(trace.allsolid && (!PM_CorrectAllSolid(&trace))){return 0;}
	if(trace.fraction == 1.0){
		return 0;
	}
	if(player){return trace.entityNum+1;}
	return 1;
}
void PM_GroundTraceMissed(void){
	trace_t		trace;
	vec3_t		point;
	if(pm->ps->groundEntityNum != ENTITYNUM_NONE){
		VectorCopy(pm->ps->origin, point );
		point[2] -= 64;
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask);
		if(trace.fraction == 1.0){
			PM_StopDash();
		}
	}
	PM_NotOnGround();
}
void PM_NotNearGround(void){
	pm->ps->bitFlags &= ~nearGround;
}
void PM_NearGroundTrace(void){
	vec3_t		point;
	trace_t		trace;
	if(pm->ps->bitFlags & atopGround){
		if(pm->ps->timers[tmFall] >= 4000){PM_AddEvent(EV_FALL_FAR);}
		else if(pm->ps->timers[tmFall] >= 2000){PM_AddEvent(EV_FALL_MEDIUM);}
		else if(pm->ps->timers[tmFall] >= 500){PM_AddEvent(EV_FALL_SHORT);}
		pm->ps->timers[tmFall] = 0;
		return;
	}
	pm->ps->timers[tmFall] += pml.msec;
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 450.0f;
	pm->trace(&trace,pm->ps->origin,pm->mins,pm->maxs,point,pm->ps->clientNum,pm->tracemask);
	if(trace.allsolid && (!PM_CorrectAllSolid(&trace))){return;}
	if(trace.fraction == 1.0){
		PM_NotNearGround();
		return;
	}
	pm->ps->bitFlags |= nearGround;
}
/*=============
PM_GroundTrace
=============*/
void PM_GroundTrace(void){
	vec3_t		point;
	trace_t		trace;
	if(pm->ps->bitFlags & usingSoar){return;}
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
		pm->ps->bitFlags &= ~atopGround;
		return;
	}
	if(pm->ps->velocity[2] > 0 && DotProduct(pm->ps->velocity, trace.plane.normal)> 10){
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
	pm->ps->bitFlags |= atopGround;
	if(pm->ps->groundEntityNum == ENTITYNUM_NONE && (pm->ps->bitFlags & usingJump || pm->ps->bitFlags & usingFlight
		|| pm->ps->bitFlags & usingBallFlip || pm->ps->bitFlags & isUnconcious || pm->ps->bitFlags & isDead)){
		PM_StopFlight();
		PM_Land();
		PM_StopJump();
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
	if(pm->ps->bitFlags & usingZanzoken || pm->ps->bitFlags & usingJump || pm->ps->bitFlags & usingBallFlip){return;}
	if(pm->ps->bitFlags & keyTierUp || pm->ps->bitFlags & keyTierDown && VectorLength(pm->ps->velocity) <= 5){return;}
	if((pm->ps->bitFlags & usingAlter) && (VectorLength(pm->ps->velocity) <= 5)){
		if(pm->cmd.rightmove < 0){
			PM_ContinueLegsAnim(ANIM_IDLE);
			return;
		}
		if(pm->ps->powerLevel[plCurrent] > 31000){PM_ContinueLegsAnim(ANIM_KI_CHARGE);}
		else{pm->ps->powerLevel[plCurrent] > pm->ps->powerLevel[plFatigue] ? PM_ContinueLegsAnim(ANIM_KI_CHARGE) : PM_ContinueLegsAnim(ANIM_PL_UP);}
		return;
	}
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] +  pm->ps->velocity[1] * pm->ps->velocity[1]);
	if(pm->waterlevel > 2 && !(pm->ps->bitFlags & usingFlight)){
		if(pm->cmd.forwardmove > 0){
			PM_ContinueLegsAnim(ANIM_SWIM);
			return;
		}
		else if(!pm->cmd.forwardmove && !pm->cmd.rightmove && !pm->cmd.upmove){
			PM_ContinueLegsAnim(ANIM_SWIM_IDLE);
			return;
		}
	}
	if(pm->ps->bitFlags & usingFlight){
		if(pm->ps->bitFlags & usingSoar || pm->ps->bitFlags & isPreparing){
			pm->ps->bitFlags & usingSoar ? PM_ContinueLegsAnim(ANIM_FLY_FORWARD) : PM_ContinueLegsAnim(ANIM_FLY_START);
			return;
		}
		if((!pm->cmd.forwardmove && !pm->cmd.rightmove) || (pm->cmd.buttons & BUTTON_WALKING )) {
			int	tempAnimIndex;
			pm->ps->bobCycle = 0;
			tempAnimIndex = pm->ps->torsoAnim & ~ANIM_TOGGLEBIT;
			if((tempAnimIndex >= ANIM_KI_ATTACK1_PREPARE) && (tempAnimIndex <= ANIM_KI_ATTACK6_ALT_FIRE)) {
				tempAnimIndex = tempAnimIndex - ANIM_KI_ATTACK1_PREPARE;
				tempAnimIndex = ANIM_KI_ATTACK1_PREPARE + tempAnimIndex;
				PM_ContinueLegsAnim(tempAnimIndex);
			} else{
				if(pm->cmd.upmove > 0){PM_ContinueLegsAnim(ANIM_FLY_UP);} 
				else if(pm->cmd.upmove < 0){PM_ContinueLegsAnim(ANIM_FLY_DOWN);}
				else if(pm->ps->lockedTarget>0){PM_ContinueLegsAnim(ANIM_IDLE_LOCKED);}
				else{PM_ContinueLegsAnim(ANIM_FLY_IDLE);}
			}
			return;
		}
		if(pm->cmd.forwardmove > 0){
			PM_ContinueLegsAnim(ANIM_DASH_FORWARD);
		} else if(pm->cmd.forwardmove < 0){
			PM_ContinueLegsAnim(ANIM_DASH_BACKWARD);
		} else if(pm->cmd.rightmove > 0){
			PM_ContinueLegsAnim(ANIM_DASH_RIGHT);
		} else if(pm->cmd.rightmove < 0){
			PM_ContinueLegsAnim(ANIM_DASH_LEFT);
		}
		return;
	}
	if(!(pm->ps->bitFlags & atopGround)){
		if(pm->ps->timers[tmFall] > 750){
			PM_ContinueLegsAnim(ANIM_FLY_DOWN);
		}
		return;
	}
	if(!pm->cmd.forwardmove && !pm->cmd.rightmove){
		if(pm->xyspeed < 5){
			int	tempAnimIndex;
			pm->ps->bobCycle = 0;
			tempAnimIndex = pm->ps->torsoAnim & ~ANIM_TOGGLEBIT;
			if((tempAnimIndex >= ANIM_KI_ATTACK1_PREPARE) && (tempAnimIndex <= ANIM_KI_ATTACK6_ALT_FIRE)) {
				tempAnimIndex = tempAnimIndex - ANIM_KI_ATTACK1_PREPARE;
				tempAnimIndex = ANIM_KI_ATTACK1_PREPARE + tempAnimIndex;
				PM_ContinueLegsAnim(tempAnimIndex);
			}
			else if(pm->ps->bitFlags & atopGround){
				if(pm->ps->lockedTarget>0){PM_ContinueLegsAnim(ANIM_IDLE_LOCKED);}
				else{PM_ContinueLegsAnim(ANIM_IDLE);}
			}
		}
		return;
	}
	footstep = qfalse;
	if(!(pm->cmd.buttons & BUTTON_WALKING)){
		bobmove = 0.4f;
		if(!pm->ps->running){
			if(pm->cmd.forwardmove < 0) {
				PM_ContinueLegsAnim(ANIM_DASH_BACKWARD);
			} else if(pm->cmd.forwardmove > 0){
				PM_ContinueLegsAnim(ANIM_DASH_FORWARD);
			} else if(pm->cmd.rightmove > 0){
				PM_ContinueLegsAnim(ANIM_DASH_RIGHT);
			} else if(pm->cmd.rightmove < 0){
				PM_ContinueLegsAnim(ANIM_DASH_LEFT);
			}
		}else{
			if(pm->cmd.forwardmove < 0){
				PM_ContinueLegsAnim(ANIM_BACKRUN);
			} else if(pm->cmd.forwardmove > 0){
				PM_ContinueLegsAnim(ANIM_RUN);
			} else if(pm->cmd.rightmove > 0){
				PM_ContinueLegsAnim(ANIM_DASH_RIGHT );
			} else if(pm->cmd.rightmove < 0){
				PM_ContinueLegsAnim(ANIM_DASH_LEFT);
			}
		}
		footstep = qtrue;
	}
	else{
		bobmove = 0.3f;
		if(pm->cmd.forwardmove < 0){
			PM_ContinueLegsAnim(ANIM_BACKWALK );
		}
		else{
			PM_ContinueLegsAnim(ANIM_WALK);
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
/*
	//
	// ifjust entered a water volume at a high velocity, play a sound and effect
	//
	if(!pml.previous_waterlevel && pm->waterlevel && pm->xyspeed > 200) {
		PM_AddEvent(EV_WATER_SPLASH );
	}
*/
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
	qboolean usable;
	charging = (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING) ? qtrue : qfalse;
	if(pm->ps->weapon == pm->cmd.weapon){
		return;
	}
	if(weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS){
		return;
	}
	if(pm->ps->stats[stMeleeState] || pm->ps->timers[tmTransform] || charging){
		return;
	}
	if(!(pm->ps->stats[stSkills] & (1 << weapon))){
		return;
	}

	if(pm->ps->weaponstate == WEAPON_DROPPING){
		return;
	}
	PM_AddEvent(EV_CHANGE_WEAPON);
	usable = qfalse;
	weapon = pm->cmd.weapon;
	while(!usable){
		if(weapon == 1 && pm->ps->powerups[PW_SKILLS] & USABLE_SKILL1){usable = qtrue;}
		if(weapon == 2 && pm->ps->powerups[PW_SKILLS] & USABLE_SKILL2){usable = qtrue;}
		if(weapon == 3 && pm->ps->powerups[PW_SKILLS] & USABLE_SKILL3){usable = qtrue;}
		if(weapon == 4 && pm->ps->powerups[PW_SKILLS] & USABLE_SKILL4){usable = qtrue;}
		if(weapon == 5 && pm->ps->powerups[PW_SKILLS] & USABLE_SKILL5){usable = qtrue;}
		if(weapon == 6 && pm->ps->powerups[PW_SKILLS] & USABLE_SKILL6){usable = qtrue;}
		if(!usable){
			if(pm->cmd.weapon < pm->ps->weapon){weapon -=1;}
			else if(pm->cmd.weapon > pm->ps->weapon){weapon +=1;}
			else{break;}
			if(weapon > 6){weapon = 1;}
			if(weapon < 1){weapon = 6;}
		}
	}
	pm->ps->weapon = weapon;
	pm->ps->timers[tmAttack1] = 0;
	pm->ps->timers[tmAttack2] = 0;
}

/*==============
PM_TorsoAnimation
==============*/
void PM_TorsoAnimation(void){
	if(pm->ps->weaponstate != WEAPON_READY){return;}
	if(pm->ps->timers[tmImpede] || pm->ps->states & isBurning){PM_ContinueLegsAnim(ANIM_STUNNED);}
	if(pm->ps->states & isRiding){PM_ContinueLegsAnim(ANIM_KNOCKBACK);}
	if(pm->ps->bitFlags & usingBlock && !(pm->ps->bitFlags & usingMelee)){
		if(pm->ps->bitFlags & isStruggling){
			PM_ContinueTorsoAnim(ANIM_PUSH);
			PM_ContinueLegsAnim(ANIM_PUSH);
			return;
		}else{
			PM_ContinueTorsoAnim(ANIM_BLOCK);
			if(!pm->cmd.forwardmove && !pm->cmd.rightmove){
				PM_ContinueLegsAnim(ANIM_BLOCK);
			}
			return;
		}
	}
	switch(pm->ps->legsAnim & ~ANIM_TOGGLEBIT){
	case ANIM_DEATH_AIR_LAND:
		PM_ContinueTorsoAnim(ANIM_DEATH_AIR_LAND);	
		break;
	case ANIM_KNOCKBACK:
		PM_ContinueTorsoAnim(ANIM_KNOCKBACK);
		break;
	case ANIM_KNOCKBACK_HIT_WALL:
		PM_ContinueTorsoAnim(ANIM_KNOCKBACK_HIT_WALL);
		break;
	case ANIM_KNOCKBACK_RECOVER_1:
		PM_ContinueTorsoAnim(ANIM_KNOCKBACK_RECOVER_1);
		break;
	case ANIM_KNOCKBACK_RECOVER_2:
		PM_ContinueTorsoAnim(ANIM_KNOCKBACK_RECOVER_2);
		break;
	case ANIM_STUNNED_MELEE:
		PM_ContinueTorsoAnim(ANIM_STUNNED_MELEE);
		break;
	case ANIM_POWER_MELEE_1_CHARGE:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_1_CHARGE);
		break;
	case ANIM_POWER_MELEE_2_CHARGE:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_2_CHARGE);
		break;
	case ANIM_POWER_MELEE_3_CHARGE:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_3_CHARGE);
		break;
	case ANIM_POWER_MELEE_4_CHARGE:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_4_CHARGE);
		break;
	case ANIM_POWER_MELEE_5_CHARGE:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_5_CHARGE);
		break;
	case ANIM_POWER_MELEE_6_CHARGE:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_6_CHARGE);
		break;
	case ANIM_POWER_MELEE_1_HIT:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_1_HIT);
		break;
	case ANIM_POWER_MELEE_2_HIT:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_2_HIT);
		break;
	case ANIM_POWER_MELEE_3_HIT:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_3_HIT);
		break;
	case ANIM_POWER_MELEE_4_HIT:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_4_HIT);
		break;
	case ANIM_POWER_MELEE_5_HIT:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_5_HIT);
		break;
	case ANIM_POWER_MELEE_6_HIT:
		PM_ContinueTorsoAnim(ANIM_POWER_MELEE_6_HIT);
		break;
	case ANIM_BREAKER_MELEE_HIT1:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_HIT1);
		break;
	case ANIM_BREAKER_MELEE_HIT2:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_HIT2);
		break;
	case ANIM_BREAKER_MELEE_HIT3:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_HIT3);
		break;
	case ANIM_BREAKER_MELEE_HIT4:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_HIT4);
		break;
	case ANIM_BREAKER_MELEE_HIT5:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_HIT5);
		break;
	case ANIM_BREAKER_MELEE_HIT6:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_HIT6);
		break;
	case ANIM_BREAKER_MELEE_ATTACK1:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_ATTACK1);
		break;
	case ANIM_BREAKER_MELEE_ATTACK2:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_ATTACK2);
		break;
	case ANIM_BREAKER_MELEE_ATTACK3:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_ATTACK3);
		break;
	case ANIM_BREAKER_MELEE_ATTACK4:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_ATTACK4);
		break;
	case ANIM_BREAKER_MELEE_ATTACK5:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_ATTACK5);
		break;
	case ANIM_BREAKER_MELEE_ATTACK6:
		PM_ContinueTorsoAnim(ANIM_BREAKER_MELEE_ATTACK6);
		break;
	case ANIM_SPEED_MELEE_DODGE:
		PM_ContinueTorsoAnim(ANIM_SPEED_MELEE_DODGE);
		break;
	case ANIM_SPEED_MELEE_ATTACK:
		PM_ContinueTorsoAnim(ANIM_SPEED_MELEE_ATTACK);
		break;
	case ANIM_SPEED_MELEE_BLOCK:
		PM_ContinueTorsoAnim(ANIM_SPEED_MELEE_BLOCK);
		break;
	case ANIM_SPEED_MELEE_HIT:
		PM_ContinueTorsoAnim(ANIM_SPEED_MELEE_HIT);
		break;
	case ANIM_RUN:
		PM_ContinueTorsoAnim(ANIM_RUN);
		break;
	case ANIM_BACKRUN:
		PM_ContinueTorsoAnim(ANIM_BACKRUN);
		break;
	case ANIM_DASH_FORWARD:
		PM_ContinueTorsoAnim(ANIM_DASH_FORWARD );
		break;
	case ANIM_DASH_BACKWARD:
		PM_ContinueTorsoAnim(ANIM_DASH_BACKWARD );
		break;
	case ANIM_DASH_RIGHT:
		PM_ContinueTorsoAnim(ANIM_DASH_RIGHT );
		break;
	case ANIM_DASH_LEFT:
		PM_ContinueTorsoAnim(ANIM_DASH_LEFT );
		break;
	case ANIM_WALK:
		PM_ContinueTorsoAnim(ANIM_WALK );
		break;
	case ANIM_BACKWALK:
		PM_ContinueTorsoAnim(ANIM_BACKWALK );
		break;
	case ANIM_SWIM_IDLE:
		PM_ContinueTorsoAnim(ANIM_SWIM_IDLE );
		break;
	case ANIM_SWIM:
		PM_ContinueTorsoAnim(ANIM_SWIM );
		break;
	case ANIM_JUMP_UP:
		PM_ContinueTorsoAnim(ANIM_JUMP_UP );
		break;
	case ANIM_JUMP_FORWARD:
		PM_ContinueTorsoAnim(ANIM_JUMP_FORWARD );
		break;
	case ANIM_JUMP_BACK:
		PM_ContinueTorsoAnim(ANIM_JUMP_BACK );
		break;
	case ANIM_LAND_UP:
		PM_ContinueTorsoAnim(ANIM_LAND_UP );
		break;
	case ANIM_LAND_FORWARD:
		PM_ContinueTorsoAnim(ANIM_LAND_FORWARD );
		break;
	case ANIM_LAND_BACK:
		PM_ContinueTorsoAnim(ANIM_LAND_BACK );
		break;
	case ANIM_STUNNED:
		PM_ContinueTorsoAnim(ANIM_STUNNED);
		break;
	case ANIM_PUSH:
		PM_ContinueTorsoAnim(ANIM_PUSH);
		break;
	case ANIM_DEFLECT:
		PM_ContinueTorsoAnim(ANIM_DEFLECT);
		break;
	case ANIM_BLOCK:
		PM_ContinueTorsoAnim(ANIM_BLOCK);
		break;
	case ANIM_KI_CHARGE:
		PM_ContinueTorsoAnim(ANIM_KI_CHARGE );
		break;
	case ANIM_PL_UP:
		PM_ContinueTorsoAnim(ANIM_PL_UP );
		break;
	case ANIM_TRANS_UP:
		PM_ContinueTorsoAnim(ANIM_TRANS_UP);
		break;
	case ANIM_TRANS_BACK:
		PM_ContinueTorsoAnim(ANIM_TRANS_BACK );
		break;
	case ANIM_FLY_IDLE:
		PM_ContinueTorsoAnim(ANIM_FLY_IDLE);
		break;
	case ANIM_FLY_START:
		PM_ContinueTorsoAnim(ANIM_FLY_START);
		break;
	case ANIM_FLY_FORWARD:
		PM_ContinueTorsoAnim(ANIM_FLY_FORWARD );
		break;
	case ANIM_FLY_BACKWARD:
		PM_ContinueTorsoAnim(ANIM_FLY_BACKWARD);
		break;
	case ANIM_FLY_UP:
		PM_ContinueTorsoAnim(ANIM_FLY_UP );
		break;
	case ANIM_FLY_DOWN:
		PM_ContinueTorsoAnim(ANIM_FLY_DOWN);
		break;
	case ANIM_IDLE:
		PM_ContinueTorsoAnim(ANIM_IDLE);
		break;
	case ANIM_FLOOR_RECOVER:
		PM_ContinueTorsoAnim(ANIM_FLOOR_RECOVER);
		break;
	default:
		(pm->ps->lockedTarget>0) ? PM_ContinueTorsoAnim(ANIM_IDLE_LOCKED) : PM_ContinueTorsoAnim(ANIM_IDLE);
		break;
	}
}
/*=============================================*\
    Amazin' Melee (now with 99% more strategy!)
\*=============================================*/
void PM_StopMelee(void){
	if(pm->ps->bitFlags & usingMelee || pm->ps->stats[stMeleeState]){
		pm->ps->stats[stMeleeState] = 0;
		pm->ps->timers[tmMeleeCharge] = 0;
		pm->ps->timers[tmMeleeIdle] = 0;
		pm->ps->timers[tmMeleeBreaker] = 0;
		pm->ps->bitFlags &= ~usingMelee;
		PM_AddEvent(EV_STOPLOOPINGSOUND);
		PM_EndDrift();
		if(pm->ps->lockedPlayer){
			pm->ps->lockedPlayer->stats[stMeleeState] = 0;
			pm->ps->lockedPlayer->timers[tmMeleeCharge] = 0;
			pm->ps->lockedPlayer->timers[tmMeleeIdle] = 0;
			pm->ps->lockedPlayer->timers[tmMeleeBreaker] = 0;
			pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 0;
			pm->ps->lockedPlayer->bitFlags &= ~usingMelee;
		}
	}
}
void PM_CheckDrift(void){
	if(!pm->ps->lockedPlayer->powerups[PW_DRIFTING] && pm->ps->stats[stMeleeState] & stMeleeUsingSpeed){
		pm->ps->powerups[PW_DRIFTING] = 1;
		pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 2;
	}
}
void PM_EndDrift(void){
	pm->ps->powerups[PW_DRIFTING] = 0;
	if(pm->ps->lockedTarget > 0){pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 0;}
}
void PM_StopDrift(void){
	if(pm->ps->powerups[PW_DRIFTING] == 1){PM_EndDrift();}
}
void PM_SyncMelee(void){
	if(pm->ps->lockedPlayer->bitFlags & usingZanzoken || pm->ps->lockedPlayer->bitFlags & usingZanzoken){return;}
	pm->ps->pm_flags |= PMF_ATTACK1_HELD;
	pm->ps->pm_flags |= PMF_ATTACK2_HELD;
	pm->ps->bitFlags |= usingMelee;
	pm->ps->bitFlags |= usingFlight;
	pm->ps->timers[tmUpdateMelee] = 300;
	pm->ps->lockedPlayer->timers[tmUpdateMelee] = 300;
	pm->ps->lockedPlayer->bitFlags |= usingMelee;
	pm->ps->lockedPlayer->bitFlags |= usingFlight;
	pm->ps->lockedPlayer->weaponstate = WEAPON_READY;
	if(pm->ps->lockedPlayer->stats[stMeleeState] == 0){
		pm->ps->lockedPlayer->pm_flags |= PMF_ATTACK1_HELD;
		pm->ps->lockedPlayer->pm_flags |= PMF_ATTACK2_HELD;
		pm->ps->lockedPlayer->lockedTarget = pm->ps->clientNum + 1;
		pm->ps->lockedPlayer->lockedPlayer = 0;
	}
}
void PM_MeleeIdle(void){
	pm->ps->timers[tmMeleeIdle] += pml.msec;
	if(pm->ps->timers[tmMeleeIdle] > 1500 && pm->ps->lockedPlayer->timers[tmMeleeIdle] > 1500){
		pm->ps->powerups[PW_DRIFTING] = 2;
		pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 2;
	}
}
void PM_Melee(void){
	int meleeCharge,enemyState,state,option,damage,distance,animation,direction,entity;
	qboolean charging,movingForward,idleTime,enemyChanged,enemyDefense;
	qtime_t realRandom;
	trap_RealTime(&realRandom);
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR || pm->ps->bitFlags & isStruggling || pm->ps->timers[tmMeleeIdle] < 0){return;}
	charging = (pm->ps->weaponstate == WEAPON_CHARGING || pm->ps->weaponstate == WEAPON_ALTCHARGING) ? qtrue : qfalse;
	state = pm->ps->stats[stMeleeState];
	idleTime = qfalse;
	distance = 9000;
	pm->ps->timers[tmUpdateMelee] += pml.msec;
	// ===================
	// Setup
	// ===================
	if(pm->ps->lockedTarget > 0 && !charging){
		if(pm->ps->timers[tmUpdateMelee] >= 300){
			pm->ps->timers[tmUpdateMelee] = 0;
			PM_AddEvent(EV_MELEE_CHECK);
		}
		if(pm->ps->bitFlags & usingZanzoken){
			PM_StopMelee();
			return;
		}
		if(pm->ps->lockedPlayer){
			if(pm->ps->lockedPlayer->bitFlags & usingZanzoken){
				PM_StopMelee();
				return;
			}
			distance = Distance(pm->ps->origin,*(pm->ps->lockedPosition));
			if(!(pm->ps->bitFlags & usingMelee)){
				state = stMeleeInactive;
				if(pm->cmd.forwardmove > 0){state = stMeleeAggressing;}
				if(pm->cmd.forwardmove < 0){state = stMeleeDegressing;}
			}
			if((pm->ps->lockedPlayer->bitFlags & isDead) || (pm->ps->lockedPlayer->bitFlags & isUnconcious) || (pm->ps->lockedPlayer->bitFlags & isCrashed)){
				PM_StopMelee();
				PM_StopLockon();
				return;
			}
			if(state != stMeleeIdle){pm->ps->timers[tmMeleeIdle] = 0;}
			if(distance > 64 && pm->ps->bitFlags & usingMelee){PM_StopMelee();}
		}
	}
	else{
		PM_StopMelee();
		return;
	}
	damage = 0;
	animation = pm->ps->bitFlags & usingMelee ? ANIM_FLY_IDLE : 0;
	meleeCharge = pm->ps->timers[tmMeleeCharge];
	enemyState = pm->ps->lockedPlayer->stats[stMeleeState];
	enemyDefense = enemyState == stMeleeUsingEvade || enemyState == stMeleeUsingBlock ? qtrue : qfalse;
	// ===================
	// Melee Logic
	// ===================
	if(distance <= 64 && !pm->ps->timers[tmFreeze]){
		// Preparation
		PM_CheckDrift();
		if(state != stMeleeStartAttack && state != stMeleeUsingSpeed){PM_StopDrift();}
		// Breakers
		if(pm->ps->timers[tmMeleeBreakerWait]){
			pm->ps->timers[tmMeleeBreakerWait] -= pml.msec;
			if(pm->ps->timers[tmMeleeBreakerWait] < 0){pm->ps->timers[tmMeleeBreakerWait] = 0;}
		}
		if(pm->ps->timers[tmMeleeBreaker]){
			damage = (pm->ps->powerLevel[plCurrent] * 0.05) * pm->ps->stats[stMeleeAttack];
			if(pm->ps->timers[tmMeleeBreaker] > 0){
				state = stMeleeUsingChargeBreaker;
				if(enemyState == stMeleeUsingSpeed){
					//PM_AddEvent(EV_MELEE_BREAKER_BACKFIRE);
					pm->ps->timers[tmFreeze] = 950;
					pm->ps->powerLevel[plUseFatigue] = damage;
					state = stMeleeIdle;
				}
				else if(pm->ps->lockedPlayer->timers[tmMeleeBreaker]){
					//PM_AddEvent(EV_MELEE_BREAKER_CLASH);
					PM_AddEvent(EV_MELEE_KNOCKBACK);
					pm->ps->timers[tmFreeze] = 500;
					pm->ps->timers[tmMeleeIdle] = -480;
					pm->ps->lockedPlayer->timers[tmFreeze] = 500;
					pm->ps->lockedPlayer->timers[tmMeleeIdle] = -480;
				}
				else if(enemyState != stMeleeUsingEvade){
					PM_EndDrift();
					PM_AddEvent(EV_MELEE_BREAKER);
					enemyState = stMeleeIdle;
					pm->ps->timers[tmFreeze] = 100;
					pm->ps->powerLevel[plHealthPool] += damage * 0.5;
					pm->ps->powerLevel[plMaximumPool] += damage * 0.3;
					pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] = damage;
					pm->ps->lockedPlayer->timers[tmFreeze] = 500;
					pm->ps->lockedPlayer->timers[tmMeleeCharge] = 0;
				}
			}
			else{
				state = stMeleeUsingSpeedBreaker;
				if(pm->ps->lockedPlayer->timers[tmMeleeCharge]){
					//PM_AddEvent(EV_MELEE_BREAKER_BACKFIRE);
					pm->ps->timers[tmFreeze] = 300;
					pm->ps->lockedPlayer->timers[tmMeleeCharge] = 1000;
					state = stMeleeIdle;
				}
				else if(pm->ps->lockedPlayer->timers[tmMeleeBreaker]){
					//PM_AddEvent(EV_MELEE_CLASH);
					PM_AddEvent(EV_MELEE_KNOCKBACK);
					pm->ps->timers[tmFreeze] = 500;
					pm->ps->timers[tmMeleeIdle] = -480;
					pm->ps->lockedPlayer->timers[tmFreeze] = 500;
					pm->ps->lockedPlayer->timers[tmMeleeIdle] = -480;
				}
				else if(enemyState != stMeleeUsingBlock){
					PM_EndDrift();
					PM_AddEvent(EV_MELEE_BREAKER);
					enemyState = stMeleeIdle;
					pm->ps->powerLevel[plHealthPool] += damage * 0.5;
					pm->ps->powerLevel[plMaximumPool] += damage * 0.3;
					pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] = damage;
					pm->ps->timers[tmFreeze] = 100;
					pm->ps->lockedPlayer->timers[tmFreeze] = 500;
				}
			}
			pm->ps->timers[tmMeleeBreaker] = 0;
		}
		// Power Melee / Charge Breaker
		else if(state == stMeleeChargingPower || state == stMeleeStartPower || pm->cmd.buttons & BUTTON_ALT_ATTACK){
			if((state == stMeleeChargingPower || state == stMeleeStartPower) && (!(pm->cmd.buttons & BUTTON_ALT_ATTACK) || meleeCharge >= 550)){
				if(meleeCharge >= 550){
					damage = 0;
					pm->ps->timers[tmFreeze] = 1000;
					PM_EndDrift();
					pm->ps->powerLevel[plUseFatigue] += pm->ps->powerLevel[plMaximum] * 0.05;
					if(pm->ps->lockedPlayer->timers[tmMeleeCharge] > 50){
						pm->ps->timers[tmMeleeIdle] = -480;
						pm->ps->lockedPlayer->timers[tmMeleeIdle] = -480;
						PM_AddEvent(EV_MELEE_KNOCKBACK);
						//PM_AddEvent(EV_MELEE_CLASH);
						PM_StopMelee();
					}
					else if(enemyState != stMeleeUsingEvade){
						damage = (pm->ps->powerLevel[plCurrent] * 0.15) * pm->ps->stats[stMeleeAttack];
						if(enemyState == stMeleeUsingBlock){damage *= 0.3;}
						if(state != stMeleeStartPower){PM_AddEvent(EV_MELEE_KNOCKBACK);}
					}
					else{
						pm->ps->powerLevel[plUseFatigue] = damage * 0.8;
						pm->ps->lockedPlayer->powerLevel[plUseFatigue] = damage * 0.4;
					}
					if(enemyState != stMeleeUsingBlock && enemyState != stMeleeUsingEvade){
						pm->ps->lockedPlayer->powerups[PW_KNOCKBACK_SPEED] = (pm->ps->powerLevel[plCurrent] / 21.84) + pm->ps->stats[stKnockbackPower];
						if(pm->ps->lockedPlayer->timers[tmKnockback]){pm->ps->lockedPlayer->powerups[PW_KNOCKBACK_SPEED] *= 2;}
						if(pm->ps->bitFlags & usingBoost){
							damage *= 1.5;
							pm->ps->lockedPlayer->powerups[PW_KNOCKBACK_SPEED] *= 1.8;
						}
						pm->ps->lockedPlayer->timers[tmKnockback] = 5000;
						PM_StopMelee();
					}
					pm->ps->powerLevel[plHealthPool] += damage;
					pm->ps->powerLevel[plMaximumPool] += damage * 0.8;
					pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] += damage;
					state = stMeleeUsingPower;
					meleeCharge = 0;
				}
				else if(!pm->ps->timers[tmMeleeBreakerWait]){
					pm->ps->timers[tmMeleeBreaker] = 1;
					pm->ps->timers[tmMeleeBreakerWait] = 500;
					pm->ps->stats[stAnimState] = (Q_random(&realRandom.tm_sec) * 5)+1;
					meleeCharge = 0;
				}
				else{meleeCharge = 0;}
			}
			else{
				direction = 5;
				if(pm->cmd.upmove > 0){direction = 1;}
				else if(pm->cmd.upmove < 0){direction = 2;}
				else if(pm->cmd.rightmove > 0){direction = 3;} 
				else if(pm->cmd.rightmove < 0){direction = 4;}
				pm->ps->lockedPlayer->knockBackDirection = direction;
				state = stMeleeChargingPower;
				meleeCharge += pml.msec;
				if(meleeCharge >= 550){meleeCharge = 550;}
			}
		}
		// Stun Melee / Speed Breaker
		else if(state == stMeleeChargingStun || pm->cmd.buttons & BUTTON_ATTACK){
			if(state == stMeleeChargingStun && (!(pm->cmd.buttons & BUTTON_ATTACK) ||  meleeCharge >= 1000)){
				if(!pm->ps->timers[tmMeleeBreakerWait]){
					pm->ps->timers[tmMeleeBreaker] = -1;
					pm->ps->timers[tmMeleeBreakerWait] = 500;
					pm->ps->stats[stAnimState] = (Q_random(&realRandom.tm_sec) * 5)+1;
					meleeCharge = 0;
				}
				else{meleeCharge = 0;}
			}
			else{
				state = stMeleeChargingStun;
				meleeCharge += pml.msec;
				if(meleeCharge >= 1000){meleeCharge = 1000;}
			}
		}
		// Start / Speed Melee / Evade
		else if(pm->cmd.forwardmove){
			if(pm->cmd.forwardmove > 0){
				meleeCharge = 0;
				// Start Sequence
				if(!(pm->ps->bitFlags & usingMelee) && !(pm->ps->lockedPlayer->bitFlags & usingMelee) &&
					!(pm->ps->bitFlags & isStruggling) && !(pm->ps->lockedPlayer->bitFlags & isStruggling)){
					state = stMeleeStartAttack;
					pm->ps->timers[tmFreeze] = 1000;
					pm->ps->lockedPlayer->timers[tmFreeze] = 1000;
					PM_SyncMelee();
					if(enemyState == stMeleeAggressing){
						pm->ps->powerups[PW_DRIFTING] = 0;
						pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 0;
						PM_AddEvent(EV_MELEE_BREAKER);
						enemyState = stMeleeStartAttack;
					}
					else if(pm->ps->lockedPlayer->bitFlags & usingBlock || enemyState == stMeleeDegressing){
						pm->ps->powerups[PW_DRIFTING] = 1;
						pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 2;
						enemyState = stMeleeStartDodge;
						PM_AddEvent(EV_MELEE_BREAKER);
						pm->ps->powerLevel[plUseFatigue] = pm->ps->powerLevel[plMaximum] * 0.10;
					}
					else{
						if(pm->ps->lockedPlayer->timers[tmKnockback]){
							state = stMeleeChargingPower;
							meleeCharge = 750;
							pm->ps->timers[tmFreeze] = 300;
							pm->ps->lockedPlayer->timers[tmFreeze] = 300;
							pm->ps->lockedPlayer->knockBackDirection = 2;
						}
						else if(pm->ps->timers[tmBoost] > 2500){
							state = stMeleeStartPower;
							enemyState = stMeleeStartHit;
							meleeCharge = 750;
							pm->ps->timers[tmFreeze] = 1000;
							pm->ps->lockedPlayer->timers[tmFreeze] = 1000;
							pm->ps->lockedPlayer->knockBackDirection = 5;
							PM_AddEvent(EV_MELEE_KNOCKBACK);
						}
						else{
							pm->ps->powerups[PW_DRIFTING] = 1;
							pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 2;
							if(pm->ps->timers[tmBoost]){
								pm->ps->powerups[PW_DRIFTING] = 1;
								pm->ps->lockedPlayer->powerups[PW_DRIFTING] = 2;
								pm->ps->timers[tmFreeze] = 0;
								pm->ps->lockedPlayer->timers[tmFreeze] = 0;
								state = stMeleeUsingSpeed;
							}
							else{
								pm->ps->lockedPlayer->timers[tmFreeze] = 2500;
								enemyState = stMeleeStartHit;
								PM_AddEvent(EV_MELEE_KNOCKBACK);
							}
						}
					}
				}
				// Speed Melee
				else{
					damage = (pm->ps->powerLevel[plFatigue] * 0.013) * pm->ps->stats[stMeleeAttack];
					if(pm->ps->powerups[PW_DRIFTING]==1){damage *= 1.2;}
					if(pm->ps->bitFlags & usingBoost){damage *= 1.5;}
					pm->ps->timers[tmMeleeSpeed] += pml.msec;
					if(pm->ps->timers[tmMeleeSpeed] >= 100){
						if(enemyState == stMeleeUsingBlock){damage *= 0.2;}
						if(enemyState == stMeleeUsingEvade){
							pm->ps->powerLevel[plUseFatigue] += damage * 0.5;
							damage = 0;
						}
						if(pm->ps->lockedPlayer->powerLevel[plHealth] > damage){
							pm->ps->powerLevel[plHealthPool] += damage * 0.7;
							pm->ps->powerLevel[plMaximumPool] += damage * 0.5;
							pm->ps->lockedPlayer->powerLevel[plDamageFromMelee] += damage;
						}
						else{
							pm->ps->lockedPlayer->powerLevel[plUseFatigue] += damage;
						}
						pm->ps->timers[tmMeleeSpeed] -= 100;
					}
					state = stMeleeUsingSpeed;
				}
			}
			// Evade
			else{
				if(pm->ps->lockedPlayer->timers[tmFreeze] > 0 || enemyDefense){PM_StopMelee();}
				else{state = stMeleeUsingEvade;}
				PM_MeleeIdle();
			}
		}
		// Blocking
		else if(pm->ps->bitFlags & usingBlock){
			meleeCharge = 0;
			state = stMeleeUsingBlock;
			PM_MeleeIdle();
			if(enemyDefense){PM_StopMelee();}
		}
		else{
			meleeCharge = 0;
			state = stMeleeIdle;
			PM_MeleeIdle();
		}
	}
	// ===========================
	// Melee Animations / Events
	// ===========================
	enemyChanged = pm->ps->lockedPlayer->stats[stMeleeState] == enemyState ? qfalse : qtrue;
	if(pm->ps->lockedPlayer->lockedTarget != pm->ps->clientNum + 1 && !enemyChanged){enemyState = -1;}
	if((pm->ps->lockedPlayer->timers[tmKnockback] || pm->ps->lockedPlayer->timers[tmFreeze]) && !enemyChanged){enemyState = -1;}
	if(state){
		if(state == stMeleeStartAttack){animation = ANIM_BREAKER_MELEE_ATTACK1;}
		if(state == stMeleeStartHit){animation = ANIM_BREAKER_MELEE_HIT1;}
		if(state == stMeleeStartDodge){animation = ANIM_BLOCK;}
		if(state == stMeleeUsingSpeed || enemyState == stMeleeUsingSpeed){	
			if(state == stMeleeUsingSpeed){
				animation = ANIM_SPEED_MELEE_ATTACK;
				if(enemyState != stMeleeUsingEvade){PM_AddEvent(EV_MELEE_SPEED);}
			}
			else{		
				if(state == stMeleeUsingBlock){animation = ANIM_SPEED_MELEE_BLOCK;}
				else if(state == stMeleeUsingEvade){
					animation = ANIM_SPEED_MELEE_DODGE;
					PM_AddEvent(EV_MELEE_MISS);
				}
				else if(state != stMeleeChargingPower && state != stMeleeChargingStun){animation = ANIM_SPEED_MELEE_HIT;}
			}
		}
		if(state == stMeleeUsingPower || state == stMeleeStartPower || enemyState == stMeleeUsingPower || enemyState == stMeleeStartPower){
			if(state == stMeleeUsingPower || state == stMeleeStartPower){
				direction = pm->ps->lockedPlayer->knockBackDirection;
				if(direction == 1){animation = ANIM_POWER_MELEE_4_HIT;}
				else if(direction == 2){animation = ANIM_POWER_MELEE_2_HIT;}
				else if(direction == 3){animation = ANIM_POWER_MELEE_6_HIT;}
				else if(direction == 4){animation = ANIM_POWER_MELEE_5_HIT;}
				else if(pm->ps->bitFlags & usingBoost && state != stMeleeStartPower){animation = ANIM_POWER_MELEE_3_HIT;}
				else{animation = ANIM_POWER_MELEE_1_HIT;}
			}
			else{
				if(state == stMeleeUsingBlock){animation = ANIM_BLOCK;}
				if(state == stMeleeStartPower){animation = ANIM_BREAKER_MELEE_HIT1;}
			}
		}
		if(state == stMeleeUsingStun  || enemyState == stMeleeUsingStun){
			if(state == stMeleeUsingStun){
				animation = ANIM_POWER_MELEE_3_HIT;
			}
			else{
				if(state == stMeleeUsingBlock){animation = ANIM_BLOCK;}
				else{animation = ANIM_STUNNED_MELEE;}
			}
		}
		if(state == stMeleeChargingPower){
			direction = pm->ps->lockedPlayer->knockBackDirection;
			if(direction == 1){animation = ANIM_POWER_MELEE_4_CHARGE;}
			else if(direction == 2){animation = ANIM_POWER_MELEE_2_CHARGE;}
			else if(direction == 3){animation = ANIM_POWER_MELEE_6_CHARGE;}
			else if(direction == 4){animation = ANIM_POWER_MELEE_5_CHARGE;}
			else if(pm->ps->bitFlags & usingBoost){animation = ANIM_POWER_MELEE_3_CHARGE;}
			else{animation = ANIM_POWER_MELEE_1_CHARGE;}
		}
		if(state == stMeleeChargingStun){
			animation = ANIM_POWER_MELEE_1_CHARGE;
		}
		if(state == stMeleeUsingSpeedBreaker || enemyState == stMeleeUsingSpeedBreaker || state == stMeleeUsingChargeBreaker || enemyState == stMeleeUsingChargeBreaker){
			int breaker = pm->ps->stats[stAnimState];
			int enemyBreaker = pm->ps->lockedPlayer->stats[stAnimState];
			if(state == stMeleeUsingSpeedBreaker || state == stMeleeUsingChargeBreaker){
				if(breaker == 1){animation = ANIM_BREAKER_MELEE_ATTACK1;}
				else if(breaker == 2){animation = ANIM_BREAKER_MELEE_ATTACK2;}
				else if(breaker == 3){animation = ANIM_BREAKER_MELEE_ATTACK3;}
				else if(breaker == 4){animation = ANIM_BREAKER_MELEE_ATTACK4;}
				else if(breaker == 5){animation = ANIM_BREAKER_MELEE_ATTACK5;}
				else if(breaker == 6){animation = ANIM_BREAKER_MELEE_ATTACK6;}
			}
			else{
				if(enemyBreaker == 1){animation = ANIM_BREAKER_MELEE_HIT1;}
				else if(enemyBreaker == 2){animation = ANIM_BREAKER_MELEE_HIT2;}
				else if(enemyBreaker == 3){animation = ANIM_BREAKER_MELEE_HIT3;}
				else if(enemyBreaker == 4){animation = ANIM_BREAKER_MELEE_HIT4;}
				else if(enemyBreaker == 5){animation = ANIM_BREAKER_MELEE_HIT5;}
				else if(enemyBreaker == 6){animation = ANIM_BREAKER_MELEE_HIT6;}
			}
		}
	}
	if(animation){PM_ContinueLegsAnim(animation);}
	if(enemyState != -1){pm->ps->lockedPlayer->stats[stMeleeState] = enemyState;}
	pm->ps->stats[stMeleeState] = state;
	pm->ps->timers[tmMeleeCharge] = meleeCharge;
	if(distance <= 64){pm->cmd.rightmove = 0;}
}
/*==============
PM_Weapon
Generates weapon events and modifes the weapon counter
==============*/
void PM_WeaponRelease(void){
	if(pm->ps->bitFlags & isStruggling){return;}
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
	float energyScale;
	if(pm->ps->weaponstate != WEAPON_GUIDING){pm->ps->bitFlags &= ~isGuiding;}
	if(pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR){return;}
	if(pm->ps->bitFlags & isStruggling || pm->ps->bitFlags & usingSoar
		|| pm->ps->bitFlags & isPreparing || pm->ps->timers[tmTransform] > 1 || pm->ps->bitFlags & usingMelee
		|| (pm->ps->lockedTarget && pm->ps->lockedPlayer->timers[tmKnockback] && VectorLength(pm->ps->velocity))){
		if(pm->ps->timers[tmTransform] > 1 || pm->ps->bitFlags & usingMelee){PM_WeaponRelease();}
		return;
	}
	if(pm->ps->bitFlags & isStruggling){return;}
	if(pm->ps->weapon == WP_NONE || pm->ps->bitFlags & usingMelee){
		PM_WeaponRelease();
		return;
	}
	if(pm->ps->lockedTarget && pm->ps->lockedPlayer->timers[tmKnockback] && VectorLength(pm->ps->velocity)){return;}
	if(pm->ps->pm_flags & PMF_ATTACK1_HELD){
		if(!(pm->cmd.buttons & BUTTON_ATTACK)){pm->ps->pm_flags &= ~PMF_ATTACK1_HELD;}
		else{return;}
	}
	if(pm->ps->pm_flags & PMF_ATTACK2_HELD){
		if(!(pm->cmd.buttons & BUTTON_ALT_ATTACK)){pm->ps->pm_flags &= ~PMF_ATTACK2_HELD;}
		else{return;}
	}
	// Retrieve our weapon's settings
	weaponInfo = pm->ps->currentSkill;
	if(weaponInfo[WPSTAT_BITFLAGS] & WPF_ALTWEAPONPRESENT){
		alt_weaponInfo = &weaponInfo[WPSTAT_ALT_POWERLEVELCOST];
	} else{
		alt_weaponInfo = weaponInfo;	 // Keep both sets the same.
		if(pm->cmd.buttons & BUTTON_ALT_ATTACK) {	// ifwe have no altfire,
			pm->cmd.buttons &= ~BUTTON_ALT_ATTACK;	// override ALT_ATTACK to ATTACK.
			pm->cmd.buttons |= BUTTON_ATTACK;
		}
	}
	if(weaponInfo[WPSTAT_NUMCHECK] != pm->ps->weapon){return;}
	costPrimary = weaponInfo[WPSTAT_POWERLEVELCOST];
	costSecondary = alt_weaponInfo[WPSTAT_POWERLEVELCOST];
	energyScale = (float)pm->ps->powerLevel[plCurrent] / (float)pm->ps->powerLevel[plMaximum];
	switch(pm->ps->weaponstate){
	case WEAPON_READY:
	case WEAPON_DROPPING:
	case WEAPON_RAISING:
	case WEAPON_COOLING:
		pm->ps->timers[tmAttackLife] = 0;
		if(pm->ps->weaponTime > 0){
			pm->ps->weaponTime -= pml.msec;
		}

		if(pm->ps->weaponTime > 0){
			break;
		} else{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
		}
		if(pm->ps->weapon == pm->cmd.weapon){
			if(pm->cmd.buttons & BUTTON_ATTACK) {
				if(weaponInfo[WPSTAT_BITFLAGS] & WPF_NEEDSCHARGE){
					pm->ps->weaponstate = WEAPON_CHARGING;
					PM_StartTorsoAnim(ANIM_KI_ATTACK1_PREPARE + (pm->ps->weapon - 1) * 2 );
					break;
				}
				pm->ps->stats[stChargePercentPrimary] = 100;
				pm->ps->powerLevel[plUseFatigue] += costPrimary * pm->ps->baseStats[stEnergyAttackCost] * energyScale;
				if(weaponInfo[WPSTAT_BITFLAGS] & WPF_CONTINUOUS){
					PM_AddEvent(EV_FIRE_WEAPON );
					pm->ps->weaponstate = WEAPON_FIRING;
				} else{
					PM_AddEvent(EV_FIRE_WEAPON);
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				}
				PM_StartTorsoAnim(ANIM_KI_ATTACK1_FIRE + (pm->ps->weapon - 1) * 2 );
				break;
			}
			if(pm->cmd.buttons & BUTTON_ALT_ATTACK){
				if(alt_weaponInfo[WPSTAT_BITFLAGS] & WPF_NEEDSCHARGE){
					pm->ps->weaponstate = WEAPON_ALTCHARGING;
					PM_StartTorsoAnim(ANIM_KI_ATTACK1_ALT_PREPARE + (pm->ps->weapon - 1) * 2 );
					break;
				}
				pm->ps->stats[stChargePercentSecondary] = 100;
				pm->ps->powerLevel[plUseFatigue] += costSecondary * pm->ps->baseStats[stEnergyAttackCost] * energyScale;
				if(alt_weaponInfo[WPSTAT_BITFLAGS] & WPF_CONTINUOUS){
					pm->ps->weaponstate = WEAPON_ALTFIRING;
					PM_AddEvent(EV_ALTFIRE_WEAPON );
				} else{
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
					PM_AddEvent(EV_ALTFIRE_WEAPON );
				}
				PM_StartTorsoAnim(ANIM_KI_ATTACK1_ALT_FIRE + (pm->ps->weapon - 1) * 2 );
				break;
			}
		}
		else{
			PM_BeginWeaponChange(pm->cmd.weapon);
		}
		break;
	case WEAPON_GUIDING:
		if(pm->cmd.buttons & BUTTON_ATTACK && !(pm->ps->bitFlags & isStruggling) &&	pm->ps->timers[tmAttackLife] > 2000){
			PM_AddEvent(EV_DETONATE_WEAPON);
			pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
			PM_StartTorsoAnim(ANIM_IDLE);
		}
		pm->ps->timers[tmAttackLife] += pml.msec;
		pm->ps->timers[tmImpede] = 100;
		break;
	case WEAPON_ALTGUIDING:
		if(pm->cmd.buttons & BUTTON_ALT_ATTACK && !(pm->ps->bitFlags & isStruggling) &&	pm->ps->timers[tmAttackLife] > 2000){
			PM_AddEvent(EV_DETONATE_WEAPON);
			pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
			PM_StartTorsoAnim(ANIM_IDLE);
		}
		pm->ps->timers[tmAttackLife] += pml.msec;
		pm->ps->timers[tmImpede] = 100;
		break;
	case WEAPON_CHARGING:
		chargeRate = (pm->ps->bitFlags & usingBoost) ? 2 : 1;
		pm->ps->timers[tmAttack1] += pml.msec;
		pm->ps->timers[tmImpede] = weaponInfo[WPSTAT_RESTRICT_MOVEMENT];
		if(pm->ps->timers[tmAttack1] >= weaponInfo[WPSTAT_CHRGTIME]){
			pm->ps->timers[tmAttack1] -= weaponInfo[WPSTAT_CHRGTIME];
			if(pm->ps->stats[stChargePercentPrimary] < 100){
				if(pm->ps->stats[stChargePercentPrimary] == 0 && pm->ps->bitFlags & usingBoost){
					pm->ps->stats[stChargePercentPrimary] = 25;
					costPrimary *= 25;
				}
				pm->ps->stats[stChargePercentPrimary] += chargeRate;
				if(pm->ps->stats[stChargePercentPrimary] > 100){
					pm->ps->stats[stChargePercentPrimary] = 100;
				}
				pm->ps->powerLevel[plUseFatigue] += costPrimary * pm->ps->baseStats[stEnergyAttackCost] * energyScale;
				pm->ps->powerLevel[plUseHealth] += weaponInfo[WPSTAT_HEALTHCOST] * ((float)pm->ps->powerLevel[plMaximum] * 0.0001) * pm->ps->baseStats[stEnergyAttackCost];
				pm->ps->powerLevel[plUseMaximum] += weaponInfo[WPSTAT_MAXIMUMCOST] * ((float)pm->ps->powerLevel[plMaximum] * 0.0001) * pm->ps->baseStats[stEnergyAttackCost];
			}
		}
		if(!(pm->cmd.buttons & BUTTON_ATTACK)) {
			pm->ps->timers[tmAttack1] = 0;
			if(weaponInfo[WPSTAT_BITFLAGS] & WPF_READY){
				weaponInfo[WPSTAT_BITFLAGS] &= ~WPF_READY;
				if(weaponInfo[WPSTAT_BITFLAGS] & WPF_GUIDED){
					pm->ps->weaponstate = WEAPON_GUIDING;
					pm->ps->bitFlags |= isGuiding;
				} else{
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
				}
				PM_AddEvent(EV_FIRE_WEAPON);
				PM_StartTorsoAnim(ANIM_KI_ATTACK1_FIRE + (pm->ps->weapon - 1) * 2 );
			} else{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				pm->ps->stats[stChargePercentPrimary] = 0;
				PM_StartTorsoAnim(ANIM_IDLE);
			}
		}
		break;
	case WEAPON_ALTCHARGING:
		chargeRate = (pm->ps->bitFlags & usingBoost) ? 2 : 1;
		pm->ps->timers[tmAttack2] += pml.msec;
		pm->ps->timers[tmImpede] = weaponInfo[WPSTAT_ALT_RESTRICT_MOVEMENT];
		if(pm->ps->timers[tmAttack2] >= weaponInfo[WPSTAT_ALT_CHRGTIME]){
			pm->ps->timers[tmAttack2] -= weaponInfo[WPSTAT_ALT_CHRGTIME];
			if(pm->ps->stats[stChargePercentSecondary] < 100){
				if(pm->ps->stats[stChargePercentPrimary] == 0 && pm->ps->bitFlags & usingBoost){
					pm->ps->stats[stChargePercentPrimary] = 25;
					costSecondary *= 25;
				}
				pm->ps->stats[stChargePercentSecondary] += chargeRate;
				if(pm->ps->stats[stChargePercentSecondary] > 100){
					pm->ps->stats[stChargePercentSecondary] = 100;
				}
				pm->ps->powerLevel[plUseFatigue] += costSecondary * pm->ps->baseStats[stEnergyAttackCost] * energyScale;
				pm->ps->powerLevel[plUseHealth] += weaponInfo[WPSTAT_ALT_HEALTHCOST] * ((float)pm->ps->powerLevel[plMaximum] * 0.0001);
				pm->ps->powerLevel[plUseMaximum] += weaponInfo[WPSTAT_ALT_MAXIMUMCOST] * ((float)pm->ps->powerLevel[plMaximum] * 0.0001);
			}
		}
		if(!(pm->cmd.buttons & BUTTON_ALT_ATTACK)) {
			if(alt_weaponInfo[WPSTAT_BITFLAGS] & WPF_READY){
				alt_weaponInfo[WPSTAT_BITFLAGS] &= ~WPF_READY;
				if(alt_weaponInfo[WPSTAT_BITFLAGS] & WPF_GUIDED){
					pm->ps->weaponstate = WEAPON_ALTGUIDING;
					pm->ps->bitFlags |= isGuiding;
				} else{
					pm->ps->weaponstate = WEAPON_COOLING;
					pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
				}
				PM_AddEvent(EV_ALTFIRE_WEAPON );
				PM_StartTorsoAnim(ANIM_KI_ATTACK1_ALT_FIRE + (pm->ps->weapon - 1) * 2 );
			} else{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				pm->ps->stats[stChargePercentSecondary] = 0;
				PM_StartTorsoAnim(ANIM_IDLE );
			}
		}
		break;
	case WEAPON_FIRING:
		if(!(pm->cmd.buttons & BUTTON_ATTACK)){
			pm->ps->weaponstate = WEAPON_COOLING;
			pm->ps->weaponTime += weaponInfo[WPSTAT_COOLTIME];
			PM_StartTorsoAnim(ANIM_IDLE );
			break;
		}
		pm->ps->weaponTime += pml.msec;
		while(pm->ps->weaponTime > 100){
			pm->ps->powerLevel[plUseFatigue] += costPrimary * pm->ps->baseStats[stEnergyAttackCost] * energyScale;
		}
		break;
	case WEAPON_ALTFIRING:
		if(!(pm->cmd.buttons & BUTTON_ALT_ATTACK)){
			pm->ps->weaponstate = WEAPON_COOLING;
			pm->ps->weaponTime += alt_weaponInfo[WPSTAT_COOLTIME];
			PM_StartTorsoAnim(ANIM_IDLE );
			break;
		}
		pm->ps->weaponTime += pml.msec;
		while(pm->ps->weaponTime > 100){
			pm->ps->powerLevel[plUseFatigue] += costSecondary * pm->ps->baseStats[stEnergyAttackCost] * energyScale;
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
		pm->ps->lockedPlayer->bitFlags &= ~isTargeted;
		PM_AddEvent(EV_LOCKON_END);
	}
	pm->ps->lockedPosition = NULL;
	pm->ps->lockedTarget = 0;
}
void PM_CheckLockon(void){
	int	lockBox;
	trace_t	trace;
	vec3_t minSize,maxSize,forward,up,end;
	if(pm->ps->lockedTarget > 0){
		pm->ps->lockedPlayerData[lkPowerCurrent] = pm->ps->lockedPlayer->powerLevel[plCurrent];
		pm->ps->lockedPlayerData[lkPowerHealth] = pm->ps->lockedPlayer->powerLevel[plHealth];
		pm->ps->lockedPlayerData[lkPowerMaximum] = pm->ps->lockedPlayer->powerLevel[plMaximum];
		pm->ps->lockedPlayer->bitFlags |= isTargeted;
	}
	if(pm->cmd.buttons & BUTTON_GESTURE && pm->ps->stats[stMeleeState] == 0){
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

		for (i = 0; i < 3; i++) {
			ps->delta_angles[i] = ANGLE2SHORT(angles[i]) - cmd->angles[i];
		}
	}
	else if(pm->ps->bitFlags & usingFlight && pm->ps->states & advancedFlight){
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
		// Try and compensate for the drifting problem.
		new_angles[PITCH] += 0.05f;
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
		if(i == ROLL && ps->viewangles[i] != 0) {
			ps->viewangles[i] = 0;
			ps->delta_angles[i] = 0 - cmd->angles[i];
		}
		for (i=0; i<3; i++) {
			ps->viewangles[i] = AngleNormalize180(new_angles[i]);
			ps->delta_angles[i] = ANGLE2SHORT(ps->viewangles[i]) - cmd->angles[i];
		}
		return;
	}
	else{
		ps->viewangles[ROLL] = 0;
		ps->delta_angles[ROLL] = 0;
		offset_angles[ROLL]= 0;
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
	for (i=0 ; i<3 ; i++) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if(i == PITCH){
			if(pm->ps->lockedTarget <= 0){
				if(temp > 16000){
					ps->delta_angles[i] = 16000 - cmd->angles[i];
					temp = 16000;
				} else if(temp < -16000){
					ps->delta_angles[i] = -16000 - cmd->angles[i];
					temp = -16000;
				}
			}
			
		}
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
	pm->ps->gravity[2] = pm->ps->bitFlags & usingJump ? pm->ps->gravity[2] : 800;
	pm->ps->eFlags &= ~EF_AURA;
	memset (&pml, 0, sizeof(pml));
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if(pml.msec < 1){pml.msec = 1;}
	else if(pml.msec > 200){pml.msec = 200;}
	if(abs(pm->cmd.forwardmove)> 64 || abs(pm->cmd.rightmove)> 64){pm->cmd.buttons &= ~BUTTON_WALKING;}
	AngleVectors(pm->ps->viewangles,pml.forward,pml.right,pml.up);
	pml.previous_waterlevel = pmove->waterlevel;
	PM_Impede();
	PM_Burn();
	PM_Ride();
	PM_CheckKnockback();
	PM_CheckHover();
	if(PM_CheckTransform()){return;}
	PM_CheckLoopingSound();
	PM_SetWaterLevel();
	if(pm->cmd.buttons != 0 && pm->cmd.buttons != 2048){
		//Com_Printf("%i\n",pm->cmd.buttons);
	}
	if(pm->ps->lockedTarget > 0){
		meleeRange = Distance(pm->ps->origin,*(pm->ps->lockedPosition)) <= 48 ? qtrue : qfalse;
	}
	if(!(pm->ps->bitFlags & isTransforming)){
		PM_UsePowerLevel();
		PM_BurnPowerLevel();
		PM_CheckCrash();
		PM_CheckStatus();
		PM_CheckStruggling();
		if(!pm->ps->timers[tmKnockback] && !(pm->ps->bitFlags & isUnconcious) && !(pm->ps->bitFlags & isDead) && !(pm->ps->bitFlags & isCrashed)){
			PM_CheckBoost();
			PM_CheckPowerLevel();
			PM_CheckLockon();
			PM_CheckZanzoken();
			if(!meleeRange){
				PM_CheckBlock();
				PM_CheckJump();
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
	PM_SetView();
	if(pm->ps->bitFlags & isUnconcious || pm->ps->bitFlags & isDead || pm->ps->bitFlags & isCrashed){
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
		int state;
		PM_Melee();
		state = pm->ps->stats[stMeleeState];
		if(state != stMeleeUsingPower || state != stMeleeUsingStun){
			PM_UpdateViewAngles(pm->ps,&pm->cmd);
			PM_Weapon();
		}
	}
	if(pm->ps->pm_type == PM_SPECTATOR){
		PM_FlyMove();
	}
	PM_TorsoAnimation();
	PM_Freeze();
	PM_Blind();
	PM_GroundTrace();
	PM_NearGroundTrace();
	if(!pm->ps->timers[tmFreeze] && !pm->ps->timers[tmImpede] && pm->ps->timers[tmTransform] < 100 && pm->ps->timers[tmTransform] > -100){
		if(!(pm->ps->bitFlags & usingAlter) && !(pm->ps->bitFlags & isStruggling) && !(pm->ps->bitFlags & usingMelee)){
			PM_FlyMove();
			PM_DashMove();
			PM_WalkMove();
			PM_AirMove();
		}
		else if(pm->ps->powerups[PW_DRIFTING] || pm->ps->timers[tmMeleeIdle] < 0){PM_AirMove();}
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

