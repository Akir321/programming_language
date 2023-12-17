#ifndef  __TREE_SIMPLIFY_H__
#define  __TREE_SIMPLIFY_H__

#include "tree_of_expressions.h"

int expTreeSimplify           (Evaluator *eval, Node *node);

int expTreeSimplifyConsts     (Evaluator *eval, Node *node);
int expTreeSimplifyNeutralElem(Evaluator *eval, Node *node);

int tryNodeSimplify           (Evaluator *eval, Node *node);

int casePlus0 (Evaluator *eval, Node *node, Node *zero, Node *savedNode);
int caseTimes1(Evaluator *eval, Node *node, Node *one,  Node *savedNode);
int caseTimes0(Evaluator *eval, Node *node, Node *zero);


#endif //__TREE_SIMPLIFY_H__