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
// mathlib.c -- math primitives

#include "cmdlib.h"
#include "mathlib.h"

#ifdef _WIN32
//Improve floating-point consistency.
//without this option weird floating point issues occur
#pragma optimize( "p", on )
#endif


vec3_t vec3_origin = {0,0,0};

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right, right);
	CrossProduct (right, forward, up);
}

void AnglesToMatrix( vec3_t in, vec3_t out[3] ) {
	vec_t sr, sp, sy, cr, cp, cy;

	sy = sin(in[2]);
	cy = cos(in[2]);
	sp = sin(in[1]);
	cp = cos(in[1]);
	sr = sin(in[0]);
	cr = cos(in[0]);

	out[0][0] = cp*cy;
	out[1][0] = cp*sy;
	out[2][0] = -sp;
	out[0][1] = sr*sp*cy+cr*-sy;
	out[1][1] = sr*sp*sy+cr*cy;
	out[2][1] = sr*cp;
	out[0][2] = (cr*sp*cy+-sr*-sy);
	out[1][2] = (cr*sp*sy+-sr*cy);
	out[2][2] = cr*cp;
}

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

void MatrixInvert(vec3_t m[3], vec3_t inv[3])
{
	int i,j;
	vec3_t cofactors[3];
	float determinant;

	    cofactors[0][0]= m[1][1] * m[2][2] - m[2][1] * m[1][2];
    	cofactors[0][1]= -(m[1][0] * m[2][2] - m[1][2] * m[2][0]);
    	cofactors[0][2]= m[1][0] * m[2][1] - m[2][0] * m[1][1];
    	cofactors[1][0]= -(m[0][1] * m[2][2] - m[0][2] * m[2][1]);
    	cofactors[1][1]= m[0][0] * m[2][2] - m[0][2] * m[2][0];
    	cofactors[1][2]= -(m[0][0] * m[2][1] - m[2][0] * m[0][1]);
    	cofactors[2][0]= m[0][1] * m[1][2] - m[0][2] * m[1][1];
    	cofactors[2][1]= -(m[0][0] * m[1][2] - m[1][0] * m[0][2]);
    	cofactors[2][2]= m[0][0] * m[1][1] - m[1][0] * m[0][1];

		determinant = m[0][0] * cofactors[0][0] + m[0][1] * cofactors[0][1] + m[0][2] * m[0][2];
		
		for(i=0;i<3;i++)
		{
			for(j=0;j<3;j++)
			{
				inv[i][j] = cofactors[j][i]/determinant;
			}
		}
}
void VectorInverseRotate( vec3_t v, float r[3][3], vec3_t d ) {
	vec3_t inv[3];
	MatrixInvert(r,inv);
	VectorRotate(v,inv,d);
}

void OldVectorInverseRotate( vec3_t v, float r[3][3], vec3_t d ) {
	d[0] = v[0] * r[0][0] + v[1] * r[1][0] + v[2] * r[2][0];
	d[1] = v[0] * r[0][1] + v[1] * r[1][1] + v[2] * r[2][1];
	d[2] = v[0] * r[0][2] + v[1] * r[1][2] + v[2] * r[2][2];
}

void VectorRotate( vec3_t v, float r[3][3], vec3_t d ) {
	d[0] = v[0] * r[0][0] + v[1] * r[0][1] + v[2] * r[0][2];
	d[1] = v[0] * r[1][0] + v[1] * r[1][1] + v[2] * r[1][2];
	d[2] = v[0] * r[2][0] + v[1] * r[2][1] + v[2] * r[2][2];
}

double VectorLength( const vec3_t v ) {
	int		i;
	double	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

qboolean VectorCompare( const vec3_t v1, const vec3_t v2 ) {
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (fabs(v1[i]-v2[i]) > EQUAL_EPSILON)
			return qfalse;
			
	return qtrue;
}

vec_t Q_rint (vec_t in) {
	return floor (in + 0.5);
}

void VectorMA( const vec3_t va, double scale, const vec3_t vb, vec3_t vc ) {
	vc[0] = va[0] + scale*vb[0];
	vc[1] = va[1] + scale*vb[1];
	vc[2] = va[2] + scale*vb[2];
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross ) {
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorNormalize( const vec3_t in, vec3_t out ) {
	vec_t	length, ilength;

	length = sqrt (in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
	if (length == 0) {
		VectorClear (out);
		return 0;
	}

	ilength = 1.0/length;
	out[0] = in[0]*ilength;
	out[1] = in[1]*ilength;
	out[2] = in[2]*ilength;

	return length;
}

vec_t ColorNormalize( const vec3_t in, vec3_t out ) {
	float	max, scale;

	max = in[0];
	if (in[1] > max)
		max = in[1];
	if (in[2] > max)
		max = in[2];

	if (max == 0) {
		out[0] = out[1] = out[2] = 1.0;
		return 0;
	}

	scale = 1.0 / max;

	VectorScale (in, scale, out);

	return max;
}

void ClearBounds (vec3_t mins, vec3_t maxs) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++) {
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabs( mins[i] );
		b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal ) {
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src ) {
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ ) 	{
		if ( fabs( src[i] ) < minelem ) {
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst, dst );
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;
	float	rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD( degrees );
	zrot[0][0] = cos( rad );
	zrot[0][1] = sin( rad );
	zrot[1][0] = -sin( rad );
	zrot[1][1] = cos( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}
