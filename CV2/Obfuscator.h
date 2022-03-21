#ifndef __OBFUSCATOR_H
#define __OBFUSCATOR_H

#include "NativeRope.h"
#include "LabelManager.h"
/*
* Ideas:
*		Ret instruction remover:
*			- Replace all ret instructions with this:
*				- POP [RIP+6+Delta]
*				- JMP [RIP+Delta]
*			- Can change 'Delta' to make things weird and fill with random data in between.
* 
*		Relative Jump Remover:
*			- Similar to Ret removed.
*				- PUSH RAX
*				- LEA RAX,[RIP]
*				- MOV [RIP+9+Delta],RAX
*				- POP RAX
*				- ADD [RIP+6+Delta],JumpDisp
*				- JMP [RIP+Delta]
*			- Saving rip, adding the jump offset to it ourselves, and jumping to it.
*			- Can be alternatively accomplished using a 'RET' instruction.
* 
*		Opaque Branches:
*			- Duh
* 	
*/

typedef struct _OBF
{
	NATIVE_BLOCK Block;
}OBF, *POBF;

BOOLEAN ObfInitFromBlock(POBF Obf, PNATIVE_BLOCK Block);


#endif