/////////////////////////////////////////////////////////////
///////////////////////    LICENSE    ///////////////////////
/////////////////////////////////////////////////////////////
/*
The SPACE-Language compiler compiles an input file into a runnable program.
Copyright (C) 2024  Lukas Nian En Lampl

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "../headers/modules.h"
#include "../headers/errors.h"
#include "../headers/parsetree.h"
#include "../headers/Token.h"

/** 
 * <p>
 * The subprogram {@code SPACE.src.parsetreeGenerator} was created
 * to provide the parsetree generator for the SPACE Language. The
 * procedure is equal to the procedure used in the
 * {@code SPACE.src.syntaxAnalyzer}.
 * </p>
 * 
 * @see SPACE.src.parseTreeGenerator.md
 * 
 * @version 1.01    03.07.2024
 * @author Lukas Nian En Lampl
*/

/**
 * <p>
 * Defines the used booleans, 1 for true and 0 for false.
 * </p>
*/
#define true 1
#define false 0

////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////     PARSE TREE GENERATOR     ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

const int UNINITIALZED = -1;

/**
 * <p>
 * Defines a NodeReport, the basic unit of the parsetree generator.
 * A NodeReport contains the top node (referred as the `.node`)
 * and how many tokens got processed and thus can be skipped.
 * </p>
*/
typedef struct NodeReport {
	Node *node;
	size_t tokensToSkip;
} NodeReport;

/**
 * <p>
 * The VAR_TYPE enum provides all variable types that can be processed
 * by the parsetree generator. Before processing the variable type is
 * searched in {@code #get_var_type()} then based on the enumerator
 * the individual process is invoked.
 * </p>
 * 
 * <p>
 * Here is an overview to the different types (examples):
 * </p>
 * <ul>
 * <li><b>UDEF</b> => No type could be identified
 * <li><b>NORMAL_VAR</b> => `var a = 10;`
 * <li><b>ARRAY_VAR</b> => `var arr[];` or `var arr[] = {1, 2, 3}`
 * <li><b>COND_VAR</b> => `var a = b <= 2 ? 1 : 2;`
 * <li><b>INSTANCE_VAR</b> => `var obj = new Object();`
 * </ul>
*/
enum VAR_TYPE {
	UNDEF,
	NORMAL_VAR,
	ARRAY_VAR,
	COND_VAR,
	INSTANCE_VAR
};

/**
 * <p>
 * Enum for identifying the runnable type.
 * </p>
 * <ul>
 * <li>InBlock is used, if the runnable is in a block statment.
 * <li>SwitchStatement, when the runnable is in a switch statment.
 * <li>IsStatement, when the runnable is in a is statement.
 * </ul>
*/
enum RUNNABLE_TYPE {
	Main,
	InBlock,
	CheckStatement,
	IsStatement
};

enum CONDITION_TYPE {
	IGNORE_ALL,
	NONE
};

/**
 * <p>
 * This enum defines the different process directions, mainly used in the
 * member access tree creation process.
 * </p>
 * 
 * @see #PG_create_member_access_tree(TOKEN **tokens, size_t startPos, int useOptionalTyping);
 */
enum processDirection {
	LEFT,
	STAY,
	RIGHT
};

///// FUNCTIONS PROTOTYPES /////

void PG_print_cpu_time(float cpu_time_used);
NodeReport PG_create_runnable_tree(TOKEN **tokens, size_t startPos, enum RUNNABLE_TYPE type);
NodeReport PG_get_report_based_on_token(TOKEN **tokens, size_t startPos, enum RUNNABLE_TYPE type);
int PG_predict_function_call(TOKEN **tokens, size_t startPos);
NodeReport PG_create_else_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_else_if_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_if_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_for_statement_tree(TOKEN **tokens, size_t startPos);
int PG_predict_assignment(TOKEN **tokens, size_t startPos);
NodeReport PG_create_increment_decrement_tree(TOKEN **tokens, size_t startPos);
int PG_predict_increment_or_decrement_assignment(TOKEN **tokens, size_t startPos);
int PG_get_term_bounds(TOKEN **tokens, size_t startPos);
enum NodeType PG_get_nodeType_of_operator(TOKENTYPES type);
NodeReport PG_create_simple_assignment_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_is_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_check_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_abort_operation_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_return_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_do_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_while_statement_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_variable_tree(TOKEN **tokens, size_t startPos);
enum VAR_TYPE get_var_type(TOKEN **tokens, size_t startPos);
NodeReport PG_create_varType_tree(TOKEN *typeToken);
NodeReport PG_create_class_instance_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_instance_var_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_condition_assignment_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_conditional_var_tree(TOKEN **tokens, size_t startPos);
int PG_get_cond_assignment_bounds(TOKEN **tokens, size_t startPos);
int PG_is_calculation_operator(TOKEN *token);
NodeReport PG_create_array_var_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_array_creation_tree(TOKEN **tokens, size_t startPos);
int PG_predict_array_creation_dimension_count(TOKEN **tokens, size_t startPos);
NodeReport PG_create_array_init_tree(TOKEN **tokens, size_t startPos, int dim);
int PG_predict_array_init_count(TOKEN **tokens, size_t startPos);
int PG_get_array_element_size(TOKEN **tokens, size_t startPos);
int PG_add_dimensions_to_var_node(Node *node, TOKEN **tokens, size_t startPos, int offset);
int PG_get_dimension_count(TOKEN **tokens, size_t startPos);
NodeReport PG_create_normal_var_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_condition_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_conditional_assignment_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_chained_condition_tree(TOKEN **tokens, const size_t startPos, int inDepth);
int PG_is_logic_operator_bracket(TOKEN **tokens, size_t startPos);
int PG_contains_logical_operator(TOKEN **tokens, size_t startPos);
int PG_get_condition_iden_length(TOKEN **tokens, size_t startPos);
int PG_is_condition_operator(TOKENTYPES type);
NodeReport PG_create_class_constructor_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_class_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_try_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_catch_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_export_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_include_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_enum_tree(TOKEN **tokens, size_t startPos);
int PG_predict_enumerator_count(TOKEN **tokens, size_t starPos);
NodeReport PG_create_function_tree(TOKEN **tokens, size_t startPos);
size_t PG_get_size_till_next_semicolon(TOKEN **tokens, size_t startPos);
NodeReport PG_create_array_access_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_function_call_tree(TOKEN **tokens, size_t startPos);
size_t PG_add_params_to_node(Node *node, TOKEN **tokens, size_t startPos, int addStart, enum NodeType stdType);
int PG_get_bound_of_single_param(TOKEN **tokens, size_t startPos);
enum NodeType PG_get_node_type_by_value(char **value);
NodeReport PG_create_simple_term_node(TOKEN **tokens, size_t startPos, size_t boundaries);
int PG_forward_till_plus_or_minus(TOKEN **tokens, size_t startPos);
int PG_predict_member_access(TOKEN **tokens, size_t startPos, enum CONDITION_TYPE type);
NodeReport PG_assign_processed_node_to_node(TOKEN **tokens, size_t startPos, int useOptionalTyping);
size_t PG_go_backwards_till_operator(TOKEN **tokens, size_t startPos);
int PG_determine_bounds_for_capsulated_term(TOKEN **tokens, size_t startPos);
int PG_is_next_operator_multiply_divide_or_modulo(TOKEN **tokens, size_t startPos);
int PG_is_next_iden_a_member_access(TOKEN **tokens, size_t startPos);
NodeReport PG_create_member_access_tree(TOKEN **tokens, size_t startPos, int useOptionalTyping);
int PG_is_member_access(TOKEN **tokens, size_t startPos);
int PG_handle_member_access_brackets(TOKEN *currentToken, int *openBrackets, int *openEdgeBrackets);
char *PG_get_identifier_by_index(TOKEN *token);
int PG_is_function_call(TOKEN **tokens, size_t startPos);
int PG_handle_RBracket_function_call(TOKEN **tokens, size_t startPos);
int PG_handle_LBracket_function_call(TOKEN **tokens, size_t startPos);
int PG_execute_direct_check_for_function_call(TOKEN **tokens, size_t startPos);
NodeReport PG_get_member_access_side_node_tree(TOKEN **tokens, size_t startPos, enum processDirection direction, int useOptionalTyping);
void PG_create_post_member_access_side_node_tree(Node **topNode, TOKEN **tokens, int *internalSkip, int useOptionalTyping);
int PG_propagate_back_till_iden(TOKEN **tokens, size_t startPos);
int PG_propagate_offset_by_direction(TOKEN **tokens, size_t startPos, enum processDirection direction);
int PG_back_shift_array_access(TOKEN **tokens, size_t startPos);
int PG_predict_argument_count(TOKEN **tokens, size_t startPos, int withPredefinedBrackets);
int PG_predict_primitive_param_count(TOKEN **tokens, size_t startPos);
int PG_is_operator(const TOKEN *token);
int PG_add_varType_definition(TOKEN **tokens, size_t startPos, Node *parentNode);
int PG_count_varType_dimensions(TOKEN **tokens, size_t startPos);
Node *PG_create_modifier_node(TOKEN *token, int *skip);
Node *PG_create_node(char *value, enum NodeType type, int line, int pos);
NodeReport PG_create_node_report(Node *topNode, int tokensToSkip);
void PG_allocate_node_details(Node *node, size_t size);
void PG_print_from_top_node(Node *topNode, int depth, int pos);
int FREE_NODE(Node *node);

/**
 * <p>
 * Holds the size of the token array.
 * </p>
*/
extern size_t TOKEN_LENGTH;

/**
 * <p>
 * This is the entrypoint of the parsetree.
 * </p>
 * 
 * @param **tokens  Pointer to the token array
 * @param TOKEN_LENGTH   Length of the token array
*/
Node *GenerateParsetree(TOKEN **tokens) {
	(void)printf("\n\n\n>>>>>>>>>>>>>>>>>>>>    PARSETREE    <<<<<<<<<<<<<<<<<<<<\n\n");

	if (tokens == NULL || TOKEN_LENGTH == 0) {
		(void)PARSER_TOKEN_TRANSMISSION_EXCEPTION();
	}

	printf("TOKEN_LENGTH: %li\n", TOKEN_LENGTH);

	// CLOCK FOR DEBUG PURPOSES ONLY!!
	clock_t start, end;

	if (PARSETREE_GENERATOR_DISPLAY_USED_TIME == 1) {
		start = (clock_t)clock();
	}

	//Tree generation process
	NodeReport runnable = PG_create_runnable_tree(tokens, 0, Main);

	if (PARSETREE_GENERATOR_DISPLAY_USED_TIME == 1) {
		end = (clock_t)clock();            
	}
	
	if (PARSETREE_GENERATOR_DEBUG_MODE == 1) {
		if (runnable.node == NULL) {
			printf("Something went wrong in the parsetree generation step.");
		} else {
			(void)PG_print_from_top_node(runnable.node, 0, 0);
		}
	}

	if (PARSETREE_GENERATOR_DISPLAY_USED_TIME == 1) {
		(void)PG_print_cpu_time(((double) (end - start)) / CLOCKS_PER_SEC);
	}

	if (runnable.node == NULL) {
		printf("Something went wrong (PG)!\n");
	}

	(void)printf("\n\n\n>>>>>    Tokens converted to tree    <<<<<\n\n");

	return runnable.node;
}

/**
 * <p>
 * Prints the used CPU time of the measured time.
 * </p>
 * 
 * @param cpu_time_used Used CPU time
 */
void PG_print_cpu_time(float cpu_time_used) {
	(void)printf("\nCPU time used for PARSETREE GENERATION: %f seconds\n", cpu_time_used);
}

/**
 * <p>
 * Generates a subtree for a runnable / block statment.
 * </p>
 * 
 * <p>
 * As a block counts the inner part of a function for instance,
 * or the inner part of a class.
 * </p>
 * 
 * <p>
 * <strong>Layout</strong>
 * The generated subtree layout is as follows:
 * </p>
 * 
 * ```
 * [RUNNABLE]
 *     |
 * [STATEMENT]
 * [EXPRESSION]
 * ```
 * 
 * <p>
 * The [RUNNABLE] is created as a fully independent node,
 * whose [STATEMENT] and [EXPRESSION] can be found in
 * ´node->details[position]´.
 * </p>
 * 
 * <p>
 * <strong>Layout descriptions</strong>
 * <b>[STATEMENT]</b>: Statements in the source
 * <b>[EXPRESSION]</b>: Expressions to run
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position from where to start constructing the subtree
 * @param type      Defines the Type of the runnable, for special cases like
 *                  the check-is statement.
 */
NodeReport PG_create_runnable_tree(TOKEN **tokens, size_t startPos, enum RUNNABLE_TYPE type) {
	TOKEN *token = &(*tokens)[startPos];
	Node *parentNode = PG_create_node("RUNNABLE", _RUNNABLE_NODE_, token->line, token->tokenStart);
	int argumentCount = 0;
	size_t jumper = 0;
	
	while (startPos + jumper < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[startPos + jumper];

		if (currentToken->type == _OP_LEFT_BRACE_) {
			if (type == Main || type == InBlock) {
				jumper++;
			}
			
			break;
		} else if (currentToken->type == __EOF__) {
			break;
		} else if (currentToken->type == _KW_IS_
			&& type == IsStatement) {
			break;
		}

		NodeReport report = PG_get_report_based_on_token(tokens, startPos + jumper, type);

		if (report.node != NULL) {
			if (argumentCount == 0) {
				(void)PG_allocate_node_details(parentNode, 1);
			} else {
				Node **temp = (Node**)realloc(parentNode->details, sizeof(Node) * argumentCount + 1);

				if (temp == NULL) {
					free(parentNode->details);
					FREE_MEMORY();
					printf("Something went wrong (runnable)!\n");
					exit(EXIT_FAILURE);
				}

				parentNode->details = temp;
				parentNode->detailsCount = argumentCount + 1;
			}

			parentNode->details[argumentCount++] = report.node;
			jumper += report.tokensToSkip;
		} else {
			jumper++;
		}
	}

	return PG_create_node_report(parentNode, jumper);
}

/**
 * <p>
 * Get a keyword based NodeReport (based on prediction).
 * </p>
 * 
 * <p>
 * The SPACE Language has many constrol flow statements like the
 * for statement, which is indicated by the `for` keyword. Based on
 * the keywords in the expression the parsetree generator can predict
 * the correct tree path.
 * </p>
 * 
 * @returns The predicted NodeReport with the built subtree
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start construction the tree
 * @param type      Type of the parent runnable (only for Check statement)
 */
NodeReport PG_get_report_based_on_token(TOKEN **tokens, size_t startPos, enum RUNNABLE_TYPE type) {
	if (type == CheckStatement) {
		return PG_create_is_statement_tree(tokens, startPos);
	}
	
	switch ((*tokens)[startPos].type) {
	case _KW_VAR_:
	case _KW_CONST_:
		return PG_create_variable_tree(tokens, startPos);
	case _KW_INCLUDE_:
		return PG_create_include_tree(tokens, startPos);
	case _KW_EXPORT_:
		return PG_create_export_tree(tokens, startPos);
	case _KW_FOR_:
		return PG_create_for_statement_tree(tokens, startPos);
	case _KW_ENUM_:
		return PG_create_enum_tree(tokens, startPos);
	case _KW_FUNCTION_:
		return PG_create_function_tree(tokens, startPos);
	case _KW_CATCH_:
		return PG_create_catch_tree(tokens, startPos);
	case _KW_TRY_:
		return PG_create_try_tree(tokens, startPos);
	case _KW_CLASS_:
		return PG_create_class_tree(tokens, startPos);
	case _KW_WHILE_:
		return PG_create_while_statement_tree(tokens, startPos);
	case _KW_DO_:
		return PG_create_do_statement_tree(tokens, startPos);
	case _KW_CHECK_:
		return PG_create_check_statement_tree(tokens, startPos);
	case _KW_IF_:
		return PG_create_if_statement_tree(tokens, startPos);
	case _KW_ELSE_:
		if ((*tokens)[startPos + 1].type == _KW_IF_) {
			return PG_create_else_if_statement_tree(tokens, startPos);
		} else {
			return PG_create_else_statement_tree(tokens, startPos);
		}
	case _KW_CONTINUE_:
	case _KW_BREAK_:
		return PG_create_abort_operation_tree(tokens, startPos);
	case _KW_RETURN_:
		return PG_create_return_statement_tree(tokens, startPos);
	case _KW_GLOBAL_:
	case _KW_SECURE_:
	case _KW_PRIVATE_:
		if ((*tokens)[startPos + 1].type == _KW_FUNCTION_) {
			return PG_create_function_tree(tokens, startPos);
		} else if ((*tokens)[startPos + 1].type == _KW_CLASS_) {
			return PG_create_class_tree(tokens, startPos);
		} else if ((*tokens)[startPos + 1].type == _KW_VAR_
			|| (*tokens)[startPos + 1].type == _KW_CONST_) {
			return PG_create_variable_tree(tokens, startPos);
		}
		break;
	default:
		if ((*tokens)[startPos].type == _KW_THIS_
			&& (*tokens)[startPos + 3].type == _KW_CONSTRUCTOR_) {
			return PG_create_class_constructor_tree(tokens, startPos);
		} else if ((*tokens)[startPos].type == _OP_SEMICOLON_) {
			break;
		}

		if ((int)PG_predict_assignment(tokens, startPos) == true) {
			return PG_create_simple_assignment_tree(tokens, startPos);
		}

		int fncCallBounds = (int)PG_predict_function_call(tokens, startPos);

		if (fncCallBounds > 0) {
			return PG_create_simple_term_node(tokens, startPos, fncCallBounds);
		}

		break;
	}

	return PG_create_node_report(NULL, UNINITIALZED);
}

