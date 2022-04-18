#ifndef __JIT_H
#define __JIT_H

//hmmmmm HMMMMM..... just in time interpreter IS a virtual machine 
//I want to be able to create blocks of rip relative instructions, that xor a block of text to result in code.
//then the tail block of it which puts it back to text.

#include "Windas.h"
#include "NativeRope.h"

#define JIT_PRETYPE_RANDOM	0
#define JIT_PRETYPE_XOR		1
#define JIT_PRETYPE_AND		2
#define JIT_PRETYPE_OR		3
#define JIT_PRETYPE_MOV		4

#define JIT_POSTTYPE_RANDOM 0
#define JIT_POSTTYPE_XOR	1
#define JIT_POSTTYPE_MOV	2

//typedef union _JIT_TYPE
//{
//	UINT16 Raw;
//	struct
//	{
//		UINT8 PreType;
//		UINT8 PostType;
//	};
//}JIT_TYPE, *PJIT_TYPE;

typedef UINT16 JIT_TYPE;

#define JitPreType(JitType) (((PUINT8)&JitType)[0])
#define JitPostType(JitType) (((PUINT8)&JitType)[1])
#define MakeJitType(Pre, Post) (JIT_TYPE)(((UINT8)Pre&0xFF) | (((UINT8)Post&0xFF) << 8))


//If !Target or !Length then use random data.
BOOLEAN JitMakeJitter(PNATIVE_LINK Inst, UINT32 InstLabel, PNATIVE_BLOCK PreBlock, PNATIVE_BLOCK PostBlock, PUCHAR Text, ULONG TextLength, JIT_TYPE JitType);

BOOLEAN JitMakeText(PNATIVE_BLOCK Block, PNATIVE_BLOCK PreBlock, PNATIVE_BLOCK PostBlock, STDSTRING CONST& Text, JIT_TYPE JitType);

#endif