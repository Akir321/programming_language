#include <stdio.h>

#include "tree_of_expressions.h"
#include "exp_tree_write.h"
#include "tree_graphic_dump.h"
#include "recursive_descent_reading.h"
#include "tree_simplify.h"

const char *fileName = "code.txt";

int main()
{
    Evaluator eval = {};
    
    readTreeFromFileRecursive(&eval, fileName);
    treeGraphicDump(&eval, eval.tree.root);

    expTreeSimplify(&eval, eval.tree.root);
    treeGraphicDump(&eval, eval.tree.root);

    evaluatorDtor(&eval);
}