//// TEMPORARELY!!!
int PG_predict_function_call(TOKEN **tokens, size_t startPos) {
	int counter = 0;

	while (startPos + counter < TOKEN_LENGTH) {
		TOKEN *token = &(*tokens)[startPos + counter];

		if (token->type == _OP_SEMICOLON_) {
			break;
		}

		counter++;
	}

	return counter;
}

/**
 * <p>
 * Creates a subtree for an else statement.
 * </p>
 * 
 * <p>
 * <strong>Layout</strong>
 * ```
 * [ELSE_STMT]
 *           \
 *           [RUNNABLE]
 * ```
 * 
 * <b>[ELSE_STMT]</b>: Indicator for the else statment
 * <b>[RUNNABLE]</b>: Runnable of the else statement
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the else statement starts
 * 
 * @returns A NodeReport containing the root node and how many tokens
 *          got processed.
*/
NodeReport PG_create_else_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *node = PG_create_node(token->value, _ELSE_STMT_NODE_, token->line, token->tokenStart);
	int skip = 2;
	
	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	node->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(node, skip);
}

/**
 * <p>
 * Creates a subtree for an else-if statement.
 * </p>
 * 
 * <p>
 * <strong>Layout:</strong>
 * ```
 *     [EIF_STMT]
 *    /          \
 * [COND]     [RUNNABLE]
 * ```
 * 
 * <b>[EIF_STMT]</b>: Indicator for the else-if statment
 * <b>[COND]</b>: Condition that has to be met to run the runnable
 * <b>[RUNNABLE]</b>: Runnable of the else-if statement
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the else-if statement starts
 * 
 * @returns A NodeReport with the root node and how many tokens to skip
 */
NodeReport PG_create_else_if_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *node = PG_create_node(token->value, _ELSE_IF_STMT_NODE_, token->line, token->tokenStart);
	int skip = 0;

	NodeReport chainedCondReport = PG_create_chained_condition_tree(tokens, startPos + 3, false);
	node->leftNode = chainedCondReport.node;
	skip += chainedCondReport.tokensToSkip + 4;

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	node->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(node, skip);
}

/**
 * <p>
 * Creates a subtree for an if statement.
 * </p>
 * 
 * <p>
 * <strong>Layout:</strong>
 * ```
 *     [IF_STMT]
 *    /         \
 * [COND]    [RUNNABLE]
 * ```
 * 
 * <b>[EI_STMT]</b>: Indicator for the else-if statment
 * <b>[COND]</b>: Condition that has to be met to run the runnable
 * <b>[RUNNABLE]</b>: Runnable of the else-if statement
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the if statement starts
 * 
 * @returns A NodeReport with the root node and how many tokens to skip
 */
NodeReport PG_create_if_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *node = PG_create_node(token->value, _IF_STMT_NODE_, token->line, token->tokenStart);
	int skip = 2;

	NodeReport chainedCondReport = PG_create_chained_condition_tree(tokens, startPos + skip, false);
	node->leftNode = chainedCondReport.node;
	skip += chainedCondReport.tokensToSkip + 2;

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	node->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(node, skip);
}

/**
 * <p>
 * Creates a subtree for a for statement.
 * </p>
 * 
 * <p>
 * <strong>Layout:</strong>
 * ```
 *    [FOR_STMT]
 *    /    |   \
 * [VAR] [COND] [RUNNABLE]
 *      [ACTION]
 * 
 * <b>[FOR_STMT]</b>: Indicator for the for statment
 * <b>[VAR]</b>: Var to use as "counter" or iterator
 * <b>[COND]</b>: Condition that has to be met to run the loop
 * <b>[ACTION]</b>: Actions that occur at every iteration, like incrementing
 * <b>[RUNNABLE]</b>: Runnable in the the loop itself
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the for statement starts
 * 
 * @returns A NodeReport with the root node and how many tokens to skip
 */
NodeReport PG_create_for_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = PG_create_node("FOR", _FOR_STMT_NODE_, token->line, token->tokenStart);
	(void)PG_allocate_node_details(topNode, 2);
	int skip = 2;
	/*
	> for (var i = 0; i < 10; i++) {}
	>      ^
	> (*tokens)[startPos + 2]
	*/

	NodeReport varReport = PG_create_variable_tree(tokens, startPos + 2);
	topNode->leftNode = varReport.node;
	skip += varReport.tokensToSkip + 1; //+1 for ';'

	NodeReport chainedReport = PG_create_chained_condition_tree(tokens, startPos + skip, false);
	topNode->details[0] = chainedReport.node;
	skip += chainedReport.tokensToSkip + 1; //+1 for ';'

	NodeReport expressionReport = PG_create_simple_assignment_tree(tokens, startPos + skip);
	topNode->details[1] = expressionReport.node;
	skip += expressionReport.tokensToSkip + 2; //+2 for ')' and '{'
	
	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(topNode, skip);
}

/**
 * <p>
 * Predicts of the following token sequence matches an assignment or not.
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the else-if statement starts
 * 
 * @returns
 * <ul>
 * <li>true - Following tokens are an assignment
 * <li>false - Following tokens are not an assignment
 * </ul>
 */
int PG_predict_assignment(TOKEN **tokens, size_t startPos) {
	for (int i = startPos; i < TOKEN_LENGTH; i++) {
		switch ((*tokens)[i].type) {
		case _OP_SEMICOLON_:
			return false;
		case _OP_EQUALS_:           case _OP_PLUS_EQUALS_:
		case _OP_MINUS_EQUALS_:     case _OP_ADD_ONE_:
		case _OP_SUBTRACT_ONE_:     case _OP_MULTIPLY_EQUALS_:
		case _OP_DIVIDE_EQUALS_:
			return true;
		default: continue;
		}
	}

	return false;
}

/**
 * <p>
 * Converts a TOKENTYPES to a NodeType.
 * <p>
 * 
 * <p>
 * <strong>Warning:</strong>
 * This function only converts operators.
 * </p>
 * 
 * @param type  Type to convert
 * 
 * @returns The converted NodeType
 */
enum NodeType PG_get_nodeType_of_operator(TOKENTYPES type) {
	switch (type) {
	case _OP_SUBTRACT_ONE_:         return _DECREMENT_ONE_NODE_;
	case _OP_ADD_ONE_:              return _INCREMENT_ONE_NODE_;
	case _OP_PLUS_EQUALS_:          return _PLUS_EQUALS_NODE_;
	case _OP_MINUS_EQUALS_:         return _MINUS_EQUALS_NODE_;
	case _OP_MULTIPLY_EQUALS_:      return _MULTIPLY_EQUALS_NODE_;
	case _OP_DIVIDE_EQUALS_:        return _DIVIDE_EQUALS_NODE_;
	case _OP_EQUALS_:               return _EQUALS_NODE_;
	default:                        return _NULL_;
	}
}

/**
 * <p>
 * Evaluates the length of a simple term.
 * </p>
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start evaluating
 */
int PG_get_term_bounds(TOKEN **tokens, size_t startPos) {
	int openBrackets = 0;
	int openEdgeBrackets = 0;

	for (int i = startPos; i < TOKEN_LENGTH; i++) {
		switch ((*tokens)[i].type) {
		case _OP_LEFT_BRACKET_:
			openBrackets--;

			if (openBrackets < 0
				&& (int)PG_is_calculation_operator(&(*tokens)[i + 1]) == false) {
				return i - startPos;
			}

			break;
		case _OP_RIGHT_BRACKET_:
			openBrackets++;
			break;
		case _OP_LEFT_EDGE_BRACKET_:
			openEdgeBrackets--;

			if (openEdgeBrackets < 0) {
				return i - startPos;
			}

			break;
		case _OP_RIGHT_EDGE_BRACKET_:
			openEdgeBrackets++;
			break;
		case _OP_SEMICOLON_:        case _OP_EQUALS_:
		case _OP_PLUS_EQUALS_:      case _OP_MINUS_EQUALS_:
		case _OP_MULTIPLY_EQUALS_:  case _OP_DIVIDE_EQUALS_:
		case _OP_LEFT_BRACE_:
			return i - startPos;
		default: continue;
		}
	}

	return -1;
}

/**
 * <p>
 * Checks if a token is a calculation operator (e.g. '+', '-', '*', ...).
 * </p>
 * 
 * @param *token    Token to check
 * 
 * @returns
 * <ul>
 * <li>true - Token is calculation operator
 * <li>false - Token is not a calculation operator
 * </ul>
 */
int PG_is_calculation_operator(TOKEN *token) {
	switch (token->type) {
	case _OP_PLUS_:     case _OP_MINUS_:
	case _OP_MULTIPLY_: case _OP_DIVIDE_:
	case _OP_MODULU_:
		return true;
	default: return false;
	}
}

/**
 * <p>
 * Creates a subtree for an assignment.
 * </p>
 * 
 * <p>
 * <strong>Layout:</strong>
 * ```
 *   [ASS_TYPE]
 *    /     \
 * [VAR]   [VAL]
 * ```
 * 
 * <b>[ASS_TYPE]</b>: Determines the assignment type
 * <b>[VAL]</b>: Value to assign
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the assginment starts
 * 
 * @returns A NodeReport with the root node and how many tokens to skip
 */
NodeReport PG_create_simple_assignment_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *operatorNode = NULL;
	NodeReport lRep = {NULL, UNINITIALZED};
	int skip = 0;
	
	//Array assignment handling
	if ((*tokens)[startPos + 1].type == _OP_DOT_
		|| (*tokens)[startPos + 1].type == _OP_CLASS_ACCESSOR_) {
		lRep = PG_create_member_access_tree(tokens, startPos, false);
		lRep.tokensToSkip--;
	//Increment and decrement handling
	} else if ((int)PG_predict_increment_or_decrement_assignment(tokens, startPos) == true) {
		return PG_create_increment_decrement_tree(tokens, startPos);
	} else {
		int bounds = PG_get_term_bounds(tokens, startPos);
		lRep = PG_create_simple_term_node(tokens, startPos, bounds);
	}

	skip += lRep.tokensToSkip;
	operatorNode = PG_create_node((*tokens)[startPos + skip].value, PG_get_nodeType_of_operator((*tokens)[startPos + skip].type), token->line, token->tokenStart);
	operatorNode->leftNode = lRep.node;
	
	skip++;
	NodeReport rRep = {NULL, UNINITIALZED};
	
	if (get_var_type(tokens, startPos + skip) == COND_VAR) {
		rRep = PG_create_condition_assignment_tree(tokens, startPos + skip);
	} else if ((int)PG_predict_increment_or_decrement_assignment(tokens, startPos + skip) == true) {
		rRep = PG_create_increment_decrement_tree(tokens, startPos);
	//String assignment handling
	} else if ((*tokens)[startPos + skip].type == _STRING_
		|| (*tokens)[startPos + skip].type == _CHARACTER_ARRAY_) {
		Node *node = PG_create_node((*tokens)[startPos + skip].value, _STRING_NODE_, (*tokens)[startPos + skip].line, (*tokens)[startPos + skip].tokenStart);
		rRep = PG_create_node_report(node, 2);
	//Null assignment handling
	} else if ((*tokens)[startPos + skip].type == _KW_NULL_) {
		Node *node = PG_create_node((*tokens)[startPos + skip].value, _NULL_NODE_, (*tokens)[startPos + skip].line, (*tokens)[startPos + skip].tokenStart);
		rRep = PG_create_node_report(node, 2);
	//Memeber access handling
	} else if ((int)PG_predict_member_access(tokens, startPos, NONE) == true) {
		rRep = PG_create_member_access_tree(tokens, startPos + skip, false);
	} else {
		int bounds = (int)PG_get_term_bounds(tokens, startPos + skip);
		rRep = PG_create_simple_term_node(tokens, startPos + skip, bounds);
	}

	operatorNode->rightNode = rRep.node;
	skip += rRep.tokensToSkip;

	return PG_create_node_report(operatorNode, skip);
}

int PG_predict_member_access(TOKEN **tokens, size_t startPos, enum CONDITION_TYPE type) {
	int openEdgeBrackets = 0;

	for (int i = startPos; i < TOKEN_LENGTH; i++) {
		TOKEN *tok = &(*tokens)[i];

		if ((int)PG_is_calculation_operator(tok) == true
			&& openEdgeBrackets == 0) {
			return false;
		} else if (tok->type == _OP_SEMICOLON_) {
			return true;
		} else if ((int)PG_is_condition_operator(tok->type) == true) {
			return true;
		} else if (tok->type == _OP_COMMA_) {
			return false;
		} else if ((int)is_primitive(tok->type) == true) {
			return false;
		} else if (tok->type == _OP_LEFT_EDGE_BRACKET_) {
			openEdgeBrackets--;
		} else if (tok->type == _OP_RIGHT_EDGE_BRACKET_) {
			openEdgeBrackets++;
		}
		
		if (type == IGNORE_ALL) {
			if (tok->type == _OP_LEFT_BRACKET_
				|| (int)PG_is_calculation_operator(tok) == true) {
				return true;
			}
		}
	}

	return false;
}

/**
 * <p>
 * Creates a subtree for an increment or decrement assignment.
 * </p>
 * 
 * <p>
 * <strong>Layout:</strong>
 * ```
 *     [SASS]
 *    /      \
 * [LID]    [RID]
 * ```
 * 
 * <b>[LID]</b>: Left increments or decrements
 * <b>[RID]</b>: Right increments or decrements
 * <b>[SASS]</b>: Indicator for increments and decrements
 * </p>
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the increment / decrement statement starts
 * 
 * @returns A NodeReport with the root node and how many tokens to skip
 */
NodeReport PG_create_increment_decrement_tree(TOKEN **tokens, size_t startPos) {
	int skip = 0;
	int idenpassedBy = false;
	int breakLoop = false;
	Node *topNode = PG_create_node("SASS", _SIMPLE_INC_DEC_ASS_NODE_, (*tokens)[startPos].line, (*tokens)[startPos].tokenStart);
	Node *cache = NULL;
	
	while (startPos + skip < TOKEN_LENGTH
		&& breakLoop == false) {
		TOKEN *currentToken = &(*tokens)[startPos + skip];
		Node *currentNode = NULL;
		int line = currentToken->line;
		int tokenStart = currentToken->tokenStart;

		switch (currentToken->type) {
		case _IDENTIFIER_: {
			NodeReport idenRep = PG_create_member_access_tree(tokens, startPos + skip, false);
			(void)PG_allocate_node_details(topNode, 1);
			topNode->details[0] = idenRep.node;
			skip += idenRep.tokensToSkip;
			idenpassedBy = true;
			topNode->leftNode = cache;
			cache = NULL;
			continue;
		}
		case _OP_ADD_ONE_:
			currentNode = PG_create_node("++", _INCREMENT_ONE_NODE_, line, tokenStart);
			break;
		case _OP_SUBTRACT_ONE_:
			currentNode = PG_create_node("--", _DECREMENT_ONE_NODE_, line, tokenStart);
			break;
		default:
			breakLoop = true;
			continue;
		}

		if (cache == NULL) {
			cache = currentNode;
		} else {
			if (idenpassedBy == false) {
				currentNode->leftNode = cache;
			} else {
				currentNode->rightNode = cache;
			}

			cache = currentNode;
		}

		skip++;
	}

	topNode->rightNode = cache;
	return PG_create_node_report(topNode, skip);
}

