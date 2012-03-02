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
//
// q_math.c -- stateless support routines that are included in each code module

// Some of the vector functions are static inline in q_shared.h. q3asm
// doesn't understand static functions though, so we only want them in
// one file. That's what this is about.
#ifdef Q3_VM
#define __Q3_VM_MATH
#endif

#include "q_shared.h"

vec3_t	vec3_origin = {0,0,0};
vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

vec4_t	g_color_table[8] =
	{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	};


vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}


// this isn't a real cheap function to call!
int DirToByte( vec3_t dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy (bytedirs[b], dir);
}


unsigned ColorBytes3 (float r, float g, float b) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max;
	
	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
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

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

/*
=================
AxisToAngles

Takes an axis (forward + right + up)
and returns angles -- including a roll
=================
*/
void AxisToAngles( vec3_t axis[3], vec3_t angles ) {
	float length1;
	float yaw, pitch, roll = 0.0f;

	if ( axis[0][1] == 0 && axis[0][0] == 0 ) {
		yaw = 0;
		if ( axis[0][2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( axis[0][0] ) {
			yaw = ( atan2 ( axis[0][1], axis[0][0] ) * 180 / M_PI );
		}
		else if ( axis[0][1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		length1 = sqrt ( axis[0][0]*axis[0][0] + axis[0][1]*axis[0][1] );
		pitch = ( atan2(axis[0][2], length1) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}

		roll = ( atan2( axis[1][2], axis[2][2] ) * 180 / M_PI );
		if ( roll < 0 ) {
			roll += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = roll;
}

/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
#ifndef Q3_VM
	assert( Q_fabs(inv_denom) != 0.0f ); // zero vectors get here
#endif
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

//============================================================================

#if !idppc
/*
** float q_rsqrt( float number )
*/
float Q_rsqrt( float number )
{
	floatint_t t;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	t.f  = number;
	t.i  = 0x5f3759df - ( t.i >> 1 );               // what the fuck?
	y  = t.f;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

float Q_fabs( float f ) {
	floatint_t fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}
#endif

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


float	AngleMod(float a) {
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}


/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist[2];
	int		sides, b, i;

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	// general case
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (i=0 ; i<3 ; i++)
		{
			b = (p->signbits >> i) & 1;
			dist[ b] += p->normal[i]*emaxs[i];
			dist[!b] += p->normal[i]*emins[i];
		}
	}

	sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)
		sides |= 2;

	return sides;
}


/*
=================
RadiusFromBounds
=================
*/
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


void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}

qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs,
		const vec3_t mins2, const vec3_t maxs2)
{
	if ( maxs[0] < mins2[0] ||
		maxs[1] < mins2[1] ||
		maxs[2] < mins2[2] ||
		mins[0] > maxs2[0] ||
		mins[1] > maxs2[1] ||
		mins[2] > maxs2[2])
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs,
		const vec3_t origin, vec_t radius)
{
	if ( origin[0] - radius > maxs[0] ||
		origin[0] + radius < mins[0] ||
		origin[1] - radius > maxs[1] ||
		origin[1] + radius < mins[1] ||
		origin[2] - radius > maxs[2] ||
		origin[2] + radius < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs,
		const vec3_t origin)
{
	if ( origin[0] > maxs[0] ||
		origin[0] < mins[0] ||
		origin[1] > maxs[1] ||
		origin[1] < mins[1] ||
		origin[2] > maxs[2] ||
		origin[2] < mins[2])
	{
		return qfalse;
	}

	return qtrue;
}

vec_t VectorNormalize( vec3_t v ) {
	// NOTE: TTimo - Apple G4 altivec source uses double?
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if ( length ) {
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (length)
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
		VectorClear( out );
	}
		
	return length;

}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}



/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}
*/

/*
=================
Matrix4Compare
=================
*/
qboolean Matrix4Compare(const float a[16], const float b[16])
{
	int i;

	for ( i = 0; i < 16; i++ )
		if ( a[i] != b[i] )
			return qfalse;

	return qtrue;
}

/*
=================
Matrix4Copy
=================
*/
void Matrix4Copy(const float in[16], float out[16])
{
#if id386_sse && defined __GNUC__ && 0
	asm volatile
	(
	"movups         (%%edx),        %%xmm0\n"
	"movups         0x10(%%edx),    %%xmm1\n"
	"movups         0x20(%%edx),    %%xmm2\n"
	"movups         0x30(%%edx),    %%xmm3\n"

	"movups         %%xmm0,         (%%eax)\n"
	"movups         %%xmm1,         0x10(%%eax)\n"
	"movups         %%xmm2,         0x20(%%eax)\n"
	"movups         %%xmm3,         0x30(%%eax)\n"
	:
	: "a"( out ), "d"( in )
	: "memory"
		);
#elif id386_3dnow && defined __GNUC__
	asm volatile
	(
	"femms\n"
	"movq           (%%edx),        %%mm0\n"
	"movq           8(%%edx),       %%mm1\n"
	"movq           16(%%edx),      %%mm2\n"
	"movq           24(%%edx),      %%mm3\n"
	"movq           32(%%edx),      %%mm4\n"
	"movq           40(%%edx),      %%mm5\n"
	"movq           48(%%edx),      %%mm6\n"
	"movq           56(%%edx),      %%mm7\n"

	"movq           %%mm0,          (%%eax)\n"
	"movq           %%mm1,          8(%%eax)\n"
	"movq           %%mm2,          16(%%eax)\n"
	"movq           %%mm3,          24(%%eax)\n"
	"movq           %%mm4,          32(%%eax)\n"
	"movq           %%mm5,          40(%%eax)\n"
	"movq           %%mm6,          48(%%eax)\n"
	"movq           %%mm7,          56(%%eax)\n"
	"femms\n"
	:
	: "a"(out), "d"(in)
	: "memory"
	);
#else
        out[ 0] = in[ 0];       out[ 4] = in[ 4];       out[ 8] = in[ 8];       out[12] = in[12];
        out[ 1] = in[ 1];       out[ 5] = in[ 5];       out[ 9] = in[ 9];       out[13] = in[13];
		out[ 2] = in[ 2];       out[ 6] = in[ 6];       out[10] = in[10];       out[14] = in[14];
		out[ 3] = in[ 3];       out[ 7] = in[ 7];       out[11] = in[11];       out[15] = in[15];
#endif
}

/*
=================
Matrix4Multiply
=================
*/
void Matrix4Multiply(const float a[16], const float b[16], float out[16])
{
#if id386_sse
	int				i;
	__m128			_t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7;

	_t4 = _mm_loadu_ps(&a[0]);
	_t5 = _mm_loadu_ps(&a[4]);
	_t6 = _mm_loadu_ps(&a[8]);
	_t7 = _mm_loadu_ps(&a[12]);

	for(i = 0; i < 4; i++)
	{
		_t0 = _mm_load1_ps(&b[i * 4 + 0]);
		_t0 = _mm_mul_ps(_t4, _t0);

		_t1 = _mm_load1_ps(&b[i * 4 + 1]);
		_t1 = _mm_mul_ps(_t5, _t1);

		_t2 = _mm_load1_ps(&b[i * 4 + 2]);
		_t2 = _mm_mul_ps(_t6, _t2);

		_t3 = _mm_load1_ps(&b[i * 4 + 3]);
		_t3 = _mm_mul_ps(_t7, _t3);

		_t1 = _mm_add_ps(_t0, _t1);
		_t2 = _mm_add_ps(_t1, _t2);
		_t3 = _mm_add_ps(_t2, _t3);

		_mm_storeu_ps(&out[i * 4], _t3);
	}

#else
        out[ 0] = b[ 0]*a[ 0] + b[ 1]*a[ 4] + b[ 2]*a[ 8] + b[ 3]*a[12];
        out[ 1] = b[ 0]*a[ 1] + b[ 1]*a[ 5] + b[ 2]*a[ 9] + b[ 3]*a[13];
		out[ 2] = b[ 0]*a[ 2] + b[ 1]*a[ 6] + b[ 2]*a[10] + b[ 3]*a[14];
		out[ 3] = b[ 0]*a[ 3] + b[ 1]*a[ 7] + b[ 2]*a[11] + b[ 3]*a[15];

		out[ 4] = b[ 4]*a[ 0] + b[ 5]*a[ 4] + b[ 6]*a[ 8] + b[ 7]*a[12];
		out[ 5] = b[ 4]*a[ 1] + b[ 5]*a[ 5] + b[ 6]*a[ 9] + b[ 7]*a[13];
		out[ 6] = b[ 4]*a[ 2] + b[ 5]*a[ 6] + b[ 6]*a[10] + b[ 7]*a[14];
		out[ 7] = b[ 4]*a[ 3] + b[ 5]*a[ 7] + b[ 6]*a[11] + b[ 7]*a[15];

		out[ 8] = b[ 8]*a[ 0] + b[ 9]*a[ 4] + b[10]*a[ 8] + b[11]*a[12];
		out[ 9] = b[ 8]*a[ 1] + b[ 9]*a[ 5] + b[10]*a[ 9] + b[11]*a[13];
		out[10] = b[ 8]*a[ 2] + b[ 9]*a[ 6] + b[10]*a[10] + b[11]*a[14];
		out[11] = b[ 8]*a[ 3] + b[ 9]*a[ 7] + b[10]*a[11] + b[11]*a[15];

		out[12] = b[12]*a[ 0] + b[13]*a[ 4] + b[14]*a[ 8] + b[15]*a[12];
		out[13] = b[12]*a[ 1] + b[13]*a[ 5] + b[14]*a[ 9] + b[15]*a[13];
		out[14] = b[12]*a[ 2] + b[13]*a[ 6] + b[14]*a[10] + b[15]*a[14];
		out[15] = b[12]*a[ 3] + b[13]*a[ 7] + b[14]*a[11] + b[15]*a[15];
#endif
}

/*
================
MatrixMultiply
================
*/
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
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
	VectorNormalize( dst );
}

