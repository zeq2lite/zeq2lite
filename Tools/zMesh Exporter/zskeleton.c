/*
 *  zSkeleton - smd2md4 reborn by Alex
 *      skeletal model & animation compile tool redesigned and enhanced
 *	smd2md4 - buildmd4 reborn by Gongo
 *		md4 compile tool rebuilt from the ground up
 */

#include "zskeleton.h"

qboolean verbose, debug, force;		// command line switches
vec3_t modelOffset = {0.0,0.0,0.0};
//char srcFileName[512], dstFileName[512];	// source and destination filenames
//smdModel_t srcModel;				// holds model data from smd for processing
zSkel_Config_t config;
int configMode = 0;
int currentBoneSet = 0;

int main(int argc, char * argv[]) {
	myargc = argc;
	myargv = argv;

	// print the help message and exit if requested or not enough args passed
	if( (CheckParm("--help") != 0) ||
		(CheckParm("-h") != 0) ||
		(CheckParm("-?") != 0) ||
		(argc < 2) ) {
		PrintUsage();
		return -1;
	}

	printf("zSkel Compiler v.%.1f by Alex Darby\n based on smd2md4 v.%.1f by Gongo\n", ZSKEL_VERSION/10.0f, SMD2MD4_VERSION/10.0f);
	// copy the source filename
//	strcpy(srcFileName,argv[1]);
	LoadCFG(argv[1]);		// load the bone flag config file

	if(config.debug) {
		LoadCFG_Print();
	}
	
	if(config.numMeshes) DoMeshes();
	if(config.numAnims) DoAnims();
	//LoadSMD();		// load the smd data
	// FIXME: pick a better name for this, a lot of different clean up routines in one
	//FixCoords();	// fix world space coordinates for renderer

	return 0;
}

void DoMeshes(void)
{
	int i;
	
	if(config.verbose) printf(" Compiling %i Meshes...\n", config.numMeshes);
	for(i = 0; i < config.numMeshes; i++)
	{
		char dstFileName[512];
		strcpy(dstFileName,config.meshes[i].output);
		StripExtension(dstFileName);
		DefaultExtension(dstFileName,config.meshExtension);
		if(!config.forceCompile && FileExists(dstFileName))
		{
			printf("  Can not compile mesh \"%s\", file %s already exists.\nUse the \"Overwrite\" config attribute to force an overwrite.\n",config.meshes[i].name,dstFileName);
			continue;
		}
		if(config.verbose) printf("   Loading \"%s.smd\"\n",config.meshes[i].input);
		LoadSMD(&config.meshes[i].smd,config.meshes[i].name,config.meshes[i].input);
		if(config.debug) printf("   SMD Loaded \"%s.smd\" with %i frames, %i bones and %i surfs\n",config.meshes[i].input,config.meshes[i].smd.numFrames,config.meshes[i].smd.numBones,config.meshes[i].smd.numSurfs);
		FixCoords(&config.meshes[i].smd);
		if(config.debug) printf("   SMD \"%s.smd\" Processed with %i frames, %i bones and %i surfs\n",config.meshes[i].input,config.meshes[i].smd.numFrames,config.meshes[i].smd.numBones,config.meshes[i].smd.numSurfs);
		SaveMesh(&config.meshes[i].smd,config.meshes[i].output);
	}
}

void DoAnims(void)
{
	int i;
	
	if(config.verbose) printf(" Compiling %i Animations...\n", config.numAnims);
	for(i = 0; i < config.numAnims; i++)
	{
		char dstFileName[512];
		strcpy(dstFileName,config.anims[i].output);
		StripExtension(dstFileName);
		DefaultExtension(dstFileName,config.animExtension);
		if(!config.forceCompile && FileExists(dstFileName))
		{
			printf("  Can not compile animation \"%s\", file %s already exists.\nUse the \"Overwrite\" config attribute to force an overwrite.\n",config.anims[i].name,dstFileName);
			continue;
		}
		if(config.verbose) printf("   Loading \"%s.smd\"\n",config.anims[i].input);
		LoadSMD(&config.anims[i].smd,config.anims[i].name,config.anims[i].input);
		if(config.debug) printf("   SMD Loaded \"%s.smd\" with %i frames, %i bones and %i surfs\n",config.anims[i].input,config.anims[i].smd.numFrames,config.anims[i].smd.numBones,config.anims[i].smd.numSurfs);
		FixCoords(&config.anims[i].smd);
		if(config.debug) printf("   SMD \"%s.smd\" Processed with %i frames, %i bones and %i surfs\n",config.anims[i].input,config.anims[i].smd.numFrames,config.anims[i].smd.numBones,config.anims[i].smd.numSurfs);
		SaveAnimation(&config.anims[i].smd,config.anims[i].output);
	}
}


void PrintUsage (void) {
	printf( "zSkeleton v.%i by Alex Darby\n based on smd2md4 v.%i by Gongo\n"
			"usage: zSkeleton <configName.cfg> [options]\n"
			"options:\n"
			"  --help,-h,-?    this help message\n"
			//"  --md4           md4 output (default)\n"
			/*"  --md4a          md4anim output\n"
			"  --md4b          md4base output\n"*/
			//"  --force         overwrite if destination exists\n"
			//"  --silent        quiet program messages\n"
			/*"  --debug         enable debug output messages\n"*/, ZSKELETON_VERSION, SMD2MD4_VERSION);
}

void LoadSMD(smdModel_t *smd, char modelName[32], char fileName[32]) {
	int smdVer,i;
	char smdFileName[512];

	strcpy(smdFileName,fileName);
	// load file and check version
	StripExtension(smdFileName);
	DefaultExtension(smdFileName,".smd");
	LoadScriptFile(smdFileName);
	MatchToken("version");
	ParseInt(&smdVer);
	if(smdVer != SMD_VERSION) {
		Error("LoadSMD: expected version %i, found version %i in %s\n",SMD_VERSION,smdVer,smdFileName);
	}

	// set the model name
	strncpy(smd->name,modelName,sizeof(smd->name));

	// parse through the file
	while(GetToken(qtrue)) {
		// process the next section
		if(!strcmp(token,"nodes")) {
			// bone names and hierarchy
			LoadSMD_ParseNodes(smd);
		} else if(!strcmp(token,"skeleton")) {
			// animation data
			LoadSMD_ParseSkeleton(smd);
		} else if(!strcmp(token,"triangles")) {
			// mesh data (optional)
			LoadSMD_ParseTriangles(smd);
		} else if(!strcmp(token,"end")) {
			break;
		} else {
			if(config.debug) {
				printf("LoadSMD: unknown section \"%s\" in %s, skipping\n",token,smdFileName);
			}
			break;
		}
	}
	// children inherit parent's bone flags
	for(i = 0; i < smd->numBones; i++) {
		if((smd->bones[i].flags == 0) && (smd->bones[i].parent >= 0) && (smd->bones[smd->bones[i].parent].flags != 0)) {
			smd->bones[i].flags = smd->bones[smd->bones[i].parent].flags;
		}
	}
}
int GetFlagForBone(char boneName[64])
{
	int i, j;

	for(i = 0; i < config.numBoneSets; i++)
	{
		for(j = 0; j < config.boneSets[i].numBones; j++)
		{
			if(!strcmp(config.boneSets[i].bones[j],boneName)) return i+1;
		}
	}

	return 0;
}
void LoadSMD_ParseNodes(smdModel_t *smd) {
	int numBones = 0;
	smdBoneName_t *bones;

	if(config.verbose && !config.debug) {
		printf("   bones\r");
	}

	// allocate space and zero it out
	smd->bones = (smdBoneName_t *)calloc(MAX_BONES, sizeof(smdBoneName_t));
	bones = smd->bones;

	while(1) {
		int boneNum = -1;

		GetToken(qtrue);	// bone number
		if(!strcmp(token,"end")) {
			break;
		}

		if(numBones >= MAX_BONES) {
			Error("LoadSMD_ParseNodes: exceeded maximum of %i bones\n",MAX_BONES);
		}

		boneNum = atoi(token);
		if(boneNum < 0) {
			Error("LoadSMD_ParseNodes: invalid bone index %i\n",boneNum);
		}

		bones[boneNum].id = boneNum;
		GetToken(qfalse);	// bone name
		strncpy(bones[boneNum].name,token,sizeof(bones[boneNum].name));
		ParseInt(&bones[boneNum].parent);	// bone parent
		bones[boneNum].flags = GetFlagForBone(bones[boneNum].name);
		numBones++;
		
		if(config.debug) {
			printf("   found bone \"%s\"\n",smd->bones[boneNum].name);
		}
	}

	// update the counter and resize array to save heap space
	smd->numBones = numBones;
	smd->bones = (smdBoneName_t *)realloc(smd->bones,sizeof(smdBoneName_t) * smd->numBones);

	if(config.verbose && !config.debug) {
		printf("  \n");
	}
}

