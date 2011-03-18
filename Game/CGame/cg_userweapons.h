#define MAX_CHARGES			 2
#define MAX_TAGNAME			20
#define MAX_WEAPONNAME		40
#define MAX_CHARGE_VOICES	 6
typedef struct {
	sfxHandle_t		voice;
	float			startPct;
} chargeVoice_t;


// NOTE: In cg_userWeapon_t the trailFunc and particleFuncs have an int
//       for an argument to make sure things compile. (centity_t* is unknown
//       at this point.) So, we pass the clientside entity number to the function,
//       then retrieve a pointer to work with in there.
typedef struct {
	//lerpFrame		chargeAnimation,flashAnimation,missileAnimation,explosionAnimation,shockwaveAnimation;
	// CHARGING
	qhandle_t		chargeModel;			// charge model's .md3 file
	qhandle_t		chargeSkin;				// charge model's .skin file
	qhandle_t		chargeShader;			// charge model's shader (for sprites only!)
	vec3_t			chargeSpin;				// spin the model during the charge?
	qboolean		chargeGrowth;			// does the charge grow over time?
	int				chargeEndPct;			// percentage at which growth stops
	int				chargeStartPct;			// percentage at which charge becomes visible
											// and growth starts	
	float			chargeEndsize;			// ending size for growing attacks
	float			chargeStartsize;		// starting size for growing attacks, just size
											// for non-growing
	float			chargeDlightStartRadius;// radius of light and starting radius
	float			chargeDlightEndRadius;	// ending radius of light
	vec3_t			chargeDlightColor;		// color of light
	char			chargeTag[MAX_CHARGES][MAX_TAGNAME];
											// the names of the player model tags on which to
											// place an instance of the charge
	chargeVoice_t	chargeVoice[MAX_CHARGE_VOICES];
											// voice samples played back when charging
	sfxHandle_t		chargeLoopSound;		// sound played while charging
	char			chargeParticleSystem[MAX_QPATH];
	// FLASH
	qhandle_t		flashModel;				// flash model's .md3 file
	qhandle_t		flashSkin;				// flash model's .skin file
	qhandle_t		flashShader;			// flash model's shader (for sprites only!) 
	float			flashSize;				// If the weapon was charged, multiply with chargeSize;
	float			flashDlightRadius;		// radius of light
	vec3_t			flashDlightColor;		// color of light
	sfxHandle_t		flashSound[4];			// if more than one is specified, a random one	
											// is chosen. (Breaks repetitiveness for
											// fastfiring weapons)
	sfxHandle_t		voiceSound[4];
	sfxHandle_t		flashOnceSound;			// Played only at the start of a firing session, instead
											// of with each projectile. Resets when attack button comes up.
	sfxHandle_t		firingSound;			// When doing a sustained blast
	char			flashParticleSystem[MAX_QPATH];
	char			firingParticleSystem[MAX_QPATH];
	// MISSILE
	qhandle_t		missileModel;
	qhandle_t		missileSkin;
	qhandle_t		missileShader;
	// MISSILE STRUGGLE
	qhandle_t		missileStruggleModel;
	qhandle_t		missileStruggleSkin;
	qhandle_t		missileStruggleShader;
	float			missileSize;			// If the weapon was charged, multiply with chargeSize;
	vec3_t			missileSpin;			// Spin the missile during flight
	float			missileDlightRadius;
	vec3_t			missileDlightColor;
	float			missileTrailRadius;
	qhandle_t		missileTrailShader;
	qhandle_t		missileTrailSpiralShader;
	float			missileTrailSpiralRadius;
	float			missileTrailSpiralOffset;
	char			missileParticleSystem[MAX_QPATH];
	sfxHandle_t		missileSound;
	// EXPLOSION / SHIELD
	qhandle_t		explosionModel;
	qhandle_t		explosionSkin;
	qhandle_t		explosionShader;
	int				explosionTime;
	float			explosionSize;			// If the weapon was charged, multiply with chargeSize;
	float			explosionDlightRadius;
	vec3_t			explosionDlightColor;
	qhandle_t		shockwaveModel;
	qhandle_t		shockwaveSkin;
	char			explosionParticleSystem[MAX_QPATH];
	char			smokeParticleSystem[MAX_QPATH];
	qhandle_t		markShader;
	qhandle_t		markSize;
	qboolean		noRockDebris;

	sfxHandle_t		explosionSound[4];		// if more than one is specified, a random one	
											// is chosen. (Breaks repetitiveness for
											// fastfiring weapons)
	// HUD
	qhandle_t		weaponIcon;
	char			weaponName[MAX_WEAPONNAME];			

} cg_userWeapon_t;

