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
#include "../headers/hashmap.h"
#include "../headers/list.h"
#include "../headers/parsetree.h"
#include "../headers/semantic.h"

/**
 * <p>
 * Defines the used booleans, 1 for true and 0 for false.
 * </p>
*/
#define true 1
#define false 0

enum ErrorType {
    NONE,
    ALREADY_DEFINED_EXCEPTION,
    NOT_DEFINED_EXCEPTION,
    TYPE_MISMATCH_EXCEPTION,
    STATEMENT_MISPLACEMENT_EXCEPTION,
    WRONG_ACCESSOR_EXCEPTION,
    WRONG_ARGUMENT_EXCPEPTION,
    MODIFIER_EXCEPTION,
    NO_SUCH_ARRAY_DIMENSION_EXCEPTION
};

enum FunctionCallType {
    FNC_CALL,
    CONSTRUCTOR_CALL,
    CONSTRUCTOR_CHECK_CALL
};

enum ErrorStatus {
    SUCCESS,
    ERROR,
    NA
};

struct MemberAccessList {
    size_t size;
    Node **nodes;
};

struct ParamTransferObject {
    size_t params;
    SemanticEntry **entries;
};

struct SemanticReport {
    enum ErrorStatus status;
    struct VarDec dec;
    struct Node *errorNode;
    enum ErrorType errorType;
    char *description;
};

struct SemanticEntryReport {
    int success;
    int errorOccured;
    SemanticEntry *entry;
};

struct varTypeLookup {
    char name[12];
    enum VarType type;
};

struct varTypeLookup TYPE_LOOKUP[] = {
    {"int", INTEGER}, {"double", DOUBLE}, {"float", FLOAT},
    {"short", SHORT}, {"long", LONG}, {"char", CHAR},
    {"boolean", BOOLEAN}, {"String", STRING}, {"void", VOID}
};

void SA_init_globals();
void SA_manage_runnable(Node *root, SemanticTable *table);
void SA_add_parameters_to_runnable_table(SemanticTable *scopeTable, struct ParamTransferObject *params);

struct SemanticReport SA_evaluate_function_call(Node *topNode, SemanticEntry *functionEntry, SemanticTable *callScopeTable, enum FunctionCallType fnccType);
void SA_add_class_to_table(SemanticTable *table, Node *classNode);
void SA_add_function_to_table(SemanticTable *table, Node *functionNode);
void SA_add_normal_variable_to_table(SemanticTable *table, Node *varNode);
void SA_add_instance_variable_to_table(SemanticTable *table, Node *varNode);
void SA_add_constructor_to_table(SemanticTable *table, Node *constructorNode);
void SA_add_enum_to_table(SemanticTable *table, Node *enumNode);
void SA_add_enumerators_to_enum_table(SemanticTable *enumTable, Node *topNode);
void SA_add_include_to_table(SemanticTable *table, Node *includeNode);
void SA_add_try_statement(SemanticTable *table, Node *tryNode, Node *parentNode, int index);
void SA_add_catch_statement(SemanticTable *table, Node *catchNode, Node *parentNode, int index);
void SA_add_while_or_do_to_table(SemanticTable *table, Node *whileDoNode);
void SA_add_if_to_table(SemanticTable *table, Node *ifNode);
void SA_add_else_if_to_table(SemanticTable *table, Node *elseIfNode, Node *parentNode, int index);
void SA_add_else_to_table(SemanticTable *table, Node *elseNode, Node *parentNode, int index);
void SA_check_break_or_continue_to_table(SemanticTable *table, Node *breakOrContinueNode);

int SA_is_break_or_continue_placement_valid(SemanticTable *table);
char *SA_get_string(char bufferString[]);
struct SemanticReport SA_evaluate_chained_condition(SemanticTable *table, Node *rootNode);
int SA_is_obj_already_defined(char *key, SemanticTable *scopeTable);
SemanticTable *SA_get_next_table_of_type(SemanticTable *currentTable, enum ScopeType type);
struct SemanticReport SA_contains_constructor_of_type(SemanticTable *classTable, struct Node *paramHolder, enum FunctionCallType fnccType);
int SA_get_node_param_count(struct Node *paramHolder);

struct SemanticReport SA_evaluate_assignment(struct VarDec expectedType, Node *topNode, SemanticTable *table);
struct SemanticReport SA_evaluate_simple_term(struct VarDec expectedType, Node *topNode, SemanticTable *table);
struct SemanticReport SA_evaluate_term_side(struct VarDec expectedType, Node *node, SemanticTable *table);

SemanticTable *SA_create_new_scope_table(Node *root, enum ScopeType scope, SemanticTable *parent, struct ParamTransferObject *params, size_t line, size_t position);
int SA_is_node_arithmetic_operator(Node *node);
int SA_are_VarTypes_equal(struct VarDec type1, struct VarDec type2, int strict);
int SA_are_strict_VarTypes_equal(struct VarDec type1, struct VarDec type2);
int SA_are_non_strict_VarTypes_equal(struct VarDec type1, struct VarDec type2);

struct ParamTransferObject *SA_get_params(Node *topNode, struct VarDec stdType);
struct SemanticReport SA_evaluate_member_access(Node *topNode, SemanticTable *table);
struct SemanticReport SA_check_restricted_member_access(Node *node, SemanticTable *table, SemanticTable *topScope);
struct SemanticReport SA_check_non_restricted_member_access(Node *node, SemanticTable *table, SemanticTable *topScope);
struct SemanticReport SA_handle_array_accesses(struct VarDec *currentType, struct Node *arrayAccStart, SemanticTable *table);
struct SemanticReport SA_execute_identifier_analysis(Node *currentNode, SemanticTable *callScopeTable, struct VarDec *currentNodeType, SemanticEntry *currentEntryParam, enum FunctionCallType fnccType);
struct SemanticReport SA_execute_function_call_precheck(SemanticTable *ref, Node *topNode,  enum FunctionCallType fnccType);
struct SemanticReport SA_evaluate_modifier(SemanticTable *currentScope, enum Visibility vis, Node *node, SemanticTable *topTable);

struct SemanticReport SA_execute_access_type_checking(Node *cacheNode, SemanticTable *currentScope, SemanticTable *topScope);
SemanticTable *SA_get_next_table_with_declaration(Node *node, SemanticTable *table);
struct SemanticEntryReport SA_get_entry_if_available(Node *topNode, SemanticTable *table);
struct VarDec SA_convert_identifier_to_VarType(Node *node);
struct VarDec SA_get_VarType(Node *node, int constant);
enum Visibility SA_get_visibility(Node *visibilityNode);
char *SA_get_VarType_string(struct VarDec type);
char *SA_get_ScopeType_string(enum ScopeType type);
SemanticEntry *SA_get_param_entry_if_available(char *key, SemanticTable *table);

struct SemanticEntryReport SA_create_semantic_entry_report(SemanticEntry *entry, int success, int errorOccured);
struct SemanticReport SA_create_semantic_report(struct VarDec type, enum ErrorStatus status, Node *errorNode, enum ErrorType errorType, char *description);
SemanticEntry *SA_create_semantic_entry(char *name, struct VarDec varType, enum Visibility visibility, enum ScopeType internalType, void *ptr, size_t line, size_t position);
SemanticTable *SA_create_semantic_table(int paramCount, int symbolTableSize, SemanticTable *parent, enum ScopeType type, size_t line, size_t position);

void FREE_TABLE(SemanticTable *rootTable);

struct SemanticReport SA_create_expected_got_report(struct VarDec expected, struct VarDec got, Node *errorNode);

void THROW_NO_SUCH_ARRAY_DIMENSION_EXCEPTION(struct SemanticReport rep);
void THROW_MODIFIER_EXCEPTION(struct SemanticReport rep);
void THROW_WRONG_ARGUMENT_EXCEPTION(struct SemanticReport rep);
void THROW_WRONG_ACCESSOR_EXEPTION(struct SemanticReport rep);
void THROW_STATEMENT_MISPLACEMENT_EXEPTION(struct SemanticReport rep);
void THROW_TYPE_MISMATCH_EXCEPTION(struct SemanticReport rep);
void THROW_NOT_DEFINED_EXCEPTION(Node *node);
void THROW_ALREADY_DEFINED_EXCEPTION(Node *node);
void THROW_MEMORY_RESERVATION_EXCEPTION(char *problemPosition);
void THROW_EXCEPTION(char *message, struct SemanticReport rep);
void THROW_ASSIGNED_EXCEPTION(struct SemanticReport rep);

extern char *FILE_NAME;
extern char **BUFFER;
extern size_t BUFFER_LENGTH;

struct VarDec nullDec = {null, 0, NULL, false};
struct VarDec externalDec = {EXTERNAL_RET, 0, NULL};
struct SemanticReport nullRep;

/**
 * <p>
 * This list holds all member accesses or class accesses that
 * are in an external file. Ready to checked by the linker.
 * </p>
 */
struct List *listOfExternalAccesses = NULL;

int CheckSemantic(Node *root) {
    (void)SA_init_globals();

    SemanticTable *mainTable = SA_create_new_scope_table(root, MAIN, NULL, NULL, 0, 0);
    (void)SA_manage_runnable(root, mainTable);
    (void)FREE_TABLE(mainTable);
    return 1;
}

void SA_init_globals() {
    nullRep = SA_create_semantic_report(nullDec, SUCCESS, NULL, NONE, NULL);
    listOfExternalAccesses = CreateNewList(16);
}

