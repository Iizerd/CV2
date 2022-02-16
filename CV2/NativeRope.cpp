#include "NativeRope.h"
#include "Logging.h"

VOID NrInitZero(PNATIVE_LINK Link)
{
	RtlSecureZeroMemory(Link, sizeof(NATIVE_LINK));
}

VOID NrInitForInst(PNATIVE_LINK Link)
{
	RtlSecureZeroMemory(Link, sizeof(NATIVE_LINK));
	XedDecodedInstZeroSetMode(&Link->DecodedInst, &XedGlobalMachineState);
	Link->LinkData.GeneralFlags |= CODE_FLAG_IS_INST;
}

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 LabelId, PNATIVE_LINK Next, PNATIVE_LINK Prev)
{
	RtlSecureZeroMemory(Link, sizeof(NATIVE_LINK));
	Link->LinkData.GeneralFlags |= CODE_FLAG_IS_LABEL;
	Link->LinkData.LabelId = LabelId;
	Link->Next = Next;
	Link->Prev = Prev;
}

UINT NrCalcBlockSize(PNATIVE_BLOCK Block)
{
	UINT Total = 0;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		Total += T->RawInstSize;
	}
	return Total;
}

BOOLEAN NrCreateLabels(PNATIVE_BLOCK Block)
{
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (!(T->LinkData.GeneralFlags & CODE_FLAG_IS_INST))
			continue;

		UINT OperandCount = XedDecodedInstNumOperands(&T->DecodedInst);
		if (OperandCount != 1)
			continue;

		CONST XED_INST* Inst = XedDecodedInstInst(&T->DecodedInst);
		CONST XED_OPERAND* Operand = XedInstOperand(Inst, 0);
		if (!Operand)
			continue;

		XED_OPERAND_TYPE_ENUM OperandType = XedOperandType(Operand);
		if (OperandType != XED_OPERAND_TYPE_IMM && OperandType != XED_OPERAND_TYPE_IMM_CONST)
			continue;
		
		INT32 BranchDisplacement = XedDecodedInstGetBranchDisplacement(&T->DecodedInst);


		//continue
	}
}

BOOLEAN NrDissasemble(PNATIVE_BLOCK Block, PVOID RawCode, UINT CodeLength)
{
	PUCHAR CodePointer = (PUCHAR)RawCode;
	PUCHAR CodeEnd = CodePointer + CodeLength;
	while (CodePointer < CodeEnd)
	{
		PNATIVE_LINK Link = NrAllocateLink();
		if (!Link)
		{
			IrFreeBlock(Block);
			MLog("Could not allocate new link in NrDissasemble\n");
			return FALSE;
		}
		NrInitForInst(Link);
		UINT PossibleSize = Min(15, CodeEnd - CodePointer);

		XED_ERROR_ENUM XedError = XedDecode(&Link->DecodedInst, CodePointer, PossibleSize);
		if (XedError != XED_ERROR_NONE)
		{
			MLog("XedDecode failed in NrDissasemble. Error: %s\n", XedErrorEnumToString(XedError));
			NrFreeLink(Link);
			IrFreeBlock(Block);
			return FALSE;
		}
		
		UINT RawInstSize = XedDecodedInstGetLength(&Link->DecodedInst);
		PVOID RawInstData = malloc(RawInstSize);
		if (!RawInstData)
		{
			MLog("Could not allocate space for RawInstData in NrDissassemble\n");
			NrFreeLink(Link);
			IrFreeBlock(Block);
			return FALSE;
		}
		RtlCopyMemory(RawInstData, CodePointer, RawInstSize);
		Link->RawInstData = RawInstData;
		Link->RawInstSize = RawInstSize;

		IrPutLinkBack(Block, Link);
		CodePointer += RawInstSize;
	}
	return TRUE;
}

PVOID NrAssemble(PNATIVE_BLOCK Block, PUINT AssembledSize)
{
	if (!Block->Front || !Block->Back || !AssembledSize)
		return NULL;

	UINT TotalSize = NrCalcBlockSize(Block);
	if (!TotalSize)
		return NULL;

	//Check for jumps needing to be promoted.
	PUCHAR Buffer = (PUCHAR)malloc(TotalSize);
	if (!Buffer)
		return NULL;

	PUCHAR CodePointer = Buffer;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.GeneralFlags & CODE_FLAG_IS_LABEL)
			continue;
		RtlCopyMemory(CodePointer, T->RawInstData, T->RawInstSize);
		CodePointer += T->RawInstSize;
	}
	*AssembledSize = TotalSize;

	return NULL;
}