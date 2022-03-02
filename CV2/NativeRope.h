#ifndef __NATIVE_ROPE_H
#define __NATIVE_ROPE_H

#include "InstRope.h"
#include "XedWrap.h"

#define CODE_FLAG_IS_REL_JUMP IrSpecificFlag(0)

struct _NATIVE_LINK;


typedef BOOLEAN(*FnAssemblyPreOp)(_NATIVE_LINK* Link, PVOID Context);
typedef struct _ASSEMBLY_PREOP
{
	_ASSEMBLY_PREOP*	Next;
	PVOID				Context;
	FnAssemblyPreOp		Operation;
}ASSEMBLY_PREOP, * PASSEMBLY_PREOP;

typedef BOOLEAN(*FnAssemblyPostOp)(_NATIVE_LINK* Link, PUCHAR RawData, PVOID Context);
typedef struct _ASSEMBLY_POSTOP
{
	_ASSEMBLY_POSTOP*	Next;
	PVOID				Context;
	FnAssemblyPostOp	Operation;
}ASSEMBLY_POSTOP, *PASSEMBLY_POSTOP;

typedef struct _NATIVE_LINK
{
	_NATIVE_LINK*		Next;
	_NATIVE_LINK*		Prev;
	LINK_DATA			LinkData;
//-----------END OF HEADER-----------
	PASSEMBLY_PREOP		PreAssemblyOperations;
	PASSEMBLY_POSTOP	PostAssemblyOperations;
	PVOID				RawInstData;
	UINT				RawInstSize;
	XED_DECODED_INST	DecodedInst;
}NATIVE_LINK, *PNATIVE_LINK;

typedef struct _NATIVE_BLOCK
{
	PNATIVE_LINK Front;
	PNATIVE_LINK Back;
}NATIVE_BLOCK, *PNATIVE_BLOCK;


#define NrAllocateLink() AllocateS(NATIVE_LINK);

VOID NrFreeLink(PNATIVE_LINK Link);

VOID NrFreeBlock(PNATIVE_BLOCK Block);

VOID NrFreeBlock2(PNATIVE_LINK Start, PNATIVE_LINK End);

VOID NrZeroLink(PNATIVE_LINK Link);

VOID NrInitForInst(PNATIVE_LINK Link);

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 LabelId, PNATIVE_LINK Next, PNATIVE_LINK Prev);

PNATIVE_LINK NrTraceToLabel(PNATIVE_LINK Start, PNATIVE_LINK End, ULONG Id);

PNATIVE_LINK NrTraceToMarker(PNATIVE_LINK Start, PNATIVE_LINK End, ULONG Id);

BOOLEAN NrAddPreAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPreOp Operation, PVOID Context, BOOLEAN Front);

BOOLEAN NrAddPostAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPostOp Operation, PVOID Context, BOOLEAN Front);

BOOLEAN NrDeepCopyLink(PNATIVE_LINK Dest, PNATIVE_LINK Source);

BOOLEAN NrDeepCopyBlock(PNATIVE_BLOCK Dest, PNATIVE_LINK Start, PNATIVE_LINK End);

BOOLEAN NrDeepCopyBlock2(PNATIVE_BLOCK Dest, PNATIVE_BLOCK Source);

UINT NrCalcBlockSize(PNATIVE_BLOCK Block);

PNATIVE_LINK NcValidateJump(PNATIVE_LINK Jmp, INT32 Delta);

BOOLEAN NrCreateLabels(PNATIVE_BLOCK Block);

BOOLEAN NrCalcRelativeJumpDisp(PNATIVE_LINK Link, PINT32 DeltaOut);

BOOL NrFixRelativeJumps(PNATIVE_BLOCK Block);

BOOLEAN NrDissasemble(PNATIVE_BLOCK Block, PVOID RawCode, UINT CodeLength);

PVOID NrAssemble(PNATIVE_BLOCK Block, PUINT AssembledSize);

VOID NrDebugPrintIClass(PNATIVE_BLOCK Block);

BOOLEAN NrAreFlagsClobbered(PNATIVE_LINK Start, PNATIVE_LINK End);


#endif
