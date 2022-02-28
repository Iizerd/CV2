#include "Obfuscator.h"
#include "NativeRope.h"

BOOLEAN ObfGenerateRetReplacement(PNATIVE_BLOCK Block, UINT JunkSize)
{
	//-POP[RIP + 6 + Delta]
	//-JMP[RIP + Delta]

	XED_ENCODER_INSTRUCTION InstList[2];
	XedInst1(&InstList[0], XedGlobalMachineState, XED_ICLASS_POP, 64, XedMemBD(XED_REG_RIP, XedDisp(6 + JunkSize, 32), 64));
	XedInst1(&InstList[1], XedGlobalMachineState, XED_ICLASS_JMP, 64, XedMemBD(XED_REG_RIP, XedDisp(JunkSize, 32), 64));
	
	UINT OutSize = 0;
	PUCHAR EncodedInst = XedEncodeInstructions(InstList, 2, &OutSize);
	if (!EncodedInst || !OutSize)
		return FALSE;
	
	Block->Front = Block->Back = NULL;
	if (!NrDissasemble(Block, EncodedInst, OutSize))
		return FALSE;

	free(EncodedInst);

	PNATIVE_LINK JunkLink = NrAllocateLink();
	if (!JunkLink)
	{
		NrFreeBlock(Block);
		return FALSE;
	}

	NrInitZero(JunkLink);
	JunkLink->RawInstSize = JunkSize + 8;
	JunkLink->RawInstData = malloc(JunkSize + 8);
	JunkLink->LinkData.Flags |= CODE_FLAG_IS_RAW_DATA;
	if (!JunkLink->RawInstData)
	{
		NrFreeLink(JunkLink);
		NrFreeBlock(Block);
		return FALSE;
	}

	IrPutLinkBack(Block, JunkLink);

	return TRUE;

}

BOOLEAN ObfGenerateRelJumpReplacement(PNATIVE_BLOCK Block, INT32 Displacement)
{

}