/**
 * <p>
 * Tries to predict whether the following tokens indicate an increment or
 * decrement operation.
 * </p>
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start predicting
 * 
 * @returns
 * <ul>
 * <li>true - Following tokens are an increment or decrement operation
 * <li>false - Following tokens are neither an increment nor an decrement operation
 * </ul>
 */
int PG_predict_increment_or_decrement_assignment(TOKEN **tokens, size_t startPos) {
	int openBrackets = 0;
	int openEdgeBrackets = 0;
	
	for (int i = startPos; i < TOKEN_LENGTH; i++) {
		TOKEN *curTok = &(*tokens)[i];

		if (curTok->type == _OP_LEFT_BRACKET_) {
			openBrackets--;
		} else if (curTok->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		} else if (curTok->type == _OP_LEFT_EDGE_BRACKET_) {
			openEdgeBrackets--;
		} else if (curTok->type == _OP_RIGHT_EDGE_BRACKET_) {
			openEdgeBrackets++;
		}

		if (openBrackets != 0 || openEdgeBrackets != 0) {
			continue;
		}

		if (curTok->type == _OP_SEMICOLON_) {
			break;
		} else if (curTok->type != _OP_ADD_ONE_
			&& curTok->type != _OP_SUBTRACT_ONE_) {
			continue;
		}

		if ((*tokens)[i - 1].type == _IDENTIFIER_
			|| (*tokens)[i + 1].type == _IDENTIFIER_) {
			return true;
		}

		return false;
	}

	return false;
}

/**
 * <p>
 * Creates a subtree for an is statement.
 * </p>
 * 
 * <p>
 * <strong>Layout:</strong>
 *   [IS]
 *   /  \
 * [V]  [R]
 * 
 * [IS]: Indicator for the is statement with value to check against
 * [R]: Runnable in the is statement section
 * [V]: Value to check
 * </p>
 * 
 * @returns A NodeReport with the root node and the amount of processed tokens
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start constructing the tree
 */
NodeReport PG_create_is_statement_tree(TOKEN **tokens, size_t startPos) {
	/*
	> is 3:
	>    ^
	> (*tokens)[startPos + 2]
	*/
	Node *topNode = PG_create_node("IS", _IS_STMT_NODE_, (*tokens)[startPos].line, (*tokens)[startPos].tokenStart);

	int skip = 1;
	NodeReport idenRep = PG_create_member_access_tree(tokens, startPos + skip, false);
	topNode->leftNode = idenRep.node;
	skip += idenRep.tokensToSkip + 1; //+1 for the ':'

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, IsStatement);
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(topNode, skip);
}

/**
 * <p>
 * Creates a subtree for a check statement.
 * </p>
 * 
 * <p><strong>Layout:</strong>
 *    [CHECK]
 *    /     \
 *  [V]     [IS]
 *          /  \
 *        [V]  [R]
 * 
 * [CHECK]     := Value to check
 * [IS]        := One of multiple branch options
 * [R]         := Runnable in the is-statement
 * [V]         := Value to check (member access tree)
 * </p>
 * 
 * @returns NodeReport with the root node and how many tokens were processed
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start constructing the check
 *                  statement tree
 */
NodeReport PG_create_check_statement_tree(TOKEN **tokens, size_t startPos) {
	/*
	> check (a) {}
	>        ^
	> (*tokens)[startPos + 2]
	*/
	int skip = 2;
	Node *topNode = PG_create_node("CHECK", _CHECK_STMT_NODE_, (*tokens)[startPos].line, (*tokens)[startPos].tokenStart);

	NodeReport idenRep = PG_create_member_access_tree(tokens, startPos + skip, false);
	topNode->leftNode = idenRep.node;
	skip += idenRep.tokensToSkip + 2;

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, CheckStatement);
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip + 1;
	return PG_create_node_report(topNode, skip);
}

/**
 * <p>
 * Creates a subtree for abort operations (break, continue).
 * </p>
 * 
 * @returns NodeReport with the abort operation and tokens to skip
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position at which the abort is located at
 */
NodeReport PG_create_abort_operation_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = NULL;

	if ((*tokens)[startPos].type == _KW_CONTINUE_) {
		topNode = PG_create_node("CONTINUE", _CONTINUE_STMT_NODE_, token->line, token->tokenStart);
	} else if ((*tokens)[startPos].type == _KW_BREAK_) {
		topNode = PG_create_node("BREAK", _BREAK_STMT_NODE_, token->line, token->tokenStart);
	}

	return PG_create_node_report(topNode, 2);
}

/*
Purpose: Creates a subtree for a return statement
Return Type: NodeReport => Contains the topNode as well as how many tokens to skip
Params: TOKEN **tokens => Point to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:
 
	[RET_STMT]
	/
[RET]

The indicator node [RET_STMT] has the return value at
´´´node->leftNode´´´.

[RET_STMT]: Indicator for the return statement
[RET]: Return value
*/
NodeReport PG_create_return_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = PG_create_node("RETURN_STATMENT", _RETURN_STMT_NODE_, token->line, token->tokenStart);
	int skip = 0;
	
	if ((*tokens)[startPos + 1].type == _KW_NEW_) {
		NodeReport classInstanceReport = PG_create_class_instance_tree(tokens, startPos + 1);
		topNode->leftNode = classInstanceReport.node;
		skip += classInstanceReport.tokensToSkip + 1;
	} else if (get_var_type(tokens, startPos + 1) == COND_VAR) {
		NodeReport condReport = PG_create_condition_assignment_tree(tokens, startPos + 1);
		topNode->leftNode = condReport.node;
		skip += condReport.tokensToSkip;
	} else if ((*tokens)[startPos + 1].type == _OP_RIGHT_BRACE_) {
		int dims = (int)PG_predict_array_init_count(tokens, startPos + 1);
		NodeReport arrRep = PG_create_array_init_tree(tokens, startPos + 2, dims);
		topNode->leftNode = arrRep.node;
		skip += arrRep.tokensToSkip + 1;
	} else {
		int bounds = (int)PG_get_term_bounds(tokens, startPos + 1);
		NodeReport termReport = PG_create_simple_term_node(tokens, startPos + 1, bounds);
		topNode->leftNode = termReport.node;
		skip += bounds + 1;
	}

	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Creates a subtree for a do statement
Return Type: NodeReport => Contains the topNode as well as how many tokens to skip
Params: TOKEN **tokens => Point to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:
 
	[DO_STMT]
	/       \
[COND]   [RUNNABLE]

The indicator node [DO_STMT] has the conditions at
´´´node->leftNode´´´ and the runnable block in
´´´node->rightNode´´´.

[DO_STMT]: Indicator for the do statement
[COND]: Chained condition to be fulfilled
[RUNNABLE]: Block in the while statement
*/
NodeReport PG_create_do_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = PG_create_node("DO_STMT", _DO_STMT_NODE_, token->line, token->tokenStart);
	int skip = 2;

	/*
	> do { }
	>     ^
	> (*tokens)[startPos + skip]
	*/

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip + 2;

	/*
	> do {} while (a == 2);
	>              ^
	>  (*tokens)[startPos + skip + 2]
	*/

	NodeReport chainedCondReport = PG_create_chained_condition_tree(tokens, startPos + skip, false);
	topNode->leftNode = chainedCondReport.node;
	skip += chainedCondReport.tokensToSkip + 1; //Skip the ';'
	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Creates a subtree for a while statement
Return Type: NodeReport => Contains the topNode as well as how many tokens to skip
Params: TOKEN **tokens => Point to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:
 
   [WHILE_STMT]
	/       \
[COND]   [RUNNABLE]

The indicator node [WHILE_STMT] has the conditions at
´´´node->leftNode´´´ and the runnable block in
´´´node->rightNode´´´.

[WHILE_STMT]: Indicator for the while statement
[COND]: Chained condition to be fulfilled
[RUNNABLE]: Block in the while statment
*/
NodeReport PG_create_while_statement_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = PG_create_node("WHILE_STMT", _WHILE_STMT_NODE_, token->line, token->tokenStart);
	int skip = 2;

	/*
	> while (a == 2) {}
	>        ^
	>   (*tokens)[startPos + skip]
	*/

	NodeReport chainedCondReport = PG_create_chained_condition_tree(tokens, startPos + skip, false);
	topNode->leftNode = chainedCondReport.node;
	skip += chainedCondReport.tokensToSkip + 2; //Skip the ')' and '{'

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;
	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Returns a tree from the created var calls
Return Type: NodeReport => Contains the topNode as well as how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing
*/
NodeReport PG_create_variable_tree(TOKEN **tokens, size_t startPos) {
	enum VAR_TYPE type = get_var_type(tokens, startPos);
	NodeReport report = {NULL, UNINITIALZED};

	if (type == NORMAL_VAR) {
		report = PG_create_normal_var_tree(tokens, startPos);
	} else if (type == ARRAY_VAR) {
		report = PG_create_array_var_tree(tokens, startPos);
	} else if (type == COND_VAR) {
		report = PG_create_conditional_var_tree(tokens, startPos);
	} else if (type == INSTANCE_VAR) {
		report = PG_create_instance_var_tree(tokens, startPos);
	}

	return report;
}

/*
Purpose: Determines the variable type
Return Type: enum varType => Type of the variable
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start checking
*/
enum VAR_TYPE get_var_type(TOKEN **tokens, size_t startPos) {
	int colonBefore = false;
	int equalsPassed = false;
	int colonSkip = 0;

	for (size_t i = startPos; i < TOKEN_LENGTH; i++) {
		if (colonBefore == true) {
			if (colonSkip > 0 && (*tokens)[i].type == _IDENTIFIER_) {
				colonBefore = false;
			}

			colonSkip++;
			continue;
		}

		switch ((*tokens)[i].type) {
		case _OP_COLON_:
			colonBefore = true;
			continue;
		case _OP_EQUALS_:
			equalsPassed = true;
			continue;
		case _KW_NEW_:
			return INSTANCE_VAR;
		case _OP_SEMICOLON_:
			return NORMAL_VAR;
		case _OP_RIGHT_EDGE_BRACKET_:
			if (equalsPassed == true || colonBefore == true) {
				return NORMAL_VAR;
			} else {
				return ARRAY_VAR;
			}
		case _OP_QUESTION_MARK_:
			return COND_VAR;
		default:
			break;
		}
	}

	return UNDEF;
}

/*
Purpose: Create a subtree for a class instance definition
Return Type: NodeReport => Contains the topNode and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

[INSTANCE]
	|
  [VAL]
_______________________________
*/
NodeReport PG_create_class_instance_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos + 1];
	Node *topNode = PG_create_node(token->value, _INHERITED_CLASS_NODE_, token->line, token->tokenStart);
	int bounds = (int)PG_predict_argument_count(tokens, startPos + 2, false);
	(void)PG_allocate_node_details(topNode, bounds);
	int skip = (int)PG_add_params_to_node(topNode, tokens, startPos + 3, 0, _NULL_);

	return PG_create_node_report(topNode, skip + 4);
}

/**
 * @brief Creates a subtree for a class instance variable.
 * 
 * @returns A NodeReport with the root of the created subtree and
 * how many tokens were processed.
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start the tree construction
 * 
 * @note The Layout of the created tree is as followed:
 * @note _______________________________
 * @note Layout:
 * @note
 * @note  [INSTANCE]
 * @note     /   \
 * @note  [MOD] [VAL]
 * @note _______________________________
*/
NodeReport PG_create_instance_var_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	int skip = 0;
	Node *topNode = PG_create_node(NULL, _UNDEF_, token->line, token->tokenStart);
	topNode->leftNode = PG_create_modifier_node(token, &skip);

	if ((*tokens)[startPos + skip].type == _KW_CONST_) {
		topNode->type = _CONST_CLASS_INSTANCE_NODE_;
	} else {
		topNode->type = _VAR_CLASS_INSTANCE_NODE_;
	}

	skip++;
	
	if ((*tokens)[startPos + skip].type == _OP_COLON_) {
		skip += (int)PG_add_varType_definition(tokens, startPos + skip + 1, topNode) + 1;
	}
	
	topNode->value = (*tokens)[startPos + skip].value;
	skip += 3; //Skip the name, "=" and "new"
	token = &(*tokens)[startPos + skip];
	
	NodeReport classPathRep = PG_create_member_access_tree(tokens, startPos + skip, false);
	topNode->rightNode = classPathRep.node;
	topNode->rightNode->type = _INHERITED_CLASS_NODE_;
	skip += classPathRep.tokensToSkip;
	return PG_create_node_report(topNode, skip + 1);
}

