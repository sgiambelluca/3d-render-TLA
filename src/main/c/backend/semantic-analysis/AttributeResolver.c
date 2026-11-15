#include "AttributeResolver.h"

/* MODULE INTERNAL STATE */

static const SymbolTable * _symbolTable = NULL;

/* PRIVATE FUNCTIONS */

static bool _evaluateConstant(const Constant * constant, Value * result);
static bool _evaluateFactor(const Context * context, const Factor * factor, Value * result);
static bool _evaluateReference(const Context * context, const Reference * reference, Value * result);
static bool _evaluateVector(const Context * context, const Vector * vector, Value * result);
static bool _validateAttribute(const Context * context, const Symbol * symbol, const AttributeType attribute, Value * value);

/** Expressions. */

static bool _evaluateConstant(const Constant * constant, Value * result) {
	switch (constant->type) {
		case ANGLE_CONSTANT:
			*result = numberValue(constant->degrees, false);
			result->components[0].isAngle = true;
			return true;
		case COLOR_CONSTANT:
			*result = colorValue(constant->color);
			return true;
		case DECIMAL_CONSTANT:
			*result = numberValue(constant->decimal, false);
			return true;
		case INTEGER_CONSTANT:
			*result = numberValue(constant->integer, true);
			return true;
		default:
			reportError("The specified constant type is unknown: %d", constant->type);
			return false;
	}
}

static bool _evaluateFactor(const Context * context, const Factor * factor, Value * result) {
	switch (factor->type) {
		case CONSTANT:
			return _evaluateConstant(factor->constant, result);
		case EXPRESSION:
			return evaluateExpression(context, factor->expression, result);
		case REFERENCE:
			return _evaluateReference(context, factor->reference, result);
		case VECTOR:
			return _evaluateVector(context, factor->vector, result);
		default:
			reportError("The specified factor type is unknown: %d", factor->type);
			return false;
	}
}

/**
 * Evaluates a reference from left to right: an entity accepts the name of an
 * attribute, and a vector accepts ".x", ".y" and ".z".
 */
static bool _evaluateReference(const Context * context, const Reference * reference, Value * result) {
	Symbol * symbol = findSymbol(_symbolTable, reference->identifier);
	if (symbol == NULL) {
		reportSemanticError(context, "the identifier \"%s\" is not declared", reference->identifier);
		return false;
	}
	*result = entityValue(symbol);
	for (const Member * member = reference->firstMember; member != NULL; member = member->next) {
		if (result->type == ENTITY_VALUE) {
			AttributeType attribute;
			if (!attributeFromName(member->name, &attribute) || !isAttributeApplicable(result->entity, attribute)) {
				reportSemanticError(context, "\"%s\" doesn't have the attribute \"%s\"", result->entity->name, member->name);
				return false;
			}
			Symbol * owner = result->entity;
			if (!resolveAttribute(context, owner, attribute, result)) {
				return false;
			}
		}
		else if (result->type == VECTOR_VALUE) {
			unsigned int index = 3;
			if (strcmp(member->name, "x") == 0) {
				index = 0;
			}
			else if (strcmp(member->name, "y") == 0) {
				index = 1;
			}
			else if (strcmp(member->name, "z") == 0) {
				index = 2;
			}
			if (index == 3) {
				reportSemanticError(context, "a vector doesn't have the component \"%s\"", member->name);
				return false;
			}
			if (result->dimension <= index) {
				reportSemanticError(context, "a vector of dimension %u doesn't have the component \"%s\"", result->dimension, member->name);
				return false;
			}
			const Component component = result->components[index];
			*result = numberValue(component.number, component.isInteger);
			result->components[0].isAngle = component.isAngle;
		}
		else {
			reportSemanticError(context, "cannot access \"%s\" on a value that is not an entity or a vector", member->name);
			return false;
		}
	}
	return true;
}

static bool _evaluateVector(const Context * context, const Vector * vector, Value * result) {
	const Expression * expressions[3] = { vector->x, vector->y, vector->z };
	bool isValid = true;
	*result = numberValue(0, false);
	result->type = VECTOR_VALUE;
	result->dimension = vector->dimension;
	for (unsigned int k = 0; k < vector->dimension; ++k) {
		Value component;
		if (!evaluateExpression(context, expressions[k], &component)) {
			isValid = false;
		}
		else if (component.type != SCALAR_VALUE) {
			reportSemanticError(context, "the components of a vector must be numbers or angles");
			isValid = false;
		}
		else {
			result->components[k] = component.components[0];
		}
	}
	return isValid;
}

/** Attributes. */

/**
 * Checks the type and the range of an attribute. The size of a cube is
 * normalized to a vector, so ".x", ".y" and ".z" work on it too.
 */
