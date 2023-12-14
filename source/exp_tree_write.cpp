#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "tree_of_expressions.h"
#include "html_logfile.h"
#include "exp_tree_write.h"

#define CHECK_POISON_PTR(ptr) \
    if (ptr == PtrPoison)     \
    {                         \
        LOG("ERROR: PoisonPtr detected in %s(%d) %s\n", __FILE__, __LINE__, __func__);\
        return 0;                                                                        \
    }

int printNode(Evaluator *eval, Node *node, FILE *f)
{
    CHECK_POISON_PTR(node);
    assert(node);
    assert(f);

    switch (node->type)
    {
        case EXP_TREE_NOTHING:
            fprintf(f, "nil");
            return EXIT_FAILURE;
        
        case EXP_TREE_NUMBER:
            fprintf(f, ElemNumberFormat, node->data.number);
            return EXIT_SUCCESS;

        case EXP_TREE_OPERATOR:
            return printTreeOperator(node->data.operatorNum, f);

        case EXP_TREE_VARIABLE:
            return printTreeVariable(eval, node, f);

        default:
            LOG("ERROR: unknown NodeType: %d\n", node->type);
            return EXIT_FAILURE;
    }
}

int dumpNode(Evaluator *eval, Node *node, FILE *f)
{
    if (node == PtrPoison) { fprintf(f, "Node * = PtrPoison\n"); return EXIT_FAILURE; }
    if (node == NULL     ) { fprintf(f, "Node * = NULL\n");      return EXIT_FAILURE; }

    fprintf(f, "\nI'm Node dump:\n");

    fprintf(f, "    type  = %d\n", node->type);
    fprintf(f, "    data  = ");
    printNode(eval, node, f);    
    putchar('\n');

    fprintf(f, "    left  = %p\n", node->left);
    fprintf(f, "    right = %p\n", node->right);

    return EXIT_SUCCESS;
}

#define OPER(oper) fprintf(f, oper); return EXIT_SUCCESS

int printTreeOperator(ExpTreeOperators operatorType, FILE *f)
{
    assert(f);

    switch (operatorType)
    {
        case ADD:    OPER("add");
        case SUB:    OPER("sub");
        case MUL:    OPER("mul");
        case DIV:    OPER("div");
        case LN:     OPER("ln");
        case LOGAR:  OPER("log");
        case POW:    OPER("pow");
        case SIN:    OPER("sin");
        case COS:    OPER("cos");

        case L_BRACKET: OPER("(");
        case R_BRACKET: OPER(")");

        case OPEN_F:    OPER("{");
        case CLOSE_F:   OPER("}");

        case INSTR_END: OPER(";");

        case ASSIGN:    OPER("assign");

        case BELOW:     OPER("below");
        case ABOVE:     OPER("above");

        case WHILE:     OPER("while");

        case IF:    OPER("if");
        
        case NOT_OPER:
        default:
            LOG("ERROR: unknown ExpTree operator type: %d", operatorType);
            return EXIT_FAILURE;
    }
}

#undef OPER

int printNodeSymbol(Evaluator *eval, Node *node, FILE *f)
{
    CHECK_POISON_PTR(node);
    assert(node);
    assert(f);

    switch (node->type)
    {
        case EXP_TREE_NOTHING: return EXIT_FAILURE;
        
        case EXP_TREE_NUMBER:
            fprintf(f, ElemNumberFormat, node->data.number);
            return EXIT_SUCCESS;

        case EXP_TREE_OPERATOR:
            return printTreeOperatorSymbol(node->data.operatorNum, f);

        case EXP_TREE_VARIABLE:
            return printTreeVariable(eval, node, f);

        default:
            LOG("ERROR: unknown NodeType: %d\n", node->type);
            return EXIT_FAILURE;
    }
}

int printTreeVariable(Evaluator *eval, Node *node, FILE *f)
{
    CHECK_POISON_PTR(node);
    assert(node);
    assert(f);

    int nameIndex = node->data.variableNum;

    if (!eval)  fprintf(f, "<eval = null>");

    else if (0 <= nameIndex && nameIndex < eval->names.count)
    {
        fprintf(f, "%s", eval->names.table[node->data.variableNum].name);
    }

    else      
    { 
        fprintf(f, "ERROR: bad nameIndex in Node: %d", nameIndex); 
        return EXIT_FAILURE; 
    }

    return EXIT_SUCCESS;
}