NodeReport PG_create_condition_assignment_tree(TOKEN **tokens, size_t startPos) {
	NodeReport conditionReport = PG_create_chained_condition_tree(tokens, startPos, false);
	int skip = 0;
	TOKEN *token = &(*tokens)[startPos];

	Node *topNode = PG_create_node("?", _CONDITIONAL_ASSIGNMENT_NODE_, token->line, token->tokenStart);
	topNode->leftNode = conditionReport.node;
	skip += conditionReport.tokensToSkip + 1;

	(void)PG_allocate_node_details(topNode, 2);

	/*
	> var a = b == true ? 2 : 1;
	>                     ^
	>         (*tokens)[startPos + skip]
	*/
	NodeReport trueValue = {NULL, UNINITIALZED};
	
	if ((int)predict_is_conditional_assignment_type(tokens, startPos + skip, TOKEN_LENGTH) == true) {
		trueValue = PG_create_condition_assignment_tree(tokens, startPos + skip);
	} else {
		int bounds = PG_get_cond_assignment_bounds(tokens, startPos + skip);
		trueValue = PG_create_simple_term_node(tokens, startPos + skip, bounds);
		trueValue.tokensToSkip++;
	}

	topNode->details[0] = trueValue.node;
	skip += trueValue.tokensToSkip;

	NodeReport falseValue = {NULL, UNINITIALZED};
	
	if ((int)predict_is_conditional_assignment_type(tokens, startPos + skip, TOKEN_LENGTH) == true) {
		falseValue = PG_create_condition_assignment_tree(tokens, startPos + skip);
	} else {
		int bounds = PG_get_cond_assignment_bounds(tokens, startPos + skip);
		falseValue = PG_create_simple_term_node(tokens, startPos + skip, bounds);
		falseValue.tokensToSkip++;
	}

	topNode->details[1] = falseValue.node;
	skip += falseValue.tokensToSkip;

	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Create a subtree for a conditional variable definition
Return Type: NodeReport => Contains the topNode and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

	 [COND_VAR]
   /      |     \
[MOD]    [T]    [?]
			  /   |
		 [COND] [VAL]

[COND_VAR] is the root, appended is the modifier
at ´´´node->leftNode´´´ and the condition at
´´´node->rightNode´´´. The [?] is the assignment holder,
to it's leftNode is the condition and to the details the
assigning value if cond is true and false.

[COND_VAR]: Conditional var
[MOD]: Modifier
[?]: Conditional assignment indicator
[COND]: Condition
[VAL]: Values for true and false
[T]: Type of variable (optional)
_______________________________
*/
NodeReport PG_create_conditional_var_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = PG_create_node(NULL, _NULL_, token->line, token->tokenStart);
	int skip = 1;
	topNode->leftNode = PG_create_modifier_node(token, &skip);
	topNode->type = (*tokens)[startPos + skip].type == _KW_VAR_ ? _CONDITIONAL_VAR_NODE_ : _CONDITIONAL_CONST_NODE_;

	if ((*tokens)[startPos + skip].type == _OP_COLON_) {
		skip += PG_add_varType_definition(tokens, startPos + skip + 1, topNode);
		skip++;
	}

	topNode->value = PG_get_identifier_by_index(&(*tokens)[startPos + skip]);
	skip += 2;

	NodeReport conditionReport = PG_create_condition_assignment_tree(tokens, startPos + skip);
	topNode->rightNode = conditionReport.node;
	skip += conditionReport.tokensToSkip;

	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Get the size of an conditional assignment statement
Return Type: int => Size of the statement
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start Counting
*/
int PG_get_cond_assignment_bounds(TOKEN **tokens, size_t startPos) {
	int skip = 0;

	while (startPos + skip < TOKEN_LENGTH) {
		if ((*tokens)[startPos + skip].type == _OP_SEMICOLON_
			|| (*tokens)[startPos + skip].type == _OP_COLON_) {
			break;
		}

		skip++;
	}

	return skip;
}

/*
Purpose: Create a subtree for an array variable definition
Return Type: NodeReport => Contains the topNode and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

	 [ARR_VAR]
   /     |     \
[MOD] [DIMEN] [VAL]
		[T]

To the [ARR_VAR] appended are the modifier
([MOD]: ´´´node->leftNode´´´), the value
([VAL]: ´´´´node->rightNode´´´) and the
definitions ([NAMES]: ´´´´node->details[pos]´´´).

[ARR_VAR]: Contains variable name
[MOD]: Contains the var modifier
[VAL]: Value of the variables
[DIMEN]: Dimension of the array var
[T]: Type of the variable (optional)
_______________________________
*/
NodeReport PG_create_array_var_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	Node *topNode = PG_create_node(NULL, _ARRAY_VAR_NODE_, token->line, token->tokenStart);
	int skip = 1;
	topNode->leftNode = PG_create_modifier_node(token, &skip);

	if ((*tokens)[startPos + skip].type == _OP_COLON_) {
		skip += (int)PG_add_varType_definition(tokens, startPos + skip + 1, topNode) + 1;
	}

	topNode->value = PG_get_identifier_by_index(&(*tokens)[startPos + skip]);
	skip++;

	//DIMENSION HANDLING '_' on undefined dimension
	int dimCount = PG_get_dimension_count(tokens, startPos + skip);
	(void)PG_allocate_node_details(topNode, dimCount + 1);
	skip += (int)PG_add_dimensions_to_var_node(topNode, tokens, startPos + skip, 1);

	if ((*tokens)[startPos + skip].type == _OP_EQUALS_) {
		NodeReport rep = {NULL, UNINITIALZED};
		
		switch ((*tokens)[startPos + skip + 1].type) {
		case _OP_RIGHT_BRACE_:
			rep = PG_create_array_init_tree(tokens, startPos + skip + 2, 0);
			rep.tokensToSkip += 2;
			break;
		case _KW_NULL_: {
			Node *nullNode = PG_create_node("NULL", _NULL_NODE_, (*tokens)[startPos + skip + 1].line, (*tokens)[startPos + skip + 1].tokenStart);
			rep = PG_create_node_report(nullNode, 2);
			break;
		}
		case _STRING_:
		case _CHARACTER_ARRAY_: {
			Node *strNode = PG_create_node((*tokens)[startPos + skip + 1].value, _NULL_NODE_, (*tokens)[startPos + skip + 1].line, (*tokens)[startPos + skip + 1].tokenStart);
			rep = PG_create_node_report(strNode, 2);
			break;
		}
		case _KW_NEW_:
			rep = PG_create_array_creation_tree(tokens, startPos + skip + 2);
			break;
		default: {
			int termBounds = (int)PG_get_term_bounds(tokens, startPos + skip + 1);
			rep = PG_create_simple_term_node(tokens, startPos + skip + 1, termBounds);
			rep.tokensToSkip++;
			break;
		}
		}

		skip += rep.tokensToSkip;
		topNode->rightNode = rep.node;
	}

	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Create a subtree for an array creation
Return Type: NodeReport => Contains the topNode and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

[VAL]
  |
[DIM]
[DIM]

The dimensions of the array can be found in the
```node->details[i]```, while the type is in the
```node->value``` field.

[VAL]:	The array type
[DIM]:	Dimensions of the array
_______________________________
*/
NodeReport PG_create_array_creation_tree(TOKEN **tokens, size_t startPos) {
	Node *topNode = PG_create_node((*tokens)[startPos].value, _ARRAY_CREATION_NODE_, (*tokens)[startPos].line, (*tokens)[startPos].tokenStart);
	int skip = 1; //Skip the type
	int dims = PG_predict_array_creation_dimension_count(tokens, startPos + skip);
	(void)PG_allocate_node_details(topNode, dims);
	int currentDetail = 0;

	while (startPos + skip < TOKEN_LENGTH
		&& (*tokens)[startPos + skip].type != _OP_SEMICOLON_) {
		if ((*tokens)[startPos + skip].type != _OP_RIGHT_EDGE_BRACKET_) {
			break;
		}

		int termBounds = PG_get_term_bounds(tokens, startPos + skip + 1);
		NodeReport termRep = PG_create_simple_term_node(tokens, startPos + skip + 1, termBounds);

		topNode->details[currentDetail++] = termRep.node;
		skip += termRep.tokensToSkip + 2;
	}

	return PG_create_node_report(topNode, skip);
}

int PG_predict_array_creation_dimension_count(TOKEN **tokens, size_t startPos) {
	int jumper = 0;
	int dims = 0;
	int openEdgeBrackets = 0;

	while (startPos + jumper < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[startPos + jumper];
	
		if (currentToken->type == _OP_SEMICOLON_) {
			break;
		} else if (currentToken->type == _OP_RIGHT_EDGE_BRACKET_) {
			dims += openEdgeBrackets == 0 ? 1 : 0;
			openEdgeBrackets++;
		} else if (currentToken->type == _OP_LEFT_EDGE_BRACKET_) {
			openEdgeBrackets--;
		}

		jumper++;
	}

	return dims;
}

/*
Purpose: Creates a subtree for an array initialization
Return Type: NodeReport => Topnode and How many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing;
		int dim => Dimension of the init
_______________________________
Layout:

[ARRAY_INIT]
	 |
[ARRAY_INIT]
[ARRAY_INIT]
_______________________________
*/
NodeReport PG_create_array_init_tree(TOKEN **tokens, size_t startPos, int dim) {
	unsigned int size = sizeof(char) * sizeof(int);
	char *name = (char*)calloc(dim + 2, size);

	if (name == NULL) {
		FREE_MEMORY();
		printf("NAME ERROR!\n");
		exit(EXIT_FAILURE);
	}

	//Automatic '\0' added
	(void)snprintf(name, size, "d_%i", dim);
	Node *topNode = PG_create_node(name, _ARRAY_ASSIGNMENT_NODE_, (*tokens)[startPos].line, (*tokens)[startPos].tokenStart);
	int jumper = 0;
	int detailsPointer = 0;
	int running = true;
	int argCount = (int)PG_predict_array_init_count(tokens, startPos);
	(void)PG_allocate_node_details(topNode, argCount);

	while (startPos + jumper < TOKEN_LENGTH && running == true) {
		TOKEN *currentToken = &(*tokens)[startPos + jumper];
		
		switch (currentToken->type) {
		case _OP_RIGHT_BRACE_: {
			NodeReport arrInitReport = PG_create_array_init_tree(tokens, startPos + jumper + 1, dim + 1);
			jumper += arrInitReport.tokensToSkip + 1;
			topNode->details[detailsPointer++] = arrInitReport.node;
			break;
		}
		case _OP_LEFT_BRACE_:
			running = false;
			break;
		case _OP_COMMA_:
			if (detailsPointer == 0) {
				TOKEN *prevToken = &(*tokens)[startPos + jumper - 1];
				enum NodeType prevType = PG_get_node_type_by_value(&prevToken->value);
				topNode->details[detailsPointer++] = PG_create_node(prevToken->value, prevType, prevToken->line, prevToken->tokenStart);
			}

			if ((*tokens)[startPos + jumper + 1].type != _OP_RIGHT_BRACE_) {
				TOKEN *nextToken = &(*tokens)[startPos + jumper + 1];
				enum NodeType nextType = PG_get_node_type_by_value(&nextToken->value);
				topNode->details[detailsPointer++] = PG_create_node(nextToken->value, nextType, nextToken->line, nextToken->tokenStart);
			}

			break;
		default: break;
		}

		jumper++;
	}

	if (detailsPointer == 0) {
		TOKEN *token = &(*tokens)[startPos];
		enum NodeType type = PG_get_node_type_by_value(&token->value);
		topNode->details[detailsPointer] = PG_create_node(token->value, type, token->line, token->tokenStart);
		jumper = 1;
	}

	return PG_create_node_report(topNode, jumper);
}

/*
Purpose: Predicts how many params an array init dimension has
Return Type: int => Number of predicted params
Params: TOKEN **tokens => Pointer to tokens;
		size_t startPos => Position from where to start counting
*/
int PG_predict_array_init_count(TOKEN **tokens, size_t startPos) {
	int openBraces = 0;
	int jumper = 0;
	int count = 0;
	int isRunning = true;
	TOKEN *prevToken = &(*tokens)[startPos];

	while (startPos + jumper < TOKEN_LENGTH && isRunning == true) {
		TOKEN *currentToken = &(*tokens)[startPos + jumper];

		switch (currentToken->type) {
		case _OP_RIGHT_BRACE_:
			openBraces++;
			break;
		case _OP_COMMA_:
			if (openBraces == 0) {
				count += count == 0 ? 2 : count > 0 ? 1 : 0;
			}

			break;
		case _OP_LEFT_BRACE_:
			openBraces--;

			if (openBraces < 0) {
				if (prevToken->type != _OP_RIGHT_BRACE_
					&& count == 0) {
					count++;
				}

				isRunning = false;
				continue;
			}

			break;
		case _OP_SEMICOLON_:
			isRunning = false;
			continue;
		default: break;
		}

		jumper++;
		prevToken = currentToken;
	}

	return count;
}

/*
Purpose: Set the individual dimensions into the ´´´node->details[pos]´´´ field
Return Type: int => Tokens to skip
Params: Node *node => Node to set the dimensions in;
		TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where the dimensions start
*/
int PG_add_dimensions_to_var_node(Node *node, TOKEN **tokens, size_t startPos, int offset) {
	size_t jumper = 0;
	size_t currentDetail = offset;

	while (startPos + jumper < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[startPos + jumper];

		if (currentToken->type == _OP_RIGHT_EDGE_BRACKET_) {
			int bounds = PG_get_array_element_size(tokens, startPos + jumper);
			
			if (bounds > 0) {
				NodeReport termReport = PG_create_simple_term_node(tokens, startPos + jumper + 1, bounds);
				node->details[currentDetail++] = termReport.node;
				termReport.node->type = _ARRAY_DIM_NODE_;
				jumper += termReport.tokensToSkip;
			} else {
				node->details[currentDetail++] = PG_create_node("_", _ARRAY_DIM_NODE_, currentToken->line, currentToken->tokenStart);
				jumper++;
			}
		} else if (currentToken->type == _OP_EQUALS_
			|| currentToken->type == _OP_SEMICOLON_) {
			break;
		}

		jumper++;
	}

	return jumper;
}

/*
Purpose: Get the size of an array element
Return Type: int => Size of the array element
Params: TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where to start getting the size
*/
int PG_get_array_element_size(TOKEN **tokens, size_t startPos) {
	int bounds = 0;

	while ((*tokens)[startPos + bounds].type != _OP_LEFT_EDGE_BRACKET_) {
		bounds++;
	}

	return bounds - 1;
}

/*
Purpose: Get the dimension count of an array var
Return Type: int => Dimension count
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start counting
*/
int PG_get_dimension_count(TOKEN **tokens, size_t startPos) {
	int counter = 0;

	for (size_t i = startPos; i < TOKEN_LENGTH; i++) {
		if ((*tokens)[i].type == _OP_RIGHT_EDGE_BRACKET_) {
			counter++;
		} else if ((*tokens)[i].type == _OP_EQUALS_
			|| (*tokens)[i].type == _OP_SEMICOLON_) {
			break;
		}
	}

	return counter;
}

/*
Purpose: Create a subtree for a variable definition
Return Type: NodeReport => Contains the topNode and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

	 [VAR]
   /      \
[MOD]    [VAL]

To the [VAR] appended are the modifier
([MOD]: ´´´node->leftNode´´´) and the value
([VAL]: ´´´´node->rightNode´´´).

[VAR]: Contains the variable name and primitive type if assigned
[MOD]: Contains the var modifier
[VAL]: Value of the variable
_______________________________
*/
NodeReport PG_create_normal_var_tree(TOKEN **tokens, size_t startPos) {
	Node *varNode = PG_create_node(NULL, _VAR_NODE_, 0, 0);
	int skip = 0;
	
	varNode->leftNode = PG_create_modifier_node(&(*tokens)[startPos], &skip);

	//Determine var type
	enum NodeType type = (*tokens)[startPos + skip].type == _KW_VAR_ ? _VAR_NODE_ : _CONST_NODE_;
	varNode->type = type;
	skip++;

	if ((*tokens)[startPos + skip].type == _OP_COLON_) {
		skip += (int)PG_add_varType_definition(tokens, startPos + skip + 1, varNode) + 1;
	}

	varNode->value = PG_get_identifier_by_index(&(*tokens)[startPos + skip]);
	varNode->line = (*tokens)[startPos + skip].line;
	varNode->position = (*tokens)[startPos + skip].tokenStart;
	skip++;
	
	if ((*tokens)[startPos + skip].type == _OP_EQUALS_) {
		size_t bounds = (size_t)PG_get_size_till_next_semicolon(tokens, startPos + skip + 1);
		NodeReport termReport = PG_create_simple_term_node(tokens, startPos + skip + 1, bounds);
		varNode->rightNode = termReport.node;
		skip += termReport.tokensToSkip + 1;
	}

	return PG_create_node_report(varNode, skip);
}

/**
 * @bug FIX AND AND OR OPERATOR BETWEEN CONDITIONS FOR FUNCTIONAL CONDITIONS
 */
/*
Purpose: Generate a subtree for a condition
Return Type: NodeReport => Contains top of the subtree and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where to start constructing
_______________________________
PRECEDENCE of CONDITION operators:

+-----+-----+----+---+---+
|     | AND | OR | ( | ) |
+-----+-----+----+---+---+
| AND |  =  | =  | ( | ) |
+-----+-----+----+---+---+
| OR  |  =  | =  | ( | ) |
+-----+-----+----+---+---+
|  (  |  (  | (  | = | = |
+-----+-----+----+---+---+
|  )  |  )  | )  | = | = |
+-----+-----+----+---+---+
_______________________________
Layout:

   [CHCOND]
   /     \
[COND] [COND]

The conditions can be found in ´´´node->leftNode´´´ and
´´´node->rightNode´´´.

[COND]: Condition
[CHCOND]: Chained condition holding multiple [COND]s
_______________________________
*/
NodeReport PG_create_chained_condition_tree(TOKEN **tokens, const size_t startPos, int inDepth) {
	Node *cache = NULL;
	size_t lastCondStart = startPos;
	size_t skip = 0;
	int hasLogicOperators = (int)PG_contains_logical_operator(tokens, startPos);

	while (skip < TOKEN_LENGTH && hasLogicOperators == true) {
		TOKEN *currentToken = &(*tokens)[startPos + skip];

		switch (currentToken->type) {
		case _OP_RIGHT_BRACKET_: {
			NodeReport rep = PG_create_chained_condition_tree(tokens, startPos + skip + 1, true);
			skip += rep.tokensToSkip;

			if (cache == NULL) {
				cache = rep.node;
			} else {
				if (cache->leftNode == NULL) {
					cache->leftNode = rep.node;
				} else {
					cache->rightNode = rep.node;
				}
			}

			continue;
		}
		case _OP_LEFT_BRACKET_:
		case _OP_SEMICOLON_:
		case _OP_QUESTION_MARK_:
			hasLogicOperators = false;
			continue;
		case _KW_OR_:
		case _KW_AND_: {
			enum NodeType type = currentToken->type == _KW_AND_ ? _AND_NODE_ : _OR_NODE_;
			Node *node = PG_create_node(currentToken->value, type, currentToken->line, currentToken->tokenStart);

			if (cache == NULL) {
				NodeReport leftReport = PG_create_condition_tree(tokens, lastCondStart);
				node->leftNode = leftReport.node;
			} else {
				node->leftNode = cache;
			}

			NodeReport rightReport = {NULL, UNINITIALZED};

			if ((*tokens)[startPos + skip + 1].type == _OP_RIGHT_BRACKET_) {
				rightReport = PG_create_chained_condition_tree(tokens, startPos + skip + 2, true);
			} else {
				rightReport = PG_create_condition_tree(tokens, startPos + skip + 1);
			}

			skip += rightReport.tokensToSkip;
			node->rightNode = rightReport.node;

			cache = node;
			lastCondStart = skip + 1;
			break;
		}
		default: break;
		}

		skip++;
	}

	if (cache == NULL) {
		skip = 0;
		NodeReport rep = {NULL, UNINITIALZED};

		if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_) {
			rep = PG_create_chained_condition_tree(tokens, startPos + 1, true);
		} else {
			rep = PG_create_condition_tree(tokens, startPos);
		}

		cache = rep.node;
		skip = rep.tokensToSkip;
	}

	return PG_create_node_report(cache, skip);
}

