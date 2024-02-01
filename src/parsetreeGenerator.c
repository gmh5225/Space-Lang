/////////////////////////////////////////////////////////////
///////////////////////    LICENSE    ///////////////////////
/////////////////////////////////////////////////////////////
/*
The SPACE-Language compiler compiles an input file into a runnable program.
Copyright (C) 2023  Lukas Nian En Lampl

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

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
#include "../headers/modules.h"
#include "../headers/errors.h"
#include "../headers/parsetree.h"
#include "../headers/Token.h"

#define true 1
#define false 0

////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////     PARSE TREE GENERATOR     ////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

const int UNINITIALZED = -1;

typedef struct NodeReport {
    struct Node *node;
    size_t tokensToSkip;
} NodeReport;

struct idenValRet {
    char *value;
    size_t movedTokens;
};

enum processDirection {
    LEFT = 0,
    RIGHT
};

enum nodeSide {
    LEFT_NODE,
    RIGHT_NODE,
    TOP_NODE
};

void PG_append_node_to_root_node(struct Node *node);
NodeReport PG_create_variable_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_runnable_tree(TOKEN **tokens, size_t startPos, int inBlock);
NodeReport PG_get_report_based_on_token(TOKEN **tokens, size_t startPos);
NodeReport PG_create_condition_tree(TOKEN **tokens, size_t startPos);
NodeReport PG_create_chained_condition_tree(TOKEN **tokens, size_t startPos);
int PG_get_condition_iden_length(TOKEN **tokens, size_t startPos, enum processDirection direction);
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
NodeReport PG_create_function_call_tree(TOKEN **tokens, size_t startPos);
size_t PG_add_params_to_node(struct Node *node, TOKEN **tokens, size_t startPos, int addStart, enum NodeType stdType);
int PG_get_bound_of_single_param(TOKEN **tokens, size_t startPos);
enum NodeType PG_get_node_type_by_value(char **value);
NodeReport PG_create_simple_term_node(TOKEN **tokens, size_t startPos, size_t boundaries);
NodeReport PG_assign_processed_node_to_node(TOKEN **tokens, size_t startPos);
size_t PG_go_backwards_till_operator(TOKEN **tokens, size_t startPos);
int PG_determine_bounds_for_capsulated_term(TOKEN **tokens, size_t startPos);
int PG_is_next_operator_multiply_divide_or_modulo(TOKEN **tokens, size_t startPos);
int PG_is_next_iden_a_member_access(TOKEN **tokens, size_t startPos);
NodeReport PG_create_member_access_tree(TOKEN **tokens, size_t startPos);
struct idenValRet PG_get_identifier_by_index(TOKEN **tokens, size_t startPos);
int PG_is_function_call(TOKEN **tokens, size_t startPos);
int PG_predict_argument_count(TOKEN **tokens, size_t startPos, int withPredefinedBrackets);
int PG_is_operator(const TOKEN *token);
struct Node *PG_create_node(char *value, enum NodeType type);
NodeReport PG_create_node_report(struct Node *topNode, int tokensToSkip);
void PG_allocate_node_details(struct Node *node, size_t size, int resize);
void PG_print_from_top_node(struct Node *topNode, int depth, int pos);
int FREE_NODES();
void FREE_NODE(struct Node *node);

//TREE ROOT
struct RootNode RootNode;
size_t TOKENLENGTH = 0;

int Generate_Parsetree(TOKEN **tokens, size_t TokenLength) {
    TOKENLENGTH = TokenLength;

    (void)printf("\n\n\n>>>>>>>>>>>>>>>>>>>>    PARSETREE    <<<<<<<<<<<<<<<<<<<<\n\n");

    if (tokens == NULL || TokenLength == 0) {
        (void)PARSER_TOKEN_TRANSMISSION_EXCEPTION();
    }

    RootNode.nodeCount = 0;
    printf("TOKENLENGTH: %i\n", TokenLength);

    /*NodeReport runnable = PG_create_runnable_tree(tokens, 0, false);
    printf("TREE:\n");
    PG_print_from_top_node(runnable.node, 0, 0);

    if (runnable.node == NULL) {
        printf("Something went wrong (PG)!\n");
    }*/

    NodeReport rep = PG_create_simple_term_node(tokens, 0, TOKENLENGTH);
    PG_print_from_top_node(rep.node, 0, 0);

    (void)printf("\n\n\n>>>>>    Tokens converted to tree    <<<<<\n\n");

    return 1;
}

NodeReport PG_create_variable_tree(TOKEN **tokens, size_t startPos) {
    struct Node *varNode = PG_create_node("var", _VAR_NODE_);

    struct idenValRet nameRet = PG_get_identifier_by_index(tokens, startPos + 1);
    char *varName = nameRet.value;
    struct Node *nameNode = PG_create_node(varName, _IDEN_NODE_);
    size_t checkPosition = startPos + nameRet.movedTokens + 1;

    if ((*tokens)[checkPosition].type == _OP_EQUALS_) {
        size_t bounds = (size_t)PG_get_size_till_next_semicolon(tokens, checkPosition + 1);
        NodeReport termReport = PG_create_simple_term_node(tokens, checkPosition + 1, bounds);
        struct Node *valueNode = termReport.node;
        varNode->rightNode = valueNode;
    }

    varNode->leftNode = nameNode;
    return PG_create_node_report(varNode, 1);
}

