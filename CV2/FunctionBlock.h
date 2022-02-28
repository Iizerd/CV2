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
VOID FbPrintTakenPath(PFUNCTION_BLOCK Start);
VOID FbPrintNotTakenPath(PFUNCTION_BLOCK Start);
VOID FbFreeTree(PFUNCTION_BLOCK Start);
#endif