#ifndef SPACE_MODULES_H_
#define SPACE_MODULES_H_

#include "../headers/Token.h"
#include "../headers/stack.h"

// 1 = true; 0 = false
#define LEXER_DEBUG_MODE 1
#define LEXER_DISPLAY_USED_TIME 1

#define PARSER_DEBUG_MODE 0
#define PARSER_DISPLAY_GRAMMAR_PROCESSING 0
#define PARSER_DISPLAY_USED_TIME 1

#define GRAMMAR_LEXER_DISPLAY_GRAMMAR_PROCESSING 0
#define GRAMMAR_LEXER_DISPLAY_USED_TIME 1

//////////////////////////////////////
//////////     FUNCTIONS     /////////
//////////////////////////////////////

int check_for_operator(char input);
int is_space(char character);
int is_digit(char character);

// Lexing the input
int FREE_TOKEN_LENGTHS(int *arrayOfIndividualTokenSizes);
void Tokenize(char **buffer, int **arrayOfIndividualTokenSizes, const size_t *fileLength, const size_t requiredTokenLength);

// Parse
int Generate_Parsetree(TOKEN **tokens, size_t TokenLength);

void check(TOKEN **tokens, size_t tokenArrayLength);

//////////////////////////////////////
//////////   SYNTAX REPORT   /////////
//////////////////////////////////////
typedef enum SyntaxErrorType {
    _NONE_ = 0,              _NOT_AN_IDENTIFIER_,     _NOT_A_FLOAT_,             _NOT_AN_ATOM_,
    _NOT_A_REFERENCE_,       _NOT_A_POINTER_,         _NOT_A_PARAMETER_,         _NOT_A_POINTER_POINTING_ON_VALUE,
    _NOT_A_FUNCTION_CALL_,   _NOT_A_FUNCTION_,        _NOT_A_BREAK_,             _NOT_AN_ENUMERATOR_,
    _NOT_AN_ENUM_,           _NOT_AN_INCLUDE_,        _NOT_A_CATCH_,             _NOT_A_TRY_,
    _NOT_A_SIMPLE_TERM_,     _NOT_A_TERM_,            _NOT_AN_ASSIGNMENT_,       _NOT_A_CLASS_,
    _NOT_A_WITH_STATEMENT_,  _NOT_A_CHECK_STATEMENT_, _NOT_AN_IS_STATEMENT_,     _NOT_AN_EXPORT_,
    _NOT_AN_EXPRESSION_,     _NOT_AN_ARRAY_ELEMENT_,  _NOT_A_VARIABLE_,          _NOT_A_FUNCTION_PARAMETER_INITIALIZER_,
    _NOT_AN_ARRAY_VAR_,      _NOT_A_NORMAL_VAR_,      _NOT_A_CONDITION_,         _NOT_A_VAR_BLOCK_ASSIGNMENT_,
    _NOT_A_CLASS_INSTANCE_,  _NOT_A_WHILE_CONDITION_, _NOT_A_CHAINED_CONDITION_, _NOT_A_PARAMETERED_VAR_,
    _NOT_A_WHILE_STATEMENT_, _NOT_A_DO_STATEMENT_,    _NOT_AN_ELSE_STATEMENT_,   _NOT_A_CONDITIONAL_ASSIGNMENT_,
    _NOT_AN_IF_STATEMENT_,   _NOT_AN_IF_,             _NOT_A_FOR_STATEMENT_,    _NOT_AN_ELSE_IF_STATEMENT_
} SyntaxErrorType;

typedef struct SyntaxReport {
    TOKEN *token;
    SyntaxErrorType errorType;
    int tokensToSkip;
} SyntaxReport;

#endif  // SPACE_MODULES_H_