void LoadSMD_ParseSkeleton(smdModel_t *smd) {
	int numFrames = 0;

	if(config.verbose && !config.debug) {
		printf("   frames\r");
	}

	// allocate space and zero it out
	smd->frames = (smdFrame_t *)calloc(MAX_FRAMES,sizeof(smdFrame_t) + (sizeof(smdBoneFrame_t) * smd->numBones));

	while(1) {
		int i,frameNum = -1;
		smdBoneFrame_t *bones;
		smdFrame_t *frame;

		GetToken(qtrue);	// "time"
		if(!strcmp(token,"end")) {
			break;
		}

		if(numFrames >= MAX_FRAMES) {
			Error("LoadSMD_ParseSkeleton: exceeded maximum of %i frames\n",MAX_FRAMES);
		}

		ParseInt(&frameNum);	// frame number
		if(frameNum < 0) {
			Error("LoadSMD_ParseSkeleton: invalid frame index %i\n",frameNum);
		}

		// make sure space for the bones is properly allocated
		frame = &smd->frames[frameNum];
		frame->bones = (smdBoneFrame_t *)realloc(frame->bones,sizeof(smdBoneFrame_t) * smd->numBones);
		bones = frame->bones;

		// clear bounding box for frame
		ClearBounds(frame->bounds[0],frame->bounds[1]);

		for(i = 0; i < smd->numBones; i++) {
			int boneNum,parent;
			float rotation[3];
			smdBoneFrame_t temp;

			GetToken(qtrue);	// bone number
			if(!strcmp(token,"time")) {
				// FIXME: smd spec allows lerping of bones between unkeyed frames
				Error("LoadSMD_ParseSkeleton: missing bone keys in frame %i\n",frameNum);
			}

			boneNum = atoi(token);
			if(boneNum < 0) {
				Error("LoadSMD_ParseSkeleton: frame %i has reference to invalid bone index %i\n",frameNum,boneNum);
			}

			Parse1DMatrix(3,temp.origin);	// bone position
			Parse1DMatrix(3,rotation);		// bone rotation

			// calculate bone matrix
			parent = smd->bones[boneNum].parent;
			AnglesToMatrix(rotation,temp.matrix);
			if(parent >= 0) {
				// FIXME: assumes bones listed in order, smd spec allows bones to be in any order
				MatrixMultiply(bones[parent].matrix,temp.matrix,bones[boneNum].matrix);
				VectorRotate(temp.origin,bones[parent].matrix,bones[boneNum].origin);
				VectorAdd(bones[parent].origin,bones[boneNum].origin,bones[boneNum].origin);
			} else {
				// this is a root bone, copy as-is, child of world
				VectorCopy(temp.origin,bones[boneNum].origin);
				VectorCopy(temp.matrix[0],bones[boneNum].matrix[0]);
				VectorCopy(temp.matrix[1],bones[boneNum].matrix[1]);
				VectorCopy(temp.matrix[2],bones[boneNum].matrix[2]);
			}
		}

		numFrames++;
	}

	// update the counter and resize array to save heap space
	smd->numFrames = numFrames;
	smd->frames = (smdFrame_t *)realloc(smd->frames,(sizeof(smdFrame_t) + (sizeof(smdBoneFrame_t) * smd->numBones)) * smd->numFrames);

	if(config.verbose && !config.debug) {
		printf("  \n");
	}
}

