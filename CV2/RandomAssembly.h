#ifndef __RANDOM_ASSEMBLY_H
#define __RANDOM_ASSEMBLY_H

#include "Windas.h"
#include "XedWrap.h"

XED_REG_ENUM RaGetRandomRegister(UINT WidthInBytes);

XED_ICLASS_ENUM RaGetRandomBitwiseIClass();

XED_ICLASS_ENUM RaGetRandomBranchIClass();

#endif