/*
================
Q_isnan

Don't pass doubles to this
================
*/
int Q_isnan( float x )
{
	floatint_t fi;

	fi.f = x;
	fi.ui &= 0x7FFFFFFF;
	fi.ui = 0x7F800000 - fi.ui;

	return (int)( (unsigned int)fi.ui >> 31 );
}

// ADDING FOR ZEQ2

void AnglesToQuat (const vec3_t angles, vec4_t quat) {
	vec3_t a;
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;

	a[PITCH] = (M_PI/360.0) * angles[PITCH];
	a[YAW] = (M_PI/360.0) * angles[YAW];
	a[ROLL] = (M_PI/360.0) * angles[ROLL];

	cr = cos(a[ROLL]);
	cp = cos(a[PITCH]);
	cy = cos(a[YAW]);

	sr = sin(a[ROLL]);
	sp = sin(a[PITCH]);
	sy = sin(a[YAW]);

	cpcy = cp * cy;
	spsy = sp * sy;
	quat[0] = cr * cpcy + sr * spsy; // w
	quat[1] = sr * cpcy - cr * spsy; // x
	quat[2] = cr * sp * cy + sr * cp * sy; // y
	quat[3] = cr * cp * sy - sr * sp * cy; // z
}

void QuatToAxis(vec4_t q, vec3_t axis[3]) {
	float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;

	x2 = q[1] + q[1];
	y2 = q[2] + q[2];
	z2 = q[3] + q[3];
	xx = q[1] * x2;
	xy = q[1] * y2;
	xz = q[1] * z2;
	yy = q[2] * y2;
	yz = q[2] * z2;
	zz = q[3] * z2;
	wx = q[0] * x2;
	wy = q[0] * y2;
	wz = q[0] * z2;

	axis[0][0] = 1.0 - (yy + zz);
	axis[1][0] = xy - wz;
	axis[2][0] = xz + wy;

	axis[0][1] = xy + wz;
	axis[1][1] = 1.0 - (xx + zz);
	axis[2][1] = yz - wx;

	axis[0][2] = xz - wy;
	axis[1][2] = yz + wx;
	axis[2][2] = 1.0 - (xx + yy);
}

