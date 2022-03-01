#ifndef __BRANCH_MAN_H
#define __BRANCH_MAN_H

#include "NativeRope.h"

BOOLEAN BmGenerateRetReplacement(PNATIVE_BLOCK Block, UINT Junk);

BOOLEAN BmGenerateRelJumpReplacement(PNATIVE_BLOCK Block, INT32 Displacement);


#endif