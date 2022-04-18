#ifndef __DRIVER_IDX_CALL_H
#define __DRIVER_IDX_CALL_H

#include "Windas.h"
#include "NativeRope.h"

BOOLEAN DiGenerateNonVolatileCallGadget(PNATIVE_BLOCK Block, XED_REG_ENUM NonVolatileRegister, PUINT32 FunctionAddrOffset, PUINT32 GadgetAddrOffset);

BOOLEAN DiGenerateMovRaxJmpRax(PNATIVE_BLOCK Block);

BOOLEAN DiEnumerateCalls(PNATIVE_BLOCK Block, STDVECTOR<STDPAIR<UINT32, UINT32> >* FunctionIndices);

BOOLEAN DiEnumerateCalls2(PNATIVE_BLOCK Block, XED_REG_ENUM NonVolatileRegister, STDVECTOR<STDPAIR<UINT32, UINT32> >* FunctionIndices, STDVECTOR<UINT32>* RetGadgetIndices);

#endif
