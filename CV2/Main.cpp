
#include "XedWrap.h"
#include "InstRope.h"
#include "NativeRope.h"
#include "FunctionBlock.h"
#include "DriverIdxCall.h"
#include "Junk.h"
#include "Jit.h"
#include <iomanip>
#include <fstream>

//UCHAR TestArray[] = { 0x48, 0x09, 0xC0, 0x48, 0x09, 0xC0, 0x75, 0x06, 0x48, 0x21, 0xDB, 0x48, 0x21, 0xDB, 0x48, 0x31, 0xC9, 0x48, 0x31, 0xC9 };

//UCHAR TestArray[] = { 0x48, 0x8B, 0x05, 0x17, 0x00, 0x00, 0x00, 0x48, 0x89, 0x05, 0xE7, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0xF0, 0x06, 0x00, 0x00, 0x48, 0x31, 0xC0, 0x48, 0x31, 0xC0, 0x31, 0xC0, 0x48, 0x8D, 0x05, 0x02, 0x00, 0x00, 0x00 };

UCHAR JitTest[] = { 0x48, 0x09, 0xC9, 0x48, 0x21, 0xC9, 0x48, 0x09, 0xD2, 0x48, 0x21, 0xD2, 0x48, 0x89, 0xC8, 0x48, 0x01, 0xD0, 0x48, 0x21, 0xC0, 0x48, 0x09, 0xC0, 0x48, 0xC7, 0xC1, 0xF0, 0x06, 0x00, 0x00, 0x48, 0xC7, 0xC2, 0xCE, 0x07, 0x00, 0x00, 0x48, 0x31, 0xC9, 0x48, 0x31, 0xD2, 0xC3 };


typedef INT64(*FnAdd)(INT64 First, INT64 Second);

int main()
{
	srand(time(NULL));
	XedGlobalInit();

	UINT32 FuncAddrOff, GadgetAddrOff;
	NATIVE_BLOCK IdxCallBlock;
	if (!DiGenerateNonVolatileCallGadget(&IdxCallBlock, XED_REG_R11, &FuncAddrOff, &GadgetAddrOff))
	{
		printf("failed to create the idx call block.\n");
	}
	else
	{
		UINT32 AsmSize = 0;
		PVOID Asm = NrEncode(&IdxCallBlock, &AsmSize);

		for (ULONG i = 0; i < AsmSize; i++)
			printf("%02x ", ((PUCHAR)Asm)[i]);
		printf("\n");
		printf("%02x %02x \n", FuncAddrOff, GadgetAddrOff);
	}

	/*NATIVE_BLOCK Block;
	Block.Front = Block.Back = NULL;
	NrDecodeImperfect(&Block, JitTest, sizeof(JitTest));
	PINST_LINK RetLink = IrPopBack(&Block);
	NATIVE_BLOCK JitBlock, JitBlock2;
	if (!JitMakeText(&Block, &JitBlock, &JitBlock2, "Oh idk I guess I think about killing myself pretty frequently... And why not? Whats so great about living?", MakeJitType(JIT_PRETYPE_XOR, JIT_POSTTYPE_XOR)))
	{
		printf("failed.\n");
		system("pause");
	}
	printf("JitBlock size %d\n", IrCountLinks(&JitBlock));
	printf("JitBlock2 size %d\n", IrCountLinks(&JitBlock2));
	printf("Block size %d\n", IrCountLinks(&Block));
	IrPutBlockFront(&Block, &JitBlock);
	IrPutBlockBack(&Block, &JitBlock2);
	IrPutLinkBack(&Block, RetLink);

	NrDebugPrintIClass(&Block);

	UINT32 AsmSize = 0;
	PVOID Asm = NrEncode(&Block, &AsmSize);

	for (ULONG i = 0; i < AsmSize; i++)
		printf("%02x ", ((PUCHAR)Asm)[i]);
	printf("\n");

	std::ofstream FileThing("C:\\Users\\Iizerd\\Desktop\\Leeg Hake\\Test.m", std::ios::binary);
	FileThing.write((PCHAR)Asm, AsmSize);
	FileThing.close();


	PVOID meme = VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	RtlCopyMemory(meme, Asm, AsmSize);

	system("pause");
	printf("The value %lld\n", ((FnAdd)meme)(5, 10));*/

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

