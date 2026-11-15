#ifndef MATRIX_HEADER
#define MATRIX_HEADER

#include <math.h>
#include <string.h>

/**
 * 4x4 matrices in row-major order, used to compute the model matrix of each
 * mesh at compile time.
 */

void identityMatrix(double matrix[16]);

/**
 * Computes matrix = left * matrix.
 */
void leftMultiplyMatrix(const double left[16], double matrix[16]);

void translationMatrix(const double x, const double y, const double z, double matrix[16]);

/**
 * Computes Rz * Ry * Rx, with the angles in degrees.
 */
void rotationMatrix(const double x, const double y, const double z, double matrix[16]);

void scaleMatrix(const double x, const double y, const double z, double matrix[16]);

#endif
