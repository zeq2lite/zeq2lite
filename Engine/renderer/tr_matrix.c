/*
 * tr_matrix.c
 * Matrix management
 * Copyright (C) 2010  Jens Loehr
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

/*
 * R_MatrixFlip
 * Load flip matrix (q3 -> opengl)
 */
void R_MatrixFlip(matrix_t matrix) {
	matrix[ 0] = 0;		matrix[ 4] = -1;	matrix[ 8] = 0;		matrix[12] = 0;
	matrix[ 1] = 0;		matrix[ 5] = 0;		matrix[ 9] = 1;		matrix[13] = 0;
	matrix[ 2] = -1;	matrix[ 6] = 0;		matrix[10] = 0;		matrix[14] = 0;
	matrix[ 3] = 0;		matrix[ 7] = 0;		matrix[11] = 0;		matrix[15] = 1;
}

/*
 * R_MatrixIdentity
 * Load identity matrix
 */
void R_MatrixIdentity(matrix_t matrix) {
	matrix[ 0] = 1;	matrix[ 4] = 0;	matrix[ 8] = 0;	matrix[12] = 0;
	matrix[ 1] = 0;	matrix[ 5] = 1;	matrix[ 9] = 0;	matrix[13] = 0;
	matrix[ 2] = 0;	matrix[ 6] = 0;	matrix[10] = 1;	matrix[14] = 0;
	matrix[ 3] = 0;	matrix[ 7] = 0;	matrix[11] = 0;	matrix[15] = 1;
}

/*
 * R_MatrixClear
 * Null matrix
 */
void R_MatrixClear(matrix_t matrix) {
	matrix[ 0] = 0;	matrix[ 4] = 0;	matrix[ 8] = 0;	matrix[12] = 0;
	matrix[ 1] = 0;	matrix[ 5] = 0;	matrix[ 9] = 0;	matrix[13] = 0;
	matrix[ 2] = 0;	matrix[ 6] = 0;	matrix[10] = 0;	matrix[14] = 0;
	matrix[ 3] = 0;	matrix[ 7] = 0;	matrix[11] = 0;	matrix[15] = 0;
}

/*
 * R_MatrixCopy
 * Copy matrix
 */
void R_MatrixCopy(const matrix_t in, matrix_t out) {
	out[ 0] = in[ 0];	out[ 4] = in[ 4];	out[ 8] = in[ 8];	out[12] = in[12];
	out[ 1] = in[ 1];	out[ 5] = in[ 5];	out[ 9] = in[ 9];	out[13] = in[13];
	out[ 2] = in[ 2];	out[ 6] = in[ 6];	out[10] = in[10];	out[14] = in[14];
	out[ 3] = in[ 3];	out[ 7] = in[ 7];	out[11] = in[11];	out[15] = in[15];
}

/*
 * R_MatrixCompare
 * Compare two matrices
 */
qboolean R_MatrixCompare(const matrix_t a, const matrix_t b) {
	return (a[ 0] == b[ 0] && a[ 4] == b[ 4] && a[ 8] == b[ 8] && a[12] == b[12] &&
			a[ 1] == b[ 1] && a[ 5] == b[ 5] && a[ 9] == b[ 9] && a[13] == b[13] &&
			a[ 2] == b[ 2] && a[ 6] == b[ 6] && a[10] == b[10] && a[14] == b[14] &&
			a[ 3] == b[ 3] && a[ 7] == b[ 7] && a[11] == b[11] && a[15] == b[15]);
}

/*
 * R_MatrixTranspose
 * Transpose matrix
 */
void R_MatrixTranspose(const matrix_t in, matrix_t out) {
	out[ 0] = in[ 0];	out[ 1] = in[ 4];	out[ 2] = in[ 8];	out[ 3] = in[12];
	out[ 4] = in[ 1];	out[ 5] = in[ 5];	out[ 6] = in[ 9];	out[ 7] = in[13];
	out[ 8] = in[ 2];	out[ 9] = in[ 6];	out[10] = in[10];	out[11] = in[14];
	out[12] = in[ 3];	out[13] = in[ 7];	out[14] = in[11];	out[15] = in[15];
}

/*
 * R_MatrixMultiply
 * Multiply matrices
 */
