#ifndef __LABEL_MANAGER_H
#define __LABEL_MANAGER_H

#include "Windas.h"

typedef INT32 LABEL_MANAGER, *PLABEL_MANAGER;

//maybe change all this sometday.
#define LmClear(LabelManager)  (*LabelManager) = 0

#define LmPeekNextId(LabelManager) ((*LabelManager) + 1)

#define LmNextId(LabelManager) (++(*LabelManager))

#define LmCurrentIdPtr(LabelManager) ((PINT32)(LabelManager))

#define LmSave(LabelManager) ((INT32)LabelManager)

#define LmRestore(LabelManager, oLabelManager) LabelManager = oLabelManager

#endif