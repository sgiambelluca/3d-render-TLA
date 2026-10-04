%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus AngleLexemeAction();
extern CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label);
extern CompilationStatus BraceLexemeAction(TokenLabel label);
extern CompilationStatus ColorLexemeAction();
extern CompilationStatus DecimalLexemeAction();
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus InvalidColorLexemeAction();
extern CompilationStatus InvalidNumberLexemeAction();
extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus MemberLexemeAction();
extern CompilationStatus ParenthesisLexemeAction(TokenLabel label);
extern CompilationStatus PunctuationLexemeAction(TokenLabel label);
extern CompilationStatus StringLexemeAction();
extern CompilationStatus UnknownLexemeAction();

%}

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
comment				"//"[^\n]*
decimal				[[:digit:]]+"."[[:digit:]]+
hexadecimal			[[:xdigit:]]
identifier			[[:alpha:]_][[:alnum:]_]*
integer				[[:digit:]]+
whitespace			[[:space:]]+

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

{comment}							{ return IgnoredLexemeAction(); }

"scene"								{ return KeywordLexemeAction(SCENE); }
"camera"							{ return KeywordLexemeAction(CAMERA); }
"light"								{ return KeywordLexemeAction(LIGHT); }
"material"							{ return KeywordLexemeAction(MATERIAL); }
"mesh"								{ return KeywordLexemeAction(MESH); }
"render"							{ return KeywordLexemeAction(RENDER); }

"at"								{ return KeywordLexemeAction(AT); }
"to"								{ return KeywordLexemeAction(TO); }

"point"								{ return KeywordLexemeAction(POINT); }
"directional"						{ return KeywordLexemeAction(DIRECTIONAL); }
"sphere"							{ return KeywordLexemeAction(SPHERE); }
"box"								{ return KeywordLexemeAction(BOX); }
"plane"								{ return KeywordLexemeAction(PLANE); }

"look_at"							{ return KeywordLexemeAction(LOOK_AT); }
"fov"								{ return KeywordLexemeAction(FOV); }
"color"								{ return KeywordLexemeAction(COLOR); }
"intensity"							{ return KeywordLexemeAction(INTENSITY); }
"direction"							{ return KeywordLexemeAction(DIRECTION); }
"shininess"							{ return KeywordLexemeAction(SHININESS); }
"radius"							{ return KeywordLexemeAction(RADIUS); }
"size"								{ return KeywordLexemeAction(SIZE); }
"translate"							{ return KeywordLexemeAction(TRANSLATE); }
"rotate"							{ return KeywordLexemeAction(ROTATE); }
"scale"								{ return KeywordLexemeAction(SCALE); }

"{"									{ return BraceLexemeAction(OPEN_BRACE); }
"}"									{ return BraceLexemeAction(CLOSE_BRACE); }
"("									{ return ParenthesisLexemeAction(OPEN_PARENTHESIS); }
")"									{ return ParenthesisLexemeAction(CLOSE_PARENTHESIS); }
","									{ return PunctuationLexemeAction(COMMA); }
":"									{ return PunctuationLexemeAction(COLON); }

"-"									{ return ArithmeticOperatorLexemeAction(SUB); }
"*"									{ return ArithmeticOperatorLexemeAction(MUL); }
"/"									{ return ArithmeticOperatorLexemeAction(DIV); }
"+"									{ return ArithmeticOperatorLexemeAction(ADD); }

"#"{hexadecimal}{6}					{ return ColorLexemeAction(); }
"#"{hexadecimal}{8}					{ return ColorLexemeAction(); }
"#"[[:alnum:]]*						{ return InvalidColorLexemeAction(); }

({integer}|{decimal})"deg"			{ return AngleLexemeAction(); }
{decimal}							{ return DecimalLexemeAction(); }
{integer}							{ return IntegerLexemeAction(); }
({integer}|{decimal}){identifier}	{ return InvalidNumberLexemeAction(); }

\"[^"\n]*\"							{ return StringLexemeAction(); }

{identifier}						{ return IdentifierLexemeAction(); }
"."{identifier}						{ return MemberLexemeAction(); }

{whitespace}						{ return IgnoredLexemeAction(); }
.									{ return UnknownLexemeAction(); }

<<EOF>>								{ return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
