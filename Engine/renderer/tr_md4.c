/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_md4.c: md4 model functions for skeletal animation system

#include "tr_local.h"

// from tr_md3.c - for helper funcs R_ComputeMD4Lod
extern float ProjectRadius( float r, vec3_t location );

/*
=============
R_CullMD4
R_ComputeMD4Lod
R_ComputeMD4FogNum

  helper funcs for R_AddMD4Surfaces adapted from tr_md3.c
=============
*/
static int R_CullMD4(trRefEntity_t *ent) {
	// normal sphere culling for non-scaled entities
	
	if(!ent->e.nonNormalizedAxes) {
		switch(R_CullLocalPointAndRadius(ent->e.skel.origin,ent->e.skel.radius)) {
		case CULL_OUT:
			
	
			tr.pc.c_sphere_cull_md3_out++;
			return CULL_OUT;

		case CULL_IN:
			tr.pc.c_sphere_cull_md3_in++;
			return CULL_IN;

		case CULL_CLIP:
			tr.pc.c_sphere_cull_md3_clip++;
			break;
		}
	}

	// box culling for scaled entities
	switch(R_CullLocalBox(ent->e.skel.bounds)) {
	case CULL_IN:
		tr.pc.c_box_cull_md3_in++;
		return CULL_IN;
	case CULL_CLIP:
		tr.pc.c_box_cull_md3_clip++;
		return CULL_CLIP;
	case CULL_OUT:
	default:
		tr.pc.c_box_cull_md3_out++;
		return CULL_OUT;
	}
}

static int R_ComputeMD4Lod(trRefEntity_t *ent) {
	float flod, lodscale;
	float projectedRadius;
	int lod = 0;

	projectedRadius = ProjectRadius(ent->e.radius,ent->e.origin);
	if(projectedRadius != 0) {
		lodscale = r_lodscale->value;
		if (lodscale > 20) lodscale = 20;
		flod = 1.0f - projectedRadius * lodscale;
	} else {
		// object intersects near view plane, e.g. view weapon
		flod = 0;
	}

	flod *= 8;
	lod = myftol( flod );
		
	lod += r_lodbias->integer;
		
	if(lod>8)
		lod = 8;
	if(lod<0)
		lod = 0;

	return lod;
}

static int R_ComputeMD4FogNum(trRefEntity_t *ent) {
	int		i, j;
	fog_t	*fog;

	if(tr.refdef.rdflags & RDF_NOWORLDMODEL)
		return 0;

	for(i=1;i<tr.world->numfogs;i++) {
		fog = &tr.world->fogs[i];
		for(j=0;j<3;j++) {
			if( ent->e.skel.origin[j] - ent->e.skel.radius >= fog->bounds[1][j] ||
				ent->e.skel.origin[j] + ent->e.skel.radius <= fog->bounds[0][j] )
				break;
		}
		if(j==3)
			return i;
	}

	return 0;
}

