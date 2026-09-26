#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

/**
 * This type definitions allows self-referencing types (e.g., an expression
 * that is made of another expressions, such as talking about you in 3rd
 * person, but without the madness).
 */

typedef enum ConstantType ConstantType;
typedef enum EntityType EntityType;
typedef enum ExpressionType ExpressionType;
typedef enum FactorType FactorType;
typedef enum LightType LightType;
typedef enum MeshType MeshType;
typedef enum PropertyType PropertyType;

typedef struct Camera Camera;
typedef struct Color Color;
typedef struct Constant Constant;
typedef struct Entity Entity;
typedef struct EntityList EntityList;
typedef struct Expression Expression;
typedef struct Factor Factor;
typedef struct Light Light;
typedef struct Material Material;
typedef struct Member Member;
typedef struct Mesh Mesh;
typedef struct Program Program;
typedef struct Property Property;
typedef struct PropertyList PropertyList;
typedef struct Reference Reference;
typedef struct Render Render;
typedef struct Vector Vector;

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

enum ConstantType {
	ANGLE_CONSTANT,
	COLOR_CONSTANT,
	DECIMAL_CONSTANT,
	INTEGER_CONSTANT
};

enum EntityType {
	CAMERA_ENTITY,
	LIGHT_ENTITY,
	MATERIAL_ENTITY,
	MESH_ENTITY,
	RENDER_ENTITY
};

enum ExpressionType {
	ADDITION,
	DIVISION,
	FACTOR,
	MULTIPLICATION,
	NEGATION,
	SUBTRACTION
};

enum FactorType {
	CONSTANT,
	EXPRESSION,
	REFERENCE,
	VECTOR
};

enum LightType {
	DIRECTIONAL_LIGHT,
	POINT_LIGHT
};

enum MeshType {
	BOX_MESH,
	PLANE_MESH,
	SPHERE_MESH
};

enum PropertyType {
	COLOR_PROPERTY,
	DIRECTION_PROPERTY,
	FOV_PROPERTY,
	INTENSITY_PROPERTY,
	LOOK_AT_PROPERTY,
	MATERIAL_PROPERTY,
	RADIUS_PROPERTY,
	ROTATE_PROPERTY,
	SCALE_PROPERTY,
	SHININESS_PROPERTY,
	SIZE_PROPERTY,
	TRANSLATE_PROPERTY
};

/**
 * A color literal (#rrggbb or #rrggbbaa). If the alpha channel is omitted,
 * the color is fully opaque (alpha = 0xFF).
 */
struct Color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
	bool hasAlpha;
};

struct Constant {
	union {
		/** The value of an angle, in degrees. */
		double degrees;
		Color color;
		double decimal;
		int integer;
	};
	ConstantType type;
};

/**
 * An attribute access inside a reference (e.g., "pos", "size" or "y" in
 * "mesa.size.y").
 */
struct Member {
	char * name;
	Member * next;
};

/**
 * A reference to another entity or to one of its attributes (e.g., "pelota",
 * "pelota.pos" or "mesa.size.y"). The members are kept in order.
 */
struct Reference {
	char * identifier;
	Member * firstMember;
	Member * lastMember;
	unsigned int memberCount;
};

/**
 * A vector literal of 2 or 3 components. The "z" component is NULL when the
 * dimension is 2.
 */
struct Vector {
	Expression * x;
	Expression * y;
	Expression * z;
	unsigned int dimension;
};

struct Factor {
	union {
		Constant * constant;
		Expression * expression;
		Reference * reference;
		Vector * vector;
	};
	FactorType type;
};

struct Expression {
	union {
		Factor * factor;
		/** The operand of an unary operation (NEGATION). */
		Expression * operand;
		struct {
			Expression * leftExpression;
			Expression * rightExpression;
		};
	};
	ExpressionType type;
};

/**
 * An attribute of an entity (e.g., "radius 0.7"). Properties are kept in the
 * same order as they were written, because the transformations (translate,
 * rotate and scale) must be applied in that order.
 */
struct Property {
	Expression * value;
	PropertyType type;
	Property * next;
};

struct PropertyList {
	Property * first;
	Property * last;
	unsigned int size;
};

struct Camera {
	char * identifier;
	Expression * position;
	PropertyList * properties;
};

struct Light {
	char * identifier;
	/** Optional, it can be NULL. */
	Expression * position;
	PropertyList * properties;
	LightType type;
};

struct Material {
	char * identifier;
	PropertyList * properties;
};

struct Mesh {
	char * identifier;
	/** Optional, it can be NULL. */
	Expression * position;
	PropertyList * properties;
	MeshType type;
};

struct Render {
	Expression * resolution;
	char * output;
};

struct Entity {
	union {
		Camera * camera;
		Light * light;
		Material * material;
		Mesh * mesh;
		Render * render;
	};
	EntityType type;
	Entity * next;
};

/**
 * The entities of a scene, in the same order as they were declared.
 */
struct EntityList {
	Entity * first;
	Entity * last;
	unsigned int size;
};

/**
 * The root of the AST: a scene with its name and its entities.
 */
struct Program {
	char * name;
	EntityList * entities;
};

/**
 * Node recursive super-duper-trambolik-destructors.
 */

void destroyCamera(Camera * camera);
void destroyConstant(Constant * constant);
void destroyEntity(Entity * entity);
void destroyEntityList(EntityList * entityList);
void destroyExpression(Expression * expression);
void destroyFactor(Factor * factor);
void destroyLight(Light * light);
void destroyMaterial(Material * material);
void destroyMesh(Mesh * mesh);
void destroyProgram(Program * program);
void destroyProperty(Property * property);
void destroyPropertyList(PropertyList * propertyList);
void destroyReference(Reference * reference);
void destroyRender(Render * render);
void destroyVector(Vector * vector);

#endif
