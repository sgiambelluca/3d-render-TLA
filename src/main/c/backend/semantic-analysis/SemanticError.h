#ifndef SEMANTIC_ERROR_HEADER
#define SEMANTIC_ERROR_HEADER

#include "../../support/logging/Logger.h"
#include <stdarg.h>
#include <stdio.h>

/**
 * The attribute being analyzed, that gives context to the reported errors.
 */
typedef struct {
	const char * entity;
	const char * attribute;
} Context;

/**
 * Starts a new count of errors, which are reported with the given logger.
 */
void initializeSemanticErrors(Logger * logger);

/**
 * Reports a semantic error of an entity's attribute. The analysis continues
 * after this, to report as many errors as possible.
 */
void reportSemanticError(const Context * context, const char * const format, ...);

/**
 * Reports an error that doesn't belong to an attribute (e.g., a repeated
 * identifier), with the message as it is.
 */
void reportError(const char * const format, ...);

/**
 * The amount of errors reported since the last initialization.
 */
unsigned int semanticErrorCount(void);

#endif