/*
==============
R_AddMD4Surfaces

  adds MD4 surface to render queue.
  adapted from R_AddMD3Surfaces.
==============
*/
void R_AddMD4Surfaces( trRefEntity_t *ent ) {
	qboolean		personalModel;
	int				fogNum,lod,i;
	md4bSurface_t	*surface;
	shader_t		*shader;
	
	// calc lerped pose if RF_SKEL is not set
	if( !(ent->e.renderfx & RF_SKEL) ) {
		if ( ent->e.renderfx & RF_WRAP_FRAMES ) {
			ent->e.frame %= tr.currentModel->animations->numFrames;
			ent->e.oldframe %= tr.currentModel->animations->numFrames;
		}

		R_GetLerpPose(&ent->e.skel,ent->e.hModel,ent->e.oldframe,ent->e.frame,1.0f - ent->e.backlerp);
		ent->e.renderfx |= RF_SKEL;
	}
	
	// cull the entire model if bounding box is outside the view frustum.
	if(R_CullMD4(ent) == CULL_OUT)
		return;

	// don't add third_person objects if not in a portal
	personalModel = (ent->e.renderfx & RF_THIRD_PERSON) && !tr.viewParms.isPortal;

	// set up lighting now that we know we aren't culled
	if ( !personalModel || r_shadows->integer > 1 )
		R_SetupEntityLighting( &tr.refdef, ent );

	// see if we are in a fog volume
	fogNum = R_ComputeMD4FogNum(ent);

	// compute LOD
	lod = R_ComputeMD4Lod(ent);

	// draw all surfaces
	surface = (md4bSurface_t *)( (byte *)tr.currentModel->md4 + tr.currentModel->md4->ofsSurfs );
	for(i=0;i<tr.currentModel->md4->numSurfs;i++) {
		if ( ent->e.customShader ) {
			shader = R_GetShaderByHandle( ent->e.customShader );
		} else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) {
			skin_t *skin;
			int		j;

			skin = R_GetSkinByHandle( ent->e.customSkin );

			// match the surface name to something in the skin file
			shader = tr.defaultShader;
			for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
				// the names have both been lowercased
				if ( !strcmp( skin->surfaces[j]->name, surface->name ) ) {
					shader = skin->surfaces[j]->shader;
					break;
				}
			}
			if (shader == tr.defaultShader) {
				ri.Printf( PRINT_DEVELOPER, "WARNING: no shader for surface %s in skin %s\n", surface->name, skin->name);
			} else if (shader->defaultShader) {
				ri.Printf( PRINT_DEVELOPER, "WARNING: shader %s in skin %s not found\n", shader->name, skin->name);
			}
		} else if ( surface->shaderIndex <= 0 ) {
			shader = tr.defaultShader;
		} else {
			// only one shader per surface for now
			shader = tr.shaders[ surface->shaderIndex ];
		}

		// set lod bias
		surface->lodBias = lod;

		// we will add shadows even if the main object isn't visible in the view
		// stencil shadows can't do personal models unless I polyhedron clip
		if ( !personalModel	&& r_shadows->integer == 2 && fogNum == 0 &&
			!(ent->e.renderfx & ( RF_NOSHADOW | RF_DEPTHHACK ) ) &&
			shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( (void *)surface, tr.shadowShader, 0, qfalse );
		}

		// projection shadows work fine with personal models
		if ( r_shadows->integer == 3 && fogNum == 0	&& 
			(ent->e.renderfx & RF_SHADOW_PLANE ) &&
			shader->sort == SS_OPAQUE ) {
			R_AddDrawSurf( (void *)surface, tr.projectionShadowShader, 0, qfalse );
		}

		// don't add third_person objects if not viewing through a portal
		if ( !personalModel ) {
			R_AddDrawSurf( (void *)surface, shader, fogNum, qfalse );
		}

		surface = (md4bSurface_t *)( (byte *)surface + surface->ofsEnd );
	}
}

