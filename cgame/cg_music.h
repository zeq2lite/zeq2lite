typedef struct{
	char *playlist[6][32];
	int trackLength[6][32];
	qboolean hasPlayed[6][32];
	qboolean random;
	int playlistSize[6];
	int currentType;
	int currentIndex;
	int fadeAmount;
	int endTime;
	int volume;
}musicSystem;
typedef enum {
	battle,
	idle,
	struggle,
	losing,
	winning,
	profile
}trackTypes;
void CG_CheckMusic(void);
void CG_StartMusic(void);
void CG_NextTrack(void);