void LoadSMD_ParseTriangles(smdModel_t *smd) {
	int i,j,k,numSurfs = 0;
	smdSurface_t *surfs;
	smdBoneFrame_t *bones;

	if(config.verbose && !config.debug) {
		printf("   meshes\r");
	}

	// allocate space and zero it out
	smd->surfs = (smdSurface_t *)calloc(1,sizeof(smdSurface_t) + (sizeof(smdTriangle_t) * MAX_TRIS) + (sizeof(smdVertex_t) * MAX_VERTS) + (sizeof(int) * MAX_VERTS));
	
	// reduce typing, my hands hurt...
	surfs = smd->surfs;
	bones = smd->frames[0].bones;

	while(1) {
		int found = -1;
		int curSurf = -1,vertIndex = -1;
		smdVertex_t *verts;
		smdTriangle_t *tris;
		smdVertex_t temp;

		GetToken(qtrue);	// surface name
		if(!strcmp(token,"end")) {
			break;
		}

		// look for surf name to see if it exists
		for(i = 0; i < numSurfs; i++) {
			if(!strcmp(token,surfs[i].name)) {
				found = i;
				break;
			}
		}
		// add surf if it wasn't found
		if(found < 0) {
			if(numSurfs >= MAX_SURFS) {
				Error("LoadSMD_ParseTriangles: exceeded maximum of %i surfaces\n",MAX_SURFS);
			}

			curSurf = numSurfs;
			// expand surf data as we go
			smd->surfs = (smdSurface_t *)realloc(smd->surfs,(sizeof(smdSurface_t) + (sizeof(smdTriangle_t) * MAX_TRIS) + (sizeof(smdVertex_t) * MAX_VERTS) + (sizeof(int) * MAX_VERTS)) * (numSurfs + 1));
			surfs = smd->surfs;
			strncpy(surfs[curSurf].name,token,sizeof(surfs[curSurf].name));
			surfs[curSurf].id = ++numSurfs;
			surfs[curSurf].numTris = 0;
			surfs[curSurf].numVerts = 0;
			surfs[curSurf].minLod = 0;
			surfs[curSurf].tris = (smdTriangle_t *)realloc(surfs[curSurf].tris,sizeof(smdTriangle_t) * MAX_TRIS);
			surfs[curSurf].verts = (smdVertex_t *)realloc(surfs[curSurf].verts,sizeof(smdVertex_t) * MAX_VERTS);
			surfs[curSurf].map = (int *)realloc(surfs[curSurf].map,sizeof(int) * MAX_VERTS);
		} else {
			curSurf = found;

			if(surfs[curSurf].numTris >= MAX_TRIS) {
				Error("LoadSMD_ParseTriangles: exceeded maximum of %i triangles per surface\n",MAX_TRIS);
			}
		}
		tris = surfs[curSurf].tris;
		verts = surfs[curSurf].verts;

		// parse tri with vert indexes
		for(i = 0; i < 3; i++) {
			GetToken(qtrue);	// reference bone
			temp.parent = atoi(token);
			if(temp.parent < 0) {
				Error("LoadSMD_ParseTriangles: invalid reference bone %i for vertex\n",temp.parent);
			}

			Parse1DMatrix(3,temp.origin);		// vertex position
			Parse1DMatrix(3,temp.normal);		// vertex normal
			Parse1DMatrix(2,temp.texCoords);	// vertex texture coords

			// half-life 2 smd explicitly lists each influence per vertex
			if(GetToken(qfalse)) {
				temp.numWeights = atoi(token);	// number of influences
				if(temp.numWeights < 1) {
					Error("LoadSMD_ParseTriangles: invalid number of influences (%i) for vertex\n",temp.numWeights);
				} else if(temp.numWeights > MAX_WEIGHTS) {
					Error("LoadSMD_ParseTriangles: exceeded maximum of %i influences per vertex\n",MAX_WEIGHTS);
				}

				for(j = 0; j < temp.numWeights; j++) {
					ParseInt(&temp.weights[j].bone);	// bone index
					if(temp.weights[j].bone < 0) {
						Error("LoadSMD_ParseTriangles: invalid bone index %i for influence %i on vertex\n",temp.weights[j].bone,j);
					}
					Parse1DMatrix(1,&temp.weights[j].weight);	// influence weight
					if(temp.weights[j].weight < 0) {
						Error("LoadSMD_ParseTriangles: invalid weight %f for influence %i on vertex\n",temp.weights[j].weight,j);
					}
				}
			} else {
				// half-life 1 smd implies only 1 influence (the reference bone)
				temp.numWeights = 1;
				temp.weights[0].bone = temp.parent;
				temp.weights[0].weight = 1.0f;
			}

			// check for redundant vertices
			found = -1;
			for(j = 0; j < surfs[curSurf].numVerts; j++) {
				if( (temp.parent != verts[j].parent) ||
					(temp.numWeights != verts[j].numWeights) ||
					!VectorCompare(temp.origin,verts[j].origin) ||
					!VectorCompare(temp.normal,verts[j].normal) ||
					(fabs(temp.texCoords[0] - verts[j].texCoords[0]) > 0.0001) ||
					(fabs(temp.texCoords[1] - verts[j].texCoords[1]) > 0.0001) ) {
						continue;
				}
				for(k = 0; k < temp.numWeights; k++) {
					if( (temp.weights[k].bone != verts[j].weights[k].bone) || 
						(fabs(temp.weights[k].weight - verts[j].weights[k].weight) > 0.0001) ) {
						found = 0;
						break;
					} else {
						found = 1;
					}
				}
				if(!found) {
					found = -1;
					continue;
				} else {
					found = j;
					break;
				}
			}

			// add vertex if it wasn't found
			if(found < 0) {
				if(surfs[curSurf].numVerts >= MAX_VERTS) {
					Error("LoadSMD_ParseTriangles: exceeded maximum of %i vertices per surface\n",MAX_VERTS);
				}

				vertIndex = surfs[curSurf].numVerts;

				VectorCopy(temp.origin,verts[vertIndex].origin);
				VectorCopy(temp.normal,verts[vertIndex].normal);
				verts[vertIndex].texCoords[0] = temp.texCoords[0];
				verts[vertIndex].texCoords[1] = temp.texCoords[1];
				verts[vertIndex].parent = temp.parent;
				verts[vertIndex].numWeights = temp.numWeights;

				// calculate influence offsets
				for(j = 0; j < verts[vertIndex].numWeights; j++) {
					int boneNum;
					float tempOrg[3];

					verts[vertIndex].weights[j].bone = temp.weights[j].bone;
					verts[vertIndex].weights[j].weight = temp.weights[j].weight;

					boneNum = verts[vertIndex].weights[j].bone;
					VectorSubtract(verts[vertIndex].origin,bones[boneNum].origin,tempOrg);
					VectorInverseRotate(tempOrg,bones[boneNum].matrix,verts[vertIndex].weights[j].offset);
				}

				surfs[curSurf].map[vertIndex] = vertIndex;
				surfs[curSurf].numVerts++;
				surfs[curSurf].minLod = surfs[curSurf].numVerts;
			} else {
				vertIndex = found;
			}

			// update vertex index
			tris[surfs[curSurf].numTris].verts[i] = vertIndex;
		}
		// add triangle and update counter
		tris[surfs[curSurf].numTris].surf = surfs[curSurf].id;
		smd->surfs[curSurf].numTris++;
	}

	// update counter
	smd->numSurfs = numSurfs;

	if(verbose && !debug) {
		printf("  \n");
	}
}
void LoadCFG_Defaults(void)
{
	config.forceCompile = qfalse;
	config.verbose = qtrue;
	config.debug = qfalse;
	strcpy(config.meshExtension,"zMesh");
	strcpy(config.animExtension,"zAnimation");
	config.numMeshes = 0;
	config.numBoneSets = 0;
	config.numAnims = 0;
	config.originOffset[0] = 0.0f;
	config.originOffset[1] = 0.0f;
	config.originOffset[2] = 0.0f;
}

void LoadCFG_Print(void)
{
	printf("Config:\n");
	printf("   config.debug        = %i\n", config.debug);
	printf("   config.verbose      = %i\n", config.verbose);
	printf("   config.forceCompile = %i\n", config.forceCompile);
	printf("   config.animExtension= \"%s\"\n", config.animExtension);
	printf("   config.meshExtension= \"%s\"\n", config.meshExtension);
	printf("   config.originOffset = {%f, %f, %f}\n", config.originOffset[0], config.originOffset[1], config.originOffset[2]);
	printf("   config.numAnims     = %i\n", config.numAnims); 
	if(config.numAnims > 0)
	{
		int i;
		printf("   Animations:\n");
		for(i = 0; i < config.numAnims; i++)
		{
			printf("      Anim #%i\n",i+1);
			printf("         config.anims[%i].name = \"%s\"\n",i,config.anims[i].name);
			printf("         config.anims[%i].input = \"%s\"\n",i,config.anims[i].input);
			printf("         config.anims[%i].output = \"%s\"\n",i,config.anims[i].output);
			printf("         config.anims[%i].animOffset = {%f, %f, %f}\n",i,config.anims[i].animOffset[0], config.anims[i].animOffset[2], config.anims[i].animOffset[2]);
		}
	}
	printf("   config.numMeshes     = %i\n", config.numMeshes); 
	if(config.numMeshes > 0)
	{
		int i;
		printf("   Meshes:\n");
		for(i = 0; i < config.numMeshes; i++)
		{
			printf("      Mesh #%i\n",i+1);
			printf("         config.meshes[%i].name = \"%s\"\n",i,config.meshes[i].name);
			printf("         config.meshes[%i].input = \"%s\"\n",i,config.meshes[i].input);
			printf("         config.meshes[%i].output = \"%s\"\n",i,config.meshes[i].output);
			printf("         config.meshes[%i].meshOffset = {%f, %f, %f}\n",i,config.meshes[i].meshOffset[0], config.meshes[i].meshOffset[2], config.meshes[i].meshOffset[2]);
		}
	}
	printf("   config.numBoneSets     = %i\n", config.numBoneSets); 
	if(config.numBoneSets > 0)
	{
		int i;
		printf("   BoneSets:\n");
		for(i = 0; i < config.numBoneSets; i++)
		{
			printf("      BoneSet #%i\n",i+1);
			printf("         config.boneSets[%i].name = \"%s\"\n",i,config.boneSets[i].name);
			printf("         config.boneSets[%i].numBones = %i\n",i,config.boneSets[i].numBones);
			if(config.boneSets[i].numBones > 0)
			{
				int j;
				printf("         Bones:\n");
				for(j = 0; j < config.boneSets[i].numBones; j++)
				{
					printf("            config.boneSets[%i].bones[%i] = \"%s\"\n",i, j, config.boneSets[i].bones[j]);
				}
			}
		}
	}
}

