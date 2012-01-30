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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_image.c
#include "tr_local.h"

static byte			 s_intensitytable[256];
static unsigned char s_gammatable[256];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE		1024
static	image_t*		hashTable[FILE_HASH_SIZE];

/*
** R_GammaCorrect
*/
void R_GammaCorrect( byte *buffer, int bufSize ) {
	int i;

	for ( i = 0; i < bufSize; i++ ) {
		buffer[i] = s_gammatable[buffer[i]];
	}
}

typedef struct {
	char *name;
	int	minimize, maximize;
} textureMode_t;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( const char *string ) {
	int		i;
	image_t	*glt;

	for ( i=0 ; i< 6 ; i++ ) {
		if ( !Q_stricmp( modes[i].name, string ) ) {
			break;
		}
	}

	if ( i == 6 ) {
		ri.Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for ( i = 0 ; i < tr.numImages ; i++ ) {
		glt = tr.images[ i ];
		if ( glt->mipmap ) {
			GL_Bind (glt);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
R_SumOfUsedImages
===============
*/
int R_SumOfUsedImages( void ) {
	int	total;
	int i;

	total = 0;
	for ( i = 0; i < tr.numImages; i++ ) {
		if ( tr.images[i]->frameUsed == tr.frameCount ) {
			total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
		}
	}

	return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f( void ) {
	int		i;
	image_t	*image;
	int		texels;
	const char *yesno[] = {
		"no ", "yes"
	};

	ri.Printf (PRINT_ALL, "\n      -w-- -h-- -mm- -TMU- -if-- wrap --name-------\n");
	texels = 0;

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[ i ];

		texels += image->uploadWidth*image->uploadHeight;
		ri.Printf (PRINT_ALL,  "%4i: %4i %4i  %s   %d   ",
			i, image->uploadWidth, image->uploadHeight, yesno[image->mipmap], image->TMU );
		switch ( image->internalFormat ) {
		case 1:
			ri.Printf( PRINT_ALL, "I    " );
			break;
		case 2:
			ri.Printf( PRINT_ALL, "IA   " );
			break;
		case 3:
			ri.Printf( PRINT_ALL, "RGB  " );
			break;
		case 4:
			ri.Printf( PRINT_ALL, "RGBA " );
			break;
		case GL_RGBA8:
			ri.Printf( PRINT_ALL, "RGBA8" );
			break;
		case GL_RGB8:
			ri.Printf( PRINT_ALL, "RGB8" );
			break;
		case GL_RGB4_S3TC:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			ri.Printf( PRINT_ALL, "S3TC " );
			break;
		case GL_RGBA4:
			ri.Printf( PRINT_ALL, "RGBA4" );
			break;
		case GL_RGB5:
			ri.Printf( PRINT_ALL, "RGB5 " );
			break;
		default:
			ri.Printf( PRINT_ALL, "???? " );
		}

		switch ( image->wrapClampMode ) {
		case GL_REPEAT:
			ri.Printf( PRINT_ALL, "rept " );
			break;
		case GL_CLAMP_TO_EDGE:
			ri.Printf( PRINT_ALL, "clmp " );
			break;
		default:
			ri.Printf( PRINT_ALL, "%4i ", image->wrapClampMode );
			break;
		}
		
		ri.Printf( PRINT_ALL, " %s\n", image->imgName );
	}
	ri.Printf (PRINT_ALL, " ---------\n");
	ri.Printf (PRINT_ALL, " %i total texels (not including mipmaps)\n", texels);
	ri.Printf (PRINT_ALL, " %i total images\n\n", tr.numImages );
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function 
before or after.
================
*/
static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,  
							int outwidth, int outheight ) {
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[2048], p2[2048];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for ( i=0 ; i<outwidth ; i++ ) {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for ( i=0 ; i<outwidth ; i++ ) {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth) {
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++) {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		if ( !glConfig.deviceSupportsGamma )
		{
			int		i, c;
			byte	*p;

			p = (byte *)in;

			c = inwidth*inheight;
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[p[0]];
				p[1] = s_gammatable[p[1]];
				p[2] = s_gammatable[p[2]];
			}
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;

		if ( glConfig.deviceSupportsGamma )
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_intensitytable[p[0]];
				p[1] = s_intensitytable[p[1]];
				p[2] = s_intensitytable[p[2]];
			}
		}
		else
		{
			for (i=0 ; i<c ; i++, p+=4)
			{
				p[0] = s_gammatable[s_intensitytable[p[0]]];
				p[1] = s_gammatable[s_intensitytable[p[1]]];
				p[2] = s_gammatable[s_intensitytable[p[2]]];
			}
		}
	}
}


/*
================
R_MipMap

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap( unsigned *in, int inWidth, int inHeight ) {
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = ri.Hunk_AllocateTempMemory( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (byte *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total = 
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	Com_Memcpy( in, temp, outWidth * outHeight * 4 );
	ri.Hunk_FreeTempMemory( temp );
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, byte blend[4] ) {
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data+=4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};


/*
================
R_CreateCubeImage

This is the only way any image_t are created
================
*/

static image_t *R_AllocImage(const char *name) {
	image_t		*image;
	long		hash;

	if (strlen(name) >= MAX_QPATH)
		ri.Error(ERR_DROP, "R_AllocImage: Image name exceeds MAX_QPATH\n", name);

	if (tr.numImages == MAX_DRAWIMAGES)
		ri.Error(ERR_DROP, "R_AllocImage: MAX_DRAWIMAGES hit\n");

	image = tr.images[tr.numImages] = ri.Hunk_Alloc(sizeof(image_t), h_low);
	Com_Memset(image, 0, sizeof(image_t));

	image->texnum = 1024 + tr.numImages;
	tr.numImages++;

	strcpy(image->imgName, name);

	hash = generateHashValue(name);
	image->next = hashTable[hash];
	hashTable[hash] = image;

	return image;
}

/*
 * Upload32
 * Upload 32 bit image
 */
static void R_UploadImage(const byte **dataArray, int numData, qboolean mipmap, qboolean picmip, image_t *image) {
	const byte	*data = dataArray[0];
	byte		*scaledBuffer;
	int			scaledWidth, scaledHeight;
	GLenum		internalFormat;
	float		rMax, gMax, bMax;
	int			samples;
	int			i;

	if (glConfig.textureNPOT) {
		scaledWidth		= image->width;
		scaledHeight	= image->height;
	} else {
		/* convert to exact power of 2 sizes */
		for (scaledWidth = 1; scaledWidth < image->width; scaledWidth <<= 1);
		for (scaledHeight = 1; scaledHeight < image->height; scaledHeight <<= 1);
	}

	if (r_roundImagesDown->integer && scaledWidth > image->width)
		scaledWidth >>= 1;

	if (r_roundImagesDown->integer && scaledHeight > image->height)
		scaledHeight >>= 1;

	/* clamp to minimum size */
	if (scaledWidth < 1)
		scaledWidth = 1;

	if (scaledHeight < 1)
		scaledHeight = 1;

	/* perform optional picmip operation */
	if (picmip) {
		scaledWidth		>>= r_picmip->integer;
		scaledHeight	>>= r_picmip->integer;
	}

	/* clamp to the current upper OpenGL limit */
	while (scaledWidth > glConfig.maxTextureSize || scaledHeight > glConfig.maxTextureSize) {
		scaledWidth		>>= 1;
		scaledHeight	>>= 1;
	}

	/* scan the texture for each channel's max values and verify if the alpha channel is being used or not */
	samples	= 3;

	if(image->TMU) {
		internalFormat = GL_RGB;
	} else {
		rMax = data[0];
		gMax = data[1];
		bMax = data[2];

		for (i = 0; i < image->width * image->height; i++) {
			if (data[i * 4 + 0] > rMax)
				rMax = data[i * 4 + 0];

			if (data[i * 4 + 1] > gMax)
				gMax = data[i * 4 + 1];

			if (data[i * 4 + 2] > bMax)
				bMax = data[i * 4 + 2];

			if (data[i * 4 + 3] != 255) {
				samples = 4;
				break;
			}
		}

		/* select proper internal format */
		if (samples == 3) {
			if (glConfig.textureCompression == TC_S3TC_ARB)
				internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			else if (glConfig.textureCompression == TC_S3TC)
				internalFormat = GL_RGB4_S3TC;
			else if (r_texturebits->integer == 16)
				internalFormat = GL_RGB5;
			else if (r_texturebits->integer == 32)
				internalFormat = GL_RGB8;
			else
				internalFormat = GL_RGB;
		} else if (samples == 4) {
			if (r_texturebits->integer == 16)
				internalFormat = GL_RGBA4;
			else if (r_texturebits->integer == 32)
				internalFormat = GL_RGBA8;
			else
				internalFormat = GL_RGBA;
		}
	}

	/* allocate memory */
	scaledBuffer = ri.Hunk_AllocateTempMemory(scaledWidth * scaledHeight * 4);

	for(i = 0, data = dataArray[0]; i < numData; data += image->width * image->height * 4, i++) {
		if (scaledWidth == image->width && scaledHeight == image->height)
			Com_Memcpy(scaledBuffer, data, scaledWidth * scaledHeight * 4);
		else
			ResampleTexture((unsigned *)data, image->width, image->height, (unsigned *)scaledBuffer, scaledWidth, scaledHeight);

		R_LightScaleTexture((unsigned *)scaledBuffer, scaledWidth, scaledHeight, !image->mipmap);

		image->uploadWidth		= scaledWidth;
		image->uploadHeight		= scaledHeight;
		image->internalFormat	= internalFormat;

		switch (image->type) {
			case GL_TEXTURE_CUBE_MAP_ARB:
				qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, 0, internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
				break;
			default:
				qglTexImage2D(GL_TEXTURE_2D, 0, internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
		}

		if (image->mipmap) {
			int	 miplevel = 0;

			while (scaledWidth > 1 || scaledHeight > 1) {
				R_MipMap((unsigned *)scaledBuffer, scaledWidth, scaledHeight);

				scaledWidth		>>= 1;
				scaledHeight	>>= 1;

				if (scaledWidth < 1)
					scaledWidth = 1;

				if (scaledHeight < 1)
					scaledHeight = 1;

				miplevel++;

				if (r_colorMipLevels->integer)
					R_BlendOverTexture(scaledBuffer, scaledWidth * scaledHeight, mipBlendColors[miplevel]);

				switch (image->type) {
					case GL_TEXTURE_CUBE_MAP_ARB:
						qglTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + i, miplevel, internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
						break;
					default:
						qglTexImage2D(GL_TEXTURE_2D, miplevel, internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer);
				}
			}
		}
	}

	if (image->mipmap) {
		if (glConfig.textureFilterAnisotropic)
			qglTexParameteri(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, (GLint)Com_Clamp(1, glConfig.maxAnisotropy, r_ext_max_anisotropy->integer));

		qglTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	} else {
		if (glConfig.textureFilterAnisotropic)
			qglTexParameteri(image->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);

		qglTexParameterf(image->type, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(image->type, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	GL_CheckErrors();

	qglTexParameterf(image->type, GL_TEXTURE_WRAP_S, image->wrapClampMode);
	qglTexParameterf(image->type, GL_TEXTURE_WRAP_T, image->wrapClampMode);

	if (scaledBuffer != 0)
		ri.Hunk_FreeTempMemory(scaledBuffer);
}

image_t *R_CreateImage(const char *name, const byte *pic, int width, int height, qboolean mipmap, qboolean picmip, int glWrapClampMode) {
	image_t		*image;

	/* allocate image */
	image = R_AllocImage(name);

	/* set image information */
	image->mipmap			= mipmap;
	image->picmip			= picmip;

	image->type				= GL_TEXTURE_2D;

	image->width			= width;
	image->height			= height;
	image->wrapClampMode	= glWrapClampMode;

	/* lightmaps are always allocated on TMU 1 */
	if(!strncmp(name, "*lightmap", 9))
		image->TMU = 1;
	else
		image->TMU = 0;

	/* upload image data */
	GL_SelectTexture(image->TMU);

	GL_Bind(image);

	R_UploadImage((const byte **)&pic, 1, mipmap, picmip, image);

	qglBindTexture(image->type, 0);

	if (image->TMU)
		GL_SelectTexture(0);

	return image;
}

image_t *R_CreateCubeImage(const char *name, const byte **pics, int width, int height, qboolean mipmap, qboolean picmip, int glWrapClampMode) {
	image_t		*image;

	/* allocate image */
	image = R_AllocImage(name);

	/* set image information */
	image->mipmap			= mipmap;
	image->picmip			= picmip;

	image->type				= GL_TEXTURE_CUBE_MAP_ARB;

	image->width			= width;
	image->height			= height;
	image->wrapClampMode	= glWrapClampMode;

	image->TMU				= 0;

	/* upload image data */
	GL_SelectTexture(image->TMU);

	GL_Bind(image);

	R_UploadImage(pics, 6, mipmap, picmip, image);

	qglBindTexture(image->type, 0);

	return image;
}

//===================================================================

typedef struct
{
	char *ext;
	void (*ImageLoader)( const char *, unsigned char **, int *, int * );
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static imageExtToLoaderMap_t imageLoaders[ ] =
{
	{ "tga",  R_LoadTGA },
	{ "jpg",  R_LoadJPG },
	{ "png",  R_LoadPNG }
};

static int numImageLoaders = ARRAY_LEN( imageLoaders );

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage( const char *name, byte **pic, int *width, int *height )
{
	qboolean orgNameFailed = qfalse;
	int orgLoader = -1;
	int i;
	char localName[ MAX_QPATH ];
	const char *ext;
	char *altName;

	*pic = NULL;
	*width = 0;
	*height = 0;

	Q_strncpyz( localName, name, MAX_QPATH );

	ext = COM_GetExtension( localName );

	if( *ext )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !Q_stricmp( ext, imageLoaders[ i ].ext ) )
			{
				// Load
				imageLoaders[ i ].ImageLoader( localName, pic, width, height );
				break;
			}
		}

		// A loader was found
		if( i < numImageLoaders )
		{
			if( *pic == NULL )
			{
				// Loader failed, most likely because the file isn't there;
				// try again without the extension
				orgNameFailed = qtrue;
				orgLoader = i;
				COM_StripExtension( name, localName, MAX_QPATH );
			}
			else
			{
				// Something loaded
				return;
			}
		}
	}

	// Try and find a suitable match using all
	// the image formats supported
	for( i = 0; i < numImageLoaders; i++ )
	{
		if (i == orgLoader)
			continue;

		altName = va( "%s.%s", localName, imageLoaders[ i ].ext );

		// Load
		imageLoaders[ i ].ImageLoader( altName, pic, width, height );

		if( *pic )
		{
			if( orgNameFailed )
			{
				ri.Printf( PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n",
						name, altName );
			}

			break;
		}
	}
}


/*
===============
R_FindImageFile

Finds or loads the given image.
Returns NULL if it fails, not a default image.
==============
*/
image_t	*R_FindImageFile( const char *name, qboolean mipmap, qboolean picmip, int glWrapClampMode ) {
	image_t	*image;
	int		width, height;
	byte	*pic;
	long	hash;

	if (!name) {
		return NULL;
	}

	hash = generateHashValue(name);

	//
	// see if the image is already loaded
	//
	for (image=hashTable[hash]; image; image=image->next) {
		if ( !strcmp( name, image->imgName ) ) {
			// the white image can be used with any set of parms, but other mismatches are errors
			if ( strcmp( name, "*white" ) ) {
				if ( image->mipmap != mipmap ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed mipmap parm\n", name );
				}
				if ( image->picmip != picmip ) {
					ri.Printf( PRINT_DEVELOPER, "WARNING: reused image %s with mixed allowPicmip parm\n", name );
				}
				if ( image->wrapClampMode != glWrapClampMode ) {
					ri.Printf( PRINT_ALL, "WARNING: reused image %s with mixed glWrapClampMode parm\n", name );
				}
			}
			return image;
		}
	}

	//
	// load the pic from disk
	//
	R_LoadImage( name, &pic, &width, &height );
	if ( pic == NULL ) {
		return NULL;
	}

	image = R_CreateImage( ( char * ) name, pic, width, height, mipmap, picmip, glWrapClampMode );
	ri.Free( pic );
	return image;
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable( void ) {
	int		i;
	float	d;
	float	exp;
	
	exp = 0.5;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ ) {
		d = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );

		tr.fogTable[i] = d;
	}
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float	R_FogFactor( float s, float t ) {
	float	d;

	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	d = tr.fogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

/*
================
R_CreateFogImage
================
*/
#define	FOG_S	256
#define	FOG_T	32
static void R_CreateFogImage( void ) {
	int		x,y;
	byte	*data;
	float	d;
	float	borderColor[4];

	data = ri.Hunk_AllocateTempMemory( FOG_S * FOG_T * 4 );

	// S is distance, T is depth
	for (x=0 ; x<FOG_S ; x++) {
		for (y=0 ; y<FOG_T ; y++) {
			d = R_FogFactor( ( x + 0.5f ) / FOG_S, ( y + 0.5f ) / FOG_T );

			data[(y*FOG_S+x)*4+0] = 
			data[(y*FOG_S+x)*4+1] = 
			data[(y*FOG_S+x)*4+2] = 255;
			data[(y*FOG_S+x)*4+3] = 255*d;
		}
	}
	// standard openGL clamping doesn't really do what we want -- it includes
	// the border color at the edges.  OpenGL 1.2 has clamp-to-edge, which does
	// what we want.
	tr.fogImage = R_CreateImage("*fog", (byte *)data, FOG_S, FOG_T, qfalse, qfalse, GL_CLAMP_TO_EDGE );
	ri.Hunk_FreeTempMemory( data );

	borderColor[0] = 1.0;
	borderColor[1] = 1.0;
	borderColor[2] = 1.0;
	borderColor[3] = 1;

	qglTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
}

/*
==================
R_CreateDefaultImage
==================
*/
#define	DEFAULT_SIZE	16
static void R_CreateDefaultImage( void ) {
	int		x;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset( data, 32, sizeof( data ) );
	for ( x = 0 ; x < DEFAULT_SIZE ; x++ ) {
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE-1][x][0] =
		data[DEFAULT_SIZE-1][x][1] =
		data[DEFAULT_SIZE-1][x][2] =
		data[DEFAULT_SIZE-1][x][3] = 255;

		data[x][DEFAULT_SIZE-1][0] =
		data[x][DEFAULT_SIZE-1][1] =
		data[x][DEFAULT_SIZE-1][2] =
		data[x][DEFAULT_SIZE-1][3] = 255;
	}
	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qtrue, GL_REPEAT );
}

/*
 * R_CreateBlackImage
 * Create black image
 */
static void R_CreateBlackImage(void) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	int		i, j;

	/* black image */
	Com_Memset(data, 0, sizeof(data));
	for (i = 0; i < DEFAULT_SIZE; i++) {
		for (j = 0; j < DEFAULT_SIZE; j++) {
			data[i][j][3] = 255;
		}
	}

	tr.blackImage = R_CreateImage("*black", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qtrue, GL_REPEAT);
}

/*
 * R_CreateFlatImage
 * Create flat dot3 image
 */
static void R_CreateFlatImage(void) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	int		i, j;

	/* purple image */
	for (i = 0; i < DEFAULT_SIZE; i++) {
		for (j = 0; j < DEFAULT_SIZE; j++) {
			data[i][j][0] = 127;
			data[i][j][1] = 127;
			data[i][j][2] = 255;
			data[i][j][3] = 255;
		}
	}

	tr.flatImage = R_CreateImage("*flat", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qtrue, qtrue, GL_REPEAT);
}

/*
 * R_CreateDefaultCubeImage
 * Create default cube map
 */
static void R_CreateDefaultCubeImage(void) {
	byte	*images;

	/* just a black cube */
	images = ri.Hunk_AllocateTempMemory(6 * DEFAULT_SIZE * DEFAULT_SIZE * 4);
	Com_Memset(images, 0, 6 * DEFAULT_SIZE * DEFAULT_SIZE * 4);

	tr.defaultCubeImage = R_CreateCubeImage("*cube", (const byte **)&images, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qfalse, GL_REPEAT);

	ri.Hunk_FreeTempMemory(images);
}

/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages( void ) {
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	int		i;

	R_CreateDefaultImage();
	R_CreateBlackImage();
	R_CreateFlatImage();
	R_CreateDefaultCubeImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset( data, 255, sizeof( data ) );
	tr.whiteImage = R_CreateImage("*white", (byte *)data, 8, 8, qfalse, qfalse, GL_REPEAT );

	for(i=0;i<32;i++) {
		// scratchimage is usually used for cinematic drawing
		tr.scratchImage[i] = R_CreateImage("*scratch", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, qfalse, qfalse, GL_CLAMP_TO_EDGE );
	}

	R_CreateFogImage();
}


/*
 * R_SetColorMappings
 * Set gamma
 */
void R_SetColorMappings(void) {
	int		i, j;
	float	g;
	int		inf;

	tr.identityLight		= 1.0f;
	tr.identityLightByte	= 255;


	if (r_intensity->value <= 1)
		ri.Cvar_Set("r_intensity", "1");


	if (r_gamma->value < 0.5f)
		ri.Cvar_Set("r_gamma", "0.5");
	else if (r_gamma->value > 3.0f)
		ri.Cvar_Set("r_gamma", "3.0");


	g = r_gamma->value;

	for (i = 0; i < 256; i++) {
		if (g == 1)
			inf = i;
		else
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;

		if (inf < 0)
			inf = 0;

		if (inf > 255)
			inf = 255;

		s_gammatable[i] = inf;
	}

	for (i=0; i < 256; i++) {
		j = i * r_intensity->value;
		if (j > 255)
			j = 255;

		s_intensitytable[i] = j;
	}

	if (glConfig.deviceSupportsGamma)
		GLimp_SetGamma(s_gammatable, s_gammatable, s_gammatable);
}

/*
===============
R_InitImages
===============
*/
void	R_InitImages( void ) {
	Com_Memset(hashTable, 0, sizeof(hashTable));
	// build brightness translation tables
	R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures( void ) {
	int		i;

	for ( i=0; i<tr.numImages ; i++ ) {
		qglDeleteTextures( 1, &tr.images[i]->texnum );
	}
	Com_Memset( tr.images, 0, sizeof( tr.images ) );

	tr.numImages = 0;

	Com_Memset( glState.currentTextures, 0, sizeof( glState.currentTextures ) );

	for (i = 0; i < glConfig.numTextureUnits; i++) {
		GL_SelectTexture(i);
		qglBindTexture(GL_TEXTURE_2D, 0);
	}
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char *CommaParse( char **data_p ) {
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if ( !data ) {
		*data_p = NULL;
		return com_token;
	}

	while ( 1 ) {
		// skip whitespace
		while( (c = *data) <= ' ') {
			if( !c ) {
				break;
			}
			data++;
		}


		c = *data;

		// skip double slash comments
		if ( c == '/' && data[1] == '/' )
		{
			while (*data && *data != '\n')
				data++;
		}
		// skip /* */ comments
		else if ( c=='/' && data[1] == '*' ) 
		{
			while ( *data && ( *data != '*' || data[1] != '/' ) ) 
			{
				data++;
			}
			if ( *data ) 
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 ) {
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32 && c != ',' );

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = ( char * ) data;
	return com_token;
}


/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name ) {
	qhandle_t	hSkin;
	skin_t		*skin;
	skinSurface_t	*surf;
	union {
		char *c;
		void *v;
	} text;
	char		*text_p;
	char		*token;
	char		surfName[MAX_QPATH];

	if ( !name || !name[0] ) {
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}


	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins ; hSkin++ ) {
		skin = tr.skins[hSkin];
		if ( !Q_stricmp( skin->name, name ) ) {
			if( skin->numSurfaces == 0 ) {
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}
	tr.numSkins++;
	skin = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	tr.skins[hSkin] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if ( strcmp( name + strlen( name ) - 5, ".skin" ) ) {
		skin->numSurfaces = 1;
		skin->surfaces[0] = ri.Hunk_Alloc( sizeof(skin->surfaces[0]), h_low );
		skin->surfaces[0]->shader = R_FindShader( name, LIGHTMAP_NONE, qtrue );
		return hSkin;
	}

	// load and parse the skin file
    ri.FS_ReadFile( name, &text.v );
	if ( !text.c ) {
		return 0;
	}

	text_p = text.c;
	while ( text_p && *text_p ) {
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[0] ) {
			break;
		}
		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' ) {
			text_p++;
		}

		if ( strstr( token, "tag_" ) ) {
			continue;
		}
		
		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = ri.Hunk_Alloc( sizeof( *skin->surfaces[0] ), h_low );
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );
		surf->shader = R_FindShader( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text.v );


	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 ) {
		return 0;		// use default skin
	}

	return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void	R_InitSkins( void ) {
	skin_t		*skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[0] = ri.Hunk_Alloc( sizeof( skin_t ), h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name )  );
	skin->numSurfaces = 1;
	skin->surfaces[0] = ri.Hunk_Alloc( sizeof( *skin->surfaces ), h_low );
	skin->surfaces[0]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t	*R_GetSkinByHandle( qhandle_t hSkin ) {
	if ( hSkin < 1 || hSkin >= tr.numSkins ) {
		return tr.skins[0];
	}
	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void	R_SkinList_f( void ) {
	int			i, j;
	skin_t		*skin;

	ri.Printf (PRINT_ALL, "------------------\n");

	for ( i = 0 ; i < tr.numSkins ; i++ ) {
		skin = tr.skins[i];

		ri.Printf( PRINT_ALL, "%3i:%s\n", i, skin->name );
		for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
			ri.Printf( PRINT_ALL, "       %s = %s\n", 
				skin->surfaces[j]->name, skin->surfaces[j]->shader->name );
		}
	}
	ri.Printf (PRINT_ALL, "------------------\n");
}

