#ifndef  __TREES_H__
#define  __TREES_H__

enum ExpTreeNodeType
{
    EXP_TREE_NOTHING  = 0,
    EXP_TREE_NUMBER   = 1,
    EXP_TREE_OPERATOR = 2,
    EXP_TREE_IDENTIF  = 3,
    EXP_TREE_VARIABLE = 4,
};

#define ElemNumberFormat "%lg"

enum ExpTreeOperators
{
    NOT_OPER  = 0,
    ADD       = 1,
    SUB       = 2,
    MUL       = 3,
    DIV       = 4,
    LN        = 5,
    LOGAR     = 6,
    POW       = 7,
    SIN       = 8,
    COS       = 9,
    R_BRACKET = 10,
    L_BRACKET = 11,
    ASSIGN    = 12,
    BELOW     = 13,
    ABOVE     = 14,
    IF        = 15,
    INSTR_END = 16,
    OPEN_F    = 17,
    CLOSE_F   = 18,
    WHILE     = 19,
    IN        = 20,
    OUT       = 21,
    THEN      = 22,
    EQUAL     = 23,
    NOT_EQUAL = 24,
    SQRT      = 25,
    NEW_VAR   = 26,
};

union ExpTreeData
{
    double           number;
    ExpTreeOperators operatorNum;
    int              variableNum;
    int              idNum;
};

struct Node
{
    ExpTreeNodeType type;
    ExpTreeData     data;
    
    Node *left;
    Node *right;
};

const int NamesNumber = 10;

struct Name 
{
    char           *name;
    ExpTreeNodeType type;
    double          value;
};

struct NameTable
{
    Name table[NamesNumber];
    int  count;
};

struct Tree
{
    Node *root;
    int size;
};

struct Evaluator
{
    Tree      tree;
    NameTable names;
};

enum ExpTreeOperatorPriorities
{
    PR_UNKNOWN    = -1,
    PR_NEG_NUMBER = 1,
    PR_ADD_SUB    = 2,
    PR_MUL_DIV    = 4,
    PR_UNARY      = 103,
    PR_POW        = 50,
    PR_NUMBER     = 100,
    PR_VARIABLE   = 101,
    PR_NULL       = 102,
};

enum ExpTreeErrors
{
    TREE_NO_ERROR     = 0,
    DIVISION_BY_ZERO  = -1,
    UNKNOWN_OPERATOR  = -2,
    NODE_TYPE_NOTHING = -3,
    LOG_NEGATIVE_ARG  = -4,
    LOG_BAD_BASE      = -5,
    MEMORY_ERROR      = -6,
    BAD_NODE_TYPE     = -7,
    BAD_VAR_INDEX     = -8,
};

Node * const PtrPoison = (Node *)42;

const int  DataPoison = -11111111;
const int IndexPoison = -1;

const int WordLength = 256;

const double DefaultVarValue = 0;


ExpTreeData createNodeData(ExpTreeNodeType type, double value);

Node *createNode(ExpTreeNodeType type, ExpTreeData data, Node *left, Node *right);
int destroyNode (Node **nodePtr);

int nameTableCtor    (NameTable *names);
int nameTableDtor    (NameTable *names);
int nameTableAdd     (NameTable *names, const char *name, double value);
int nameTableSetValue(NameTable *names, const char *name, double value);
int nameTableDump    (NameTable *names, FILE *f);
int nameTableFind    (NameTable *names, const char *name);

int nameTableCopy(NameTable *from, NameTable *to);

int treeCtor(Tree *tree, Node *root);
int treeDtor(Tree *tree);

int subTreeDtor(Node *root);
int treeSize   (Node *root);

int evaluatorCtor(Evaluator *eval);
int evaluatorDtor(Evaluator *eval);

bool canBeEvaluated(Node *node);

double expTreeEvaluate(Evaluator *eval, Node *root, ExpTreeErrors *error);

double NodeCalculate(double leftTree, double rightTree, 
                     ExpTreeOperators operatorType, ExpTreeErrors *error);

const double PrecisionConst = 0.000001;
bool equalDouble(double a, double b);

#endif //__TREES_H__
