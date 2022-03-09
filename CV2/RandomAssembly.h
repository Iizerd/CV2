#ifndef __RANDOM_ASSEMBLY_H
#define __RANDOM_ASSEMBLY_H

#include "Windas.h"
#include "XedWrap.h"
#include "NativeRope.h"

XED_REG_ENUM RaGetRandomRegister(UINT WidthInBytes);

XED_ICLASS_ENUM RaGetRandomBitwiseIClass();

XED_ICLASS_ENUM RaGetRandomBranchIClass();

POSTOP_STATUS RaRandomizeInstAfterAssembly(PNATIVE_LINK Link, PUCHAR RawData, PVOID Context);

#endif
