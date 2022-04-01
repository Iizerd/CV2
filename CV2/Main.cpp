
#include "XedWrap.h"
#include "InstRope.h"
#include "NativeRope.h"
#include "FunctionBlock.h"
#include "BranchMan.h"
#include "Junk.h"
#include "Jit.h"
#include <iomanip>

//UCHAR TestArray[] = { 0x48, 0x09, 0xC0, 0x48, 0x09, 0xC0, 0x75, 0x06, 0x48, 0x21, 0xDB, 0x48, 0x21, 0xDB, 0x48, 0x31, 0xC9, 0x48, 0x31, 0xC9 };

//UCHAR TestArray[] = { 0x48, 0x8B, 0x05, 0x17, 0x00, 0x00, 0x00, 0x48, 0x89, 0x05, 0xE7, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0xF0, 0x06, 0x00, 0x00, 0x48, 0x31, 0xC0, 0x48, 0x31, 0xC0, 0x31, 0xC0, 0x48, 0x8D, 0x05, 0x02, 0x00, 0x00, 0x00 };

UCHAR JitTest[] = { 0x48, 0xC7, 0xC0, 0xF0, 0x06, 0x00, 0x00, 0xc3};


typedef INT64(*FnBlackMan)();

int main()
{
	srand(time(NULL));
	XedGlobalInit();

	NATIVE_BLOCK Block;
	Block.Front = Block.Back = NULL;
	NrDecodeImperfect(&Block, JitTest, sizeof(JitTest));

	NATIVE_BLOCK JitBlock;
	JitMakeText(&Block, &JitBlock, "HOLYSHITIMBLACK", JIT_TYPE_XOR);
	printf("JitBlock size %d\n", IrCountLinks(&JitBlock));
	printf("Block size %d\n", IrCountLinks(&Block));
	IrPutBlockFront(&Block, &JitBlock);

	NrDebugPrintIClass(&Block);

	UINT32 AsmSize = 0;
	PVOID Asm = NrEncode(&Block, &AsmSize);

	for (ULONG i = 0; i < AsmSize; i++)
		printf("%02x ", ((PUCHAR)Asm)[i]);
	printf("\n");

	PVOID meme = VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	RtlCopyMemory(meme, Asm, AsmSize);

	system("pause");
	printf("The value %lld\n", ((FnBlackMan)meme)());

	//NATIVE_BLOCK Block;
	//Block.Front = Block.Back = NULL;
	//NrDecodeImperfect(&Block, TestArray, sizeof(TestArray));
	//NrPromoteAllRelativeJumpsTo32BitDisplacement(&Block);
	//
	//NrDebugPrintIClass(&Block);

	//NrFreeBlock(&Block);
	//
	//printf("\n\n");

	//for (int i = 0; i < 5; i++)
	//{
	//	NATIVE_BLOCK Block;
	//	Block.Front = Block.Back = NULL;
	//	JnkGenerateFunctionPrologue(&Block, i, 128, FALSE);
	//	UINT32 AsmSize = 0;
	//	PVOID Asm = NrEncode(&Block, &AsmSize);

	//	for (ULONG i = 0; i < AsmSize; i++)
	//		std::cout << std::setw(2) << std::setfill('0') << std::hex << (INT)((PUCHAR)Asm)[i] << ' '; //printf("%X ", ((PUCHAR)Asm)[i]);
	//	printf("\n");
	//}
	
	/*NATIVE_BLOCK Block;
	Block.Back = Block.Front = NULL;
	NrDecodePerfect(&Block, TestArray, sizeof(TestArray));*/
	//NrPromoteAllRelativeJumpsTo32BitDisplacement(&Block);
	
	//PNATIVE_LINK* EnumArray = (PNATIVE_LINK*)IrEnumerateBlock(&Block, IrCountLinks(&Block));
	//PNATIVE_LINK JmpLink = EnumArray[2];

	//NATIVE_BLOCK ConvBlock;
	//if (!BmConvertRelativeConditionalJumpToAbsolute(&ConvBlock, JmpLink, 0, 1, 0))
	//{
	//	printf("failed to convert jmp.\n");
	//	system("pause");
	//	return 0;
	//}

	//IrReplaceLinkWithBlock(&Block, JmpLink, &ConvBlock);

	//NrFreeLink(JmpLink);

	//UINT32 AsmSize = 0;
	//PVOID Asm = NrEncode(&Block, &AsmSize);

	//for (ULONG i = 0; i < AsmSize; i++)
	//	std::cout << std::setw(2) << std::setfill('0') << std::hex << (INT)((PUCHAR)Asm)[i] << ' '; //printf("%X ", ((PUCHAR)Asm)[i]);
	//printf("\n");

	//NrFreeBlock(&Block);
	system("pause");

	return 1;
}

