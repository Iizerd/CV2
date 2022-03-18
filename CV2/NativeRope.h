#ifndef __NATIVE_ROPE_H
#define __NATIVE_ROPE_H

#include "InstRope.h"
#include "XedWrap.h"

struct _NATIVE_LINK;

#define DECODER_FLAG_DONT_GENERATE_OPERATIONS IrGeneralFlag(0)		//Dont generate any pre or post operations for rip rel or branches.

#define CODE_FLAG_IS_REL_JUMP		IrSpecificFlag(0)
#define CODE_FLAG_IS_RIP_RELATIVE	IrSpecificFlag(1)
#define CODE_FLAG_IS_JUMP_TARGET	IrSpecificFlag(2)

#define ASMOP_FLAG_FREE_CONTEXT		IrGeneralFlag(0)

typedef UINT32 PREOP_STATUS;
#define PREOP_SUCCESS			0	//Continue to next operation and further instructions
#define PREOP_RESTART			1	//Go back to the first instruction and start again(most likely a displacement width changed). Try not to ever return this.
#define PREOP_CRITICAL_ERROR	2	//Cancel assembly completely. Something bad happened
typedef PREOP_STATUS(*FnAssemblyPreOp)(_NATIVE_LINK* Link, PVOID Context);
typedef struct _ASSEMBLY_PREOP
{
	_ASSEMBLY_PREOP*	Next;
	PVOID				Context;
	FnAssemblyPreOp		Operation;
	UINT32				Flags;
}ASSEMBLY_PREOP, * PASSEMBLY_PREOP;

typedef UINT32 POSTOP_STATUS;
#define POSTOP_SUCCESS			0	//Continue to next operation and further instructions
#define POSTOP_CRITICAL_ERROR	1	//Cancel assembly completely
typedef POSTOP_STATUS(*FnAssemblyPostOp)(_NATIVE_LINK* Link, PUCHAR RawData, PVOID Context);
typedef struct _ASSEMBLY_POSTOP
{
	_ASSEMBLY_POSTOP*	Next;
	PVOID				Context;
	FnAssemblyPostOp	Operation;
	UINT32				Flags;
}ASSEMBLY_POSTOP, *PASSEMBLY_POSTOP;

typedef struct _NATIVE_LINK
{
	_NATIVE_LINK*		Next;
	_NATIVE_LINK*		Prev;
	LINK_DATA			LinkData;
//-----------END OF HEADER-----------
	PASSEMBLY_PREOP		PreAssemblyOperations;
	PASSEMBLY_POSTOP	PostAssemblyOperations;
	PVOID				RawData;
	UINT32				RawDataSize;
	XED_DECODED_INST	DecodedInst;
}NATIVE_LINK, *PNATIVE_LINK;

typedef struct _NATIVE_BLOCK
{
	PNATIVE_LINK Front;
	PNATIVE_LINK Back;
}NATIVE_BLOCK, *PNATIVE_BLOCK;

typedef struct _DECODE_BLOCK
{
	NATIVE_BLOCK Block;
	INT32 StartAddress;	
	INT32 EndAddress;
}DECODE_BLOCK, * PDECODE_BLOCK;
#define NrAllocateLink() AllocateS(NATIVE_LINK)

VOID NrFreeLink(PNATIVE_LINK Link);

VOID NrFreeBlock(PNATIVE_BLOCK Block);

VOID NrFreeBlock2(PNATIVE_LINK Start, PNATIVE_LINK End);

VOID NrInitForInst(PNATIVE_LINK Link);

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 Id, PNATIVE_LINK Next, PNATIVE_LINK Prev);

VOID NrMarkBlockAsGroup(PNATIVE_BLOCK Block);

PNATIVE_LINK NrTraceToLabel(PNATIVE_LINK Start, PNATIVE_LINK End, ULONG Id);

BOOLEAN NrAddPreAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPreOp Operation, PVOID Context, UINT32 Flags, BOOLEAN Front);

BOOLEAN NrAddPostAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPostOp Operation, PVOID Context, UINT32 Flags, BOOLEAN Front);

BOOLEAN NrDeepCopyLink(PNATIVE_LINK Dest, PNATIVE_LINK Source);

BOOLEAN NrDeepCopyBlock(PNATIVE_BLOCK Dest, PNATIVE_LINK Start, PNATIVE_LINK End);

BOOLEAN NrDeepCopyBlock2(PNATIVE_BLOCK Dest, PNATIVE_BLOCK Source);

UINT32 NrCalcBlockSize(PNATIVE_BLOCK Block);

PNATIVE_LINK NrValidateDelta(PNATIVE_LINK Start, INT32 Delta, PINT32 LeftOver);

BOOLEAN NrCalcRipDelta(PNATIVE_LINK Link, PINT32 DeltaOut);

//It is very slow to promote all jumps one by one as NrEncode will do if nessicary. 
//Promoting them all now is very fast. This is because you have to iterate through all previous jumps after u change one of them.
BOOLEAN NrPromoteAllRelativeJumpsTo32BitDisplacement(PNATIVE_BLOCK Block);

PREOP_STATUS NrRelativeJumpPreOp(PNATIVE_LINK Link, PVOID Context);

PREOP_STATUS NrRipRelativePreOp(PNATIVE_LINK Link, PVOID Context);

BOOLEAN NrIsRelativeJump(PNATIVE_LINK Link);

BOOLEAN NrIsRipRelativeInstruction(PNATIVE_LINK Link, PINT32 Delta);

BOOLEAN NrHandleDisplacementInstructions(PNATIVE_BLOCK Block);

BOOLEAN NrIsAddressInDecodedBlockRange(INT32 Address, STDVECTOR<PDECODE_BLOCK>* DecodeBlocks);

INT32 NrCalculateMaxSizeOfCurrentBlock(INT32 StartAddress, STDVECTOR<PDECODE_BLOCK>* DecodeBlocks);

BOOLEAN NrGetNextDecodeBlock(INT32 Address, PINT32 OutDelta, PUINT32 OutIndex, STDVECTOR<PDECODE_BLOCK>* DecodeBlocks);

//Decodes until it descovers a jump, at which point it recursively calls itself to generate the next block, at the delta by the jump.
PDECODE_BLOCK NrDecodeToBlocks(PUCHAR CodeStart, INT32 StartAddress, INT32 MaxAddress, STDVECTOR<PDECODE_BLOCK>* DecodeBlocks);

//The imperfect decoder is for when you cant confirm that the instructions all come one after another. If there might be padding somewhere, use this.
//This will not decode multiple functions because the second function wont be referenced by a relative jump in the first one.
BOOLEAN NrDecodeImperfect(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength);

BOOLEAN NrDecodeImperfectEx(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength, UINT32 Flags);

//The perfect decoder is for decoding blocks of instructions we know contain no padding after any absolute jumps. 
BOOLEAN NrDecodePerfect(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength);

BOOLEAN NrDecodePerfectEx(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength, UINT32 Flags);

PVOID NrEncode(PNATIVE_BLOCK Block, PUINT32 AssembledSize);

VOID NrDebugPrintIClass(PNATIVE_BLOCK Block);

BOOLEAN NrAreFlagsClobbered(PNATIVE_LINK Start, PNATIVE_LINK End);


#endif
