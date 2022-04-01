#include "Jit.h"
#include "Logging.h"


PREOP_STATUS JitPreop(PNATIVE_LINK Link, PVOID Context)
{
	INT32 BranchDisp = 0;
	if (!NrCalcRipDelta(Link, &BranchDisp))
	{
		MLog("Could not calculate rip delta.\n");
		return PREOP_CRITICAL_ERROR;
	}

	BranchDisp += (INT64)Context;

	XedPatchDisp(&Link->DecodedInst, (PUCHAR)Link->RawData, XedDisp(BranchDisp, 32));
	return PREOP_SUCCESS;
}

VOID JitGetSizes(UINT32 TotalSize, STDVECTOR<UINT32>* Sizes)
{
	Sizes->clear();
	while (TotalSize)
	{
		UINT32 Val = RndGetRandomNum<UINT32>(1, 4);
		if (Val > TotalSize || Val == 3)
			continue;
		Sizes->push_back(Val);
		TotalSize -= Val;
	}
}

XED_ICLASS_ENUM JitTypeToIClass(UINT32 JitType)
{
	switch (JitType)
	{
	case JIT_TYPE_XOR: return XED_ICLASS_XOR;
	case JIT_TYPE_AND: return XED_ICLASS_AND;
	case JIT_TYPE_OR: return XED_ICLASS_OR;
	case JIT_TYPE_MOV: return XED_ICLASS_MOV;
	default: return XED_ICLASS_INVALID;
	}
}

PNATIVE_LINK JitCreateJitInstLink(UINT32 ImmSize, UINT32 JitType)
{
	UINT32 ImmSizeBits = ImmSize * 8;
	XED_ENCODER_INSTRUCTION InstList;
	XedInst2(&InstList, XedGlobalMachineState, JitTypeToIClass(JitType), ImmSizeBits, 
		XedMemBD(XED_REG_RIP, XedDisp(0, 32), ImmSizeBits), 
		XedImm0(0, ImmSizeBits));

	UINT32 OutSize = 0;
	PUCHAR EncodedInst = XedEncodeInstructions(&InstList, 1, &OutSize);
	if (!EncodedInst || !OutSize)
		return NULL;

	PNATIVE_LINK Link = NrAllocateLink();
	if (!Link)
	{
		Free(EncodedInst);
		return NULL;
	}
	NrInitForInst(Link);
	XED_ERROR_ENUM XedError = XedDecode(&Link->DecodedInst, EncodedInst, OutSize);
	if (XedError != XED_ERROR_NONE)
	{
		NrFreeLink(Link);
		Free(EncodedInst);
		return NULL;
	}

	Link->RawData = EncodedInst;
	Link->RawDataSize = OutSize;
	return Link;
}

PNATIVE_LINK JitDoForData(PUCHAR InstData, PUCHAR Text, UINT32 Length, UINT32 InstLabel, INT32 Offset, UINT32 JitType)
{
	if (JitType == JIT_TYPE_RANDOM)
		JitType = RndGetRandomNum<UINT32>(1, 3);

	switch (JitType)
	{
	case JIT_TYPE_XOR:
	{
		/*
		*	Needs to be:A Looks like:B => B^X = A
		*	JitData: what the instruction gets XORd with before execution
		*	ToData: original instruction bytes.
		*
		*	Xor		: 011
		*	Curr Is : 110
		*	Must Be : 101
		*/
		PUCHAR Buffer = (PUCHAR)Allocate(Length);
		if (!Buffer)
		{
			MLog("Failed to allocate buffer.\n");
			return NULL;
		}
		for (UINT32 i = 0; i < Length; i++)
		{
			Buffer[i] = InstData[i] ^ Text[i];
			InstData[i] = Text[i];
		}
		PNATIVE_LINK JitLink = JitCreateJitInstLink(Length, JitType);
		if (!JitLink)
		{
			MLog("Failed to create Jit Inst Link.\n");
			return NULL;
		}
		JitLink->LinkData.Id = InstLabel;

		NrAddPreAssemblyOperation(JitLink, JitPreop, (PVOID)(INT64)Offset, 0UL, FALSE);

		switch (Length)
		{
		case 1: *(PUINT8) & (((PUCHAR)JitLink->RawData)[JitLink->RawDataSize - Length]) = *(PUINT8)Buffer; break;
		case 2: *(PUINT16) & (((PUCHAR)JitLink->RawData)[JitLink->RawDataSize - Length]) = *(PUINT16)Buffer; break;
		case 4:	*(PUINT32) & (((PUCHAR)JitLink->RawData)[JitLink->RawDataSize - Length]) = *(PUINT32)Buffer; break;
		}

		return JitLink;
	}
	case JIT_TYPE_AND:
	case JIT_TYPE_OR:
	case JIT_TYPE_MOV:
		return NULL;
	}
	return NULL;
}

