#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "tree_of_expressions.h"
#include "tree_graphic_dump.h"
#include "html_logfile.h"

#include "exp_tree_write.h"

#define CHECK_POISON_PTR(ptr) \
    assert(ptr != PtrPoison)
#undef  CHECK_POISON_PTR

#define CHECK_POISON_PTR(ptr) \
    if (ptr == PtrPoison)     \
    {                         \
        LOG("ERROR: PoisonPtr detected in %s(%d) %s\n", __FILE__, __LINE__, __func__);\
        return 0;                                                                        \
    }

ExpTreeData createNodeData(ExpTreeNodeType type, double value)
{
    ExpTreeData data = {};

    switch (type)
    {
        case EXP_TREE_NUMBER:       data.number = value;
                                    return data;

        case EXP_TREE_OPERATOR:     data.operatorNum = (ExpTreeOperators) value;
                                    return data;
                    
        case EXP_TREE_VARIABLE:     data.variableNum = (int) value;
                                    return data;

        case EXP_TREE_NOTHING:      data.number = DataPoison;
                                    return data;

        default:                    data.number = DataPoison;
                                    return data;
    }
}

Node *createNode(ExpTreeNodeType type, ExpTreeData data, Node *left, Node *right)
{
    Node *node = (Node *)calloc(1, sizeof(Node));
    if (!node) return NULL;

    node->type  = type;
    node->data  = data;

    node->left  = left;
    node->right = right;

    return node;
}

int destroyNode(Node **nodePtr)
{
    assert(nodePtr);

    Node *node = *nodePtr;
    CHECK_POISON_PTR(node);

    node->type        = EXP_TREE_NOTHING;
    node->data.number = DataPoison;
    node->left        = PtrPoison;
    node->right       = PtrPoison;

    free(node);
    *nodePtr = PtrPoison;

    return EXIT_SUCCESS;
}

int nameTableCtor(NameTable *names)
{
    assert(names);

    for (size_t i = 0; i < NamesNumber; i++)
    {
        names->table[i].value = DataPoison;
        names->table[i].name  = NULL;
    }

    return EXIT_SUCCESS;
}

int nameTableDtor(NameTable *names)
{
    assert(names);

    for (size_t i = 0; i < NamesNumber; i++)
    {
        names->table[i].value = DataPoison;

        free(names->table[i].name);
        names->table[i].name = NULL;
    }

    return EXIT_SUCCESS;
}

int nameTableAdd(NameTable *names, const char *name, double value)
{
    assert(names);
    assert(name);

    if (names->count >= NamesNumber) return IndexPoison;

    names->table[names->count].name = strdup(name);
    if (!names->table[names->count].name) return IndexPoison;

    names->table[names->count].value = value;
    names->count++;

    return (int)(names->count - 1);
}

int nameTableFind(NameTable *names, const char *name)
{
    assert(names);
    assert(name);

    for (int i = 0; i < names->count; i++)
    {
        if (strcmp(name, names->table[i].name) == 0)
        {
            return i;
        }
    }

    return IndexPoison;
}

int nameTableSetValue(NameTable *names, const char *name, double value)
{
    assert(names);
    assert(name);

    int index = nameTableFind(names, name);
    if (index == IndexPoison) return IndexPoison;

    names->table[index].value = value;

    return index;
}

int nameTableDump(NameTable *names, FILE *f)
{
    assert(names);
    assert(f);
    
    fprintf(f, "I'm NameTable\n");

    if (!names->count) fprintf(f, "  no names in NameTable\n");

    for (int i = 0; i < names->count; i++)
    {
        fprintf(f, "  [%d] <%s> = %lg\n", i, names->table[i].name, names->table[i].value);
    }

    return EXIT_SUCCESS;
}

int nameTableCopy(NameTable *from, NameTable *to)
{
    assert(from);
    assert(to);

    nameTableDtor(to);
    nameTableCtor(to);

    for (int i = 0; i < from->count; i++)
    {
        nameTableAdd(to, from->table[i].name, from->table[i].value);
    }

    return EXIT_SUCCESS;
}

