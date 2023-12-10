#include <stdio.h>

#include "tree_of_expressions.h"
#include "exp_tree_write.h"
#include "tree_graphic_dump.h"
#include "recursive_descent_reading.h"

const char *fileName = "code.txt";

int main()
{
    //Token *tokenArray = createTokenArray(Expression);
    //printTokenArray(tokenArray, stdout);

    Evaluator eval = {};
    //evaluatorCtor(&eval);

    //eval.tree.root = getG(Expression);
    readTreeFromFileRecursive(&eval, fileName);
    treeGraphicDump(&eval, eval.tree.root);


    evaluatorDtor(&eval);
}





