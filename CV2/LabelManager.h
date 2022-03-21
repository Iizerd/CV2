#ifndef __LABEL_MANAGER_H
#define __LABEL_MANAGER_H

#include "Windas.h"
#include "NativeRope.h"

typedef INT32 LABEL_MANAGER, *PLABEL_MANAGER;


INLINE VOID LmInitFromBlock(PLABEL_MANAGER LabelManager, PNATIVE_BLOCK Block)
{
	IrGetMaxId(Block, LabelManager);
}

#define LmPeekNextId(LabelManager) ((*LabelManager) + 1)
//INLINE INT32 LmPeekNextId(INT32 LabelManager)
//{
//	return LabelManager + 1;
//}

#define LmNextId(LabelManager) (++(*LabelManager))
//INLINE INT32 LmNextId(INT32 LabelManager)
//{
//	return ++LabelManager;
//}

#endif