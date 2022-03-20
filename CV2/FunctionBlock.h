#ifndef __FUNCTION_BLOCK_H
#define __FUNCTION_BLOCK_H

#include "NativeRope.h"

typedef struct _FUNCTION_BLOCK
{
	NATIVE_BLOCK Block;
	union
	{
		ULONG64 IsConditional;
		struct
		{
			_FUNCTION_BLOCK* Taken;
			_FUNCTION_BLOCK* NotTaken;
		}Conditional;
		struct
		{
			ULONG64 Dummy;
			_FUNCTION_BLOCK* NextBlock;
		}Absolute;
	};
}FUNCTION_BLOCK, * PFUNCTION_BLOCK;
STATIC_ASSERT(offsetof(FUNCTION_BLOCK, Conditional.Taken) == offsetof(FUNCTION_BLOCK, IsConditional), "FUNCTION_BLOCK union doesn't match up.");
STATIC_ASSERT(offsetof(FUNCTION_BLOCK, Absolute.Dummy) == offsetof(FUNCTION_BLOCK, IsConditional), "FUNCTION_BLOCK union doesn't match up.");
STATIC_ASSERT(offsetof(FUNCTION_BLOCK, Absolute.NextBlock) == offsetof(FUNCTION_BLOCK, Conditional.NotTaken), "FUNCTION_BLOCK union doesn't match up.");


//if the current block position independent
// - if conditional, NotTaken path is given a jump, and the corresponding block gets a label at the front(if it doesnt already have one)
// - if not conditional, and there is no jump at the end(meaning break is because of a label), a jump is added leading to a label
BOOLEAN FbMakeFunctionBlockPositionIndependent(PFUNCTION_BLOCK FunctionBlock, INT32 LabelId);

//Create a tree that represents the flow of a NATIVE_BLOCK
PFUNCTION_BLOCK FbCreateTree(PNATIVE_BLOCK Block);

//Frees the tree by iterating the NotTaken path, which goes straight through the code. Even after obfuscation(MAKE SURE THIS STILL WORKS AFTER MOVING THINGS AROUND)
VOID FbFreeTree(PFUNCTION_BLOCK TreeHead);

VOID FbPrintTakenPath(PFUNCTION_BLOCK TreeHead);

VOID FbPrintNotTakenPath(PFUNCTION_BLOCK TreeHead);



#endif

