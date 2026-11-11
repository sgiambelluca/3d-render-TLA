#include "Generator.h"

/* MODULE INTERNAL STATE */

const char _indentationCharacter = '\t';
const char _indentationSize = 1;
static FILE * _file = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownGeneratorModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: Generator...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeGeneratorModule() {
	_logger = createLogger("Generator");
	return _shutdownGeneratorModule;
}

/** PRIVATE FUNCTIONS */

static bool _createParentDirectories(const char * path);
static void _generateCamera(const unsigned int indentationLevel, const SceneCamera * camera);
static void _generateEpilogue(void);
static void _generateHtmlText(const char * text);
static void _generateJavaScriptString(const char * string);
static void _generateLight(const unsigned int indentationLevel, const SceneLight * light);
static void _generateLights(const unsigned int indentationLevel, const Scene * scene);
static void _generateMesh(const unsigned int indentationLevel, const SceneMesh * mesh);
static void _generateMeshes(const unsigned int indentationLevel, const Scene * scene);
static void _generateNumber(const double number);
static void _generateNumbers(const double * numbers, const unsigned int count);
static void _generatePrologue(const Scene * scene);
static void _generateRuntime(void);
static void _generateScene(const Scene * scene);
static char * _indentation(const unsigned int indentationLevel);
static void _output(const unsigned int indentationLevel, const char * const format, ...);

/**
 * Creates every directory of the path, except for the last segment, which is
 * the name of the file.
 */
static bool _createParentDirectories(const char * path) {
	char * directory = strdup(path);
	bool succeeded = true;
	for (char * separator = strchr(directory + 1, '/'); succeeded && separator != NULL; separator = strchr(separator + 1, '/')) {
		*separator = '\0';
		if (mkdir(directory, 0777) != 0 && errno != EEXIST) {
			logError(_logger, "The directory \"%s\" cannot be created: %s.", directory, strerror(errno));
			succeeded = false;
		}
		*separator = '/';
	}
	free(directory);
	return succeeded;
}

/**
 * Generates the camera of the scene object.
 */
static void _generateCamera(const unsigned int indentationLevel, const SceneCamera * camera) {
	_output(indentationLevel, "%s", "camera: {\n");
	_output(1 + indentationLevel, "%s", "position: ");
	_generateNumbers(camera->position, 3);
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "target: ");
	_generateNumbers(camera->target, 3);
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "fov: ");
	_generateNumber(camera->fov);
	_output(0, "%s", "\n");
	_output(indentationLevel, "%s", "},\n");
}

/**
 * Creates the epilogue of the generated output, that is, the final lines that
 * completes a valid HTML document.
 */
static void _generateEpilogue(void) {
	_output(0, "%s",
		"</body>\n"
		"</html>\n"
	);
}

/**
 * Generates a text with the special characters of HTML escaped.
 */
static void _generateHtmlText(const char * text) {
	for (const char * character = text; *character != '\0'; ++character) {
		switch (*character) {
			case '&':
				_output(0, "%s", "&amp;");
				break;
			case '<':
				_output(0, "%s", "&lt;");
				break;
			case '>':
				_output(0, "%s", "&gt;");
				break;
			case '"':
				_output(0, "%s", "&quot;");
				break;
			case '\'':
				_output(0, "%s", "&#39;");
				break;
			default:
				_output(0, "%c", *character);
				break;
		}
	}
}

/**
 * Generates a JavaScript string literal. The angle brackets are escaped too,
 * because a closing tag would end the script block, even inside a string. The
 * same applies to the line separators U+2028 and U+2029 (in UTF-8), which are
 * not valid inside of a string in some browsers.
 */
