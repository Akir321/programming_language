#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctime>
#include <sys/time.h>

#include "tree_graphic_dump.h"
#include "exp_tree_write.h"
#include "html_logfile.h"

const int DumpFileNameAddedLength = 64;
const int CommandSize             = 28;

struct timeHolder
{
    int year;
    int month;
    int day;
    int hours;
    int minutes;
    int seconds;
    int miliseconds;
};

timeHolder getTime(void);

static timeHolder OpenTime = getTime();

int treeGraphicDump(Evaluator *eval, Node *node)
{
    assert(eval);

    static int dumpNumber = 0;
    dumpNumber++;

    char *fileName = createDumpFileName(dumpNumber);

    FILE *f = fopen(fileName, "w");

    writeTreeToDotFile(eval, node, f);
    fclose(f);

    char *command = NULL;
    __mingw_asprintf(&command, "dot %s -T png -o %s.png", fileName, fileName);
    LOG("<img src = ../%s.png width = 50%%>\n",  fileName);

    system(command);
    free(fileName);
    free(command);

    return dumpNumber;
}

char *createDumpFileName(int fileNumber)
{
    int numberLength = 0;
    int number = fileNumber;

    for (  ; number > 0; number /= 10, numberLength++) {}

    int fileNameLength = DumpFileNameAddedLength + numberLength;
    char *fileName  = (char *)calloc(fileNameLength, sizeof(char));

    sprintf(fileName, "gr_dump/dump_%d.%d.%d_%d.%d.%d.%d_%d.dot",
            OpenTime.day, OpenTime.month, OpenTime.year,
            OpenTime.hours, OpenTime.minutes, OpenTime.seconds, OpenTime.miliseconds,
            fileNumber);

    return fileName;
}

timeHolder getTime()
{
    timeHolder time = {};

    timeval openTime        = {};
    time_t  openTimeSec     = {};
    tm      openTimeTM = {};
    
    if (gettimeofday(&openTime, NULL) == -1)
    {
        printf("couldn't get time for logfile name\n"); 
        return time;
    }
    
    openTimeSec = openTime.tv_sec;

    if (localtime_s(&openTimeTM, &openTimeSec) != 0)
    {
        printf("couldn't convert time to tm\n"); 
        return time;
    }

    time.year        = openTimeTM.tm_year + 1900;
    time.month       = openTimeTM.tm_mon + 1;
    time.day         = openTimeTM.tm_mday;

    time.hours       = openTimeTM.tm_hour;
    time.minutes     = openTimeTM.tm_min;
    time.seconds     = openTimeTM.tm_sec;
    time.miliseconds = openTime.tv_usec / 1000;

    return time;
}


#define dotWrite(...) fprintf(f, __VA_ARGS__)

int writeTreeToDotFile(Evaluator *eval, Node *node, FILE *f)
{
    assert(eval);
    assert(f);

    dotWrite("digraph\n{\n");
    dotWrite("node [shape = Mrecord, color = \"navy\", ");
    dotWrite("style = filled, fillcolor = \"#bcaaa4\"];\n\n");

    dotWrite("nodeL [label = \"L\", style = filled, fillcolor = \"cornFlowerBlue\"];\n");
    dotWrite("nodeR [label = \"R\", style = filled, fillcolor = \"salmon\"];\n\n");

    dotWriteNodes(eval, node, f, 0);
    dotWrite("\n");
    dotWriteEdges(node, f);
    dotWrite("}");

    return EXIT_SUCCESS;
}

int dotWriteNodes(Evaluator *eval, Node *node, FILE *f, int rank)
{
    assert(f);

    if (!node) return 0;
    if (node == (Node *)PtrPoison) return 0;


    switch (node->type)
    {
        case EXP_TREE_NOTHING:
            dotWrite("node%p [label = \"NULL\", rank = %d,  ", node, rank);
            dotWrite("fillcolor = \"#fafafa\"];\n");
            break;

        case EXP_TREE_NUMBER:
            dotWrite("node%p [label = \"%lg\", rank = %d, ", node, node->data.number, rank);
            dotWrite("fillcolor = \"#f0f4c3\"];\n");
            break;

        case EXP_TREE_OPERATOR:
        {
            dotWrite("node%p [label = \"", node);
            printNodeSymbol(eval, node, f);
            dotWrite("\", rank = %d, ", rank);
            dotWrite("fillcolor = \"#66bb6a\"];\n");
            break;
        }
        case EXP_TREE_VARIABLE:
            dotWrite("node%p [label = \"%s\", rank = %d, ", 
                      node, eval->names.table[node->data.variableNum].name, rank);
            dotWrite("fillcolor = \"#b39ddb\"];\n");
            break;

        default:
            dotWrite("node%p [label = \"ERROR\", rank = %d, ", node, rank);
            dotWrite("fillcolor = \"#d50000\"];\n");
            break;
    }

    dotWriteNodes(eval, node->left,  f, rank + 1);
    dotWriteNodes(eval, node->right, f, rank + 1);

    return EXIT_SUCCESS;
}

int dotWriteEdges(Node *node, FILE *f)
{
    assert(f);
    if (!node) return 0;
    if (node == (Node *)PtrPoison) return 0;

    if (node->left)  dotWrite("node%p -> node%p [color = \"cornFlowerBlue\"];\n", node, node->left);
    if (node->right) dotWrite("node%p -> node%p [color = \"salmon\"];\n",         node, node->right);

    dotWriteEdges(node->left,  f);
    dotWriteEdges(node->right, f);

    return 0;
}

#undef dotWrite