void LoadCFG(const char srcFileName[512]) {
	int i,cfgVer,flag = 0;
	int animationIndex = 0;
	char cfgFileName[512];
	
	// Mode List
	// See config.h

	vec_t *moff = &modelOffset;

	LoadCFG_Defaults();
	strcpy(cfgFileName,srcFileName);
	StripExtension(cfgFileName);
	DefaultExtension(cfgFileName,".cfg");
	LoadScriptFile(cfgFileName);
	while(GetToken(qtrue)){
		//if(!strlen(token)){continue;}
		if(verbose && !debug) {
			
		}
		if(!stricmp(token,"General")){
			configMode = ZSC_GENERAL;
			continue;
		}
		if(!stricmp(token,"Skeleton")){
			configMode = ZSC_SKELETON;
			continue;
		}
		if(!stricmp(token,"Animations")){
			configMode = ZSC_ANIM;
			continue;
		}
		if(!stricmp(token,"Meshes")){
			configMode = ZSC_MESH;
			continue;
		}
		else{
			if(configMode == ZSC_GENERAL){
				LoadCFG_General();
			}
			else if(configMode == ZSC_SKELETON){
				LoadCFG_Skeleton();
			}
			else if(configMode == ZSC_MESH){
				LoadCFG_Mesh();
			}
			else if(configMode == ZSC_ANIM){
				LoadCFG_Anim();
			}
		}
	}
	
}

void LoadCFG_General(void)
{
	if(!stricmp(token,"offset")){
		vec3_t *moff;
		moff = &config.originOffset;
		Parse1DMatrix(3,moff);
		config.originOffset[2] = -config.originOffset[2];
	}
	else if(!stricmp(token,"silent") && !config.debug)
	{
		config.verbose = qfalse;
	}
	else if(!stricmp(token,"debug"))
	{
		config.debug = qtrue;
		config.verbose = qtrue;
		printf(" LoadCFG_General: Debug Enabled\n");
	}
	else if(!stricmp(token,"overwrite"))
	{
		config.forceCompile = qtrue;
	}
	else if(!stricmp(token,"DefaultExtension"))
	{
		GetToken(qfalse);
		if(!stricmp(token, "Anim"))
		{
			GetToken(qfalse);
			strcpy(config.animExtension,token);
		}
		else if(!stricmp(token, "Mesh"))
		{
			GetToken(qfalse);
			strcpy(config.meshExtension,token);
		}
	}
}

void LoadCFG_AddBoneToSet(int set, char bone[32])
{
	int boneId = config.boneSets[set].numBones;
	if(boneId < ZSKEL_MAXBONES && set < ZSKEL_MAXBONESETS)
	{
		strcpy(config.boneSets[set].bones[boneId], bone);
		config.boneSets[set].numBones++;
	}
}
void LoadCFG_Skeleton(void)
{
	if(!stricmp(token,"BoneSet")){
		GetToken(qfalse);
		if(config.numBoneSets < ZSKEL_MAXBONESETS)
		{
			strcpy(config.boneSets[config.numBoneSets].name, token);
			currentBoneSet = config.numBoneSets;
			config.numBoneSets++;
		}
		else
		{
			printf(" LoadCFG_Skeleton: Can Not Add BoneSet \"%s\" - Max BoneSets Exceeded %i\n",token, config.numBoneSets);
		}
	}
	else if(!stricmp(token,"Bone"))
	{
		GetToken(qfalse);
		LoadCFG_AddBoneToSet(currentBoneSet,token);
		if(config.debug) printf("LoadCFG_Skeleton: Adding bone \"%s\" to boneset \"%s\"\n",token,config.boneSets[currentBoneSet].name);
	}
}

void LoadCFG_Mesh(void)
{
	if(!stricmp(token,"Mesh"))
	{
		// Grab Mesh Name
		GetToken(qfalse);

		//Check there's room for another mesh
		if(config.numMeshes < ZSKEL_MAXMESHES)
		{
			// Save Typing
			int meshID = config.numMeshes;

			// Set Mesh Name
			strcpy(config.meshes[meshID].name, token);

			// Default input and output to the mesh name
			strcpy(config.meshes[meshID].input,token);
			strcpy(config.meshes[meshID].output,token);

			// Zero Mesh Offset
			config.meshes[meshID].meshOffset[0] = 0.0f;
			config.meshes[meshID].meshOffset[1] = 0.0f;
			config.meshes[meshID].meshOffset[2] = 0.0f;

			// Increase Mesh Counter!
			config.numMeshes++;

			if(config.debug) printf("LoadCFG_Mesh: Adding Mesh %i: \"%s\"\n",config.numMeshes,token);
		}
		else
		{
			// Oh shit! Too Many Meshes! Do Something About it!
			Error(" LoadCFG_Mesh: Can Not Add Mesh \"%s\" - Max Meshes Exceeded %i\n",token, config.numMeshes);
		}
	}
	else if(!stricmp(token,"input"))
	{
		//Check we have a mesh to declare an input for!
		if(config.numMeshes == 0) Error(" LoadCFG_Mesh: Input for no mesh: \"Output %s\"\n",token); // Oh shit! There's no mesh!
		else
		{
			// Grab input file name!
			GetToken(qfalse);

			// Set the file name!
			strcpy(config.meshes[config.numMeshes - 1].input,token);
			if(config.debug) printf(" LoadCFG_Mesh: Setting mesh \"%s\" input to \"%s\"\n",config.meshes[config.numMeshes - 1].name,token);
		}
	}
	else if(!stricmp(token,"output"))
	{
		// See comments for "input" :)
		if(config.numMeshes == 0) Error(" LoadCFG_Mesh: Output for no mesh: \"Output %s\"\n",token);
		else
		{
			GetToken(qfalse);
			strcpy(config.meshes[config.numMeshes - 1].output,token);
			if(config.debug) printf(" LoadCFG_Mesh: Setting mesh \"%s\" output to \"%s\"\n",config.meshes[config.numMeshes - 1].name,token);
		}
	}
	if(!stricmp(token,"offset")){ // Mesh has it's own offset! :o
		vec3_t *moff;											// Parse1DMatrix takes a pointer!
		moff = &config.meshes[config.numMeshes - 1].meshOffset; // So Let's make one!
		Parse1DMatrix(3,moff);
		config.meshes[config.numMeshes - 1].meshOffset[2] = -config.meshes[config.numMeshes - 1].meshOffset[2]; // Invert the Z axis. 'cos otherwise it's wrong
	}
}

