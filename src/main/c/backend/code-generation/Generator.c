#include "Generator.h"

/* MODULE INTERNAL STATE */

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

/** PUBLIC FUNCTIONS */

/**
 * @todo Stage III: resolve the scene (semantic analysis) and generate the
 *	final HTML document with the embedded software rasterizer.
 */
void executeGenerator(CompilerState * compilerState) {
	logDebugging(_logger, "Generating final output...");
	Program * program = compilerState->abstractSyntaxtTree;
	logInformation(_logger, "The scene \"%s\" has %d entities. The code generation is not implemented yet.",
		program->name,
		program->entities->size);
	logDebugging(_logger, "Generation is done.");
}
