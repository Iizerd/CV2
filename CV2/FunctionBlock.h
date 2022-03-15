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

//if the current block position independent
// - if conditional, NotTaken path is given a jump, and the corresponding block gets a label at the front(if it doesnt already have one)
// - if not conditional, and there is no jump at the end(meaning break is because of a label), a jump is added leading to a label
BOOLEAN FbMakeFunctionBlockPositionIndependent(PFUNCTION_BLOCK FunctionBlock, UINT32 LabelId);

VOID FbPrintTakenPath(PFUNCTION_BLOCK TreeHead);

VOID FbPrintNotTakenPath(PFUNCTION_BLOCK TreeHead);

VOID FbFreeTree(PFUNCTION_BLOCK TreeHead);

#endif