void LoadCFG_Anim(void)
{
	// Blah blah blah, same comments as LoadCFG_Mesh until more functionallity is added/needed
	if(!stricmp(token,"Anim"))
	{
		GetToken(qfalse);
		if(config.numAnims < ZSKEL_MAXMESHES)
		{
			int animID = config.numAnims;
			strcpy(config.anims[animID].name, token);
			strcpy(config.anims[animID].input, token);
			strcpy(config.anims[animID].output, token);
			config.anims[animID].animOffset[0] = 0.0f;
			config.anims[animID].animOffset[1] = 0.0f;
			config.anims[animID].animOffset[2] = 0.0f;
			config.numAnims++;
			if(config.debug) printf("LoadCFG_Anim: Adding Anim %i: \"%s\"\n",config.numAnims,token);
		}
		else
		{
			printf(" LoadCFG_Anim: Can Not Add Anim \"%s\" - Max Anims Exceeded %i\n",token, config.numAnims);
		}
	}
	else if(!stricmp(token,"input"))
	{
		if(config.numAnims == 0) Error(" LoadCFG_Anim: Input for no anim: \"Output %s\"\n",token);
		else
		{
			GetToken(qfalse);
			strcpy(config.anims[config.numAnims - 1].input,token);
			if(config.debug) printf(" LoadCFG_Anim: Setting anim \"%s\" input to \"%s\"\n",config.anims[config.numAnims - 1].name,token);
		}
	}
	else if(!stricmp(token,"output"))
	{
		if(config.numAnims == 0) Error(" LoadCFG_Anim: Output for no anim: \"Output %s\"\n",token);
		else
		{
			GetToken(qfalse);
			strcpy(config.anims[config.numAnims - 1].output,token);
			if(config.debug) printf(" LoadCFG_Anim: Setting anim \"%s\" output to \"%s\"\n",config.anims[config.numAnims - 1].name,token);
		}
	}
	if(!stricmp(token,"offset")){ // Mesh has it's own offset! :o
		vec3_t *moff;											// Parse1DMatrix takes a pointer!
		moff = &config.anims[config.numAnims - 1].animOffset; // So Let's make one!
		Parse1DMatrix(3,moff);
		config.anims[config.numAnims - 1].animOffset[2] = -config.anims[config.numAnims - 1].animOffset[2]; // Invert the Z axis. 'cos otherwise it's wrong
		if(config.debug) printf(" LoadCFG_Anim: Setting anim \"%s\" offset to {%f, %f, %f}\n", config.anims[config.numAnims - 1].animOffset[0], config.anims[config.numAnims - 1].animOffset[1], config.anims[config.numAnims - 1].animOffset[2]);
	}
}
/*
void Old_LoadCFG(void) {
	int i,cfgVer,flag = 0;
	char cfgFileName[512];
	vec_t *moff = &modelOffset;

	// look for filename.cfg and load it
	strcpy(cfgFileName,srcFileName);
	StripExtension(cfgFileName);
	DefaultExtension(cfgFileName,".cfg");
	LoadScriptFile(cfgFileName);

	if(verbose && !debug) {
		printf("   flags\r");
	}

	// check version
	MatchToken("version");
	ParseInt(&cfgVer);
	if(cfgVer != CFG_VERSION) {
		Error("LoadCFG: expected version %i, found version %i in %s\n",CFG_VERSION,cfgVer,cfgFileName);
	}
	type = MD4_IDENT;
	// parse the file
	while(GetToken(qtrue)) {
		if(verbose && !debug) {
			printf(" %c\r",indicator[++progress % strlen(indicator)]);
		}
		if(!stricmp(token,"offset"))
		{
			Parse1DMatrix(3,moff);
			modelOffset[2] = -modelOffset[2];
			continue;
		}
		else if(!stricmp(token,"output"))
		{
			GetToken(qfalse);
			strcpy(dstFileName,token);
			continue;
		}
		else if(!stricmp(token,"type"))
		{
			GetToken(qfalse);
			if(!stricmp(token,"base"))
			{
				type = MD4B_IDENT;
			}
			else if(!stricmp(token,"animation"))
			{
				type = MD4A_IDENT;
			}
			continue;
		}
		else if(!strcmp(token,"boneflag")) {
			// get the bone flag
			GetToken(qfalse);
			if(!strcmp(token,"A")) {
				flag = 1;
			} else if(!strcmp(token,"B")) {
				flag = 2;
			} else if(!strcmp(token,"C")) {
				flag = 3;
			} else {
				Error("LoadCFG: unknown bone flag \"%s\" in %s\n",token,cfgFileName);
			}
		} else {
			int found = 0;

			// token is a bone name
			for(i = 0; i < srcModel.numBones; i++) {
				// find the bone and set the flag
				if(!strcmp(token,srcModel.bones[i].name)) {
					srcModel.bones[i].flags = flag;
					found = 1;

					if(debug) {
						printf(" setting flag %i for bone \"%s\"\n",srcModel.bones[i].flags,srcModel.bones[i].name);
					}

					break;
				}
			}

			// couldn't find bone
			if(!found) {
				Error("LoadCFG: unknown bone \"%s\" in %s, not found in %s\n",token,cfgFileName,srcFileName);
			}
		}
	}

	// children inherit parent's bone flags
	for(i = 0; i < srcModel.numBones; i++) {
		if((srcModel.bones[i].flags == 0) && (srcModel.bones[i].parent >= 0) && (srcModel.bones[srcModel.bones[i].parent].flags != 0)) {
			srcModel.bones[i].flags = srcModel.bones[srcModel.bones[i].parent].flags;
		}
	}

	if(verbose && !debug) {
		printf("  \n");
	}
}
*/
void FixCoords(smdModel_t *smd) {
	int i,j,k,v,w,x;
	smdSurface_t *surfs;
	smdTriangle_t *tris;
	smdVertex_t *verts;
	smdFrame_t *frames;
	smdBoneFrame_t *bones;

	if(config.verbose && !config.debug) {
		printf("   triangles\r");
	}

	// void CalcLOD(void)
	surfs = smd->surfs;
	for(i = 0; i < smd->numSurfs; i++) {
		int temp;

		// reverse vertex order for renderer
		tris = surfs[i].tris;
		for(j = 0; j < surfs[i].numTris; j++) {
			temp = tris[j].verts[2];
			tris[j].verts[2] = tris[j].verts[1];
			tris[j].verts[1] = temp;
		}
		
		// calculate collapse map for dynamic lod
		ProgressiveMesh(&surfs[i]);
	}

	// reorder mesh into triangle strips and fans
	surfs = smd->surfs;
	for(i = 0; i < smd->numSurfs; i++) {
		int srcTris[MAX_TRIS][3];
		int dstTris[MAX_TRIS][3];

		tris = surfs[i].tris;
		for(j = 0; j < surfs[i].numTris; j++) {
			srcTris[j][0] = tris[j].verts[0];
			srcTris[j][1] = tris[j].verts[1];
			srcTris[j][2] = tris[j].verts[2];
		}

		OrderMesh(srcTris,dstTris,surfs[i].numTris);

		// copy back reordered triangles
		for(j = 0; j < smd->surfs[i].numTris; j++) {
			tris[j].verts[0] = dstTris[j][0];
			tris[j].verts[1] = dstTris[j][1];
			tris[j].verts[2] = dstTris[j][2];
		}
	}

	if(config.verbose && !config.debug) {
		printf("  \n");
		printf("   vertices\r");
	}

	// TODO: combine bone influences per vertex

	// void FixCoords(void)
	// fix bone axis orientation and position
	frames = smd->frames;
	for(i = 0; i < smd->numFrames; i++) {
		bones = frames[i].bones;

		for(j = 0; j < smd->numBones; j++) {
			float swapAxis[3];
			float swapOrg;

			VectorCopy(bones[j].matrix[1],swapAxis);
			VectorNegate(swapAxis);
			VectorCopy(bones[j].matrix[0],bones[j].matrix[1]);
			VectorCopy(swapAxis,bones[j].matrix[0]);

			swapOrg = -bones[j].origin[1];
			bones[j].origin[1] = bones[j].origin[0];
			bones[j].origin[0] = swapOrg;

			// transform all vertices to final position
			for(k = 0; k < smd->numSurfs; k++) {
				verts = surfs[k].verts;

				for(v = 0; v < surfs[k].numVerts; v++) {
					float tempVert[3];

					VectorClear(tempVert);
					for(w = 0; w < verts[v].numWeights; w++) {
						int boneNum;

						boneNum = verts[v].weights[w].bone;
						for(x = 0; x < 3; x++) {
							tempVert[x] += verts[v].weights[w].weight * ( DotProduct( bones[boneNum].matrix[x], verts[v].weights[w].offset ) + bones[boneNum].matrix[x][3] );
						}
					}
					// update bounding box
					AddPointToBounds(tempVert,frames[i].bounds[0],frames[i].bounds[1]);
				}
			}

			// FIXME: assumes origin bone is "Bip01"
			// set frame origin if this is origin bone
			if(!strcmp(smd->bones[j].name,"Bip01")) {
				VectorCopy(bones[j].origin,frames[i].origin);
			}

			// calculate frame radius
			frames[i].radius = RadiusFromBounds(frames[i].bounds[0],frames[i].bounds[1]);
		}
	}

	// void CalcNormals(void)
	// calculate vertex normals and fix texture coordinates
	bones = frames[0].bones;
	for(i = 0; i < smd->numSurfs; i++) {
		verts = surfs[i].verts;

		for(j = 0; j < surfs[i].numVerts; j++) {
			int parent, k;
			float oldNormal[3];
			
			// vertex normal given relative to first influence
			parent = verts[j].parent;//weights[0].bone;
			VectorCopy(verts[j].normal,oldNormal);
			VectorInverseRotate(oldNormal,bones[parent].matrix,verts[j].normal);
			for(k = 0; k < surfs[i].verts[j].numWeights; k++)
			{
				vec3_t normal;
				VectorInverseRotate(verts[j].normal,bones[verts[j].weights[k].bone].matrix ,normal);
				VectorNormalize(normal,verts[j].weights[k].normalOffset);
			}
			// invert texture coord for renderer
			verts[j].texCoords[1] = 1.0f - verts[j].texCoords[1];
		}
	}

	if(config.verbose && !config.debug) {
		printf("  \n");
	}
}

