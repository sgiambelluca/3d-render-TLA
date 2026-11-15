#include "Value.h"

/* PRIVATE FUNCTIONS */

static bool _addComponents(const Context * context, const Component left, const Component right, const double sign, Component * result);
static bool _divideComponents(const Context * context, const Component left, const Component right, Component * result);
static bool _multiplyComponents(const Context * context, const Component left, const Component right, Component * result);

/**
 * Adds or subtracts two components of the same type. A numeric zero can be
 * combined with an angle, because it's zero degrees too (e.g., "45deg + 0").
 */
static bool _addComponents(const Context * context, const Component left, const Component right, const double sign, Component * result) {
	const bool isLeftZero = !left.isAngle && left.number == 0;
	const bool isRightZero = !right.isAngle && right.number == 0;
	if (left.isAngle != right.isAngle && !isLeftZero && !isRightZero) {
		reportSemanticError(context, "an angle and a number cannot be added or subtracted");
		return false;
	}
	result->number = left.number + sign * right.number;
	result->isAngle = left.isAngle || right.isAngle;
	result->isInteger = left.isInteger && right.isInteger;
	return true;
}

static bool _divideComponents(const Context * context, const Component left, const Component right, Component * result) {
	if (right.isAngle) {
		reportSemanticError(context, "the divisor must be a number, not an angle");
		return false;
	}
	if (right.number == 0) {
		reportSemanticError(context, "division by zero");
		return false;
	}
	result->number = left.number / right.number;
	result->isAngle = left.isAngle;
	result->isInteger = left.isInteger && right.isInteger && fmod(left.number, right.number) == 0;
	return true;
}

static bool _multiplyComponents(const Context * context, const Component left, const Component right, Component * result) {
	if (left.isAngle && right.isAngle) {
		reportSemanticError(context, "two angles cannot be multiplied");
		return false;
	}
	result->number = left.number * right.number;
	result->isAngle = left.isAngle || right.isAngle;
	result->isInteger = left.isInteger && right.isInteger;
	return true;
}

/* PUBLIC FUNCTIONS */

Value colorValue(const Color color) {
	Value value = {0};
	value.type = COLOR_VALUE;
	value.color = color;
	return value;
}

Value entityValue(Symbol * symbol) {
	Value value = {0};
	value.type = ENTITY_VALUE;
	value.entity = symbol;
	return value;
}

Value numberValue(const double number, const bool isInteger) {
	Value value = {0};
	value.type = SCALAR_VALUE;
	value.dimension = 1;
	value.components[0].number = number;
	value.components[0].isInteger = isInteger;
	return value;
}

Value zeroVectorValue(void) {
	Value value = numberValue(0, true);
	value.type = VECTOR_VALUE;
	value.dimension = 3;
	value.components[1] = value.components[0];
	value.components[2] = value.components[0];
	return value;
}

bool addValues(const Context * context, const Value * left, const Value * right, const double sign, Value * result) {
	if (left->type != right->type || (left->type != SCALAR_VALUE && left->type != VECTOR_VALUE)) {
		reportSemanticError(context, "addition and subtraction need two scalars or two vectors");
		return false;
	}
	if (left->dimension != right->dimension) {
		reportSemanticError(context, "cannot add or subtract vectors of dimensions %u and %u", left->dimension, right->dimension);
		return false;
	}
	*result = *left;
	for (unsigned int k = 0; k < left->dimension; ++k) {
		if (!_addComponents(context, left->components[k], right->components[k], sign, &result->components[k])) {
			return false;
		}
	}
	return true;
}

bool divideValues(const Context * context, const Value * left, const Value * right, Value * result) {
	if ((left->type != SCALAR_VALUE && left->type != VECTOR_VALUE) || right->type != SCALAR_VALUE) {
		reportSemanticError(context, "a division needs an scalar or a vector, over a number");
		return false;
	}
	*result = *left;
	for (unsigned int k = 0; k < left->dimension; ++k) {
		if (!_divideComponents(context, left->components[k], right->components[0], &result->components[k])) {
			return false;
		}
	}
	return true;
}

bool multiplyValues(const Context * context, const Value * left, const Value * right, Value * result) {
	if (left->type == SCALAR_VALUE && right->type == SCALAR_VALUE) {
		*result = *left;
		return _multiplyComponents(context, left->components[0], right->components[0], &result->components[0]);
	}
	const bool isVectorByScalar = left->type == VECTOR_VALUE && right->type == SCALAR_VALUE;
	const bool isScalarByVector = left->type == SCALAR_VALUE && right->type == VECTOR_VALUE;
	if (!isVectorByScalar && !isScalarByVector) {
		reportSemanticError(context, "a product needs two scalars, or a vector and a number");
		return false;
	}
	const Value * vector = isVectorByScalar ? left : right;
	const Value * scalar = isVectorByScalar ? right : left;
	if (scalar->components[0].isAngle) {
		reportSemanticError(context, "a vector can only be multiplied by a number, not by an angle");
		return false;
	}
	*result = *vector;
	for (unsigned int k = 0; k < vector->dimension; ++k) {
		if (!_multiplyComponents(context, vector->components[k], scalar->components[0], &result->components[k])) {
			return false;
		}
	}
	return true;
}

bool negateValue(const Context * context, const Value * operand, Value * result) {
	if (operand->type != SCALAR_VALUE && operand->type != VECTOR_VALUE) {
		reportSemanticError(context, "the unary minus needs an scalar or a vector");
		return false;
	}
	*result = *operand;
	for (unsigned int k = 0; k < operand->dimension; ++k) {
		result->components[k].number = -operand->components[k].number;
	}
	return true;
}

bool isAnglesVector(const Value * value) {
	if (value->type != VECTOR_VALUE || value->dimension != 3) {
		return false;
	}
	for (unsigned int k = 0; k < 3; ++k) {
		const Component component = value->components[k];
		if (!component.isAngle && component.number != 0) {
			return false;
		}
	}
	return true;
}

bool isFiniteValue(const Value * value) {
	for (unsigned int k = 0; k < value->dimension; ++k) {
		if (!isfinite(value->components[k].number)) {
			return false;
		}
	}
	return true;
}

bool isNumber(const Value * value) {
	return value->type == SCALAR_VALUE && !value->components[0].isAngle;
}

bool isNumbersVector(const Value * value, const unsigned int dimension) {
	if (value->type != VECTOR_VALUE || value->dimension != dimension) {
		return false;
	}
	for (unsigned int k = 0; k < dimension; ++k) {
		if (value->components[k].isAngle) {
			return false;
		}
	}
	return true;
}

bool isPositiveNumbersVector(const Value * value, const unsigned int dimension) {
	if (!isNumbersVector(value, dimension)) {
		return false;
	}
	for (unsigned int k = 0; k < dimension; ++k) {
		if (value->components[k].number <= 0) {
			return false;
		}
	}
	return true;
}
