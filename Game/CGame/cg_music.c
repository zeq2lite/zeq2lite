#include "cg_local.h"
void CG_CheckMusic(void){
	playerState_t *ps;
	ps = &cg.predictedPlayerState;
	if(!cgs.music.started){
		CG_StartMusic();
	}
	if(ps->bitFlags & isTransforming){
		if(cgs.music.currentType != 9){
			cgs.music.currentType = 9;
			CG_NextTrack();
		}
	}
	else if(ps->bitFlags & isStruggling){
		if(cgs.music.currentType != 4){
			cgs.music.currentType = 4;
			CG_NextTrack();
		}
	}
	else if(!(ps->bitFlags & isUnsafe)){
		if(ps->bitFlags & isTargeted || ps->lockedTarget > 0){
			if(ps->powerLevel[plHealth] < (ps->powerLevel[plMaximum] * 0.4)){
				if(cgs.music.currentType != 6){
					cgs.music.currentType = 6;
					CG_NextTrack();
				}
			}
			else if(cgs.music.currentType != 5){
				cgs.music.currentType = 5;
				CG_NextTrack();
			}
		}
		else{
			if(ps->powerLevel[plHealth] < (ps->powerLevel[plMaximum] * 0.4)){
				if(cgs.music.currentType != 3){
					cgs.music.currentType = 3;
					CG_FadeNext();
				}
			}
			else if(ps->bitFlags & underWater){
				if(cgs.music.currentType != 2){
					cgs.music.currentType = 2;
					CG_NextTrack();
				}
			}
			else if(cgs.music.currentType != 1){
				cgs.music.currentType = 1;
				CG_FadeNext();
			}
		}
	}
	else if(cgs.music.currentType != 0){
		cgs.music.currentType = 0;
		CG_NextTrack();		
	}
	if(cg.time > cgs.music.endTime){	
		int difference = (cg.time - cgs.music.endTime);
		float percent = 0.0;
		if(difference < cgs.music.fadeAmount){
			percent = 1.0 - ((float)difference / (float)cgs.music.fadeAmount);
		}
		else{
			cgs.music.fading = qfalse;
			CG_NextTrack();
			return;
		}
		trap_Cvar_Set("s_musicvolume",va("%f",percent * cg_music.value));
	}
}
int CG_GetMilliseconds(char *time){
	int index,compareIndex;
	int amount;
	char current[8];
	int i;
	index = 0;
	compareIndex = 0;
	amount = 0;
	while(1){
		if(index > Q_PrintStrlen(time)){break;}
		current[compareIndex] = time[index];
		if(strcmp(&current[index],":") == 0){
			amount += atoi(current) * 60 * 1000;
			compareIndex = -1;
			memset(current,0,8);
		}
		++index;
		++compareIndex;
	}
	amount += atoi(current) * 1000;
	return amount;
}
void CG_ParsePlaylist(void){
	fileHandle_t playlist;
	char *token,*parse,*name;
	char first,last;
	int trackIndex,typeIndex;
	int fileLength;
	char fileContents[32000];
	//char *strdup(const char *);
	trackIndex = 0;
	typeIndex = -1;
	cgs.music.currentIndex = -1;
	cgs.music.fadeAmount = 0;
	cgs.music.started = qtrue;
	cgs.music.random = qfalse;
	if(trap_FS_FOpenFile("music/playlist.cfg",0,FS_READ)>0){
		fileLength = trap_FS_FOpenFile("music/playlist.cfg",&playlist,FS_READ);
		trap_FS_Read(fileContents,fileLength,playlist);
		fileContents[fileLength] = 0;
		trap_FS_FCloseFile(playlist);
		parse = fileContents;
		while(1){
			token = COM_Parse(&parse);
			if(!token[0]){break;}
			first = token[0];
			last = token[Q_PrintStrlen(token)-1];
			last = *(va("%c",last));  // Psyco Fix?!
			if(!Q_stricmp(token,"type")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				if(!Q_stricmp(token,"random")){
					cgs.music.random = qtrue;
				}
			}
			else if(!Q_stricmp(token,"fade")){
				token = COM_Parse(&parse);
				if(!token[0]){break;}
				cgs.music.fadeAmount = CG_GetMilliseconds(token);
			}
			else if(strcmp(&last,"{") == 0){
				cgs.music.lastTrack[typeIndex] = -1;
				++typeIndex;
				//CG_Printf("Music Type [%i]\n",typeIndex);
			}
			else if(strcmp(&first,"}") == 0){
				cgs.music.typeSize[typeIndex] = trackIndex;
				trackIndex = 0;
			}
			else{
				static char temp[16][32][256];
				strncpy(temp[typeIndex][trackIndex],token,256-1);
				cgs.music.playlist[typeIndex][trackIndex] = temp[typeIndex][trackIndex];
				cgs.music.trackLength[typeIndex][trackIndex] = CG_GetMilliseconds(COM_Parse(&parse));
				//cgs.music.hasPlayed[typeIndex][trackIndex] = qfalse;
				//CG_Printf("  Track %i is %i seconds\n",trackIndex,cgs.music.trackLength[typeIndex][trackIndex]/1000);
				++trackIndex;
			}
		}
	}
}
void CG_StartMusic(void){
	CG_ParsePlaylist();
}
void CG_FadeNext(void){
	cgs.music.fading = qtrue;
	cgs.music.endTime = cg.time;
}
void CG_NextTrack(void){
	int nextIndex;
	int typeSize;
	char *path;
	int i;
	int duration;
	if(cgs.music.fading){return;}
	typeSize = cgs.music.typeSize[cgs.music.currentType];
	nextIndex = (nextIndex < typeSize) ? cgs.music.currentIndex + 1 : 0;
	if(cgs.music.random){
		nextIndex = fabs(crandom()) * typeSize;
	}
	if(nextIndex == cgs.music.lastTrack[cgs.music.currentType]){nextIndex += 1;}
	if(nextIndex < 0){nextIndex = typeSize-1;}
	if(nextIndex >= typeSize){nextIndex = 0;}
	path = va("music/%s",cgs.music.playlist[cgs.music.currentType][nextIndex]);
	duration = cgs.music.trackLength[cgs.music.currentType][nextIndex];
	//CG_Printf("Playing type %i | track %i.  Next track in %i seconds.\n",cgs.music.currentType,nextIndex,duration/1000);
	if(duration > 300000){duration = 300000;}
	cgs.music.endTime = cg.time + duration - cgs.music.fadeAmount;
	cgs.music.lastTrack[cgs.music.currentType] = nextIndex;
	trap_S_StartBackgroundTrack(path,path);
	trap_Cvar_Set("s_musicvolume",va("%f",cg_music.value));
}