//BE SURE TO MAKE DEEP COPY OF THIS BECAUSE YOU NEED TO XOR IT BACK AFTER EXECUTION
BOOLEAN JitMakeJitter(PNATIVE_LINK Inst, UINT32 InstLabel, PNATIVE_BLOCK JitterBlock, PUCHAR Text, ULONG TextLength, UINT32 JitType)
{
	if (!Inst->RawData || !Inst->RawDataSize)
		return FALSE;

	JitterBlock->Front = JitterBlock->Back = NULL;
	LmClear(&JitterBlock->LabelManager);

	PUCHAR FullText = (PUCHAR)Allocate(Inst->RawDataSize);
	if (!FullText)
	{
		MLog("Failed to allocate FullText.\n");
		return FALSE;
	}
	if (TextLength >= Inst->RawDataSize)
		RtlCopyMemory(FullText, Text, Inst->RawDataSize);
	else
	{
		RtlCopyMemory(FullText, Text, TextLength);
		for (INT i = TextLength; i < Inst->RawDataSize; i++)
			FullText[i] = RndGetRandomNum<UINT32>(0, 255);
	}

	STDVECTOR<UINT32> JitSizes;
	JitGetSizes(Inst->RawDataSize, &JitSizes);
	printf("Sizes %llu\n", JitSizes.size());
	INT32 Offset = 0;
	for (UINT32 Size : JitSizes)
	{
		printf("Size %u\n", Size);
		PNATIVE_LINK JitLink = JitDoForData(&((PUCHAR)Inst->RawData)[Offset], &FullText[Offset], Size, InstLabel, Offset, JitType);
		if (!JitLink)
		{
			MLog("JitDoForData Failed.\n");
			Free(FullText);
			return FALSE;
		}

		IrPutLinkBack(JitterBlock, JitLink);
		Offset += Size;
	}

	Free(FullText);
	Inst->LinkData.Flags |= CODE_FLAG_DO_NOT_TOUCH;
}

BOOLEAN JitMakeText(PNATIVE_BLOCK Block, PNATIVE_BLOCK JitInstructions, STDSTRING CONST& Text, UINT32 JitType)
{
	UINT32 TextOffset = 0;
	INT32 LabelState = LmSave(Block->LabelManager);
	JitInstructions->Front = JitInstructions->Back = NULL;
	LmClear(&JitInstructions->LabelManager);
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		INT32 LabelId;
		if (T->Prev && T->Prev->LinkData.Flags & CODE_FLAG_IS_LABEL)
			LabelId = T->Prev->LinkData.Id;
		else
		{
			PNATIVE_LINK LabelLink = NrAllocateLink();
			if (!LabelLink)
			{
				NrFreeBlock(JitInstructions);
				LmRestore(Block->LabelManager, LabelState);
				return FALSE;
			}
			LabelId = LmNextId(&Block->LabelManager);
			NrInitForLabel(LabelLink, LabelId, NULL, NULL);
			IrInsertLinkBefore(Block, T, LabelLink);
		}
		NATIVE_BLOCK Temp;
		if (!JitMakeJitter(T, LabelId, &Temp, (PUCHAR)&Text[TextOffset], Text.length() - TextOffset, JitType))
		{
			NrFreeBlock(JitInstructions);
			LmRestore(Block->LabelManager, LabelState);
			return FALSE;
		}
		IrPutBlockBack(JitInstructions, &Temp);
		TextOffset += T->RawDataSize;
	}
}
