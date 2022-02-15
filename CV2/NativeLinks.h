#ifndef __NATIVE_LINKS_H
#define __NATIVE_LINKS_H

#include "InstRope.h"

typedef struct _NATIVE_LINK
{
	INST_LINK	Link;
	UINT		RawInstSize;
	PVOID		RawInstBytes;
	XED_DECODED_INST DecodedInst;
}NATIVE_LINK, *PNATIVE_LINK;

typedef struct _NATIVE_BLOCK
{
	PNATIVE_LINK Front;
	PNATIVE_LINK Back;
}NATIVE_BLOCK, *PNATIVE_BLOCK;

VOID TestFunc(PNATIVE_BLOCK a, PNATIVE_LINK b, PVOID c)
{

}

void meme()
{
	NATIVE_BLOCK Block1;
	IrFreeBlock(&Block1);

	IrForEachLink(&Block1, [](PINST_BLOCK, PINST_LINK, PVOID) -> VOID
		{

		}, NULL);
}

#endif