void R_MatrixMultiply(const matrix_t a, const matrix_t b, matrix_t out) {
	out[ 0] = b[ 0] * a[ 0] + b[ 1] * a[ 4] + b[ 2] * a[ 8] + b[ 3] * a[12];
	out[ 1] = b[ 0] * a[ 1] + b[ 1] * a[ 5] + b[ 2] * a[ 9] + b[ 3] * a[13];
	out[ 2] = b[ 0] * a[ 2] + b[ 1] * a[ 6] + b[ 2] * a[10] + b[ 3] * a[14];
	out[ 3] = b[ 0] * a[ 3] + b[ 1] * a[ 7] + b[ 2] * a[11] + b[ 3] * a[15];

	out[ 4] = b[ 4] * a[ 0] + b[ 5] * a[ 4] + b[ 6] * a[ 8] + b[ 7] * a[12];
	out[ 5] = b[ 4] * a[ 1] + b[ 5] * a[ 5] + b[ 6] * a[ 9] + b[ 7] * a[13];
	out[ 6] = b[ 4] * a[ 2] + b[ 5] * a[ 6] + b[ 6] * a[10] + b[ 7] * a[14];
	out[ 7] = b[ 4] * a[ 3] + b[ 5] * a[ 7] + b[ 6] * a[11] + b[ 7] * a[15];

	out[ 8] = b[ 8] * a[ 0] + b[ 9] * a[ 4] + b[10] * a[ 8] + b[11] * a[12];
	out[ 9] = b[ 8] * a[ 1] + b[ 9] * a[ 5] + b[10] * a[ 9] + b[11] * a[13];
	out[10] = b[ 8] * a[ 2] + b[ 9] * a[ 6] + b[10] * a[10] + b[11] * a[14];
	out[11] = b[ 8] * a[ 3] + b[ 9] * a[ 7] + b[10] * a[11] + b[11] * a[15];

	out[12] = b[12] * a[ 0] + b[13] * a[ 4] + b[14] * a[ 8] + b[15] * a[12];
	out[13] = b[12] * a[ 1] + b[13] * a[ 5] + b[14] * a[ 9] + b[15] * a[13];
	out[14] = b[12] * a[ 2] + b[13] * a[ 6] + b[14] * a[10] + b[15] * a[14];
	out[15] = b[12] * a[ 3] + b[13] * a[ 7] + b[14] * a[11] + b[15] * a[15];
}

/*
 * R_MatrixTranslate
 * Translate matrix
 */
void R_MatrixTranslate(matrix_t matrix, float x, float y, float z) {
	matrix_t a, b;

	R_MatrixCopy(matrix, a);

	/* translation matrix */
	b[ 0] = 1;	b[ 4] = 0;	b[ 8] = 0;	b[12] = x;
	b[ 1] = 0;	b[ 5] = 1;	b[ 9] = 0;	b[13] = y;
	b[ 2] = 0;	b[ 6] = 0;	b[10] = 1;	b[14] = z;
	b[ 3] = 0;	b[ 7] = 0;	b[11] = 0;	b[15] = 1;

	R_MatrixMultiply(a, b, matrix);
}

/*
 * R_MatrixOrthogonalProjection
 * Multiply matrix with an orthographic matrix
 */
void R_MatrixOrthogonalProjection(matrix_t matrix, float left, float right, float bottom, float top, float nearVal, float farVal) {
	matrix_t a, b;

	R_MatrixCopy(matrix, a);

	/* orthogonal projection matrix */
	b[ 0] = 2.0f / (right - left);	b[ 4] = 0;						b[ 8] = 0;							b[12] = -(right + left) / (right - left);
	b[ 1] = 0;						b[ 5] = 2.0f / (top - bottom);	b[ 9] = 0;							b[13] = -(top + bottom) / (top - bottom);
	b[ 2] = 0;						b[ 6] = 0;						b[10] = -2.0f / (farVal - nearVal);	b[14] = -(farVal + nearVal) / (farVal - nearVal);
	b[ 3] = 0;						b[ 7] = 0;						b[11] = 0;							b[15] = 1;

	R_MatrixMultiply(a, b, matrix);
}

/*
 * R_LoadModelViewMatrix
 * Load modelview matrix
 */
void R_LoadModelViewMatrix(const matrix_t matrix) {
	if (R_MatrixCompare(matrix, glState.modelViewMatrix))
		return;

	R_MatrixCopy(matrix, glState.modelViewMatrix);
	R_MatrixMultiply(glState.projectionMatrix, glState.modelViewMatrix, glState.modelViewProjectionMatrix);
}

/*
 * R_LoadProjectionMatrix
 * Load projection matrix
 */
void R_LoadProjectionMatrix(const matrix_t matrix) {
	if (R_MatrixCompare(matrix, glState.projectionMatrix))
		return;

	R_MatrixCopy(matrix, glState.projectionMatrix);
	R_MatrixMultiply(glState.projectionMatrix, glState.modelViewMatrix, glState.modelViewProjectionMatrix);
}