int PG_is_logic_operator_bracket(TOKEN **tokens, size_t startPos) {
	int openBrackets = 0;
	
	for (int i = startPos; i < TOKEN_LENGTH; i++) {
		switch ((*tokens)[i].type) {
		case _KW_AND_:
		case _KW_OR_:
			return true;
		case _OP_LEFT_BRACKET_:
			openBrackets--;

			if (openBrackets <= 0) {
				return false;
			}

			break;
		case _OP_RIGHT_BRACKET_:
			openBrackets++;
			break;
		case _OP_SEMICOLON_:
		case _OP_RIGHT_BRACE_:
			return false;
		default: break;
		}
	}

	return false;
}

int PG_contains_logical_operator(TOKEN **tokens, size_t startPos) {
	for (int i = startPos; i < TOKEN_LENGTH; i++) {
		if ((*tokens)[i].type == _KW_AND_
			|| (*tokens)[i].type == _KW_OR_) {
			return true;
		} else if ((*tokens)[i].type == _OP_RIGHT_BRACE_
			|| (*tokens)[i].type == _OP_SEMICOLON_
			|| (*tokens)[i].type == _OP_QUESTION_MARK_) {
			return false;
		}
	}

	return false;
}

/*
Purpose: Creates a condition subtree
Return Type: NodeReport => Contains the subtree and how many tokens to skip
Params: TOKEN **tokens => Point to the tokens array;
		size_t startPos => Position from where to start constructing;
*/
NodeReport PG_create_condition_tree(TOKEN **tokens, size_t startPos) {
	size_t skip = 0;

	while (startPos + skip < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[startPos + skip];

		if ((int)PG_is_condition_operator(currentToken->type) == true) {
			Node *conditionNode = PG_create_node(currentToken->value, PG_get_node_type_by_value(&currentToken->value), currentToken->line, currentToken->tokenStart);
			int leftBounds = (int)PG_get_condition_iden_length(tokens, startPos);
			int rightBounds = (int)PG_get_condition_iden_length(tokens, startPos + skip + 1);

			NodeReport rightTermReport = PG_create_simple_term_node(tokens, startPos + skip + 1, rightBounds);
			NodeReport leftTermReport = leftTermReport = PG_create_simple_term_node(tokens, startPos, leftBounds);

			conditionNode->leftNode = leftTermReport.node;
			conditionNode->rightNode = rightTermReport.node;

			skip += rightBounds + 1;
			return PG_create_node_report(conditionNode, skip);
		} else if ((currentToken->type == _KW_TRUE_
			|| currentToken->type == _KW_FALSE_)
			&& (int)PG_is_condition_operator((*tokens)[startPos + skip + 1].type) == false) {
				Node *boolNode = PG_create_node(currentToken->value, _BOOL_NODE_, currentToken->line, currentToken->tokenStart);
				return PG_create_node_report(boolNode, 1);
			}

		skip++;
	}

	return PG_create_node_report(NULL, 0);
}

int PG_get_condition_iden_length(TOKEN **tokens, size_t startPos) {
	int counter = 0;
	int openBrackets = 0;

	while (startPos + counter < TOKEN_LENGTH) {
		if ((int)PG_is_condition_operator((*tokens)[startPos + counter].type) == true) {
			break;
		} else if ((*tokens)[startPos + counter].type == _OP_SEMICOLON_) {
			break;
		} else if ((*tokens)[startPos + counter].type == _OP_LEFT_BRACKET_) {
			openBrackets--;

			if (openBrackets < 0) {
				break;
			}
		} else if ((*tokens)[startPos + counter].type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		}

		counter++;
	}

	return counter;
}

int PG_is_condition_operator(TOKENTYPES type) {
	switch (type) {
	case _OP_EQUALS_CONDITION_:
	case _OP_GREATER_CONDITION_:
	case _OP_SMALLER_CONDITION_:
	case _OP_GREATER_OR_EQUAL_CONDITION_:
	case _OP_SMALLER_OR_EQUAL_CONDITION_:
	case _OP_NOT_EQUALS_CONDITION_:
	case _KW_AND_:
	case _KW_OR_:
	case _OP_QUESTION_MARK_:
		return true;
	default:
		return false;
	}
}

/*
Purpose: Create a subtree for a class constructor definition
Return Type: NodeReport => Contains top of the subtree and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

[CONSTRUCTOR]
	 |
 [RUNNABLE]

The [CONSTRUCTOR] node only contains a [RUNNABLE]
in the ´´´node->details[position]´´´ field.
_______________________________
*/
NodeReport PG_create_class_constructor_tree(TOKEN **tokens, size_t startPos) {
	int skip = 5;
	TOKEN *token = &(*tokens)[startPos + 3];
	Node *topNode = PG_create_node("CONSTRUCTOR", _CLASS_CONSTRUCTOR_NODE_, token->line, token->tokenStart);
	token = &(*tokens)[startPos + skip];
	//Start (from ´´´this´´´ to ´´´constructor´´´) is SYNTAXANALYSIS
	//not tree generation.
	/*
	> this::constructor(param) {}
	>                  ^
	>         (*tokens)[startPos + 5]
	*/
	if ((*tokens)[startPos + skip].type == _OP_LEFT_BRACKET_) {
		skip += 2;
	} else {
		int arguments = (int)PG_predict_argument_count(tokens, startPos + skip, false);
		(void)PG_allocate_node_details(topNode, arguments);
		skip += PG_add_params_to_node(topNode, tokens, startPos + skip, 0, _PARAM_NODE_) + 2;
	}

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;
	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Create a subtree for a class definition
Return Type: NodeReport => Contains top of the subtree and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

	   [CLASS]
	/     |     \
[MOD]  [INHRTS] [RUNNABLE]
	   [INTFCS]

The [CLASS] node contains the class name
and appended to the node are the [MOD] for
the modifier (´´´´node->leftNode´´´) and
[PARAMS] for the params in the
´´´node->details[position]´´´ field. In
addition to that is the [INTFCS] node.

[MOD]: Modifier of the class
[PARAMS]: Parameters of the class
[RUNNABLE]: Runnable of the class
[INTFCS]: Interfaces bound with the class
_______________________________
*/
NodeReport PG_create_class_tree(TOKEN **tokens, size_t startPos) {
	int skip = 0;
	Node *modNode = NULL;
	TOKEN *token = &(*tokens)[startPos];
	//public class obj => {}
	modNode = PG_create_modifier_node(token, &skip);

	skip++;
	token = &(*tokens)[startPos + skip];
	Node *classNode = PG_create_node(token->value, _CLASS_NODE_, token->line, token->tokenStart);
	classNode->leftNode = modNode;
	skip++;

	if ((*tokens)[startPos + skip].type == _KW_EXTENDS_) {
		(void)PG_allocate_node_details(classNode, 1);
		TOKEN *inTok = &(*tokens)[startPos + skip + 1];
		classNode->details[0] = PG_create_node(inTok->value, _INHERITANCE_NODE_, inTok->line, inTok->tokenStart);
		skip += 2;
	}

	if ((*tokens)[startPos + skip].type == _KW_WITH_) {
		int offset = classNode->detailsCount;
		int arguments = 0;
		
		if ((*tokens)[startPos + skip + 1].type == _IDENTIFIER_
			&& (*tokens)[startPos + skip + 2].type == _OP_CLASS_CREATOR_) {
			arguments = 1;
		} else {
			arguments += (int)PG_predict_argument_count(tokens, startPos + skip, false);
		}

		(void)PG_allocate_node_details(classNode, offset + arguments);
		skip += PG_add_params_to_node(classNode, tokens, startPos + skip + 1, offset, _INTERFACE_NODE_);
	}

	skip += 2;
	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	classNode->rightNode = runnableReport.node;

	return PG_create_node_report(classNode, skip + runnableReport.tokensToSkip);
}

/*
Purpose: Create a subtree for a try statement
Return Type: NodeReport => Contains top of the subtree and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

 [TRY]
	|
[RUNNABLE]

In the [TRY] node contained are the runnable
statements ´´´node->details[position]´´´.
_______________________________
*/
NodeReport PG_create_try_tree(TOKEN **tokens, size_t startPos) {
	/*
	> try {}
	>     ^
	> (*tokens)[startPos + 1]
	*/
	//Rest is checked by the SYNTAXANALYZER for correctness
	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + 2, InBlock);
	Node *tryNode = runnableReport.node;
	tryNode->type = _TRY_NODE_;
	tryNode->value = "TRY";
	return PG_create_node_report(tryNode, runnableReport.tokensToSkip + 2);
}

/*
Purpose: Create a subtree for a catch statement
Return Type: NodeReport => Contains top of the subtree and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens array;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

 [CATCH]
	|
[RUNNABLE]

In the [CATCH] node contained are the runnable
statements ´´´node->details[position]´´´.
_______________________________
*/
NodeReport PG_create_catch_tree(TOKEN **tokens, size_t startPos) {
	/*
	> catch (Exception e) {}
	>                     ^
	>           (*tokens)[startPos + 5]
	*/
	int skip = 2;
	Node *topNode = PG_create_node("CATCH", _CATCH_NODE_, (*tokens)[startPos].line, (*tokens)[startPos].tokenStart);
	NodeReport exceptionType = PG_create_member_access_tree(tokens, startPos + skip, false);
	
	skip += exceptionType.tokensToSkip;
	
	NodeReport exceptionName = PG_create_member_access_tree(tokens, startPos + skip, false);
	
	skip += exceptionName.tokensToSkip + 2;

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	topNode->leftNode = exceptionName.node;
	topNode->leftNode->leftNode = exceptionType.node;
	topNode->rightNode = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(topNode, skip);
}

/*
Purpose: Generate a subtree for an inclusion
Return Type: NodeReport => Contains topNode and tokensToSkip
Params: TOKEN **tokens => Pointer to token array;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

[EXPORT]

In the [EXPORT] node is a indicator and the exported
file can be found in ´´´node->value´´´.
_______________________________
*/
NodeReport PG_create_export_tree(TOKEN **tokens, size_t startPos) {
	//Here: export "name";
	TOKEN *token = &(*tokens)[startPos + 1];
	Node *topNode = PG_create_node(token->value, _EXPORT_NODE_, token->line, token->tokenStart);
	return PG_create_node_report(topNode, 3);
}

/*
Purpose: Generate a subtree for an inclusion
Return Type: NodeReport => Contains topNode and tokensToSkip
Params: TOKEN **tokens => Pointer to token array;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

[INCLUDE]

In the [INCLUDE] node is a indicator and the included
file can be found in ´´´node->value´´´.
_______________________________
*/
NodeReport PG_create_include_tree(TOKEN **tokens, size_t startPos) {
	NodeReport includeRep = PG_create_member_access_tree(tokens, startPos + 1, false);
	includeRep.node->value = "INCLUDE";
	includeRep.node->type = _INCLUDE_NODE_;
	includeRep.node->line = (*tokens)[startPos].line;
	includeRep.node->position = (*tokens)[startPos].tokenStart;
	return PG_create_node_report(includeRep.node, includeRep.tokensToSkip + 2);
}

/*
Purpose: Generate a subtree for an enum
Return Type: NodeReport => Contains topNode and how many tokens to skip
Params: TOKEN **tokens => Pointer to the array of tokens;
		size_t startPos => Position from where to start constructing
_______________________________
Layout:

   [ENUM]
	 |
[ENUMERATOR]
		   \
		 [VALUE]

To the [ENUM] node appended are the [ENUMERATOR] nodes,
int the ´´´node->details[position]´´´ field. The
´´´enumerator->rightNode´´´ field contains the value of
the enumerator (If not set it assigns it automatically).

[ENUM]: Contains the enum's name
[ENUMERATOR]: A single enumerator element
[VALUE]: Value of a enumerator element
_______________________________
*/
NodeReport PG_create_enum_tree(TOKEN **tokens, size_t startPos) {
	/*
	> enum exampleEnum = {...}
	>      ^^^^^^^^^^^
	> (*tokens)[startPos + 1]
	*/
	TOKEN *token = &(*tokens)[startPos + 1];
	Node *enumNode = PG_create_node(token->value, _ENUM_NODE_, token->line, token->tokenStart);
	int argumentCount = (int)PG_predict_enumerator_count(tokens, startPos + 2);
	(void)PG_allocate_node_details(enumNode, argumentCount);
	
	argumentCount = 0;
	size_t skip = 2;
	int currentEnumeratorValue = 0;

	while (startPos + skip < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[startPos + skip];

		if (currentToken->type == _OP_LEFT_BRACE_) {
			skip++;
			break;
		}

		if (argumentCount > enumNode->detailsCount) {
			FREE_MEMORY();
			printf("SIZE (enum) %li!\n", enumNode->detailsCount);
			exit(EXIT_FAILURE);
		}

		token = &(*tokens)[startPos + skip + 1];
		Node *enumeratorNode = PG_create_node(token->value, _ENUMERATOR_NODE_, token->line, token->tokenStart);

		if (currentToken->type == _OP_COMMA_
			|| currentToken->type == _OP_RIGHT_BRACE_) {
			/*
			> Looking for: enumerator : [NUMBER]
			>                         ^
			*/
			token = &(*tokens)[startPos + skip + 3];

			if ((*tokens)[startPos + skip + 2].type == _OP_COLON_) {
				currentEnumeratorValue = (int)atoi(token->value);
				skip++;
			}

			//Size for long
			char *value = (char*)calloc(24, sizeof(char));

			if (value == NULL) {
				FREE_MEMORY();
				printf("VALUE = NULL! (enum)\n");
				exit(EXIT_FAILURE);
			}

			(void)snprintf(value, 24, "%d", currentEnumeratorValue++);
			enumeratorNode->rightNode = PG_create_node(value, _VALUE_NODE_, token->line, token->tokenStart);
			enumNode->details[argumentCount++] = enumeratorNode;
			skip++;
		}

		skip++;
	}
	
	return PG_create_node_report(enumNode, skip);
}

/*
Purpose: Predict how many enumerators an enum has
Return Type: int => Number of predicted enumerators
Params: TOKEN **tokens => Pointer to the token array;
		size_t startPos => Position from where to start counting enumerators
*/
int PG_predict_enumerator_count(TOKEN **tokens, size_t starPos) {
	int enumCount = 1;
	int jumper = 0;

	while (starPos + jumper < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[starPos + jumper];

		if (currentToken->type == _OP_LEFT_BRACE_) {
			break;
		} else if (currentToken->type == _OP_COMMA_) {
			enumCount++;
		}

		jumper++;
	}

	return enumCount;
}