void SA_manage_runnable(Node *root, SemanticTable *table) {
    printf("Main instructions count: %li\n", root->detailsCount);
    
    for (int i = 0; i < root->detailsCount; i++) {
        Node *currentNode = root->details[i];

        switch (currentNode->type) {
        case _VAR_NODE_:
        case _CONST_NODE_:
            (void)SA_add_normal_variable_to_table(table, currentNode);
            break;
        case _FUNCTION_NODE_:
            (void)SA_add_function_to_table(table, currentNode);
            break;
        case _CLASS_NODE_:
            (void)SA_add_class_to_table(table, currentNode);
            break;
        case _VAR_CLASS_INSTANCE_NODE_:
            (void)SA_add_instance_variable_to_table(table, currentNode);
            break;
        case _CLASS_CONSTRUCTOR_NODE_:
            (void)SA_add_constructor_to_table(table, currentNode);
            break;
        case _ENUM_NODE_:
            (void)SA_add_enum_to_table(table, currentNode);
            break;
        case _INCLUDE_NODE_:
            (void)SA_add_include_to_table(table, currentNode);
            break;
        case _TRY_NODE_:
            (void)SA_add_try_statement(table, currentNode, root, i);
            break;
        case _CATCH_NODE_:
            (void)SA_add_catch_statement(table, currentNode, root, i);
            break;
        case _WHILE_STMT_NODE_:
        case _DO_STMT_NODE_:
            (void)SA_add_while_or_do_to_table(table, currentNode);
            break;
        case _IF_STMT_NODE_:
            (void)SA_add_if_to_table(table, currentNode);
            break;
        case _ELSE_IF_STMT_NODE_:
            (void)SA_add_else_if_to_table(table, currentNode, root, i);
            break;
        case _ELSE_STMT_NODE_:
            (void)SA_add_else_to_table(table, currentNode, root, i);
            break;
        case _CONTINUE_STMT_NODE_:
        case _BREAK_STMT_NODE_:
            (void)SA_check_break_or_continue_to_table(table, currentNode);
            break;
        default: continue;
        }
    }

    /*char *search = "add";
    struct HashMapEntry *entry = HM_get_entry(search, mainTable->symbolTable);
    printf("\n\nEntry found: %s\n\n", entry == NULL ? "(null)" : "availabe");
    SemanticEntry *sentry = entry->value;
    printf("(%s) Name: %s, Value: %s, Vis: %i, Type: %i, Internal Type: %i, Reference: %p\n", search, sentry->name, sentry->value, sentry->visibility, sentry->type, sentry->internalType, (void*)sentry->reference);
    */
}

/**
 * <p>
 * Adds all parameters that are included in the ParameterTransferObject
 * into the parameter table of the local SemanticTable.
 * </p>
 * 
 * @param *params       Transfer object to add
 * @param *scopeTable   The current scope table
 */
void SA_add_parameters_to_runnable_table(SemanticTable *scopeTable, struct ParamTransferObject *params) {
    if (params == NULL) {
        return;
    }
    
    for (int i = 0; i < params->params; i++) {
        SemanticEntry *entry = params->entries[i];
        (void)L_add_item(scopeTable->paramList, entry);
    }

    (void)free(params->entries);
    (void)free(params);
}

