#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "tree_of_expressions.h"
#include "exp_tree_write.h"
#include "recursive_descent_reading.h"
#include "html_logfile.h"
#include "tree_graphic_dump.h"


#define syntax_assert(exp) if (!(exp))                                 \
    {                                                                  \
        printf("SYNTAX_ERROR: %s\n", #exp);                            \
        fprintf(LogFile, "function throwing s_error: %s\n", __func__); \
        syntaxError(&tokenArray[*arrPosition], *arrPosition);          \
        return NULL;                                                   \
    }

#define CHECK_POISON_PTR(ptr) \
    if (ptr == PtrPoison)     \
    {                         \
        LOG("ERROR: PoisonPtr detected in %s(%d) %s\n", __FILE__, __LINE__, __func__);\
        return 0;                                                                     \
    }

#define NEW_NODE(type, value, left, right)                     \
    createNode(type, createNodeData(type, value), left, right)

#define NUM_NODE(value)  NEW_NODE(EXP_TREE_NUMBER, value, NULL, NULL)

#define cL     copy      (eval, node->left)
#define cR     copy      (eval, node->right)
#define cN     copy      (eval, node)

#define dL_W     derivative(eval, node->left,  writeToTex)
#define dR_W     derivative(eval, node->right, writeToTex)

#define dL       derivative(eval, node->left)
#define dR       derivative(eval, node->right)

#define _ADD(left, right) NEW_NODE(EXP_TREE_OPERATOR, ADD,  left, right)

#define _SUB(left, right) NEW_NODE(EXP_TREE_OPERATOR, SUB,  left, right)

#define _MUL(left, right) NEW_NODE(EXP_TREE_OPERATOR, MUL,  left, right)

#define _DIV(left, right) NEW_NODE(EXP_TREE_OPERATOR, DIV,  left, right)

#define _LN(       right) NEW_NODE(EXP_TREE_OPERATOR, LN,   NULL, right)

#define _POW(left, right) NEW_NODE(EXP_TREE_OPERATOR, POW,  left, right)

#define _SIN(      right) NEW_NODE(EXP_TREE_OPERATOR, SIN,  NULL, right)

#define _COS(      right) NEW_NODE(EXP_TREE_OPERATOR, COS,  NULL, right)

#define _SQRT(     right) NEW_NODE(EXP_TREE_OPERATOR, SQRT, NULL, right)


int readTreeFromFileRecursive(Evaluator *eval, const char *fileName)
{
    assert(eval);
    assert(fileName);

    nameTableCtor(&eval->names);

    int size = fileSize(fileName);
    char *text = (char *)calloc(size + 1, sizeof(char));
    if (!text) return MEMORY_ERROR;

    FILE *f = fopen(fileName, "r");
    if (!f) return EXIT_FAILURE;

    fread(text, sizeof(char), size, f);

    Node *root = getG(eval, text);
    fclose(f);
    free(text);

    if (!root) return EXIT_FAILURE;

    eval->tree.root = root;
    eval->tree.size = treeSize(eval->tree.root);

    return EXIT_SUCCESS;
}

int fileSize(const char *name)
{
    assert(name);

    struct stat stats = {};
    if (stat(name, &stats) == -1)  return -1;

    int size = stats.st_size;
    return size;
}


Token *createTokenArray(Evaluator *eval, const char *source)
{
    assert(source);

    ReadBuf readBuf = { strdup(source), (int) strlen(source), 0, 0, 0 };
    if (!readBuf.str) return NULL;

    int arrPosition = 0;
    Token *buf = (Token *)calloc(strlen(source) + 1, sizeof(Token));
    if (!buf) return NULL;  

    while (readBuf.position < readBuf.size)
    {
        LOG("\nreadBuf pos = %d\n", readBuf.position);
        LOG( "Tokenarr pos = %d\n", arrPosition);

        int error = getToken(eval, buf + arrPosition, &readBuf);
        printTokenArray(buf, LogFile);
        if (error) { LOG("getToken: ERROR occured: %d\n", error); return NULL; }

        arrPosition++;
    }

    free(readBuf.str);

    return buf;
}



#define SET_TOKEN(type, value)\
    setToken(token, readBuf->line, readBuf->linePosition, type, createNodeData(type, value))

#define TOKEN_OP(opType)                 \
    SET_TOKEN(EXP_TREE_OPERATOR, opType);\
    readBuf->linePosition++;             \
    readBuf->position++;    


int getToken(Evaluator *eval, Token *token, ReadBuf *readBuf)
{
    assert(token);
    assert(readBuf);
    assert(readBuf->str);

    switch (readBuf->str[readBuf->position])
    {
        case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9': case '0':
        {
            caseNumber(token, readBuf); return EXIT_SUCCESS;
        }


        case '(':   TOKEN_OP(L_BRACKET); return EXIT_SUCCESS;
        case ')':   TOKEN_OP(R_BRACKET); return EXIT_SUCCESS;

        case ' ':  case '\n':
        case '\t': case '\r':   skipSpaces(readBuf);
                                getToken(eval, token, readBuf);
                                return EXIT_SUCCESS;

        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': 
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': 
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': 
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': 
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        {
            caseLetter(eval, token, readBuf); return EXIT_SUCCESS;
        }

        default:    LOG("ERROR: unknown operator: %d\n\tSYNTAX_ERROR since line %d, position %d\n", 
                        readBuf->str[readBuf->position], readBuf->line, readBuf->linePosition);
                    return EXIT_FAILURE;
    }
}

int setToken(Token *token, int line, int startPosition, ExpTreeNodeType type, ExpTreeData data)
{
    assert(token);

    (*token).type          = type;
    (*token).data          = data;
    (*token).line          = line;
    (*token).startPosition = startPosition;

    return EXIT_SUCCESS;
}

int skipSpaces(ReadBuf *readBuf)
{
    assert(readBuf);
    assert(readBuf->str);

    while (isspace(readBuf->str[readBuf->position]))
    {
        char c = readBuf->str[readBuf->position];

        switch (c)
        {
            case ' ': case '\t':
            case '\r':
                                readBuf->position++;
                                readBuf->linePosition++;
                                break;           
            
            case '\n':          readBuf->position++;
                                readBuf->line++;
                                readBuf->linePosition = 0;
                                break;
        
            default:            LOG("ERROR: unknown space operator: %c(%d)\n",c, c);
                                break;
        }
    }

    return EXIT_SUCCESS;
}

int caseNumber(Token *token, ReadBuf *readBuf)
{
    assert(token);
    assert(readBuf);
    assert(readBuf->str);

    //LOG("position  = %d\n", readBuf->position);
    //LOG("str (before strtod) = %p\n", readBuf->str);
    char *end = readBuf->str + readBuf->position;
    double value = strtod(readBuf->str + readBuf->position, &end);

    //LOG("str (after strtod) = %p\n", readBuf->str);
    //LOG("caseNumber value = %lg\n", value);
    //LOG("position  = %d\n", readBuf->position);
    //LOG("buf + pos = %p, end = %p\n", readBuf->str + readBuf->position, end);

    SET_TOKEN(EXP_TREE_NUMBER, value);

    int shift = (int)(end - readBuf->str - readBuf->position) / sizeof(char);
    //LOG("shift = %d\n", shift);

    readBuf->linePosition += shift;
    readBuf->position     += shift;
    //LOG("new position  = %d\n", readBuf->position);

    return EXIT_SUCCESS;
}
#undef SET_TOKEN

#define SET_TOKEN(type, value)\
    setToken(token, readBuf->line, linePosition, type, createNodeData(type, value))

int caseLetter(Evaluator *eval, Token *token, ReadBuf *readBuf)
{
    assert(token);
    assert(readBuf);
    assert(readBuf->str);

    int position = 0;
    char *word   = (char *)calloc(readBuf->size + 1 - readBuf->position, sizeof(char));
    if (!word) return MEMORY_ERROR;

    int linePosition = readBuf->linePosition;
    char *c = &readBuf->str[readBuf->position];
    
    while (isalnum(*c) || *c == '_')
    {
        word[position++] = *c;
        readBuf->position++;
        readBuf->linePosition++;
        c++;
    }

    ExpTreeOperators op = getWordOperator(word);
    if (!op) 
    {
        int index = nameTableFind(&eval->names, word);
        if (index == IndexPoison) index = nameTableAdd(&eval->names, word, 0);

        free(word);

        SET_TOKEN(EXP_TREE_IDENTIF, index);
        return EXIT_SUCCESS;
    }
    
    free(word);
    SET_TOKEN(EXP_TREE_OPERATOR, op);
    return EXIT_SUCCESS;
}
#undef SET_TOKEN

#define COMPARE_WORD(kw, oper) else if (strcmp(word, kw)   == 0) return oper

ExpTreeOperators getWordOperator(const char *word)
{
    assert(word);

    LOG("(%s) -- word\n", word);

    if (*word == '\0') return NOT_OPER;

    COMPARE_WORD("sin",      SIN);
    COMPARE_WORD("cos",      COS);
    COMPARE_WORD("log",      LOGAR);
    COMPARE_WORD("ln",       LN);
    COMPARE_WORD("koreshok", SQRT);
    COMPARE_WORD("koli",     IF);
    COMPARE_WORD("pokuda",   WHILE);
    COMPARE_WORD("vvedi",    IN);
    COMPARE_WORD("vivedi",   OUT);
    COMPARE_WORD("plus",     ADD);
    COMPARE_WORD("minus",    SUB);
    COMPARE_WORD("umnozhit", MUL);
    COMPARE_WORD("delit",    DIV);
    COMPARE_WORD("vozvesti", POW);
    COMPARE_WORD("prisvoy",  ASSIGN);
    COMPARE_WORD("menshe",   BELOW);
    COMPARE_WORD("bolshe",   ABOVE);
    COMPARE_WORD("togda",    THEN);
    COMPARE_WORD("ravno",    EQUAL);
    COMPARE_WORD("neravno",  NOT_EQUAL);

    COMPARE_WORD("perem",    NEW_VAR);

    COMPARE_WORD("pole_polushko_nachnis",   OPEN_F);
    COMPARE_WORD("pole_polushko_zakonchis", CLOSE_F);

    COMPARE_WORD("slavsya_rus", INSTR_END);


    else return NOT_OPER;
}
#undef COMPARE_WORD

int printTokenArray(Token *tokenArray, FILE *f)
{
    assert(tokenArray);
    assert(f);

    fprintf(f, "I'm Token Array\n{\n");

    for (int i = 0; tokenArray[i].type != EXP_TREE_NOTHING; i++)
    {
        switch (tokenArray[i].type)
        {
            case EXP_TREE_NUMBER:
                fprintf(f, " [%d] %lg\n", i, tokenArray[i].data.number);
                break;

            case EXP_TREE_OPERATOR:
                fprintf(f, " [%d] ", i);
                printTreeOperator(tokenArray[i].data.operatorNum, f);
                putc('\n', f);
                break;

            case EXP_TREE_VARIABLE: 
                fprintf(f, " [%d] var%d\n", i, tokenArray[i].data.variableNum);
                break;

            case EXP_TREE_IDENTIF: 
                fprintf(f, " [%d] id%d\n", i, tokenArray[i].data.variableNum);
                break;
            
            case EXP_TREE_NOTHING:
                break;

            default: printf("ERROR: incorrect operator type: %d\n", tokenArray[i].data.operatorNum);
        }
    }

    fprintf(f, "}\n");

    return EXIT_SUCCESS;
}






Node *getG(Evaluator *eval, const char *str)
{
    Token *tokenArray = createTokenArray(eval, str);
    if (!tokenArray) return NULL;

    int arrPosition = 0;

    Node *val = getMultOp(eval, tokenArray, &arrPosition);

    if (tokenArray[arrPosition].type != EXP_TREE_NOTHING) syntaxError(tokenArray + arrPosition, arrPosition);

    return val;
}

#define SYNTAX_ERROR                                          \
    {                                                         \
        fprintf(LogFile, "function throwing s_error: %s\n", __func__); \
        syntaxError(tokenArray + *arrPosition, *arrPosition); \
        return PtrPoison;                                     \
    }

#define TOKEN_IS_NUM  (tokenArray[*arrPosition].type == EXP_TREE_NUMBER)
#define TOKEN_IS_OPER (tokenArray[*arrPosition].type == EXP_TREE_OPERATOR)
#define TOKEN_IS_NULL (tokenArray[*arrPosition].type == EXP_TREE_NOTHING)
#define TOKEN_IS_VAR  ((int)eval->names.table[tokenArray[*arrPosition].data.variableNum].value == EXP_TREE_VARIABLE)
#define TOKEN_IS_ID   (tokenArray[*arrPosition].type == EXP_TREE_IDENTIF)

#define TOKEN_IS(oper) (tokenArray[*arrPosition].data.operatorNum == oper)

#define TOKEN_PRIORITY_IS(oper)\
    (expTreeOperatorPriority(curToken->data.operatorNum) == oper)

Node *getOp(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *val = NULL;

    if (TOKEN_IS_OPER && TOKEN_IS(OPEN_F))
    {
        (*arrPosition)++;
        val = getMultOp(eval, tokenArray, arrPosition);

        syntax_assert(TOKEN_IS_OPER && TOKEN_IS(CLOSE_F));
        (*arrPosition)++;

        return val;
    }

    val = getNewVar(eval, tokenArray, arrPosition);
    if (val) return val;

    val = getIfWhile(eval, tokenArray, arrPosition);
    if (val) return val;

    val = getInOut(eval, tokenArray, arrPosition);
    if (val) return val;

    val = getA(eval, tokenArray, arrPosition);
    if (val) return val;

    SYNTAX_ERROR;
    return PtrPoison;
}

Node *getMultOp(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *val = NULL;

    
    val = getOp(eval, tokenArray, arrPosition);
    val = NEW_NODE(EXP_TREE_OPERATOR, INSTR_END, val, NULL);

    Node *curVal = val;

    int oldPos = *arrPosition;

    while (!((TOKEN_IS_OPER && TOKEN_IS(CLOSE_F)) || TOKEN_IS_NULL))
    {
        Node *val2 = getOp(eval, tokenArray, arrPosition);
        if (oldPos == *arrPosition) SYNTAX_ERROR;

        curVal->right = NEW_NODE(EXP_TREE_OPERATOR, INSTR_END, val2, NULL);
        curVal = curVal->right;
    }

    return val;
}

Node *getIfWhile(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    if (TOKEN_IS_OPER && (TOKEN_IS(IF) || TOKEN_IS(WHILE)))
    {
        int oper = tokenArray[*arrPosition].data.operatorNum;
        (*arrPosition)++;

        Node *val = getB(eval, tokenArray, arrPosition);
        if (!val) SYNTAX_ERROR;

        syntax_assert(TOKEN_IS_OPER && TOKEN_IS(THEN));
        (*arrPosition)++;

        Node *val2 = getOp(eval, tokenArray, arrPosition);
        if (!val2) SYNTAX_ERROR;

        return NEW_NODE(EXP_TREE_OPERATOR, oper, val, val2);
    }

    return NULL;
}

Node *getInOut(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    if (TOKEN_IS_OPER && (TOKEN_IS(IN) || TOKEN_IS(OUT)))
    {
        int oper = tokenArray[*arrPosition].data.operatorNum;
        (*arrPosition)++;

        Node *val = NULL;
        
        if (oper == IN)  val = getId(eval, tokenArray, arrPosition);
        if (oper == OUT) val = getP(eval, tokenArray, arrPosition);

        if (val == PtrPoison) SYNTAX_ERROR;

        syntax_assert(TOKEN_IS_OPER && TOKEN_IS(INSTR_END));
        (*arrPosition)++;

        return NEW_NODE(EXP_TREE_OPERATOR, oper, NULL, val);
    }

    return NULL;
}

Node *getA(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *var = getId(eval, tokenArray, arrPosition);
    if (var == PtrPoison) SYNTAX_ERROR;

    if (TOKEN_IS_OPER && TOKEN_IS(ASSIGN))
    {
        (*arrPosition)++;
        Node *expression = getE(eval, tokenArray, arrPosition);

        syntax_assert(TOKEN_IS_OPER && TOKEN_IS(INSTR_END));
        (*arrPosition)++;
        
        return NEW_NODE(EXP_TREE_OPERATOR, ASSIGN, expression, var);
    }

    return NULL;
}

Node *getB(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *val = getE(eval, tokenArray, arrPosition);

    if (TOKEN_IS_OPER && (TOKEN_IS(BELOW) || TOKEN_IS(ABOVE) ||
                          TOKEN_IS(EQUAL) || TOKEN_IS(NOT_EQUAL)))
    {
        int oper = tokenArray[*arrPosition].data.operatorNum;
        (*arrPosition)++;

        Node *val2 = getE(eval, tokenArray, arrPosition);

        val = NEW_NODE(EXP_TREE_OPERATOR, oper, val, val2);
    }

    return val;
}

Node *getE(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *val = getT(eval, tokenArray, arrPosition);

    while (TOKEN_IS_OPER && (TOKEN_IS(ADD) || TOKEN_IS(SUB)))
    {
        int oper = tokenArray[*arrPosition].data.operatorNum;
        (*arrPosition)++;

        Node *val2 = getT(eval, tokenArray, arrPosition);
    
        switch (oper)
        {
            case ADD:   val = _ADD(val, val2); break;
            case SUB:   val = _SUB(val, val2); break;
            default:    val = PtrPoison;       break;
        }
    }
    return val;
}

Node *getT(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *val = getPow(eval, tokenArray, arrPosition);

    while (TOKEN_IS_OPER && (TOKEN_IS(MUL) || TOKEN_IS(DIV)))
    {
        int oper = tokenArray[*arrPosition].data.operatorNum;
        (*arrPosition)++;

        Node *val2 = getPow(eval, tokenArray, arrPosition);

        switch (oper)
        {
            case MUL:   val = _MUL(val, val2); break;
            case DIV:   val = _DIV(val, val2); break;
            default:    val = PtrPoison;       break;
        }
    }
    return val;
}

Node *getPow(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Node *base = getP(eval, tokenArray, arrPosition);

    if (TOKEN_IS_OPER && TOKEN_IS(POW))
    {
        (*arrPosition)++;

        Node *deg = getP(eval, tokenArray, arrPosition);
        if (!deg) SYNTAX_ERROR;

        return _POW(base, deg);
    }

    return base;
}

Node *getP(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    if (TOKEN_IS_OPER && TOKEN_IS(L_BRACKET))
    {
        (*arrPosition)++;
        Node *val = getE(eval, tokenArray, arrPosition);
        if (!val) SYNTAX_ERROR;

        syntax_assert(TOKEN_IS_OPER && TOKEN_IS(R_BRACKET));
        (*arrPosition)++;

        return val;
    }
    else
    {
        return getU(eval, tokenArray, arrPosition);
    }
}

Node *getU(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Token *curToken = tokenArray + *arrPosition;

    if (TOKEN_IS_OPER && TOKEN_PRIORITY_IS(PR_UNARY))
    {
        if (TOKEN_IS(LOGAR))
        {
            (*arrPosition)++;

            Node *val1 = getPow(eval, tokenArray, arrPosition);
            Node *val2 = getPow(eval, tokenArray, arrPosition);
            if (!val1 || !val2) SYNTAX_ERROR;

            return NEW_NODE(EXP_TREE_OPERATOR, LOGAR, val1, val2);
        }

        int oper = tokenArray[*arrPosition].data.operatorNum;
        (*arrPosition)++;

        Node *val = getPow(eval, tokenArray, arrPosition);
        if (!val) SYNTAX_ERROR;

        switch (oper)
        {
            case SIN:   return _SIN (val);
            case COS:   return _COS (val);
            case LN:    return _LN  (val);
            case SQRT:  return _SQRT(val);
        
            default:    SYNTAX_ERROR;
        }
    }

    return getN(eval, tokenArray, arrPosition);
}

Node *getN(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Token *curToken = tokenArray + *arrPosition;

    if (TOKEN_IS_NUM)
    {   
        double val = curToken->data.number;
        (*arrPosition)++;

        return NUM_NODE(val);
    }

    if (TOKEN_IS_VAR)
    {
        return getId(eval, tokenArray, arrPosition);
    }

    if (TOKEN_IS_OPER && TOKEN_IS(SUB))
    {
        (*arrPosition)++;
        Node *val = getP(eval, tokenArray, arrPosition);

        if (val->type == EXP_TREE_NUMBER)
        {
            val->data.number = - val->data.number;
            return val;
        }

        return _SUB(NUM_NODE(0), val);
    }

    if (TOKEN_IS_NULL) return NULL;
    
    SYNTAX_ERROR;
}

Node *getId(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    //LOG("hi, i'm function %s\n  I'm currently on token Arr pos %d\n\n", __func__, *arrPosition);

    Token *curToken = tokenArray + *arrPosition;

    if (TOKEN_IS_VAR)
    {
        int index = curToken->data.variableNum;
        (*arrPosition)++;

        return NEW_NODE(EXP_TREE_VARIABLE, index, NULL, NULL);
    }

    SYNTAX_ERROR;
}

#define SET_ID_TO_VAR nameTableSetValue(&eval->names,                                                   \
                                         eval->names.table[(tokenArray[*arrPosition].data.idNum)].name, \
                                         EXP_TREE_VARIABLE)

