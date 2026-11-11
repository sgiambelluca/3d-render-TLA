#include "SemanticAnalyzer.h"

#define DEGREES_TO_RADIANS (3.14159265358979323846 / 180.0)
#define ERROR_MESSAGE_SIZE 512

/**
 * The attribute being analyzed, that gives context to the reported errors.
 */
typedef struct {
	const char * entity;
	const char * attribute;
} Context;

/* MODULE INTERNAL STATE */

static unsigned int _errorCount = 0;
static Logger * _logger = NULL;
static SymbolTable * _symbolTable = NULL;

/** Shutdown module's internal state. */
void _shutdownSemanticAnalyzerModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: SemanticAnalyzer...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeSemanticAnalyzerModule() {
	_logger = createLogger("SemanticAnalyzer");
	return _shutdownSemanticAnalyzerModule;
}

/* PRIVATE FUNCTIONS */

static void _semanticError(const Context * context, const char * const format, ...);

static Value _colorValue(const Color color);
static Value _entityValue(Symbol * symbol);
static Value _numberValue(const double number, const bool isInteger);
static Value _zeroVectorValue(void);

static bool _addComponents(const Context * context, const Component left, const Component right, const double sign, Component * result);
static bool _divideComponents(const Context * context, const Component left, const Component right, Component * result);
static bool _multiplyComponents(const Context * context, const Component left, const Component right, Component * result);
static bool _evaluateAddition(const Context * context, const Value * left, const Value * right, const double sign, Value * result);
static bool _evaluateConstant(const Constant * constant, Value * result);
static bool _evaluateDivision(const Context * context, const Value * left, const Value * right, Value * result);
static bool _evaluateExpression(const Context * context, const Expression * expression, Value * result);
static bool _evaluateFactor(const Context * context, const Factor * factor, Value * result);
static bool _evaluateMultiplication(const Context * context, const Value * left, const Value * right, Value * result);
static bool _evaluateNegation(const Context * context, const Value * operand, Value * result);
static bool _evaluateReference(const Context * context, const Reference * reference, Value * result);
static bool _evaluateVector(const Context * context, const Vector * vector, Value * result);

static bool _isAnglesVector(const Value * value);
static bool _isFinite(const Value * value);
static bool _isNumber(const Value * value);
static bool _isNumbersVector(const Value * value, const unsigned int dimension);
static bool _isPositiveNumbersVector(const Value * value, const unsigned int dimension);
static bool _validateAttribute(const Context * context, const Symbol * symbol, const AttributeType attribute, Value * value);
static bool _resolveAttribute(const Context * context, Symbol * symbol, const AttributeType attribute, Value * result);

static void _declareEntities(const Program * program, unsigned int * cameraCount, const Render ** render, unsigned int * renderCount);
static void _declareAttribute(Symbol * symbol, const AttributeType attribute, Expression * expression);
static void _declareProperties(Symbol * symbol);
static void _checkDeclaredAttribute(Symbol * symbol, const AttributeType attribute);
static void _setDefaultAttribute(Symbol * symbol, const AttributeType attribute, const Value value);
static void _applyDefaultsAndRequirements(Symbol * symbol);
static void _analyzeCamera(Symbol * symbol);
static void _analyzeMesh(Symbol * symbol);
static void _analyzeSymbol(Symbol * symbol);
static void _analyzeRender(const Render * render, int * width, int * height);

static void _identityMatrix(double matrix[16]);
static void _leftMultiplyMatrix(const double left[16], double matrix[16]);
static void _translationMatrix(const double x, const double y, const double z, double matrix[16]);
static void _rotationMatrix(const double x, const double y, const double z, double matrix[16]);
static void _scaleMatrix(const double x, const double y, const double z, double matrix[16]);

static void _copyVector(const Value * value, double vector[3]);
static double _resolvedNumber(const Symbol * symbol, const AttributeType attribute);
static Scene * _createScene(const Program * program, const int width, const int height);

/**
 * Reports a semantic error of an entity's attribute. The analysis continues
 * after this, to report as many errors as possible.
 */
static void _semanticError(const Context * context, const char * const format, ...) {
	char message[ERROR_MESSAGE_SIZE];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, ERROR_MESSAGE_SIZE, format, arguments);
	va_end(arguments);
	logError(_logger, "Semantic error in \"%s.%s\": %s.", context->entity, context->attribute, message);
	++_errorCount;
}

/** Values. */

static Value _colorValue(const Color color) {
	Value value = {0};
	value.type = COLOR_VALUE;
	value.color = color;
	return value;
}

static Value _entityValue(Symbol * symbol) {
	Value value = {0};
	value.type = ENTITY_VALUE;
	value.entity = symbol;
	return value;
}

static Value _numberValue(const double number, const bool isInteger) {
	Value value = {0};
	value.type = SCALAR_VALUE;
	value.dimension = 1;
	value.components[0].number = number;
	value.components[0].isInteger = isInteger;
	return value;
}