/*
Purpose: Generate a subtree for a function definition
Return Type: NodeReport => Contains the root node of the subtree and the number of tokens to skip
Params: TOKEN **tokens => Pointer to the array of tokens;
        size_t startPos => The position from where to start constructing
_______________________________
Layout:

 [RUNNABLE]
     |
[STATEMENT]
[EXPRESSION]

The [RUNNABLE] is created as a fully independent node,
whose [STATEMENT] and [EXPRESSION] con be found in
´´´node->details[position]´´´.

[STATEMENT]: Statements in the source
[EXPRESSION]: Expressions to run
_______________________________
*/
NodeReport PG_create_runnable_tree(TOKEN **tokens, size_t startPos, int inBlock) {
    struct Node *parentNode = PG_create_node("RUNNABLE", _RUNNABLE_NODE_);
    int argumentCount = 0;
    size_t jumper = 0;

    while (startPos + jumper < TOKENLENGTH) {
        TOKEN *currentToken = &(*tokens)[startPos + jumper];

        if (currentToken->type == _OP_LEFT_BRACE_
            && inBlock == true) {
            jumper++;
            break;
        } else if (currentToken->type == __EOF__) {
            break;
        }

        NodeReport report = PG_get_report_based_on_token(tokens, startPos + jumper);

        if (report.node != NULL) {
            if (argumentCount == 0) {
                (void)PG_allocate_node_details(parentNode, 1, false);
            } else {
                parentNode->details = (struct Node**)realloc(parentNode->details, sizeof(struct Node) * argumentCount + 1);

                if (parentNode->details == NULL) {
                    printf("Something went wrong (runnable)!\n");
                    exit(EXIT_FAILURE);
                }

                parentNode->detailsCount = argumentCount + 1;
            }

            parentNode->details[argumentCount++] = report.node;
            jumper += report.tokensToSkip;
            printf("NEXT TOK: %s\n", (*tokens)[startPos + jumper].value);
        } else {
            jumper++;
        }
    }

    return PG_create_node_report(parentNode, jumper);
}

/*
Purpose: Get the correct NodeReport based on a token
Return Type: NodeReport => NodeReport the called function returns
Params: TOKEN **tokens => Pointer to the tokens array;
        size_t startPos => Position where the crucial token is located
        and from where to start constructing subtrees
*/
NodeReport PG_get_report_based_on_token(TOKEN **tokens, size_t startPos) {
    switch ((*tokens)[startPos].type) {
    case _KW_VAR_:
        return PG_create_variable_tree(tokens, startPos);
    case _KW_INCLUDE_:
        return PG_create_include_tree(tokens, startPos);
    case _KW_EXPORT_:
        return PG_create_export_tree(tokens, startPos);
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
    case _KW_THIS_:
        if ((*tokens)[startPos + 3].type == _KW_CONSTRUCTOR_) {
            return PG_create_class_constructor_tree(tokens, startPos);
        }
    case _KW_GLOBAL_:
    case _KW_SECURE_:
    case _KW_PRIVATE_:
        if ((*tokens)[startPos + 1].type == _KW_FUNCTION_) {
            return PG_create_function_tree(tokens, startPos);
        } else if ((*tokens)[startPos + 1].type == _KW_CLASS_) {
            return PG_create_class_tree(tokens, startPos);
        }
    default:
        break;
    }

    return PG_create_node_report(NULL, UNINITIALZED);
}

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
*/
NodeReport PG_create_chained_condition_tree(TOKEN **tokens, size_t startPos) {
    struct Node *cache = NULL;

    return PG_create_node_report(cache, 0);
}

NodeReport PG_create_condition_tree(TOKEN **tokens, size_t startPos) {
    size_t skip = 0;

    while (startPos + skip < TOKENLENGTH) {
        TOKEN *currentToken = &(*tokens)[startPos + skip];

        if ((int)PG_is_condition_operator(currentToken->type) == true) {
            struct Node *conditionNode = PG_create_node(currentToken->value, PG_get_node_type_by_value(&currentToken->value));
            int leftBounds = (int)PG_get_condition_iden_length(tokens, startPos + skip - 1, LEFT);
            int rightBounds = (int)PG_get_condition_iden_length(tokens, startPos + skip + 1, RIGHT);

            NodeReport rightTermReport = PG_create_simple_term_node(tokens, startPos + skip + 1, rightBounds);
            printf("LB: %i\n", leftBounds);
            printf("LTOK START: %s\n", (*tokens)[startPos + skip - leftBounds].value);
            NodeReport leftTermReport = PG_create_simple_term_node(tokens, startPos + skip - leftBounds, leftBounds);

            conditionNode->leftNode = leftTermReport.node;
            conditionNode->rightNode = rightTermReport.node;

            skip += rightTermReport.tokensToSkip;
            return PG_create_node_report(conditionNode, skip);
        }

        skip++;
    }

    printf("CONDITION ERROR!!!\n");
    return PG_create_node_report(NULL, 0);
}

