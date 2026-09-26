#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule();

/**
 * Reports a syntactic error detected by Bison.
 */
void SyntacticErrorAction(const char * message);

/**
 * Bison semantic actions.
 */

Constant * AngleConstantSemanticAction(const double degrees);
Constant * ColorConstantSemanticAction(const Color color);
Constant * DecimalConstantSemanticAction(const double value);
Constant * IntegerConstantSemanticAction(const int value);

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type);
Expression * FactorExpressionSemanticAction(Factor * factor);
Expression * NegationExpressionSemanticAction(Expression * operand);

Factor * ConstantFactorSemanticAction(Constant * constant);
Factor * ExpressionFactorSemanticAction(Expression * expression);
Factor * ReferenceFactorSemanticAction(Reference * reference);
Factor * VectorFactorSemanticAction(Vector * vector);

Reference * MemberReferenceSemanticAction(Reference * reference, char * member);
Reference * ReferenceSemanticAction(char * identifier);
Vector * VectorSemanticAction(Expression * x, Expression * y, Expression * z);

Property * PropertySemanticAction(PropertyType type, Expression * value);
PropertyList * AppendPropertySemanticAction(PropertyList * propertyList, Property * property);
PropertyList * EmptyPropertyListSemanticAction();

Camera * CameraSemanticAction(char * identifier, Expression * position, PropertyList * properties);
Light * LightSemanticAction(char * identifier, LightType type, Expression * position, PropertyList * properties);
Material * MaterialSemanticAction(char * identifier, PropertyList * properties);
Mesh * MeshSemanticAction(char * identifier, MeshType type, Expression * position, PropertyList * properties);
Render * RenderSemanticAction(Expression * resolution, char * output);

Entity * CameraEntitySemanticAction(Camera * camera);
Entity * LightEntitySemanticAction(Light * light);
Entity * MaterialEntitySemanticAction(Material * material);
Entity * MeshEntitySemanticAction(Mesh * mesh);
Entity * RenderEntitySemanticAction(Render * render);
EntityList * AppendEntitySemanticAction(EntityList * entityList, Entity * entity);
EntityList * EmptyEntityListSemanticAction();

Program * SceneProgramSemanticAction(char * name, EntityList * entities);

#endif