static Value _zeroVectorValue(void) {
	Value value = _numberValue(0, true);
	value.type = VECTOR_VALUE;
	value.dimension = 3;
	value.components[1] = value.components[0];
	value.components[2] = value.components[0];
	return value;
}

/** Arithmetic. */

/**
 * Adds or subtracts two components of the same type. A numeric zero can be
 * combined with an angle, because it's zero degrees too (e.g., "45deg + 0").
 */
static bool _addComponents(const Context * context, const Component left, const Component right, const double sign, Component * result) {
	const bool isLeftZero = !left.isAngle && left.number == 0;
	const bool isRightZero = !right.isAngle && right.number == 0;
	if (left.isAngle != right.isAngle && !isLeftZero && !isRightZero) {
		_semanticError(context, "an angle and a number cannot be added or subtracted");
		return false;
	}
	result->number = left.number + sign * right.number;
	result->isAngle = left.isAngle || right.isAngle;
	result->isInteger = left.isInteger && right.isInteger;
	return true;
}

static bool _divideComponents(const Context * context, const Component left, const Component right, Component * result) {
	if (right.isAngle) {
		_semanticError(context, "the divisor must be a number, not an angle");
		return false;
	}
	if (right.number == 0) {
		_semanticError(context, "division by zero");
		return false;
	}
	result->number = left.number / right.number;
	result->isAngle = left.isAngle;
	result->isInteger = left.isInteger && right.isInteger && fmod(left.number, right.number) == 0;
	return true;
}

static bool _multiplyComponents(const Context * context, const Component left, const Component right, Component * result) {
	if (left.isAngle && right.isAngle) {
		_semanticError(context, "two angles cannot be multiplied");
		return false;
	}
	result->number = left.number * right.number;
	result->isAngle = left.isAngle || right.isAngle;
	result->isInteger = left.isInteger && right.isInteger;
	return true;
}

