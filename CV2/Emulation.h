#ifndef __EMULATION_H
#define __EMULATION_H


#include "Windas.h"
#include "NativeRope.h"

#define DEADSTORE_METHOD_PUSH		0
#define DEADSTORE_METHOD_MOV		1
#define DEADSTORE_METHOD_BITWISE	2
#define DEADSTORE_METHOD_RANDOM		3


/*
* 
*	PUSH/POP RBP/RSP
*	ADD/SUB RBP/RSP
*	MOV	RBP,RSP
* 
*/

BOOLEAN EmPush(PNATIVE_BLOCK Block, XED_REG_ENUM Register);

BOOLEAN EmPop(PNATIVE_BLOCK Block, XED_REG_ENUM Register);

BOOLEAN EmAddRegImm(PNATIVE_BLOCK Block, XED_REG_ENUM Register, INT32 Imm);

BOOLEAN EmSubRegImm(PNATIVE_BLOCK Block, XED_REG_ENUM Register, INT32 Imm);

BOOLEAN EmMovRegReg(PNATIVE_BLOCK Block, XED_REG_ENUM Register1, XED_REG_ENUM Register2);


//TODO: Add pre-assembly operations so that all of these no longer need group tags.

PNATIVE_LINK EmBranch(XED_ICLASS_ENUM BranchIClass, INT32 TargetLabelId, UINT32 DispWidthBits);

PNATIVE_LINK EmUnConditionalBranch(INT32 TargetLabelId, UINT32 DispWidthBits);

//Pops the return address into a space allocated in the code, POP [RIP+??]
BOOLEAN EmRet1(PNATIVE_BLOCK Block, UINT32 JunkSize);

//Deadstores 0xc3 somewhere ahead of a jump, make relative jump to it.
BOOLEAN EmRet2(PNATIVE_BLOCK Block, UINT32 JunkSize, UINT32 DeadstoreMethod);

PREOP_STATUS EmInternalRipDeltaFinder(PNATIVE_LINK Link, PVOID Context);

//Convert relative non conditional jump to absolute one.
BOOLEAN EmRelativeNonConditionalJumpToAbsolute(PNATIVE_BLOCK Block, INT32 TargetLabelId, INT64 LeftOver);

//Convert relative conditional jump to absolute one using previous function.
BOOLEAN EmRelativeConditionalJumpToAbsolute(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, PLABEL_MANAGER LabelManager, INT64 LeftOver);

//Convert relative conditional jump to absolute one using CMOVcc to edit rip.
BOOLEAN EmRelativeConditionalJumpToAbsolute2(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, PLABEL_MANAGER LabelManager, INT64 LeftOver);

//Same as above, but with added "obfuscation" of inverting the CMOVcc if you want
BOOLEAN EmRelativeConditionalJumpToAbsolute2Randomize(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, PLABEL_MANAGER LabelManager, INT64 LeftOver);

//Maybe do this with flags? but then the jump target must have a popfq when it starts.
BOOLEAN EmNonConditionalConditionalBranch(PNATIVE_BLOCK Block);



#endif

