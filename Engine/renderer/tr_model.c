/*
 * tr_models.c
 * Model loading and caching
 * Copyright (C) 2009  Jens Loehr <jloehr85@googlemail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "tr_local.h"

#define	LL(x) x=LittleLong(x)

static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *name );
static qboolean R_LoadMDM( model_t *mod, void *buffer, const char *name );
static qboolean R_LoadMDX( model_t *mod, void *buffer, const char *name );

model_t	*loadmodel;

/*
** R_GetModelByHandle
*/
model_t	*R_GetModelByHandle( qhandle_t index ) {
	model_t		*mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t *R_AllocModel( void ) {
	model_t		*mod;

	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	mod = ri.Hunk_Alloc( sizeof( *tr.models[tr.numModels] ), h_low );
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t RE_RegisterModel( const char *name ) {
	model_t     *mod;
	unsigned    *buf;
	int lod;
	int ident = 0;         // TTimo: init
	qboolean loaded;
	qhandle_t hModel;
	int numLoaded;
	char filename[1024];

	if ( !name || !name[0] ) {
		ri.Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	//
	// search the currently loaded models
	//
	for ( hModel = 1 ; hModel < tr.numModels; hModel++ ) {
		mod = tr.models[hModel];
		if ( !Q_stricmp( mod->name, name ) ) {
			if ( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t

	if ( ( mod = R_AllocModel() ) == NULL ) {
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );


	// make sure the render thread is stopped
	R_SyncRenderThread();

	mod->numLods = 0;

	//
	// load the files
	//
	numLoaded = 0;

	if ( strstr( name, ".mdm" ) || strstr( name, ".mdx" ) ) {    // try loading skeletal file
		loaded = qfalse;
		ri.FS_ReadFile( name, (void **)&buf );
		if ( buf ) {
			loadmodel = mod;

			ident = LittleLong( *(unsigned *)buf );
			if ( ident == MDM_IDENT ) {
				loaded = R_LoadMDM( mod, buf, name );
			} else if ( ident == MDX_IDENT ) {
				loaded = R_LoadMDX( mod, buf, name );
			}

			ri.FS_FreeFile( buf );
		}

		if ( loaded ) {
			return mod->index;
		}
	}

	for ( lod = MD3_MAX_LODS - 1 ; lod >= 0 ; lod-- ) {

		strcpy( filename, name );

		if ( lod != 0 ) {
			char namebuf[80];

			if ( strrchr( filename, '.' ) ) {
				*strrchr( filename, '.' ) = 0;
			}
			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}

		filename[strlen( filename ) - 1] = '3';  // try MD3 second
		ri.FS_ReadFile( filename, (void **)&buf );
		if ( !buf ) {
			continue;
		}

		loadmodel = mod;

		ident = LittleLong( *(unsigned *)buf );
		// Ridah, mesh compression
		if ( ident != MD3_IDENT ) {
			ri.Printf( PRINT_WARNING,"RE_RegisterModel: unknown fileid for %s\n", name );
			goto fail;
		}

		loaded = R_LoadMD3( mod, lod, buf, name );
		// done.

		ri.FS_FreeFile( buf );

		if ( !loaded ) {
			if ( lod == 0 ) {
				goto fail;
			} else {
				break;
			}
		} else {
			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
// Arnout: don't need this anymore,
//			if ( lod <= r_lodbias->integer ) {
//				break;
//			}
		}
	}


	if ( numLoaded ) {
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod-- ; lod >= 0 ; lod-- ) {
			mod->numLods++;
			mod->model.md3[lod] = mod->model.md3[lod + 1];
		}

		return mod->index;
	}

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;
	return 0;
}


/*
=================
R_LoadMD3
=================
*/
static qboolean R_LoadMD3 (model_t *mod, int lod, void *buffer, const char *mod_name ) {
	int					i, j;
	md3Header_t			*pinmodel;
    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	md3Tag_t			*tag;
	int					version;
	int					size;

	pinmodel = (md3Header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MD3_VERSION) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MD3;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	mod->model.md3[lod] = ri.Hunk_Alloc( size, h_low );

	Com_Memcpy (mod->model.md3[lod], buffer, LittleLong(pinmodel->ofsEnd) );

    LL(mod->model.md3[lod]->ident);
    LL(mod->model.md3[lod]->version);
    LL(mod->model.md3[lod]->numFrames);
    LL(mod->model.md3[lod]->numTags);
    LL(mod->model.md3[lod]->numSurfaces);
    LL(mod->model.md3[lod]->ofsFrames);
    LL(mod->model.md3[lod]->ofsTags);
    LL(mod->model.md3[lod]->ofsSurfaces);
    LL(mod->model.md3[lod]->ofsEnd);

	if ( mod->model.md3[lod]->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name );
		return qfalse;
	}
    
	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)mod->model.md3[lod] + mod->model.md3[lod]->ofsFrames );
    for ( i = 0 ; i < mod->model.md3[lod]->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the tags
    tag = (md3Tag_t *) ( (byte *)mod->model.md3[lod] + mod->model.md3[lod]->ofsTags );
    for ( i = 0 ; i < mod->model.md3[lod]->numTags * mod->model.md3[lod]->numFrames ; i++, tag++) {
        for ( j = 0 ; j < 3 ; j++ ) {
			tag->origin[j] = LittleFloat( tag->origin[j] );
			tag->axis[0][j] = LittleFloat( tag->axis[0][j] );
			tag->axis[1][j] = LittleFloat( tag->axis[1][j] );
			tag->axis[2][j] = LittleFloat( tag->axis[2][j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)mod->model.md3[lod] + mod->model.md3[lod]->ofsSurfaces );
	for ( i = 0 ; i < mod->model.md3[lod]->numSurfaces ; i++) {

        LL(surf->ident);
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on a surface (%i).\n",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
			return qfalse;
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i triangles on a surface (%i).\n",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
			return qfalse;
		}
	
		// change to surface identifier
		surf->ident = SF_MD3;

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surf->name );

		// strip off a trailing _1 or _2
		// this is a crutch for q3data being a mess
		j = strlen( surf->name );
		if ( j > 2 && surf->name[j-2] == '_' ) {
			surf->name[j-2] = 0;
		}

        // register the shaders
        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );
        for ( j = 0 ; j < surf->numShaders ; j++, shader++ ) {
            shader_t	*sh;

            sh = R_FindShader( shader->name, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				shader->shaderIndex = 0;
			} else {
				shader->shaderIndex = sh->index;
			}
        }

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}
    
	return qtrue;
}

/*
=================
R_LoadMDM
=================
*/
static qboolean R_LoadMDM( model_t *mod, void *buffer, const char *mod_name ) {
	int i, j, k;
	mdmHeader_t         *pinmodel, *mdm;
//    mdmFrame_t			*frame;
	mdmSurface_t        *surf;
	mdmTriangle_t       *tri;
	mdmVertex_t         *v;
	mdmTag_t            *tag;
	int version;
	int size;
	shader_t            *sh;
	int                 *collapseMap, *boneref;

	pinmodel = (mdmHeader_t *)buffer;

	version = LittleLong( pinmodel->version );
	if ( version != MDM_VERSION ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDM: %s has wrong version (%i should be %i)\n",
				   mod_name, version, MDM_VERSION );
		return qfalse;
	}

	mod->type = MOD_MDM;
	size = LittleLong( pinmodel->ofsEnd );
	mod->dataSize += size;
	mdm = mod->model.mdm = ri.Hunk_Alloc( size, h_low );

	memcpy( mdm, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mdm->ident );
	LL( mdm->version );
//    LL(mdm->numFrames);
	LL( mdm->numTags );
	LL( mdm->numSurfaces );
//    LL(mdm->ofsFrames);
	LL( mdm->ofsTags );
	LL( mdm->ofsEnd );
	LL( mdm->ofsSurfaces );
	mdm->lodBias = LittleFloat( mdm->lodBias );
	mdm->lodScale = LittleFloat( mdm->lodScale );

/*	mdm->skel = RE_RegisterModel(mdm->bonesfile);
	if ( !mdm->skel ) {
		ri.Error (ERR_DROP, "R_LoadMDM: %s skeleton not found\n", mdm->bonesfile );
	}

	if ( mdm->numFrames < 1 ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDM: %s has no frames\n", mod_name );
		return qfalse;
	}*/

	if ( LittleLong( 1 ) != 1 ) {
		// swap all the frames
		/*frameSize = (int) ( sizeof( mdmFrame_t ) );
		for ( i = 0 ; i < mdm->numFrames ; i++, frame++) {
			frame = (mdmFrame_t *) ( (byte *)mdm + mdm->ofsFrames + i * frameSize );
			frame->radius = LittleFloat( frame->radius );
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
				frame->parentOffset[j] = LittleFloat( frame->parentOffset[j] );

		for ( i = 0 ; i < mdr->numFrames ; i++) 
		{
			for(j = 0; j < 3; j++)
			{
				frame->bounds[0][j] = LittleFloat(curframe->bounds[0][j]);
				frame->bounds[1][j] = LittleFloat(curframe->bounds[1][j]);
				frame->localOrigin[j] = LittleFloat(curframe->localOrigin[j]);
			}
			
			frame->radius = LittleFloat(curframe->radius);
			Q_strncpyz(frame->name, curframe->name, sizeof(frame->name));
			
			for (j = 0; j < (int) (mdr->numBones * sizeof(mdrBone_t) / 4); j++) 
			{
				((float *)frame->bones)[j] = LittleFloat( ((float *)curframe->bones)[j] );
			}
			
			curframe = (mdrFrame_t *) &curframe->bones[mdr->numBones];
			frame = (mdrFrame_t *) &frame->bones[mdr->numBones];
		}
	}
	
	// frame should now point to the first free address after all frames.
	lod = (mdrLOD_t *) frame;
	mdr->ofsLODs = (int) ((byte *) lod - (byte *)mdr);
	
	curlod = (mdrLOD_t *)((byte *) pinmodel + LittleLong(pinmodel->ofsLODs));
		
	// swap all the LOD's
	for ( l = 0 ; l < mdr->numLODs ; l++)
	{
		// simple bounds check
		if((byte *) (lod + 1) > (byte *) mdr + size)
		{
			ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
			return qfalse;
		}

		lod->numSurfaces = LittleLong(curlod->numSurfaces);
		
		// swap all the surfaces
		surf = (mdrSurface_t *) (lod + 1);
		lod->ofsSurfaces = (int)((byte *) surf - (byte *) lod);
		cursurf = (mdrSurface_t *) ((byte *)curlod + LittleLong(curlod->ofsSurfaces));
		
		for ( i = 0 ; i < lod->numSurfaces ; i++)
		{
			// simple bounds check
			if((byte *) (surf + 1) > (byte *) mdr + size)
			{
				ri.Printf(PRINT_WARNING, "R_LoadMDR: %s has broken structure.\n", mod_name);
				return qfalse;
			}
		}*/

		// swap all the tags
		tag = ( mdmTag_t * )( (byte *)mdm + mdm->ofsTags );
		for ( i = 0 ; i < mdm->numTags ; i++ ) {
			int ii;
			for ( ii = 0; ii < 3; ii++ )
			{
				tag->axis[ii][0] = LittleFloat( tag->axis[ii][0] );
				tag->axis[ii][1] = LittleFloat( tag->axis[ii][1] );
				tag->axis[ii][2] = LittleFloat( tag->axis[ii][2] );
			}

			LL( tag->boneIndex );
			//tag->torsoWeight = LittleFloat( tag->torsoWeight );
			tag->offset[0] = LittleFloat( tag->offset[0] );
			tag->offset[1] = LittleFloat( tag->offset[1] );
			tag->offset[2] = LittleFloat( tag->offset[2] );

			LL( tag->numBoneReferences );
			LL( tag->ofsBoneReferences );
			LL( tag->ofsEnd );

			// swap the bone references
			boneref = ( int * )( ( byte *)tag + tag->ofsBoneReferences );
			for ( j = 0; j < tag->numBoneReferences; j++, boneref++ ) {
				*boneref = LittleLong( *boneref );
			}

			// find the next tag
			tag = ( mdmTag_t * )( (byte *)tag + tag->ofsEnd );
		}
	}

	// swap all the surfaces
	surf = ( mdmSurface_t * )( (byte *)mdm + mdm->ofsSurfaces );
	for ( i = 0 ; i < mdm->numSurfaces ; i++ ) {
		if ( LittleLong( 1 ) != 1 ) {
			//LL(surf->ident);
			LL( surf->shaderIndex );
			LL( surf->minLod );
			LL( surf->ofsHeader );
			LL( surf->ofsCollapseMap );
			LL( surf->numTriangles );
			LL( surf->ofsTriangles );
			LL( surf->numVerts );
			LL( surf->ofsVerts );
			LL( surf->numBoneReferences );
			LL( surf->ofsBoneReferences );
			LL( surf->ofsEnd );
		}

		// change to surface identifier
		surf->ident = SF_MDM;

		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			ri.Error( ERR_DROP, "R_LoadMDM: %s has more than %i verts on a surface (%i)",
					  mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles * 3 > SHADER_MAX_INDEXES ) {
			ri.Error( ERR_DROP, "R_LoadMDM: %s has more than %i triangles on a surface (%i)",
					  mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// register the shaders
		if ( surf->shader[0] ) {
			sh = R_FindShader( surf->shader, LIGHTMAP_NONE, qtrue );
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
		} else {
			surf->shaderIndex = 0;
		}

		if ( LittleLong( 1 ) != 1 ) {
			// swap all the triangles
			tri = ( mdmTriangle_t * )( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL( tri->indexes[0] );
				LL( tri->indexes[1] );
				LL( tri->indexes[2] );
			}

			// swap all the vertexes
			v = ( mdmVertex_t * )( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ ) {
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );

				for ( k = 0 ; k < v->numWeights ; k++ ) {
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
					v->weights[k].offset[0] = LittleFloat( v->weights[k].offset[0] );
					v->weights[k].offset[1] = LittleFloat( v->weights[k].offset[1] );
					v->weights[k].offset[2] = LittleFloat( v->weights[k].offset[2] );
				}

				v = (mdmVertex_t *)&v->weights[v->numWeights];
			}

			// swap the collapse map
			collapseMap = ( int * )( (byte *)surf + surf->ofsCollapseMap );
			for ( j = 0; j < surf->numVerts; j++, collapseMap++ ) {
				*collapseMap = LittleLong( *collapseMap );
			}

			// swap the bone references
			boneref = ( int * )( ( byte *)surf + surf->ofsBoneReferences );
			for ( j = 0; j < surf->numBoneReferences; j++, boneref++ ) {
				*boneref = LittleLong( *boneref );
			}
		}

		// find the next surface
		surf = ( mdmSurface_t * )( (byte *)surf + surf->ofsEnd );
	}

	return qtrue;
}

/*
=================
R_LoadMDX
=================
*/
static qboolean R_LoadMDX( model_t *mod, void *buffer, const char *mod_name ) {
	int i, j;
	mdxHeader_t                 *pinmodel, *mdx;
	mdxFrame_t                  *frame;
	short                       *bframe;
	mdxBoneInfo_t               *bi;
	int version;
	int size;
	int frameSize;

	pinmodel = (mdxHeader_t *)buffer;

	version = LittleLong( pinmodel->version );
	if ( version != MDX_VERSION ) {
		ri.Printf( PRINT_WARNING, "R_LoadMDX: %s has wrong version (%i should be %i)\n",
				   mod_name, version, MDX_VERSION );
		return qfalse;
	}

	mod->type = MOD_MDX;
	size = LittleLong( pinmodel->ofsEnd );
	mod->dataSize += size;
	mdx = mod->model.mdx = ri.Hunk_Alloc( size, h_low );

	memcpy( mdx, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mdx->ident );
	LL( mdx->version );
	LL( mdx->numFrames );
	LL( mdx->numBones );
	LL( mdx->ofsFrames );
	LL( mdx->ofsBones );
	LL( mdx->ofsEnd );
	LL( mdx->torsoParent );

	if ( LittleLong( 1 ) != 1 ) {
		// swap all the frames
		frameSize = (int) ( sizeof( mdxBoneFrameCompressed_t ) ) * mdx->numBones;
		for ( i = 0 ; i < mdx->numFrames ; i++ ) {
			frame = ( mdxFrame_t * )( (byte *)mdx + mdx->ofsFrames + i * frameSize + i * sizeof( mdxFrame_t ) );
			frame->radius = LittleFloat( frame->radius );
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
				frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
				frame->parentOffset[j] = LittleFloat( frame->parentOffset[j] );
			}

			bframe = ( short * )( (byte *)mdx + mdx->ofsFrames + i * frameSize + ( ( i + 1 ) * sizeof( mdxFrame_t ) ) );
			for ( j = 0 ; j < mdx->numBones * sizeof( mdxBoneFrameCompressed_t ) / sizeof( short ) ; j++ ) {
				( (short *)bframe )[j] = LittleShort( ( (short *)bframe )[j] );
			}
		}

		// swap all the bones
		for ( i = 0 ; i < mdx->numBones ; i++ ) {
			bi = ( mdxBoneInfo_t * )( (byte *)mdx + mdx->ofsBones + i * sizeof( mdxBoneInfo_t ) );
			LL( bi->parent );
			bi->torsoWeight = LittleFloat( bi->torsoWeight );
			bi->parentDist = LittleFloat( bi->parentDist );
			LL( bi->flags );
		}
	}

	return qtrue;
}


//=============================================================================

/*
** RE_BeginRegistration
*/
void RE_BeginRegistration( glconfig_t *glconfigOut ) {

	R_Init();

	*glconfigOut = glConfig;

	R_SyncRenderThread();

	tr.viewCluster = -1;		// force markleafs to regenerate
	RE_ClearScene();

	tr.registered = qtrue;

	// NOTE: this sucks, for some reason the first stretch pic is never drawn
	// without this we'd see a white flash on a level load because the very
	// first time the level shot would not be drawn
//	RE_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0);
}

//=============================================================================

/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void ) {
	model_t		*mod;

	// leave a space for NULL model
	tr.numModels = 0;

	mod = R_AllocModel();
	mod->type = MOD_BAD;
}