int treeCtor(Tree *tree, Node *root)
{
    assert(tree);
    CHECK_POISON_PTR(root);
    assert(root);

    tree->root = root;
    tree->size = treeSize(root);

    return EXIT_SUCCESS;
}

int treeSize(Node *root)
{
    CHECK_POISON_PTR(root);
    if (root == NULL) return 0;

    return treeSize(root->left) + treeSize(root->right);
}

int treeDtor(Tree *tree)
{
    assert(tree);

    subTreeDtor  (tree->root);
    tree->size = -1;

    return EXIT_SUCCESS;
}

int subTreeDtor(Node *root)
{
    CHECK_POISON_PTR(root);
    if (root == NULL) return 0;

    subTreeDtor(root->left);
    subTreeDtor(root->right);
    destroyNode(&root);

    return EXIT_SUCCESS;
}

int evaluatorCtor(Evaluator *eval)
{
    assert(eval);

    nameTableCtor(&eval->names);
    eval->tree.root = NULL;
    eval->tree.size = 0;

    return EXIT_SUCCESS;
}

int evaluatorDtor(Evaluator *eval)
{
    assert(eval);

    treeDtor(&eval->tree);
    nameTableDtor(&eval->names);

    return EXIT_SUCCESS;
}

double expTreeEvaluate(Evaluator *eval, Node *root, ExpTreeErrors *error)
{
    assert(eval);
    CHECK_POISON_PTR(root);
    
    if (!root)                           return 0;
    if (root->type == EXP_TREE_NUMBER)   return root->data.number;
    if (root->type == EXP_TREE_VARIABLE) return eval->names.table[root->data.variableNum].value;

    double leftTree  = expTreeEvaluate(eval, root->left,  error);
    LOG("leftTree = %lg\n", leftTree);
    double rightTree = expTreeEvaluate(eval, root->right, error);
    LOG("rightTree = %lg\n", rightTree);

    if (*error) return DataPoison;

    double nodeValue = NodeCalculate(leftTree, rightTree, root->data.operatorNum, error);
    LOG("nodeValue = %lg\n", nodeValue);
    return nodeValue;
}

#define CHECK_ERROR(expression, type) \
    if (expression)             \
        {                       \
            *error = type;      \
            return DataPoison;  \
        }

double NodeCalculate(double leftTree, double rightTree, 
                     ExpTreeOperators operatorType, ExpTreeErrors *error)
{
    switch (operatorType)
    {
        case ADD:       return leftTree + rightTree;

        case SUB:       return leftTree - rightTree;

        case MUL:       return leftTree * rightTree;

        case DIV:       CHECK_ERROR(equalDouble(rightTree, 0), DIVISION_BY_ZERO);
                        return leftTree / rightTree;
            
        case LN:        CHECK_ERROR(rightTree < 0, LOG_NEGATIVE_ARG);
                        return log(rightTree);
            
        case LOGAR:     CHECK_ERROR(rightTree < 0, LOG_NEGATIVE_ARG);
                        CHECK_ERROR(leftTree  < 0, LOG_BAD_BASE);
                        CHECK_ERROR(equalDouble(leftTree, 1), LOG_BAD_BASE);

                        return log(rightTree) / log(leftTree);

        case POW:       return pow(leftTree, rightTree);

        case SIN:       return sin(rightTree);

        case COS:       return cos(rightTree);

        case L_BRACKET: case R_BRACKET: 
                        return 0;

        case NOT_OPER:
        default:        *error = UNKNOWN_OPERATOR;
                        return DataPoison;
    }
    return DataPoison;
}

bool canBeEvaluated(Node *node)
{
    CHECK_POISON_PTR(node);

    if (!node) return true;
    
    if (node->type == EXP_TREE_NUMBER)   return true;
    if (node->type == EXP_TREE_VARIABLE) return false;

    bool left  = canBeEvaluated(node->left);
    bool right = canBeEvaluated(node->right);

    return left && right;
}

bool equalDouble(double a, double b)
{
    return fabs(a - b) < PrecisionConst;
}
