#include "BisonActions.h"

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

/* IMPORTED FUNCTIONS */

/* PRIVATE FUNCTIONS */

static Constant * _createConstant(ConstantType type);
static Entity * _createEntity(EntityType type);
static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Creates a new constant node of the specified type.
 */
static Constant * _createConstant(ConstantType type) {
	Constant * constant = calloc(1, sizeof(Constant));
	constant->type = type;
	return constant;
}

/**
 * Creates a new entity node of the specified type.
 */
static Entity * _createEntity(EntityType type) {
	Entity * entity = calloc(1, sizeof(Entity));
	entity->type = type;
	entity->next = NULL;
	return entity;
}

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

void SyntacticErrorAction(const char * message) {
	logError(_logger, "Syntactic error: %s.", message);
}

/** Constants. */

Constant * AngleConstantSemanticAction(const double degrees) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = _createConstant(ANGLE_CONSTANT);
	constant->degrees = degrees;
	return constant;
}

Constant * ColorConstantSemanticAction(const Color color) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = _createConstant(COLOR_CONSTANT);
	constant->color = color;
	return constant;
}

Constant * DecimalConstantSemanticAction(const double value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = _createConstant(DECIMAL_CONSTANT);
	constant->decimal = value;
	return constant;
}

Constant * IntegerConstantSemanticAction(const int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = _createConstant(INTEGER_CONSTANT);
	constant->integer = value;
	return constant;
}

/** Expressions. */

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = leftExpression;
	expression->rightExpression = rightExpression;
	expression->type = type;
	return expression;
}

Expression * FactorExpressionSemanticAction(Factor * factor) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->factor = factor;
	expression->type = FACTOR;
	return expression;
}

Expression * NegationExpressionSemanticAction(Expression * operand) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->operand = operand;
	expression->type = NEGATION;
	return expression;
}

/** Factors. */

Factor * ConstantFactorSemanticAction(Constant * constant) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->constant = constant;
	factor->type = CONSTANT;
	return factor;
}

Factor * ExpressionFactorSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->expression = expression;
	factor->type = EXPRESSION;
	return factor;
}

Factor * ReferenceFactorSemanticAction(Reference * reference) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->reference = reference;
	factor->type = REFERENCE;
	return factor;
}

Factor * VectorFactorSemanticAction(Vector * vector) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->vector = vector;
	factor->type = VECTOR;
	return factor;
}

/** References and vectors. */

Reference * MemberReferenceSemanticAction(Reference * reference, char * name) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Member * member = calloc(1, sizeof(Member));
	member->name = name;
	member->next = NULL;
	if (reference->lastMember == NULL) {
		reference->firstMember = member;
	}
	else {
		reference->lastMember->next = member;
	}
	reference->lastMember = member;
	++reference->memberCount;
	return reference;
}

Reference * ReferenceSemanticAction(char * identifier) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Reference * reference = calloc(1, sizeof(Reference));
	reference->identifier = identifier;
	reference->firstMember = NULL;
	reference->lastMember = NULL;
	reference->memberCount = 0;
	return reference;
}

Vector * VectorSemanticAction(Expression * x, Expression * y, Expression * z) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Vector * vector = calloc(1, sizeof(Vector));
	vector->x = x;
	vector->y = y;
	vector->z = z;
	vector->dimension = z == NULL ? 2 : 3;
	return vector;
}

/** Properties. */

Property * PropertySemanticAction(PropertyType type, Expression * value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Property * property = calloc(1, sizeof(Property));
	property->type = type;
	property->value = value;
	property->next = NULL;
	return property;
}

PropertyList * AppendPropertySemanticAction(PropertyList * propertyList, Property * property) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (propertyList->last == NULL) {
		propertyList->first = property;
	}
	else {
		propertyList->last->next = property;
	}
	propertyList->last = property;
	++propertyList->size;
	return propertyList;
}

PropertyList * EmptyPropertyListSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	PropertyList * propertyList = calloc(1, sizeof(PropertyList));
	propertyList->first = NULL;
	propertyList->last = NULL;
	propertyList->size = 0;
	return propertyList;
}

/** Entities. */

Camera * CameraSemanticAction(char * identifier, Expression * position, PropertyList * properties) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Camera * camera = calloc(1, sizeof(Camera));
	camera->identifier = identifier;
	camera->position = position;
	camera->properties = properties;
	return camera;
}

Light * LightSemanticAction(char * identifier, LightType type, Expression * position, PropertyList * properties) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Light * light = calloc(1, sizeof(Light));
	light->identifier = identifier;
	light->type = type;
	light->position = position;
	light->properties = properties;
	return light;
}

Material * MaterialSemanticAction(char * identifier, PropertyList * properties) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Material * material = calloc(1, sizeof(Material));
	material->identifier = identifier;
	material->properties = properties;
	return material;
}

Mesh * MeshSemanticAction(char * identifier, MeshType type, Expression * position, PropertyList * properties) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Mesh * mesh = calloc(1, sizeof(Mesh));
	mesh->identifier = identifier;
	mesh->type = type;
	mesh->position = position;
	mesh->properties = properties;
	return mesh;
}

Render * RenderSemanticAction(Expression * resolution, char * output) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Render * render = calloc(1, sizeof(Render));
	render->resolution = resolution;
	render->output = output;
	return render;
}

Entity * CameraEntitySemanticAction(Camera * camera) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Entity * entity = _createEntity(CAMERA_ENTITY);
	entity->camera = camera;
	return entity;
}

Entity * LightEntitySemanticAction(Light * light) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Entity * entity = _createEntity(LIGHT_ENTITY);
	entity->light = light;
	return entity;
}

Entity * MaterialEntitySemanticAction(Material * material) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Entity * entity = _createEntity(MATERIAL_ENTITY);
	entity->material = material;
	return entity;
}

Entity * MeshEntitySemanticAction(Mesh * mesh) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Entity * entity = _createEntity(MESH_ENTITY);
	entity->mesh = mesh;
	return entity;
}

Entity * RenderEntitySemanticAction(Render * render) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Entity * entity = _createEntity(RENDER_ENTITY);
	entity->render = render;
	return entity;
}

EntityList * AppendEntitySemanticAction(EntityList * entityList, Entity * entity) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (entityList->last == NULL) {
		entityList->first = entity;
	}
	else {
		entityList->last->next = entity;
	}
	entityList->last = entity;
	++entityList->size;
	return entityList;
}

EntityList * EmptyEntityListSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	EntityList * entityList = calloc(1, sizeof(EntityList));
	entityList->first = NULL;
	entityList->last = NULL;
	entityList->size = 0;
	return entityList;
}

/** Program. */

Program * SceneProgramSemanticAction(char * name, EntityList * entities) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->name = name;
	program->entities = entities;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}