// NOTE: If we only want the 'forward' component of the axes,
//       save us some calculations
void QuatToVector(vec4_t q, vec3_t outVec) {
	float wy, wz, yy, xy, xz, zz, y2, z2;

	y2 = q[2] + q[2];
	z2 = q[3] + q[3];
	xy = q[1] * y2;
	xz = q[1] * z2;
	yy = q[2] * y2;
	zz = q[3] * z2;
	wy = q[0] * y2;
	wz = q[0] * z2;

	outVec[0] = 1.0 - (yy + zz);
	outVec[1] = xy + wz;
	outVec[2] = xz - wy;
}

void QuatMul(const vec4_t q1, const vec4_t q2, vec4_t res) {
	float A, B, C, D, E, F, G, H;

	A = (q1[0] + q1[1])*(q2[0] + q2[1]);
	B = (q1[3] - q1[2])*(q2[2] - q2[3]);
	C = (q1[0] - q1[1])*(q2[2] + q2[3]);
	D = (q1[2] + q1[3])*(q2[0] - q2[1]);
	E = (q1[1] + q1[3])*(q2[1] + q2[2]);
	F = (q1[1] - q1[3])*(q2[1] - q2[2]);
	G = (q1[0] + q1[2])*(q2[0] - q2[3]);
	H = (q1[0] - q1[2])*(q2[0] + q2[3]);

	res[0] = B + (H - E - F + G)*0.5;
	res[1] = A - (E + F + G + H)*0.5;
	res[2] = C + (E - F + G - H)*0.5;
	res[3] = D + (E - F - G + H)*0.5;
}

