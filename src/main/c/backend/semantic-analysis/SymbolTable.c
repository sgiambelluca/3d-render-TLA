#include "SymbolTable.h"

/* MODULE INTERNAL STATE */

static const char * _attributeNames[ATTRIBUTE_COUNT] = {
	[COLOR_ATTRIBUTE] = "color",
	[DIRECTION_ATTRIBUTE] = "direction",
	[FOV_ATTRIBUTE] = "fov",
	[INTENSITY_ATTRIBUTE] = "intensity",
	[LOOK_AT_ATTRIBUTE] = "look_at",
	[MATERIAL_ATTRIBUTE] = "material",
	[POSITION_ATTRIBUTE] = "pos",
	[RADIUS_ATTRIBUTE] = "radius",
	[SHININESS_ATTRIBUTE] = "shininess",
	[SIZE_ATTRIBUTE] = "size"
};

/* PRIVATE FUNCTIONS */

static Symbol * _createSymbol(const char * name, const EntityType type);

/**
 * Creates a new symbol, with every attribute undeclared and unresolved.
 */
static Symbol * _createSymbol(const char * name, const EntityType type) {
	Symbol * symbol = calloc(1, sizeof(Symbol));
	symbol->name = name;
	symbol->type = type;
	for (int k = 0; k < ATTRIBUTE_COUNT; ++k) {
		symbol->attributes[k].expression = NULL;
		symbol->attributes[k].state = UNRESOLVED;
	}
	return symbol;
}

/* PUBLIC FUNCTIONS */

SymbolTable * createSymbolTable() {
	SymbolTable * symbolTable = calloc(1, sizeof(SymbolTable));
	symbolTable->defaultMaterial = _createSymbol("default material", MATERIAL_ENTITY);
	return symbolTable;
}

void destroySymbolTable(SymbolTable * symbolTable) {
	if (symbolTable != NULL) {
		Symbol * symbol = symbolTable->first;
		while (symbol != NULL) {
			Symbol * next = symbol->next;
			free(symbol);
			symbol = next;
		}
		free(symbolTable->defaultMaterial);
		free(symbolTable);
	}
}

Symbol * addSymbol(SymbolTable * symbolTable, const char * name, const EntityType type) {
	if (findSymbol(symbolTable, name) != NULL) {
		return NULL;
	}
	Symbol * symbol = _createSymbol(name, type);
	if (symbolTable->last == NULL) {
		symbolTable->first = symbol;
	}
	else {
		symbolTable->last->next = symbol;
	}
	symbolTable->last = symbol;
	return symbol;
}

Symbol * findSymbol(const SymbolTable * symbolTable, const char * name) {
	for (Symbol * symbol = symbolTable->first; symbol != NULL; symbol = symbol->next) {
		if (strcmp(symbol->name, name) == 0) {
			return symbol;
		}
	}
	return NULL;
}

bool attributeFromName(const char * name, AttributeType * attribute) {
	for (int k = 0; k < ATTRIBUTE_COUNT; ++k) {
		if (strcmp(_attributeNames[k], name) == 0) {
			*attribute = k;
			return true;
		}
	}
	return false;
}

const char * attributeName(const AttributeType attribute) {
	return _attributeNames[attribute];
}

bool isAttributeApplicable(const Symbol * symbol, const AttributeType attribute) {
	switch (symbol->type) {
		case CAMERA_ENTITY:
			return attribute == POSITION_ATTRIBUTE
				|| attribute == LOOK_AT_ATTRIBUTE
				|| attribute == FOV_ATTRIBUTE;
		case LIGHT_ENTITY:
			return attribute == POSITION_ATTRIBUTE
				|| attribute == COLOR_ATTRIBUTE
				|| attribute == INTENSITY_ATTRIBUTE
				|| (attribute == DIRECTION_ATTRIBUTE && symbol->lightType == DIRECTIONAL_LIGHT);
		case MATERIAL_ENTITY:
			return attribute == COLOR_ATTRIBUTE
				|| attribute == SHININESS_ATTRIBUTE;
		case MESH_ENTITY:
			return attribute == POSITION_ATTRIBUTE
				|| attribute == MATERIAL_ATTRIBUTE
				|| (attribute == RADIUS_ATTRIBUTE && symbol->meshType == SPHERE_MESH)
				|| (attribute == SIZE_ATTRIBUTE && symbol->meshType != SPHERE_MESH);
		default:
			return false;
	}
}
