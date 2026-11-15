#ifndef VALUE_HEADER
#define VALUE_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include <math.h>
#include <stdbool.h>

/**
 * The values that an expression can produce, and the operations between them.
 * Every operation checks the types of its operands, and reports an error in
 * the given context if they are not compatible.
 */

/** Constructors. */

Value colorValue(const Color color);
Value entityValue(Symbol * symbol);
Value numberValue(const double number, const bool isInteger);
Value zeroVectorValue(void);

/** Arithmetic. */

/**
 * Adds (sign = 1) or subtracts (sign = -1) two scalars, or two vectors of the
 * same dimension.
 */
bool addValues(const Context * context, const Value * left, const Value * right, const double sign, Value * result);

/**
 * Divides an scalar or a vector by a non-zero number.
 */
bool divideValues(const Context * context, const Value * left, const Value * right, Value * result);

/**
 * Multiplies two scalars, or a vector and a number.
 */
bool multiplyValues(const Context * context, const Value * left, const Value * right, Value * result);

/**
 * Negates an scalar or a vector.
 */
bool negateValue(const Context * context, const Value * operand, Value * result);

/** Predicates. */

/**
 * A vector of 3 angles. A numeric zero is accepted as an angle too.
 */
bool isAnglesVector(const Value * value);

/**
 * Every component of the value is a finite number.
 */
bool isFiniteValue(const Value * value);

/**
 * An scalar that is not an angle.
 */
bool isNumber(const Value * value);

/**
 * A vector of the given dimension, without angles.
 */
bool isNumbersVector(const Value * value, const unsigned int dimension);

/**
 * A vector of the given dimension, with positive numbers only.
 */
bool isPositiveNumbersVector(const Value * value, const unsigned int dimension);

#endif