/*
Purpose: Generate a subtree for a function definition
Return Type: NodeReport => Contains the root node of the subtree and the number of tokens to skip
Params: TOKEN **tokens => Pointer to the array of tokens;
		size_t startPos => The position from where to start constructing
_______________________________
Layout:

	 [FUNCTION]
   /      |     \
[MOD]  [PARAMS]  [RET]
	  [RUNNABLE]

To the [FUNCTION] node appended are the
function name ´´´node->leftNode´´´ and
the parameters ´´´node->rightNode´´´.
In addition to that the modifier and
return type.

[FUNCTION]: Node the holds the function and name
[PARAMS]: Parameters, that get invoked
[MOD]: Modifier
[RET]: Return type
[RUNNABLE]: All processes, that happen in the function
_______________________________
*/
NodeReport PG_create_function_tree(TOKEN **tokens, size_t startPos) {
	int skip = 1; //Skip the "function" keyword
	Node *modNode = NULL;
	Node *functionNode = PG_create_node("FNC", _FUNCTION_NODE_, 0, 0);
	TOKEN *token = &(*tokens)[startPos];

	/*
	> global function:int add(number1, number2)
	> ^^^^^^         ^^^^ ^^^ ^^^^^^^^^^^^^^^^
	> [POS1]        [POS2] |       [POS4]
	>                    [POS3]
	>
	> [POS1]: (*tokens)[startPos] (skip gets increased by 1)
	> [POS2]: (*tokens)[startPos + skip + 2] (skip gets increased by 2)
	> [POS3]: (*tokens)[startPos + skip + 1]
	> [POS4]: (*tokens)[startPos + skip + 2]
	*/

	modNode = PG_create_modifier_node(token, &skip);
	//No NULL check needed, if leftNode or rightNode == NULL nothing changes
	token = &(*tokens)[startPos + skip];
	functionNode->value = token->value;
	functionNode->line = token->line;
	functionNode->position = token->tokenStart;
	functionNode->leftNode = modNode;
	skip += 2;

	int argumentCount = (int)PG_predict_argument_count(tokens, startPos + skip, true);
	(void)PG_allocate_node_details(functionNode, argumentCount + 2);
	skip += (size_t)PG_add_params_to_node(functionNode, tokens, startPos + skip, 1, _NULL_) + 1;

	if ((*tokens)[startPos + skip].type == _OP_CLASS_ACCESSOR_) {
		skip += (int)PG_add_varType_definition(tokens, startPos + skip + 1, functionNode) + 1;
	}

	skip++;

	NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, InBlock);
	functionNode->details[functionNode->detailsCount - 1] = runnableReport.node;
	skip += runnableReport.tokensToSkip;

	return PG_create_node_report(functionNode, skip);
}

/*
Purpose: Generate a subtree for a function call
Return Type: NodeReport => Contains the root node of the subtree and the number of tokens to skip
Params: TOKEN **tokens => Pointer to the array of tokens;
		size_t startPos => The index of the first token of the function call
*/
NodeReport PG_create_function_call_tree(TOKEN **tokens, size_t startPos) {
	TOKEN *token = &(*tokens)[startPos];
	char *name = PG_get_identifier_by_index(token);
	Node *functionCallNode = PG_create_node(name, _FUNCTION_CALL_NODE_, token->line, token->tokenStart);
	int argumentSize = (int)PG_predict_argument_count(tokens, startPos + 1, true);
	(void)PG_allocate_node_details(functionCallNode, argumentSize);
	size_t paramSize = (size_t)PG_add_params_to_node(functionCallNode, tokens, startPos + 2, 0, _NULL_);
	paramSize = paramSize > 0 ? paramSize - 1 : paramSize;
	return PG_create_node_report(functionCallNode, paramSize + 3);
}

/*
Purpose: Adds the parameters to the function call node
Return Type: size_t => The number of tokens to skip
Params: Node *node => The root node of the function call subtree;
		TOKEN **tokens => Pointer to the array of tokens;
		size_t startPos => The index of the first token of the function call;
		int addStart => Index at which to start writing;
		enum NodeType stdType => StandartType;
NOTICE: THE PARENTNODE HAS TO HAVE ALLOCATED SPACE OR ELSE THE PROGRAM
		WILL CRASH!!!
_______________________________
Layout:

[FUNCTION_CALL]
	   |
	[PARAM]
	[PARAM]       

To the [FUNCTION_CALL] node appended are the
params. The params are appended at ´´´node->details[pos]´´´.

[FUNCTION_CALL]: Node the holds the function call name
[PARAM]: Parameter, that gets invoked
_______________________________
*/
size_t PG_add_params_to_node(Node *node, TOKEN **tokens, size_t startPos, int addStart, enum NodeType stdType) {
	int detailsPointer = addStart;
	int skip = 0;

	for (size_t i = startPos; i < TOKEN_LENGTH; skip = i - startPos, i++) {
		TOKEN *currentToken = &(*tokens)[i];

		if (currentToken->type != _OP_LEFT_BRACKET_
			&& currentToken->type != _OP_RIGHT_BRACE_
			&& currentToken->type != _OP_CLASS_CREATOR_
			&& currentToken->type != _KW_WITH_
			&& currentToken->type != _KW_EXTENDS_
			&& currentToken->type != _OP_CLASS_ACCESSOR_) {
			//Check if the param is going to be out of the allocated space
			if (detailsPointer == node->detailsCount) {
				break;
			}

			NodeReport report = {NULL, -1};

			if ((int)predict_is_conditional_assignment_type(tokens, i, TOKEN_LENGTH) == true) {
				report = PG_create_condition_assignment_tree(tokens, i);
			} else {
				int bounds = (int)PG_get_bound_of_single_param(tokens, i);
				report = PG_create_simple_term_node(tokens, i, bounds);

				if ((*tokens)[i + bounds].type == _OP_COLON_) {
					i += (int)PG_add_varType_definition(tokens, i + bounds + 1, report.node) + 1;
				}
			}

			node->details[detailsPointer] = report.node;
			node->details[detailsPointer++]->type = stdType == _NULL_ ? report.node->type : stdType;
			i += report.tokensToSkip;
		} else {
			break;
		}
	}

	return skip;
}

/*
Purpose: Get the boundaries of a single parameter
Return Type: int => Boundaries of the single parameter
Params: TOKEN **tokens => Pointer to the array of tokens;
		size_t startPos => The index of the first token of the single parameter
*/
int PG_get_bound_of_single_param(TOKEN **tokens, size_t startPos) {
	int bound = 0;
	int openBrackets = 0;

	for (size_t i = startPos; i < TOKEN_LENGTH; i++) {
		TOKEN *token = &(*tokens)[i];

		if ((token->type == _OP_COMMA_ || token->type == _OP_CLASS_CREATOR_
			|| token->type == _OP_COLON_)
			&& openBrackets <= 0) {
			break;
		} else if (token->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		} else if (token->type == _OP_LEFT_BRACKET_) {
			openBrackets--;

			if (openBrackets < 0) {
				break;
			}
		}

		bound++;
	}

	return bound;
}

/**
 * <p>
 * Predicts the number of arguments in a sequence of tokens, seperated by ','.
 * </p>
 * 
 * @return The number of arguments
 * 
 * @param **tokens					Pointer to the token array
 * @param startPos					Position from where to start counting
 * @param withPredefinedBrackets	Flag for whether the argument counter should stop at openBrackets = 0 or not
 */
int PG_predict_argument_count(TOKEN **tokens, size_t startPos, int withPredefinedBrackets) {
	int primitiveCheck = (int)PG_predict_primitive_param_count(tokens, startPos);

	if (primitiveCheck != -1) {
		return primitiveCheck;
	}

	int count = 0;
	int openBrackets = withPredefinedBrackets;

	for (size_t i = startPos; i < TOKEN_LENGTH; i++) {
		TOKEN *token = &(*tokens)[i];
		
		if (token->type == _OP_COMMA_) {
			count += count == 0 ? 2 : 1;
		} else if (token->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		} else if (token->type == _OP_LEFT_BRACKET_) {
			openBrackets--;

			if ((*tokens)[i - 1].type != _OP_RIGHT_BRACKET_
				&& count == 0) {
				count++;
			}

			if (openBrackets <= 0) {
				break;
			}
		} else if (token->type == _OP_RIGHT_BRACE_
			|| token->type == _OP_CLASS_CREATOR_) {
			break;
		}
	}

	return count;
}

/**
 * <p>
 * Predicts the paramcount based on the most primitive definition
 * way.
 * </p>
 * 
 * @returns
 * <ul>
 * <li> >= 0 - Parameter count
 * <li> -1 - Not a primitive param definition
 * </ul>
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position at which to check
 */
int PG_predict_primitive_param_count(TOKEN **tokens, size_t startPos) {
	if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_
		&& (*tokens)[startPos + 1].type == _OP_LEFT_BRACKET_) {
		return 0;
	} else if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_
		&& ((*tokens)[startPos + 1].type == _IDENTIFIER_
		|| (*tokens)[startPos + 1].type == _NUMBER_)
		&& (*tokens)[startPos + 2].type == _OP_LEFT_BRACKET_) {
		return 1;
	}

	return -1;
}

/**
 * <p>
 * Get the amount of tokens to skip until the next semicolon is met.
 * </p>
 * 
 * @returns The amount of tokens to skip
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start counting
 */
size_t PG_get_size_till_next_semicolon(TOKEN **tokens, size_t startPos) {
	size_t size = 0;

	while ((*tokens)[startPos + size].type != _OP_SEMICOLON_) {
		size++;
	}

	return size;
}

/**
 * <p>
 * Allocates space for the details field of the passed Node.
 * </p>
 * 
 * <p><strong>Note:</strong>
 * If the details field is not NULL the function resizes instead.
 * </p>
 * 
 * @param *node     Node to allocate the details to
 * @param size      Number of entries to allocate
 */
void PG_allocate_node_details(Node *node, size_t size) {
	if (node == NULL) {
		FREE_MEMORY();
		printf("NODE NULL\n");
		exit(EXIT_FAILURE);
	}

	Node **temp = NULL;
	int resize = node->details == NULL ? false : true;

	if (resize == false) {
		temp = (Node**)calloc(size, sizeof(Node*));
	} else {
		temp = (Node**)realloc(node->details, sizeof(Node) * size);

		for (int i = node->detailsCount; i < size; i++) {
			node->details[i] = NULL;
		}
	}

	if (temp == NULL) {
		free(node->details);
		FREE_MEMORY();
		printf("DETAILS ERROR!\n");
		exit(EXIT_FAILURE);
	}
	
	node->details = temp;
	node->detailsCount = size;
}

/*
Purpose: Print out a tree base on the nodes
Return Type: void
Params: Node *topNode => Pointer to the root node of the tree;
		int depth => The depth of the tree;
		int pos => The position of the node (0 = Center, 1 = Left, 2 = Right)
*/
void PG_print_from_top_node(Node *topNode, int depth, int pos) {
	if (topNode == NULL || topNode->value == NULL) {
		return;
	}

	for (int i = 0; i < depth; ++i) {
		if (i + 1 == depth) {
			(void)printf("+-- ");
		} else {
			(void)printf("|   ");
		}
	}

	if (pos == 0) {
		(void)printf("C: %s -> %i\n", topNode->value, topNode->type);
	} else if (pos == 1) {
		(void)printf("L: %s -> %i\n", topNode->value, topNode->type);
	} else {
		(void)printf("R: %s -> %i\n", topNode->value, topNode->type);
	}

	for (int i = 0; i < topNode->detailsCount; i++) {
		if (topNode->details[i] != NULL) {
			for (int n = 0; n < depth + 1; n++) {
				if (n + 1 == depth + 1) {
					(void)printf("+-- ");
				} else {
					(void)printf("|   ");
				}
			}

			(void)printf("(%s) detail: %s -> %i\n", topNode->value, topNode->details[i]->value, topNode->details[i]->type);
			(void)PG_print_from_top_node(topNode->details[i]->leftNode, depth + 2, 1);
			(void)PG_print_from_top_node(topNode->details[i]->rightNode, depth + 2, 2);

			for (int n = 0; n < topNode->details[i]->detailsCount; n++) {
				(void)PG_print_from_top_node(topNode->details[i]->details[n], depth + 2, 0);
			}
		} else {
			(void)printf("(%s) detail: NULL -> NULL\n", topNode->value);
		}
	}

	(void)PG_print_from_top_node(topNode->leftNode, depth + 1, 1);
	(void)PG_print_from_top_node(topNode->rightNode, depth + 1, 2);
}

/**
 * <p>
 * Get the node type by the value of the node.
 * </p>
 * 
 * @returns The node type of the NodeType enum
 * 
 * @param **value   Pointer to the value
 */
enum NodeType PG_get_node_type_by_value(char **value) {
	if ((*value)[0] == '"') {
		return _STRING_NODE_;
	} else if ((*value)[0] == '\'') {
		return _CHAR_ARRAY_NODE_;
	} else if ((int)is_digit((*value)[0]) == true) {
		for (size_t i = 0; (*value)[i] != '\0'; i++) {
			if ((*value)[i] == '.') {
				return _FLOAT_NODE_;
			}
		}

		return _NUMBER_NODE_;
	} else if ((*value)[0] == '*') {
		if ((*value)[1] == '\0') {
			return _MULTIPLY_NODE_;
		}
		
		return _POINTER_NODE_;
	} else if ((*value)[0] == '&') {
		return _REFERENCE_NODE_;
	} else if ((*value)[0] == '+') {
		return _PLUS_NODE_;
	} else if ((*value)[0] == '-') {
		return _MINUS_NODE_;
	} else if ((*value)[0] == '/') {
		return _DIVIDE_NODE_;
	} else if ((*value)[0] == '%') {
		return _MODULO_NODE_;
	} else if ((int)strcmp((*value), "true") == 0
		|| (int)strcmp((*value), "false") == 0) {
		return _BOOL_NODE_;
	} else if ((int)strcmp((*value), "null") == 0) {
		return _NULL_NODE_;
	} else if ((int)strcmp((*value), "==") == 0) {
		return _EQUALS_CONDITION_NODE_;
	} else if ((int)strcmp((*value), "!=") == 0) {
		return _NOT_EQUALS_CONDITION_NODE_;
	} else if ((int)strcmp((*value), "<=") == 0) {
		return _SMALLER_OR_EQUAL_CONDITION_NODE_;
	} else if ((int)strcmp((*value), ">=") == 0) {
		return _GREATER_OR_EQUAL_CONDITION_NODE_;
	} else if ((int)strcmp((*value), "<") == 0) {
		return _SMALLER_CONDITION_NODE_;
	} else if ((int)strcmp((*value), ">") == 0) {
		return _GREATER_CONDITION_NODE_;
	} else if ((int)strcmp((*value), "this") == 0) {
		return _THIS_NODE_;
	} else {
		for (size_t i = 0; (*value)[i] != '\0'; i++) {
			if ((*value)[i] == '-'
				&& (*value)[i + 1] == '>') {
				return _CLASS_ACCESS_NODE_;
			} else if ((*value)[i] == '[') {
				return _ARRAY_NODE_;
			}
		}
	}

	return _IDEN_NODE_;
}

/**
 * <p>
 * This function creates a subtree for simple terms.
 * </p>
 * 
 * <p>
 * A simple term is defined as a sequence of tokens, that contain
 * an arithmetic symbol or a standalone token.
 * </p>
 * 
 * <p><strong>PRECEDENCE of term OPERATORS:</strong>
 * +---+---+---+---+---+---+---+---+
 * |   | * | / | % | + | - | ( | ) |
 * +---+---+---+---+---+---+---+---+
 * | * | = | = | = | * | * | ( | ) |
 * +---+---+---+---+---+---+---+---+
 * | / | = | = | = | / | / | ( | ) |
 * +---+---+---+---+---+---+---+---+
 * | % | = | = | = | % | % | ( | ) |
 * +---+---+---+---+---+---+---+---+
 * | + | * | / | % | = | = | ( | ) |
 * +---+---+---+---+---+---+---+---+
 * | - | * | / | % | = | = | ( | ) |
 * +---+---+---+---+---+---+---+---+
 * | ( | ( | ( | ( | ( | ( | = | = |
 * +---+---+---+---+---+---+---+---+
 * | ) | ) | ) | ) | ) | ) | = | = |
 * +---+---+---+---+---+---+---+---+
 * </p>
 * 
 * <p><strong>Layout:</strong>
 * ```
 * Layout:
 * 
 *      [OP]
 *    /      \
 * [IDEN]  [IDEN]
 * 
 * After [OP] the [IDEN] is either on the ´´´node->leftNode´´´ or
 * ´´´node->rightNode´´´.
 * 
 * [OP]: All operators like: '+', '-', '*', '/', '%'
 * [IDEN]: Can be a number, function call, a string or an identifier
 * ```
 * </p>
 * 
 * @returns NodeReport with the topNode and how many tokens got processed (the given bounds param)
 * 
 * @param **tokens      Pointer to the token array
 * @param startPos      Position at which the term starts
 * @param boundaries    Bounds of the term
 */