static bool _evaluateAddition(const Context * context, const Value * left, const Value * right, const double sign, Value * result) {
	if (left->type != right->type || (left->type != SCALAR_VALUE && left->type != VECTOR_VALUE)) {
		_semanticError(context, "addition and subtraction need two scalars or two vectors");
		return false;
	}
	if (left->dimension != right->dimension) {
		_semanticError(context, "cannot add or subtract vectors of dimensions %u and %u", left->dimension, right->dimension);
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

static bool _evaluateDivision(const Context * context, const Value * left, const Value * right, Value * result) {
	if ((left->type != SCALAR_VALUE && left->type != VECTOR_VALUE) || right->type != SCALAR_VALUE) {
		_semanticError(context, "a division needs an scalar or a vector, over a number");
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

static bool _evaluateMultiplication(const Context * context, const Value * left, const Value * right, Value * result) {
	if (left->type == SCALAR_VALUE && right->type == SCALAR_VALUE) {
		*result = *left;
		return _multiplyComponents(context, left->components[0], right->components[0], &result->components[0]);
	}
	const bool isVectorByScalar = left->type == VECTOR_VALUE && right->type == SCALAR_VALUE;
	const bool isScalarByVector = left->type == SCALAR_VALUE && right->type == VECTOR_VALUE;
	if (!isVectorByScalar && !isScalarByVector) {
		_semanticError(context, "a product needs two scalars, or a vector and a number");
		return false;
	}
	const Value * vector = isVectorByScalar ? left : right;
	const Value * scalar = isVectorByScalar ? right : left;
	if (scalar->components[0].isAngle) {
		_semanticError(context, "a vector can only be multiplied by a number, not by an angle");
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

static bool _evaluateNegation(const Context * context, const Value * operand, Value * result) {
	if (operand->type != SCALAR_VALUE && operand->type != VECTOR_VALUE) {
		_semanticError(context, "the unary minus needs an scalar or a vector");
		return false;
	}
	*result = *operand;
	for (unsigned int k = 0; k < operand->dimension; ++k) {
		result->components[k].number = -operand->components[k].number;
	}
	return true;
}

/** Expressions. */

static bool _evaluateConstant(const Constant * constant, Value * result) {
	switch (constant->type) {
		case ANGLE_CONSTANT:
			*result = _numberValue(constant->degrees, false);
			result->components[0].isAngle = true;
			return true;
		case COLOR_CONSTANT:
			*result = _colorValue(constant->color);
			return true;
		case DECIMAL_CONSTANT:
			*result = _numberValue(constant->decimal, false);
			return true;
		case INTEGER_CONSTANT:
			*result = _numberValue(constant->integer, true);
			return true;
		default:
			logError(_logger, "The specified constant type is unknown: %d", constant->type);
			++_errorCount;
			return false;
	}
}

static bool _evaluateExpression(const Context * context, const Expression * expression, Value * result) {
	Value left;
	Value right;
	bool succeeded = false;
	switch (expression->type) {
		case ADDITION:
		case DIVISION:
		case MULTIPLICATION:
		case SUBTRACTION: {
			// Both operands are evaluated, to report the errors of each one.
			const bool isLeftValid = _evaluateExpression(context, expression->leftExpression, &left);
			const bool isRightValid = _evaluateExpression(context, expression->rightExpression, &right);
			if (!isLeftValid || !isRightValid) {
				return false;
			}
			if (expression->type == ADDITION) {
				succeeded = _evaluateAddition(context, &left, &right, 1, result);
			}
			else if (expression->type == SUBTRACTION) {
				succeeded = _evaluateAddition(context, &left, &right, -1, result);
			}
			else if (expression->type == MULTIPLICATION) {
				succeeded = _evaluateMultiplication(context, &left, &right, result);
			}
			else {
				succeeded = _evaluateDivision(context, &left, &right, result);
			}
			break;
		}
		case FACTOR:
			return _evaluateFactor(context, expression->factor, result);
		case NEGATION:
			if (!_evaluateExpression(context, expression->operand, &left)) {
				return false;
			}
			succeeded = _evaluateNegation(context, &left, result);
			break;
		default:
			logError(_logger, "The specified expression type is unknown: %d", expression->type);
			++_errorCount;
			return false;
	}
	if (succeeded && !_isFinite(result)) {
		_semanticError(context, "the result of the expression is too big");
		return false;
	}
	return succeeded;
}

static bool _evaluateFactor(const Context * context, const Factor * factor, Value * result) {
	switch (factor->type) {
		case CONSTANT:
			return _evaluateConstant(factor->constant, result);
		case EXPRESSION:
			return _evaluateExpression(context, factor->expression, result);
		case REFERENCE:
			return _evaluateReference(context, factor->reference, result);
		case VECTOR:
			return _evaluateVector(context, factor->vector, result);
		default:
			logError(_logger, "The specified factor type is unknown: %d", factor->type);
			++_errorCount;
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
		_semanticError(context, "the identifier \"%s\" is not declared", reference->identifier);
		return false;
	}
	*result = _entityValue(symbol);
	for (const Member * member = reference->firstMember; member != NULL; member = member->next) {
		if (result->type == ENTITY_VALUE) {
			AttributeType attribute;
			if (!attributeFromName(member->name, &attribute) || !isAttributeApplicable(result->entity, attribute)) {
				_semanticError(context, "\"%s\" doesn't have the attribute \"%s\"", result->entity->name, member->name);
				return false;
			}
			Symbol * owner = result->entity;
			if (!_resolveAttribute(context, owner, attribute, result)) {
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
				_semanticError(context, "a vector doesn't have the component \"%s\"", member->name);
				return false;
			}
			if (result->dimension <= index) {
				_semanticError(context, "a vector of dimension %u doesn't have the component \"%s\"", result->dimension, member->name);
				return false;
			}
			const Component component = result->components[index];
			*result = _numberValue(component.number, component.isInteger);
			result->components[0].isAngle = component.isAngle;
		}
		else {
			_semanticError(context, "cannot access \"%s\" on a value that is not an entity or a vector", member->name);
			return false;
		}
	}
	return true;
}

static bool _evaluateVector(const Context * context, const Vector * vector, Value * result) {
	const Expression * expressions[3] = { vector->x, vector->y, vector->z };
	bool isValid = true;
	*result = _numberValue(0, false);
	result->type = VECTOR_VALUE;
	result->dimension = vector->dimension;
	for (unsigned int k = 0; k < vector->dimension; ++k) {
		Value component;
		if (!_evaluateExpression(context, expressions[k], &component)) {
			isValid = false;
		}
		else if (component.type != SCALAR_VALUE) {
			_semanticError(context, "the components of a vector must be numbers or angles");
			isValid = false;
		}
		else {
			result->components[k] = component.components[0];
		}
	}
	return isValid;
}

/** Attributes. */

static bool _isAnglesVector(const Value * value) {
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

static bool _isFinite(const Value * value) {
	for (unsigned int k = 0; k < value->dimension; ++k) {
		if (!isfinite(value->components[k].number)) {
			return false;
		}
	}
	return true;
}

static bool _isNumber(const Value * value) {
	return value->type == SCALAR_VALUE && !value->components[0].isAngle;
}

static bool _isNumbersVector(const Value * value, const unsigned int dimension) {
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

static bool _isPositiveNumbersVector(const Value * value, const unsigned int dimension) {
	if (!_isNumbersVector(value, dimension)) {
		return false;
	}
	for (unsigned int k = 0; k < dimension; ++k) {
		if (value->components[k].number <= 0) {
			return false;
		}
	}
	return true;
}

/**
 * Checks the type and the range of an attribute. The size of a cube is
 * normalized to a vector, so ".x", ".y" and ".z" work on it too.
 */
static bool _validateAttribute(const Context * context, const Symbol * symbol, const AttributeType attribute, Value * value) {
	switch (attribute) {
		case COLOR_ATTRIBUTE:
			if (value->type != COLOR_VALUE) {
				_semanticError(context, "it must be a color");
				return false;
			}
			return true;
		case DIRECTION_ATTRIBUTE:
			if (!_isNumbersVector(value, 3)) {
				_semanticError(context, "it must be a vector of 3 numbers");
				return false;
			}
			if (value->components[0].number == 0 && value->components[1].number == 0 && value->components[2].number == 0) {
				_semanticError(context, "the direction cannot be null");
				return false;
			}
			return true;
		case FOV_ATTRIBUTE:
			if (value->type != SCALAR_VALUE || !value->components[0].isAngle) {
				_semanticError(context, "it must be an angle (e.g., 60deg)");
				return false;
			}
			if (value->components[0].number <= 0 || 180 <= value->components[0].number) {
				_semanticError(context, "it must be greater than 0deg and less than 180deg");
				return false;
			}
			return true;
		case INTENSITY_ATTRIBUTE:
			if (!_isNumber(value) || value->components[0].number < 0 || 1 < value->components[0].number) {
				_semanticError(context, "it must be a number between 0 and 1");
				return false;
			}
			return true;
		case LOOK_AT_ATTRIBUTE:
			if (value->type == ENTITY_VALUE) {
				if (value->entity->type == MATERIAL_ENTITY) {
					_semanticError(context, "the material \"%s\" doesn't have a position", value->entity->name);
					return false;
				}
				return true;
			}
			if (!_isNumbersVector(value, 3)) {
				_semanticError(context, "it must be a vector of 3 numbers, or an entity with a position");
				return false;
			}
			return true;
		case MATERIAL_ATTRIBUTE:
			if (value->type != ENTITY_VALUE || value->entity->type != MATERIAL_ENTITY) {
				_semanticError(context, "it must be a material");
				return false;
			}
			return true;
		case POSITION_ATTRIBUTE:
			if (!_isNumbersVector(value, 3)) {
				_semanticError(context, "it must be a vector of 3 numbers");
				return false;
			}
			return true;
		case RADIUS_ATTRIBUTE:
			if (!_isNumber(value) || value->components[0].number <= 0) {
				_semanticError(context, "it must be a positive number");
				return false;
			}
			return true;
		case SHININESS_ATTRIBUTE:
			if (!_isNumber(value) || !value->components[0].isInteger || value->components[0].number < 0) {
				_semanticError(context, "it must be an integer greater than or equal to 0");
				return false;
			}
			return true;
		case SIZE_ATTRIBUTE:
			if (symbol->meshType == PLANE_MESH) {
				if (!_isPositiveNumbersVector(value, 2)) {
					_semanticError(context, "the size of a plane must be a vector of 2 positive numbers");
					return false;
				}
				return true;
			}
			if (_isNumber(value) && 0 < value->components[0].number) {
				value->type = VECTOR_VALUE;
				value->dimension = 3;
				value->components[1] = value->components[0];
				value->components[2] = value->components[0];
				return true;
			}
			if (!_isPositiveNumbersVector(value, 3)) {
				_semanticError(context, "the size of a box must be a positive number or a vector of 3 positive numbers");
				return false;
			}
			return true;
		default:
			return true;
	}
}

/**
 * Resolves an attribute lazily, with memoization. Finding an attribute that is
 * being resolved means that it depends on itself.
 */
static bool _resolveAttribute(const Context * context, Symbol * symbol, const AttributeType attribute, Value * result) {
	Attribute * target = &symbol->attributes[attribute];
	switch (target->state) {
		case RESOLVED:
			*result = target->value;
			return true;
		case RESOLVING:
			_semanticError(context, "cyclic dependency on \"%s.%s\"", symbol->name, attributeName(attribute));
			return false;
		case UNRESOLVABLE:
			return false;
		default:
			break;
	}
	if (target->expression == NULL) {
		_semanticError(context, "\"%s\" doesn't declare the attribute \"%s\"", symbol->name, attributeName(attribute));
		return false;
	}
	const Context attributeContext = {
		.entity = symbol->name,
		.attribute = attributeName(attribute)
	};
	target->state = RESOLVING;
	Value value;
	if (!_evaluateExpression(&attributeContext, target->expression, &value)
		|| !_validateAttribute(&attributeContext, symbol, attribute, &value)) {
		target->state = UNRESOLVABLE;
		return false;
	}
	target->value = value;
	target->state = RESOLVED;
	*result = value;
	return true;
}

/** Declarations. */

static void _declareAttribute(Symbol * symbol, const AttributeType attribute, Expression * expression) {
	const Context context = {
		.entity = symbol->name,
		.attribute = attributeName(attribute)
	};
	if (!isAttributeApplicable(symbol, attribute)) {
		_semanticError(&context, "the attribute doesn't apply to this type of entity");
	}
	else if (symbol->attributes[attribute].expression != NULL) {
		_semanticError(&context, "the attribute is repeated");
	}
	else {
		symbol->attributes[attribute].expression = expression;
	}
}

static void _declareProperties(Symbol * symbol) {
	for (const Property * property = symbol->properties->first; property != NULL; property = property->next) {
		switch (property->type) {
			case COLOR_PROPERTY:
				_declareAttribute(symbol, COLOR_ATTRIBUTE, property->value);
				break;
			case DIRECTION_PROPERTY:
				_declareAttribute(symbol, DIRECTION_ATTRIBUTE, property->value);
				break;
			case FOV_PROPERTY:
				_declareAttribute(symbol, FOV_ATTRIBUTE, property->value);
				break;
			case INTENSITY_PROPERTY:
				_declareAttribute(symbol, INTENSITY_ATTRIBUTE, property->value);
				break;
			case LOOK_AT_PROPERTY:
				_declareAttribute(symbol, LOOK_AT_ATTRIBUTE, property->value);
				break;
			case MATERIAL_PROPERTY:
				_declareAttribute(symbol, MATERIAL_ATTRIBUTE, property->value);
				break;
			case RADIUS_PROPERTY:
				_declareAttribute(symbol, RADIUS_ATTRIBUTE, property->value);
				break;
			case SHININESS_PROPERTY:
				_declareAttribute(symbol, SHININESS_ATTRIBUTE, property->value);
				break;
			case SIZE_PROPERTY:
				_declareAttribute(symbol, SIZE_ATTRIBUTE, property->value);
				break;
			default:
				// The transformations can be repeated, and they are analyzed later.
				break;
		}
	}
}

/**
 * Adds every entity with an identifier to the symbol table, and counts the
 * ones that must be unique. Declaring them all first allows forward references.
 */
static void _declareEntities(const Program * program, unsigned int * cameraCount, const Render ** render, unsigned int * renderCount) {
	for (const Entity * entity = program->entities->first; entity != NULL; entity = entity->next) {
		const char * identifier = NULL;
		const PropertyList * properties = NULL;
		Expression * position = NULL;
		switch (entity->type) {
			case CAMERA_ENTITY:
				identifier = entity->camera->identifier;
				properties = entity->camera->properties;
				position = entity->camera->position;
				++*cameraCount;
				break;
			case LIGHT_ENTITY:
				identifier = entity->light->identifier;
				properties = entity->light->properties;
				position = entity->light->position;
				break;
			case MATERIAL_ENTITY:
				identifier = entity->material->identifier;
				properties = entity->material->properties;
				break;
			case MESH_ENTITY:
				identifier = entity->mesh->identifier;
				properties = entity->mesh->properties;
				position = entity->mesh->position;
				break;
			default:
				*render = entity->render;
				++*renderCount;
				continue;
		}
		Symbol * symbol = addSymbol(_symbolTable, identifier, entity->type);
		if (symbol == NULL) {
			logError(_logger, "Semantic error: the identifier \"%s\" is repeated.", identifier);
			++_errorCount;
			continue;
		}
		if (entity->type == LIGHT_ENTITY) {
			symbol->lightType = entity->light->type;
		}
		else if (entity->type == MESH_ENTITY) {
			symbol->meshType = entity->mesh->type;
		}
		symbol->properties = properties;
		symbol->attributes[POSITION_ATTRIBUTE].expression = position;
		_declareProperties(symbol);
	}
}

static void _setDefaultAttribute(Symbol * symbol, const AttributeType attribute, const Value value) {
	Attribute * target = &symbol->attributes[attribute];
	if (target->expression == NULL) {
		target->value = value;
		target->state = RESOLVED;
	}
}

/**
 * Reports a required attribute without declaration. It's left as UNRESOLVABLE
 * to not report the same error on every reference to it.
 */
static void _checkDeclaredAttribute(Symbol * symbol, const AttributeType attribute) {
	Attribute * target = &symbol->attributes[attribute];
	if (target->expression == NULL) {
		const Context context = {
			.entity = symbol->name,
			.attribute = attributeName(attribute)
		};
		_semanticError(&context, "the attribute is required");
		target->state = UNRESOLVABLE;
	}
}

static void _applyDefaultsAndRequirements(Symbol * symbol) {
	const Color white = { .red = 0xFF, .green = 0xFF, .blue = 0xFF, .alpha = 0xFF, .hasAlpha = false };
	switch (symbol->type) {
		case CAMERA_ENTITY:
			_checkDeclaredAttribute(symbol, LOOK_AT_ATTRIBUTE);
			_checkDeclaredAttribute(symbol, FOV_ATTRIBUTE);
			break;
		case LIGHT_ENTITY:
			_setDefaultAttribute(symbol, COLOR_ATTRIBUTE, _colorValue(white));
			_setDefaultAttribute(symbol, INTENSITY_ATTRIBUTE, _numberValue(1, true));
			if (symbol->lightType == POINT_LIGHT) {
				_checkDeclaredAttribute(symbol, POSITION_ATTRIBUTE);
			}
			else {
				_checkDeclaredAttribute(symbol, DIRECTION_ATTRIBUTE);
			}
			break;
		case MATERIAL_ENTITY:
			_setDefaultAttribute(symbol, SHININESS_ATTRIBUTE, _numberValue(0, true));
			_checkDeclaredAttribute(symbol, COLOR_ATTRIBUTE);
			break;
		case MESH_ENTITY:
			_setDefaultAttribute(symbol, POSITION_ATTRIBUTE, _zeroVectorValue());
			_setDefaultAttribute(symbol, MATERIAL_ATTRIBUTE, _entityValue(_symbolTable->defaultMaterial));
			_checkDeclaredAttribute(symbol, symbol->meshType == SPHERE_MESH ? RADIUS_ATTRIBUTE : SIZE_ATTRIBUTE);
			break;
		default:
			break;
	}
}

/** Analysis. */

/**
 * Checks that the camera doesn't look at its own position.
 */
static void _analyzeCamera(Symbol * symbol) {
	const Context context = {
		.entity = symbol->name,
		.attribute = attributeName(LOOK_AT_ATTRIBUTE)
	};
	Value position;
	Value target;
	if (!_resolveAttribute(&context, symbol, LOOK_AT_ATTRIBUTE, &target)) {
		return;
	}
	if (target.type == ENTITY_VALUE && !_resolveAttribute(&context, target.entity, POSITION_ATTRIBUTE, &target)) {
		return;
	}
	if (!_resolveAttribute(&context, symbol, POSITION_ATTRIBUTE, &position)) {
		return;
	}
	if (position.components[0].number == target.components[0].number
		&& position.components[1].number == target.components[1].number
		&& position.components[2].number == target.components[2].number) {
		_semanticError(&context, "the camera cannot look at its own position");
	}
}

/**
 * Validates the transformations in the order they were written, and computes
 * the model matrix: T(at) * Tn * ... * T1. That is, the mesh is transformed
 * around its own origin, and then it's placed in its position.
 */
static void _analyzeMesh(Symbol * symbol) {
	double transformations[16];
	double matrix[16];
	bool isValid = true;
	_identityMatrix(transformations);
	for (const Property * property = symbol->properties->first; property != NULL; property = property->next) {
		Value value;
		if (property->type == TRANSLATE_PROPERTY) {
			const Context context = { .entity = symbol->name, .attribute = "translate" };
			if (!_evaluateExpression(&context, property->value, &value)) {
				isValid = false;
			}
			else if (!_isNumbersVector(&value, 3)) {
				_semanticError(&context, "it must be a vector of 3 numbers");
				isValid = false;
			}
			else {
				_translationMatrix(value.components[0].number, value.components[1].number, value.components[2].number, matrix);
				_leftMultiplyMatrix(matrix, transformations);
			}
		}
		else if (property->type == ROTATE_PROPERTY) {
			const Context context = { .entity = symbol->name, .attribute = "rotate" };
			if (!_evaluateExpression(&context, property->value, &value)) {
				isValid = false;
			}
			else if (!_isAnglesVector(&value)) {
				_semanticError(&context, "it must be a vector of 3 angles");
				isValid = false;
			}
			else {
				_rotationMatrix(value.components[0].number, value.components[1].number, value.components[2].number, matrix);
				_leftMultiplyMatrix(matrix, transformations);
			}
		}
		else if (property->type == SCALE_PROPERTY) {
			const Context context = { .entity = symbol->name, .attribute = "scale" };
			if (!_evaluateExpression(&context, property->value, &value)) {
				isValid = false;
			}
			else if (_isNumber(&value) && 0 < value.components[0].number) {
				_scaleMatrix(value.components[0].number, value.components[0].number, value.components[0].number, matrix);
				_leftMultiplyMatrix(matrix, transformations);
			}
			else if (_isPositiveNumbersVector(&value, 3)) {
				_scaleMatrix(value.components[0].number, value.components[1].number, value.components[2].number, matrix);
				_leftMultiplyMatrix(matrix, transformations);
			}
			else {
				_semanticError(&context, "it must be a positive number or a vector of 3 positive numbers");
				isValid = false;
			}
		}
	}
	const Context context = {
		.entity = symbol->name,
		.attribute = attributeName(POSITION_ATTRIBUTE)
	};
	Value position;
	if (isValid && _resolveAttribute(&context, symbol, POSITION_ATTRIBUTE, &position)) {
		_translationMatrix(position.components[0].number, position.components[1].number, position.components[2].number, matrix);
		_leftMultiplyMatrix(matrix, transformations);
		memcpy(symbol->model, transformations, sizeof(transformations));
	}
}

/**
 * Resolves every declared attribute of a symbol (even if nobody references
 * them), and applies the validations that involve many attributes.
 */
static void _analyzeSymbol(Symbol * symbol) {
	const Context context = { .entity = symbol->name, .attribute = "" };
	for (int k = 0; k < ATTRIBUTE_COUNT; ++k) {
		if (symbol->attributes[k].expression != NULL) {
			Value value;
			_resolveAttribute(&context, symbol, k, &value);
		}
	}
	if (symbol->type == CAMERA_ENTITY) {
		_analyzeCamera(symbol);
	}
	else if (symbol->type == MESH_ENTITY) {
		_analyzeMesh(symbol);
	}
}

static void _analyzeRender(const Render * render, int * width, int * height) {
	const Context context = { .entity = "render", .attribute = "resolution" };
	Value value;
	if (_evaluateExpression(&context, render->resolution, &value)) {
		if (!_isPositiveNumbersVector(&value, 2)
			|| !value.components[0].isInteger
			|| !value.components[1].isInteger
			|| INT_MAX < value.components[0].number
			|| INT_MAX < value.components[1].number) {
			_semanticError(&context, "it must be a vector of 2 positive integers");
		}
		else {
			*width = (int) value.components[0].number;
			*height = (int) value.components[1].number;
		}
	}
	if (render->output[0] == '\0') {
		const Context outputContext = { .entity = "render", .attribute = "to" };
		_semanticError(&outputContext, "the name of the output file cannot be empty");
	}
}

/** Matrices (row-major). */

static void _identityMatrix(double matrix[16]) {
	memset(matrix, 0, 16 * sizeof(double));
	matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
}

/**
 * Computes matrix = left * matrix.
 */
static void _leftMultiplyMatrix(const double left[16], double matrix[16]) {
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

static void _translationMatrix(const double x, const double y, const double z, double matrix[16]) {
	_identityMatrix(matrix);
	matrix[3] = x;
	matrix[7] = y;
	matrix[11] = z;
}

/**
 * Computes Rz * Ry * Rx, with the angles in degrees.
 */
static void _rotationMatrix(const double x, const double y, const double z, double matrix[16]) {
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
	_leftMultiplyMatrix(rotationY, matrix);
	_leftMultiplyMatrix(rotationZ, matrix);
}

static void _scaleMatrix(const double x, const double y, const double z, double matrix[16]) {
	_identityMatrix(matrix);
	matrix[0] = x;
	matrix[5] = y;
	matrix[10] = z;
}

/** Scene. */

static void _copyVector(const Value * value, double vector[3]) {
	for (unsigned int k = 0; k < value->dimension; ++k) {
		vector[k] = value->components[k].number;
	}
}

static double _resolvedNumber(const Symbol * symbol, const AttributeType attribute) {
	return symbol->attributes[attribute].value.components[0].number;
}

/**
 * Builds the resolved scene. It must be used only if there are no errors, so
 * every attribute required here is RESOLVED.
 */
static Scene * _createScene(const Program * program, const int width, const int height) {
	Scene * scene = calloc(1, sizeof(Scene));
	scene->name = strdup(program->name);
	scene->width = width;
	scene->height = height;
	for (const Entity * entity = program->entities->first; entity != NULL; entity = entity->next) {
		if (entity->type == RENDER_ENTITY) {
			scene->output = strdup(entity->render->output);
		}
	}
	for (const Symbol * symbol = _symbolTable->first; symbol != NULL; symbol = symbol->next) {
		if (symbol->type == LIGHT_ENTITY) {
			++scene->lightCount;
		}
		else if (symbol->type == MESH_ENTITY) {
			++scene->meshCount;
		}
	}
	scene->lights = calloc(scene->lightCount, sizeof(SceneLight));
	scene->meshes = calloc(scene->meshCount, sizeof(SceneMesh));
	SceneLight * light = scene->lights;
	SceneMesh * mesh = scene->meshes;
	for (const Symbol * symbol = _symbolTable->first; symbol != NULL; symbol = symbol->next) {
		if (symbol->type == CAMERA_ENTITY) {
			const Value * target = &symbol->attributes[LOOK_AT_ATTRIBUTE].value;
			if (target->type == ENTITY_VALUE) {
				target = &target->entity->attributes[POSITION_ATTRIBUTE].value;
			}
			_copyVector(&symbol->attributes[POSITION_ATTRIBUTE].value, scene->camera.position);
			_copyVector(target, scene->camera.target);
			scene->camera.fov = _resolvedNumber(symbol, FOV_ATTRIBUTE);
		}
		else if (symbol->type == LIGHT_ENTITY) {
			const Color color = symbol->attributes[COLOR_ATTRIBUTE].value.color;
			light->name = strdup(symbol->name);
			light->type = symbol->lightType;
			light->color[0] = color.red / 255.0;
			light->color[1] = color.green / 255.0;
			light->color[2] = color.blue / 255.0;
			light->intensity = _resolvedNumber(symbol, INTENSITY_ATTRIBUTE);
			if (symbol->lightType == POINT_LIGHT) {
				_copyVector(&symbol->attributes[POSITION_ATTRIBUTE].value, light->vector);
			}
			else {
				_copyVector(&symbol->attributes[DIRECTION_ATTRIBUTE].value, light->vector);
				// The biggest component is used first, to avoid an overflow in the length.
				const double biggest = fmax(fabs(light->vector[0]), fmax(fabs(light->vector[1]), fabs(light->vector[2])));
				const double x = light->vector[0] / biggest;
				const double y = light->vector[1] / biggest;
				const double z = light->vector[2] / biggest;
				const double length = sqrt(x * x + y * y + z * z);
				light->vector[0] = x / length;
				light->vector[1] = y / length;
				light->vector[2] = z / length;
			}
			++light;
		}
		else if (symbol->type == MESH_ENTITY) {
			const Symbol * material = symbol->attributes[MATERIAL_ATTRIBUTE].value.entity;
			const Color color = material->attributes[COLOR_ATTRIBUTE].value.color;
			mesh->name = strdup(symbol->name);
			mesh->type = symbol->meshType;
			memcpy(mesh->model, symbol->model, sizeof(mesh->model));
			mesh->color[0] = color.red / 255.0;
			mesh->color[1] = color.green / 255.0;
			mesh->color[2] = color.blue / 255.0;
			mesh->color[3] = color.alpha / 255.0;
			mesh->shininess = _resolvedNumber(material, SHININESS_ATTRIBUTE);
			if (symbol->meshType == SPHERE_MESH) {
				mesh->size[0] = _resolvedNumber(symbol, RADIUS_ATTRIBUTE);
			}
			else {
				_copyVector(&symbol->attributes[SIZE_ATTRIBUTE].value, mesh->size);
			}
			++mesh;
		}
	}
	return scene;
}

/* PUBLIC FUNCTIONS */

CompilationStatus executeSemanticAnalysis(CompilerState * compilerState) {
	logDebugging(_logger, "Analyzing the program...");
	const Program * program = compilerState->abstractSyntaxtTree;
	const Render * render = NULL;
	unsigned int cameraCount = 0;
	unsigned int renderCount = 0;
	int width = 0;
	int height = 0;
	_errorCount = 0;
	_symbolTable = createSymbolTable();
	Symbol * defaultMaterial = _symbolTable->defaultMaterial;
	const Color gray = { .red = 0x80, .green = 0x80, .blue = 0x80, .alpha = 0xFF, .hasAlpha = false };
	_setDefaultAttribute(defaultMaterial, COLOR_ATTRIBUTE, _colorValue(gray));
	_setDefaultAttribute(defaultMaterial, SHININESS_ATTRIBUTE, _numberValue(0, true));
	_declareEntities(program, &cameraCount, &render, &renderCount);
	if (cameraCount != 1) {
		logError(_logger, "Semantic error: the scene must have exactly one camera, but it has %u.", cameraCount);
		++_errorCount;
	}
	if (renderCount != 1) {
		logError(_logger, "Semantic error: the scene must have exactly one render, but it has %u.", renderCount);
		++_errorCount;
	}
	for (Symbol * symbol = _symbolTable->first; symbol != NULL; symbol = symbol->next) {
		_applyDefaultsAndRequirements(symbol);
	}
	for (Symbol * symbol = _symbolTable->first; symbol != NULL; symbol = symbol->next) {
		_analyzeSymbol(symbol);
	}
	if (render != NULL) {
		_analyzeRender(render, &width, &height);
	}
	CompilationStatus status = FAILED;
	if (_errorCount == 0) {
		compilerState->scene = _createScene(program, width, height);
		status = SUCCEEDED;
	}
	else {
		logError(_logger, "The semantic analysis found %u error(s).", _errorCount);
	}
	destroySymbolTable(_symbolTable);
	_symbolTable = NULL;
	logDebugging(_logger, "The analysis is done.");
	return status;
}

void destroyScene(Scene * scene) {
	if (scene != NULL) {
		for (unsigned int k = 0; k < scene->lightCount; ++k) {
			free(scene->lights[k].name);
		}
		for (unsigned int k = 0; k < scene->meshCount; ++k) {
			free(scene->meshes[k].name);
		}
		free(scene->lights);
		free(scene->meshes);
		free(scene->name);
		free(scene->output);
		free(scene);
	}
}