// Hooray for stupidity; have to copy this from bg_lib.c or the VM compiler chokes.
double hack_acostable[] = {
3.14159265,3.07908248,3.05317551,3.03328655,3.01651113,3.00172442,2.98834964,2.97604422,
2.96458497,2.95381690,2.94362719,2.93393068,2.92466119,2.91576615,2.90720289,2.89893629,
2.89093699,2.88318015,2.87564455,2.86831188,2.86116621,2.85419358,2.84738169,2.84071962,
2.83419760,2.82780691,2.82153967,2.81538876,2.80934770,2.80341062,2.79757211,2.79182724,
2.78617145,2.78060056,2.77511069,2.76969824,2.76435988,2.75909250,2.75389319,2.74875926,
2.74368816,2.73867752,2.73372510,2.72882880,2.72398665,2.71919677,2.71445741,2.70976688,
2.70512362,2.70052613,2.69597298,2.69146283,2.68699438,2.68256642,2.67817778,2.67382735,
2.66951407,2.66523692,2.66099493,2.65678719,2.65261279,2.64847088,2.64436066,2.64028133,
2.63623214,2.63221238,2.62822133,2.62425835,2.62032277,2.61641398,2.61253138,2.60867440,
2.60484248,2.60103507,2.59725167,2.59349176,2.58975488,2.58604053,2.58234828,2.57867769,
2.57502832,2.57139977,2.56779164,2.56420354,2.56063509,2.55708594,2.55355572,2.55004409,
2.54655073,2.54307530,2.53961750,2.53617701,2.53275354,2.52934680,2.52595650,2.52258238,
2.51922417,2.51588159,2.51255441,2.50924238,2.50594525,2.50266278,2.49939476,2.49614096,
2.49290115,2.48967513,2.48646269,2.48326362,2.48007773,2.47690482,2.47374472,2.47059722,
2.46746215,2.46433933,2.46122860,2.45812977,2.45504269,2.45196720,2.44890314,2.44585034,
2.44280867,2.43977797,2.43675809,2.43374890,2.43075025,2.42776201,2.42478404,2.42181622,
2.41885841,2.41591048,2.41297232,2.41004380,2.40712480,2.40421521,2.40131491,2.39842379,
2.39554173,2.39266863,2.38980439,2.38694889,2.38410204,2.38126374,2.37843388,2.37561237,
2.37279910,2.36999400,2.36719697,2.36440790,2.36162673,2.35885335,2.35608768,2.35332964,
2.35057914,2.34783610,2.34510044,2.34237208,2.33965094,2.33693695,2.33423003,2.33153010,
2.32883709,2.32615093,2.32347155,2.32079888,2.31813284,2.31547337,2.31282041,2.31017388,
2.30753373,2.30489988,2.30227228,2.29965086,2.29703556,2.29442632,2.29182309,2.28922580,
2.28663439,2.28404881,2.28146900,2.27889490,2.27632647,2.27376364,2.27120637,2.26865460,
2.26610827,2.26356735,2.26103177,2.25850149,2.25597646,2.25345663,2.25094195,2.24843238,
2.24592786,2.24342836,2.24093382,2.23844420,2.23595946,2.23347956,2.23100444,2.22853408,
2.22606842,2.22360742,2.22115104,2.21869925,2.21625199,2.21380924,2.21137096,2.20893709,
2.20650761,2.20408248,2.20166166,2.19924511,2.19683280,2.19442469,2.19202074,2.18962092,
2.18722520,2.18483354,2.18244590,2.18006225,2.17768257,2.17530680,2.17293493,2.17056692,
2.16820274,2.16584236,2.16348574,2.16113285,2.15878367,2.15643816,2.15409630,2.15175805,
2.14942338,2.14709226,2.14476468,2.14244059,2.14011997,2.13780279,2.13548903,2.13317865,
2.13087163,2.12856795,2.12626757,2.12397047,2.12167662,2.11938600,2.11709859,2.11481435,
2.11253326,2.11025530,2.10798044,2.10570867,2.10343994,2.10117424,2.09891156,2.09665185,
2.09439510,2.09214129,2.08989040,2.08764239,2.08539725,2.08315496,2.08091550,2.07867884,
2.07644495,2.07421383,2.07198545,2.06975978,2.06753681,2.06531651,2.06309887,2.06088387,
2.05867147,2.05646168,2.05425445,2.05204979,2.04984765,2.04764804,2.04545092,2.04325628,
2.04106409,2.03887435,2.03668703,2.03450211,2.03231957,2.03013941,2.02796159,2.02578610,
2.02361292,2.02144204,2.01927344,2.01710710,2.01494300,2.01278113,2.01062146,2.00846399,
2.00630870,2.00415556,2.00200457,1.99985570,1.99770895,1.99556429,1.99342171,1.99128119,
1.98914271,1.98700627,1.98487185,1.98273942,1.98060898,1.97848051,1.97635399,1.97422942,
1.97210676,1.96998602,1.96786718,1.96575021,1.96363511,1.96152187,1.95941046,1.95730088,
1.95519310,1.95308712,1.95098292,1.94888050,1.94677982,1.94468089,1.94258368,1.94048818,
1.93839439,1.93630228,1.93421185,1.93212308,1.93003595,1.92795046,1.92586659,1.92378433,
1.92170367,1.91962459,1.91754708,1.91547113,1.91339673,1.91132385,1.90925250,1.90718266,
1.90511432,1.90304746,1.90098208,1.89891815,1.89685568,1.89479464,1.89273503,1.89067683,
1.88862003,1.88656463,1.88451060,1.88245794,1.88040664,1.87835668,1.87630806,1.87426076,
1.87221477,1.87017008,1.86812668,1.86608457,1.86404371,1.86200412,1.85996577,1.85792866,
1.85589277,1.85385809,1.85182462,1.84979234,1.84776125,1.84573132,1.84370256,1.84167495,
1.83964848,1.83762314,1.83559892,1.83357582,1.83155381,1.82953289,1.82751305,1.82549429,
1.82347658,1.82145993,1.81944431,1.81742973,1.81541617,1.81340362,1.81139207,1.80938151,
1.80737194,1.80536334,1.80335570,1.80134902,1.79934328,1.79733848,1.79533460,1.79333164,
1.79132959,1.78932843,1.78732817,1.78532878,1.78333027,1.78133261,1.77933581,1.77733985,
1.77534473,1.77335043,1.77135695,1.76936428,1.76737240,1.76538132,1.76339101,1.76140148,
1.75941271,1.75742470,1.75543743,1.75345090,1.75146510,1.74948002,1.74749565,1.74551198,
1.74352900,1.74154672,1.73956511,1.73758417,1.73560389,1.73362426,1.73164527,1.72966692,
1.72768920,1.72571209,1.72373560,1.72175971,1.71978441,1.71780969,1.71583556,1.71386199,
1.71188899,1.70991653,1.70794462,1.70597325,1.70400241,1.70203209,1.70006228,1.69809297,
1.69612416,1.69415584,1.69218799,1.69022062,1.68825372,1.68628727,1.68432127,1.68235571,
1.68039058,1.67842588,1.67646160,1.67449772,1.67253424,1.67057116,1.66860847,1.66664615,
1.66468420,1.66272262,1.66076139,1.65880050,1.65683996,1.65487975,1.65291986,1.65096028,
1.64900102,1.64704205,1.64508338,1.64312500,1.64116689,1.63920905,1.63725148,1.63529416,
1.63333709,1.63138026,1.62942366,1.62746728,1.62551112,1.62355517,1.62159943,1.61964388,
1.61768851,1.61573332,1.61377831,1.61182346,1.60986877,1.60791422,1.60595982,1.60400556,
1.60205142,1.60009739,1.59814349,1.59618968,1.59423597,1.59228235,1.59032882,1.58837536,
1.58642196,1.58446863,1.58251535,1.58056211,1.57860891,1.57665574,1.57470259,1.57274945,
1.57079633,1.56884320,1.56689007,1.56493692,1.56298375,1.56103055,1.55907731,1.55712403,
1.55517069,1.55321730,1.55126383,1.54931030,1.54735668,1.54540297,1.54344917,1.54149526,
1.53954124,1.53758710,1.53563283,1.53367843,1.53172389,1.52976919,1.52781434,1.52585933,
1.52390414,1.52194878,1.51999323,1.51803748,1.51608153,1.51412537,1.51216900,1.51021240,
1.50825556,1.50629849,1.50434117,1.50238360,1.50042576,1.49846765,1.49650927,1.49455060,
1.49259163,1.49063237,1.48867280,1.48671291,1.48475270,1.48279215,1.48083127,1.47887004,
1.47690845,1.47494650,1.47298419,1.47102149,1.46905841,1.46709493,1.46513106,1.46316677,
1.46120207,1.45923694,1.45727138,1.45530538,1.45333893,1.45137203,1.44940466,1.44743682,
1.44546850,1.44349969,1.44153038,1.43956057,1.43759024,1.43561940,1.43364803,1.43167612,
1.42970367,1.42773066,1.42575709,1.42378296,1.42180825,1.41983295,1.41785705,1.41588056,
1.41390346,1.41192573,1.40994738,1.40796840,1.40598877,1.40400849,1.40202755,1.40004594,
1.39806365,1.39608068,1.39409701,1.39211264,1.39012756,1.38814175,1.38615522,1.38416795,
1.38217994,1.38019117,1.37820164,1.37621134,1.37422025,1.37222837,1.37023570,1.36824222,
1.36624792,1.36425280,1.36225684,1.36026004,1.35826239,1.35626387,1.35426449,1.35226422,
1.35026307,1.34826101,1.34625805,1.34425418,1.34224937,1.34024364,1.33823695,1.33622932,
1.33422072,1.33221114,1.33020059,1.32818904,1.32617649,1.32416292,1.32214834,1.32013273,
1.31811607,1.31609837,1.31407960,1.31205976,1.31003885,1.30801684,1.30599373,1.30396951,
1.30194417,1.29991770,1.29789009,1.29586133,1.29383141,1.29180031,1.28976803,1.28773456,
1.28569989,1.28366400,1.28162688,1.27958854,1.27754894,1.27550809,1.27346597,1.27142257,
1.26937788,1.26733189,1.26528459,1.26323597,1.26118602,1.25913471,1.25708205,1.25502803,
1.25297262,1.25091583,1.24885763,1.24679802,1.24473698,1.24267450,1.24061058,1.23854519,
1.23647833,1.23440999,1.23234015,1.23026880,1.22819593,1.22612152,1.22404557,1.22196806,
1.21988898,1.21780832,1.21572606,1.21364219,1.21155670,1.20946958,1.20738080,1.20529037,
1.20319826,1.20110447,1.19900898,1.19691177,1.19481283,1.19271216,1.19060973,1.18850553,
1.18639955,1.18429178,1.18218219,1.18007079,1.17795754,1.17584244,1.17372548,1.17160663,
1.16948589,1.16736324,1.16523866,1.16311215,1.16098368,1.15885323,1.15672081,1.15458638,
1.15244994,1.15031147,1.14817095,1.14602836,1.14388370,1.14173695,1.13958808,1.13743709,
1.13528396,1.13312866,1.13097119,1.12881153,1.12664966,1.12448556,1.12231921,1.12015061,
1.11797973,1.11580656,1.11363107,1.11145325,1.10927308,1.10709055,1.10490563,1.10271831,
1.10052856,1.09833638,1.09614174,1.09394462,1.09174500,1.08954287,1.08733820,1.08513098,
1.08292118,1.08070879,1.07849378,1.07627614,1.07405585,1.07183287,1.06960721,1.06737882,
1.06514770,1.06291382,1.06067715,1.05843769,1.05619540,1.05395026,1.05170226,1.04945136,
1.04719755,1.04494080,1.04268110,1.04041841,1.03815271,1.03588399,1.03361221,1.03133735,
1.02905939,1.02677830,1.02449407,1.02220665,1.01991603,1.01762219,1.01532509,1.01302471,
1.01072102,1.00841400,1.00610363,1.00378986,1.00147268,0.99915206,0.99682798,0.99450039,
0.99216928,0.98983461,0.98749636,0.98515449,0.98280898,0.98045980,0.97810691,0.97575030,
0.97338991,0.97102573,0.96865772,0.96628585,0.96391009,0.96153040,0.95914675,0.95675912,
0.95436745,0.95197173,0.94957191,0.94716796,0.94475985,0.94234754,0.93993099,0.93751017,
0.93508504,0.93265556,0.93022170,0.92778341,0.92534066,0.92289341,0.92044161,0.91798524,
0.91552424,0.91305858,0.91058821,0.90811309,0.90563319,0.90314845,0.90065884,0.89816430,
0.89566479,0.89316028,0.89065070,0.88813602,0.88561619,0.88309116,0.88056088,0.87802531,
0.87548438,0.87293806,0.87038629,0.86782901,0.86526619,0.86269775,0.86012366,0.85754385,
0.85495827,0.85236686,0.84976956,0.84716633,0.84455709,0.84194179,0.83932037,0.83669277,
0.83405893,0.83141877,0.82877225,0.82611928,0.82345981,0.82079378,0.81812110,0.81544172,
0.81275556,0.81006255,0.80736262,0.80465570,0.80194171,0.79922057,0.79649221,0.79375655,
0.79101352,0.78826302,0.78550497,0.78273931,0.77996593,0.77718475,0.77439569,0.77159865,
0.76879355,0.76598029,0.76315878,0.76032891,0.75749061,0.75464376,0.75178826,0.74892402,
0.74605092,0.74316887,0.74027775,0.73737744,0.73446785,0.73154885,0.72862033,0.72568217,
0.72273425,0.71977644,0.71680861,0.71383064,0.71084240,0.70784376,0.70483456,0.70181469,
0.69878398,0.69574231,0.69268952,0.68962545,0.68654996,0.68346288,0.68036406,0.67725332,
0.67413051,0.67099544,0.66784794,0.66468783,0.66151492,0.65832903,0.65512997,0.65191753,
0.64869151,0.64545170,0.64219789,0.63892987,0.63564741,0.63235028,0.62903824,0.62571106,
0.62236849,0.61901027,0.61563615,0.61224585,0.60883911,0.60541564,0.60197515,0.59851735,
0.59504192,0.59154856,0.58803694,0.58450672,0.58095756,0.57738911,0.57380101,0.57019288,
0.56656433,0.56291496,0.55924437,0.55555212,0.55183778,0.54810089,0.54434099,0.54055758,
0.53675018,0.53291825,0.52906127,0.52517867,0.52126988,0.51733431,0.51337132,0.50938028,
0.50536051,0.50131132,0.49723200,0.49312177,0.48897987,0.48480547,0.48059772,0.47635573,
0.47207859,0.46776530,0.46341487,0.45902623,0.45459827,0.45012983,0.44561967,0.44106652,
0.43646903,0.43182577,0.42713525,0.42239588,0.41760600,0.41276385,0.40786755,0.40291513,
0.39790449,0.39283339,0.38769946,0.38250016,0.37723277,0.37189441,0.36648196,0.36099209,
0.35542120,0.34976542,0.34402054,0.33818204,0.33224495,0.32620390,0.32005298,0.31378574,
0.30739505,0.30087304,0.29421096,0.28739907,0.28042645,0.27328078,0.26594810,0.25841250,
0.25065566,0.24265636,0.23438976,0.22582651,0.21693146,0.20766198,0.19796546,0.18777575,
0.17700769,0.16554844,0.15324301,0.13986823,0.12508152,0.10830610,0.08841715,0.06251018,
};

