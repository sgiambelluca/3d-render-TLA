#include "SemanticError.h"

#define ERROR_MESSAGE_SIZE 512

/* MODULE INTERNAL STATE */

static unsigned int _errorCount = 0;
static Logger * _logger = NULL;

/* PUBLIC FUNCTIONS */

void initializeSemanticErrors(Logger * logger) {
	_errorCount = 0;
	_logger = logger;
}

void reportSemanticError(const Context * context, const char * const format, ...) {
	char message[ERROR_MESSAGE_SIZE];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, ERROR_MESSAGE_SIZE, format, arguments);
	va_end(arguments);
	logError(_logger, "Semantic error in \"%s.%s\": %s.", context->entity, context->attribute, message);
	++_errorCount;
}

void reportError(const char * const format, ...) {
	char message[ERROR_MESSAGE_SIZE];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, ERROR_MESSAGE_SIZE, format, arguments);
	va_end(arguments);
	logError(_logger, "%s", message);
	++_errorCount;
}

unsigned int semanticErrorCount(void) {
	return _errorCount;
}
