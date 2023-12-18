#ifndef  __ASSEMBLER_CODE_H__
#define  __ASSEMBLER_CODE_H__

int createAssemblerCodeFile(Evaluator *eval, const char *fileInName);

int convertToAssemblyCode(Evaluator *eval, Node *root, FILE *f);

int printAssemblyRegister(Evaluator *eval, Node *node, FILE *f);

int printCaseAssign(Evaluator *eval, Node *root, FILE *f);
int printCaseIf    (Evaluator *eval, Node *root, FILE *f);

char *getFileName(const char *fileInName, const char *postfix);

#endif //__ASSEMBLER_CODE_H__