int PG_get_condition_iden_length(TOKEN **tokens, size_t startPos, enum processDirection direction) {
    int counter = 0;

    if (direction == RIGHT) {
        while (startPos + counter < TOKENLENGTH) {
            if ((int)PG_is_condition_operator((*tokens)[startPos + counter].type) == true
                || (*tokens)[startPos + counter].type == _OP_LEFT_BRACKET_
                || (*tokens)[startPos + counter].type == _OP_RIGHT_BRACKET_) {
                break;
            }

            counter++;
        }
    } else {
        if (startPos == 0) {
            return 1;
        }

        while (startPos - counter > 0) {
            if ((int)PG_is_condition_operator((*tokens)[startPos - counter].type) == true
                || (*tokens)[startPos - counter].type == _OP_LEFT_BRACKET_
                || (*tokens)[startPos - counter].type == _OP_RIGHT_BRACKET_) {
                counter++;
                break;
            } else if (startPos - counter - 1 == 0) {
                counter += 2;
                break;
            }

            counter++;
        }
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
    //Start (from ´´´this´´´ to ´´´constructor´´´) is SYNTAXANALYSIS
    //not tree generation.
    /*
    > this::constructor {}
    >                   ^
    >         (*tokens)[startPos + 5]
    */

    NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + 5, true);
    runnableReport.node->type = _CLASS_CONSTRUCTOR_NODE_;
    runnableReport.node->value = "CONSTRUCTOR";

    return PG_create_node_report(runnableReport.node, runnableReport.tokensToSkip + 5);
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
[MOD]  [PARAMS] [RUNNABLE]
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
    struct Node *modNode = NULL;
    //public class obj(param1, param2) => {}
    if ((*tokens)[startPos].type == _KW_PRIVATE_
        || (*tokens)[startPos].type == _KW_GLOBAL_
        || (*tokens)[startPos].type == _KW_SECURE_) {
        modNode = PG_create_node((*tokens)[startPos].value, _MODIFIER_NODE_);
        skip++;
    }

    struct Node *classNode = PG_create_node((*tokens)[startPos + skip + 1].value, _CLASS_NODE_);
    classNode->leftNode = modNode;

    int arguments = (int)PG_predict_argument_count(tokens, startPos + skip + 2, true);
    (void)PG_allocate_node_details(classNode, arguments + 1, false);
    skip += (size_t)PG_add_params_to_node(classNode, tokens, startPos + skip + 2, 0, _NULL_) + 4;

    if ((*tokens)[startPos + skip].type == _KW_WITH_) {
        arguments = (int)PG_predict_argument_count(tokens, startPos + skip + 1, false);
        (void)PG_allocate_node_details(classNode, arguments, true);
        (void)PG_add_params_to_node(classNode, tokens, startPos + skip, classNode->detailsCount - arguments, _INTERFACE_NODE_);
        skip += arguments * 2;
    }

    NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip, true);
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
    NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + 1, true);
    struct Node *tryNode = runnableReport.node;
    tryNode->type = _TRY_NODE_;
    tryNode->value = "TRY";

    return PG_create_node_report(tryNode, runnableReport.tokensToSkip + 1);
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
    //Rest is checked by the SYNTAXANALYZER for correctness
    NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + 5, true);
    struct Node *catchNode = runnableReport.node;
    catchNode->type = _CATCH_NODE_;
    catchNode->value = "CATCH";

    return PG_create_node_report(catchNode, runnableReport.tokensToSkip + 5);
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
    struct Node *topNode = PG_create_node((*tokens)[startPos + 1].value, _EXPORT_NODE_);
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
    //Here: include "../file.spc";
    struct Node *topNode = PG_create_node((*tokens)[startPos + 1].value, _INCLUDE_NODE_);
    return PG_create_node_report(topNode, 3);
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
    struct Node *enumNode = PG_create_node((*tokens)[startPos + 1].value, _ENUM_NODE_);
    int argumentCount = (int)PG_predict_enumerator_count(tokens, startPos + 2);
    (void)PG_allocate_node_details(enumNode, argumentCount, false);
    
    argumentCount = 0;
    size_t skip = 2;
    int currentEnumeratorValue = 0;

    while (startPos + skip < TOKENLENGTH) {
        TOKEN *currentToken = &(*tokens)[startPos + skip];

        if (currentToken->type == _OP_LEFT_BRACE_) {
            skip++;
            break;
        }

        if (argumentCount > enumNode->detailsCount) {
            printf("SIZE (enum) %i!\n", enumNode->detailsCount);
            exit(EXIT_FAILURE);
        }

        struct Node *enumeratorNode = PG_create_node((*tokens)[startPos + skip + 1].value, _ENUMERATOR_NODE_);

        if (currentToken->type == _OP_COMMA_
            || currentToken->type == _OP_RIGHT_BRACE_) {
            /*
            > Looking for: enumerator : [NUMBER]
            >                         ^
            */
            if ((*tokens)[startPos + skip + 2].type == _OP_COLON_) {
                currentEnumeratorValue = (int)atoi((*tokens)[startPos + skip + 3].value);
                skip++;
            }

            //Size for long
            char *value = malloc(sizeof(char) * 24);

            if (value == NULL) {
                printf("VALUE = NULL! (enum)\n");
                exit(EXIT_FAILURE);
            }

            (void)snprintf(value, 24, "%d", currentEnumeratorValue++);
            enumeratorNode->rightNode = PG_create_node(value, _VALUE_NODE_);
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

    while (starPos + jumper < TOKENLENGTH) {
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
    int skip = 0;

    struct Node *modNode = NULL;
    struct Node *retTypeNode = NULL;

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

    if ((*tokens)[startPos].type == _KW_PRIVATE_
        || (*tokens)[startPos].type == _KW_GLOBAL_
        || (*tokens)[startPos].type == _KW_SECURE_) {
        modNode = PG_create_node((*tokens)[startPos].value, _MODIFIER_NODE_);
        skip++;
    }

    if ((*tokens)[startPos + skip + 1].type == _OP_COLON_) {
        retTypeNode = PG_create_node((*tokens)[startPos + skip + 2].value, _RET_TYPE_NODE_);
        skip += 2;
    }

    //No NULL check needed, if leftNode or rightNode == NULL nothing changes
    struct Node *functionNode = PG_create_node((*tokens)[startPos + skip + 1].value, _FUNCTION_NODE_);
    functionNode->leftNode = modNode;
    functionNode->rightNode = retTypeNode;

    int argumentCount = (int)PG_predict_argument_count(tokens, startPos + skip + 2, true);
    (void)PG_allocate_node_details(functionNode, argumentCount + 1, false);
    skip += (size_t)PG_add_params_to_node(functionNode, tokens, startPos + skip + 2, 0, _NULL_);

    NodeReport runnableReport = PG_create_runnable_tree(tokens, startPos + skip + 1, true);
    functionNode->details[argumentCount] = runnableReport.node;

    return PG_create_node_report(functionNode, skip + runnableReport.tokensToSkip + 1);
}

/*
Purpose: Generate a subtree for a function call
Return Type: NodeReport => Contains the root node of the subtree and the number of tokens to skip
Params: TOKEN **tokens => Pointer to the array of tokens;
        size_t startPos => The index of the first token of the function call
*/
NodeReport PG_create_function_call_tree(TOKEN **tokens, size_t startPos) {
    struct idenValRet nameRet = PG_get_identifier_by_index(tokens, startPos);
    struct Node *functionCallNode = PG_create_node(nameRet.value, _FUNCTION_CALL_NODE_);

    int argumentSize = (int)PG_predict_argument_count(tokens, startPos + nameRet.movedTokens, true);
    (void)PG_allocate_node_details(functionCallNode, argumentSize, false);

    size_t paramSize = (size_t)PG_add_params_to_node(functionCallNode, tokens, startPos + nameRet.movedTokens, 0, _NULL_);

    return PG_create_node_report(functionCallNode, paramSize + 2);
}

/*
Purpose: Adds the parameters to the function call node
Return Type: size_t => The number of tokens to skip
Params: struct Node *node => The root node of the function call subtree;
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
size_t PG_add_params_to_node(struct Node *node, TOKEN **tokens, size_t startPos, int addStart, enum NodeType stdType) {
    int detailsPointer = addStart;
    int skip = 0;

    for (size_t i = startPos; i < TOKENLENGTH; i++) {
        TOKEN *currentToken = &(*tokens)[i];

        if (currentToken->type == _OP_COMMA_
            || currentToken->type == _OP_RIGHT_BRACKET_
            || currentToken->type == _KW_WITH_) {
            //Check if the param is going to be out of the allocated space
            if (detailsPointer == node->detailsCount) {
                printf("SIZE!\n");
                skip = i - startPos;
                break;
            }

            int bounds = (int)PG_get_bound_of_single_param(tokens, i + 1);
            NodeReport termReport = PG_create_simple_term_node(tokens, i + 1, bounds);
            node->details[detailsPointer] = termReport.node;
            node->details[detailsPointer++]->type = stdType == _NULL_ ? termReport.node->type : stdType;
            i += termReport.tokensToSkip;
        } else if (currentToken->type == _OP_LEFT_BRACKET_
            || currentToken->type == _OP_RIGHT_BRACE_) {
            skip = i - startPos - 1;
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

    for (size_t i = startPos; i < TOKENLENGTH; i++) {
        TOKEN *token = &(*tokens)[i];

        if (token->type == _OP_COMMA_
            && openBrackets == 0) {
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

/*
Purpose: Predict the number of arguments of a function call
Return Type: int => Number of arguments of the function call
Params: TOKEN **tokens => Pointer to the array of tokens;
        size_t startPos => The index of the first token of the function call;
        int withPredefinedBrackets => Is "openBrackets" awaited to be 1
*/
int PG_predict_argument_count(TOKEN **tokens, size_t startPos, int withPredefinedBrackets) {
    if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_
        && (*tokens)[startPos + 1].type == _OP_LEFT_BRACKET_) {
        return 0;
    }

    int count = 1;
    int openBrackets = 0;

    for (size_t i = startPos; i < TOKENLENGTH; i++) {
        TOKEN *token = &(*tokens)[i];
        
        if (token->type == _OP_COMMA_) {
            if (withPredefinedBrackets == true
                && openBrackets != 1) {
                continue;
            }

            count++;
        } else if (token->type == _OP_RIGHT_BRACKET_) {
            openBrackets++;
        } else if (token->type == _OP_LEFT_BRACKET_) {
            openBrackets--;

            if (openBrackets <= 0) {
                break;
            }
        } else if (token->type == _OP_RIGHT_BRACE_) {
            break;
        }
    }

    return count;
}

/*
Purpose: Get the token count to skip till the next semicolon
Return Type: size_t => Number of tokens to skip till the next semicolon
Params: TOKEN **tokens => Pointer to the array of tokens;
        size_t startPos => The index of the first token of the subtree
*/
size_t PG_get_size_till_next_semicolon(TOKEN **tokens, size_t startPos) {
    size_t size = 0;

    while ((*tokens)[startPos + size].type != _OP_SEMICOLON_) {
        size++;
    }

    return size;
}

/*
Purpose: Allocate space for the details of the node
Return Type: void
Params: struct Node *node => Pointer to the node;
        size_t size => The size of the details to allocate,
        int resize => Determines if the call is a resize or not
*/
void PG_allocate_node_details(struct Node *node, size_t size, int resize) {
    if (node == NULL) {
        printf("NODE NULL\n");
        exit(EXIT_FAILURE);
    }

    if (resize == false) {
        node->details = (struct Node**)malloc(sizeof(struct Node*) * size);
        node->detailsCount = size;

        if (node->details == NULL) {
            printf("DETAILS ERROR!\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < size; i++) {
            node->details[i] = NULL;
        }
    } else {
        node->details = (struct Node**)realloc(node->details, sizeof(struct Node) * (node->detailsCount + size));
        node->detailsCount = node->detailsCount + size;

        if (node->details == NULL) {
            printf("DETAILS ERROR!\n");
            exit(EXIT_FAILURE);
        }

        for (int i = node->detailsCount; i < node->detailsCount + size; i++) {
            node->details[i] = NULL;
        }
    }
}

/*
Purpose: Print out a tree base on the nodes
Return Type: void
Params: struct Node *topNode => Pointer to the root node of the tree;
        int depth => The depth of the tree;
        int pos => The position of the node (0 = Center, 1 = Left, 2 = Right)
*/
void PG_print_from_top_node(struct Node *topNode, int depth, int pos) {
    if (topNode == NULL || topNode->value == NULL) {
        return;
    }

    for (int i = 0; i < depth; ++i) {
        printf("  ");
    }

    if (pos == 0) {
        printf("C: %s -> %i\n", topNode->value, topNode->type);
    } else if (pos == 1) {
        printf("L: %s -> %i\n", topNode->value, topNode->type);
    } else {
        printf("R: %s -> %i\n", topNode->value, topNode->type);
    }

    for (int i = 0; i < topNode->detailsCount; i++) {
        if (topNode->details[i] != NULL) {
            for (int i = 0; i < depth + 1; ++i) {
                printf("  ");
            }

            printf("(%s) detail: %s -> %i\n", topNode->value, topNode->details[i]->value, topNode->details[i]->type);
            PG_print_from_top_node(topNode->details[i]->leftNode, depth + 2, 1);
            PG_print_from_top_node(topNode->details[i]->rightNode, depth + 2, 2);

            for (int n = 0; n < topNode->details[i]->detailsCount; n++) {
                PG_print_from_top_node(topNode->details[i]->details[n], depth + 2, 0);
            }
        } else {
            printf("(%s) detail: NULL -> NULL\n", topNode->value);
        }
    }

    PG_print_from_top_node(topNode->leftNode, depth + 1, 1);
    PG_print_from_top_node(topNode->rightNode, depth + 1, 2);
}

/*
Purpose: Get the node type by the value of the node
Return Type: enum NodeType => The node type of the node
Params: char **value => Pointer to the value of the node
*/
enum NodeType PG_get_node_type_by_value(char **value) {
    if ((*value)[0] == '"') {
        return _STRING_NODE_;
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
    } else {
        for (size_t i = 0; (*value)[i] != '\0'; i++) {
            if ((*value)[i] == '-'
                && (*value)[i + 1] == '>') {
                return _CLASS_ACCESS_NODE_;
            }
        }
    }

    return _IDEN_NODE_;
}

/*
Purpose: Creates a subTree for a simple term
Return Type: NodeReport => Report containing the topNode (start operation) and how many tokens got processed
Params: TOKEN **tokens => Tokens array to be processed;
        size_t startPos => Position from where to start processing;
        size_t boundaries => How many tokens should be checked
_______________________________
PRECEDENCE of term OPERATORS:
+---+---+---+---+---+---+---+---+
|   | * | / | % | + | - | ( | ) |
+---+---+---+---+---+---+---+---+
| * | = | = | = | * | * | ( | ) |
+---+---+---+---+---+---+---+---+
| / | = | = | = | / | / | ( | ) |
+---+---+---+---+---+---+---+---+
| % | = | = | = | % | % | ( | ) |
+---+---+---+---+---+---+---+---+
| + | * | / | % | = | = | ( | ) |
+---+---+---+---+---+---+---+---+
| - | * | / | % | = | = | ( | ) |
+---+---+---+---+---+---+---+---+
| ( | ( | ( | ( | ( | ( | = | = |
+---+---+---+---+---+---+---+---+
| ) | ) | ) | ) | ) | ) | = | = |
+---+---+---+---+---+---+---+---+
_______________________________
Layout:

     [OP]
   /      \
[IDEN]  [IDEN]

After [OP] the [IDEN] is either on the ´´´node->leftNode´´´ or
´´´node->rightNode´´´.

[OP]: All operators like: '+', '-', '*', '/', '%'
[IDEN]: Can be a number, function call, a string or an identifier
_______________________________
*/

NodeReport PG_create_simple_term_node(TOKEN **tokens, size_t startPos, size_t boundaries) {
    struct Node *cache = NULL;
    struct Node *temp = NULL;
    size_t lastIdenPos = UNINITIALZED;
    int waitingToEndPlusOrMinus = false;

    for (size_t i = startPos; i < startPos + boundaries; i++) {
        TOKEN *currentToken = &(*tokens)[i];

        if (currentToken->type == __EOF__) {
            break;
        } else if (boundaries == 1) {
            enum NodeType nodeType = PG_get_node_type_by_value(&currentToken->value);
            cache = PG_create_node(currentToken->value, nodeType);
            break;
        }

        switch (currentToken->type) {
        case _OP_RIGHT_BRACKET_: {
            struct Node *target = waitingToEndPlusOrMinus == true ? temp : cache;
            NodeReport report = {NULL, UNINITIALZED};

            if ((int)PG_is_function_call(tokens, i) > 0) {
                break;
            } else {
                size_t bounds = (size_t)PG_determine_bounds_for_capsulated_term(tokens, i);
                report = PG_create_simple_term_node(tokens, i + 1, bounds - 1);
                i += bounds;
            }

            if (target != NULL) {
                if (target->leftNode == NULL) {
                    target->leftNode = report.node;
                } else {
                    target->rightNode = report.node;
                }
            } else {
                cache = report.node;
            }
            
            break;
        }
        case _OP_PLUS_:
        case _OP_MINUS_: {
            if (waitingToEndPlusOrMinus == true && temp != NULL) {
                cache->rightNode = temp;
                temp = NULL;
            }

            struct Node *node = PG_create_node(currentToken->value, PG_get_node_type_by_value(&currentToken->value));

            if (cache == NULL) {
                if ((int)PG_is_next_iden_a_member_access(tokens, lastIdenPos) == true) {
                    NodeReport memberAccReport = PG_create_member_access_tree(tokens, lastIdenPos);
                    node->leftNode = memberAccReport.node;
                } else {
                    struct idenValRet lvalRet = PG_get_identifier_by_index(tokens, lastIdenPos);
                    node->leftNode = PG_create_node(lvalRet.value, PG_get_node_type_by_value(&lvalRet.value));
                }
            } else {
                node->leftNode = cache;
            }

            int multOrDivOrModAtRight = (int)PG_is_next_operator_multiply_divide_or_modulo(tokens, i + 1);

            if (multOrDivOrModAtRight == false) {
                NodeReport assignRep = PG_assign_processed_node_to_node(tokens, i);
                node->rightNode = assignRep.node;
                i += assignRep.tokensToSkip;
            } else {
                waitingToEndPlusOrMinus = true;
            }

            lastIdenPos = UNINITIALZED;
            cache = node;
            break;
        }
        case _OP_DIVIDE_:
        case _OP_MULTIPLY_:
        case _OP_MODULU_: {
            struct Node *node = PG_create_node(currentToken->value, PG_get_node_type_by_value(&currentToken->value));
            NodeReport rightAssignRep = PG_assign_processed_node_to_node(tokens, i);
            node->rightNode = rightAssignRep.node;
            i += rightAssignRep.tokensToSkip;

            if (cache == NULL) {
                NodeReport leftAssignRep = PG_assign_processed_node_to_node(tokens, lastIdenPos - 1);
                node->leftNode = leftAssignRep.node;
            }

            if (waitingToEndPlusOrMinus == false) {
                if (node->leftNode == NULL) {
                    node->leftNode = cache;
                }

                cache = node;
            } else {
                if (temp == NULL) {
                    NodeReport leftAssignRep = PG_assign_processed_node_to_node(tokens, lastIdenPos - 1);
                    node->leftNode = leftAssignRep.node;
                } else {
                    node->leftNode = temp;
                }
                
                temp = node;
            }

            lastIdenPos = UNINITIALZED;
            break;
        }
        default:
            if (lastIdenPos == UNINITIALZED) {
                lastIdenPos = i;
            }

            break;
        }

        if (i + 1 == startPos + boundaries) {
            if (waitingToEndPlusOrMinus == true
                && temp != NULL) {
                if (cache != NULL) {
                    if (cache->leftNode == NULL) {
                        cache->leftNode = temp;
                    } else {
                        cache->rightNode = temp;
                    }
                } else {
                    cache = temp;
                }

                temp = NULL;
            } else if (cache == NULL) {
                struct Node *node = NULL;
                NodeReport assignRep = PG_assign_processed_node_to_node(tokens, startPos);
                node = assignRep.node;
                i += assignRep.tokensToSkip;
                cache = node;
            }
        }
    }

    return PG_create_node_report(cache, boundaries);
}

/*
Purpose: Assign the correct simple term node to the given node
Return Type: NodeReport => Contains the top node and how many tokens to skip
Params: TOKEN **tokens => Pointer to the tokens;
        size_t startPos => Position from where to start processing
*/
NodeReport PG_assign_processed_node_to_node(TOKEN **tokens, size_t startPos) {
    NodeReport report = {NULL, UNINITIALZED};
    
    if ((*tokens)[startPos + 1].type == _OP_RIGHT_BRACKET_) {
        size_t bounds = (size_t)PG_determine_bounds_for_capsulated_term(tokens, startPos + 1);
        report = PG_create_simple_term_node(tokens, startPos + 2, bounds);
    } else if ((*tokens)[startPos + 2].type == _OP_RIGHT_BRACKET_) {
        report = PG_create_function_call_tree(tokens, startPos + 1);
        report.tokensToSkip -= 1;
    } else if ((int)PG_is_next_iden_a_member_access(tokens, startPos + 1) == true) {
        report = PG_create_member_access_tree(tokens, startPos + 1);
    } else {
        struct idenValRet rvalRet = PG_get_identifier_by_index(tokens, startPos + 1);
        struct Node *node = PG_create_node(rvalRet.value, _IDEN_NODE_);
        return PG_create_node_report(node, rvalRet.movedTokens);
    }
    
    if (report.node != NULL && report.tokensToSkip != UNINITIALZED) {
        return PG_create_node_report(report.node, report.tokensToSkip);
    }

    return PG_create_node_report(NULL, 1);
}

/*
Purpose: Get the index of the last operator in the token array
Return Type: size_t => How many tokens to go back till the operator
Params: TOKEN **tokens => Tokens to checked for the operator;
        size_t startPos => Position from where to start checking
*/
size_t PG_go_backwards_till_operator(TOKEN **tokens, size_t startPos) {
    for (size_t i = 0; startPos - i > 0; i++) {
        if (PG_is_operator(&(*tokens)[startPos - i]) == true) {
            return i - 1;
        } else if (startPos - (i + 1) == 0) {
            return i + 1;
        }
    }

    return 0;
}

/*
Purpose: Determine the bounds of a capsulated term
Return Type: int => Size of the capsulated term
Params: TOKEN **tokens => Tokens to checked for the capsulated term;
        size_t startPos => Position from where to start checking
*/
int PG_determine_bounds_for_capsulated_term(TOKEN **tokens, size_t startPos) {
    size_t bounds = 0;
    int openBrackets = 0;

    while ((*tokens)[startPos + bounds].type != __EOF__) {
        if ((*tokens)[startPos + bounds].type == _OP_LEFT_BRACKET_) {
            openBrackets--;

            if (openBrackets == 0) {
                break;
            }
        } else if ((*tokens)[startPos + bounds].type == _OP_RIGHT_BRACKET_) {
            openBrackets++;
        }

        bounds++;
    }

    return bounds;
}

/*
Purpose: Check if the next Operator in a term is a Multiply, Modulo or Divide operator
Return Type: int => true = the next OP is '*', '/' or '%'; false = is not a '*', '/' or '%'
Params: TOKEN **tokens => Token array for next operator determination;
        size_t startPos => Position from where to start checking
*/
int PG_is_next_operator_multiply_divide_or_modulo(TOKEN **tokens, size_t startPos) {
    int jumper = 0;
    
    while ((*tokens)[startPos + jumper].type != __EOF__) {
        TOKEN *token = &(*tokens)[startPos + jumper];
        if (token->type == _OP_PLUS_
            || token->type == _OP_MINUS_
            || token->type == _OP_LEFT_BRACKET_
            || token->type == _OP_RIGHT_BRACKET_
            || token->type == _OP_COMMA_) {
            return false;
        } else if (token->type == _OP_MULTIPLY_
            || token->type == _OP_DIVIDE_
            || token->type == _OP_MODULU_) {
            return true;
        }

        jumper++;
    }

    return false;
}

/*
Purpose: Checks if the identifier at the position is a member access
Return Type: int => true = is member access; false => not a member access
Params: TOKEN **tokens => Pointer to the tokens;
        size_t startPos => Position from where to start checking
*/
int PG_is_next_iden_a_member_access(TOKEN **tokens, size_t startPos) {
    for (size_t i = startPos; i < TOKENLENGTH; i++) {
        if ((int)PG_is_operator(&(*tokens)[i]) == true) {
            if ((*tokens)[i].type == _OP_DOT_) {
                return true;
            } else if ((*tokens)[i].type == _OP_CLASS_ACCESSOR_) {
                continue;
            }  else if ((*tokens)[i].type == _OP_RIGHT_BRACKET_) {
                continue;
            } else if ((*tokens)[i].type == _OP_LEFT_BRACKET_) {
                continue;
            } else {
                return false;
            }
        }
    }

    return false;
}

/*
Purpose: Creates a subtree for member accesses
Return Type: NodeReport => Contains the topNode and how many tokens got consumed
Params: TOKEN **tokens => Pointer to the tokens array;
        size_t startPos => Position from where to start constructing
*/
NodeReport PG_create_member_access_tree(TOKEN **tokens, size_t startPos) {
    struct Node *cache = NULL;
    size_t skip = 0;

    while (startPos + skip < TOKENLENGTH) {
        TOKEN *currentToken = &(*tokens)[startPos + skip];

        if (currentToken->type == _OP_DOT_) {
            if (cache == NULL) {
                struct Node *topNode = PG_create_node(".", _MEMBER_ACCESS_NODE_);
                /*
                > get()
                >    ^
                > (*tokens)[startPos + 1]
                */
                if ((int)PG_is_function_call(tokens, startPos + 1) > 0) {
                    NodeReport functionCallReport = PG_create_function_call_tree(tokens, startPos);
                    topNode->leftNode = functionCallReport.node;
                } else {
                    struct idenValRet lvalRet = PG_get_identifier_by_index(tokens, startPos);
                    topNode->leftNode = PG_create_node(lvalRet.value, _IDEN_NODE_);
                }

                /*
                > .get()
                >     ^
                >  (*tokens)[startPos + skip + 2]
                */
                if ((int)PG_is_function_call(tokens, startPos + skip + 2) > 0) {
                    NodeReport functionCallReport = PG_create_function_call_tree(tokens, startPos + skip + 1);
                    topNode->rightNode = functionCallReport.node;
                    skip += functionCallReport.tokensToSkip + 1;
                } else {
                    struct idenValRet rvalRet = PG_get_identifier_by_index(tokens, startPos + skip + 1);
                    topNode->rightNode = PG_create_node(rvalRet.value, _IDEN_NODE_);
                }

                cache = topNode;
            } else {
                struct Node *topNode = PG_create_node(".", _MEMBER_ACCESS_NODE_);
                
                /*
                > .get()
                >     ^
                >  (*tokens)[startPos + skip + 2]
                */
                if ((int)PG_is_function_call(tokens, startPos + skip + 2) > 0) {
                    NodeReport functionCallReport = PG_create_function_call_tree(tokens, startPos + skip + 1);
                    topNode->rightNode = functionCallReport.node;
                    skip += functionCallReport.tokensToSkip + 1;
                } else {
                    struct idenValRet rvalRet = PG_get_identifier_by_index(tokens, startPos + skip + 1);
                    topNode->rightNode = PG_create_node(rvalRet.value, _IDEN_NODE_);
                }

                topNode->leftNode = cache;
                cache = topNode;
            }
        } else if ((int)PG_is_operator(currentToken) == true) {
            if (cache == NULL
                && (currentToken->type == _OP_LEFT_BRACKET_
                || currentToken->type == _OP_RIGHT_BRACKET_)) {
                    skip++;
                    continue;
                }

            break;
        }

        skip++;
    }

    return PG_create_node_report(cache, skip);
}

/*
Purpose: Get the value of an identifier list like "Math.sqrt()"
Return Type: struct idenValRet => Contains the value of the identifier and how many tokens to skip
Params: TOKEN **tokens => Token array in which the identifiers are located;
        size_t startPos => Position from where to start to get the values
*/
struct idenValRet PG_get_identifier_by_index(TOKEN **tokens, size_t startPos) {
    size_t idenStartPos = startPos;
    size_t idenEndPos = startPos;
    int nullParam = false;

    if ((*tokens)[startPos + 1].type == _OP_LEFT_BRACKET_
        && (*tokens)[startPos - 1].type == _OP_RIGHT_BRACKET_) {
        idenStartPos = startPos;
        idenEndPos = startPos + 1;
        nullParam = true;
    }

    while ((*tokens)[idenEndPos].type != __EOF__ && nullParam == false) {
        TOKEN *currentToken = &(*tokens)[idenEndPos];

        if ((int)PG_is_operator(currentToken) == true) {
            int isFunctionCall = (int)PG_is_function_call(tokens, idenEndPos);

            if (isFunctionCall == false) {
                break;
            } else {
                break;
            }

            idenEndPos += isFunctionCall + 1;
        }

        idenEndPos++;
    }
    
    char *cache = NULL;
    size_t cacheSize = 0;

    for (size_t i = idenStartPos; i < idenEndPos; i++) {
        TOKEN *currentToken = &(*tokens)[i];

        if (cache == NULL) {
            //The currentToken->size already includes a +1 for the '\0'
            cache = (char*)malloc(sizeof(char) * currentToken->size);

            if (cache == NULL) {
                (void)printf("ERROR! (CACHE MALLOC)\n");
            }
            //currentToken->value already has '\0'
            (void)strncpy(cache, currentToken->value, currentToken->size);
            cacheSize = currentToken->size;
        } else {
            cache = (char*)realloc(cache, sizeof(char) * (cacheSize + currentToken->size - 1));

            if (cache == NULL) {
                (void)printf("ERROR! (CACHE REALLOC)\n");
            }

            (void)strncat(cache, currentToken->value, currentToken->size);
            cacheSize += currentToken->size;
        }
    }

    struct idenValRet ret = {cache, idenEndPos - idenStartPos};
    return ret;
}

/*
Purpose: Predicts whether the following tokens are a function call or not
Return Type: int => true = is function call; false = not a function call
Params: TOKEN **tokens => Pointer to the tokens array;
        size_t startPos => Position from where to start checking
*/
int PG_is_function_call(TOKEN **tokens, size_t startPos) {
    if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_
        && (*tokens)[startPos + 1].type == _OP_LEFT_BRACKET_) {
        return true;
    } else if ((*tokens)[startPos].type == _OP_LEFT_BRACKET_
        && (*tokens)[startPos - 1].type == _OP_RIGHT_BRACKET_) {
        return true;
    }

    int mover = 0;
    int openBrackets = 0;

    if ((*tokens)[startPos].type == _OP_LEFT_BRACKET_) {
        while (startPos - mover > 0) {
            if ((*tokens)[startPos - mover].type == _OP_LEFT_BRACKET_) {
                openBrackets--;
            } else if ((*tokens)[startPos - mover].type == _OP_RIGHT_BRACKET_) {
                openBrackets++;

                if (openBrackets == 0
                    && (*tokens)[startPos - mover - 1].type == _IDENTIFIER_) {
                    return mover;
                }
            }

            mover++;
        }
    } else if ((*tokens)[startPos].type == _OP_RIGHT_BRACKET_) {
        while ((*tokens)[startPos + mover].type != __EOF__) {
            if ((*tokens)[startPos + mover].type == _OP_LEFT_BRACKET_) {
                openBrackets--;

                if (openBrackets == 0
                    && (*tokens)[startPos - 1].type == _IDENTIFIER_) {
                    return mover;
                }
            } else if ((*tokens)[startPos + mover].type == _OP_RIGHT_BRACKET_) {
                openBrackets++;
            }

            mover++;
        }
    }

    return false;
}

/*
Purpose: Create a node based on the params
Return Type: struct Node * => Pointer to the Node
Params: char *value => Value of the node;
        enum NodeType type => Type of the Node
*/
struct Node *PG_create_node(char *value, enum NodeType type) {
    struct Node *node = (struct Node*)malloc(sizeof(struct Node));

    if (node == NULL) {
        (void)PARSE_TREE_NODE_RESERVATION_EXCEPTION();
    }

    node->type = type;
    node->value = value;
    node->leftNode = NULL;
    node->rightNode = NULL;
    node->details = NULL;
    node->detailsCount = 0;
    return node;
}

/*
Purpose: Check if a given TOKEN is an operator or not
Return Type: int => true = is operator; false = not an operator
Params: const TOKEN *token => Token to be checked
*/
const TOKENTYPES operators[] = {
_OP_PLUS_, _OP_MINUS_, _OP_MULTIPLY_, _OP_DIVIDE_, _OP_MODULU_,
_OP_LEFT_BRACKET_, _OP_RIGHT_BRACKET_, _OP_EQUALS_, _OP_SEMICOLON_,
_OP_COMMA_, _OP_RIGHT_BRACE_, _OP_DOT_};

int PG_is_operator(const TOKEN *token) {
    if (token->type == __EOF__) {
        return true;
    }

    for (int i = 0; i < (sizeof(operators) / sizeof(operators[0])); i++) {
        if (token->type == operators[i]
            || (int)PG_is_condition_operator(token->type) == true) {
            return true;
        }
    }

    return false;
}

/*
Purpose: Creates a NodeReport containing the topNode and how many tokens got processed
Return Type: NodeReport => Created NodeReport
Params: struct Node *topNode => TopNode;
        int tokensToSkip => How many tokens should be skipped
*/
NodeReport PG_create_node_report(struct Node *topNode, int tokensToSkip) {
    NodeReport report;
    report.node = topNode;
    report.tokensToSkip = tokensToSkip;
    return report;
}

/*
Purpose: Appends a Node to the RootNode
Return Type: void
Params: struct Node *node => Node to be appended
*/
void PG_append_node_to_root_node(struct Node *node) {
    RootNode.nodeCount++;
    RootNode.nodes = realloc(RootNode.nodes, sizeof(struct Node) * (RootNode.nodeCount + 1));

    if (RootNode.nodes == NULL) {
        (void)PARSE_TREE_NODE_RESERVATION_EXCEPTION();
    }

    RootNode.nodes[RootNode.nodeCount] = node;
}

/*
Purpose: Free all nodes from the RootNode
ReturnType: int => Return 1 on finish
Params: void
*/
int FREE_NODES() {
    for (size_t i = 0; i < RootNode.nodeCount; i++) {
        FREE_NODE(RootNode.nodes[i]);
        free(RootNode.nodes[i]->value);
        free(RootNode.nodes[i]);
    }

    return 1;
}

/*
Purpose: Free all nodes from the parent Node
ReturnType: void
Params: struct Node *node => Node to be freed
*/
void FREE_NODE(struct Node *node) {
    if (node != NULL) {
        FREE_NODE(node->leftNode);
        FREE_NODE(node->rightNode);
        free(node->value);

        for (size_t i = 0; i < node->detailsCount; i++) {
            FREE_NODE(node->details[i]);
            free(node->details[i]);
        }

        free(node->details);

        free(node);
    }
}