static void _generateJavaScriptString(const char * string) {
	_output(0, "%s", "\"");
	for (const unsigned char * character = (const unsigned char *) string; *character != '\0'; ++character) {
		if (*character == '\\' || *character == '"') {
			_output(0, "\\%c", *character);
		}
		else if (*character < 0x20 || *character == 0x7F || *character == '<' || *character == '>') {
			_output(0, "\\u%04x", *character);
		}
		else if (character[0] == 0xE2 && character[1] == 0x80 && (character[2] == 0xA8 || character[2] == 0xA9)) {
			_output(0, "\\u%04x", 0x2000 + character[2] - 0x80);
			character += 2;
		}
		else {
			_output(0, "%c", *character);
		}
	}
	_output(0, "%s", "\"");
}

/**
 * Generates a light of the scene object.
 */
static void _generateLight(const unsigned int indentationLevel, const SceneLight * light) {
	_output(indentationLevel, "%s", "{\n");
	_output(1 + indentationLevel, "%s", "name: ");
	_generateJavaScriptString(light->name);
	_output(0, "%s", ",\n");
	if (light->type == POINT_LIGHT) {
		_output(1 + indentationLevel, "%s", "type: \"point\",\n");
		_output(1 + indentationLevel, "%s", "position: ");
	}
	else {
		_output(1 + indentationLevel, "%s", "type: \"directional\",\n");
		_output(1 + indentationLevel, "%s", "direction: ");
	}
	_generateNumbers(light->vector, 3);
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "color: ");
	_generateNumbers(light->color, 3);
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "intensity: ");
	_generateNumber(light->intensity);
	_output(0, "%s", "\n");
	_output(indentationLevel, "%s", "},\n");
}

/**
 * Generates the list of lights of the scene object.
 */
static void _generateLights(const unsigned int indentationLevel, const Scene * scene) {
	_output(indentationLevel, "%s", "lights: [\n");
	for (unsigned int k = 0; k < scene->lightCount; ++k) {
		_generateLight(1 + indentationLevel, &scene->lights[k]);
	}
	_output(indentationLevel, "%s", "],\n");
}

/**
 * Generates a mesh of the scene object.
 */
static void _generateMesh(const unsigned int indentationLevel, const SceneMesh * mesh) {
	_output(indentationLevel, "%s", "{\n");
	_output(1 + indentationLevel, "%s", "name: ");
	_generateJavaScriptString(mesh->name);
	_output(0, "%s", ",\n");
	switch (mesh->type) {
		case BOX_MESH:
			_output(1 + indentationLevel, "%s", "type: \"box\",\n");
			_output(1 + indentationLevel, "%s", "size: ");
			_generateNumbers(mesh->size, 3);
			break;
		case PLANE_MESH:
			_output(1 + indentationLevel, "%s", "type: \"plane\",\n");
			_output(1 + indentationLevel, "%s", "size: ");
			_generateNumbers(mesh->size, 2);
			break;
		case SPHERE_MESH:
			_output(1 + indentationLevel, "%s", "type: \"sphere\",\n");
			_output(1 + indentationLevel, "%s", "size: ");
			_generateNumbers(mesh->size, 1);
			break;
		default:
			logError(_logger, "The specified mesh type is unknown: %d", mesh->type);
			break;
	}
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "model: ");
	_generateNumbers(mesh->model, 16);
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "color: ");
	_generateNumbers(mesh->color, 4);
	_output(0, "%s", ",\n");
	_output(1 + indentationLevel, "%s", "shininess: ");
	_generateNumber(mesh->shininess);
	_output(0, "%s", "\n");
	_output(indentationLevel, "%s", "},\n");
}

/**
 * Generates the list of meshes of the scene object.
 */
static void _generateMeshes(const unsigned int indentationLevel, const Scene * scene) {
	_output(indentationLevel, "%s", "meshes: [\n");
	for (unsigned int k = 0; k < scene->meshCount; ++k) {
		_generateMesh(1 + indentationLevel, &scene->meshes[k]);
	}
	_output(indentationLevel, "%s", "]\n");
}

/**
 * Generates a number with the shortest representation that keeps its value.
 */
static void _generateNumber(const double number) {
	char text[32];
	for (int precision = 15; precision <= 17; ++precision) {
		snprintf(text, sizeof(text), "%.*g", precision, number);
		if (strtod(text, NULL) == number) {
			break;
		}
	}
	_output(0, "%s", text);
}

