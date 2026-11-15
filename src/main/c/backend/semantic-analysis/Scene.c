#include "Scene.h"

/* PRIVATE FUNCTIONS */

static void _copyVector(const Value * value, double vector[3]);
static double _resolvedNumber(const Symbol * symbol, const AttributeType attribute);

static void _copyVector(const Value * value, double vector[3]) {
	for (unsigned int k = 0; k < value->dimension; ++k) {
		vector[k] = value->components[k].number;
	}
}

static double _resolvedNumber(const Symbol * symbol, const AttributeType attribute) {
	return symbol->attributes[attribute].value.components[0].number;
}

/* PUBLIC FUNCTIONS */

Scene * createScene(const Program * program, const SymbolTable * symbolTable, const int width, const int height) {
	Scene * scene = calloc(1, sizeof(Scene));
	scene->name = strdup(program->name);
	scene->width = width;
	scene->height = height;
	for (const Entity * entity = program->entities->first; entity != NULL; entity = entity->next) {
		if (entity->type == RENDER_ENTITY) {
			scene->output = strdup(entity->render->output);
		}
	}
	for (const Symbol * symbol = symbolTable->first; symbol != NULL; symbol = symbol->next) {
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
	for (const Symbol * symbol = symbolTable->first; symbol != NULL; symbol = symbol->next) {
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