/*
==============
RB_SurfaceMD4

  calculates new pose if no current pose is available.
  adjusts surface to lod using the collapse map.
  deforms verts by bone weights and exports to be rendered.
==============
*/
void RB_SurfaceMD4( md4bSurface_t *surface ) {
	int				i,j,k;
	int				*cmap,lod,tessStart;
	md4bVertex_t		*v;
	md4bTriangle_t	*tris;
	trRefEntity_t	*cent;
	vec3_t tmpNorms;

	cent = backEnd.currentEntity;

	// use skeleton pose from cent->e.skel if render flag is set
	if( !(cent->e.renderfx & RF_SKEL) ) {
		// calc lerped pose and set RF_SKEL so other surfs use this
		R_GetLerpPose(&cent->e.skel,cent->e.hModel,cent->e.oldframe,cent->e.frame,1.0f - cent->e.backlerp);
		
		cent->e.renderfx |= RF_SKEL;
	}

//	if ( r_showbones->integer ) {
//		DrawBones (&cent->e.skel);
//	}
	// make sure the surf is within spec
	RB_CheckOverflow( surface->numVerts, surface->numTris * 3 );

	// set surf lod by the collapse map
	cmap = (int *)((byte *)surface + surface->ofsCollapseMap );
	tris = (md4bTriangle_t *) ((byte *)surface + surface->ofsTris);
	lod = (int)(surface->numVerts);//(int)(surface->numVerts - ((surface->numVerts - surface->minLod)*((float)surface->lodBias/8.0f)));

	tessStart = tess.numIndexes;
	// go through all tris
	for(i=0;i<surface->numTris;i++,tris++) {
		int	tempTri[3];

		// check all indexes per tri
		for(j=0;j<3;j++) {
			// copy the index from the tri list
			tempTri[j] = tris->indexes[j];
			// collapse by index in collapse map
			/*while(tempTri[j]>=lod) {
				tempTri[j] = cmap[tempTri[j]];
			}*/
		}
		
		// skip degenerate tris
		if( (tempTri[0] == tempTri[1]) ||
			(tempTri[1] == tempTri[2]) ||
			(tempTri[2] == tempTri[0]) )
			continue;

		// tri passed collapse mapping, add to list
		tess.indexes[tess.numIndexes + 0] = tempTri[0];
		tess.indexes[tess.numIndexes + 1] = tempTri[1];
		tess.indexes[tess.numIndexes + 2] = tempTri[2];
		tess.numIndexes += 3;
	}

	// this loops surf->numVerts if lodBias is 0, or surf->minLod if lodBias is 4
	v = (md4bVertex_t *) ((byte *)surface + surface->ofsVerts);
	for(i=0;i<lod;i++) {
		vec3_t	tempVert, tempNormal, rotMatrix[3], rotMatrix2[3];
		vec4_t	matrix,baseMatrix, origin;
		qboolean setWeight;
		vec3_t norm, normPerWeight[16];
		md4bWeight_t	*w;
		vec_t currentWeight;
		VectorClear(tempVert);
		VectorClear(tempNormal);
		currentWeight = 0.0;
		//VectorCopy(cent->e.skel.bones[w->boneIndex].origin,origin);
		w = v->weights;
		
		for(j=0;j<v->numWeights;j++,w++) {
			vec_t weight;
			weight = w->boneWeight;
			//Com_Printf("RB_SurfaceMD4: Vert %i weighted to bone %i by %.4f\n",i,w->boneIndex,w->boneWeight);
			for(k=0;k<3;k++) {
				matrix[0] = cent->e.skel.bones[w->boneIndex].axis[0][k];
				matrix[1] = cent->e.skel.bones[w->boneIndex].axis[1][k];
				matrix[2] = cent->e.skel.bones[w->boneIndex].axis[2][k];
				matrix[3] = cent->e.skel.bones[w->boneIndex].origin[k];
				// deform vertex by bone weight
				tempVert[k] += weight * ( DotProduct( w->offset, matrix ) + matrix[3] );

				normPerWeight[j][k] = DotProduct( v->normal, matrix );
				tempNormal[k] += weight * DotProduct( w->normalOffset, matrix );
			}
			
		}
		tmpNorms[0] = -tempNormal[0];
		tmpNorms[1] = -tempNormal[1];
		tmpNorms[2] = tempNormal[2];



		for(k = 0; k<3; k++)
			tempNormal[k] = tmpNorms[k];
		VectorNormalize(tempNormal);
		

		VectorCopy(tempVert,tess.xyz[tess.numVertexes]);
		VectorCopy(tempNormal,tess.normal[tess.numVertexes]);
		tess.texCoords[tess.numVertexes][0][0] = v->texCoords[0];
		tess.texCoords[tess.numVertexes][0][1] = v->texCoords[1];
		tess.numVertexes++;

		v = (md4bVertex_t *)&v->weights[v->numWeights];
	}
	for(i=0;i<surface->numTris;i++,tris++) {
		int	tempTri[3];
		
	}
}

/*
====================
R_CalcQuickBounds

  just a quick func to get a rough bounding
  box for a custom skeleton pose.
  used by R_GetLerpPose and R_SetBlendPose.
====================
*/
void R_CalcQuickBounds( skel_t *skel ) {
	int		i,j;

	// clear the bounds
	ClearBounds(skel->bounds[0],skel->bounds[1]);

	// set bounds by bone origins
	for(i=0;i<skel->numBones;i++) {
		for(j=0;j<3;j++) {
			skel->bounds[0][j] = skel->bones[i].origin[j] < skel->bounds[0][j] ? skel->bones[i].origin[j] : skel->bounds[0][j];
			skel->bounds[1][j] = skel->bones[i].origin[j] > skel->bounds[1][j] ? skel->bones[i].origin[j] : skel->bounds[1][j];
		}

		// compute new origin
		// FIXME: assumes origin is bone "Bip01"
		if(!strcmp(skel->bones[i].name,"Bip01")) {
			VectorCopy(skel->bones[i].origin,skel->origin);
		}
	}

	// compute bounding sphere radius
	skel->radius = RadiusFromBounds(skel->bounds[0],skel->bounds[1]);

	return;
}