void SA_add_class_to_table(SemanticTable *table, Node *classNode) {
    if (table->type != MAIN) {
        char *msg = "Classes have to be in the outest scope.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, classNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    char *name = classNode->value;
    enum Visibility vis = SA_get_visibility(classNode->leftNode);
    struct VarDec std = {EXT_CLASS_OR_INTERFACE, 0, NULL};
    struct ParamTransferObject *params = SA_get_params(classNode, std);
    Node *runnableNode = classNode->rightNode;

    if ((int)SA_is_obj_already_defined(name, table) == true) {
        (void)THROW_ALREADY_DEFINED_EXCEPTION(classNode);
        return;
    }

    SemanticTable *scopeTable = SA_create_new_scope_table(runnableNode, CLASS, table, params, classNode->line, classNode->position);
    scopeTable->name = name;
    
    SemanticEntry *referenceEntry = SA_create_semantic_entry(name, nullDec, vis, CLASS, scopeTable, classNode->line, classNode->position);
    (void)HM_add_entry(name, referenceEntry, table->symbolTable);
    (void)SA_manage_runnable(runnableNode, scopeTable);
}

void SA_add_function_to_table(SemanticTable *table, Node *functionNode) {
    if (table->type != MAIN && table->type != CLASS) {
        char *msg = "Functions are only allowed in classes and the outest scope.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, functionNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    char *name = functionNode->value;
    enum Visibility vis = SA_get_visibility(functionNode->leftNode);
    struct VarDec type = SA_get_VarType(functionNode->details == NULL ? NULL : functionNode->details[0], false);
    int paramsCount = functionNode->detailsCount - 1; //-1 because of the runnable
    struct VarDec std = {VARIABLE, 0, NULL};
    struct ParamTransferObject *params = SA_get_params(functionNode, std);
    Node *runnableNode = functionNode->details[paramsCount];
    SemanticTable *scopeTable = SA_create_new_scope_table(runnableNode, FUNCTION, table, params, functionNode->line, functionNode->position);
    scopeTable->name = name;

    SemanticEntry *referenceEntry = SA_create_semantic_entry(name, type, vis, FUNCTION, scopeTable, functionNode->line, functionNode->position);
    (void)HM_add_entry(name, referenceEntry, table->symbolTable);
    (void)SA_manage_runnable(runnableNode, scopeTable);
}

/**
 * <p>
 * Adds a variable as an entry into the current
 * Semantic table.
 * </p>
 * 
 * @param *table    Table to add the variable to
 * @param *varNode  AST-Node that defines a variable
 */
void SA_add_normal_variable_to_table(SemanticTable *table, Node *varNode) {
    if (table->type == ENUM) {
        char *msg = "Vars are not allowed within enums.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, varNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    char *name = varNode->value;
    enum Visibility vis = SA_get_visibility(varNode->leftNode);
    int constant = varNode->type == _VAR_NODE_ ? false : true;
    struct VarDec type = SA_get_VarType(varNode->details == NULL ? NULL : varNode->details[0], constant);
    SemanticTable *actualTable = table->type == TRY ? table->parent : table;

    if ((int)SA_is_obj_already_defined(name, actualTable) == true) {
        (void)THROW_ALREADY_DEFINED_EXCEPTION(varNode);
        return;
    }

    if (varNode->rightNode != NULL) {
        struct SemanticReport assignmentRep = SA_evaluate_assignment(type, varNode->rightNode, actualTable);

        if (assignmentRep.status == ERROR) {
            (void)THROW_ASSIGNED_EXCEPTION(assignmentRep);
            return;
        }
    }

    SemanticEntry *entry = SA_create_semantic_entry(name, type, vis, VARIABLE, NULL, varNode->line, varNode->position);
    (void)HM_add_entry(name, entry, table->symbolTable);
}

void SA_add_instance_variable_to_table(SemanticTable *table, Node *varNode) {
    char *name = varNode->value;
    enum Visibility vis = SA_get_visibility(varNode->leftNode);
    struct VarDec type = {CLASS_REF, 0, varNode->value};
    SemanticTable *actualTable = table->type == TRY ? table->parent : table;

    if ((int)SA_is_obj_already_defined(name, actualTable) == true) {
        (void)THROW_ALREADY_DEFINED_EXCEPTION(varNode);
        return;
    }

    if (varNode->rightNode != NULL) {
        struct Node *valueNode = varNode->rightNode;

        if (valueNode->type == _FUNCTION_CALL_NODE_) {
            SemanticTable *topTable = SA_get_next_table_of_type(table, MAIN);
            struct SemanticEntryReport classEntry = SA_get_entry_if_available(valueNode, topTable);

            if (classEntry.entry == NULL) {
                (void)THROW_NOT_DEFINED_EXCEPTION(valueNode);
                return;
            }

            SemanticTable *classTable = (SemanticTable*)classEntry.entry->reference;
            struct SemanticReport containsConstructor = SA_contains_constructor_of_type(classTable, valueNode, CONSTRUCTOR_CHECK_CALL);

            if (containsConstructor.status == NA) {
                (void)THROW_NOT_DEFINED_EXCEPTION(valueNode);
                return;
            }
        }
    }

    SemanticEntry *entry = SA_create_semantic_entry(name, type, vis, VARIABLE, NULL, varNode->line, varNode->position);
    (void)HM_add_entry(name, entry, table->symbolTable);
}

void SA_add_constructor_to_table(SemanticTable *table, Node *constructorNode) {
    if (table->type != CLASS) {
        char *msg = "Constructors are only allowed in classes.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, constructorNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    struct SemanticReport hasConstructor = SA_contains_constructor_of_type(table, constructorNode, CONSTRUCTOR_CALL);

    if ((int)hasConstructor.status == SUCCESS) {
        (void)THROW_ALREADY_DEFINED_EXCEPTION(constructorNode);
        return;
    }

    char *name = SA_get_string("Constructor");
    struct Node *runnableNode = constructorNode->rightNode;
    struct VarDec cust = {CUSTOM, 0, NULL};
    struct VarDec constructDec = {CONSTRUCTOR_PARAM, 0, NULL};
    struct ParamTransferObject *params = SA_get_params(constructorNode, cust);
    SemanticTable *scopeTable = SA_create_new_scope_table(constructorNode, CONSTRUCTOR, table, params, constructorNode->line, constructorNode->position);
    SemanticEntry *entry = SA_create_semantic_entry(name, constructDec, GLOBAL, CONSTRUCTOR, scopeTable, constructorNode->line, constructorNode->position);
    (void)L_add_item(table->paramList, entry);
    (void)SA_manage_runnable(runnableNode, scopeTable);
}

void SA_add_enum_to_table(SemanticTable *table, Node *enumNode) {
    if (table->type != MAIN) {
        char *msg = "Enums have to be in the outest scope.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, enumNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    char *name = enumNode->value;
    enum Visibility vis = table->type == MAIN ? P_GLOBAL : GLOBAL;

    if ((int)SA_is_obj_already_defined(name, table) == true) {
        (void)THROW_ALREADY_DEFINED_EXCEPTION(enumNode);
        return;
    }

    SemanticTable *scopeTable = SA_create_new_scope_table(enumNode, ENUM, table, NULL, enumNode->line, enumNode->position);
    (void)SA_add_enumerators_to_enum_table(scopeTable, enumNode);
    SemanticEntry *entry = SA_create_semantic_entry(name, nullDec, vis, ENUM, scopeTable, enumNode->line, enumNode->position);
    (void)HM_add_entry(name, entry, table->symbolTable);
}

void SA_add_enumerators_to_enum_table(SemanticTable *enumTable, struct Node *topNode) {
    struct VarDec enumDec = {INTEGER, 0, NULL};
    
    for (int i = 0; i < topNode->detailsCount; i++) {
        struct Node *enumerator = topNode->details[i];

        if (enumerator == NULL) {
            continue;
        }

        char *name = enumerator->value;

        if ((int)HM_contains_key(name, enumTable->symbolTable) == true) {
            (void)THROW_ALREADY_DEFINED_EXCEPTION(enumerator);
            return;
        }

        struct SemanticEntry *entry = SA_create_semantic_entry(name, enumDec, P_GLOBAL, ENUMERATOR, NULL, enumerator->line, enumerator->position);
        (void)HM_add_entry(name, entry, enumTable->symbolTable);
    }
}

void SA_add_include_to_table(SemanticTable *table, struct Node *includeNode) {
    if (table->type != MAIN) {
        char *msg = "Includes have to be in the outest scope.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, includeNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    struct Node *actualInclude = NULL;
    struct Node *cacheNode = includeNode;

    while (cacheNode != NULL) {
        actualInclude = cacheNode->leftNode;
        cacheNode = cacheNode->rightNode;
    }

    char *name = actualInclude->value;
    struct SemanticEntry *entry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, EXTERNAL, NULL, includeNode->line, includeNode->position);
    
    if ((int)SA_is_obj_already_defined(name, table) == true) {
        (void)THROW_ALREADY_DEFINED_EXCEPTION(includeNode);
        return;
    }
    
    (void)HM_add_entry(name, entry, table->symbolTable);
    (void)L_add_item(listOfExternalAccesses, includeNode);
}

/**
 * <p>
 * Evaluates a try statement for correctness.
 * </p>
 * 
 * @param *table        The parent table
 * @param *tryNode      The node with the try runnable
 * @param *parentNode   Node abover the catchNode (evaluates if the statement is before a "catch" statement)
 * @param index         Index at the parent node (evaluates if the statement is before a "catch" statement)
 */
void SA_add_try_statement(SemanticTable *table, Node *tryNode, Node *parentNode, int index) {
    if (table->type == ENUM) {
        char *msg = "Try statements are not allowed in enums.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, tryNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    struct Node *estimatedCatchNode = parentNode->details[index + 1];

    if (estimatedCatchNode == NULL
        || estimatedCatchNode->type != _CATCH_NODE_) {
        char *msg = "Try statements have to have a catch statement.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, tryNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
    }

    char *name = SA_get_string("try");
    SemanticTable *tempTable = SA_create_new_scope_table(NULL, TRY, table, NULL, tryNode->line, tryNode->position);
    struct SemanticEntry *tryEntry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, WHILE, tempTable, tryNode->line, tryNode->position);
    (void)HM_add_entry(name, tryEntry, table->symbolTable);
    (void)SA_manage_runnable(tryNode, tempTable);
}

/**
 * <p>
 * Evaluates a catch statement for correctness.
 * </p>
 * 
 * @param *table        The parent table
 * @param *catchNode    The node with the catch runnable and error node
 * @param *parentNode   Node abover the catchNode (evaluates if the statement is after a "try" statement)
 * @param index         Index at the parent node (evaluates if the statement is after a "try" statement)
 */
void SA_add_catch_statement(SemanticTable *table, Node *catchNode, Node *parentNode, int index) {
    if (table->type == ENUM) {
        char *msg = "Catch statements are not allowed in enums.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, catchNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    struct Node *estimatedTryNode = parentNode->details[index - 1];

    if (estimatedTryNode == NULL
        || estimatedTryNode->type != _TRY_NODE_) {
        char *msg = "Catch statements have to be placed after a try statement.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, catchNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    char *name = SA_get_string("catch");
    SemanticTable *tempTable = SA_create_new_scope_table(catchNode->rightNode, CATCH, table, NULL, catchNode->line, catchNode->position);
    Node *errorHandleNode = catchNode->leftNode;
    struct VarDec errorType = {CLASS_REF, 0, errorHandleNode->leftNode->value, true};
    struct SemanticEntry *param = SA_create_semantic_entry(errorHandleNode->value, errorType, P_GLOBAL, VARIABLE, NULL, errorHandleNode->line, errorHandleNode->position);
    (void)L_add_item(tempTable->paramList, param);
    struct SemanticEntry *catchEntry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, WHILE, tempTable, catchNode->line, catchNode->position);
    (void)HM_add_entry(name, catchEntry, table->symbolTable);
    (void)SA_manage_runnable(catchNode->rightNode, tempTable);
}

void SA_add_while_or_do_to_table(SemanticTable *table, Node *whileDoNode) {
    if (table->type == ENUM) {
        char *msg = "While and Do statements are not allowed in enums.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, whileDoNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    struct SemanticReport conditionRep = SA_evaluate_chained_condition(table, whileDoNode->leftNode);

    if (conditionRep.status == ERROR) {
        (void)THROW_ASSIGNED_EXCEPTION(conditionRep);
        return;
    }

    char *name = whileDoNode->type == _WHILE_STMT_NODE_ ? SA_get_string("while") : SA_get_string("do");
    enum ScopeType type = whileDoNode->type == _WHILE_STMT_NODE_ ? WHILE : DO;
    SemanticTable *whileTable = SA_create_new_scope_table(whileDoNode->rightNode, type, table, NULL, whileDoNode->line, whileDoNode->position);
    struct SemanticEntry *whileEntry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, type, whileTable, whileDoNode->line, whileDoNode->position);
    (void)HM_add_entry(name, whileEntry, table->symbolTable);
    (void)SA_manage_runnable(whileDoNode->rightNode, whileTable);
}

void SA_add_if_to_table(SemanticTable *table, Node *ifNode) {
    if (table->type == ENUM) {
        char *msg = "If statements are not allowed in enums.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, ifNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
    }

    struct SemanticReport conditionRep = SA_evaluate_chained_condition(table, ifNode->leftNode);

    if (conditionRep.status == ERROR) {
        (void)THROW_ASSIGNED_EXCEPTION(conditionRep);
        return;
    }

    char *name = SA_get_string("if");
    SemanticTable *whileTable = SA_create_new_scope_table(ifNode->rightNode, IF, table, NULL, ifNode->line, ifNode->position);
    struct SemanticEntry *ifEntry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, IF, whileTable, ifNode->line, ifNode->position);
    (void)HM_add_entry(name, ifEntry, table->symbolTable);
    (void)SA_manage_runnable(ifNode->rightNode, whileTable);
}

void SA_add_else_if_to_table(SemanticTable *table, Node *elseIfNode, Node *parentNode, int index) {
    struct Node *estimatedIfNode = parentNode->details[index - 1];

    if (estimatedIfNode->type != _IF_STMT_NODE_
        && estimatedIfNode->type != _ELSE_IF_STMT_NODE_) {
        char *msg = "Else-if statements are only allowed after an if and else-if statement.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, elseIfNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    struct SemanticReport conditionRep = SA_evaluate_chained_condition(table, elseIfNode->leftNode);

    if (conditionRep.status == ERROR) {
        (void)THROW_ASSIGNED_EXCEPTION(conditionRep);
        return;
    }

    char *name = SA_get_string("else_if");
    SemanticTable *whileTable = SA_create_new_scope_table(elseIfNode->rightNode, ELSE_IF, table, NULL, elseIfNode->line, elseIfNode->position);
    struct SemanticEntry *elseIfEntry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, ELSE_IF, whileTable, elseIfNode->line, elseIfNode->position);
    (void)HM_add_entry(name, elseIfEntry, table->symbolTable);
    (void)SA_manage_runnable(elseIfNode->rightNode, whileTable);
}

void SA_add_else_to_table(SemanticTable *table, Node *elseNode, Node *parentNode, int index) {
    struct Node *estimatedIfOrElseIfNode = parentNode->details[index - 1];

    if (estimatedIfOrElseIfNode->type != _IF_STMT_NODE_
        && estimatedIfOrElseIfNode->type != _ELSE_IF_STMT_NODE_) {
        char *msg = "Else statements are only allowed after an if and else-if statement.";
        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, elseNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        return;
    }

    char *name = SA_get_string("else");
    SemanticTable *whileTable = SA_create_new_scope_table(elseNode->rightNode, ELSE, table, NULL, elseNode->line, elseNode->position);
    struct SemanticEntry *elseEntry = SA_create_semantic_entry(name, nullDec, P_GLOBAL, ELSE, whileTable, elseNode->line, elseNode->position);
    (void)HM_add_entry(name, elseEntry, table->symbolTable);
    (void)SA_manage_runnable(elseNode->rightNode, whileTable);
}

void SA_check_break_or_continue_to_table(SemanticTable *table, Node *breakOrContinueNode) {
    if ((int)SA_is_break_or_continue_placement_valid(table) == false) {
        char *msg = NULL;

        if (breakOrContinueNode->type == _BREAK_STMT_NODE_) {
            msg = "Breaks are only allowed within a loop scope.\0";
        } else {
            msg = "Continues are only allowed within a loop scope.\0";
        }

        struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, breakOrContinueNode, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
    }
}

/**
 * <p>
 * Checks the placement of the break or continue statement.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - If the statement is placed correctly
 * <li>false - If the statement is place incorrectly
 * </ul>
 * 
 * @param *table    The innerscope from which to work upwards, till a loop is detected
 */
int SA_is_break_or_continue_placement_valid(SemanticTable *table) {
    SemanticTable *temp = table;
    int metLoop = false;

    while (temp != NULL) {
        switch (temp->type) {
        case FOR: case WHILE: case DO:
        case IS:
            metLoop = true;
            break;
        case IF: case ELSE_IF: case ELSE:
        case TRY: case CATCH:
            break;
        default:
            temp = NULL;
            break;
        }

        temp = temp == NULL ? NULL : temp->parent;
    }

    return metLoop;
}

/**
 * <p>
 * Evaluates a chained condition for correctness.
 * </p>
 * 
 * <p>
 * It goes down the chained condition tree recursively by checkinig for the 'and' and 'or'
 * keyword. If thse are found another recursion happens, else the resulting terms are
 * checked. There's no need to check the rational operator, due to syntax analysis.
 * </p>
 * 
 * @returns A SeamnticReport with possible errors.
 * 
 * @param *table        Table in which the condition is called from
 * @param *rootNode     The root of the condition tree
 */
struct SemanticReport SA_evaluate_chained_condition(SemanticTable *table, Node *rootNode) {
    if (rootNode->type == _OR_NODE_
        || rootNode->type == _AND_NODE_) {
        struct SemanticReport leftCond = SA_evaluate_chained_condition(table, rootNode->leftNode);
        struct SemanticReport rightCond = SA_evaluate_chained_condition(table, rootNode->rightNode);

        if (leftCond.status == ERROR) {
            return leftCond;
        } else if (rightCond.status == ERROR) {
            return rightCond;
        }

        return nullRep;
    } else {
        struct VarDec cust = {CUSTOM, 0, NULL};
        struct SemanticReport lVal = SA_evaluate_simple_term(cust, rootNode->leftNode, table);
        struct SemanticReport rVal = SA_evaluate_simple_term(cust, rootNode->rightNode, table);

        if (lVal.status == ERROR) {
            return lVal;
        } else if (rVal.status == ERROR) {
            return rVal;
        }

        return nullRep;
    }
}

char *SA_get_string(char bufferString[]) {
    int length = (int)strlen(bufferString);
    char *string = (char*)calloc(length + 1, sizeof(char));

    if (string == NULL) {
        (void)THROW_MEMORY_RESERVATION_EXCEPTION("toString");
        return NULL;
    }

    for (int i = 0; i < length; i++) {
        string[i] = bufferString[i];
    }

    string[length] = '\0';
    return string;
}

/**
 * <p>
 * Checks if a constructor with the exact same types is already defined or not.
 * </p>
 * 
 * <p>
 * The types have to be different for the constructor to be recognized as "different".
 * 
 * Example:
 * 
 * ```
 * this::constructor(param1, param2) {}
 * this::constructor(number1, number2) {}
 * => ERROR, because params are of equal types
 * 
 * this::constructor(param1:int, param2:char) {}
 * this::constructor(param1:int, param2:double) {}
 * => ALLOWED, due to different types
 * ```
 * </p>
 * 
 * @returns A SemanticReport with possible errors and if the class contains another constructor
 * of the same type or not.
 * 
 * @param *classTable       The table in the current class with ass other constructors
 * @param *paramHolder      The new constructor node, that holds the params
 * @param fncctype          Determines the "strictness", for declarations it is stricter than checks
 */
struct SemanticReport SA_contains_constructor_of_type(SemanticTable *classTable, struct Node *paramHolder, enum FunctionCallType fncctype) {
    if (classTable == NULL || paramHolder == NULL) {
        return SA_create_semantic_report(nullDec, NA, NULL, NONE, NULL);
    }

    int actualNodeParamCount = (int)SA_get_node_param_count(paramHolder);

    for (int i = 0; i < classTable->paramList->load; i++) {
        SemanticEntry *entry = (SemanticEntry*)L_get_item(classTable->paramList, i);

        if (entry == NULL) {
            continue;
        } else if (entry->dec.type != CONSTRUCTOR_PARAM) {
            continue;
        }
        
        SemanticTable *entryTable = (SemanticTable*)entry->reference;

        if (entryTable == NULL) {
            continue;
        } else if (entryTable->paramList->load != actualNodeParamCount) {
            continue;
        }
        
        struct SemanticReport fncCallRep = SA_evaluate_function_call(paramHolder, entry, classTable, fncctype);
        
        //Checks if another constructor is already defined with the same parameter types,
        //on error there is, else not
        if (fncCallRep.status == ERROR) {
            continue;
        } else {
            fncCallRep.status = SUCCESS;
            return fncCallRep;
        }
    }

    return SA_create_semantic_report(nullDec, NA, NULL, NONE, NULL);
}

/**
 * <p>
 * Gets the parameter count of the provided node.
 * RUNNABLES and NULL nodes are excluded in the count.
 * </p>
 * 
 * @returns The number of parameters
 * 
 * @param *paramHolder  The node that holds the parameters
 */
int SA_get_node_param_count(struct Node *paramHolder) {
    int actualNodeParamCount = 0;

    for (int i = 0; i < paramHolder->detailsCount; i++) {
        if (paramHolder->details[i] == NULL) {
            continue;
        } if (paramHolder->details[i]->type == _RUNNABLE_NODE_) {
            continue;
        }

        actualNodeParamCount++;
    }

    return actualNodeParamCount;
}

/**
 * <p>
 * This function evaluates a simple term with the help of recursion.
 * </p>
 * 
 * <p>
 * First the topNode is checked for an arithmetic operator, if it is
 * not an arithmetic operator the one node is evaluated. If it is an
 * arithmetic operator the simple_term function is invoked again, until
 * the top node is not an arithmetic operator anymore.
 * </p>
 * 
 * <p><strong>Note:</strong>
 * This function also evaluates the optional typesafety!
 * </p>
 * 
 * @returns A report with possible errors
 * 
 * @param expectedType      Type to check for typesafety
 * @param *topNode          Current top node to check
 * @param *table            Current scope table
 */
struct SemanticReport SA_evaluate_simple_term(struct VarDec expectedType, Node *topNode, SemanticTable *table) {
    int isTopNodeArithmeticOperator = (int)SA_is_node_arithmetic_operator(topNode);
    
    if (isTopNodeArithmeticOperator == true) {
        struct SemanticReport leftTerm = SA_evaluate_simple_term(expectedType, topNode->leftNode, table);
        struct SemanticReport rightTerm = SA_evaluate_simple_term(expectedType, topNode->rightNode, table);
        
        if (leftTerm.status == ERROR) {
            return leftTerm;
        } else if (rightTerm.status == ERROR) {
            return rightTerm;
        }

        return nullRep;
    } else {
        return SA_evaluate_term_side(expectedType, topNode, table);
    }
}

/**
 * <p>
 * The function checks if a term side makes sense.
 * </p>
 * 
 * <p><strong>Allowed Objects:</strong>
 * <ul>
 * <li>Number
 * <li>Member access
 * <li>Class access
 * <li>Identifier
 * <li>Function call
 * </ul>
 * </p>
 * 
 * @returns A report with possible errors and the identified type
 * 
 * @param expectedType      Type to which the term should stick
 * @param *node             Node to check (root of subtree)
 * @param *table            Table in which the term is called
 */
struct SemanticReport SA_evaluate_term_side(struct VarDec expectedType, Node *node, SemanticTable *table) {
    struct VarDec predictedType = {CUSTOM, 0, NULL};
    
    switch (node->type) {
    case _NUMBER_NODE_:
    case _FLOAT_NODE_:
        predictedType = SA_convert_identifier_to_VarType(node);
        break;
    case _NULL_NODE_:
        predictedType = nullDec;
        break;
    case _STRING_NODE_:
        predictedType.type = STRING;
        break;
    case _CHAR_ARRAY_NODE_:
        if ((int)strlen(node->value) > 3) { //3 for 1 letter + 2 quotationmarks
            predictedType.type = STRING;
        } else {
            predictedType.type = CHAR;
        }

        break;
    case _MEM_CLASS_ACC_NODE_:
    case _IDEN_NODE_:
    case _FUNCTION_CALL_NODE_: {
        struct SemanticReport rep = SA_evaluate_member_access(node, table);

        if (rep.status == ERROR) {
            return rep;
        }

        predictedType = rep.dec;
        node = node->type == _MEM_CLASS_ACC_NODE_ ? node->leftNode : node;
        break;
    }
    case _BOOL_NODE_:
        predictedType.type = BOOLEAN;
        break;
    default: break;
    }

    if ((int)SA_are_VarTypes_equal(expectedType, predictedType, false) == false) {
        return SA_create_expected_got_report(expectedType, predictedType, node);
    }

    return SA_create_semantic_report(predictedType, SUCCESS, NULL, NONE, NULL);
}

/**
 * <p>
 * Evaluates a member acces as well as a class access.
 * </p>
 * 
 * <p>
 * Due to the structure of the trees the accesses are divided into two groups,
 * the first includes access with '.' or '->', while the other describes
 * itself without any access operator (objects are in local scope).
 * </p>
 * 
 * @return A report containing the analysis result, like error occured, success and resulting type
 * 
 * @param *topNode      Start node of the member/class access tree
 * @param *table        Table in which the expression was written in (current scope)
 */
struct SemanticReport SA_evaluate_member_access(Node *topNode, SemanticTable *table) {
    SemanticTable *topScope = NULL;
    struct SemanticReport rep;

    if (topNode->type == _MEM_CLASS_ACC_NODE_) {
        topScope = SA_get_next_table_with_declaration(topNode->leftNode, table);
        rep = SA_check_non_restricted_member_access(topNode, table, topScope);
    } else {
        topScope = SA_get_next_table_with_declaration(topNode, table);
        rep = SA_check_restricted_member_access(topNode, table, topScope);
    }

    printf(">>>> >>>> >>>> EXIT! (%i)\n", rep.dec.type);
    return rep.status == ERROR ? rep : SA_create_semantic_report(rep.dec, SUCCESS, NULL, NONE, NULL);
}

/**
 * <p>
 * Checks a member access tree with multiple accesses.
 * </p>
 * 
 * <p>
 * The function only goes down the tree, actual checking occures
 * here: #SA_check_restricted_member_access(Node *node, SemanticTable *table, SemanticTable *topScope).
 * </p>
 * 
 * <p>
 * Examples:
 * ```
 * this->a
 * test()
 * Math->add()
 * List->toList().getItem()
 * ```
 * </p>
 * 
 * @returns A SemanticReport with the analyzed type and flags for error an success
 * 
 * @param *node     Node to check
 * @param *table    The table from the scope, at which the member access occured
 * @param *topScope The current top scope in the process
 */
struct SemanticReport SA_check_non_restricted_member_access(Node *node, SemanticTable *table,
                                                            SemanticTable *topScope) {
    SemanticTable *currentScope = topScope;
    Node *cacheNode = node;
    struct VarDec retType = {CUSTOM, 0, NULL};

    while (cacheNode != NULL) {
        struct SemanticEntryReport entry = SA_get_entry_if_available(cacheNode->leftNode, currentScope);
        struct SemanticReport resMemRep = SA_check_restricted_member_access(cacheNode->leftNode, table, currentScope);
        
        if (entry.entry == NULL) {
            return SA_create_semantic_report(nullDec, ERROR, cacheNode->leftNode, NOT_DEFINED_EXCEPTION, NULL);
        } else if (resMemRep.status == ERROR) {
            return resMemRep;
        } else if (entry.entry->internalType == EXTERNAL) {
            return SA_create_semantic_report(externalDec, SUCCESS, NULL, NONE, NULL);
        }

        struct SemanticReport checkRes = SA_execute_access_type_checking(cacheNode, currentScope, topScope);

        if (checkRes.status == ERROR) {
            return checkRes;
        }

        retType = resMemRep.dec;
        currentScope = entry.entry->reference;
        cacheNode = cacheNode->rightNode;
    }

    return SA_create_semantic_report(retType, SUCCESS, NULL, NONE, NULL);
}

/**
 * <p>
 * Checks a member access, with only one identifier.
 * </p>
 * 
 * <p>
 * Examples:
 * ```
 * a
 * test()
 * add()
 * list()[0]
 * ```
 * </p>
 * 
 * @returns A SemanticReport with the analyzed type and flags for error an success
 * 
 * @param *node     Node to check
 * @param *table    The table from the scope, at which the member access occured
 * @param *topScope The current top scope in the process
 * 
 * <p>
 * The topScope is defined as the table at which the call occures, with the
 * importance of previously analyzed member access identifiers. That means, if
 * the source contains `add()` as an example, the top scope would be the table
 * at which the function call was called. If we change the example to `Math->add()`,
 * then the top scope would be the table, where the class is defined in.
 * </p>
 */
struct SemanticReport SA_check_restricted_member_access(Node *node, SemanticTable *table,
                                                            SemanticTable *topScope) {
    Node *cacheNode = node;
    struct VarDec retType = {CUSTOM, 0, NULL};
    struct SemanticEntryReport entry = SA_get_entry_if_available(cacheNode, topScope);
    
    if (entry.entry == NULL) {
        return SA_create_semantic_report(nullDec, ERROR, cacheNode, NOT_DEFINED_EXCEPTION, NULL);
    }
    
    retType = entry.entry->dec;
    
    if (cacheNode->type == _FUNCTION_CALL_NODE_) {
        struct SemanticReport rep = SA_evaluate_function_call(cacheNode, entry.entry, table, FNC_CALL);

        if (rep.status == ERROR) {
            return rep;
        }

        retType = rep.dec;
    }
    
    struct SemanticReport arrayRep = SA_handle_array_accesses(&retType, cacheNode, topScope);

    if (arrayRep.status == ERROR) {
        return arrayRep;
    }

    return SA_create_semantic_report(retType, SUCCESS, NULL, NONE, NULL);
}

/**
 * <p>
 * Evaluates a function call for correctness.
 * </p>
 * 
 * <p>
 * A function call can contain another member access, term or function call.
 * The return types are matched with the params of the function.
 * </p>
 * 
 * @return A report with possible error
 * 
 * @param *topNode          Top node, that holds the function call details
 * @param *functionEntry    Entry of the function in the nearest scope table
 * @param *callScopeTable   Table from which the function call was called
 */
struct SemanticReport SA_evaluate_function_call(Node *topNode, SemanticEntry *functionEntry,
                                                SemanticTable *callScopeTable, enum FunctionCallType fnccType) {
    if (functionEntry == NULL || topNode == NULL || callScopeTable == NULL) {
        return nullRep;
    }
    
    SemanticTable *ref = (SemanticTable*)functionEntry->reference;
    struct SemanticReport preCheck = SA_execute_function_call_precheck(ref, topNode, fnccType);
    
    if (preCheck.status == ERROR) {
        return preCheck;
    } else if (fnccType == FNC_CALL) {
        struct SemanticReport modCheck = SA_evaluate_modifier(ref, functionEntry->visibility, topNode, callScopeTable);

        if (modCheck.status == ERROR) {
            return modCheck;
        }
    }

    int actualParams = (int)SA_get_node_param_count(topNode);
    int strictCheck = fnccType == FNC_CALL ? false : true;

    for (int i = 0; i < actualParams; i++) {
        Node *currentNode = topNode->details[i];
        SemanticEntry *currentEntryParam = (SemanticEntry*)L_get_item(ref->paramList, i);
        
        struct VarDec currentNodeType = {CUSTOM, 0, NULL};
        struct SemanticReport idenRep = SA_execute_identifier_analysis(currentNode, ref, &currentNodeType, currentEntryParam, fnccType);

        if (idenRep.status == ERROR) {
            return idenRep;
        }

        if ((int)SA_are_VarTypes_equal(currentEntryParam->dec, currentNodeType, strictCheck) == false) {
            struct Node *errorNode = currentNode->type == _MEM_CLASS_ACC_NODE_ ? currentNode->leftNode : currentNode;
            return SA_create_expected_got_report(currentEntryParam->dec, currentNodeType, errorNode);
        }
    }

    return SA_create_semantic_report(functionEntry->dec, SUCCESS, NULL, NONE, NULL);
}

/**
 * <p>
 * Creates an expected-got-exception template message and fills it
 * out with the provided information.
 * </p>
 * 
 * @returns A SemanticReport with the message and errorNode and errorType
 * 
 * @param expected      The expected type
 * @param got           The got type instead of the expected type
 * @param *errorNode    Pointer to the error node
 */
struct SemanticReport SA_create_expected_got_report(struct VarDec expected, struct VarDec got, Node *errorNode) {
    char *expected_str = SA_get_VarType_string(expected);
    char *got_str = SA_get_VarType_string(got);
    size_t length = (size_t)strlen(expected_str) + (size_t)strlen(got_str) + 32 + 1; //+32 for the raw sentence
    char *buffer = (char*)calloc(length, sizeof(char));

    if (buffer == NULL) {
        (void)THROW_MEMORY_RESERVATION_EXCEPTION("EXP_GOT_ERR_CREATION");
        return SA_create_semantic_report(nullDec, ERROR, errorNode, TYPE_MISMATCH_EXCEPTION, NULL);
    }

    (void)snprintf(buffer, length * sizeof(char), "Expected %s, but got %s instead.", expected_str, got_str);
    (void)free(expected_str);
    (void)free(got_str);
    return SA_create_semantic_report(nullDec, ERROR, errorNode, TYPE_MISMATCH_EXCEPTION, buffer);
}

/**
 * <p>
 * Gets the VarType of an identifier or function call parameter. The evaluated
 * type is then written into the provided type pointer.
 * </p>
 * 
 * @returns Report with errors.
 * 
 * @param *currentNode          Node to check
 * @param *callScopeTable       Table from which the function call was called
 * @param *currentNodeType      Pointer to a modifyable VarType
 * @param *currentEntryParam    The entry of the current parameter in the function
 */
struct SemanticReport SA_execute_identifier_analysis(Node *currentNode, SemanticTable *callScopeTable, struct VarDec *currentNodeType,
                                    SemanticEntry *currentEntryParam, enum FunctionCallType fnccType) {
    switch (fnccType) {
    case FNC_CALL: {
        struct SemanticReport rep;

        if (currentNode->type == _MEM_CLASS_ACC_NODE_
            || currentNode->type == _FUNCTION_CALL_NODE_) {
            rep = SA_evaluate_member_access(currentNode, callScopeTable);
        } else {
            rep = SA_evaluate_simple_term(currentEntryParam->dec, currentNode, callScopeTable);
        }

        if (rep.status == ERROR) {
            return rep;
        }

        (*currentNodeType) = rep.dec;
        return nullRep;
    }
    case CONSTRUCTOR_CALL:
    case CONSTRUCTOR_CHECK_CALL: {
        struct VarDec dec = {CUSTOM, 0, NULL};
        
        if (currentNode->details != NULL && currentNode->detailsCount > 0) {
            dec = SA_get_VarType(currentNode->details[0], false);
        }

        if (dec.type == CUSTOM && fnccType == CONSTRUCTOR_CHECK_CALL) {
            struct SemanticReport termRep = SA_evaluate_simple_term(currentEntryParam->dec, currentNode, callScopeTable);
            dec = currentEntryParam->dec;

            if (termRep.status == ERROR) {
                return termRep;
            }
        }
        
        (*currentNodeType) = dec;
        return nullRep;
    }
    default: return nullRep;
    }

    return nullRep;
}

struct SemanticReport SA_handle_array_accesses(struct VarDec *currentType, struct Node *arrayAccStart, SemanticTable *table) {
    if (arrayAccStart->leftNode == NULL) {
        return nullRep;
    } else if (arrayAccStart->leftNode->type != _ARRAY_ACCESS_NODE_) {
        return nullRep;
    }

    struct Node *cache = arrayAccStart->leftNode != NULL ? arrayAccStart->leftNode : NULL;

    while (cache != NULL) {
        if (cache->leftNode != NULL) {
            struct VarDec expected = {INTEGER, 0, NULL};
            struct SemanticReport termRep = SA_evaluate_simple_term(expected, cache->leftNode, table);

            if (termRep.status == ERROR) {
                return termRep;
            }
        }

        cache = cache->rightNode;
        currentType->dimension--;
    }

    if (currentType->dimension < 0) {
        char *msg = "Negative arrays are not allowed.";
        return SA_create_semantic_report(nullDec, ERROR, arrayAccStart, NO_SUCH_ARRAY_DIMENSION_EXCEPTION, msg);
    }

    return nullRep;
}

struct SemanticReport SA_execute_function_call_precheck(SemanticTable *ref, Node *topNode, enum FunctionCallType fnccType) {
    if (ref == NULL) {
        return nullRep;
    } else if (topNode->detailsCount != ref->paramList->load) {
        char *msg = "The argument count is not equal to the definition.";
        return SA_create_semantic_report(nullDec, ERROR, topNode, WRONG_ARGUMENT_EXCPEPTION, msg);
    } else if (fnccType == CONSTRUCTOR_CHECK_CALL) {
        return nullRep;
    } else if ((topNode->type == _FUNCTION_CALL_NODE_ && ref->type != FUNCTION)
        || (topNode->type == _CLASS_CONSTRUCTOR_NODE_ && ref->type != CONSTRUCTOR)) {
        struct VarDec exp = {E_FUNCTION_CALL, 0, NULL};
        struct VarDec got = {E_NON_FUNCTION_CALL, 0, NULL};
        return SA_create_expected_got_report(exp, got, topNode);
    }

    return nullRep;
}

/**
 * <p>
 * Evaluates if a member access is valid or not, by checking
 * the modifier of the accessed object.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - No error occured
 * <li>false - Error occured
 * </ul>
 * 
 * @param *currentScope     The table of the modifier scope
 * @param vis               Assigned modifier to the object that tried to accessed
 * @param *node             Node at which the assignment happens
 * @param *topTable         The outer table of the call
 * 
 * <p><strong>Note:</strong>
 * By currentScope the table of the current scope is meant, that means if
 * the expression would be `Book->getPage().number`, the current scope is
 * the `Book`, while the topTable remains in the MAIN routine.
 * </p>
 */
struct SemanticReport SA_evaluate_modifier(SemanticTable *currentScope, enum Visibility vis, Node *node, SemanticTable *topTable) {
    if (topTable->type == MAIN) {
        if (vis != P_GLOBAL) {
            char *msg = "Modifiers outside of classes are not allowed.";
            return SA_create_semantic_report(nullDec, ERROR, node, STATEMENT_MISPLACEMENT_EXCEPTION, msg);
        } else {
            return nullRep;
        }
    }
    
    SemanticTable *nextClassTable = SA_get_next_table_of_type(currentScope, CLASS);

    if (nextClassTable == NULL) {
        return nullRep;
    } if ((int)strcmp(currentScope->name, nextClassTable->name) == 0
        && nextClassTable->type != MAIN) {
        return nullRep;
    } else {
        if (vis == PRIVATE || vis == SECURE) {
            char *msg = "Tried to access \"hidden\" declaration.";
            return SA_create_semantic_report(nullDec, ERROR, node, MODIFIER_EXCEPTION, msg);
        }
    }

    return nullRep;
}

/**
 * <p>
 * This function is specially designed to check the acces operator
 * used in a member/class access.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>-1 - Error occured (break)
 * <li>0 - Exit condition reached (break)
 * <li>1 - Nothing to worry about (continue)
 * </ul>
 * 
 * @param *cacheNode        Operator node
 * @param *currentScope     The SemanticTable of the current scope
 * @param *topScope         The SemanticTable when entering the member access checking
 * 
 */
struct SemanticReport SA_execute_access_type_checking(Node *cacheNode, SemanticTable *currentScope,
                                    SemanticTable *topScope) {
    if (cacheNode != NULL) {
        if (cacheNode->type == _CLASS_ACCESS_NODE_) {
            if (currentScope->type != CLASS) {
                char *msg = "Used \"->\" for non-class access instead of \".\".";
                return SA_create_semantic_report(nullDec, ERROR, cacheNode, WRONG_ACCESSOR_EXCEPTION, msg);
            }
        } else if (cacheNode->type == _MEMBER_ACCESS_NODE_) {
            if ((topScope->type != CLASS
                || (int)strcmp(topScope->name, topScope->name) != 0)
                && currentScope->type != ENUM) {
                char *msg = "Used \"->\" for member access instead of \".\".";
                return SA_create_semantic_report(nullDec, ERROR, cacheNode, WRONG_ACCESSOR_EXCEPTION, msg);
            }
        } else {
            return nullRep;
        }
    }

    return SA_create_semantic_report(nullDec, SUCCESS, NULL, NONE, NULL);
}

/**
 * <p>
 * Get the SemanticTable with the provided declaration.
 * </p>
 * 
 * <p><strong>Note:</strong>
 * Since classes and variables are only allowed to be used, if declared
 * prior, the function only has to check the parent tables until it reaches
 * the MAIN table.
 * </p>
 * 
 * @return Table that contains the declaration, if non was found it returns NULL
 * 
 * @param *node     Node to search / identifier / function call to search
 * @param *table    The table in which the searching starts (current scope).
 */
SemanticTable *SA_get_next_table_with_declaration(Node *node, SemanticTable *table) {
    SemanticTable *temp = table;

    while (temp != NULL) {
        if ((int)HM_contains_key(node->value, temp->symbolTable) == false
            && SA_get_param_entry_if_available(node->value, temp) == NULL) {
            temp = temp->parent;
        } else {
            break;
        }
    }

    return temp;
}

/**
 * <p>
 * Returns an entry of the table, of the topNode->value (key) is found
 * int the table.
 * </p>
 * 
 * @returns A SemanticEntryReport with the found entry. If nothing was found
 * NULL is set as entry.entry
 * 
 * @param *topNode      The node->value to search
 * @param *table        Table in which to search in
 */
struct SemanticEntryReport SA_get_entry_if_available(Node *topNode, SemanticTable *table) {
    if (topNode == NULL || table == NULL) {
        return SA_create_semantic_entry_report(NULL, false, true);
    }
    
    SemanticEntry *entry = SA_get_param_entry_if_available(topNode->value, table);
    
    if ((int)HM_contains_key(topNode->value, table->symbolTable) == true) {
        entry = HM_get_entry(topNode->value, table->symbolTable)->value;
    }

    if (entry == NULL) {
        return SA_create_semantic_entry_report(NULL, false, true);
    }

    return SA_create_semantic_entry_report(entry, true, false);
}

/**
 * <p>
 * Returns a table with the provided type.
 * </p>
 * 
 * @returns The table with the provided type
 * 
 * @param *currentTable     The table from which to go up from
 * @param type              Type to search
 */
SemanticTable *SA_get_next_table_of_type(SemanticTable *currentTable, enum ScopeType type) {
    SemanticTable *temp = currentTable;

    while (temp != NULL) {
        if (type == temp->type
            || temp->type == MAIN) {
            break;
        }

        temp = temp->parent;
    }

    return temp;
}

/**
 * <p>
 * Evaluates if an assignment (simple assignment for vars) is correct.
 * </p>
 * 
 * @returns A SemanticReport with potential errors and return type
 * 
 * @param expectedType      The type expected by the statement that is assigned
 * @param *topNode          The root node of the assignment operation
 * @param *table            Table in which the assignment occured
 */
struct SemanticReport SA_evaluate_assignment(struct VarDec expectedType, Node *topNode, SemanticTable *table) {
    SemanticTable *mainTable = SA_get_next_table_of_type(table, MAIN);
    struct SemanticEntryReport possibleEnumEntry = SA_get_entry_if_available(topNode, mainTable);

    if (possibleEnumEntry.entry != NULL) {
        if (possibleEnumEntry.entry->internalType == ENUM) {
            return SA_evaluate_member_access(topNode, mainTable);
        }
    }

    return SA_evaluate_simple_term(expectedType, topNode, table);
}

/**
 * <p>
 * This function creates a new SemanticTable for the current scope.
 * </p>
 * 
 * @returns The created SemanticTable
 * 
 * @param *root         The root node of the new scope (usually a `RUNNABLE`)
 * @param scope         The ScopeType of the table (e.g. "MAIN", "CLASS", "FUNCTION", ...)
 * @param *parent       The parent of the table
 * @param *params       Parameters for the table (e.g. functions, constructors ect.)
 * @param line          Line at which the table is created
 * @param position      Character position at which the table is created
 */
SemanticTable *SA_create_new_scope_table(Node *root, enum ScopeType scope,
                                            SemanticTable *parent, struct ParamTransferObject *params,
                                            size_t line, size_t position) {
    int paramCount = params == NULL ? 0 : params->params;
    int rootParamCount = root == NULL ? 0 : root->detailsCount;
    SemanticTable *table = SA_create_semantic_table(paramCount, rootParamCount, NULL, scope, line, position);
    table->parent = parent;
    (void)SA_add_parameters_to_runnable_table(table, params);
    return table;
}

/**
 * <p>
 * Checks if a node is an arithmetic operator or not.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Node is an arithmetic operator
 * <li>false - Node is not an arithmetic operator
 * </ul>
 * 
 * @param *node     Pointer to the node to check
 */
int SA_is_node_arithmetic_operator(Node *node) {
    switch (node->type) {
    case _PLUS_NODE_: case _MINUS_NODE_: case _MULTIPLY_NODE_: case _MODULO_NODE_:
    case _DIVIDE_NODE_:
        return true;
    default: return false;
    }
}

/**
 * <p>
 * Ensures that both provided types are equal.
 * </p>
 * 
 * <p>
 * The `strict` flag sets the standard. For strict, the
 * types have to match by 100%, for non-strict the
 * types only have to match the format (FLOAT and DOUBLE).
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Types are equal
 * <li>false - Types are not equal
 * </ul>
 * 
 * @param type1     The first type to check against the second
 * @param type2     The second type to check agains the first
 * @param strict    Flag to enable the strict mode
 */
int SA_are_VarTypes_equal(struct VarDec type1, struct VarDec type2, int strict) {
    return strict == true ?
        (int)SA_are_strict_VarTypes_equal(type1, type2) :
        (int)SA_are_non_strict_VarTypes_equal(type1, type2);
}

/**
 * <p>
 * Checks if two VarTypes are equal, but on a higher standard
 * basis. By that both types have to be the same as the other,
 * unlike the `#...non_strict_var_types...`.
 * </p>
 * 
 * <p><strong>Usage:</strong>
 * This is used to evaluate type equality in a constructor
 * definition, to prevent multiple constructors with equal
 * parameters.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Types are equal
 * <li>false - Types are not equal
 * </ul>
 * 
 * @param type1     The first type to check against the second
 * @param type2     The second type to check agains the first
 */
int SA_are_strict_VarTypes_equal(struct VarDec type1, struct VarDec type2) {
    if (type1.type == CLASS_REF && type2.type == CLASS_REF) {
        if ((int)strcmp(type1.classType, type2.classType) == 0
            && type1.dimension == type2.dimension) {
            return true;
        }
    }

    if (type1.type == EXTERNAL_RET || type2.type == EXTERNAL_RET) {
        return true;
    }

    return type1.type == type2.type
            && type1.dimension == type2.dimension ? true : false;
}

/**
 * <p>
 * Checks if two VarTypes are equal, but on a lower standard
 * basis. By that FLOAT's and DOUBLE's are handled equally for
 * instance and are assigned later.
 * </p>
 * 
 * <p><strong>Usage:</strong>
 * This is used to evaluate type equality in a function call.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Types are equal
 * <li>false - Types are not equal
 * </ul>
 * 
 * @param type1     The first type to check against the second
 * @param type2     The second type to check agains the first
 */
int SA_are_non_strict_VarTypes_equal(struct VarDec type1, struct VarDec type2) {
    if (((type1.type == DOUBLE || type1.type == FLOAT)
        && (type2.type == DOUBLE || type2.type == FLOAT))
        && type1.dimension == type2.dimension) {
        return true;
    }
    
    if (type1.type == CUSTOM && type1.dimension == type2.dimension) {
        return true;
    }

    if (type1.type == CLASS_REF && type2.type == CLASS_REF) {
        if ((int)strcmp(type1.classType, type2.classType) == 0
            && type1.dimension == type2.dimension) {
            return true;
        }
    }

    if (type1.type == EXTERNAL_RET || type2.type == EXTERNAL_RET) {
        return true;
    }

    return (type1.type == type2.type
            && type1.dimension == type2.dimension) ? true : false;
}

/**
 * <p>
 * Checks if an object is already defined or not.
 * </p>
 * 
 * <p>
 * The checking first occures on the lowest scope
 * going to the highest scope and then stops the search.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>true - Object is already defined
 * <li>false - Object is not defined
 * </ul>
 * 
 * @param *key          Object name to search
 * @param *scopeTable   Current table in the current scope
 */
int SA_is_obj_already_defined(char *key, SemanticTable *scopeTable) {
    SemanticTable *temp = scopeTable;

    while (temp != NULL) {
        if ((int)HM_contains_key(key, temp->symbolTable) == true
            || SA_get_param_entry_if_available(key, temp) != NULL) {
            return true;
        }

        temp = temp->parent;
    }

    return false;
}

/**
 * <p>
 * Returns the params of a provided node.
 * </p>
 * 
 * <p>
 * The params are always in the node->details pointer.
 * </p>
 * 
 * @returns An object, that contains the parameters
 * 
 * @param *topNode      Node to get the params from
 * @param stdType       The standard type of the params
 */
struct ParamTransferObject *SA_get_params(Node *topNode, struct VarDec stdType) {
    struct ParamTransferObject *obj = (struct ParamTransferObject*)calloc(1, sizeof(struct ParamTransferObject));

    if (obj == NULL) {
        (void)THROW_MEMORY_RESERVATION_EXCEPTION("Param_Transfer_Object");
    }

    obj->entries = (SemanticEntry**)calloc(topNode->detailsCount, sizeof(SemanticEntry));

    if (obj->entries == NULL) {
        (void)THROW_MEMORY_RESERVATION_EXCEPTION("Parameter_Entries");
    }

    int actualParams = 0;

    for (int i = 0; i < topNode->detailsCount; i++) {
        Node *innerNode = topNode->details[i];

        if (innerNode == NULL) {
            continue;
        } else if (innerNode->type == _RUNNABLE_NODE_
            || innerNode->type == _VAR_TYPE_NODE_) {
            continue;
        }
        
        Node *typeNode = innerNode->detailsCount > 0 ? innerNode->details[0] : NULL;
        struct VarDec type = SA_get_VarType(typeNode, false);
        SemanticEntry *entry = SA_create_semantic_entry(innerNode->value, type, P_GLOBAL, VARIABLE, NULL, innerNode->line, innerNode->position);
        obj->entries[actualParams++] = entry;
    }

    obj->params = actualParams;
    return obj;
}

/**
 * <p>
 * Get an entry in the param list of the provided table,
 * by the provided key.
 * </p>
 * 
 * @returns
 * <ul>
 * <li>SemanticEntry - The found entry in the table
 * <li>NULL - When no entry was found
 * </ul>
 * 
 * @param *key      Key to search in the param list
 * @param *table    The table to search in
 */
SemanticEntry *SA_get_param_entry_if_available(char *key, SemanticTable *table) {
    if (table == NULL) {
        return NULL;
    } else if (table->paramList == NULL) {
        return NULL;
    }

    for (int i = 0; i < table->paramList->load; i++) {
        SemanticEntry *entry = (SemanticEntry*)L_get_item(table->paramList, i);

        if ((int)strcmp(entry->name, key) == 0) {
            return entry;
        }
    }

    return NULL;
}

/**
 * <p>
 * Returns the VarType of the provided identifier.
 * </p>
 * 
 * @returns The converted VarDec
 * 
 * @param *node     Node to convert
 */
struct VarDec SA_convert_identifier_to_VarType(Node *node) {
    struct VarDec cust = {CUSTOM, 0, NULL};

    switch (node->type) {
    case _FLOAT_NODE_:
        cust.type = DOUBLE;
        break;
    case _NUMBER_NODE_:
        cust.type = INTEGER;
        break;
    default:
        break;
    }
    
    cust.dimension = node->leftNode == NULL ? 0 : (int)atoi(node->leftNode->value);
    return cust;
}

/**
 * <p>
 * Converts a node to the according VarType.
 * </p>
 * 
 * <p>
 * The VarType is received by the details.
 * </p>
 * 
 * @returns The converted VarType
 * 
 * @param *topNode  Node to convert
 * @param constant  Flag for whether the type is a constant
 */
struct VarDec SA_get_VarType(Node *node, int constant) {
    struct VarDec cust = {CUSTOM, 0, NULL, constant};
    int setType = false;

    if (node == NULL) {
        return cust;
    }

    int length = sizeof(TYPE_LOOKUP) / sizeof(TYPE_LOOKUP[0]);

    for (int i = 0; i < length; i++) {
        char *occurance = (char*)strstr(node->value, TYPE_LOOKUP[i].name);
        int pos = occurance - node->value;

        if (pos == 0) {
            cust.type = TYPE_LOOKUP[i].type;
            cust.dimension = node->leftNode == NULL ? 0 : (int)atoi(node->leftNode->value);
            setType = true;
            break;
        }
    }

    if (node->value != NULL && setType == false) {
        cust.classType = node->value;
        cust.type = CLASS_REF;
        cust.dimension = node->leftNode == NULL ? 0 : (int)atoi(node->leftNode->value);
    }

    return cust;
}

/**
 * <p>
 * Converts a modifier string into a visiblity type.
 * </p>
 * 
 * @returns The converted visibility type
 * 
 * @param *visibilityNode   Node to convert
 */
enum Visibility SA_get_visibility(Node *visibilityNode) {
    if (visibilityNode == NULL) {
        return P_GLOBAL;
    } else if (visibilityNode->type != _MODIFIER_NODE_) {
        printf("MODIFIER NODE IS INCORRECT!\n\n");
        exit(EXIT_FAILURE);
    }

    if ((int)strcmp("global", visibilityNode->value) == 0) {
        return GLOBAL;
    } else if ((int)strcmp("secure", visibilityNode->value) == 0) {
        return SECURE;
    } else if ((int)strcmp("private", visibilityNode->value) == 0) {
        return PRIVATE;
    }

    return P_GLOBAL;
}

/**
 * <p>
 * Creates a semantic report structure with the provided
 * information.
 * </p>
 * 
 * @returns A SemanticReport with the information
 * 
 * @param type          Report return type
 * @param status        Error status
 * @param *errorNode    Node that caused the error
 * @param errorType     Type of the error
 * @param *description  Description of the error
 */
struct SemanticReport SA_create_semantic_report(struct VarDec type, enum ErrorStatus status, Node *errorNode, enum ErrorType errorType,
                                                char *description) {
    struct SemanticReport rep;
    rep.dec = type;
    printf(">>>> >>>> >>>> >>>> ERROR OCC: %i %i\n", status, errorType);
    rep.status = status;
    rep.errorNode = errorNode;
    rep.errorType = errorType;
    rep.description = description;
    return rep;
}

/**
 * <p>
 * Creates an entry report.
 * </p>
 * 
 * @returns A SemanticEntryReport with the provided information
 * 
 * @param *entry        Entry to report
 * @param success       Flag for success
 * @param errorOccured  Flag for errors
 */
struct SemanticEntryReport SA_create_semantic_entry_report(SemanticEntry *entry, int success, int errorOccured) {
    struct SemanticEntryReport rep;
    rep.entry = entry;
    rep.success = success;
    rep.errorOccured = errorOccured;
    return rep;
}

/**
 * <p>
 * Creates an entry for the symboltable.
 * </p>
 * 
 * @returns A SemanticEntry with the provided information
 * 
 * @param *name         Name of the entry
 * @param varType       Return type / type of the entry
 * @param visibility    Visibility of the entry
 * @param internalType  Scope type
 * @param *ptr          Pointer to a reference table (optional)
 */
SemanticEntry *SA_create_semantic_entry(char *name, struct VarDec varType,
                                                enum Visibility visibility, enum ScopeType internalType,
                                                void *ptr, size_t line, size_t position) {
    SemanticEntry *entry = (SemanticEntry*)calloc(1, sizeof(SemanticEntry));
    entry->name = name;
    entry->reference = ptr;
    entry->dec = varType;
    entry->visibility = visibility;
    entry->internalType = internalType;
    entry->line = line;
    entry->position = position;
    return entry;
}

/**
 * <p>
 * Creates a semantic table and fills it with the provided information.
 * </p>
 * 
 * @returns A SemanticTable with the provided information
 * 
 * @param paramCount        How many parameters the table has
 * @param symbolTableSize   How many objects will be in the table (dynamic)
 * @param *parent           Pointer to the parent table
 * @param type              Type of the semantic table
 */
SemanticTable *SA_create_semantic_table(int paramCount, int symbolTableSize, SemanticTable *parent,
                                                enum ScopeType type, size_t line, size_t position) {
    SemanticTable *table = (SemanticTable*)calloc(1, sizeof(SemanticTable));

    if (table == NULL) {
        printf("Error on semantic table reservation!\n");
        return NULL;
    }

    table->paramList = CreateNewList(paramCount);
    table->symbolTable = CreateNewHashMap(symbolTableSize > 0 ? symbolTableSize : 1);
    table->parent = parent;
    table->type = type;
    table->line = line;
    table->position = position;
    return table;
}

/**
 * UNDER CONSTRUCTION!!!!
 */
void FREE_TABLE(SemanticTable *rootTable) {
    for (int i = 0; i < rootTable->paramList->load; i++) {
        (void)free(rootTable->paramList->entries[i]);
    }

    (void)FREE_LIST(rootTable->paramList);

    for (int i = 0; i < rootTable->symbolTable->capacity; i++) {
        if (rootTable->symbolTable->entries[i] == NULL) {
            continue;
        } else if (rootTable->symbolTable->entries[i]->value == NULL) {
            continue;
        }

        SemanticEntry *entry = (SemanticEntry*)rootTable->symbolTable->entries[i]->value;

        if (entry->reference != NULL) {
            (void)FREE_TABLE((SemanticTable*)entry->reference);
        }
    }

    (void)HM_free(rootTable->symbolTable);
}

void THROW_NO_SUCH_ARRAY_DIMENSION_EXCEPTION(struct SemanticReport rep) {
    (void)THROW_EXCEPTION("NoSuchArrayDimension", rep);
}

void THROW_MODIFIER_EXCEPTION(struct SemanticReport rep) {
    (void)THROW_EXCEPTION("ModifierException", rep);
}

void THROW_WRONG_ARGUMENT_EXCEPTION(struct SemanticReport rep) {
    (void)THROW_EXCEPTION("WrongArgumentException", rep);
}

void THROW_WRONG_ACCESSOR_EXEPTION(struct SemanticReport rep) {
    (void)THROW_EXCEPTION("WrongAccessorException", rep);
}

void THROW_STATEMENT_MISPLACEMENT_EXEPTION(struct SemanticReport rep) {
    (void)THROW_EXCEPTION("StatementMisplacementException", rep);
}

void THROW_TYPE_MISMATCH_EXCEPTION(struct SemanticReport rep) {
    (void)THROW_EXCEPTION("TypeMismatchException", rep);
    (void)free(rep.description);
}

void THROW_NOT_DEFINED_EXCEPTION(Node *node) {
    struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, node, NOT_DEFINED_EXCEPTION, NULL);
    (void)THROW_EXCEPTION("NotDefinedException", rep);
}

void THROW_ALREADY_DEFINED_EXCEPTION(Node *node) {
    struct SemanticReport rep = SA_create_semantic_report(nullDec, ERROR, node, ALREADY_DEFINED_EXCEPTION, NULL);
    (void)THROW_EXCEPTION("AlreadyDefinedException", rep);
}

void THROW_MEMORY_RESERVATION_EXCEPTION(char *problemPosition) {
    printf(TEXT_COLOR_RED "MemoryReservationException: at %s\n", problemPosition);
    printf("Error was thrown while semantic analysis.\n");
    printf("This error is an internal issue, please recompile.\n" TEXT_COLOR_RESET);
    exit(EXIT_FAILURE);
}

/**
 * <p>
 * Throws an standard exception with the provided message and node details.
 * </p>
 * 
 * @param *message  Message / Exception to write
 * @param rep       Report to print
 */
void THROW_EXCEPTION(char *message, struct SemanticReport rep) {
    int charsInLine = 0;
    int errorCharsAwayFromNL = 0;
    struct Node *node = rep.errorNode;

    for (int i = node->position; i > 0; i--, errorCharsAwayFromNL++) {
        if ((*BUFFER)[i - 1] == '\n' || (*BUFFER)[i - 1] == '\0') {
            break;
        }
    }

    for (int i = node->position - charsInLine; (*BUFFER)[i] != '\n' && (*BUFFER)[i] != '\0'; i++, charsInLine++);
    charsInLine += errorCharsAwayFromNL;

    (void)printf(TEXT_COLOR_RED);
    (void)printf("%s: at line ", message);
    (void)printf(TEXT_UNDERLINE);
    (void)printf(TEXT_COLOR_BLUE);
    (void)printf("%li:%i", node->line + 1, errorCharsAwayFromNL);
    (void)printf(TEXT_COLOR_RESET);
    (void)printf(TEXT_COLOR_RED);
    (void)printf(" from \"%s\"\n", FILE_NAME);
    (void)printf("    msg: %s\n", rep.description == NULL ? "N/A" : rep.description);

    char firstFoldMeta[12];
    int minSkip = (int)snprintf(firstFoldMeta, 12, "    at: ");
    (void)printf(firstFoldMeta);
    (void)printf(TEXT_COLOR_GRAY);

    for (int i = 0; i < charsInLine; i++) {
        int pos = node->position - errorCharsAwayFromNL + i;
        (void)printf("%c", (*BUFFER)[pos]);
    }

    (void)printf("\n");
    (void)printf(TEXT_COLOR_RED);
    int WSSP = node->position - errorCharsAwayFromNL;
    int WSLen = node->position + minSkip;

    for (int i = WSSP; i < WSLen; i++) {
        (void)printf(" ");
    }

    (void)printf(TEXT_COLOR_YELLOW);

    for (int i = 0; i < (int)strlen(node->value); i++) {
        (void)printf("^");
    }

    (void)printf("\n");
    (void)printf(TEXT_COLOR_RESET);
}

/**
 * <p>
 * Takes a SemanticReport and Throws the according error.
 * </p>
 */
void THROW_ASSIGNED_EXCEPTION(struct SemanticReport rep) {
    switch (rep.errorType) {
    case ALREADY_DEFINED_EXCEPTION:
        (void)THROW_ALREADY_DEFINED_EXCEPTION(rep.errorNode);
        break;
    case NOT_DEFINED_EXCEPTION:
        (void)THROW_NOT_DEFINED_EXCEPTION(rep.errorNode);
        break;
    case TYPE_MISMATCH_EXCEPTION:
        (void)THROW_TYPE_MISMATCH_EXCEPTION(rep);
        break;
    case STATEMENT_MISPLACEMENT_EXCEPTION:
        (void)THROW_STATEMENT_MISPLACEMENT_EXEPTION(rep);
        break;
    case WRONG_ACCESSOR_EXCEPTION:
        (void)THROW_WRONG_ACCESSOR_EXEPTION(rep);
        break;
    case WRONG_ARGUMENT_EXCPEPTION:
        (void)THROW_WRONG_ARGUMENT_EXCEPTION(rep);
        break;
    case MODIFIER_EXCEPTION:
        (void)THROW_MODIFIER_EXCEPTION(rep);
        break;
    case NO_SUCH_ARRAY_DIMENSION_EXCEPTION:
        (void)THROW_NO_SUCH_ARRAY_DIMENSION_EXCEPTION(rep);
        break;
    default:
        (void)THROW_EXCEPTION("Exception", rep);
        break;
    }
}

/**
 * <p>
 * The lookup structure of the VarType to String function.
 * </p>
 */
struct VarTypeString {
    enum VarType type;
    char *string;
};

/**
 * <p>
 * Collection of all primitive VarTypes, that can be matched.
 * </p>
 */
struct VarTypeString VarTypeStringLookup[] = {
    {INTEGER, "INTEGER"}, {DOUBLE, "DOUBLE"}, {FLOAT, "FLOAT"}, {STRING, "STRING"}, {LONG, "LONG"},
    {SHORT, "SHORT"}, {BOOLEAN, "BOOLEAN"}, {CHAR, "CHAR"}, {CUSTOM, "CUSTOM"}, {VOID, "VOID"},
    {null, "null"}, {EXTERNAL_RET, "EXT"}, {E_FUNCTION_CALL ,"<FUNCTION_CALL>"},
    {E_NON_FUNCTION_CALL, "<NON_FUNCTION_CALL>"}
};

/**
 * <p>
 * Converts a VarType into the string version.
 * </p>
 * 
 * @returns The converted VarType
 * 
 * @param type  Type to convert
 */
char *SA_get_VarType_string(struct VarDec type) {
    const int size = 8 + ((int)abs(type.dimension) * 2);
    int lookupSize = sizeof(VarTypeStringLookup) / sizeof(VarTypeStringLookup[0]);
    char *buffer = NULL;

    for (int i = 0; i < lookupSize; i++) {
        if (type.type == VarTypeStringLookup[i].type) {
            buffer = VarTypeStringLookup[i].string;
        }
    }

    if (buffer == NULL) {
        buffer = type.classType;
    }

    char *string = (char*)calloc(size + 1, sizeof(char));
    (void)strncpy(string, buffer, 7);

    if (type.dimension < 0) {
        (void)strcat(string, "-");
    }

    for (int i = 0; i < abs(type.dimension); i++) {
        (void)strcat(string, "[]");
    }

    return string;
}

/**
 * <p>
 * Get the string of the scope type.
 * This function basically "stringyfies" the scope type.
 * </p>
 * 
 * @returns The converted string
 * 
 * @param type  Scope type to convert
 */
char *SA_get_ScopeType_string(enum ScopeType type) {
    switch (type) {
    case VARIABLE:
        return "VARIABLE";
    case FUNCTION_CALL:
        return "FUNCTION_CALL";
    case CLASS:
        return "CLASS";
    case IF:
        return "IF";
    default: return "<REST>";
    }
}