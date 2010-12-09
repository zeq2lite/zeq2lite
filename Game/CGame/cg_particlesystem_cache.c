// Copyright (C) 2003-2004 RiO
//
// cg_particlesystem_cache.c -- Contains caching and parsing functions for particle systems.

#include "cg_local.h"
#include "cg_particlesystem.h"

#define MAX_CACHED_SYSTEMS	1024	// A maximum of 1024 different particle systems can be kept in cache.
#define MAX_PSYS_FILELEN	32000	// slightly below 32k, which is the maximum size of a local variable in VMs

static PSys_SystemTemplate_t	PSys_Cache[MAX_CACHED_SYSTEMS];
static int						PSys_CurCacheSize;


static qboolean PSys_ParseVector( char **text_pp, int x, float *m, qboolean normalized ) {
	char	*token;
	int		i;

	token = COM_Parse( text_pp );
	if ( !token[0] ) {
		CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
		return qfalse;
	}
	if ( Q_stricmp( token, "(" ) ) {
		CG_Printf( S_COLOR_RED "ERROR: vector expected\n" );
		return qfalse;
	}

	for (i = 0 ; i < x ; i++) {
		token = COM_Parse( text_pp );
		if ( !token[0] ) {
			CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
			return qfalse;
		}
		m[i] = atof(token);
		if ( normalized && ( m[i] > 1 || m[i] < 0)) {
			CG_Printf( S_COLOR_YELLOW "WARNING: expected normalized value, clamped to 1\n" );
			m[i] = 1;
		}
	}

	token = COM_Parse( text_pp );
	if ( !token[0] ) {
		CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
		return qfalse;
	}
	if ( Q_stricmp( token, ")" ) ) {
		CG_Printf( S_COLOR_RED "ERROR: vector not closed correctly\n" );
		return qfalse;
	}

	return qtrue;
}

