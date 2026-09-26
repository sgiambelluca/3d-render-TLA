%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison parser.
 *
 * @todo Add location to the grammar and "pushToken" API function.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Tracking-Locations.html
 */
void yyerror(const YYLTYPE * location, const char * message) {
	SyntacticErrorAction(message);
}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */

	Color color;
	double decimal;
	signed int integer;
	char * string;
	TokenLabel token;

	/** Non-terminals. */

	Camera * camera;
	Constant * constant;
	Entity * entity;
	EntityList * entityList;
	Expression * expression;
	Factor * factor;
	Light * light;
	LightType lightType;
	Material * material;
	Mesh * mesh;
	MeshType meshType;
	Program * program;
	Property * property;
	PropertyList * propertyList;
	Reference * reference;
	Render * render;
	Vector * vector;
}

/**
 * Destructors. This functions are executed after the parsing ends, so if the
 * AST must be used in the following phases of the compiler you shouldn't used
 * this approach for the AST root node ("program" non-terminal, in this
 * grammar), or it will drop the entire tree even if the parsing succeeds.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { free($$); } <string>
%destructor { destroyCamera($$); } <camera>
%destructor { destroyConstant($$); } <constant>
%destructor { destroyEntity($$); } <entity>
%destructor { destroyEntityList($$); } <entityList>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyFactor($$); } <factor>
%destructor { destroyLight($$); } <light>
%destructor { destroyMaterial($$); } <material>
%destructor { destroyMesh($$); } <mesh>
%destructor { destroyProperty($$); } <property>
%destructor { destroyPropertyList($$); } <propertyList>
%destructor { destroyReference($$); } <reference>
%destructor { destroyRender($$); } <render>
%destructor { destroyVector($$); } <vector>

/** Terminals: literals and identifiers. */
%token <decimal> ANGLE
%token <decimal> DECIMAL
%token <color> HEX_COLOR
%token <string> IDENTIFIER
%token <integer> INTEGER
%token <string> MEMBER
%token <string> STRING

/** Terminals: entities. */
%token <token> CAMERA
%token <token> LIGHT
%token <token> MATERIAL
%token <token> MESH
%token <token> RENDER
%token <token> SCENE

/** Terminals: subtypes. */
%token <token> BOX
%token <token> DIRECTIONAL
%token <token> PLANE
%token <token> POINT
%token <token> SPHERE

/** Terminals: attributes. */
%token <token> COLOR
%token <token> DIRECTION
%token <token> FOV
%token <token> INTENSITY
%token <token> LOOK_AT
%token <token> RADIUS
%token <token> ROTATE
%token <token> SCALE
%token <token> SHININESS
%token <token> SIZE
%token <token> TRANSLATE

/** Terminals: other keywords. */
%token <token> AT
%token <token> TO

/** Terminals: operators and punctuation. */
%token <token> ADD
%token <token> CLOSE_BRACE
%token <token> CLOSE_PARENTHESIS
%token <token> COLON
%token <token> COMMA
%token <token> DIV
%token <token> MUL
%token <token> OPEN_BRACE
%token <token> OPEN_PARENTHESIS
%token <token> SUB

%token <token> IGNORED
%token <token> UNKNOWN "invalid lexeme"

/** Non-terminals. */
%type <camera> camera
%type <constant> constant
%type <entity> entity
%type <entityList> entities
%type <expression> expression
%type <expression> position
%type <factor> factor
%type <light> light
%type <lightType> light_type
%type <material> material
%type <mesh> mesh
%type <meshType> mesh_type
%type <program> program
%type <property> camera_property
%type <property> light_property
%type <property> material_property
%type <property> mesh_property
%type <propertyList> camera_properties
%type <propertyList> light_properties
%type <propertyList> material_properties
%type <propertyList> mesh_properties
%type <reference> reference
%type <render> render
%type <vector> vector

/**
 * Precedence and associativity.
 *
 * @see https://en.cppreference.com/w/cpp/language/operator_precedence.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%left ADD SUB
%left MUL DIV
%precedence UNARY

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

program: SCENE STRING[name] OPEN_BRACE entities CLOSE_BRACE						{ $$ = SceneProgramSemanticAction($name, $entities); }
	;

/** Entities (in any order, the list keeps the order of declaration). */

entities: %empty																{ $$ = EmptyEntityListSemanticAction(); }
	| entities[list] entity														{ $$ = AppendEntitySemanticAction($list, $entity); }
	;

entity: camera																	{ $$ = CameraEntitySemanticAction($1); }
	| light																		{ $$ = LightEntitySemanticAction($1); }
	| material																	{ $$ = MaterialEntitySemanticAction($1); }
	| mesh																		{ $$ = MeshEntitySemanticAction($1); }
	| render																	{ $$ = RenderEntitySemanticAction($1); }
	;

camera: CAMERA IDENTIFIER AT expression OPEN_BRACE camera_properties CLOSE_BRACE	{ $$ = CameraSemanticAction($IDENTIFIER, $expression, $camera_properties); }
	;

light: LIGHT IDENTIFIER COLON light_type position
		OPEN_BRACE light_properties CLOSE_BRACE									{ $$ = LightSemanticAction($IDENTIFIER, $light_type, $position, $light_properties); }
	;

light_type: POINT																{ $$ = POINT_LIGHT; }
	| DIRECTIONAL																{ $$ = DIRECTIONAL_LIGHT; }
	;

