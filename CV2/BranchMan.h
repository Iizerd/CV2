#ifndef __BRANCH_MAN_H
#define __BRANCH_MAN_H

#include "NativeRope.h"

#define DEADSTORE_METHOD_PUSH		0
#define DEADSTORE_METHOD_MOV		1
#define DEADSTORE_METHOD_BITWISE	2
#define DEADSTORE_METHOD_RANDOM		3

//Pops the return address into a space allocated in the code, POP [RIP+??]
//TODO: Add pre-assembly operations so that it doesnt need to be a single group.
BOOLEAN BmGenerateEmulateRet1(PNATIVE_BLOCK Block, UINT JunkSize);

//Deadstores 0xc3 somewhere ahead of a jump, make relative jump to it.
//TODO: Add pre-assembly operations so that it doesnt need to be a single group.
BOOLEAN BmGenerateEmulateRet2(PNATIVE_BLOCK Block, UINT JunkSize, UINT DeadstoreMethod);

//Convert relative non conditional jump to absolute one.
BOOLEAN BmConvertRelNonConJumpToAbsolute(PNATIVE_BLOCK Block, INT32 Displacement, UINT JunkSize);

//Convert relative conditional jump to absolute one.
BOOLEAN BmConvertRelConJumpToAbsolute(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 Displacement, UINT JunkSize);

//Maybe do this with flags? but then the jump target must have a popfq when it starts.
BOOLEAN BmGenerateNonConditionalConditionalBranch(PNATIVE_BLOCK Block);


#endif