/*
================
R_Modellist_f
================
*/
void R_Modellist_f( void ) {
	int		i, j;
	model_t	*mod;
	int		total;
	int		lods;

	total = 0;
	for ( i = 1 ; i < tr.numModels; i++ ) {
		mod = tr.models[i];
		lods = 1;
		for ( j = 1 ; j < MD3_MAX_LODS ; j++ ) {
			if ( mod->model.md3[j] && mod->model.md3[j] != mod->model.md3[j-1] ) {
				lods++;
			}
		}
		ri.Printf( PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name );
		total += mod->dataSize;
	}
	ri.Printf( PRINT_ALL, "%8i : Total models\n", total );

#if	0		// not working right with new hunk
	if ( tr.world ) {
		ri.Printf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}
#endif
}


//=============================================================================


/*
================
R_GetTag
================
*/
static md3Tag_t *R_GetTag( md3Header_t *mod, int frame, const char *tagName ) {
	md3Tag_t		*tag;
	int				i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = (md3Tag_t *)((byte *)mod + mod->ofsTags) + frame * mod->numTags;
	for ( i = 0 ; i < mod->numTags ; i++, tag++ ) {
		if ( !strcmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

/*
================
R_LerpTag
================
*/
int R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName ) {
	md3Tag_t	*start, *end;
	int		i;
	float		frontLerp, backLerp;
	model_t		*model;

	model = R_GetModelByHandle( handle );
	if ( !model->model.md3[0] )
	{
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return qfalse;
	}
	else
	{
		start = R_GetTag( model->model.md3[0], startFrame, tagName );
		end = R_GetTag( model->model.md3[0], endFrame, tagName );
		if ( !start || !end ) {
			AxisClear( tag->axis );
			VectorClear( tag->origin );
			return qfalse;
		}
	}
	
	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0 ; i < 3 ; i++ ) {
		tag->origin[i] = start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	VectorNormalize( tag->axis[0] );
	VectorNormalize( tag->axis[1] );
	VectorNormalize( tag->axis[2] );
	return qtrue;
}


/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t		*model;
	md3Header_t	*header;
	md3Frame_t	*frame;

	model = R_GetModelByHandle( handle );

	if ( model->model.bmodel ) {
		VectorCopy( model->model.bmodel->bounds[0], mins );
		VectorCopy( model->model.bmodel->bounds[1], maxs );
		return;
	}

	if ( !model->model.md3[0] ) {
		VectorClear( mins );
		VectorClear( maxs );
		return;
	}

	header = model->model.md3[0];

	frame = (md3Frame_t *)( (byte *)header + header->ofsFrames );

	VectorCopy( frame->bounds[0], mins );
	VectorCopy( frame->bounds[1], maxs );
}

