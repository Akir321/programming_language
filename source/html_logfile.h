#ifndef  __HTML_LOGFILE_H__
#define  __HTML_LOGFILE_H__

#include <stdio.h>

extern FILE *LogFile;

#define LOG(...) { fprintf(LogFile, __VA_ARGS__); fflush(LogFile); }

#endif //__HTML_LOGFILE_H__
