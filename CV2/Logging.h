#ifndef __LOGGING_H
#define __LOGGING_H

#include "Windas.h"

//rework this so that its threadsafe.

//#define RECORD_LOG
//#define THREAD_SAFE_LOG
#define PRINT_LOG

#if defined(RECORD_LOG)
#define MLog _RecordLog
#elif defined(PRINT_LOG)
#define MLog printf
#else
#define MLog(X)
#endif

VOID _RecordLog(STDSTRING CONST& Str);

VOID DisplayLog();

#endif