/**
 * Generates a JavaScript array of numbers.
 */
static void _generateNumbers(const double * numbers, const unsigned int count) {
	_output(0, "%s", "[");
	for (unsigned int k = 0; k < count; ++k) {
		_output(0, "%s", k == 0 ? "" : ", ");
		_generateNumber(numbers[k]);
	}
	_output(0, "%s", "]");
}

/**
 * Creates the prologue of the generated output, that is, the first lines of
 * the HTML document, up to the canvas.
 */
static void _generatePrologue(const Scene * scene) {
	_output(0, "%s",
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"\t<meta charset=\"utf-8\">\n"
		"\t<title>"
	);
	_generateHtmlText(scene->name);
	_output(0, "%s",
		"</title>\n"
		"\t<style>\n"
		"\t\thtml, body { background: #101418; height: 100%; margin: 0; }\n"
		"\t\tbody { align-items: center; display: flex; justify-content: center; }\n"
		"\t\tcanvas { display: block; height: auto; max-height: 100vh; max-width: 100vw; object-fit: contain; width: auto; }\n"
		"\t</style>\n"
		"</head>\n"
		"<body>\n"
	);
	_output(1, "<canvas id=\"canvas\" width=\"%d\" height=\"%d\"></canvas>\n", scene->width, scene->height);
}

/**
 * Generates the script block with the embedded runtime.
 */
static void _generateRuntime(void) {
	_output(1, "%s", "<script>\n");
	_output(0, "%s", runtime);
	_output(1, "%s", "</script>\n");
}

/**
 * Generates the script block with the scene object, already computed, and the
 * call that renders it.
 */
static void _generateScene(const Scene * scene) {
	_output(1, "%s", "<script>\n");
	_output(2, "%s", "const scene = {\n");
	_output(3, "%s", "name: ");
	_generateJavaScriptString(scene->name);
	_output(0, "%s", ",\n");
	_generateCamera(3, &scene->camera);
	_generateLights(3, scene);
	_generateMeshes(3, scene);
	_output(2, "%s", "};\n");
	_output(2, "%s", "Runtime.render(document.getElementById(\"canvas\"), scene);\n");
	_output(1, "%s", "</script>\n");
}

/**
 * Generates an indentation string for the specified level.
 */
static char * _indentation(const unsigned int level) {
	return indentation(_indentationCharacter, level, _indentationSize);
}

/**
 * Outputs a formatted string to the output file. The "fprintf" errors are
 * sticky, so they are checked once, when the file is closed.
 */
static void _output(const unsigned int indentationLevel, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char * indentation = _indentation(indentationLevel);
	fputs(indentation, _file);
	vfprintf(_file, format, arguments);
	free(indentation);
	va_end(arguments);
}

/** PUBLIC FUNCTIONS */

CompilationStatus executeGenerator(CompilerState * compilerState) {
	logDebugging(_logger, "Generating final output...");
	const Scene * scene = compilerState->scene;
	char * path = concatenate(3, getStringOrDefault("OUTPUT_DIRECTORY", "."), "/", scene->output);
	if (!_createParentDirectories(path)) {
		free(path);
		return FAILED;
	}
	_file = fopen(path, "w");
	if (_file == NULL) {
		logError(_logger, "The output file \"%s\" cannot be opened: %s.", path, strerror(errno));
		free(path);
		return FAILED;
	}
	_generatePrologue(scene);
	_generateRuntime();
	_generateScene(scene);
	_generateEpilogue();
	const bool hasWriteError = ferror(_file) != 0;
	const bool hasCloseError = fclose(_file) != 0;
	_file = NULL;
	if (hasWriteError || hasCloseError) {
		logError(_logger, "The output file \"%s\" cannot be written.", path);
		free(path);
		return FAILED;
	}
	logInformation(_logger, "The scene \"%s\" was generated in \"%s\".", scene->name, path);
	free(path);
	logDebugging(_logger, "Generation is done.");
	return SUCCEEDED;
}
