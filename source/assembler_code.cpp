#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "tree_of_expressions.h"
#include "html_logfile.h"
#include "exp_tree_write.h"
#include "assembler_code.h"

#define CHECK_POISON_PTR(ptr) \
    if (ptr == PtrPoison)     \
    {                         \
        LOG("ERROR: PoisonPtr detected in %s(%d) %s\n", __FILE__, __LINE__, __func__);\
        return 0;                                                                     \
    }


int createAssemblerCodeFile(Evaluator *eval, const char *fileInName)
{
    assert(eval);
    assert(fileInName);

    char *fileName = getFileName(fileInName, "assembler.txt");
    FILE *f = fopen(fileName, "w");

    if (!f) return MEMORY_ERROR;

    convertToAssemblyCode(eval, eval->tree.root, f);

    fprintf(f, "\nhlt\n");

    fclose(f);
    free(fileName);

    return EXIT_SUCCESS;
}

char *getFileName(const char *fileInName, const char *postfix)
{
    assert(fileInName);
    assert(postfix);

    char *temp = strdup(fileInName);
    strtok(temp, ".");

    char *fileName = NULL;
    __mingw_asprintf(&fileName, "%s%s", temp, postfix);

    free(temp);
    return fileName;
}

int convertToAssemblyCode(Evaluator *eval, Node *root, FILE *f)
{
    assert(eval);
    assert(f);

    dumpNode(eval, root, LogFile);
    LOG("\n");

    if (!root) return EXIT_SUCCESS;

    switch (root->type)
    {
        case EXP_TREE_NOTHING:  return EXIT_SUCCESS;

        case EXP_TREE_NUMBER:   fprintf(f, "push %lg\n", root->data.number);
                                return EXIT_SUCCESS;
        
        case EXP_TREE_VARIABLE: fprintf(f, "push ");
                                printAssemblyRegister(eval, root, f);
                                fprintf(f, "\n");
                                return EXIT_SUCCESS;

        case EXP_TREE_OPERATOR: return printAssemblyOperator(eval, root, f);
    
        default:                LOG("ERROR in %s(%d) in function %s: unknown Node type: %d\n",
                                    __FILE__, __LINE__ - 1, __func__, root->type);
                                return EXIT_FAILURE;
    }

    
}

int printAssemblyOperator(Evaluator *eval, Node *root, FILE *f)
{
    assert(eval);
    assert(root);
    assert(f);

    switch (root->data.operatorNum)
    {
        case INSTR_END:             convertToAssemblyCode(eval, root->left,  f);
                                    convertToAssemblyCode(eval, root->right, f);
                                    return EXIT_SUCCESS;

        case ASSIGN:                return printCaseAssign(eval, root, f);
                                    
        case ADD: case SUB: case MUL: case DIV:
        case POW: case LN:  case LOGAR:
        case SIN: case COS:         
        case OUT:                   convertToAssemblyCode(eval, root->left,  f);
                                    convertToAssemblyCode(eval, root->right, f);
                                    printTreeOperator(root->data.operatorNum, f);

                                    fprintf(f, "\n");
                                    return EXIT_SUCCESS;

        case IN:                    printTreeOperator(root->data.operatorNum, f);
                                    fprintf(f, "\npop ");
                                    printAssemblyRegister(eval, root->right, f);
                                    fprintf(f, "\n");
                                    return EXIT_SUCCESS;


        case BELOW: case ABOVE:     convertToAssemblyCode(eval, root->left,  f);
                                    convertToAssemblyCode(eval, root->right, f);
                                    fprintf(f, "sub\n\n");
                                    return EXIT_SUCCESS;

        case IF:                    return printCaseIf   (eval, root, f);

        case WHILE:                 return printCaseWhile(eval, root, f);

        case OPEN_F:    case CLOSE_F:
        case R_BRACKET: case L_BRACKET:
        case NOT_OPER:
        default:                    LOG("ERROR in %s(%d) in function %s: unsupported operator: %d\n",
                                        __FILE__, __LINE__ - 1, __func__, root->data.operatorNum);
                                    return EXIT_FAILURE;
    }
}