static bool _validateAttribute(const Context * context, const Symbol * symbol, const AttributeType attribute, Value * value) {
	switch (attribute) {
		case COLOR_ATTRIBUTE:
			if (value->type != COLOR_VALUE) {
				reportSemanticError(context, "it must be a color");
				return false;
			}
			return true;
		case DIRECTION_ATTRIBUTE:
			if (!isNumbersVector(value, 3)) {
				reportSemanticError(context, "it must be a vector of 3 numbers");
				return false;
			}
			if (value->components[0].number == 0 && value->components[1].number == 0 && value->components[2].number == 0) {
				reportSemanticError(context, "the direction cannot be null");
				return false;
			}
			return true;
		case FOV_ATTRIBUTE:
			if (value->type != SCALAR_VALUE || !value->components[0].isAngle) {
				reportSemanticError(context, "it must be an angle (e.g., 60deg)");
				return false;
			}
			if (value->components[0].number <= 0 || 180 <= value->components[0].number) {
				reportSemanticError(context, "it must be greater than 0deg and less than 180deg");
				return false;
			}
			return true;
		case INTENSITY_ATTRIBUTE:
			if (!isNumber(value) || value->components[0].number < 0 || 1 < value->components[0].number) {
				reportSemanticError(context, "it must be a number between 0 and 1");
				return false;
			}
			return true;
		case LOOK_AT_ATTRIBUTE:
			if (value->type == ENTITY_VALUE) {
				if (value->entity->type == MATERIAL_ENTITY) {
					reportSemanticError(context, "the material \"%s\" doesn't have a position", value->entity->name);
					return false;
				}
				return true;
			}
			if (!isNumbersVector(value, 3)) {
				reportSemanticError(context, "it must be a vector of 3 numbers, or an entity with a position");
				return false;
			}
			return true;
		case MATERIAL_ATTRIBUTE:
			if (value->type != ENTITY_VALUE || value->entity->type != MATERIAL_ENTITY) {
				reportSemanticError(context, "it must be a material");
				return false;
			}
			return true;
		case POSITION_ATTRIBUTE:
			if (!isNumbersVector(value, 3)) {
				reportSemanticError(context, "it must be a vector of 3 numbers");
				return false;
			}
			return true;
		case RADIUS_ATTRIBUTE:
			if (!isNumber(value) || value->components[0].number <= 0) {
				reportSemanticError(context, "it must be a positive number");
				return false;
			}
			return true;
		case SHININESS_ATTRIBUTE:
			if (!isNumber(value) || !value->components[0].isInteger || value->components[0].number < 0) {
				reportSemanticError(context, "it must be an integer greater than or equal to 0");
				return false;
			}
			return true;
		case SIZE_ATTRIBUTE:
			if (symbol->meshType == PLANE_MESH) {
				if (!isPositiveNumbersVector(value, 2)) {
					reportSemanticError(context, "the size of a plane must be a vector of 2 positive numbers");
					return false;
				}
				return true;
			}
			if (isNumber(value) && 0 < value->components[0].number) {
				value->type = VECTOR_VALUE;
				value->dimension = 3;
				value->components[1] = value->components[0];
				value->components[2] = value->components[0];
				return true;
			}
			if (!isPositiveNumbersVector(value, 3)) {
				reportSemanticError(context, "the size of a box must be a positive number or a vector of 3 positive numbers");
				return false;
			}
			return true;
		default:
			return true;
	}
}

/* PUBLIC FUNCTIONS */

bool evaluateExpression(const Context * context, const Expression * expression, Value * result) {
	Value left;
	Value right;
	bool succeeded = false;
	switch (expression->type) {
		case ADDITION:
		case DIVISION:
		case MULTIPLICATION:
		case SUBTRACTION: {
			// Both operands are evaluated, to report the errors of each one.
			const bool isLeftValid = evaluateExpression(context, expression->leftExpression, &left);
			const bool isRightValid = evaluateExpression(context, expression->rightExpression, &right);
			if (!isLeftValid || !isRightValid) {
				return false;
			}
			if (expression->type == ADDITION) {
				succeeded = addValues(context, &left, &right, 1, result);
			}
			else if (expression->type == SUBTRACTION) {
				succeeded = addValues(context, &left, &right, -1, result);
			}
			else if (expression->type == MULTIPLICATION) {
				succeeded = multiplyValues(context, &left, &right, result);
			}
			else {
				succeeded = divideValues(context, &left, &right, result);
			}
			break;
		}
		case FACTOR:
			return _evaluateFactor(context, expression->factor, result);
		case NEGATION:
			if (!evaluateExpression(context, expression->operand, &left)) {
				return false;
			}
			succeeded = negateValue(context, &left, result);
			break;
		default:
			reportError("The specified expression type is unknown: %d", expression->type);
			return false;
	}
	if (succeeded && !isFiniteValue(result)) {
		reportSemanticError(context, "the result of the expression is too big");
		return false;
	}
	return succeeded;
}

/**
 * Resolves an attribute lazily, with memoization. Finding an attribute that is
 * being resolved means that it depends on itself.
 */
bool resolveAttribute(const Context * context, Symbol * symbol, const AttributeType attribute, Value * result) {
	Attribute * target = &symbol->attributes[attribute];
	switch (target->state) {
		case RESOLVED:
			*result = target->value;
			return true;
		case RESOLVING:
			reportSemanticError(context, "cyclic dependency on \"%s.%s\"", symbol->name, attributeName(attribute));
			return false;
		case UNRESOLVABLE:
			return false;
		default:
			break;
	}
	if (target->expression == NULL) {
		reportSemanticError(context, "\"%s\" doesn't declare the attribute \"%s\"", symbol->name, attributeName(attribute));
		return false;
	}
	const Context attributeContext = {
		.entity = symbol->name,
		.attribute = attributeName(attribute)
	};
	target->state = RESOLVING;
	Value value;
	if (!evaluateExpression(&attributeContext, target->expression, &value)
		|| !_validateAttribute(&attributeContext, symbol, attribute, &value)) {
		target->state = UNRESOLVABLE;
		return false;
	}
	target->value = value;
	target->state = RESOLVED;
	*result = value;
	return true;
}

void setResolverSymbolTable(const SymbolTable * symbolTable) {
	_symbolTable = symbolTable;
}