NodeReport PG_create_simple_term_node(TOKEN **tokens, size_t startPos, size_t boundaries) {
	Node *cache = NULL;
	size_t lastIdenPos = UNINITIALZED;
	int length = startPos + boundaries;
	int isCalc = (PG_predict_member_access(tokens, startPos, NONE) == true || boundaries == 1) ? false : true;

	for (size_t i = startPos; i < length && isCalc == true; i++) {
		TOKEN *currentToken = &(*tokens)[i];
		
		if (currentToken->type == __EOF__) {
			break;
		}

		switch ((*tokens)[i].type) {
		case _OP_RIGHT_BRACKET_: {
			int bounds = PG_determine_bounds_for_capsulated_term(tokens, i);
			NodeReport rep = PG_create_simple_term_node(tokens, i + 1, bounds);

			if (cache == NULL) {
				cache = rep.node;
			} else {
				cache->rightNode = rep.node;
			}

			i += rep.tokensToSkip;
			break;
		}
		case _OP_PLUS_:
		case _OP_MINUS_: {
			Node *node = PG_create_node(currentToken->value, PG_get_node_type_by_value(&currentToken->value), currentToken->line, currentToken->tokenStart);
			node->leftNode = cache == NULL ? PG_create_member_access_tree(tokens, lastIdenPos, true).node : cache;

			NodeReport rRep = {NULL, UNINITIALZED};

			if (((int)PG_is_next_operator_multiply_divide_or_modulo(tokens, i + 1)) == false) {
				rRep = PG_assign_processed_node_to_node(tokens, i + 1, true);
			} else {
				int bounds = (int)PG_forward_till_plus_or_minus(tokens, i + 1);
				rRep = PG_create_simple_term_node(tokens, i + 1, bounds);
			}

			node->rightNode = rRep.node;
			i += rRep.tokensToSkip;
			cache = node;
			lastIdenPos = UNINITIALZED;
			break;
		}
		case _OP_DIVIDE_:
		case _OP_MULTIPLY_:
		case _OP_MODULU_: {
			Node *node = PG_create_node(currentToken->value, PG_get_node_type_by_value(&currentToken->value), currentToken->line, currentToken->tokenStart);
			node->leftNode = cache == NULL ? PG_assign_processed_node_to_node(tokens, lastIdenPos, true).node : cache;

			NodeReport rRep = PG_assign_processed_node_to_node(tokens, i + 1, true);
			node->rightNode = rRep.node;
			i += rRep.tokensToSkip;
			cache = node;
			lastIdenPos = UNINITIALZED;
			break;
		}
		default:
			lastIdenPos = lastIdenPos == UNINITIALZED ? i : lastIdenPos;
			break;
		}
	}

	if (cache == NULL) {
		int useOptionalTyping = boundaries < 3 ? false : true;
		NodeReport assignRep = PG_assign_processed_node_to_node(tokens, startPos, useOptionalTyping);
		cache = assignRep.node;
	}

	return PG_create_node_report(cache, boundaries);
}

/**
 * <p>
 * Get the tokens to move until the next '+' or '-' (or EOF).
 * </p>
 * 
 * @returns The amount of tokens to move
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start moving
 */
int PG_forward_till_plus_or_minus(TOKEN **tokens, size_t startPos) {
	int skip = 0;
	int openBrackets = 0;

	while (startPos + skip < TOKEN_LENGTH) {
		TOKEN *tok = &(*tokens)[startPos + skip];

		if (tok->type == _OP_PLUS_
			|| tok->type == _OP_MINUS_) {
			if (openBrackets != 0) {
				skip++;
				continue;
			}

			break;
		} else if (tok->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		} else if (tok->type == _OP_LEFT_BRACKET_) {
			openBrackets--;
		} else if (tok->type == _OP_SEMICOLON_
			|| tok->type == _OP_RIGHT_BRACE_) {
			break;
		}
	
		skip++;
	}

	return skip;
}

/*
Purpose: Assign the correct simple term node to the given node
Return Type: NodeReport => Contains the top node and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
		size_t startPos => Position from where to start processing;
		int useOptionalTyping => Flag for whether the optional typing is activated or not
*/
NodeReport PG_assign_processed_node_to_node(TOKEN **tokens, size_t startPos, int useOptionalTyping) {
	NodeReport report = {NULL, UNINITIALZED};
	TOKEN *startTok = &(*tokens)[startPos];

	if (startTok->type == _OP_RIGHT_BRACKET_) {
		size_t bounds = (size_t)PG_determine_bounds_for_capsulated_term(tokens, startPos);
		report = PG_create_simple_term_node(tokens, startPos, bounds + 1);
	} else if (startTok->type == _STRING_
		|| startTok->type == _CHARACTER_ARRAY_) {
		Node *strNode = PG_create_node(startTok->value, _STRING_NODE_, startTok->line, startTok->tokenStart);
		return PG_create_node_report(strNode, 2);
	} else if (startTok->type == _KW_TRUE_
		|| startTok->type == _KW_FALSE_) {
		Node *boolNode = PG_create_node(startTok->value, _BOOL_NODE_, startTok->line, startTok->tokenStart);
		return PG_create_node_report(boolNode, 1);
	} else if ((int)PG_predict_increment_or_decrement_assignment(tokens, startPos) == true) {
		return PG_create_increment_decrement_tree(tokens, startPos);
	} else {
		report = PG_create_member_access_tree(tokens, startPos, useOptionalTyping);
	}

	return report;
}

/**
 * <p>
 * Creates an array access tree.
 * </p>
 * 
 * @returns A NodeReport with the topNode and tokens to skip
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position where the array access starts
 */
NodeReport PG_create_array_access_tree(TOKEN **tokens, size_t startPos) {
	Node *topNode = NULL;
	Node *prevDimNode = NULL;
	int skip = 0;
	
	while ((*tokens)[startPos + skip].type == _OP_RIGHT_EDGE_BRACKET_
		&& startPos + skip < TOKEN_LENGTH) {
		TOKEN *tok = &(*tokens)[startPos + skip];
		Node *arrAccNode = PG_create_node("ARR_ACC", _ARRAY_ACCESS_NODE_, tok->line, tok->tokenStart);
		NodeReport rep = {NULL, 0};

		if ((int)PG_predict_increment_or_decrement_assignment(tokens, startPos + skip + 1) == true) {
			rep = PG_create_increment_decrement_tree(tokens, startPos + skip + 1);
		} else {
			int termBounds = (int)PG_get_term_bounds(tokens, startPos + skip + 1);
			rep = PG_create_simple_term_node(tokens, startPos + skip + 1, termBounds);
		}

		arrAccNode->leftNode = rep.node;

		if (topNode == NULL) {
			topNode = arrAccNode;
		} else {
			prevDimNode->rightNode = arrAccNode;
		}

		prevDimNode = arrAccNode;
		skip += rep.tokensToSkip + 2;
	}

	return PG_create_node_report(topNode, skip);
}

/**
 * <p>
 * Get the index of the last operator in the token array.
 * </p>
 * 
 * @returns The amount of tokens to go back until the next operator
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start goinf back
 */
size_t PG_go_backwards_till_operator(TOKEN **tokens, size_t startPos) {
	for (size_t i = 0; startPos - i > 0; i++) {
		if ((int)PG_is_operator(&(*tokens)[startPos - i]) == true) {
			return i - 1;
		} else if (startPos - (i + 1) == 0) {
			return i + 1;
		}
	}

	return 0;
}

/**
 * <p>
 * Determines the bounds of a capsulated term.
 * </p>
 * 
 * <p>
 * A capsulated term is a term, that is inside another term.
 * </p>
 * 
 * @return The size of the capsulated term
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start determining to bounds
 */
int PG_determine_bounds_for_capsulated_term(TOKEN **tokens, size_t startPos) {
	size_t bounds = 0;
	int openBrackets = 0;

	while ((*tokens)[startPos + bounds].type != __EOF__) {
		TOKEN *token = &(*tokens)[startPos + bounds];

		if (token->type == _OP_LEFT_BRACKET_) {
			openBrackets--;
		} else if (token->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		}

		if (openBrackets == 0) {
			break;
		}

		bounds++;
	}

	return bounds;
}

/**
 * <p>
 * Check if the next operator in a term is a '*', '/' or '%' operator.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Next OP is a '*', '/' or '%'
 * <li>false - Next OP is not a '*', '/' or '%'
 * </ul>
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start checking
 */
int PG_is_next_operator_multiply_divide_or_modulo(TOKEN **tokens, size_t startPos) {
	int jumper = 0;
	int openBrackets = 0;
	
	while (startPos + jumper < TOKEN_LENGTH) {
		switch ((*tokens)[startPos + jumper].type) {
		case _OP_PLUS_:
		case _OP_MINUS_:
		case _OP_COMMA_:
			if (openBrackets != 0) {
				jumper++;
				continue;
			}

			return false;
		case _OP_MULTIPLY_:
		case _OP_DIVIDE_:
		case _OP_MODULU_:
			if (openBrackets != 0) {
				jumper++;
				continue;
			}

			return true;
		case _OP_LEFT_BRACKET_:
			openBrackets--;
			break;
		case _OP_RIGHT_BRACKET_:
			openBrackets++;
			break;
		default:
			break;
		}

		jumper++;
	}

	return false;
}

/**
 * <p>
 * Checks if the identifier at the position is a member access
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - The next identifier is a member access
 * <li>false - The next identifier is not a member access
 * </ul>
 * 
 * @param **tokens  Pointer to the token array
 * @param startPos  Position from where to start predicting
*/
int PG_is_next_iden_a_member_access(TOKEN **tokens, size_t startPos) {
	int openEdgeBrackets = 0;

	for (size_t i = startPos; i < TOKEN_LENGTH; i++) {
		TOKEN *currentToken = &(*tokens)[i];

		if (currentToken->type == _OP_LEFT_EDGE_BRACKET_) {
			openEdgeBrackets--;
		} else if (currentToken->type == _OP_RIGHT_EDGE_BRACKET_) {
			openEdgeBrackets++;
		}

		if (openEdgeBrackets != 0) {
			continue;
		}

		if ((int)PG_is_operator(currentToken) == true) {
			if (currentToken->type == _OP_DOT_
				|| currentToken->type == _OP_CLASS_ACCESSOR_) {
				return true;
			} else if (currentToken->type == _OP_RIGHT_BRACKET_
				|| currentToken->type == _OP_LEFT_BRACKET_) {
				continue;
			} else {
				return false;
			}
		}
	}

	return false;
}

/**
 * <p>
 * Creates a subtree for member or class accesses.
 * </p>
 * 
 * @returns NodeReport with the topNode and tokens to skip
 * 
 * @param **tokens              Pointer to the tokens array
 * @param startPos              Position from where to start the construction
 * @param useOptionalTyping     Flag for whether optional typing should be used or not
 */
NodeReport PG_create_member_access_tree(TOKEN **tokens, size_t startPos, int useOptionalTyping) {
	Node *topNode = NULL;
	Node *cache = NULL;
	size_t skip = 0;
	int openBrackets = 0;
	int openEdgeBrackets = 0;
	int isMemAcc = (int)PG_is_member_access(tokens, startPos);

	while (startPos + skip < TOKEN_LENGTH && isMemAcc == true) {
		TOKEN *currentToken = &(*tokens)[startPos + skip];

		if (useOptionalTyping == false && currentToken->type == _OP_COLON_) {
			break;
		}

		if (currentToken->type == _OP_DOT_
			|| currentToken->type == _OP_CLASS_ACCESSOR_) {
			enum NodeType type = currentToken->type == _OP_DOT_ ? _MEMBER_ACCESS_NODE_ : _CLASS_ACCESS_NODE_;
			Node *tempNode = PG_create_node(currentToken->value, type, currentToken->line, currentToken->tokenStart);
			NodeReport val = {NULL, -1};

			if (topNode == NULL) {
				val = PG_get_member_access_side_node_tree(tokens, startPos + skip, LEFT, useOptionalTyping);
				topNode = tempNode;
			} else {
				val = PG_get_member_access_side_node_tree(tokens, startPos + skip, RIGHT, useOptionalTyping);
				skip += val.tokensToSkip;
				cache->rightNode = tempNode;
			}

			tempNode->leftNode = val.node;
			cache = tempNode;
			continue;
		} else if ((int)PG_is_operator(currentToken) == true) {
			if (topNode == NULL) {
				int handledBrackets = (int)PG_handle_member_access_brackets(currentToken, &openBrackets, &openEdgeBrackets);
				skip++;

				if (handledBrackets == -1) {
					break;
				}
			}

			break;
		} else if (currentToken->type == _IDENTIFIER_
			&& (*tokens)[startPos + skip - 1].type == _IDENTIFIER_) {
			break;
		}

		skip++;
	}

	if (topNode == NULL) {
		NodeReport rVal = PG_get_member_access_side_node_tree(tokens, startPos, STAY, useOptionalTyping);
		topNode = rVal.node;
		skip = rVal.tokensToSkip;
	} else {
		topNode->type = _MEM_CLASS_ACC_NODE_;
		topNode->value = "MEMCLASSACC";
	}

	return PG_create_node_report(topNode, skip);
}

int PG_is_member_access(TOKEN **tokens, size_t startPos) {
	int openBrackets = 0;
	int openEdgeBrackets = 0;
	int skip = 0;

	while (startPos + skip < TOKEN_LENGTH) {
		TOKEN *currentToken = &(*tokens)[startPos + skip];

		if (currentToken->type == _OP_LEFT_BRACKET_) {
			openBrackets--;
		} else if (currentToken->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		} else if (currentToken->type == _OP_LEFT_EDGE_BRACKET_) {
			openEdgeBrackets--;
		} else if (currentToken->type == _OP_RIGHT_EDGE_BRACKET_) {
			openEdgeBrackets++;
		} else if (currentToken->type == _OP_DOT_
			|| currentToken->type == _OP_CLASS_ACCESSOR_) {
			if (openBrackets == 0 && openEdgeBrackets == 0) {
				return true;
			}
		} else if ((int)is_end_indicator(currentToken) == true) {
			return false;
		}

		skip++;
	}

	return false;
}

/**
 * <p>
 * This function limits the boundaries of a member access tree.
 * </p>
 * 
 * <p>
 * It essentially checks if the current token is an operator or not
 * and based on that calculates the current state.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>0 - Nothing to do
 * <li>-1 - Break the member access loop
 * </ul>
 * 
 * @param *currentToken         Current token in the tree build process
 * @param *openBrackets         Pointer to the openBrackets counter
 * @param *openEdgeBrackets     Pointer to the openEdgeBrackets counter
 */
int PG_handle_member_access_brackets(TOKEN *currentToken, int *openBrackets, int *openEdgeBrackets) {
	int action = 0;

	switch (currentToken->type) {
	case _OP_LEFT_BRACKET_:
	case _OP_RIGHT_BRACKET_:
		action = currentToken->type == _OP_LEFT_BRACKET_ ? -1 : 1;
		*openBrackets += action;
		break;
	case _OP_LEFT_EDGE_BRACKET_:
	case _OP_RIGHT_EDGE_BRACKET_:
		action = currentToken->type == _OP_LEFT_EDGE_BRACKET_ ? -1 : 1;
		*openEdgeBrackets += action;
		break;
	case _OP_SEMICOLON_: return -1;
	default: return 0;
	}

	if (*openBrackets < 0 || *openEdgeBrackets < 0) {
		return -1;
	}

	return 0;
}

