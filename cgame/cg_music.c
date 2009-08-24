#include "cg_local.h"
void CG_FadeNext(void);
void CG_CheckMusic(void){
	playerState_t *ps;
	ps = &cg.snap->ps;
	if(cg.time < 10000){return;}
	if(ps->bitFlags & isStruggling){
		if(cgs.music.currentType != 3){
			cgs.music.currentType = 3;
			CG_NextTrack();		
		}
	}
	else if(ps->bitFlags & isSafe){
		if(ps->powerLevel[plHealth] > (ps->powerLevel[plMaximum] * 0.4)){
			if(cgs.music.currentType != 1){
				cgs.music.currentType = 1;
				CG_FadeNext();
			}
		}
		else if(cgs.music.currentType != 2){
			cgs.music.currentType = 2;
			CG_FadeNext();
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
			CG_NextTrack();
			return;
		}
		trap_Cvar_Set("s_musicvolume",va("%f",percent * cg_music.value));
	}
}
void CG_FadeCurrent(void){
	cgs.music.endTime = cg.time;
}
int CG_GetMilliseconds(char *time){
	int index,compareIndex;
	int amount;
	char current[8];
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
	char *strdup(const char *);
	trackIndex = 0;
	typeIndex = -1;
	cgs.music.currentIndex = -1;
	cgs.music.currentType = 1;
	cgs.music.fadeAmount = 0;
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
			else if(strcmp(&last,"{") == 0){++typeIndex;}
			else if(strcmp(&first,"}") == 0){
				cgs.music.playlistSize[typeIndex] = trackIndex;
				trackIndex = 0;
			}
			else{
				static char temp[16][32][256];
				strncpy(temp[typeIndex][trackIndex],token,256-1);
				cgs.music.playlist[typeIndex][trackIndex] = temp[typeIndex][trackIndex];
				cgs.music.trackLength[typeIndex][trackIndex] = CG_GetMilliseconds(COM_Parse(&parse));
				cgs.music.hasPlayed[typeIndex][trackIndex] = qfalse;
				++trackIndex;
			}
		}
	}
}
void CG_StartMusic(void){
	CG_ParsePlaylist();
	CG_NextTrack();
}
void CG_FadeNext(void){
	cgs.music.endTime = cg.time + cgs.music.fadeAmount;
}
void CG_NextTrack(void){
	int nextIndex;
	int playlistSize;
	char *path;
	int i;
	qtime_t realRandom;
	playlistSize = cgs.music.playlistSize[cgs.music.currentType];
	CG_Printf("playlist size : %i\n",playlistSize);
	nextIndex = (nextIndex < playlistSize) ? cgs.music.currentIndex + 1 : 0;
	if(cgs.music.random){
		trap_RealTime(&realRandom);
		nextIndex = (int)(Q_random(&realRandom.tm_sec) * (float)(playlistSize-1));
	}
	path = va("music/%s",cgs.music.playlist[cgs.music.currentType][nextIndex]);
	cgs.music.endTime = cg.time + cgs.music.trackLength[cgs.music.currentType][nextIndex] - cgs.music.fadeAmount;
	trap_S_StartBackgroundTrack(path,path);
	trap_Cvar_Set("s_musicvolume",va("%f",cg_music.value));
}
