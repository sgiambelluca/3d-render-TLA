#include "SemanticAnalyzer.h"

/* MODULE INTERNAL STATE */

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

/** Declarations. */

static void _declareAttribute(Symbol * symbol, const AttributeType attribute, Expression * expression) {
	const Context context = {
		.entity = symbol->name,
		.attribute = attributeName(attribute)
	};
	if (!isAttributeApplicable(symbol, attribute)) {
		reportSemanticError(&context, "the attribute doesn't apply to this type of entity");
	}
	else if (symbol->attributes[attribute].expression != NULL) {
		reportSemanticError(&context, "the attribute is repeated");
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
			reportError("Semantic error: the identifier \"%s\" is repeated.", identifier);
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
		reportSemanticError(&context, "the attribute is required");
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
			_setDefaultAttribute(symbol, COLOR_ATTRIBUTE, colorValue(white));
			_setDefaultAttribute(symbol, INTENSITY_ATTRIBUTE, numberValue(1, true));
			if (symbol->lightType == POINT_LIGHT) {
				_checkDeclaredAttribute(symbol, POSITION_ATTRIBUTE);
			}
			else {
				_checkDeclaredAttribute(symbol, DIRECTION_ATTRIBUTE);
			}
			break;
		case MATERIAL_ENTITY:
			_setDefaultAttribute(symbol, SHININESS_ATTRIBUTE, numberValue(0, true));
			_checkDeclaredAttribute(symbol, COLOR_ATTRIBUTE);
			break;
		case MESH_ENTITY:
			_setDefaultAttribute(symbol, POSITION_ATTRIBUTE, zeroVectorValue());
			_setDefaultAttribute(symbol, MATERIAL_ATTRIBUTE, entityValue(_symbolTable->defaultMaterial));
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
	if (!resolveAttribute(&context, symbol, LOOK_AT_ATTRIBUTE, &target)) {
		return;
	}
	if (target.type == ENTITY_VALUE && !resolveAttribute(&context, target.entity, POSITION_ATTRIBUTE, &target)) {
		return;
	}
	if (!resolveAttribute(&context, symbol, POSITION_ATTRIBUTE, &position)) {
		return;
	}
	if (position.components[0].number == target.components[0].number
		&& position.components[1].number == target.components[1].number
		&& position.components[2].number == target.components[2].number) {
		reportSemanticError(&context, "the camera cannot look at its own position");
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
	identityMatrix(transformations);
	for (const Property * property = symbol->properties->first; property != NULL; property = property->next) {
		Value value;
		if (property->type == TRANSLATE_PROPERTY) {
			const Context context = { .entity = symbol->name, .attribute = "translate" };
			if (!evaluateExpression(&context, property->value, &value)) {
				isValid = false;
			}
			else if (!isNumbersVector(&value, 3)) {
				reportSemanticError(&context, "it must be a vector of 3 numbers");
				isValid = false;
			}
			else {
				translationMatrix(value.components[0].number, value.components[1].number, value.components[2].number, matrix);
				leftMultiplyMatrix(matrix, transformations);
			}
		}
		else if (property->type == ROTATE_PROPERTY) {
			const Context context = { .entity = symbol->name, .attribute = "rotate" };
			if (!evaluateExpression(&context, property->value, &value)) {
				isValid = false;
			}
			else if (!isAnglesVector(&value)) {
				reportSemanticError(&context, "it must be a vector of 3 angles");
				isValid = false;
			}
			else {
				rotationMatrix(value.components[0].number, value.components[1].number, value.components[2].number, matrix);
				leftMultiplyMatrix(matrix, transformations);
			}
		}
		else if (property->type == SCALE_PROPERTY) {
			const Context context = { .entity = symbol->name, .attribute = "scale" };
			if (!evaluateExpression(&context, property->value, &value)) {
				isValid = false;
			}
			else if (isNumber(&value) && 0 < value.components[0].number) {
				scaleMatrix(value.components[0].number, value.components[0].number, value.components[0].number, matrix);
				leftMultiplyMatrix(matrix, transformations);
			}
			else if (isPositiveNumbersVector(&value, 3)) {
				scaleMatrix(value.components[0].number, value.components[1].number, value.components[2].number, matrix);
				leftMultiplyMatrix(matrix, transformations);
			}
			else {
				reportSemanticError(&context, "it must be a positive number or a vector of 3 positive numbers");
				isValid = false;
			}
		}
	}
	const Context context = {
		.entity = symbol->name,
		.attribute = attributeName(POSITION_ATTRIBUTE)
	};
	Value position;
	if (isValid && resolveAttribute(&context, symbol, POSITION_ATTRIBUTE, &position)) {
		translationMatrix(position.components[0].number, position.components[1].number, position.components[2].number, matrix);
		leftMultiplyMatrix(matrix, transformations);
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
			resolveAttribute(&context, symbol, k, &value);
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
	if (evaluateExpression(&context, render->resolution, &value)) {
		if (!isPositiveNumbersVector(&value, 2)
			|| !value.components[0].isInteger
			|| !value.components[1].isInteger
			|| INT_MAX < value.components[0].number
			|| INT_MAX < value.components[1].number) {
			reportSemanticError(&context, "it must be a vector of 2 positive integers");
		}
		else {
			*width = (int) value.components[0].number;
			*height = (int) value.components[1].number;
		}
	}
	if (render->output[0] == '\0') {
		const Context outputContext = { .entity = "render", .attribute = "to" };
		reportSemanticError(&outputContext, "the name of the output file cannot be empty");
	}
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
	initializeSemanticErrors(_logger);
	_symbolTable = createSymbolTable();
	setResolverSymbolTable(_symbolTable);
	Symbol * defaultMaterial = _symbolTable->defaultMaterial;
	const Color gray = { .red = 0x80, .green = 0x80, .blue = 0x80, .alpha = 0xFF, .hasAlpha = false };
	_setDefaultAttribute(defaultMaterial, COLOR_ATTRIBUTE, colorValue(gray));
	_setDefaultAttribute(defaultMaterial, SHININESS_ATTRIBUTE, numberValue(0, true));
	_declareEntities(program, &cameraCount, &render, &renderCount);
	if (cameraCount != 1) {
		reportError("Semantic error: the scene must have exactly one camera, but it has %u.", cameraCount);
	}
	if (renderCount != 1) {
		reportError("Semantic error: the scene must have exactly one render, but it has %u.", renderCount);
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
	if (semanticErrorCount() == 0) {
		compilerState->scene = createScene(program, _symbolTable, width, height);
		status = SUCCEEDED;
	}
	else {
		logError(_logger, "The semantic analysis found %u error(s).", semanticErrorCount());
	}
	setResolverSymbolTable(NULL);
	destroySymbolTable(_symbolTable);
	_symbolTable = NULL;
	logDebugging(_logger, "The analysis is done.");
	return status;
}