/*
====================
R_CalcBoundsForPose

  this func actually calculates the final pos
  for each vert for the given pose and outputs
  a more accurate bounding box than the above
  function provides, but at much greater cost.
====================
*/
void R_CalcBoundsForPose( skel_t *skel, qhandle_t mod ) {
	int				i,j,k,m;
	model_t			*model;
	md4bSurface_t	*surf;
	md4bVertex_t		*vert;

	model = R_GetModelByHandle( mod );

	if(!model->md4) return;

	// clear the bounds
	ClearBounds(skel->bounds[0],skel->bounds[1]);

	// go through all surfs
	surf = (md4bSurface_t *)((byte *)model->md4 + model->md4->ofsSurfs);
	for(i=0;i<model->md4->numSurfs;i++) {
		// go through minLod verts of each surf
		// verts are in collapse map order
		vert = (md4bVertex_t *)((byte *)surf + surf->ofsVerts);
		for(j=0;j<surf->minLod;j++) {
			vec3_t	tmpVert;
			vec4_t	matrix;
			md4bWeight_t	*w;

			// go through all weights for each vert
			VectorClear(tmpVert);
			w = vert->weights;
			for (k=0;k<vert->numWeights;k++,w++) {
				for(m=0;m<3;m++) {
					// find bone by index
					matrix[0] = skel->bones[w->boneIndex].axis[0][m];
					matrix[1] = skel->bones[w->boneIndex].axis[1][m];
					matrix[2] = skel->bones[w->boneIndex].axis[2][m];
					matrix[3] = skel->bones[w->boneIndex].origin[m];

					// deform vert by sum of bone weights
					tmpVert[m] += w->boneWeight * (DotProduct(matrix,w->offset) + matrix[3]);
				}
			}

			// set bounds by the deformed verts
			for(k=0;k<3;k++) {
				skel->bounds[0][k] = tmpVert[k] < skel->bounds[0][k] ? tmpVert[k] : skel->bounds[0][k];
				skel->bounds[1][k] = tmpVert[k] > skel->bounds[1][k] ? tmpVert[k] : skel->bounds[1][k];
			}

			// next vert
			vert = (md4bVertex_t *)&vert->weights[vert->numWeights];
		}

		// next surf
		surf = (md4bSurface_t *)((byte *)surf + surf->ofsEnd);
	}

	// compute new origin
	// FIXME: assumes origin is bone "Bip01"
	for(i=0;i<skel->numBones;i++) {
		if(!strcmp(skel->bones[i].name,"Bip01")) {
			VectorCopy(skel->bones[i].origin,skel->origin);
			break;
		}
	}

	// compute bounding sphere radius
	skel->radius = RadiusFromBounds(skel->bounds[0],skel->bounds[1]);

	return;
}

