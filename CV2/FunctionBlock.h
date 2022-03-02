#ifndef __FUNCTION_BLOCK_H
#define __FUNCTION_BLOCK_H

#include "NativeRope.h"

typedef struct _FUNCTION_BLOCK
{
	NATIVE_BLOCK Block;
	_FUNCTION_BLOCK* NotTaken;
	union
	{
		ULONG64 Conditional;
		_FUNCTION_BLOCK* Taken;
	};
}FUNCTION_BLOCK, *PFUNCTION_BLOCK;


PFUNCTION_BLOCK FbCreateTree(PNATIVE_BLOCK Block);

VOID FbPrintTakenPath(PFUNCTION_BLOCK TreeHead);

VOID FbPrintNotTakenPath(PFUNCTION_BLOCK TreeHead);

VOID FbFreeTree(PFUNCTION_BLOCK TreeHead);

#endif

