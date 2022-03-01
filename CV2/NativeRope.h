#ifndef __NATIVE_ROPE_H
#define __NATIVE_ROPE_H

#include "InstRope.h"
#include "XedWrap.h"

#define CODE_FLAG_IS_REL_JUMP IrSpecificFlag(0)

struct _NATIVE_LINK;

typedef BOOLEAN(*FnAssemblyOperation)(_NATIVE_LINK* Link, PUCHAR RawData, PVOID Context);
typedef struct _ASSEMBLY_OPERATION
{
	_ASSEMBLY_OPERATION*	Next;
	PVOID					Context;
	FnAssemblyOperation		Operation;
}ASSEMBLY_OPERATION, *PASSEMBLY_OPERATION;

typedef struct _NATIVE_LINK
{
	_NATIVE_LINK*	 Next;
	_NATIVE_LINK*	 Prev;
	LINK_DATA		 LinkData;
//-----------END OF HEADER-----------
	PASSEMBLY_OPERATION AssemblyOperations;
	PVOID			 RawInstData;
	UINT			 RawInstSize;
	XED_DECODED_INST DecodedInst;
}NATIVE_LINK, *PNATIVE_LINK;

typedef struct _NATIVE_BLOCK
{
	PNATIVE_LINK Front;
	PNATIVE_LINK Back;
}NATIVE_BLOCK, *PNATIVE_BLOCK;


#define NrAllocateLink() AllocateS(NATIVE_LINK);
#define NrFreeLink Free

VOID NrFreeBlock(PNATIVE_BLOCK Block);

VOID NrFreeBlock2(PNATIVE_LINK Start, PNATIVE_LINK End);

VOID NrZeroLink(PNATIVE_LINK Link);

VOID NrInitForInst(PNATIVE_LINK Link);

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 LabelId, PNATIVE_LINK Next, PNATIVE_LINK Prev);

BOOLEAN NrAddAssemblyOperation(PNATIVE_LINK Link, FnAssemblyOperation Operation, PVOID Context, BOOLEAN Front);

BOOLEAN NrDeepCopyLink(PNATIVE_LINK Dest, PNATIVE_LINK Source);

BOOLEAN NrDeepCopyBlock(PNATIVE_BLOCK Dest, PNATIVE_LINK Start, PNATIVE_LINK End);

BOOLEAN NrDeepCopyBlock2(PNATIVE_BLOCK Dest, PNATIVE_BLOCK Source);

UINT NrCalcBlockSize(PNATIVE_BLOCK Block);

PNATIVE_LINK NcValidateJump(PNATIVE_LINK Jmp, INT32 Delta);

BOOLEAN NrCreateLabels(PNATIVE_BLOCK Block);

BOOLEAN NrDissasemble(PNATIVE_BLOCK Block, PVOID RawCode, UINT CodeLength);

PVOID NrAssemble(PNATIVE_BLOCK Block, PUINT AssembledSize);

VOID NrDebugPrintIClass(PNATIVE_BLOCK Block);

BOOLEAN NrAreFlagsClobbered(PNATIVE_LINK Start, PNATIVE_LINK End);


#endif