/*
====================
R_GetPoseForFrame

  grabs the skeleton pose for any given frame.
  used by R_GetLerpPose.
====================
*/
int R_GetPoseForFrame( skel_t *skel, qhandle_t mod, int framenum ) {
	int				i, j, frameSize;
	model_t			*model;
	md4aFrame_t		*frame;
	md4aBoneName_t	*boneNames = 0;
	md4bBoneName_t	*boneNames2 = 0;

	model = R_GetModelByHandle( mod );

	if(!model->animations) return 0;

	// clamp frame values
	if((framenum >= model->animations->numFrames) || (framenum < 0)) {
		ri.Printf( PRINT_DEVELOPER, "R_GetPoseForFrame: no such frame %d for '%s'\n",framenum,model->animations->name);
		framenum = framenum < 0 ? 0 : model->animations->numFrames - 1;
		if(framenum >= model->animations->numFrames) framenum = 0;
	}

	frameSize = (int)( &((md4aFrame_t *)0)->bones[model->animations->numBones] );
	frame = (md4aFrame_t *)((byte *)model->animations + model->animations->ofsFrames + framenum * frameSize);

	boneNames = (md4aBoneName_t *)((byte *)model->animations + model->animations->ofsBones);

	skel->numBones = model->animations->numBones;

	
	for(i=0;i<skel->numBones;i++) {
		strcpy(skel->bones[i].name, boneNames[i].name);
		skel->bones[i].flags = boneNames[i].flags;
		if(boneNames[i].parent >= skel->numBones) 
			skel->bones[i].parent = -1;
		else
			skel->bones[i].parent = boneNames[i].parent;
		ri.Printf( PRINT_DEVELOPER, "R_GetPoseForFrame: Bone: %i Origin: %.2f %.2f %.2f\n",i,frame->bones[i].matrix[0][3],frame->bones[i].matrix[1][3],frame->bones[i].matrix[2][3]);
		for(j=0;j<3;j++) {
			skel->bones[i].axis[0][j] = frame->bones[i].matrix[j][0];
			skel->bones[i].axis[1][j] = frame->bones[i].matrix[j][1];
			skel->bones[i].axis[2][j] = frame->bones[i].matrix[j][2];
			skel->bones[i].origin[j]  = frame->bones[i].matrix[j][3];
		}

	}

	VectorCopy(frame->bounds[0],skel->bounds[0]);
	VectorCopy(frame->bounds[1],skel->bounds[1]);

	return skel->numBones;
}

/*
====================
R_GetLerpPose

  outputs the skeleton pose lerped between the
  two specified frames. useful for getting a
  specific whole body pose to manipulate in
  cgame (like dead poses for ragdoll effects).
  used by R_LerpBone and R_SetBlendPose.
====================
*/
int R_GetLerpPose( skel_t *skel, qhandle_t mod, int startFrame, int endFrame, float frontLerp ) {
	int		i, j;
	skel_t	endPose;
	model_t	*model;

	model = R_GetModelByHandle( mod );

	if(!model->md4) return 0;

	// get the poses to lerp between
	R_GetPoseForFrame(skel,mod,startFrame);
	R_GetPoseForFrame(&endPose,mod,endFrame);

	// lerp all bones
	for(i=0;i<skel->numBones;i++) {
		for(j=0;j<3;j++) {
			VectorScale(skel->bones[i].axis[j],1.0f - frontLerp,skel->bones[i].axis[j]);
			VectorMA(skel->bones[i].axis[j],frontLerp,endPose.bones[i].axis[j],skel->bones[i].axis[j]);
			VectorNormalize(skel->bones[i].axis[j]);
		}
		VectorScale(skel->bones[i].origin,1.0f - frontLerp,skel->bones[i].origin);
		VectorMA(skel->bones[i].origin,frontLerp,endPose.bones[i].origin,skel->bones[i].origin);
	}
	
	// calc new frame bounds
	R_CalcQuickBounds(skel);

	return skel->numBones;
}

/*
===================================================
	renderer traps for skeletal animation below
===================================================
*/

/*
================
R_GetLerpBone

  finds any given bone out of the specified pose.
  used by R_LerpTag.
================
*/
int R_GetLerpBone( skelBone_t *bone, qhandle_t mod, int startFrame, int endFrame, float frontLerp, const char *name ) {
	skel_t	pose;
	model_t	*model;
	int		i;

	model = R_GetModelByHandle( mod );

	if(!model->md4) return 0;
	
	// get the whole lerped pose
	R_GetLerpPose(&pose,mod,startFrame,endFrame,frontLerp);

	// find the bone and copy it out to 'bone'
	for(i=0;i<pose.numBones;i++) {
		if(!strcmp(pose.bones[i].name,name)) {
			memcpy(bone,&pose.bones[i],sizeof(pose.bones[i]));

			return 1;
		}
	}

	return 0;
}

