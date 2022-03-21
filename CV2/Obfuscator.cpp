#include "Obfuscator.h"
#include "NativeRope.h"


BOOLEAN ObfInitFromBlock(POBF Obf, PNATIVE_BLOCK Block)
{
	LmInitFromBlock(&Obf->LabelManager, Block);
}
