#ifndef __VIRTUAL_INST_H
#define __VIRTUAL_INST_H

#include "Windas.h"
#include "VmStructure.h"

typedef struct _VIRTUAL_INSTRUCTION_REPRESENTATION
{
	USHORT OpCode;

}VIRT_INST_REP, *PVIRT_INST_REP;

PVOID* ViGetNativeHandler(UINT32 IClass, UINT32 OpSize1 = 0, UINT32 OpSize2 = 0);

PVOID* ViGetMemoryOperandHandler(UINT32 LoadStore, UINT32 IReg, UINT32 OpSize, UINT32 MemOpType);

PVOID* ViGetRegisterOperandHandler(UINT32 LoadStore, UINT32 IReg, UINT32 OpSize);

PVOID* ViGetImmediateOperandHandler(UINT32 IReg, UINT32 OpSize);

#endif