double hack_acos( double x ) {
	int index;

	if (x < -1)
		x = -1;
	if (x > 1)
		x = 1;
	index = (float) (1.0 + x) * 511.9;
	return hack_acostable[index];
}

// Arc Sine should equal Arc Cosine + 1/2 Pi.
double hack_asin(double x)
{
	return (hack_acos(x) - 0.5 * M_PI);
}

void QuatToAngles (const vec4_t q, vec3_t a) {
	vec4_t q2;
	q2[0] = q[0]*q[0];
	q2[1] = q[1]*q[1];
	q2[2] = q[2]*q[2];
	q2[3] = q[3]*q[3];
	a[ROLL] = (180.0/M_PI)*atan2 (2*(q[2]*q[3] + q[1]*q[0]) , (-q2[1] - q2[2] + q2[3] + q2[0]));
	a[PITCH] = -1 * (180.0/M_PI)* hack_asin( -2*(q[1]*q[3] - q[2]*q[0]) );
	a[YAW] = (180.0/M_PI)*atan2 (2*(q[1]*q[2] + q[3]*q[0]) , (q2[1] - q2[2] - q2[3] + q2[0]));
}

float DistancePointLine (const vec3_t point, const vec3_t line) {
	vec3_t cross, neg_point;
	float dist;

	VectorScale(point, -1, neg_point);
	CrossProduct(line, neg_point, cross);

	dist = VectorLength(cross) / VectorLength(line);
	return dist;	
}

