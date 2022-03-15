#ifndef __JUNK_H
#define __JUNK_H

#include "NativeRope.h"

//Generate raw data link with 0xCCs in it
PNATIVE_LINK JnkGeneratePadding(UINT32 Count);

//TODO: make xed function to get needed displacement width.
PNATIVE_BLOCK JnkGenerateFunctionPrologue(PNATIVE_BLOCK Block, UINT32 ArgumentCount, INT32 StackAdjustment, BOOLEAN BpBasedFrame);

//Generates as many 9 byte nops as possible.
PNATIVE_LINK JnkGenerateNop(UINT32 SpaceOccupied);

#endif

