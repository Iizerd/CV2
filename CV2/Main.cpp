
#include "XedWrap.h"
#include "InstRope.h"
#include "NativeRope.h"
#include "FunctionBlock.h"

#include <iomanip>

UCHAR TestArray[] = { 0x48, 0x09, 0xC0, 0x48, 0x09, 0xC0, 0x75, 0x06, 0x48, 0x21, 0xDB, 0x48, 0x21, 0xDB, 0x48, 0x31, 0xC9, 0x48, 0x31, 0xC9 };

UCHAR RipRelTestArray[] = { 0x48, 0x8B, 0x05, 0x17, 0x00, 0x00, 0x00, 0x48, 0x89, 0x05, 0xE7, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0xF0, 0x06, 0x00, 0x00, 0x48, 0x31, 0xC0, 0x48, 0x31, 0xC0, 0x31, 0xC0, 0x48, 0x8D, 0x05, 0x02, 0x00, 0x00, 0x00 };

int main()
{
	XedGlobalInit();

	NATIVE_BLOCK Block;
	Block.Back = Block.Front = NULL;
	NrDissasemble(&Block, RipRelTestArray, sizeof(RipRelTestArray));
	NrPromoteAllRelativeJumpsTo32BitDisplacement(&Block);
	NrHandleRipRelativeInstructions(&Block);

	UINT AsmSize = 0;
	PVOID Asm = NrAssemble(&Block, &AsmSize);

	for (ULONG i = 0; i < AsmSize; i++)
		std::cout << std::setw(2) << std::setfill('0') << std::hex << (INT)((PUCHAR)Asm)[i] << ' '; //printf("%X ", ((PUCHAR)Asm)[i]);
	printf("\n");

	//NrDebugPrintIClass(&Block);
	PFUNCTION_BLOCK FbTree = FbCreateTree(&Block);
	FbPrintNotTakenPath(FbTree);
	FbFreeTree(FbTree);
	system("pause");

	return 1;
}