float Det_2D( const vec2_t in1, const vec2_t in2 ) {
	return in1[0]*in2[1] - in1[1]*in2[0];
}

float DotProduct_2D( const vec2_t in1, const vec2_t in2 ) {
	return in1[0]*in2[0] + in1[1]*in2[1];
}

qboolean PointsSameSide_2D( const vec2_t lineBase, const vec2_t lineDelta, const vec2_t point1, const vec2_t point2 ) {
	float det1, det2;
	vec2_t point1Delta, point2Delta;

	point1Delta[0] = point1[0] - lineBase[0];
	point1Delta[1] = point1[1] - lineBase[1];
	point2Delta[0] = point2[0] - lineBase[0];
	point2Delta[1] = point2[1] - lineBase[1];

	det1 = Det_2D( lineDelta, point1Delta );
	det2 = Det_2D( lineDelta, point2Delta );

	if ( (det1 < 0) && (det2 < 0) ) {
		return qtrue;
	}
	
	if ( (det1 > 0) && (det2 > 0) ) {
		return qtrue;
	}

	if ( (det1 == 0) || (det2 == 0) ) {
		return qtrue;
	}

	return qfalse;
}

float Q_angle2D( const vec2_t in1, const vec2_t in2 ) {
	vec3_t	temp1, temp2;
	vec2_t	out1, out2;

	// Do it this way to make use of the vec3_t normalize function
	temp1[0] = in1[0];
	temp1[1] = in1[1];
	temp2[0] = in2[0];
	temp2[1] = in2[1];
	temp1[2] = temp2[2] = 0;
	VectorNormalize( temp1 );
	VectorNormalize( temp2 );

	out1[0] = temp1[0];
	out1[1] = temp1[1];
	out2[0] = temp2[0];
	out2[1] = temp2[1];

	return (hack_acos( DotProduct_2D( out1, out2 ) ));
}

