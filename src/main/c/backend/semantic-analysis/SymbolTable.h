#ifndef SYMBOL_TABLE_HEADER
#define SYMBOL_TABLE_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef enum AttributeState AttributeState;
typedef enum AttributeType AttributeType;
typedef enum ValueType ValueType;

typedef struct Attribute Attribute;
typedef struct Component Component;
typedef struct Symbol Symbol;
typedef struct SymbolTable SymbolTable;
typedef struct Value Value;

/**
 * The attributes that can be accessed through a reference (e.g., "a.pos"). The
 * transformations are not attributes, because they are never referenced and
 * can be repeated.
 */
enum AttributeType {
	COLOR_ATTRIBUTE,
	DIRECTION_ATTRIBUTE,
	FOV_ATTRIBUTE,
	INTENSITY_ATTRIBUTE,
	LOOK_AT_ATTRIBUTE,
	MATERIAL_ATTRIBUTE,
	POSITION_ATTRIBUTE,
	RADIUS_ATTRIBUTE,
	SHININESS_ATTRIBUTE,
	SIZE_ATTRIBUTE,
	ATTRIBUTE_COUNT
};

/**
 * The state of the lazy resolution of an attribute. Finding an attribute in
 * the RESOLVING state means that it depends on itself. An UNRESOLVABLE
 * attribute already reported its error, so whoever depends on it fails without
 * reporting it again.
 */
enum AttributeState {
	UNRESOLVED,
	RESOLVING,
	RESOLVED,
	UNRESOLVABLE
};

enum ValueType {
	COLOR_VALUE,
	ENTITY_VALUE,
	SCALAR_VALUE,
	VECTOR_VALUE
};

/**
 * A number or an angle (in degrees), which is an scalar or a vector component.
 */
struct Component {
	double number;
	bool isAngle;
	bool isInteger;
};

/**
 * The result of evaluating an expression. An scalar uses only the first
 * component.
 */
struct Value {
	Component components[3];
	unsigned int dimension;
	Color color;
	Symbol * entity;
	ValueType type;
};

struct Attribute {
	/** It is NULL if the attribute was not declared. */
	Expression * expression;
	Value value;
	AttributeState state;
};

/**
 * An entity with an identifier. The identifier and the properties belong to
 * the AST.
 */
struct Symbol {
	const char * name;
	const PropertyList * properties;
	Attribute attributes[ATTRIBUTE_COUNT];
	/** The model matrix (row-major), only for meshes. */
	double model[16];
	EntityType type;
	LightType lightType;
	MeshType meshType;
	Symbol * next;
};

/**
 * The symbols of the only scope of the language (the scene), in the same order
 * as they were declared.
 */
struct SymbolTable {
	Symbol * first;
	Symbol * last;
	/** The material of the meshes without one. It cannot be referenced by name. */
	Symbol * defaultMaterial;
};

SymbolTable * createSymbolTable();
void destroySymbolTable(SymbolTable * symbolTable);

/**
 * Adds a new symbol, or returns NULL if the identifier already exists.
 */
Symbol * addSymbol(SymbolTable * symbolTable, const char * name, const EntityType type);

/**
 * Finds a symbol by identifier, or returns NULL if it was not declared.
 */
Symbol * findSymbol(const SymbolTable * symbolTable, const char * name);

/**
 * Finds the attribute with the name used in a reference. Returns false if
 * there is no attribute with that name.
 */
bool attributeFromName(const char * name, AttributeType * attribute);

/**
 * The name of an attribute, as it's used in a reference.
 */
const char * attributeName(const AttributeType attribute);

/**
 * Determines if an attribute can be used in a symbol, according to its entity
 * type and subtype (e.g., "radius" only applies to spheres).
 */
bool isAttributeApplicable(const Symbol * symbol, const AttributeType attribute);

#endif
