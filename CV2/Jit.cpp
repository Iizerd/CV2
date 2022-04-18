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

XED_ICLASS_ENUM JitPreTypeToIClass(JIT_TYPE JitType)
{
	switch (JitPreType(JitType))
	{
	case JIT_PRETYPE_XOR: return XED_ICLASS_XOR;
	case JIT_PRETYPE_AND: return XED_ICLASS_AND;
	case JIT_PRETYPE_OR: return XED_ICLASS_OR;
	case JIT_PRETYPE_MOV: return XED_ICLASS_MOV;
	default: return XED_ICLASS_INVALID;
	}
}

XED_ICLASS_ENUM JitPostTypeToIClass(JIT_TYPE JitType)
{
	switch (JitPostType(JitType))
	{
	case JIT_POSTTYPE_XOR: return XED_ICLASS_XOR;
	case JIT_POSTTYPE_MOV: return XED_ICLASS_AND;
	default: return XED_ICLASS_INVALID;
	}
}

PNATIVE_LINK JitCreateJitInstLink(UINT32 ImmSize, JIT_TYPE JitType)
{
	UINT32 ImmSizeBits = ImmSize * 8;
	XED_ENCODER_INSTRUCTION InstList;
	XedInst2(&InstList, XedGlobalMachineState, JitPreTypeToIClass(JitType), ImmSizeBits, 
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

BOOLEAN JitMakeLinks(PUCHAR InstData, PNATIVE_LINK* PreLink, PNATIVE_LINK* PostLink, PUCHAR Text, UINT32 Length, UINT32 InstLabel, INT32 Offset, JIT_TYPE JitType)
{
	UCHAR Buffer[4];
	switch (JitPreType(JitType))
	{
	case JIT_PRETYPE_XOR:
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

		for (UINT32 i = 0; i < Length; i++)
		{
			Buffer[i] = InstData[i] ^ Text[i];
			InstData[i] = Text[i];
		}

		PNATIVE_LINK _PostLink = NULL, _PreLink = JitCreateJitInstLink(Length, JitType);
		if (!_PreLink)
			return FALSE;

		_PreLink->LinkData.Id = InstLabel;
		NrAddPreAssemblyOperation(_PreLink, JitPreop, (PVOID)(INT64)Offset, 0UL, FALSE);
		switch (Length)
		{
		case 1: *(PUINT8)&(((PUCHAR)_PreLink->RawData)[_PreLink->RawDataSize - Length]) = *(PUINT8)Buffer; break;
		case 2: *(PUINT16)&(((PUCHAR)_PreLink->RawData)[_PreLink->RawDataSize - Length]) = *(PUINT16)Buffer; break;
		case 4:	*(PUINT32)&(((PUCHAR)_PreLink->RawData)[_PreLink->RawDataSize - Length]) = *(PUINT32)Buffer; break;
		}

		switch (JitPostType(JitType))
		{
		case JIT_POSTTYPE_MOV:
			_PostLink = JitCreateJitInstLink(Length, JitType);
			if (!_PostLink)
			{
				MLog("failed to make mov xor postlink.\n");
				NrFreeLink(_PreLink);
				return FALSE;
			}
			break;
		case JIT_POSTTYPE_XOR:
			_PostLink = NrAllocateLink();
			if (!PostLink)
			{
				MLog("Failed to make jit xor postlink.\n");
				NrFreeLink(_PreLink);
				return FALSE;
			}
			NrDeepCopyLink(_PostLink, _PreLink);

		}
		*PostLink = _PostLink;
		*PreLink = _PreLink;
		
		return TRUE;
	}
	case JIT_PRETYPE_AND:
	case JIT_PRETYPE_OR:
	case JIT_PRETYPE_MOV:
		return FALSE;
	}
	return FALSE;
}

BOOLEAN JitMakeJitter(PNATIVE_LINK Inst, UINT32 InstLabel, PNATIVE_BLOCK PreBlock, PNATIVE_BLOCK PostBlock, PUCHAR Text, ULONG TextLength, JIT_TYPE JitType)
{
	if (!Inst->RawData || !Inst->RawDataSize)
		return FALSE;

	PreBlock->Front = PreBlock->Back = PostBlock->Front = PostBlock->Back = NULL;
	LmClear(&PreBlock->LabelManager);
	LmClear(&PostBlock->LabelManager);

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

	//Pre and post have the same operand width... This is ungood. Fix
	STDVECTOR<UINT32> JitSizes;
	JitGetSizes(Inst->RawDataSize, &JitSizes);
	INT32 Offset = 0;
	for (UINT32 Size : JitSizes)
	{
		PNATIVE_LINK PreLink = NULL, PostLink = NULL;
		if (!JitMakeLinks(&((PUCHAR)Inst->RawData)[Offset], &PreLink, &PostLink, &FullText[Offset], Size, InstLabel, Offset, JitType) || !PreLink || !PostLink)
		{
			MLog("JitMakeLinks Failed\n");
			Free(FullText);
			return FALSE;
		}

		IrPutLinkBack(PreBlock, PreLink);
		IrPutLinkBack(PostBlock, PostLink);
		Offset += Size;
	}
	Free(FullText);
}

BOOLEAN JitMakeText(PNATIVE_BLOCK Block, PNATIVE_BLOCK PreBlock, PNATIVE_BLOCK PostBlock, STDSTRING CONST& Text, JIT_TYPE JitType)
{
	if (JitPreType(JitType) == JIT_PRETYPE_RANDOM)
		JitPreType(JitType) = RndGetRandomNum<UINT32>(1, 3);
	if (JitPostType(JitType) == JIT_POSTTYPE_RANDOM)
		JitPostType(JitType) = RndGetRandomNum<INT32>(1, 2);

	UINT32 TextOffset = 0;
	LABEL_MANAGER LabelState = LmSave(Block->LabelManager);
	PreBlock->Front = PreBlock->Back = PostBlock->Front = PostBlock->Back = NULL;
	LmClear(&PreBlock->LabelManager);
	LmClear(&PostBlock->LabelManager);
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
				NrFreeBlock(PreBlock);
				NrFreeBlock(PostBlock);
				LmRestore(Block->LabelManager, LabelState);
				return FALSE;
			}
			LabelId = LmNextId(&Block->LabelManager);
			NrInitForLabel(LabelLink, LabelId, NULL, NULL);
			IrInsertLinkBefore(Block, T, LabelLink);
		}
		NATIVE_BLOCK _PreBlock, _PostBlock;
		if (!JitMakeJitter(T, LabelId, &_PreBlock, &_PostBlock, (PUCHAR)&Text[TextOffset], Text.length() - TextOffset, JitType))
		{
			NrFreeBlock(PreBlock);
			NrFreeBlock(PostBlock);
			LmRestore(Block->LabelManager, LabelState);
			return FALSE;
		}

		T->LinkData.Flags |= CODE_FLAG_DO_NOT_TOUCH;
		IrPutBlockBack(PreBlock, &_PreBlock);
		IrPutBlockBack(PostBlock, &_PostBlock);
		TextOffset += T->RawDataSize;
	}
	return TRUE;
}
