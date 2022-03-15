#ifndef __FUNCTION_BLOCK_H
#define __FUNCTION_BLOCK_H

#include "NativeRope.h"

//if the current block position independent
// - if conditional, NotTaken path is given a jump, and the corresponding block gets a label at the front(if it doesnt already have one)
// - if not conditional, and there is no jump at the end(meaning break is because of a label), a jump is added leading to a label
BOOLEAN FbMakeFunctionBlockPositionIndependent(PFUNCTION_BLOCK FunctionBlock, UINT32 LabelId);

#endif

