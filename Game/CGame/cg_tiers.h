//
// cg_tiers.h -- client tier system headers

#define TIERNAMELENGTH 15

typedef struct{
	char name[TIERNAMELENGTH];
	char transformMusic[MAX_QPATH];
	qhandle_t crosshair;
	qhandle_t crosshairPowering;
	qhandle_t screenEffect[10];
	qhandle_t screenEffectPowering;
	qhandle_t screenEffectTransforming;
	qhandle_t icon2D[10];
	qhandle_t icon2DPowering;
	qhandle_t icon2DTransforming;
	qboolean damageFeatures;
	qboolean damageModelsRevertHealed;
	qboolean damageTexturesRevertHealed;
	qboolean transformScriptExists;
	float meshScale;
	float icon3DZoom;
	float hudMultiplier;
	int icon3DOffset[2];
	int icon3DRotation[3];
	int icon3DSize[2];
	int cameraOffset[3];
	int meshOffset;
	int sustainCurrent;
	int sustainCurrentPercent;
	int sustainFatigue;
	int sustainHealth;
	int sustainMaximum;
	int requirementCurrent;
	int requirementCurrentPercent;
	int requirementFatigue;
	int requirementHealth;
	int requirementHealthMaximum;
	int requirementMaximum;
	int transformMusicLength;
	sfxHandle_t soundTransformFirst;
	sfxHandle_t soundTransformUp;
	sfxHandle_t soundTransformDown;
	sfxHandle_t soundPoweringUp;
}tierConfig_cg;

