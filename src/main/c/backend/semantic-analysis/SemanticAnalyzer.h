#ifndef SEMANTIC_ANALYZER_HEADER
#define SEMANTIC_ANALYZER_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "AttributeResolver.h"
#include "Matrix.h"
#include "Scene.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "Value.h"
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Initialize module's internal state. */
ModuleDestructor initializeSemanticAnalyzerModule();

/**
 * Validates the AST and resolves every value of the scene, reporting all the
 * errors found. If it succeeds, the resolved scene is left in the compiler
 * state.
 */
CompilationStatus executeSemanticAnalysis(CompilerState * compilerState);

#endif
