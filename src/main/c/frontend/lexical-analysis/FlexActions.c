#include "FlexActions.h"

/* MODULE INTERNAL STATE */

static bool _logIgnoredLexemes = true;
static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownFlexActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: FlexActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_lexicalAnalyzer = NULL;
}

ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer) {
	_lexicalAnalyzer = lexicalAnalyzer;
	_logger = createLogger("FlexActions");
	_logIgnoredLexemes = getBooleanOrDefault("LOG_IGNORED_LEXEMES", _logIgnoredLexemes);
	return _shutdownFlexActionsModule;
}

/* PRIVATE FUNCTIONS */

static CompilationStatus _lexicalError(Token * token, const char * reason);
static void _logTokenAction(const char * actionName, Token * token);
static unsigned char _parseHexadecimalByte(const char * digits);
static CompilationStatus _pushLabelOnlyToken(const char * actionName, TokenLabel label);
static CompilationStatus _pushAndDestroyToken(const char * actionName, Token * token);

/**
 * Logs a lexical error related to the specified token, destroys it, and
 * aborts the compilation. The token is pushed to the parser (it must be
 * labeled as UNKNOWN), because the grammar never accepts it, so Bison
 * discards its stack and releases the partial AST with its destructors.
 */
static CompilationStatus _lexicalError(Token * token, const char * reason) {
	char * _lexeme = escape(token->lexeme);
	logError(_logger, "Lexical error at line %d: %s (lexeme=\"%s\").", token->line, reason, _lexeme);
	free(_lexeme);
	token->label = UNKNOWN;
	pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return FAILED;
}

/**
 * Logs a lexical-analyzer action over a token in DEBUGGING level.
 */
static void _logTokenAction(const char * actionName, Token * token) {
	char * _lexeme = escape(token->lexeme);
	logDebugging(_logger, WARNING_COLOR "%s" DEFAULT_COLOR ": Token(context=%d, label=%d, length=%d, lexeme=%s\"%s\"%s, line=%d, semanticValue=%p)",
		actionName,
		token->context,
		token->label,
		token->length,
		INFORMATION_COLOR, _lexeme, DEFAULT_COLOR,
		token->line,
		token->semanticValue);
	free(_lexeme);
	_lexeme = NULL;
}

/**
 * Converts 2 hexadecimal digits into its byte value.
 */
static unsigned char _parseHexadecimalByte(const char * digits) {
	char byte[3] = { digits[0], digits[1], '\0' };
	return (unsigned char) strtoul(byte, NULL, 16);
}

/**
 * Pushes a token that doesn't carry any semantic value (e.g., a keyword or a
 * punctuation symbol).
 */
static CompilationStatus _pushLabelOnlyToken(const char * actionName, TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	token->semanticValue->token = label;
	return _pushAndDestroyToken(actionName, token);
}

/**
 * Logs, pushes and destroys the token. The semantic value is copied by the
 * parser, so any heap-memory referenced by it is owned by the parser.
 */
static CompilationStatus _pushAndDestroyToken(const char * actionName, Token * token) {
	_logTokenAction(actionName, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

/* PUBLIC FUNCTIONS */

CompilationStatus AngleLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, ANGLE);
	// "strtod" stops at the "deg" suffix.
	token->semanticValue->decimal = strtod(token->lexeme, NULL);
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label) {
	return _pushLabelOnlyToken(__FUNCTION__, label);
}

CompilationStatus BraceLexemeAction(TokenLabel label) {
	return _pushLabelOnlyToken(__FUNCTION__, label);
}

CompilationStatus ColorLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, HEX_COLOR);
	const char * digits = token->lexeme + 1;
	const bool hasAlpha = token->length == 9;
	token->semanticValue->color.red = _parseHexadecimalByte(digits);
	token->semanticValue->color.green = _parseHexadecimalByte(digits + 2);
	token->semanticValue->color.blue = _parseHexadecimalByte(digits + 4);
	token->semanticValue->color.alpha = hasAlpha ? _parseHexadecimalByte(digits + 6) : 0xFF;
	token->semanticValue->color.hasAlpha = hasAlpha;
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus DecimalLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, DECIMAL);
	token->semanticValue->decimal = strtod(token->lexeme, NULL);
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus EOFLexemeAction() {
	CompilationStatus status = IN_PROGRESS;
	Token * token = createToken(_lexicalAnalyzer, 0);
	_logTokenAction(__FUNCTION__, token);
	if (!popInputBuffer(_lexicalAnalyzer)) {
		status = pushToken(_lexicalAnalyzer, token);
		FlexContext context = currentLexicalAnalyzerContext(_lexicalAnalyzer);
		if (0 < context) {
			logError(_logger, "The final context is not closed (context=%d).", context);
			status = FAILED;
		}
	}
	destroyToken(token);
	return status;
}

CompilationStatus IdentifierLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, IDENTIFIER);
	token->semanticValue->string = strdup(token->lexeme);
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus IgnoredLexemeAction() {
	if (_logIgnoredLexemes) {
		Token * token = createToken(_lexicalAnalyzer, IGNORED);
		_logTokenAction(__FUNCTION__, token);
		destroyToken(token);
	}
	return IN_PROGRESS;
}

CompilationStatus IntegerLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, INTEGER);
	errno = 0;
	const long value = strtol(token->lexeme, NULL, 10);
	if (errno == ERANGE || INT_MAX < value) {
		return _lexicalError(token, "the integer is too big");
	}
	token->semanticValue->integer = (int) value;
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus InvalidColorLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	return _lexicalError(token, "a color must have exactly 6 (#rrggbb) or 8 (#rrggbbaa) hexadecimal digits");
}

CompilationStatus InvalidNumberLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	return _lexicalError(token, "a number can only be followed by the \"deg\" suffix");
}

CompilationStatus KeywordLexemeAction(TokenLabel label) {
	return _pushLabelOnlyToken(__FUNCTION__, label);
}

CompilationStatus MemberLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, MEMBER);
	// Skips the leading dot (e.g., ".pos" is the member "pos").
	token->semanticValue->string = strdup(token->lexeme + 1);
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus ParenthesisLexemeAction(TokenLabel label) {
	return _pushLabelOnlyToken(__FUNCTION__, label);
}

CompilationStatus PunctuationLexemeAction(TokenLabel label) {
	return _pushLabelOnlyToken(__FUNCTION__, label);
}

CompilationStatus StringLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, STRING);
	// Removes the surrounding double-quotes.
	token->semanticValue->string = strndup(token->lexeme + 1, token->length - 2);
	return _pushAndDestroyToken(__FUNCTION__, token);
}

CompilationStatus UnknownLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	_logTokenAction(__FUNCTION__, token);
	return _lexicalError(token, "unknown lexeme");
}
