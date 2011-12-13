#include "tr_local.h"

#define MOTIONBLUR_ALPHA	0.5f

static qboolean	blurReady = qfalse;

void RB_MotionBlur( void ) {
	float vs, vt;
	int vwidth = 1, vheight = 1;

	// No motion blur if it's disabled.
	if ( r_motionBlur->integer == 0 )
		return;
	
	// No motion blur if hardware doesn't allow it.
	if( tr.motionBlurImage == NULL )
		return;

	// No motion blur if the scene is a non-world one.
	if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL )
		return;

	// No motion blur if the motion blur flag is not set
	if ( !( backEnd.refdef.rdflags & RDF_MOTIONBLUR )) {
		// FIX: Because we know this refdef is part of the world scene,
		//      we know the world doesn't use motion blur this frame.
		//      To make sure the next motion blurred sequence initializes
		//      correctly, we disable blurReady to ensure a clear start
		//      frame.
		blurReady = qfalse; 
		return;
	}	

	while (vwidth < glConfig.vidWidth) {
		vwidth *= 2;
	}
	while (vheight < glConfig.vidHeight) {
		vheight *= 2;
	}

	GL_Bind( tr.motionBlurImage );

	if ( blurReady ) {
		vs = (float)glConfig.vidWidth  / vwidth;
		vt = (float)glConfig.vidHeight / vheight;

		qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );		
		qglMatrixMode( GL_PROJECTION );
		qglPushMatrix();
		qglLoadIdentity();
		qglOrtho( 0, glConfig.vidWidth, 0, glConfig.vidHeight, -99999, 99999 );
		qglMatrixMode( GL_MODELVIEW );
		qglPushMatrix();
		qglLoadIdentity();

		qglDisable (GL_DEPTH_TEST);
		qglDisable (GL_CULL_FACE);
		qglDisable (GL_ALPHA_TEST);
		qglEnable(GL_BLEND);
		GL_TexEnv(GL_MODULATE);
		qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		qglColor4f(1.0f, 1.0f, 1.0f, MOTIONBLUR_ALPHA );

		qglBegin( GL_QUADS );
			qglTexCoord2f( 0, 0 );
			qglVertex2f( 0, 0 );
			qglTexCoord2f( vs, 0 );
			qglVertex2f( glConfig.vidWidth, 0 );
			qglTexCoord2f( vs, vt );
			qglVertex2f( glConfig.vidWidth, glConfig.vidHeight );
			qglTexCoord2f( 0, vt );
			qglVertex2f( 0, glConfig.vidHeight );
		qglEnd();

		qglMatrixMode( GL_PROJECTION );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
		qglPopMatrix();
	}

	qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 0, 0, vwidth, vheight, 0 );
    qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	blurReady = qtrue;	
}


