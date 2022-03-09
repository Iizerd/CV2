#ifndef __BRANCH_MAN_H
#define __BRANCH_MAN_H

#include "NativeRope.h"

#define DEADSTORE_METHOD_PUSH		0
#define DEADSTORE_METHOD_MOV		1
#define DEADSTORE_METHOD_BITWISE	2
#define DEADSTORE_METHOD_RANDOM		3

//Pops the return address into a space allocated in the code, POP [RIP+??]
BOOLEAN BmGenerateEmulateRet1(PNATIVE_BLOCK Block, UINT JunkSize);

//Deadstores 0xc3 somewhere ahead of a jump, make relative jump to it.
BOOLEAN BmGenerateEmulateRet2(PNATIVE_BLOCK Block, UINT JunkSize, UINT DeadstoreMethod);

BOOLEAN BmGenerateRelJumpReplacement(PNATIVE_BLOCK Block, INT32 Displacement);


#endif