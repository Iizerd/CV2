#ifndef __BRANCH_MAN_H
#define __BRANCH_MAN_H

#include "NativeRope.h"

#define DEADSTORE_METHOD_PUSH		0
#define DEADSTORE_METHOD_MOV		1
#define DEADSTORE_METHOD_BITWISE	2
#define DEADSTORE_METHOD_RANDOM		3

//TODO: Add pre-assembly operations so that all of these no longer need group tags.

//Pops the return address into a space allocated in the code, POP [RIP+??]
BOOLEAN BmGenerateEmulateRet1(PNATIVE_BLOCK Block, UINT JunkSize);

//Deadstores 0xc3 somewhere ahead of a jump, make relative jump to it.
BOOLEAN BmGenerateEmulateRet2(PNATIVE_BLOCK Block, UINT JunkSize, UINT DeadstoreMethod);

PREOP_STATUS BmAbsJumpLabelFinderPreOp(PNATIVE_LINK Link, PVOID Context);

//Convert relative non conditional jump to absolute one.
BOOLEAN BmConvertRelativeNonConditionalJumpToAbsolute(PNATIVE_BLOCK Block, INT32 TargetLabelId, INT64 LeftOver);

//Convert relative conditional jump to absolute one using previous function.
BOOLEAN BmConvertRelativeConditionalJumpToAbsolute(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, UINT32 NewLabelId, INT64 LeftOver);

//Convert relative conditional jump to absolute one using CMOVcc to edit rip.
BOOLEAN BmConvertRelativeConditionalJumpToAbsolute2(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, UINT32 NewLabelId, INT64 LeftOver);

//Maybe do this with flags? but then the jump target must have a popfq when it starts.
BOOLEAN BmGenerateNonConditionalConditionalBranch(PNATIVE_BLOCK Block);


#endif