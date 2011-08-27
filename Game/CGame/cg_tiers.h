#define TIERNAMELENGTH 15

typedef struct{
	char name[TIERNAMELENGTH];
	char transformMusic[MAX_QPATH];
	qhandle_t icon;
	qboolean damageFeatures;
	qboolean damageModelsRevertHealed;
	qboolean damageTexturesRevertHealed;
	qboolean transformScriptExists;
	float hudMultiplier;
	int sustainCurrent;
	int sustainCurrentPct;
	int sustainFatigue;
	int sustainHealth;
	int sustainMaximum;
	int requirementCurrent;
	int requirementCurrentPct;
	int requirementFatigue;
	int requirementHealth;
	int requirementHealthMaximum;
	int requirementMaximum;
	int transformCameraDefault[3];
	int transformCameraOrbit[2];
	int transformCameraZoom[2];
	int transformCameraPan[2];
	int transformMusicLength;
	sfxHandle_t soundTransformFirst;
	sfxHandle_t soundTransformUp;
	sfxHandle_t soundTransformDown;
	sfxHandle_t soundPoweringUp;
}tierConfig_cg;
