#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* PUBLIC FUNCTIONS */

void destroyCamera(Camera * camera) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (camera != NULL) {
		free(camera->identifier);
		destroyExpression(camera->position);
		destroyPropertyList(camera->properties);
		free(camera);
	}
}

void destroyConstant(Constant * constant) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (constant != NULL) {
		free(constant);
	}
}

void destroyEntity(Entity * entity) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (entity != NULL) {
		switch (entity->type) {
			case CAMERA_ENTITY:
				destroyCamera(entity->camera);
				break;
			case LIGHT_ENTITY:
				destroyLight(entity->light);
				break;
			case MATERIAL_ENTITY:
				destroyMaterial(entity->material);
				break;
			case MESH_ENTITY:
				destroyMesh(entity->mesh);
				break;
			case RENDER_ENTITY:
				destroyRender(entity->render);
				break;
		}
		free(entity);
	}
}

void destroyEntityList(EntityList * entityList) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (entityList != NULL) {
		Entity * entity = entityList->first;
		while (entity != NULL) {
			Entity * next = entity->next;
			destroyEntity(entity);
			entity = next;
		}
		free(entityList);
	}
}

void destroyExpression(Expression * expression) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (expression != NULL) {
		switch (expression->type) {
			case ADDITION:
			case DIVISION:
			case MULTIPLICATION:
			case SUBTRACTION:
				destroyExpression(expression->leftExpression);
				destroyExpression(expression->rightExpression);
				break;
			case FACTOR:
				destroyFactor(expression->factor);
				break;
			case NEGATION:
				destroyExpression(expression->operand);
				break;
		}
		free(expression);
	}
}

void destroyFactor(Factor * factor) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (factor != NULL) {
		switch (factor->type) {
			case CONSTANT:
				destroyConstant(factor->constant);
				break;
			case EXPRESSION:
				destroyExpression(factor->expression);
				break;
			case REFERENCE:
				destroyReference(factor->reference);
				break;
			case VECTOR:
				destroyVector(factor->vector);
				break;
		}
		free(factor);
	}
}

void destroyLight(Light * light) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (light != NULL) {
		free(light->identifier);
		destroyExpression(light->position);
		destroyPropertyList(light->properties);
		free(light);
	}
}

void destroyMaterial(Material * material) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (material != NULL) {
		free(material->identifier);
		destroyPropertyList(material->properties);
		free(material);
	}
}

void destroyMesh(Mesh * mesh) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (mesh != NULL) {
		free(mesh->identifier);
		destroyExpression(mesh->position);
		destroyPropertyList(mesh->properties);
		free(mesh);
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		free(program->name);
		destroyEntityList(program->entities);
		free(program);
	}
}

void destroyProperty(Property * property) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (property != NULL) {
		destroyExpression(property->value);
		free(property);
	}
}

void destroyPropertyList(PropertyList * propertyList) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (propertyList != NULL) {
		Property * property = propertyList->first;
		while (property != NULL) {
			Property * next = property->next;
			destroyProperty(property);
			property = next;
		}
		free(propertyList);
	}
}

void destroyReference(Reference * reference) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (reference != NULL) {
		Member * member = reference->firstMember;
		while (member != NULL) {
			Member * next = member->next;
			free(member->name);
			free(member);
			member = next;
		}
		free(reference->identifier);
		free(reference);
	}
}

void destroyRender(Render * render) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (render != NULL) {
		destroyExpression(render->resolution);
		free(render->output);
		free(render);
	}
}

void destroyVector(Vector * vector) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (vector != NULL) {
		destroyExpression(vector->x);
		destroyExpression(vector->y);
		destroyExpression(vector->z);
		free(vector);
	}
}