Node *getNewVar(Evaluator *eval, Token *tokenArray, int *arrPosition)
{
    assert(tokenArray);
    assert(arrPosition);

    Token *curToken = tokenArray + *arrPosition;

    if (TOKEN_IS_OPER && TOKEN_IS(NEW_VAR))
    {
        int oper = curToken->data.operatorNum;
        (*arrPosition)++;

        syntax_assert(TOKEN_IS_ID);

        switch(oper)
        {
            case NEW_VAR:   SET_ID_TO_VAR;
                            (*arrPosition)++;
                            syntax_assert(TOKEN_IS_OPER && TOKEN_IS(INSTR_END));
                            (*arrPosition)++;
                            return NEW_NODE(EXP_TREE_OPERATOR, INSTR_END, NULL, NULL);

            default:        SYNTAX_ERROR;
        }
    }

    return NULL;
}

int syntaxError(Token *token, int arrPosition)
{
    assert(token);

    LOG("SYNTAX_ERROR: in line %d starting from position %d:\n  token with type %d and data (in int) %d\n", 
        token->line, token->startPosition, token->type, token->data.operatorNum);
    LOG("   Token Array pos = %d\n", arrPosition);

    return EXIT_SUCCESS;
}

//"1000-7*100/(30+5*10-5*(100/50))+1"
//"100-5*3-14/7"
