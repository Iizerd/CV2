
#include "XedWrap.h"
#include "InstRope.h"
#include "NativeRope.h"
#include "FunctionBlock.h"
#include "BranchMan.h"

#include <iomanip>

UCHAR TestArray[] = { 0x48, 0x09, 0xC0, 0x48, 0x09, 0xC0, 0x75, 0x06, 0x48, 0x21, 0xDB, 0x48, 0x21, 0xDB, 0x48, 0x31, 0xC9, 0x48, 0x31, 0xC9 };

//UCHAR TestArray[] = { 0x48, 0x8B, 0x05, 0x17, 0x00, 0x00, 0x00, 0x48, 0x89, 0x05, 0xE7, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0xF0, 0x06, 0x00, 0x00, 0x48, 0x31, 0xC0, 0x48, 0x31, 0xC0, 0x31, 0xC0, 0x48, 0x8D, 0x05, 0x02, 0x00, 0x00, 0x00 };

int main()
{
	srand(time(NULL));
	XedGlobalInit();


	//UINT OutSize = 0;
	//PUCHAR Result = XedEncodeInstructions(InstList, 10, &OutSize);
	//if (Result && OutSize)
	//{
	//	for (ULONG i = 0; i < OutSize; i++)
	//		std::cout << std::setw(2) << std::setfill('0') << std::hex << (INT)((PUCHAR)Result)[i] << ' '; //printf("%X ", ((PUCHAR)Asm)[i]);
	//	printf("\n");
	//}

	//for (int i = 0; i < 10; i++)
	//{
	//	NATIVE_BLOCK Block;
	//	Block.Front = Block.Back = NULL;
	//	BmGenerateEmulateRet2(&Block, 1, DEADSTORE_METHOD_MOV);
	//	UINT AsmSize = 0;
	//	PVOID Asm = NrEncode(&Block, &AsmSize);

	//	for (ULONG i = 0; i < AsmSize; i++)
	//		std::cout << std::setw(2) << std::setfill('0') << std::hex << (INT)((PUCHAR)Asm)[i] << ' '; //printf("%X ", ((PUCHAR)Asm)[i]);
	//	printf("\n");
	//}


	NATIVE_BLOCK Block;
	Block.Back = Block.Front = NULL;
	NrDecode(&Block, TestArray, sizeof(TestArray));
	NrPromoteAllRelativeJumpsTo32BitDisplacement(&Block);
	

	PNATIVE_LINK* EnumArray = (PNATIVE_LINK*)IrEnumerateBlock(&Block, IrCountLinks(&Block));

	PNATIVE_LINK JmpLink = EnumArray[2];

	printf("Iclass for it %s\n", XedIClassEnumToString(XedDecodedInstGetIClass(&JmpLink->DecodedInst)));
	NATIVE_BLOCK ConvBlock;
	if (!BmConvertRelativeConditionalJumpToAbsolute(&ConvBlock, JmpLink, 0, 1, 0))
	{
		printf("failed to convert jmp.\n");
		return 0;
	}

	IrReplaceLinkWithBlock(&Block, JmpLink, &ConvBlock);

	UINT AsmSize = 0;
	PVOID Asm = NrEncode(&Block, &AsmSize);

	for (ULONG i = 0; i < AsmSize; i++)
		std::cout << std::setw(2) << std::setfill('0') << std::hex << (INT)((PUCHAR)Asm)[i] << ' '; //printf("%X ", ((PUCHAR)Asm)[i]);
	printf("\n");

	NrDebugPrintIClass(&Block);
	/*PFUNCTION_BLOCK FbTree = FbCreateTree(&Block);
	FbPrintNotTakenPath(FbTree);
	FbFreeTree(FbTree);*/
	NrFreeBlock(&Block);
	system("pause");

	return 1;
}