static qboolean PSys_ParseParticleTemplate( char **text_pp, PSys_ParticleTemplate_t *cachePtcl ) {
	char	*token;

	// parse the text
	while (1) {
		token = COM_Parse( text_pp );

		if ( !token[0] ) {
			CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
			break;

		} else if ( !Q_stricmp( token, "type" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "regular" )) {
				cachePtcl->rType = RTYPE_DEFAULT;
			} else if ( !Q_stricmp( token, "spark" )) {
				cachePtcl->rType = RTYPE_SPARK;
			} else if ( !Q_stricmp( token, "ray" )) {
				cachePtcl->rType = RTYPE_RAY;
			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of particle\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "map" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( !(cachePtcl->shader = trap_R_RegisterShader( token ))) {
				CG_Printf( S_COLOR_YELLOW "WARNING: '%s': could not register\n", token );
			}
			
		} else if ( !Q_stricmp( token, "model" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( !(cachePtcl->model = trap_R_RegisterModel( token ))) {
				CG_Printf( S_COLOR_YELLOW "WARNING:'%s': could not register\n", token );
			}

		} else if ( !Q_stricmp( token, "scale" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->scale.midVal = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cachePtcl->scale.midVal = 0;
			}

		} else if ( !Q_stricmp( token, "inScale" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->scale.startVal = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cachePtcl->scale.startVal = 0;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->scale.startTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->scale.startTime = 0;
			}

		} else if ( !Q_stricmp( token, "outScale" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->scale.endVal = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cachePtcl->scale.endVal = 0;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->scale.endTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->scale.endTime = 0;
			}

		} else if ( !Q_stricmp( token, "rgba" )) {
			if ( !PSys_ParseVector( text_pp, 4, cachePtcl->rgba.midVal, qtrue )) {
				return qfalse;
			}
			cachePtcl->rgba.midVal[0] *= 255;
			cachePtcl->rgba.midVal[1] *= 255;
			cachePtcl->rgba.midVal[2] *= 255;
			cachePtcl->rgba.midVal[3] *= 255;

		} else if ( !Q_stricmp( token, "inRgba" )) {
			if ( !PSys_ParseVector( text_pp, 4, cachePtcl->rgba.startVal, qtrue )) {
				return qfalse;
			}
			cachePtcl->rgba.startVal[0] *= 255;
			cachePtcl->rgba.startVal[1] *= 255;
			cachePtcl->rgba.startVal[2] *= 255;
			cachePtcl->rgba.startVal[3] *= 255;

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->rgba.startTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->rgba.startTime = 0;
			}

		} else if ( !Q_stricmp( token, "outRgba" )) {
			if ( !PSys_ParseVector( text_pp, 4, cachePtcl->rgba.endVal, qtrue )) {
				return qfalse;
			}
			cachePtcl->rgba.endVal[0] *= 255;
			cachePtcl->rgba.endVal[1] *= 255;
			cachePtcl->rgba.endVal[2] *= 255;
			cachePtcl->rgba.endVal[3] *= 255;

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->rgba.endTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->rgba.endTime = 0;
			}

		} else if ( !Q_stricmp( token, "rotate" )) {
			if ( !PSys_ParseVector( text_pp, 4, cachePtcl->rotate.midVal, qtrue )) {
				return qfalse;
			}
			cachePtcl->rotate.midVal[0] *= 255;
			cachePtcl->rotate.midVal[1] *= 255;
			cachePtcl->rotate.midVal[2] *= 255;
			cachePtcl->rotate.midVal[3] *= 255;

		} else if ( !Q_stricmp( token, "inRotate" )) {
			if ( !PSys_ParseVector( text_pp, 4, cachePtcl->rotate.startVal, qtrue )) {
				return qfalse;
			}
			cachePtcl->rotate.startVal[0] *= 255;
			cachePtcl->rotate.startVal[1] *= 255;
			cachePtcl->rotate.startVal[2] *= 255;
			cachePtcl->rotate.startVal[3] *= 255;

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->rotate.startTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->rotate.startTime = 0;
			}

		} else if ( !Q_stricmp( token, "outRotate" )) {
			if ( !PSys_ParseVector( text_pp, 4, cachePtcl->rotate.endVal, qtrue )) {
				return qfalse;
			}
			cachePtcl->rotate.endVal[0] *= 255;
			cachePtcl->rotate.endVal[1] *= 255;
			cachePtcl->rotate.endVal[2] *= 255;
			cachePtcl->rotate.endVal[3] *= 255;

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->rotate.endTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->rotate.endTime = 0;
			}

		} else if ( !Q_stricmp( token, "life" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->lifeTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->lifeTime = 0;
			}

		} else if ( !Q_stricmp( token, "mass" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cachePtcl->mass = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cachePtcl->mass = 0;
			}

		} else if ( !Q_stricmp( token, "speed" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			cachePtcl->speed = atoi(token);

		} else if ( !Q_stricmp( token, "}" )) {
			return qtrue;

		} else {
			CG_Printf( S_COLOR_RED "ERROR: '%s': not a valid keyword\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

static qboolean PSys_ParseEmitter( char **text_pp, PSys_Emitter_t *cacheEmit ) {
	char		*token;
	qboolean	ptclKey;

	// parse the text
	cacheEmit->nrTemplates = 0;
	ptclKey = qfalse;

	while (1) {
		token = COM_Parse( text_pp );

		if ( !token[0] ) {
			CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
			break;

		} else if ( !Q_stricmp( token, "{" )) {
			if ( !ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: template not properly started with 'Particle' keyword\n" );

				if ( cacheEmit->nrTemplates == MAX_PARTICLE_TEMPLATES ) {
					CG_Printf( S_COLOR_RED "ERROR: maximum number of particle templates (%i) reached\n", MAX_PARTICLE_TEMPLATES );
					return qfalse;
				}
			}

			// Parse a new particle template
			if ( !PSys_ParseParticleTemplate( text_pp, &(cacheEmit->particleTemplates[cacheEmit->nrTemplates])) ) {
				CG_Printf( S_COLOR_RED "ERROR: failed to parse particle template\n" );
				return qfalse;
			}

			// Prepare for a new template
			cacheEmit->nrTemplates++;
			ptclKey = qfalse;

		} else if ( !Q_stricmp( token, "particle" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_RED "ERROR: header for particle template already present\n" );
				return qfalse;
			}

			if ( cacheEmit->nrTemplates == MAX_PARTICLE_TEMPLATES ) {
				CG_Printf( S_COLOR_RED "ERROR: maximum number of particle templates (%i) reached\n", MAX_PARTICLE_TEMPLATES );
				return qfalse;
			}

			ptclKey = qtrue;

		} else if ( !Q_stricmp( token, "life" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheEmit->lifeTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cacheEmit->lifeTime = 0;
			}

		} else if ( !Q_stricmp( token, "type" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "point" )) {
				cacheEmit->type = ETYPE_POINT;
			} else if ( !Q_stricmp( token, "radius" )) {
				cacheEmit->type = ETYPE_RADIUS;
			} else if ( !Q_stricmp( token, "sphere" )) {
				cacheEmit->type = ETYPE_SPHERE;
			} else if ( !Q_stricmp( token, "pointGround" )) {
				cacheEmit->type = ETYPE_POINT_SURFACE;
			} else if ( !Q_stricmp( token, "radiusGround" )) {
				cacheEmit->type = ETYPE_RADIUS_SURFACE;
			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of emitter\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "count" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			cacheEmit->amount = atoi(token);
			if ( cacheEmit->amount < 1 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: non-zero positive int required, clamped to 1\n" );
				cacheEmit->amount = 1;
			}

		} else if ( !Q_stricmp( token, "period" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheEmit->waitTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cacheEmit->waitTime = 0;
			}

		} else if ( !Q_stricmp( token, "delay" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheEmit->initTime = atoi(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative int not allowed, clamped to 0\n" );
				cacheEmit->initTime = 0;
			}

		} else if ( !Q_stricmp( token, "radius" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheEmit->radius = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cacheEmit->radius = 0;
			}

		} else if ( !Q_stricmp( token, "grndDist" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheEmit->grndDist = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cacheEmit->grndDist = 0;
			}

		} else if ( !Q_stricmp( token, "slide" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			cacheEmit->offset = atof(token);

		} else if ( !Q_stricmp( token, "jitter" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheEmit->posJit = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cacheEmit->posJit = 0;
			}

		} else if ( !Q_stricmp( token, "link" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "root" )) {
				cacheEmit->orientation.autoLink = qfalse;
				cacheEmit->orientation.entityLink = qfalse;
				cacheEmit->orientation.tagName[0] = 0;
			} else if ( !Q_stricmp( token, "entity" )) {
				cacheEmit->orientation.entityLink = qtrue;
				cacheEmit->orientation.autoLink = qfalse;
				cacheEmit->orientation.tagName[0] = 0;

			} else if ( !Q_stricmp( token, "tag" )) {
				cacheEmit->orientation.entityLink = qtrue;
				cacheEmit->orientation.autoLink = qfalse;
				
				token = COM_Parse( text_pp );

				if ( !token[0] ) {
					CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
					return qfalse;
				}

				Q_strncpyz( cacheEmit->orientation.tagName, token, MAX_QPATH );

			} else if ( !Q_stricmp( token, "auto" )) {
				cacheEmit->orientation.autoLink = qtrue;
				cacheEmit->orientation.entityLink = qfalse;
				cacheEmit->orientation.tagName[0] = 0;
			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of link\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "offset" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}
			if (!PSys_ParseVector( text_pp, 3, cacheEmit->orientation.offset, qfalse )) {
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "dir" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
				ptclKey = qfalse;
			}
			if (!PSys_ParseVector( text_pp, 3, cacheEmit->orientation.dir, qfalse )) {
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "}" )) {
			if ( ptclKey ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty particle declaration\n" );
			}
			return qtrue;

		} else {
			CG_Printf( S_COLOR_RED "ERROR: '%s': not a valid keyword\n", token );
			return qfalse;
		}

	}

	return qfalse;
}

static qboolean PSys_ParseForce( char **text_pp, PSys_Force_t *cacheForce ) {
	char	*token;

	while (1) {
		token = COM_Parse( text_pp );

		if ( !token[0] ) {
			CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
			break;
		
		} else if ( !Q_stricmp( token, "type" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "directional" )) {
				cacheForce->type = FTYPE_DIRECTIONAL;
			} else if ( !Q_stricmp( token, "spherical" )) {
				cacheForce->type = FTYPE_SPHERICAL;
			} else if ( !Q_stricmp( token, "drag" )) {
				cacheForce->type = FTYPE_DRAG;
			} else if ( !Q_stricmp( token, "spin" )) {
				cacheForce->type = FTYPE_SWIRL;
			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of force\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "area" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "infinite" )) {
				cacheForce->AOItype = AOI_INFINITE;
			} else if ( !Q_stricmp( token, "sphere" )) {
				cacheForce->AOItype = AOI_SPHERE;

				token = COM_Parse( text_pp );

				if ( !token[0] ) {
					CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
					return qfalse;
				}

				if ( (cacheForce->AOIrange[0] = atof(token)) < 0 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
					cacheForce->AOIrange[0] = 0;
				}

			} else if ( !Q_stricmp( token, "cylinder" )) {
				cacheForce->AOItype = AOI_CYLINDER;

				token = COM_Parse( text_pp );

				if ( !token[0] ) {
					CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
					return qfalse;
				}

				if ( (cacheForce->AOIrange[0] = atof(token)) < 0 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
					cacheForce->AOIrange[0] = 0;
				}

				token = COM_Parse( text_pp );

				if ( !token[0] ) {
					CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
					return qfalse;
				}

				if ( (cacheForce->AOIrange[1] = atof(token)) < 0 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
					cacheForce->AOIrange[1] = 0;
				}

			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of area\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "falloff" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheForce->falloff = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cacheForce->falloff = 0;
			}

		} else if ( !Q_stricmp( token, "amount" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			cacheForce->value = atof(token);

		} else if ( !Q_stricmp( token, "centripetal" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( (cacheForce->pullIn = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cacheForce->pullIn = 0;
			}
			
		} else if ( !Q_stricmp( token, "link" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "root" )) {
				cacheForce->orientation.autoLink = qfalse;
				cacheForce->orientation.entityLink = qfalse;
				cacheForce->orientation.tagName[0] = 0;

			} else if ( !Q_stricmp( token, "entity" )) {
				cacheForce->orientation.entityLink = qtrue;
				cacheForce->orientation.autoLink = qfalse;
				cacheForce->orientation.tagName[0] = 0;

			} else if ( !Q_stricmp( token, "tag" )) {
				cacheForce->orientation.entityLink = qtrue;
				cacheForce->orientation.autoLink = qfalse;
				
				token = COM_Parse( text_pp );

				if ( !token[0] ) {
					CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
					return qfalse;
				}

				Q_strncpyz( cacheForce->orientation.tagName, token, MAX_QPATH );

			} else if ( !Q_stricmp( token, "auto" )) {
				cacheForce->orientation.autoLink = qtrue;
				cacheForce->orientation.entityLink = qfalse;
				cacheForce->orientation.tagName[0] = 0;
			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of link\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "offset" )) {
			if (!PSys_ParseVector( text_pp, 3, cacheForce->orientation.offset, qfalse )) {
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "dir" )) {
			if (!PSys_ParseVector( text_pp, 3, cacheForce->orientation.dir, qfalse )) {
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "}" )) {
			return qtrue;

		} else {
			CG_Printf( S_COLOR_RED "ERROR: '%s': not a valid keyword\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

static qboolean PSys_ParseConstraint( char **text_pp, PSys_Constraint_t *cacheConst ) {
	char	*token;

	while (1) {
		token = COM_Parse( text_pp );

		if ( !token[0] ) {
			CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
			break;

		} else if ( !Q_stricmp( token, "type" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			if ( !Q_stricmp( token, "plane" )) {
				cacheConst->type = CTYPE_PLANE;
			} else if ( !Q_stricmp( token, "maxDistance" )) {
				cacheConst->type = CTYPE_DISTANCE_MAX;
			} else if ( !Q_stricmp( token, "minDistance" )) {
				cacheConst->type = CTYPE_DISTANCE_MIN;
			} else if ( !Q_stricmp( token, "exactDistance" )) {
				cacheConst->type = CTYPE_DISTANCE_EXACT;
			} else {
				CG_Printf( S_COLOR_RED "ERROR: '%s': unknown type of constraint\n", token );
				return qfalse;
			}

		} else if ( !Q_stricmp( token, "value" )) {
			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: Unexpected end of file\n" );
				return qfalse;
			}

			cacheConst->value = atof(token);

		} else if ( !Q_stricmp( token, "}" )) {
			return qtrue;

		} else {
			CG_Printf( S_COLOR_RED "ERROR: '%s': not a valid keyword\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

static qboolean PSys_ParseSystem( char **text_pp, PSys_SystemTemplate_t *cacheSys ) {
	char				*token;
	PSys_MemberType_t	memberType;
	int					num;

	// parse the text
	memberType = MEM_NONE;
	num = 0;

	while (1) {
		token = COM_Parse( text_pp );

		if ( !token[0] ) {
			CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
			break;

		} else if ( !Q_stricmp( token, "gravity" )) {
			if ( memberType != MEM_NONE ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty member declaration\n" );
				memberType = MEM_NONE;
			}

			token = COM_Parse( text_pp );

			if ( !token[0] ) {
				CG_Printf( S_COLOR_RED "ERROR: unexpected end of file\n" );
				break;
			}

			if ( (cacheSys->gravity = atof(token)) < 0 ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: negative float not allowed, clamped to 0\n" );
				cacheSys->gravity = 0;
			}
			
		} else if ( !Q_stricmp( token, "{" )) {

			cacheSys->members[num].type = memberType;

			// Typecheck and parse the new member
			switch ( memberType ) {
			case MEM_EMITTER:
				if ( !PSys_ParseEmitter( text_pp, &(cacheSys->members[num].data.emitter)) ) {
					CG_Printf( S_COLOR_RED "ERROR: failed to parse emitter\n" );
					return qfalse;
				}
				break;

			case MEM_FORCE:
				if ( !PSys_ParseForce( text_pp, &(cacheSys->members[num].data.force)) ) {
					CG_Printf( S_COLOR_RED "ERROR: failed to parse force\n" );
					return qfalse;
				}
				break;

			case MEM_CONSTRAINT:
				if ( !PSys_ParseConstraint( text_pp, &(cacheSys->members[num].data.constraint)) ) {
					CG_Printf( S_COLOR_RED "ERROR: failed to parse constraint\n" );
					return qfalse;
				}
				break;

			default:
				CG_Printf( S_COLOR_RED "ERROR: member type unspecified\n" );
				return qfalse;
				break;
			}

			// Prepare for a new member
			memberType = MEM_NONE;
			num++;

		} else if ( !Q_stricmp( token, "emitter" )) {
			if ( num == MAX_PARTICLESYSTEM_MEMBERS ) {
				CG_Printf( S_COLOR_RED "ERROR: maximum number of members (%i) reached\n", MAX_PARTICLESYSTEM_MEMBERS );
				return qfalse;
			}

			if ( memberType != MEM_NONE ) {
				CG_Printf( S_COLOR_RED "ERROR: trying to redefine member type\n" );
				return qfalse;
			}

			memberType = MEM_EMITTER;

		} else if ( !Q_stricmp( token, "force" )) {
			if ( num == MAX_PARTICLESYSTEM_MEMBERS ) {
				CG_Printf( S_COLOR_RED "ERROR: maximum number of members (%i) reached\n", MAX_PARTICLESYSTEM_MEMBERS );
				return qfalse;
			}

			if ( memberType != MEM_NONE ) {
				CG_Printf( S_COLOR_RED "ERROR: trying to redefine member type\n" );
				return qfalse;
			}

			memberType = MEM_FORCE;

		} else if ( !Q_stricmp( token, "constraint" )) {
			if ( num == MAX_PARTICLESYSTEM_MEMBERS ) {
				CG_Printf( S_COLOR_RED "ERROR: maximum number of members (%i) reached\n", MAX_PARTICLESYSTEM_MEMBERS );
				return qfalse;
			}

			if ( memberType != MEM_NONE ) {
				CG_Printf( S_COLOR_RED "ERROR: trying to redefine member type\n" );
				return qfalse;
			}

			memberType = MEM_CONSTRAINT;

		} else if ( !Q_stricmp( token, "}" )) {
			if ( memberType != MEM_NONE ) {
				CG_Printf( S_COLOR_YELLOW "WARNING: empty member declaration\n" );
			}
			return qtrue;

		} else {
			CG_Printf( S_COLOR_RED "ERROR: '%s': not a valid keyword\n", token );
			return qfalse;
		}
	}

	return qfalse;
}

static void PSys_ParseFile( char *filename, int *num ) {
	fileHandle_t	file;
	int				len;
	char			text[MAX_PSYS_FILELEN];
	char			*text_p;
	char			*token;

	char			sysName[MAX_QPATH];
	qboolean		isNamed;


	// Try to open the file
	len = trap_FS_FOpenFile( filename, &file, FS_READ );
	if ( !file ) {
		return;
	}

	// Feedback which file is being loaded here, after the check for its existence.
	// Prevents inexistent files from showing up as being loaded.
	CG_Printf("...loading '%s'\n", filename );

	// If the file is too long to fit into the buffer, dump an error and abort reading
	// the file.
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( S_COLOR_RED "ERROR: file too large: %s is %i, max allowed is %i", filename, len, MAX_PSYS_FILELEN );
		trap_FS_FCloseFile( file );
		return;
	}

	trap_FS_Read( text, len, file );
	text[len] = 0;
	trap_FS_FCloseFile( file );

	// parse the text
	text_p = text;
	isNamed = qfalse;
	
	while(1) {
		token = COM_Parse( &text_p );

		if (!token[0]) {
			break;

		} else if (!Q_stricmp( token, "{" )) {
			int i;

			// Check if system is named
			if ( !isNamed ) {
				CG_Printf( S_COLOR_RED "ERROR: particle system has no name\n" );
				return;
			}

			// Check for a name clash
			for ( i = 0; i < *num; i++ ) {
				if ( !Q_stricmp( sysName, PSys_Cache[i].name )) {
					CG_Printf( S_COLOR_RED "ERROR: previous particle system named '%s' exists\n", sysName );
					return;
				}
			}

			Q_strncpyz( PSys_Cache[*num].name, sysName, MAX_QPATH );

			// Parse the new system
			if ( !PSys_ParseSystem( &text_p, &(PSys_Cache[*num])) ) {
				
				// Reset the current cache element to be used for the first system
				// in the next valid file.
				memset( &PSys_Cache[*num], 0, sizeof(PSys_SystemTemplate_t));

				CG_Printf( S_COLOR_RED "ERROR: failed to parse particle system '%s'\n", sysName );
				return;
			}

			// Prepare for a new system name
			isNamed = qfalse;
			(*num)++;

		} else if ( !isNamed ) {
			int i;

			// Check if the maximum amount of new systems has already been reached
			if ( *num == MAX_CACHED_SYSTEMS ) {
				CG_Printf( S_COLOR_RED "ERROR: maximum number of particle systems (%i) reached\n", MAX_CACHED_SYSTEMS );
				return;
			}

			// Name the new system
			Q_strncpyz( sysName, token, MAX_QPATH );

			// Scan the new name for any non alpha-numerical characters
			for ( i = 0; sysName[i]; i++ ) {
				if (!((sysName[i] >= 'a') && (sysName[i] <= 'z')) &&
					!((sysName[i] >= 'A') && (sysName[i] <= 'Z')) &&
					!((sysName[i] >= '0') && (sysName[i] <= '9')) ) {

					CG_Printf( S_COLOR_RED "ERROR: illegal character '%c' found in particle system name\n", sysName[i] );
					return;
				}
			}

			isNamed = qtrue;

		} else {
			CG_Printf( S_COLOR_RED "ERROR: particle system already known as '%s'\n", sysName );
			return;
		}
	}

	return;
}


PSys_SystemTemplate_t* PSys_LoadSystemFromCache( char *systemName ) {
	int low, high, mid;

	// No string
	if ( !systemName ) return NULL;

	// Empty string
	if ( !*systemName ) return NULL;

	// Zero systems cached
	if ( !PSys_CurCacheSize ) {
		return NULL;
	}

	// Binary search the cache
	low = 0;
	high = PSys_CurCacheSize;
	while ( low + 1 != high ) {
		mid = ( low + high ) / 2;
		if ( Q_stricmp( PSys_Cache[mid].name, systemName ) < 1  ) low  = mid;
		if ( Q_stricmp( PSys_Cache[mid].name, systemName ) == 1 ) high = mid;
		// if ( PSys_Cache[mid].name <= systemName ) low  = mid;
		// if ( PSys_Cache[mid].name >  systemName ) high = mid;
	}
	if ( Q_stricmp( PSys_Cache[low].name, systemName )) return NULL;
	// if ( PSys_Cache[low].name != systemName ) return NULL;

	// Found the system we were looking for
	return &PSys_Cache[low];
}


void PSys_QSortCache( int lowbound, int highbound ) {
	int						low, high;
	PSys_SystemTemplate_t	mid, tempElem;

	low = lowbound;
	high = highbound;
	mid = PSys_Cache[(low + high) / 2];

	do {
		while ( Q_stricmp( PSys_Cache[low].name, mid.name ) == -1 ) low++;
		while ( Q_stricmp( PSys_Cache[high].name, mid.name ) == 1 ) high--;
		//while ( PSys_Cache[low].name  < mid.name ) low++;
		//while ( PSys_Cache[high].name > mid.name ) high--;

		if ( low <= high ) {
			// swap elements
			tempElem = PSys_Cache[low];
			PSys_Cache[low] = PSys_Cache[high];
			PSys_Cache[high] = tempElem;

			low++;
			high--;
		}

	} while ( !(low > high));

	if ( high > lowbound ) PSys_QSortCache( lowbound, high );
	if ( low < highbound ) PSys_QSortCache( low, highbound );	
}


void PSys_InitCache( void ) {
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	// Feedback start of loading particle systems
	CG_Printf( "\nInitializing Particle Systems\n" );

	// Clear the cache
	memset(PSys_Cache, 0, sizeof(PSys_Cache));
	PSys_CurCacheSize = 0;
	
	// Parse all files fitting the /effects/*.psys pattern
	numdirs = trap_FS_GetFileList("effects", ".psys", dirlist, 1024 );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs; i++, dirptr += dirlen+1 ) {
		dirlen = strlen(dirptr);
		strcpy(filename, "effects/");
		strcat(filename, dirptr);

		PSys_ParseFile(filename, &PSys_CurCacheSize );
	}

	// Quicksort loaded systems to retrieve them more efficiently with a binary search
	// NOTE: Must not give a negative 2nd parameter, or will read out of bounds!
	if ( PSys_CurCacheSize > 1 ) {
		PSys_QSortCache( 0, PSys_CurCacheSize - 1 );
	}

	CG_Printf( "%i Particle Systems Initialized\n\n", PSys_CurCacheSize );
}
