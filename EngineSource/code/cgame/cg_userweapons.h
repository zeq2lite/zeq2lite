#define MAX_CHARGES			 2
#define MAX_TAGNAME			20
#define MAX_WEAPONNAME		40
#define MAX_CHARGE_VOICES	 6


typedef struct {
	sfxHandle_t		voice;
	float			startPct;
} chargeVoice_t;

typedef enum {
	MRK_NONE,
	MRK_BURN,
	MRK_CRATER	
} markType_t;



// NOTE: In cg_userWeapon_t the trailFunc and particleFuncs have an int
//       for an argument to make sure things compile. (centity_t* is unknown
//       at this point.) So, we pass the clientside entity number to the function,
//       then retrieve a pointer to work with in there.
typedef struct {

	// CHARGING
	qhandle_t		chargeModel;			// charge model's .md3 file
	qhandle_t		chargeSkin;				// charge model's .skin file
	qhandle_t		chargeShader;			// charge model's shader (for sprites only!)

	float			chargeSpinYaw;			// spin the model during the charge?
	
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

	void			(*chargeParticleFunc)( int );
											// function with behaviour of particles
	float			chargeParticleRadius;	// radius of particles
	float			chargeParticleTime;		// existance time of particles
	float			chargeParticleShader;	// shader of particles

		
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
	sfxHandle_t		flashVoice[4];			// if more than one is specified, a random one	
											// is chosen. (Breaks repetitiveness for
											// fastfiring weapons)
	qboolean		repeatFlashVoice;		// whether to play the voice only once for each
											// volley, or for every shot in each volley

	// MISSILE
	qhandle_t		missileModel;
	qhandle_t		missileSkin;
	qhandle_t		missileShader;

	float			missileSize;			// If the weapon was charged, multiply with chargeSize;
	
	qboolean		missileSpinYaw;

	float			missileDlightRadius;
	vec3_t			missileDlightColor;
	
	void			(*missileTrailFunc)( int );
	float			missileTrailRadius;
	qhandle_t		missileTrailShader;
	qhandle_t		missileTrailSpiralShader;
	float			missileTrailSpiralRadius;

	void			(*missileParticleFunc)( int );	//  }
	float			missileParticleRadius;					//  } 
	float			missileParticleTime;					//  } --> used for torch attacks
	float			missileParticleShader;					//  }     as well
	float			missileParticleSpeed;					//  }

	sfxHandle_t		missileSound;
	sfxHandle_t		firingSound;


	// HITSCAN PARTICULAR
	qboolean		hasRailTrail;
	vec3_t			railTrailColor;


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

	float			smokeDuration;
	int				smokeAmount;
	float			smokeSize;
	float			smokeRadius;
	markType_t		markType;

	sfxHandle_t		explosionSound[4];		// if more than one is specified, a random one	
											// is chosen. (Breaks repetitiveness for
											// fastfiring weapons)
	// HUD
	qhandle_t		weaponIcon;
	char			weaponName[MAX_WEAPONNAME];			

} cg_userWeapon_t;

cg_userWeapon_t *CG_FindUserWeaponGraphics( int clientNum, int index );
void CG_ParseWeaponList( void );



// Trail functions from cg_ents.c
void CG_TrailFunc_FadeTail( int number );
void CG_TrailFunc_StraightBeam( int number );
void CG_TrailFunc_SpiralBeam( int number );
void CG_TrailFunc_BendyBeam( int number );