#define OPER(oper) fprintf(f, oper); return EXIT_SUCCESS

int printTreeOperatorSymbol(ExpTreeOperators operatorType, FILE *f)
{
    assert(f);

    switch (operatorType)
    {
        case ADD:    OPER("+");
        case SUB:    OPER("-");
        case MUL:    OPER("*");
        case DIV:    OPER("/");
        case LN:     OPER("ln");
        case LOGAR:  OPER("log");
        case POW:    OPER("^");
        case SIN:    OPER("sin");
        case COS:    OPER("cos");

        case L_BRACKET: OPER("(");
        case R_BRACKET: OPER(")");

        case OPEN_F:    OPER("{");
        case CLOSE_F:   OPER("}");

        case INSTR_END: OPER(";");

        case ASSIGN:    OPER("=");

        case BELOW:     OPER("below");
        case ABOVE:     OPER("above");

        case WHILE:     OPER("while");

        case IF:    OPER("if");

        case NOT_OPER:
        default:
            LOG("ERROR: unknown ExpTree operator type: %d", operatorType);
            return EXIT_FAILURE;
    }
}

#undef OPER

int printTreePrefix(Evaluator *eval, Node *root, FILE *f)
{
    CHECK_POISON_PTR(root);
    assert(eval);
    assert(f);

    if (!root) 
    {
        fprintf(f, "nil");
        return EXIT_SUCCESS;
    }

    putc('(', f);

    printNode(eval, root, f);                  putc(' ', f);
    printTreePrefix(eval, root->left, f);      putc(' ', f);
    printTreePrefix(eval, root->right, f);   

    putc(')', f);

    return EXIT_SUCCESS;
}

int printTreeInfix(Evaluator *eval, Node *root, FILE *f)
{
    CHECK_POISON_PTR(root);
    assert(eval);
    assert(f);

    if (!root) 
    {
        fprintf(f, "nil");
        return EXIT_SUCCESS;
    }

    putc('(', f);

    printTreeInfix(eval, root->left, f);       putc(' ', f);
    printNode(eval, root, f);                  putc(' ', f);
    printTreeInfix(eval, root->right, f);

    putc(')', f);

    return EXIT_SUCCESS;
}

int printTreePostfix(Evaluator *eval, Node *root, FILE *f)
{
    CHECK_POISON_PTR(root);
    assert(eval);
    assert(f);

    if (!root) 
    {
        fprintf(f, "nil");
        return EXIT_SUCCESS;
    }

    putc('(', f);

    printTreePostfix(eval, root->left, f);     putc(' ', f);
    printTreePostfix(eval, root->right, f);    putc(' ', f);
    printNode(eval, root, f);
    
    putc(')', f);

    return EXIT_SUCCESS;
}

int expTreeNodePriority(Node *node)
{
    CHECK_POISON_PTR(node);
    if (!node) return PR_NULL;

    
    if (node->type == EXP_TREE_NUMBER)  
    {
        if (node->data.number < 0) return PR_NEG_NUMBER;
        else                       return PR_NUMBER;
    }
    if (node->type == EXP_TREE_VARIABLE) return PR_NUMBER;

    return expTreeOperatorPriority(node->data.operatorNum);
}

int expTreeOperatorPriority(ExpTreeOperators oper)
{
    switch (oper)
    {
        case ADD: case SUB:
            return PR_ADD_SUB;
        
        case MUL: case DIV:
            return PR_MUL_DIV;

        case LN:  case LOGAR:
        case SIN: case COS:
            return PR_UNARY;

        case POW:
            return PR_POW;
        
        case R_BRACKET: case L_BRACKET:
        case ASSIGN: case IF:
        case BELOW: case ABOVE:
        case OPEN_F: case CLOSE_F:
        case INSTR_END:
        case NOT_OPER:
        case WHILE:
        default:
            return PR_UNKNOWN;
    }
}

bool isCommutative(Node *node)
{
    CHECK_POISON_PTR(node);
    if (!node) return true;

    if (node->data.operatorNum == ADD ||
        node->data.operatorNum == MUL) return true;

    return false;
}