/*
====================
R_SetBlendPose

  given the frame numbers and lerp values for
  each of the 3 bone flag groups, this func
  will output a pose blending all 3 together.
  probably the most useful of the traps, use
  this for combining upper and lower body
  animations.
====================
*/
int R_SetBlendPose( skel_t *skel, qhandle_t mod, int startFrame[3], int endFrame[3], float frontLerp[3], vec3_t angles[3] ) {
	int		i,j,group,parent;
	skel_t	poses[3];
	model_t	*model;
	vec3_t	axis[3][3],aMatrix[3][3];
	vec3_t	boneOrg,tempVec;
	
	model = R_GetModelByHandle( mod );

	if(!model->md4) return 0;

	// get the lerped poses for each bone group
	for(i=0;i<3;i++) {
		R_GetLerpPose(&poses[i],mod,startFrame[i],endFrame[i],frontLerp[i]);
		AnglesToAxis(angles[i],axis[i]);
	}

	// make matrix from axis
	for(i=0;i<3;i++) {
		for(j=0;j<3;j++) {
			aMatrix[i][j][0] = axis[i][0][j];
			aMatrix[i][j][1] = axis[i][1][j];
			aMatrix[i][j][2] = axis[i][2][j];
		}
	}

	// assume the first pose as the basis for the final pose
	memcpy(skel,&poses[0],sizeof(poses[0]));

	for(i=0;i<skel->numBones;i++) {
		// copy bone pose from respective group
		group = skel->bones[i].flags - 1;
		parent = skel->bones[i].parent;
		memcpy(&skel->bones[i],&poses[group].bones[i],sizeof(skel->bones[i]));
		
		// calc origin offset and rotate bone axis then attach to new skeleton
		if(parent > -1) {
			VectorClear(boneOrg);
			VectorClear(tempVec);
			VectorSubtract(poses[group].bones[i].origin,poses[group].bones[parent].origin,tempVec);
			VectorRotate(tempVec,aMatrix[group],boneOrg);
			VectorAdd(skel->bones[parent].origin,boneOrg,skel->bones[i].origin);
			VectorRotate(poses[group].bones[i].axis[0],aMatrix[group],skel->bones[i].axis[0]);
			VectorRotate(poses[group].bones[i].axis[1],aMatrix[group],skel->bones[i].axis[1]);
			VectorRotate(poses[group].bones[i].axis[2],aMatrix[group],skel->bones[i].axis[2]);
		}
	}

#if 0	//this is experimental mirroring code
	memcpy(&poses[0],skel,sizeof(poses[0]));	// source copy into poses[0]
	memcpy(&poses[1],skel,sizeof(poses[1]));	// dest copy into poses[1]

	for(i=0;i<poses[0].numBones;i++) {
		// mirror all bones in source
		// tags maintain correct orientation
		// other bones mirrored differently to flip triangles correctly
		if(!strncmp(poses[0].bones[i].name,"tag_",4)) {
			poses[0].bones[i].axis[1][0] *= -1;
			poses[0].bones[i].axis[1][2] *= -1;
			poses[0].bones[i].axis[0][1] *= -1;
			poses[0].bones[i].axis[2][1] *= -1;
		} else {
			poses[0].bones[i].axis[2][0] *= -1;
			poses[0].bones[i].axis[2][2] *= -1;
			poses[0].bones[i].axis[0][1] *= -1;
			poses[0].bones[i].axis[1][1] *= -1;
		}

		poses[0].bones[i].origin[1] *= -1;

		// find Bip01 L/Bip01 R pairs
		if( strncmp(poses[0].bones[i].name,"Bip01 L ",8) == 0 ||
			strncmp(poses[0].bones[i].name,"Bip01 R ",8) == 0 ) {
			char	tempName[32];
			// copy left <-> right
			memcpy(&tempName,&poses[0].bones[i].name,sizeof(poses[0].bones[i].name));
			tempName[6] = tempName[6] == 'L' ? 'R' : 'L';

			for(j=0;j<poses[0].numBones;j++) {
				if(!strcmp(tempName,poses[1].bones[j].name)) {
					VectorCopy(poses[0].bones[i].origin,poses[1].bones[j].origin);
					AxisCopy(poses[0].bones[i].axis,poses[1].bones[j].axis);
				}
			}
		} else {
			// no pair, copy to dest as-is
			VectorCopy(poses[0].bones[i].origin,poses[1].bones[i].origin);
			AxisCopy(poses[0].bones[i].axis,poses[1].bones[i].axis);
		}
	}

	// copy out the newly-mirrored skeleton
	memcpy(skel,&poses[1],sizeof(poses[1]));
#endif

	R_CalcBoundsForPose(skel,mod);

	return skel->numBones;
}