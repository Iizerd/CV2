#ifndef __JIT_H
#define __JIT_H

//hmmmmm HMMMMM..... just in time interpreter IS a virtual machine 
//I want to be able to create blocks of rip relative instructions, that xor a block of text to result in code.
//then the tail block of it which puts it back to text.

#include "Windas.h"
#include "NativeRope.h"

#define JIT_TYPE_RANDOM 0
#define JIT_TYPE_XOR	1
#define JIT_TYPE_AND	2
#define JIT_TYPE_OR		3
#define JIT_TYPE_MOV	4

typedef struct _JIT_DATA
{
	UINT32 ImmediateSize; 
	INT32 OffsetInInstruction;
}JIT_DATA, *PJIT_DATA;

//If !Target or !Length then use random data.
BOOLEAN JitMakeJitter(PNATIVE_LINK Inst, UINT32 InstLabel, PNATIVE_BLOCK JitterBlock, PUCHAR Text, ULONG TextLength, UINT32 JitType);

BOOLEAN JitMakeText(PNATIVE_BLOCK Block, PNATIVE_BLOCK JitInstructions, STDSTRING CONST& Text, UINT32 JitType);

#endif