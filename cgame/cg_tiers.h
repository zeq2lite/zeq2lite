typedef struct{
	char *name;
	qhandle_t icon;
	float hudMultiplier;
	int transformCameraDefault[3];
	int transformCameraOrbit[2];
	int transformCameraZoom[2];
	int transformCameraPan[2];
	sfxHandle_t soundTransformFirst;
	sfxHandle_t soundTransformUp;
	sfxHandle_t soundTransformDown;
}tierConfig_t;