int printAssemblyRegister(Evaluator *eval, Node *node, FILE *f)
{
    if (node->type != EXP_TREE_VARIABLE) return BAD_NODE_TYPE;

    int varIndex = node->data.variableNum;

    if (0 <= varIndex && varIndex < eval->names.count)
    {
        fprintf(f, "r%cx", varIndex + 'a');
        return EXIT_SUCCESS;
    }

    LOG("ERROR: unknown var number: %d\n", varIndex);
    return EXIT_FAILURE; 
}

int printCaseAssign(Evaluator *eval, Node *root, FILE *f)
{
    assert(eval);
    assert(root);
    assert(f);

    if (root->right->type != EXP_TREE_VARIABLE) return BAD_NODE_TYPE;

    convertToAssemblyCode(eval, root->left,  f);

    int varIndex = root->right->data.variableNum;
    LOG("varIndex is %d\n", varIndex);

    if (0 <= varIndex && varIndex < eval->names.count)
    {
        fprintf(f, "pop ");
        printAssemblyRegister(eval, root->right, f);
        fprintf(f, "\n\n");

        return EXIT_SUCCESS;
    }

    LOG("ERROR: unknown var number: %d\n", varIndex);
    return EXIT_FAILURE;                                                         
}

int printCaseIf(Evaluator *eval, Node *root, FILE *f)
{
    assert(eval);
    assert(root);
    assert(f);

    static int ifStaticNumber = 0;
    int ifNumber = ifStaticNumber + 1;

    const char *prefix = "end_if_";

    convertToAssemblyCode(eval, root->left,  f);
    fprintf(f, "push 0\n");

    Node *node = root->left;

    if (node->type == EXP_TREE_OPERATOR)
    {
         printJmpOperator(node->data.operatorNum, prefix, ifNumber, f);
    }
    else fprintf(f, "jne :%s%d\n\n", prefix, ifNumber);
    
    ifStaticNumber++;
    convertToAssemblyCode(eval, root->right,  f);
    fprintf(f, ":%s%d\n\n", prefix, ifNumber);
    ifNumber++;

    return EXIT_SUCCESS;
}

int printCaseWhile(Evaluator *eval, Node *root, FILE *f)
{
    assert(eval);
    assert(root);
    assert(f);

    static int whileStaticNumber = 0;
    int whileNumber = whileStaticNumber + 1;

    const char *prefixBegin = "while_";
    const char *prefixEnd   = "end_while_";

    fprintf(f, ":%s%d\n\n", prefixBegin, whileNumber);

    convertToAssemblyCode(eval, root->left,  f);
    fprintf(f, "push 0\n");

    Node *node = root->left;

    if (node->type == EXP_TREE_OPERATOR)
    {
         printJmpOperator(node->data.operatorNum, prefixEnd, whileNumber, f);
    }
    else fprintf(f, "jne :%s%d\n\n", prefixEnd, whileNumber);
    
    whileStaticNumber++;
    convertToAssemblyCode(eval, root->right,  f);
    fprintf(f, "jmp :%s%d\n",   prefixBegin, whileNumber);
    fprintf(f, ":%s%d\n\n", prefixEnd,   whileNumber);
    whileNumber++;

    return EXIT_SUCCESS;
}

int printJmpOperator(int oper, const char *labelPrefix, int labelNum,  FILE *f)
{
    assert(labelPrefix);
    assert(labelNum);

    switch (oper)
    {
        case ABOVE:     fprintf(f, "jbe :%s%d\n\n", labelPrefix, labelNum);
                        break;

        case BELOW:     fprintf(f, "jae :%s%d\n\n", labelPrefix, labelNum);
                        break;

        default:        fprintf(f, "jne :%s%d\n\n", labelPrefix, labelNum);
                        break;
    }

    return EXIT_SUCCESS;
}