float Distance_2D( const vec2_t in1, const vec2_t in2 ) {
	float temp1, temp2;

	temp1 = in1[0] - in2[0];
	temp1 *= temp1;

	temp2 = in1[1] - in2[1];
	temp2 *= temp2;

	return Q_fabs( sqrt( temp1 + temp2 ) );
}

float Q_hypot (float a, float b) {
  return sqrt(a*a + b*b);
}

void VectorPllComponent( const vec3_t A, const vec3_t B, vec3_t Pll ) {
	Pll[0] = DotProduct(A, B) * B[0] / ( B[0] * B[0] + B[1] * B[1] + B[2] * B[2] );
	Pll[1] = DotProduct(A, B) * B[1] / ( B[0] * B[0] + B[1] * B[1] + B[2] * B[2] );
	Pll[2] = DotProduct(A, B) * B[2] / ( B[0] * B[0] + B[1] * B[1] + B[2] * B[2] );
}

void ProjectPointOnLine( const vec3_t src, const vec3_t lpt1, const vec3_t lpt2, vec3_t prjct ) {
	vec3_t lpt1_src, lpt1_lpt2;

	VectorSubtract( lpt2, lpt1, lpt1_lpt2 );
	VectorSubtract( src,  lpt1, lpt1_src  );

	VectorPllComponent( lpt1_src, lpt1_lpt2, prjct );
	VectorAdd( prjct, lpt1, prjct );
}

float DistancePointToLine( const vec3_t point, const vec3_t lineBase, const vec3_t lineDelta ) {
	vec3_t pointDelta, Pll, Perp;

	VectorSubtract( point, lineBase, pointDelta );
	VectorPllComponent( pointDelta, lineDelta, Pll );
	VectorSubtract( pointDelta, Pll, Perp );
	return VectorLength( Perp );
}

void VectorPieceWiseMultiply( const vec3_t in1, const vec3_t in2, vec3_t out ) {
	out[0] = in1[0] * in2[0];
	out[1] = in1[1] * in2[1];
	out[2] = in1[2] * in2[2];
}

int Q_Sign( const float a ) {
	if ( a > 0 )
		return 1;
	
	if ( a < 0 )
		return -1;

	return 0;
}

//------------------------------------------------------------------------

// END ADDING

#ifndef Q3_VM
/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

=====================
*/
float Q_acos(float c) {
	float angle;

	angle = acos(c);

	if (angle > M_PI) {
		return (float)M_PI;
	}
	if (angle < -M_PI) {
		return (float)M_PI;
	}
	return angle;
}
#endif
