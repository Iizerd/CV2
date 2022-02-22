#include "VirtualInst.h"

//Maybe rework this at some point
PVOID NativeHandlers[XED_ICLASS_LAST * VM_OPSIZE_COUNT * VM_OPSIZE_COUNT] = { NULL };
PVOID MemoryOperandHandlers[VM_OPHANDLER_COUNT * VM_IREG_COUNT * VM_OPSIZE_COUNT * VM_MEMOP_TYPE_COUNT] = { NULL };
PVOID RegisterOperandHandlers[VM_OPHANDLER_COUNT * VM_IREG_COUNT * VM_OPSIZE_COUNT] = { NULL };
PVOID ImmediateOperandHandlers[VM_IREG_COUNT * VM_OPSIZE_COUNT] = { NULL };

PVOID* ViGetNativeHandler(UINT IClass, UINT OpSize1 = 0, UINT OpSize2 = 0)
{
	return &NativeHandlers[
		(IClass * VM_OPSIZE_COUNT * VM_OPSIZE_COUNT) + 
		(OpSize1 * VM_OPSIZE_COUNT) + 
		OpSize2
	];
}

PVOID* ViGetMemoryOperandHandler(UINT LoadStore, UINT IReg, UINT OpSize, UINT MemOpType)
{
	return &MemoryOperandHandlers[
		(LoadStore * VM_IREG_COUNT * VM_OPSIZE_COUNT * VM_MEMOP_TYPE_COUNT) +
		(IReg * VM_OPSIZE_COUNT * VM_MEMOP_TYPE_COUNT) +
		(OpSize * VM_MEMOP_TYPE_COUNT) +
		MemOpType
	];
}

PVOID* ViGetRegisterOperandHandler(UINT LoadStore, UINT IReg, UINT OpSize)
{
	return &RegisterOperandHandlers[
		(LoadStore * VM_IREG_COUNT * VM_OPSIZE_COUNT) +
		(IReg * VM_OPSIZE_COUNT) +
		OpSize
	];
}

PVOID* ViGetImmediateOperandHandler(UINT IReg, UINT OpSize)
{
	return &ImmediateOperandHandlers[
		(IReg * VM_OPSIZE_COUNT) +
		OpSize
	];
}

