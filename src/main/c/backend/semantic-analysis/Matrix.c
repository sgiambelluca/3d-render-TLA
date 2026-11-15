#include "Matrix.h"

#define DEGREES_TO_RADIANS (3.14159265358979323846 / 180.0)

/* PUBLIC FUNCTIONS */

void identityMatrix(double matrix[16]) {
	memset(matrix, 0, 16 * sizeof(double));
	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
}

/**
 * Computes matrix = left * matrix.
 */
void leftMultiplyMatrix(const double left[16], double matrix[16]) {
	double result[16];
	for (int row = 0; row < 4; ++row) {
		for (int column = 0; column < 4; ++column) {
			result[4 * row + column] = 0;
			for (int k = 0; k < 4; ++k) {
				result[4 * row + column] += left[4 * row + k] * matrix[4 * k + column];
			}
		}
	}
	memcpy(matrix, result, sizeof(result));
}

void translationMatrix(const double x, const double y, const double z, double matrix[16]) {
	identityMatrix(matrix);
	matrix[3] = x;
	matrix[7] = y;
	matrix[11] = z;
}

/**
 * Computes Rz * Ry * Rx, with the angles in degrees.
 */
void rotationMatrix(const double x, const double y, const double z, double matrix[16]) {
	const double cx = cos(x * DEGREES_TO_RADIANS);
	const double sx = sin(x * DEGREES_TO_RADIANS);
	const double cy = cos(y * DEGREES_TO_RADIANS);
	const double sy = sin(y * DEGREES_TO_RADIANS);
	const double cz = cos(z * DEGREES_TO_RADIANS);
	const double sz = sin(z * DEGREES_TO_RADIANS);
	const double rotationX[16] = { 1, 0, 0, 0, 0, cx, -sx, 0, 0, sx, cx, 0, 0, 0, 0, 1 };
	const double rotationY[16] = { cy, 0, sy, 0, 0, 1, 0, 0, -sy, 0, cy, 0, 0, 0, 0, 1 };
	const double rotationZ[16] = { cz, -sz, 0, 0, sz, cz, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	memcpy(matrix, rotationX, sizeof(rotationX));
	leftMultiplyMatrix(rotationY, matrix);
	leftMultiplyMatrix(rotationZ, matrix);
}

void scaleMatrix(const double x, const double y, const double z, double matrix[16]) {
	identityMatrix(matrix);
	matrix[0] = x;
	matrix[5] = y;
	matrix[10] = z;
}