/**
 * <p>
 * Processes a member access part.
 * </p>
 * 
 * <p>
 * This specifically evaluates function calls and
 * the individual identifiers with potential array
 * accesses.
 * </p>
 * 
 * @returns A NodeReport with the topNode and how many tokens got processed
 * 
 * @param **tokens              Pointer to the token array
 * @param startPos              Position from where to start constructing
 * @param direction             Direction in which to process
 * @param useOptionalTyping     Flag for whether optional typing is allowed or not
 */
NodeReport PG_get_member_access_side_node_tree(TOKEN **tokens, size_t startPos, enum processDirection direction, int useOptionalTyping) {
	int offset = (int)PG_propagate_offset_by_direction(tokens, startPos, direction);
	int internalSkip = startPos + offset;
	Node *topNode = NULL;

	if ((int)PG_is_function_call(tokens, internalSkip) == true) {
		int nextIden = direction == LEFT ? internalSkip - (int)PG_propagate_back_till_iden(tokens, internalSkip) : internalSkip;
		NodeReport functionCallReport = PG_create_function_call_tree(tokens, nextIden);
		topNode = functionCallReport.node;
		internalSkip = direction == LEFT ? nextIden + functionCallReport.tokensToSkip : internalSkip + functionCallReport.tokensToSkip;
	} else {
		TOKEN *token = &(*tokens)[internalSkip];
		char *value = (char*)PG_get_identifier_by_index(token);
		topNode = PG_create_node(value, PG_get_node_type_by_value(&value), token->line, token->tokenStart);
		internalSkip++;
	}

	(void)PG_create_post_member_access_side_node_tree(&topNode, tokens, &internalSkip, useOptionalTyping);
	int actualSkip = internalSkip - startPos;
	return PG_create_node_report(topNode, actualSkip);
}

/**
 * <p>
 * This function processes post actions of a member access,
 * like array accesses or increment/decrement by one.
 * </p>
 * 
 * @param **topNode         The root node to which to append the action
 * @param **tokens          The pointer to the token array to process
 * @param *internalSkip     The skip counter in the member access tree generator (pointer for modification)
 * @param useOptionalTyping Flag for whether optional typesafety is on or not
 */
void PG_create_post_member_access_side_node_tree(Node **topNode, TOKEN **tokens, int *internalSkip, int useOptionalTyping) {
	NodeReport rep = {NULL, 0};
	
	switch ((*tokens)[(*internalSkip)].type) {
	case _OP_RIGHT_EDGE_BRACKET_:
		rep = PG_create_array_access_tree(tokens, *internalSkip);
		break;
	case _OP_COLON_:
		if (useOptionalTyping == true) {
			(void)PG_allocate_node_details(*topNode, 1);
			rep.tokensToSkip = (int)PG_add_varType_definition(tokens, (*internalSkip) + 1, *topNode);
		}

		break;
	default: break;
	}

	(*internalSkip) += rep.tokensToSkip;
	(*topNode)->leftNode = rep.node;
}

/**
 * <p>
 * Goes all tokens back, until it finds an IDENTIFIER.
 * </p>
 * 
 * @returns The number of tokens to backup until the IDENTIFIER appears
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position from where to start backtracking
 * 
 */
int PG_propagate_back_till_iden(TOKEN **tokens, size_t startPos) {
	int back = 0;
	int openBrackets = 0;

	for (int i = startPos; i >= 0; i--) {
		if ((*tokens)[i].type == _OP_LEFT_BRACKET_) {
			openBrackets--;
		} else if ((*tokens)[i].type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		}

		if ((*tokens)[i].type == _IDENTIFIER_
			&& openBrackets == 0) {
			back = startPos - i;
			break;
		}
	}

	return back;
}

/**
 * <p>
 * Calculates the offset by "going" into the direction for the tokens.
 * </p>
 * 
 * <p>
 * This should and is only used, to trace back array accesses.
 * </p>
 * 
 * @returns The offset
 * 
 * @param **tokens      Pointer to the token array
 * @param startPos      Position from where to start offsetting
 * @param direction     Determines the direction to go to
 */
int PG_propagate_offset_by_direction(TOKEN **tokens, size_t startPos, enum processDirection direction) {
	switch (direction) {
	case LEFT:	return -1 * (PG_back_shift_array_access(tokens, startPos));
	case RIGHT:	return 1;
	case STAY:	return 0;
	default:	return 0;
	}
}

/**
 * <p>
 * Counts how many tokens to go back, until the array
 * access is passed.
 * </p>
 * 
 * @returns The number of tokens to go back
 * 
 * @param **tokens  Pointer to the tokens array with the array access
 * @param startPos  Position from where to start going back
 */
int PG_back_shift_array_access(TOKEN **tokens, size_t startPos) {
	int back = 0;

	for (int i = startPos; i >= 0; i--) {
		if (i <= 0) {
			return 0;
		}

		switch ((*tokens)[i].type) {
			case _OP_SEMICOLON_:
			case _OP_EQUALS_:
			case _OP_PLUS_EQUALS_:
			case _OP_MINUS_EQUALS_:
			case _OP_MULTIPLY_EQUALS_:
			case _OP_DIVIDE_EQUALS_:
				i = 0;
				break;
			case _OP_RIGHT_EDGE_BRACKET_:
				if (((*tokens)[i - 1].type == _OP_LEFT_BRACKET_
					|| (*tokens)[i - 1].type == _IDENTIFIER_)) {
					back = startPos - (i + 1);
					i = 0;
					break;
				}
			default: continue;
		}
	}

	return back;
}

/**
 * <p>
 * Returns the copied value of a token.
 * </p>
 * 
 * @return A buffer with the copied token value
 * 
 * @param *token    Token to copy
 */
char *PG_get_identifier_by_index(TOKEN *token) {
	char *cache = (char*)calloc(token->size, sizeof(char));

	if (cache == NULL) {
		free(cache);
		FREE_MEMORY();
		(void)printf("ERROR! (CACHE CALLOC)\n");
	}

	(void)strncpy(cache, token->value, token->size);
	return cache;
}

/**
 * <p>
 * Predicts if the following token sequence matches a function
 * call or not.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Sequence matches a function call
 * <li>false - Sequence does not match a function call
 * </ul>
 * 
 * @param **tokens  Pointer to the tokens sequence to check
 * @param startPos  Position from where to start checking
 */
int PG_is_function_call(TOKEN **tokens, size_t startPos) {
	if ((int)PG_execute_direct_check_for_function_call(tokens, startPos) == true) {
		return true;
	}

	if ((*tokens)[startPos].type == _OP_LEFT_BRACKET_
		|| (*tokens)[startPos].type == _OP_LEFT_EDGE_BRACKET_) {
		return (int)PG_handle_LBracket_function_call(tokens, startPos);
	} else if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_) {
		return (int)PG_handle_RBracket_function_call(tokens, startPos);
	}

	return false;
}

/**
 * <p>
 * Checks if a function call, which starts with '('
 * is a function call or not.
 * </p>
 * 
 * @returns The number of tokens to go forward
 * 
 * @param **tokens  Pointer to the token array to check
 * @param startPos  Position from where to start
 */
int PG_handle_RBracket_function_call(TOKEN **tokens, size_t startPos) {
	int jumper = 0;
	int openBrackets = 0;

	while ((*tokens)[startPos + jumper].type != __EOF__) {
		if ((*tokens)[startPos + jumper].type == _OP_LEFT_BRACKET_) {
			openBrackets--;

			if (openBrackets == 0
				&& (*tokens)[startPos - 1].type == _IDENTIFIER_) {
				return jumper;
			}
		} else if ((*tokens)[startPos + jumper].type == _OP_RIGHT_BRACKET_) {
			openBrackets++;
		}

		jumper++;
	}

	return jumper;
}

/**
 * <p>
 * Checks if a function call, which starts with either ')' or ']'
 * is a function call or not.
 * </p>
 * 
 * @returns The number of tokens to go back
 * 
 * @param **tokens  Pointer to the token array to check
 * @param startPos  Position from where to start
 */
int PG_handle_LBracket_function_call(TOKEN **tokens, size_t startPos) {
	int jumper = 0;
	int openBrackets = 0;
	int openEdgeBrackets = 0;

	while (startPos - jumper > 0) {
		TOKEN *tok = &(*tokens)[startPos - jumper];

		if (tok->type == _OP_LEFT_EDGE_BRACKET_) {
			openEdgeBrackets--;
		} else if (tok->type == _OP_RIGHT_EDGE_BRACKET_) {
			openEdgeBrackets++;
		}
		
		if (openEdgeBrackets != 0) {
			jumper++;
			continue;
		}
		
		if (tok->type == _OP_LEFT_BRACKET_) {
			openBrackets--;
		} else if (tok->type == _OP_RIGHT_BRACKET_) {
			openBrackets++;

			if (openBrackets == 0
				&& (*tokens)[startPos - jumper - 1].type == _IDENTIFIER_) {
				return jumper;
			}
		} else if (tok->type == _OP_EQUALS_
			|| tok->type == _OP_SEMICOLON_) {
			return 0;
		}

		jumper++;
	}

	return jumper;
}

/**
 * <p>
 * Checks for the most primitive function call.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - If it is a function call
 * <li>false - If it is not a function call
 * </ul>
 * 
 * @param **tokens  Pointer to the tokens to check
 * @param startPos  Position from where to start checking
 */
int PG_execute_direct_check_for_function_call(TOKEN **tokens, size_t startPos) {
	if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_
		&& ((*tokens)[startPos + 1].type == _OP_LEFT_BRACKET_
		|| (*tokens)[startPos - 1].type == _IDENTIFIER_)) {
		return true;
	} else if ((*tokens)[startPos].type == _OP_LEFT_BRACKET_
		&& (*tokens)[startPos - 1].type == _OP_RIGHT_BRACKET_) {
		return true;
	} else if ((*tokens)[startPos].type == _IDENTIFIER_
		&& (*tokens)[startPos + 1].type == _OP_RIGHT_BRACKET_) {
		return true;
	}

	return false;
}

/**
 * @brief Adds a variable type if defined.
 * 
 * @note The startPos starts after the ":"!!!
 * 
 * @param **tokens  Pointer to the tokens array
 * @param startPos  Position from where to start addding the type definition
 * @param *parentNode   Pointer to the node above the type def (e.g. variable)
*/
int PG_add_varType_definition(TOKEN **tokens, size_t startPos, Node *parentNode) {
	int skip = 1;
	int dimensions = (int)PG_count_varType_dimensions(tokens, startPos + skip);
	skip += dimensions * 2;

	TOKEN *nameTok = &(*tokens)[startPos];
	Node *nameOfType = PG_create_node(nameTok->value, _VAR_TYPE_NODE_, nameTok->line, nameTok->tokenStart);

	if (dimensions > 0) {
		char *buffer = (char*)calloc(16, sizeof(char));
		int ret = (int)snprintf(buffer, 16 * sizeof(char), "%i", dimensions);

		if (ret <= 16 && ret > 0) {
			nameOfType->leftNode = PG_create_node(buffer, _VAR_DIM_NODE_, nameTok->line, nameTok->tokenStart);
		} else {
			(void)PARSE_TREE_NODE_RESERVATION_EXCEPTION();
		}
	}

	if (parentNode->details == NULL) {
		(void)PG_allocate_node_details(parentNode, 1);
	} else {
		(void)PG_allocate_node_details(parentNode, parentNode->detailsCount);
	}

	parentNode->details[0] = nameOfType;
	return skip;
}

/**
 * <p>
 * Counts the dimensions of a varType definition.
 * </p>
 * 
 * @returns The number of dimensions
 * 
 * @param **tokens      Pointer to the tokens array
 * @param startPos      Position from where to start counting
 */
int PG_count_varType_dimensions(TOKEN **tokens, size_t startPos) {
	int skip = 0;
	int dimensions = 0;

	while ((*tokens)[startPos + skip].type == _OP_RIGHT_EDGE_BRACKET_
		&& (*tokens)[startPos + skip + 1].type == _OP_LEFT_EDGE_BRACKET_
		&& startPos + skip < TOKEN_LENGTH) {
		skip += 2;
		dimensions++;
	}

	return dimensions;
}

/**
 * @brief Create a pointer to a node, when the token is a modifier.
 * 
 * @note As an modifier counts:
 * @note - private
 * @note - secure
 * @note - global
 * 
 * @param *token    Token to check
 * @param *skip     Pointer to the skipper of the current function
*/
Node *PG_create_modifier_node(TOKEN *token, int *skip) {
	if (token->type == _KW_PRIVATE_
		|| token->type == _KW_SECURE_
		|| token->type == _KW_GLOBAL_) {
		(*skip)++;
		return PG_create_node(token->value, _MODIFIER_NODE_, token->line, token->tokenStart);
	}

	return NULL;
}

/**
 * @brief Creates a Node structure on invokation with all the provided data.
 * 
 * @returns A Node with all the provided data
 * 
 * @param *value    Value of the node
 * @param type  Type of the Node (e.g. _VAR_NODE_ or _FUNCTION_CALL_NODE_, etc.)
 * @param line  Line of the token
 * @param pos   Position of the token
*/
Node *PG_create_node(char *value, enum NodeType type, int line, int pos) {
	Node *node = (Node*)calloc(1, sizeof(Node));

	if (node == NULL) {
		(void)PARSE_TREE_NODE_RESERVATION_EXCEPTION();
	}

	node->line = line;
	node->position = pos;
	node->type = type;
	node->value = value;
	node->leftNode = NULL;
	node->rightNode = NULL;
	node->details = NULL;
	node->detailsCount = 0;
	return node;
}

/**
 * This is an array containing all "mark worthy" operators.
*/
const TOKENTYPES operators[] = {
	_OP_PLUS_, _OP_MINUS_, _OP_MULTIPLY_, _OP_DIVIDE_, _OP_MODULU_,
	_OP_LEFT_BRACKET_, _OP_RIGHT_BRACKET_, _OP_EQUALS_, _OP_SEMICOLON_,
	_OP_COMMA_, _OP_RIGHT_BRACE_, _OP_DOT_, _OP_RIGHT_EDGE_BRACKET_,
	_OP_LEFT_EDGE_BRACKET_, _OP_COLON_, _OP_PLUS_EQUALS_, _OP_MINUS_EQUALS_,
	_OP_MULTIPLY_EQUALS_, _OP_DIVIDE_EQUALS_, _OP_CLASS_ACCESSOR_,
	_OP_ADD_ONE_, _OP_SUBTRACT_ONE_
};

/**
 * @brief Check if a given token is an operator or not.
 * 
 * @returns `True (1)` if the token is an operator, else `False (0)`
 * 
 * @param *token    Token to check
*/
int PG_is_operator(const TOKEN *token) {
	if (token->type == __EOF__) {
		return true;
	}

	int length = (sizeof(operators) / sizeof(operators[0]));

	for (int i = 0; i < length; i++) {
		if (token->type == operators[i]
			|| (int)PG_is_condition_operator(token->type) == true) {
			return true;
		}
	}

	return false;
}

/**
 * @brief Creates a NodeReport structure, which contains the root node of the
 * subtree as well as the token count tht got processed and can now be skipped.
 * 
 * @returns Created NodeReport with all the provided data
 * 
 * @param *topNode  Root node of the subtree
 * @param tokensToSkip  Number of tokens, that can be skipped
*/
NodeReport PG_create_node_report(Node *topNode, int tokensToSkip) {
	NodeReport report;
	report.node = topNode;
	report.tokensToSkip = tokensToSkip;
	return report;
}

/**
 * <p>
 * Frees all nodes recursively.
 * </p>
 * 
 * @returns 1 if it reaches the end
 * 
 * @param *node     Topnode to free
 */
int FREE_NODE(Node *node) {
	if (node != NULL) {
		FREE_NODE(node->leftNode);
		FREE_NODE(node->rightNode);

		for (size_t i = 0; i < node->detailsCount; i++) {
			FREE_NODE(node->details[i]);
			free(node->details[i]);
		}

		free(node->value);
		free(node->details);
		free(node);
		node = NULL;
	}

	return true;
}