#ifndef SCENE_HEADER
#define SCENE_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "SymbolTable.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct Scene Scene;
typedef struct SceneCamera SceneCamera;
typedef struct SceneLight SceneLight;
typedef struct SceneMesh SceneMesh;

/**
 * The resolved scene: every value in here is final, so the code generation
 * doesn't need the AST.
 */

struct SceneCamera {
	double position[3];
	double target[3];
	/** The vertical field of view, in degrees. */
	double fov;
};

struct SceneLight {
	char * name;
	/** The position of a point light, or the normalized direction of a directional one. */
	double vector[3];
	/** The RGB channels, in the range [0, 1]. */
	double color[3];
	double intensity;
	LightType type;
};

struct SceneMesh {
	char * name;
	/** A box uses (w, h, d), a plane uses (w, d) and a sphere uses (radius). */
	double size[3];
	/** The model matrix (row-major). */
	double model[16];
	/** The RGBA channels, in the range [0, 1]. */
	double color[4];
	double shininess;
	MeshType type;
};

struct Scene {
	char * name;
	char * output;
	int width;
	int height;
	SceneCamera camera;
	SceneLight * lights;
	unsigned int lightCount;
	SceneMesh * meshes;
	unsigned int meshCount;
};

/**
 * Builds the resolved scene from the symbol table. It must be used only if the
 * analysis found no errors, so every attribute required here is RESOLVED.
 */
Scene * createScene(const Program * program, const SymbolTable * symbolTable, const int width, const int height);

/**
 * Releases a resolved scene.
 */
void destroyScene(Scene * scene);

#endif