cg_userWeapon_t *CG_FindUserWeaponGraphics( int clientNum, int index );
void CG_CopyUserWeaponGraphics( int from, int to );


// cg_userWeaponParseBuffer is used by cg_weapGfxParser.
// Instead of qhandle_t, we will store filenames as char[].
// This means we won't have to cache models and sounds that
// will be overrided in inheritance.

typedef struct {
	char			voice[MAX_QPATH];
	float			startPct;
} chargeVoiceParseBuffer_t;

typedef struct {

	// CHARGING
	char			chargeModel[MAX_QPATH];	// charge model's .md3 file
	char			chargeSkin[MAX_QPATH];	// charge model's .skin file
	char			chargeShader[MAX_QPATH];// charge model's shader (for sprites only!)

	vec3_t			chargeSpin;
	
	qboolean		chargeGrowth;			// does the charge grow over time?
	int				chargeEndPct;			// percentage at which growth stops
	int				chargeStartPct;			// percentage at which charge becomes visible
											// and growth starts	
	float			chargeEndsize;			// ending size for growing attacks
	float			chargeStartsize;		// starting size for growing attacks, just size
											// for non-growing
	
	float			chargeDlightStartRadius;// radius of light and starting radius
	float			chargeDlightEndRadius;	// ending radius of light
	vec3_t			chargeDlightColor;		// color of light

	char			chargeTag[MAX_CHARGES][MAX_TAGNAME];
											// the names of the player model tags on which to
											// place an instance of the charge
	
	chargeVoiceParseBuffer_t	chargeVoice[MAX_CHARGE_VOICES];
											// voice samples played back when charging
	char			chargeLoopSound[MAX_QPATH];	// sound played while charging
	char			chargeParticleSystem[MAX_QPATH];

		
	// FLASH
	char			flashModel[MAX_QPATH];	// flash model's .md3 file
	char			flashSkin[MAX_QPATH];	// flash model's .skin file
	char			flashShader[MAX_QPATH];	// flash model's shader (for sprites only!) 

	float			flashSize;				// If the weapon was charged, multiply with chargeSize;
	
	float			flashDlightRadius;		// radius of light
	vec3_t			flashDlightColor;		// color of light
	
	char			flashSound[4][MAX_QPATH];	// if more than one is specified, a random one	
												// is chosen. (Breaks repetitiveness for
												// fastfiring weapons)
	char			voiceSound[4][MAX_QPATH];
	char			flashOnceSound[MAX_QPATH];	// Played only at the start of a firing session, instead
												// of with each projectile. Resets when attack button comes up.
	char			firingSound[MAX_QPATH];		// When doing a sustained blast

	char			flashParticleSystem[MAX_QPATH];
	char			firingParticleSystem[MAX_QPATH];

	// MISSILE
	char			missileModel[MAX_QPATH];
	char			missileSkin[MAX_QPATH];
	char			missileShader[MAX_QPATH];

	// MISSILE STRUGGLE
	char			missileStruggleModel[MAX_QPATH];
	char			missileStruggleSkin[MAX_QPATH];
	char			missileStruggleShader[MAX_QPATH];

	float			missileSize;			// If the weapon was charged, multiply with chargeSize;
	
	vec3_t			missileSpin;			// Spin the missile during flight
	
	float			missileDlightRadius;
	vec3_t			missileDlightColor;
	
	float			missileTrailRadius;
	char			missileTrailShader[MAX_QPATH];
	char			missileTrailSpiralShader[MAX_QPATH];
	float			missileTrailSpiralRadius;
	float			missileTrailSpiralOffset;

	char			missileParticleSystem[MAX_QPATH];

	char			missileSound[MAX_QPATH];
	

	// EXPLOSION / SHIELD
	char			explosionModel[MAX_QPATH];
	char			explosionSkin[MAX_QPATH];
	char			explosionShader[MAX_QPATH];
	int				explosionTime;

	float			explosionSize;			// If the weapon was charged, multiply with chargeSize;

	float			explosionDlightRadius;
	vec3_t			explosionDlightColor;

	char			shockwaveModel[MAX_QPATH];
	char			shockwaveSkin[MAX_QPATH];

	char			explosionParticleSystem[MAX_QPATH];
	char			smokeParticleSystem[MAX_QPATH];
	char			markShader[MAX_QPATH];
	qhandle_t		markSize;
	qboolean		noRockDebris;

	char			explosionSound[4][MAX_QPATH];	// if more than one is specified, a random one	
													// is chosen. (Breaks repetitiveness for
													// fastfiring weapons)
	// HUD
	char			weaponIcon[MAX_QPATH];
	char			weaponName[MAX_WEAPONNAME];			

} cg_userWeaponParseBuffer_t;