void SaveAnimation(smdModel_t *smd, char fileName[32]) {
	int i,j,k;
	int surfSum = 0;
	FILE *dstFile;
	char dstFileName[512];
	md4aHeader_t dstHeader;
	md4aFrame_t dstFrame;
	md4aBoneFrame_t dstBoneFrame;
	md4aBoneName_t dstBoneName;
	int cmap[MAX_VERTS];
	smdSurface_t *surfs;
	smdTriangle_t *tris;
	smdVertex_t *verts;
	smdFrame_t *frames;
	smdBoneFrame_t *bones;

	strcpy(dstFileName,fileName);
	StripExtension(dstFileName);
	DefaultExtension(dstFileName,config.animExtension);

	if(config.verbose && !config.debug) {
		printf("   %s\r",dstFileName);
	}

	// open destination file for write
	CreatePath(dstFileName);
	dstFile = SafeOpenWrite(dstFileName);

	// prepare header information
	dstHeader.ident = LittleLong(MD4A_IDENT);
	dstHeader.version = LittleLong(MD4A_VERSION);
	strncpy(dstHeader.name,smd->name,sizeof(dstHeader.name));

	dstHeader.numFrames = LittleLong(smd->numFrames);
	dstHeader.numBones = LittleLong(smd->numBones);

	

	// calculate offsets
	dstHeader.ofsFrames = sizeof(md4aHeader_t);
	dstHeader.ofsBones = dstHeader.ofsFrames + ((sizeof(md4aFrame_t) + (sizeof(md4aBoneFrame_t) * (smd->numBones - 1))) * smd->numFrames);
	dstHeader.ofsEnd = dstHeader.ofsBones + (sizeof(md4aBoneName_t) * smd->numBones);
	

	dstHeader.ofsFrames = LittleLong(dstHeader.ofsFrames);
	dstHeader.ofsBones = LittleLong(dstHeader.ofsBones);
	dstHeader.ofsEnd = LittleLong(dstHeader.ofsEnd);

	// write out header
	SafeWrite(dstFile,&dstHeader,sizeof(md4aHeader_t));

	if(config.debug) {
		printf( "md4Header {\n"
				"	ident		%i\n"
				"	version		%i\n"
				"	name		%s\n"
				"	numFrames	%i\n"
				"	numBones	%i\n"
				"	ofsFrames	%i\n"
				"	ofsBones	%i\n"
				"	ofsEnd		%i\n"
				"}\n",
				dstHeader.ident,
				dstHeader.version,
				dstHeader.name,
				dstHeader.numFrames,
				dstHeader.numBones,
				dstHeader.ofsFrames,
				dstHeader.ofsBones,
				dstHeader.ofsEnd );
	}

	// write out frames
	frames = smd->frames;
	for(i = 0; i < smd->numFrames; i++) {

		dstFrame.bounds[0][0] = LittleFloat(frames[i].bounds[0][0] - config.originOffset[0]);
		dstFrame.bounds[0][1] = LittleFloat(frames[i].bounds[0][1] - config.originOffset[1]);
		dstFrame.bounds[0][2] = LittleFloat(frames[i].bounds[0][2] - config.originOffset[2]);

		dstFrame.bounds[1][0] = LittleFloat(frames[i].bounds[1][0] - config.originOffset[0]);
		dstFrame.bounds[1][1] = LittleFloat(frames[i].bounds[1][1] - config.originOffset[1]);
		dstFrame.bounds[1][2] = LittleFloat(frames[i].bounds[1][2] - config.originOffset[2]);

		dstFrame.localOrigin[0] = LittleFloat(frames[i].origin[0] - config.originOffset[0]);
		dstFrame.localOrigin[1] = LittleFloat(frames[i].origin[1] - config.originOffset[1]);
		dstFrame.localOrigin[2] = LittleFloat(frames[i].origin[2] - config.originOffset[2]);

		dstFrame.radius = LittleFloat(frames[i].radius);

		SafeWrite(dstFile,&dstFrame,sizeof(md4aFrame_t) - sizeof(md4aBoneFrame_t));

		if(config.debug) {
			printf( "md4Frame %i {\n"
					"	mins	(%f, %f, %f)\n"
					"	maxs	(%f, %f, %f)\n"
					"	org		(%f, %f, %f)\n"
					"	radius	%f\n",
					i,
					dstFrame.bounds[0][0],
					dstFrame.bounds[0][1],
					dstFrame.bounds[0][2],
					dstFrame.bounds[1][0],
					dstFrame.bounds[1][1],
					dstFrame.bounds[1][2],
					dstFrame.localOrigin[0],
					dstFrame.localOrigin[1],
					dstFrame.localOrigin[2],
					dstFrame.radius	);
		}

		bones = frames[i].bones;
		for(j = 0; j < smd->numBones; j++) {

			dstBoneFrame.matrix[0][0] = LittleFloat(bones[j].matrix[0][0]);
			dstBoneFrame.matrix[1][0] = LittleFloat(bones[j].matrix[1][0]);
			dstBoneFrame.matrix[2][0] = LittleFloat(bones[j].matrix[2][0]);

			dstBoneFrame.matrix[0][1] = LittleFloat(bones[j].matrix[0][1]);
			dstBoneFrame.matrix[1][1] = LittleFloat(bones[j].matrix[1][1]);
			dstBoneFrame.matrix[2][1] = LittleFloat(bones[j].matrix[2][1]);

			dstBoneFrame.matrix[0][2] = LittleFloat(bones[j].matrix[0][2]);
			dstBoneFrame.matrix[1][2] = LittleFloat(bones[j].matrix[1][2]);
			dstBoneFrame.matrix[2][2] = LittleFloat(bones[j].matrix[2][2]);

			dstBoneFrame.matrix[0][3] = LittleFloat(bones[j].origin[0] - config.originOffset[0]);
			dstBoneFrame.matrix[1][3] = LittleFloat(bones[j].origin[1] - config.originOffset[1]);
			dstBoneFrame.matrix[2][3] = LittleFloat(bones[j].origin[2] - config.originOffset[2]);

			// write out bone frame
			SafeWrite(dstFile,&dstBoneFrame,sizeof(md4aBoneFrame_t));

			if(config.debug) {
				printf( "	md4BoneFrame %i {\n"
						"		matrix[0]	(%f, %f, %f)\n"
						"		matrix[1]	(%f, %f, %f)\n"
						"		matrix[2]	(%f, %f, %f)\n"
						"		matrix[3]	(%f, %f, %f)\n"
						"	}\n",
						j,
						dstBoneFrame.matrix[0][0],
						dstBoneFrame.matrix[1][0],
						dstBoneFrame.matrix[2][0],
						dstBoneFrame.matrix[0][1],
						dstBoneFrame.matrix[1][1],
						dstBoneFrame.matrix[2][1],
						dstBoneFrame.matrix[0][2],
						dstBoneFrame.matrix[1][2],
						dstBoneFrame.matrix[2][2],
						dstBoneFrame.matrix[0][3],
						dstBoneFrame.matrix[1][3],
						dstBoneFrame.matrix[2][3] );
			}
		}

		if(debug) {
			printf("}\n");
		}
	}

	// write out bone names
	for(i = 0; i < smd->numBones; i++) {

		strcpy(dstBoneName.name,smd->bones[i].name);
		dstBoneName.parent = LittleLong(smd->bones[i].parent);
		dstBoneName.flags = LittleLong(smd->bones[i].flags);

		SafeWrite(dstFile,&dstBoneName,sizeof(md4aBoneName_t));

		if(config.debug) {
			printf( "md4BoneName %i {\n"
					"	name	%s\n"
					"	parent	%i\n"
					"	flags	%i\n"
					"}\n",
					i,
					dstBoneName.name,
					dstBoneName.parent,
					dstBoneName.flags );
		}
	}


	if(config.verbose && !config.debug) {

		printf( "  \n");


		// print out file summary
		printf( " stats\n"
				"   size:   %.2f kb\n"
				"   bones:  %i\n"
				"   frames: %i\n",
				ftell(dstFile)/1024.0f,
				smd->numBones,
				smd->numFrames );
	}
}

