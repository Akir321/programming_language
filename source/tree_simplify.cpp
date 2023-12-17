#include <assert.h>
#include <stdio.h>

#include "tree_of_expressions.h"
#include "html_logfile.h"
#include "tree_simplify.h"



#define CHECK_POISON_PTR(ptr) \
    if (ptr == PtrPoison)     \
    {                         \
        LOG("ERROR: PoisonPtr detected in %s(%d) %s\n", __FILE__, __LINE__, __func__);\
        return 0;                                                                     \
    }

#define EV_L canBeEvaluated(node->left)
#define EV_R canBeEvaluated(node->right)

#define VALUE_0(node) (canBeEvaluated(node) && equalDouble(expTreeEvaluate(eval, node, &error), 0))
#define VALUE_1(node) (canBeEvaluated(node) && equalDouble(expTreeEvaluate(eval, node, &error), 1))


int expTreeSimplify(Evaluator *eval, Node *node)
{
    assert(eval);

    int changeCount = 0;
    int prevCount   = -1;

    while (changeCount > prevCount)
    {
        prevCount = changeCount;

        changeCount += expTreeSimplifyConsts     (eval, node);
        changeCount += expTreeSimplifyNeutralElem(eval, node);
    }

    return EXIT_SUCCESS;
}

#define CHANGED 1

int expTreeSimplifyConsts(Evaluator *eval, Node *node)
{
    CHECK_POISON_PTR(node);
    
    if (!node)  return EXIT_SUCCESS;

    if (node->type == EXP_TREE_NUMBER)   return EXIT_SUCCESS;
    if (node->type == EXP_TREE_VARIABLE) return EXIT_SUCCESS;
    if (node->type == EXP_TREE_NOTHING)  return EXIT_FAILURE;

    int count = 0;
    count += expTreeSimplifyConsts(eval, node->left);
    count += expTreeSimplifyConsts(eval, node->right);

    if (EV_L && EV_R)
    {
        ExpTreeErrors error = TREE_NO_ERROR;
        double left  = expTreeEvaluate(eval, node->left,  &error);
        double right = expTreeEvaluate(eval, node->right, &error);
        if (error) return error;

        subTreeDtor(node->left);
        subTreeDtor(node->right);
        node->data.number  = NodeCalculate(left, right, node->data.operatorNum, &error);
        if (error) return error;

        node->type = EXP_TREE_NUMBER;
        node->left  = NULL;
        node->right = NULL;

        return 1;
    }
    return EXIT_SUCCESS;
}

int expTreeSimplifyNeutralElem(Evaluator *eval, Node *node)
{
    CHECK_POISON_PTR(node);
    
    if (!node)  return EXIT_SUCCESS;

    if (node->type == EXP_TREE_NUMBER)   return EXIT_SUCCESS;
    if (node->type == EXP_TREE_VARIABLE) return EXIT_SUCCESS;
    if (node->type == EXP_TREE_NOTHING)  return NODE_TYPE_NOTHING;

    int count = 0;

    count += tryNodeSimplify(eval, node);

    count += expTreeSimplifyNeutralElem(eval, node->left);
    count += expTreeSimplifyNeutralElem(eval, node->right);

    return count;
}

int tryNodeSimplify(Evaluator *eval, Node *node)
{
    assert(eval);
    CHECK_POISON_PTR(node);

    switch (node->data.operatorNum)
    {
        case ADD: case SUB:
        {
            if (casePlus0(eval, node, node->left,  node->right))  return CHANGED;
            if (casePlus0(eval, node, node->right, node->left))   return CHANGED;

            return EXIT_SUCCESS;
        }
        case MUL:
        {
            if (caseTimes0(eval, node, node->left))               return CHANGED;
            if (caseTimes0(eval, node, node->right))              return CHANGED;
            if (caseTimes1(eval, node, node->left,  node->right)) return CHANGED;
            if (caseTimes1(eval, node, node->right, node->left))  return CHANGED;

            return EXIT_SUCCESS;
        }
        case DIV:
        {
            if (caseTimes0(eval, node, node->left)) return CHANGED;
            return EXIT_SUCCESS;
        }
        case POW:
            return EXIT_SUCCESS;

        case LN:     case LOGAR:
        case SIN:    case COS:
        case ASSIGN: case WHILE:
        case BELOW:  case ABOVE:
        case IF:     case INSTR_END:
        case OPEN_F: case CLOSE_F:
        
            return EXIT_SUCCESS;

        case L_BRACKET: case R_BRACKET: case NOT_OPER:
        default:
            printf("ERROR: unknown operator: %d\n", node->data.operatorNum);
            return UNKNOWN_OPERATOR;
    }
}

int casePlus0(Evaluator *eval, Node *node, Node *zero, Node *savedNode) 
{
    CHECK_POISON_PTR(zero);
    CHECK_POISON_PTR(savedNode);

    ExpTreeErrors error = TREE_NO_ERROR;

    if (VALUE_0(zero))                           
    {   
        if (error) return error;                                       
        subTreeDtor(zero);               
        *node = *savedNode;               
        destroyNode(&savedNode);

        return CHANGED;         
    }

    return EXIT_SUCCESS;
}

int caseTimes0(Evaluator *eval, Node *node, Node *zero)
{   
    ExpTreeErrors error = TREE_NO_ERROR;

    if (VALUE_0(zero)) 
    {   if (error) return error;                                             
        subTreeDtor(node->left);                     
        subTreeDtor(node->right);                    
                                                     
        node->type = EXP_TREE_NUMBER;                
        node->data.number = 0;                       
        node->left  = NULL;                          
        node->right = NULL;

        return CHANGED;                        
    }

    return EXIT_SUCCESS;
}

int caseTimes1(Evaluator *eval, Node *node, Node *one, Node *savedNode) 
{
    CHECK_POISON_PTR(one);
    CHECK_POISON_PTR(savedNode);

    ExpTreeErrors error = TREE_NO_ERROR;

    if (VALUE_1(one))                           
    {   
        if (error) return error;                                       
        subTreeDtor(one);               
        *node = *savedNode;               
        destroyNode(&savedNode);

        return CHANGED;       
    }

    return EXIT_SUCCESS;
}