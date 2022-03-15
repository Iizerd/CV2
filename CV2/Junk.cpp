#include "Junk.h"
#include "Logging.h"


PNATIVE_LINK JnkGeneratePadding(UINT32 Count)
{
	PNATIVE_LINK Link = NrAllocateLink();
	if (!Link)
	{
		MLog("Failed to allocate link.\n");
		return NULL;
	}
	Link->LinkData.Flags |= CODE_FLAG_IS_RAW_DATA;
	Link->RawData = Allocate(Count);
	if (!Link->RawData)
	{
		MLog("Failed to allocate raw data for function padding link.\n");
		Free(Link);
		return NULL;
	}
	Link->RawDataSize = Count;

	for (UINT32 i = 0; i < Count; i++)
		((PUCHAR)Link->RawData)[i] = 0xCC;

	return Link;
}

PNATIVE_BLOCK JnkGenerateFunctionPrologue(PNATIVE_BLOCK Block, UINT32 ArgumentCount, INT32 StackAdjustment, BOOLEAN BpBasedFrame)
{
	/*
	*	Store registers in home space, reverse order(r9 to rcx)
	*	PUSH RBP
	*	MOV RBP,RSP
	*	SUB RSP,StackAdjustment
	*/
	CONST STATIC XED_REG_ENUM FastcallRegisters[] = { XED_REG_RCX, XED_REG_RDX, XED_REG_R8, XED_REG_R9 };

	Block->Front = Block->Back = NULL;
	XED_ENCODER_INSTRUCTION InstList[7];
	UINT32 RegHomeStoreCount = MinVal(4, ArgumentCount);
	for (UINT32 i = 0; i < RegHomeStoreCount; i++)
	{
		XedInst2(&InstList[i], XedGlobalMachineState, XED_ICLASS_MOV, 64,
			XedMemBD(XED_REG_RSP, XedDisp(8 + ((RegHomeStoreCount - i - 1) * 8), 8), 64),
			XedReg(FastcallRegisters[RegHomeStoreCount - i - 1])
		);
	}

	if (BpBasedFrame)
	{
		XedInst1(&InstList[RegHomeStoreCount++], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(XED_REG_RBP));
		XedInst2(&InstList[RegHomeStoreCount++], XedGlobalMachineState, XED_ICLASS_MOV, 64, XedReg(XED_REG_RBP), XedReg(XED_REG_RSP));
	}
	XedInst2(&InstList[RegHomeStoreCount++], XedGlobalMachineState, XED_ICLASS_SUB, 64, XedReg(XED_REG_RSP), XedImm0(StackAdjustment, XedSignedDispNeededWidth(StackAdjustment)));

	UINT32 OutSize = 0;
	PUCHAR EncodedData = XedEncodeInstructions(InstList, RegHomeStoreCount, &OutSize);
	if (!EncodedData || !OutSize)
	{
		MLog("Failed to encode function prologue.\n");
		return NULL;
	}

	if (!NrDecodePerfectEx(Block, EncodedData, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
	{
		MLog("Failed to decoded encoded prologue.\n");
		Free(EncodedData);
		return NULL;
	}
	Free(EncodedData);
	return Block;
}

PNATIVE_LINK JnkGenerateNop(UINT32 SpaceOccupied)
{
	UINT32 Nines = SpaceOccupied / 9;
	UINT32 LeftOver = SpaceOccupied % 9;
	UINT32 Cursor = 0;

	PNATIVE_LINK Link = NrAllocateLink();
	if (!Link)
	{
		MLog("Failed to allocate link for Nops.\n");
		return FALSE;
	}

	Link->LinkData.Flags |= CODE_FLAG_IS_RAW_DATA;
	PUCHAR RawData = (PUCHAR)Allocate(SpaceOccupied);
	if (!RawData)
	{
		MLog("Failed to allocate space for nops.\n");
		Free(Link); //Havn't added anything special, can free it normally.
		return FALSE;
	}

	for (UINT32 i = 0; i < Nines; i++)
	{
		if (XED_ERROR_NONE != XedEncodeNop(&RawData[Cursor], 9))
		{
			MLog("Failed to encode 9 byte nop.\n");
			Free(RawData);
			Free(Link);
			return FALSE;
		}
		Cursor += 9;
	}

	if (LeftOver && XED_ERROR_NONE != XedEncodeNop(&RawData[Cursor], LeftOver))
	{
		MLog("Failed to encode nop of size %u.\n", LeftOver);
		Free(RawData);
		Free(Link);
		return FALSE;
	}

	Link->RawData = RawData;
	Link->RawDataSize = SpaceOccupied;
	return Link;
}