void SaveMesh(smdModel_t *smd, char fileName[32]) {
	int i,j,k;
	int surfSum = 0;
	FILE *dstFile;
	char dstFileName[512];
	md4bHeader_t dstHeader;
	md4bBoneName_t dstBoneName;
	md4bSurface_t dstSurface;
	md4bVertex_t dstVertex;
	md4bWeight_t dstWeight;
	md4bTriangle_t dstTriangle;
	int cmap[MAX_VERTS];
	smdSurface_t *surfs;
	smdTriangle_t *tris;
	smdVertex_t *verts;
	smdFrame_t *frames;
	smdBoneFrame_t *bones;

	strcpy(dstFileName,fileName);
	StripExtension(dstFileName);
	DefaultExtension(dstFileName,config.meshExtension);

	if(config.verbose && !config.debug) {
		printf("   %s\r",dstFileName);
	}

	// open destination file for write
	CreatePath(dstFileName);
	dstFile = SafeOpenWrite(dstFileName);

	// prepare header information
	dstHeader.ident = LittleLong(MD4B_IDENT);
	dstHeader.version = LittleLong(MD4B_VERSION);
	strncpy(dstHeader.name,dstFileName,sizeof(dstHeader.name));

	dstHeader.numBones = LittleLong(smd->numBones);
	dstHeader.numSurfs = LittleLong(smd->numSurfs);

	// calculate surface offset
	for(i = 0; i < smd->numSurfs; i++) {

		surfSum += sizeof(md4bSurface_t);
		for(j = 0; j < smd->surfs[i].numVerts; j++) {
			surfSum += sizeof(md4bVertex_t) + (sizeof(md4bWeight_t) * (smd->surfs[i].verts[j].numWeights - 1));
			// FIXME: sizeof(md4Vertex_t) is wrong, it's +4 larger than it should be...
			surfSum -= 4;
		}
		surfSum += sizeof(md4bTriangle_t) * smd->surfs[i].numTris;
		surfSum += sizeof(cmap[0]) * smd->surfs[i].numVerts;
	}

	// calculate offsets
	dstHeader.ofsBones = sizeof(md4bHeader_t);
	dstHeader.ofsSurfs = dstHeader.ofsBones + (sizeof(md4bBoneName_t) * smd->numBones);
	dstHeader.ofsEnd = dstHeader.ofsSurfs + surfSum;

	dstHeader.ofsBones = LittleLong(dstHeader.ofsBones);
	dstHeader.ofsSurfs = LittleLong(dstHeader.ofsSurfs);
	dstHeader.ofsEnd = LittleLong(dstHeader.ofsEnd);

	// write out header
	SafeWrite(dstFile,&dstHeader,sizeof(md4bHeader_t));

	if(config.debug) {
		printf( "md4Header {\n"
				"	ident		%i\n"
				"	version		%i\n"
				"	name		%s\n"
				"	numFrames	%i\n"
				"	numBones	%i\n"
				"	numSurfs	%i\n"
				"	ofsFrames	%i\n"
				"	ofsBones	%i\n"
				"	ofsSurfs	%i\n"
				"	ofsEnd		%i\n"
				"}\n",
				dstHeader.ident,
				dstHeader.version,
				dstHeader.name,
				dstHeader.numBones,
				dstHeader.numSurfs,
				dstHeader.ofsBones,
				dstHeader.ofsSurfs,
				dstHeader.ofsEnd );
	}

	// write out frames
	frames = smd->frames;
	
	

	// write out bone names
	for(i = 0; i < smd->numBones; i++) {

		strcpy(dstBoneName.name,smd->bones[i].name);
		dstBoneName.parent = LittleLong(smd->bones[i].parent);
		dstBoneName.flags = LittleLong(smd->bones[i].flags);

		SafeWrite(dstFile,&dstBoneName,sizeof(md4bBoneName_t));

		if(config.debug) {
			printf( "md4BoneName %i {\n"
					"	name	%s\n"
					"	parent	%i\n"
					"	flags	%i\n"
					"}\n",
					i,
					dstBoneName.name,
					dstBoneName.parent,
					dstBoneName.flags );
		}
	}

	// write out surfaces
	surfs = smd->surfs;
	for(i = 0; i < smd->numSurfs; i++) {
		int vertSum = 0;

		// prepare surface header
		dstSurface.ident = LittleLong(MD4_IDENT);
		strcpy(dstSurface.name,surfs[i].name);
		strcpy(dstSurface.shader,"");
		dstSurface.shaderIndex = LittleLong(0);
		dstSurface.lodBias = LittleLong(0);
		dstSurface.minLod = LittleLong(surfs[i].minLod);
		dstSurface.numVerts = LittleLong(surfs[i].numVerts);
		dstSurface.numTris = LittleLong(surfs[i].numTris);

		// calculate vertex offset
		for(j = 0; j < surfs[i].numVerts; j++) {
			
			vertSum += sizeof(md4Vertex_t) + (sizeof(md4Weight_t) * (surfs[i].verts[j].numWeights - 1));
			// FIXME: sizeof(md4Vertex_t) is wrong, it's +4 larger than it should be...
			vertSum -= 4;
		}

		// calculate offsets
		dstSurface.ofsHeader = -ftell(dstFile);
		dstSurface.ofsVerts = sizeof(md4Surface_t);
		dstSurface.ofsTris = dstSurface.ofsVerts + vertSum;
		dstSurface.ofsCollapseMap = dstSurface.ofsTris + (sizeof(md4Triangle_t) * surfs[i].numTris);
		dstSurface.ofsEnd = dstSurface.ofsCollapseMap + (sizeof(cmap[0]) * surfs[i].numVerts);

		dstSurface.ofsHeader = LittleLong(dstSurface.ofsHeader);
		dstSurface.ofsVerts = LittleLong(dstSurface.ofsVerts);
		dstSurface.ofsTris = LittleLong(dstSurface.ofsTris);
		dstSurface.ofsCollapseMap = LittleLong(dstSurface.ofsCollapseMap);
		dstSurface.ofsEnd = LittleLong(dstSurface.ofsEnd);

		SafeWrite(dstFile,&dstSurface,sizeof(md4Surface_t));

		if(config.debug) {
			printf( "md4Surface %i {\n"
					"	ident	%i\n"
					"	name	%s\n"
					"	shader	%s\n"
					"	shaderIndex	%i\n"
					"	lodBias	%i\n"
					"	minLod	%i\n"
					"	numVerts	%i\n"
					"	numTris	%i\n"
					"	ofsHeader	%i\n"
					"	ofsVerts	%i\n"
					"	ofsTris	%i\n"
					"	ofsCollapseMap	%i\n"
					"	ofsEnd	%i\n",
					i, 
					dstSurface.ident,
					dstSurface.name,
					dstSurface.shader,
					dstSurface.shaderIndex,
					dstSurface.lodBias,
					dstSurface.minLod,
					dstSurface.numVerts,
					dstSurface.numTris,
					dstSurface.ofsHeader,
					dstSurface.ofsVerts,
					dstSurface.ofsTris,
					dstSurface.ofsCollapseMap,
					dstSurface.ofsEnd );
		}

		// write out vertices
		verts = surfs[i].verts;
		for(j = 0; j < surfs[i].numVerts; j++) {
			
			dstVertex.normal[0] = LittleFloat(verts[j].normal[0]);
			dstVertex.normal[1] = LittleFloat(verts[j].normal[1]);
			dstVertex.normal[2] = LittleFloat(verts[j].normal[2]);
			
			dstVertex.texCoords[0] = LittleFloat(verts[j].texCoords[0]);
			dstVertex.texCoords[1] = LittleFloat(verts[j].texCoords[1]);

			dstVertex.numWeights = LittleLong(verts[j].numWeights);

			// FIXME: sizeof(md4Vertex_t) is wrong, it's +4 larger than it should be...
			SafeWrite(dstFile,&dstVertex.normal[0],sizeof(dstVertex.normal[0]));
			SafeWrite(dstFile,&dstVertex.normal[1],sizeof(dstVertex.normal[1]));
			SafeWrite(dstFile,&dstVertex.normal[2],sizeof(dstVertex.normal[2]));
			SafeWrite(dstFile,&dstVertex.texCoords[0],sizeof(dstVertex.texCoords[0]));
			SafeWrite(dstFile,&dstVertex.texCoords[1],sizeof(dstVertex.texCoords[1]));
			SafeWrite(dstFile,&dstVertex.numWeights,sizeof(dstVertex.numWeights));

			if(config.debug) {
				printf( "	md4Vertex %i {\n"
						"		normal	(%f, %f, %f)\n"
						"		texCoords	(%f, %f)\n"
						"		numWeights	%i\n",
						j,
						dstVertex.normal[0],
						dstVertex.normal[1],
						dstVertex.normal[2],
						dstVertex.texCoords[0],
						dstVertex.texCoords[1],
						dstVertex.numWeights );
			}

			// write out influences
			for(k = 0; k < verts[j].numWeights; k++) {
				float normal[3];
				dstWeight.boneIndex = LittleLong(verts[j].weights[k].bone);
				dstWeight.boneWeight = LittleFloat(verts[j].weights[k].weight);

				dstWeight.offset[0] = LittleFloat(verts[j].weights[k].offset[0]);
				dstWeight.offset[1] = LittleFloat(verts[j].weights[k].offset[1]);
				dstWeight.offset[2] = LittleFloat(verts[j].weights[k].offset[2]);

				VectorInverseRotate(verts[j].normal,bones[verts[j].weights[k].bone].matrix ,normal);
				VectorNormalize(normal,dstWeight.normalOffset);
				dstWeight.normalOffset[0] = LittleFloat(verts[j].weights[k].normalOffset[0]);
				dstWeight.normalOffset[1] = LittleFloat(verts[j].weights[k].normalOffset[1]);
				dstWeight.normalOffset[2] = LittleFloat(verts[j].weights[k].normalOffset[2]);

				SafeWrite(dstFile,&dstWeight,sizeof(md4Weight_t));

				if(config.debug) {
					printf( "		md4Weight %i {\n"
							"			boneIndex	%i\n"
							"			boneWeight	%f\n"
							"			offset	(%f, %f, %f)\n"
							"			normalOffset	(%f, %f, %f)\n"
							"		}\n",
							k,
							dstWeight.boneIndex,
							dstWeight.boneWeight,
							dstWeight.offset[0],
							dstWeight.offset[1],
							dstWeight.offset[2],
							dstWeight.normalOffset[0],
							dstWeight.normalOffset[1],
							dstWeight.normalOffset[2]);
				}
			}

			if(config.debug) {
				printf("	}\n");
			}
		}

		// write out triangles
		tris = surfs[i].tris;
		for(j = 0; j < surfs[i].numTris; j++) {

			dstTriangle.indexes[0] = LittleLong(tris[j].verts[0]);
			dstTriangle.indexes[1] = LittleLong(tris[j].verts[1]);
			dstTriangle.indexes[2] = LittleLong(tris[j].verts[2]);

			SafeWrite(dstFile,&dstTriangle,sizeof(md4Triangle_t));

			if(config.debug) {
				printf( "	md4Triangle %i {\n"
						"		indexes	(%i, %i, %i)\n"
						"	}\n",
						j,
						dstTriangle.indexes[0],
						dstTriangle.indexes[1],
						dstTriangle.indexes[2] );
			}
		}

		if(config.debug) {
			printf( "	md4CollapseMap %i {\n",
					i );
		}

		// write out collapse map
		for(j = 0; j < surfs[i].numVerts; j++) {
			cmap[j] = LittleLong(surfs[i].map[j]);

			if(config.debug) {
				printf( "		%i	%i\n",
						j,
						cmap[j] );
			}
		}
		SafeWrite(dstFile,cmap,sizeof(cmap[0]) * surfs[i].numVerts);

		if(config.debug) {
			printf("	}\n");
		}

		if(config.debug) {
			printf("}\n");
		}
	}

	if(config.verbose && !config.debug) {
		int totalTris = 0, totalVerts = 0;

		printf( "  \n");

		for(i = 0; i < smd->numSurfs; i++) {
			totalTris += surfs[i].numTris;
			totalVerts += surfs[i].numVerts;
		}

		// print out file summary
		printf( " stats\n"
				"   size:   %.2f kb\n"
				"   bones:  %i\n"
				"   frames: %i\n"
				"   surfaces:  %i\n"
				"   triangles: %i\n"
				"   vertices:  %i\n",
				ftell(dstFile)/1024.0f,
				smd->numBones,
				smd->numFrames,
				smd->numSurfs,
				totalTris,
				totalVerts );
	}
}
