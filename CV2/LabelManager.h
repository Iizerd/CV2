#ifndef __LABEL_MANAGER_H
#define __LABEL_MANAGER_H

#include "Windas.h"
#include "NativeRope.h"

INT32 LmInitForBlock(PNATIVE_BLOCK Block);

INT32 LmPeekNextId(INT32 LableManager);

INT32 LmNextId(INT32 LabelManager);

#endif