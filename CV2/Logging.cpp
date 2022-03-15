#include "Logging.h"

//brutalll

STDVECTOR<STDSTRING> TheLog = {};
STDMUTEX LogMutex;

VOID _RecordLog(STDSTRING CONST& Str)
{
#if defined(THREAD_SAFE_LOG)
	LogMutex.lock();
	TheLog.push_back(Str);
	LogMutex.unlock();
#else
	TheLog.push_back(Str);
#endif

}

VOID DisplayLog()
{
#if defined(THREAD_SAFE_LOG)
	LogMutex.lock();
	for (UINT32 i = 0; i < TheLog.size(); i++)
		printf("%s", TheLog[i].data());
	LogMutex.unlock();
#else
		for (UINT32 i = 0; i < TheLog.size(); i++)
			printf("%s", TheLog[i].data());
#endif
}

