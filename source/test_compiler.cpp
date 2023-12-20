#include <stdio.h>

#include "tree_of_expressions.h"
#include "exp_tree_write.h"
#include "tree_graphic_dump.h"
#include "recursive_descent_reading.h"
#include "tree_simplify.h"
#include "assembler_code.h"

const char *fileName = "square_solver.txt";

int main()
{
    Evaluator eval = {};
    
    readTreeFromFileRecursive(&eval, fileName);
    treeGraphicDump(&eval, eval.tree.root);
    if (eval.tree.root == PtrPoison)
    {
        printf("SYNTAX_ERROR detected\n");
        return 0;
    }

    expTreeSimplify(&eval, eval.tree.root);
    treeGraphicDump(&eval, eval.tree.root);

    createAssemblerCodeFile(&eval, fileName);

    evaluatorDtor(&eval);
}
