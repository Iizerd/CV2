#ifndef __NATIVE_ROPE_H
#define __NATIVE_ROPE_H

#include "InstRope.h"
#include "XedWrap.h"

#define CODE_FLAG_DOES_READ_FLAGS IrSpecificFlag(0);
#define CODE_FLAG_DOES_WRITE_FLAGS IrSpecificFlag(1);
#define CODE_FLAG_IS_REL_JUMP IrSpecificFlag(2);

typedef struct _NATIVE_LINK
{
	_NATIVE_LINK* Next;
	_NATIVE_LINK* Prev;
	LINK_DATA	LinkData;
	PVOID		RawInstData;
	UINT		RawInstSize;
	XED_DECODED_INST DecodedInst;
}NATIVE_LINK, *PNATIVE_LINK;

typedef struct _NATIVE_BLOCK
{
	PNATIVE_LINK Front;
	PNATIVE_LINK Back;
}NATIVE_BLOCK, *PNATIVE_BLOCK;


#define NrAllocateLink() (PNATIVE_LINK)IrAllocateLink(sizeof(NATIVE_LINK));
#define NrFreeLink IrFreeLink

VOID NrInitZero(PNATIVE_LINK Link);

VOID NrInitForInst(PNATIVE_LINK Link);

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 LabelId, PNATIVE_LINK Next, PNATIVE_LINK Prev);

UINT NrCalcBlockSize(PNATIVE_BLOCK Block);

BOOLEAN NrCreateLabels(PNATIVE_BLOCK Block);

BOOLEAN NrDissasemble(PNATIVE_BLOCK Block, PVOID RawCode, UINT CodeLength);

PVOID NrAssemble(PNATIVE_BLOCK Block, PUINT AssembledSize);


#endif
