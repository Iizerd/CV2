
#include "XedWrap.h"
#include "InstRope.h"
#include "NativeRope.h"


UCHAR TestArray[] = { 0x48, 0x89, 0xC0, 0x48, 0x31, 0xC0, 0xEB, 0x03, 0x48, 0x89, 0xC0, 0x48, 0x31, 0xC0 };

int main()
{
	XedGlobalInit();

	NATIVE_BLOCK Block;
	Block.Back = Block.Front = NULL;
	NrDissasemble(&Block, TestArray, sizeof(TestArray));
	NrDebugPrintIClass(&Block);
	system("pause");

	return 1;
}

