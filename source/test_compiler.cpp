#include <stdio.h>

#include "tree_of_expressions.h"
#include "exp_tree_write.h"
#include "tree_graphic_dump.h"
#include "recursive_descent_reading.h"
#include "tree_simplify.h"
#include "assembler_code.h"

//const char *fileName = "factorial_while.txt";

int main(int argc, const char *argv[])
{
    const char *fileInName  = NULL;

    if (argc == 1)
    {
        printf("ERROR: file name not given\n");
        return 0;
    }

    fileInName = argv[1];

    Evaluator eval = {};
    
    readTreeFromFileRecursive(&eval, fileInName);
    treeGraphicDump(&eval, eval.tree.root);
    if (eval.tree.root == PtrPoison)
    {
        printf("SYNTAX_ERROR detected\n");
        return 0;
    }

    expTreeSimplify(&eval, eval.tree.root);
    treeGraphicDump(&eval, eval.tree.root);

    createAssemblerCodeFile(&eval, fileInName);

    evaluatorDtor(&eval);
}

//.\test_compiler.exe factorial_while.txt
//.\test_compiler.exe square_solver.txt
