#ifndef GENERATOR_HEADER
#define GENERATOR_HEADER

#include "../../support/configuration/Environment.h"
#include "../../support/language/String.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../semantic-analysis/SemanticAnalyzer.h"
#include "Runtime.h"
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/** Initialize module's internal state. */
ModuleDestructor initializeGeneratorModule();

/**
 * Generates the final HTML document using the resolved scene of the current
 * compiler state.
 */
CompilationStatus executeGenerator(CompilerState * compilerState);

#endif
