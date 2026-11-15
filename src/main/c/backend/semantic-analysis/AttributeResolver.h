#ifndef ATTRIBUTE_RESOLVER_HEADER
#define ATTRIBUTE_RESOLVER_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "SemanticError.h"
#include "SymbolTable.h"
#include "Value.h"
#include <stdbool.h>
#include <string.h>

/**
 * Evaluates the expressions of the AST and resolves the attributes of the
 * symbols. Both depend on each other: a reference (e.g., "mesa.size.y")
 * resolves the attribute of another symbol, and resolving an attribute
 * evaluates its expression.
 */

/**
 * The symbol table where the references are looked up. It must be set before
 * evaluating any expression.
 */
void setResolverSymbolTable(const SymbolTable * symbolTable);

/**
 * Evaluates an expression and synthesizes its type, reporting the errors in
 * the given context.
 */
bool evaluateExpression(const Context * context, const Expression * expression, Value * result);

/**
 * Resolves an attribute lazily, with memoization: its expression is evaluated
 * and validated only the first time. Finding an attribute that is being
 * resolved means that it depends on itself (a cyclic dependency).
 */
bool resolveAttribute(const Context * context, Symbol * symbol, const AttributeType attribute, Value * result);

#endif
