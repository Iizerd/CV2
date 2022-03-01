
#include "XedWrap.h"
#include "InstRope.h"
#include "NativeRope.h"
#include "FunctionBlock.h"

UCHAR TestArray[] = { 0x48, 0x09, 0xC0, 0x48, 0x09, 0xC0, 0x75, 0x06, 0x48, 0x21, 0xDB, 0x48, 0x21, 0xDB, 0x48, 0x31, 0xC9, 0x48, 0x31, 0xC9 };

int main()
{
	XedGlobalInit();

	NATIVE_BLOCK Block;
	Block.Back = Block.Front = NULL;
	NrDissasemble(&Block, TestArray, sizeof(TestArray));
	//NrDebugPrintIClass(&Block);
	PFUNCTION_BLOCK FbTree = FbCreateTree(&Block);
	FbPrintNotTakenPath(FbTree);
	FbFreeTree(FbTree);
	system("pause");

	return 1;
}

