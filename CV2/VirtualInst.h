#ifndef __VIRTUAL_INST_H
#define __VIRTUAL_INST_H

#include "Windas.h"
#include "VmStructure.h"

typedef struct _VIRTUAL_INSTRUCTION_REPRESENTATION
{
	USHORT OpCode;

}VIRT_INST_REP, *PVIRT_INST_REP;

PVOID* ViGetNativeHandler(UINT IClass, UINT OpSize1 = 0, UINT OpSize2 = 0);

PVOID* ViGetMemoryOperandHandler(UINT LoadStore, UINT IReg, UINT OpSize, UINT MemOpType);

PVOID* ViGetRegisterOperandHandler(UINT LoadStore, UINT IReg, UINT OpSize);

PVOID* ViGetImmediateOperandHandler(UINT IReg, UINT OpSize);

#endif