material: MATERIAL IDENTIFIER OPEN_BRACE material_properties CLOSE_BRACE			{ $$ = MaterialSemanticAction($IDENTIFIER, $material_properties); }
	;

mesh: MESH IDENTIFIER COLON mesh_type position
		OPEN_BRACE mesh_properties CLOSE_BRACE									{ $$ = MeshSemanticAction($IDENTIFIER, $mesh_type, $position, $mesh_properties); }
	;

mesh_type: BOX																	{ $$ = BOX_MESH; }
	| PLANE																		{ $$ = PLANE_MESH; }
	| SPHERE																	{ $$ = SPHERE_MESH; }
	;

render: RENDER expression[resolution] TO STRING[output]							{ $$ = RenderSemanticAction($resolution, $output); }
	;

position: %empty																{ $$ = NULL; }
	| AT expression																{ $$ = $expression; }
	;

/** Attributes of each entity (in any order, the list keeps the order). */

camera_properties: %empty														{ $$ = EmptyPropertyListSemanticAction(); }
	| camera_properties[list] camera_property									{ $$ = AppendPropertySemanticAction($list, $camera_property); }
	;

camera_property: LOOK_AT expression												{ $$ = PropertySemanticAction(LOOK_AT_PROPERTY, $expression); }
	| FOV expression															{ $$ = PropertySemanticAction(FOV_PROPERTY, $expression); }
	;

light_properties: %empty														{ $$ = EmptyPropertyListSemanticAction(); }
	| light_properties[list] light_property										{ $$ = AppendPropertySemanticAction($list, $light_property); }
	;

light_property: COLOR expression												{ $$ = PropertySemanticAction(COLOR_PROPERTY, $expression); }
	| DIRECTION expression														{ $$ = PropertySemanticAction(DIRECTION_PROPERTY, $expression); }
	| INTENSITY expression														{ $$ = PropertySemanticAction(INTENSITY_PROPERTY, $expression); }
	;

material_properties: %empty														{ $$ = EmptyPropertyListSemanticAction(); }
	| material_properties[list] material_property								{ $$ = AppendPropertySemanticAction($list, $material_property); }
	;

material_property: COLOR expression												{ $$ = PropertySemanticAction(COLOR_PROPERTY, $expression); }
	| SHININESS expression														{ $$ = PropertySemanticAction(SHININESS_PROPERTY, $expression); }
	;

mesh_properties: %empty															{ $$ = EmptyPropertyListSemanticAction(); }
	| mesh_properties[list] mesh_property										{ $$ = AppendPropertySemanticAction($list, $mesh_property); }
	;

mesh_property: MATERIAL expression												{ $$ = PropertySemanticAction(MATERIAL_PROPERTY, $expression); }
	| RADIUS expression															{ $$ = PropertySemanticAction(RADIUS_PROPERTY, $expression); }
	| ROTATE expression															{ $$ = PropertySemanticAction(ROTATE_PROPERTY, $expression); }
	| SCALE expression															{ $$ = PropertySemanticAction(SCALE_PROPERTY, $expression); }
	| SIZE expression															{ $$ = PropertySemanticAction(SIZE_PROPERTY, $expression); }
	| TRANSLATE expression														{ $$ = PropertySemanticAction(TRANSLATE_PROPERTY, $expression); }
	;

/** Arithmetic expressions over scalars, angles, colors, vectors and references. */

expression: expression[left] ADD expression[right]								{ $$ = ArithmeticExpressionSemanticAction($left, $right, ADDITION); }
	| expression[left] DIV expression[right]									{ $$ = ArithmeticExpressionSemanticAction($left, $right, DIVISION); }
	| expression[left] MUL expression[right]									{ $$ = ArithmeticExpressionSemanticAction($left, $right, MULTIPLICATION); }
	| expression[left] SUB expression[right]									{ $$ = ArithmeticExpressionSemanticAction($left, $right, SUBTRACTION); }
	| SUB expression[operand] %prec UNARY										{ $$ = NegationExpressionSemanticAction($operand); }
	| factor																	{ $$ = FactorExpressionSemanticAction($1); }
	;

factor: OPEN_PARENTHESIS expression CLOSE_PARENTHESIS							{ $$ = ExpressionFactorSemanticAction($expression); }
	| constant																	{ $$ = ConstantFactorSemanticAction($1); }
	| reference																	{ $$ = ReferenceFactorSemanticAction($1); }
	| vector																	{ $$ = VectorFactorSemanticAction($1); }
	;

vector: OPEN_PARENTHESIS expression[x] COMMA expression[y] CLOSE_PARENTHESIS	{ $$ = VectorSemanticAction($x, $y, NULL); }
	| OPEN_PARENTHESIS expression[x] COMMA expression[y] COMMA expression[z]
		CLOSE_PARENTHESIS														{ $$ = VectorSemanticAction($x, $y, $z); }
	;

reference: IDENTIFIER															{ $$ = ReferenceSemanticAction($IDENTIFIER); }
	| reference[base] MEMBER													{ $$ = MemberReferenceSemanticAction($base, $MEMBER); }
	;

constant: ANGLE																	{ $$ = AngleConstantSemanticAction($1); }
	| DECIMAL																	{ $$ = DecimalConstantSemanticAction($1); }
	| HEX_COLOR																	{ $$ = ColorConstantSemanticAction($1); }
	| INTEGER																	{ $$ = IntegerConstantSemanticAction($1); }
	;

%%
