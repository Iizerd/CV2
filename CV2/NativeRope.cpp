#include "NativeRope.h"
#include "Logging.h"

VOID NrFreeLink(PNATIVE_LINK Link)
{
	if (Link->RawInstData)
		Free(Link->RawInstData);
	for (PASSEMBLY_OPERATION AsmOp = Link->AssemblyOperations; AsmOp;)
	{
		PASSEMBLY_OPERATION NextOp = AsmOp->Next;
		if (AsmOp->Context)
			Free(AsmOp->Context);
		Free(AsmOp);
		AsmOp = NextOp;
	}
	Free(Link);
}

VOID NrFreeBlock(PNATIVE_BLOCK Block)
{
	NrFreeBlock2(Block->Front, Block->Back);
}

VOID NrFreeBlock2(PNATIVE_LINK Start, PNATIVE_LINK End)
{
	for (PNATIVE_LINK T = Start; T && T != End->Next;)
	{
		PNATIVE_LINK Next = T->Next;
		NrFreeLink(T);
		T = Next;
	}
}

VOID NrZeroLink(PNATIVE_LINK Link)
{
	RtlSecureZeroMemory(Link, sizeof(NATIVE_LINK));
}

VOID NrInitForInst(PNATIVE_LINK Link)
{
	RtlSecureZeroMemory(Link, sizeof(NATIVE_LINK));
	XedDecodedInstZeroSetMode(&Link->DecodedInst, &XedGlobalMachineState);
	Link->LinkData.Flags |= CODE_FLAG_IS_INST;
}

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 LabelId, PNATIVE_LINK Next, PNATIVE_LINK Prev)
{
	RtlSecureZeroMemory(Link, sizeof(NATIVE_LINK));
	Link->LinkData.Flags |= CODE_FLAG_IS_LABEL;
	Link->LinkData.Id = LabelId;
	Link->Next = Next;
	Link->Prev = Prev;
}

BOOLEAN NrAddAssemblyOperation(PNATIVE_LINK Link, FnAssemblyOperation Operation, PVOID Context, BOOLEAN Front)
{
	PASSEMBLY_OPERATION OpStruct = AllocateS(ASSEMBLY_OPERATION);
	if (!OpStruct)
	{
		MLog("Failed to allocate memory for Operation Structure.\n");
		return FALSE;
	}
	OpStruct->Context = Context;
	OpStruct->Operation = Operation;
	
	if (!Link->AssemblyOperations)
		Link->AssemblyOperations = OpStruct;
	else if (Front)
	{
		OpStruct->Next = Link->AssemblyOperations;
		Link->AssemblyOperations = OpStruct;
	}
	else
	{
		for (PASSEMBLY_OPERATION T = Link->AssemblyOperations; T; T = T->Next)
		{
			if (T->Next == NULL)
			{
				T->Next = OpStruct;
				break;
			}
		}
	}
	return TRUE;
};

BOOLEAN NrDeepCopyLink(PNATIVE_LINK Dest, PNATIVE_LINK Source)
{
	if (Source->LinkData.Flags & CODE_FLAG_IS_INST)
	{
		*(PVOID*)&Dest->LinkData = *(PVOID*)&Source->LinkData;
		Dest->RawInstSize = Source->RawInstSize;
		Dest->RawInstData = Allocate(Source->RawInstSize);
		if (!Dest->RawInstData)
			return FALSE;
		RtlCopyMemory(Dest->RawInstData, Source->RawInstData, Source->RawInstSize);

		XED_ERROR_ENUM XedError = XedDecode(&Dest->DecodedInst, (CONST PUCHAR)Dest->RawInstData, Dest->RawInstSize);
		if (XedError != XED_ERROR_NONE)
		{
			MLog("Failed to decode in NrDeepCopy. Error: %s\n", XedErrorEnumToString(XedError));
			Free(Dest->RawInstData);
			return FALSE;
		}
	}
	else //its a lebl
	{
		*(PVOID*)&Dest->LinkData = *(PVOID*)&Source->LinkData;
	}
}

BOOLEAN NrDeepCopyBlock(PNATIVE_BLOCK Dest, PNATIVE_LINK Start, PNATIVE_LINK End)
{
	Dest->Front = Dest->Back = NULL;
	for (PNATIVE_LINK T = Start; T && T != End->Next; T = T->Next)
	{
		PNATIVE_LINK Link = NrAllocateLink();
		if (!NrDeepCopyLink(Link, T))
		{
			MLog("Failed to create deep copy of link.\n");
			NrFreeLink(Link);
			NrFreeBlock(Dest);
			return FALSE;
		}
		IrPutLinkBack(Dest, Link);
	}
	return TRUE;
}

BOOLEAN NrDeepCopyBlock2(PNATIVE_BLOCK Dest, PNATIVE_BLOCK Source)
{
	return NrDeepCopyBlock(Dest, Source->Front, Source->Back);
}

UINT NrCalcBlockSize(PNATIVE_BLOCK Block)
{
	UINT Total = 0;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		Total += T->RawInstSize;
	}
	return Total;
}

PNATIVE_LINK NcValidateJump(PNATIVE_LINK Jmp, INT32 Delta)
{
	PNATIVE_LINK T;
	if (Delta > 0)
	{
		T = Jmp->Next;
		while (Delta > 0 && T)
		{
			if (T->LinkData.Flags & CODE_FLAG_IS_INST)
				Delta -= XedDecodedInstGetLength(&T->DecodedInst);
			T = T->Next;
		}
		if (Delta != 0 || !T)
			return NULL;
		while (T && !(T->LinkData.Flags & CODE_FLAG_IS_INST))
			T = T->Next;
		return T;
	}
	else if (Delta < 0)
	{
		T = Jmp;
		while (T)
		{
			if (T->LinkData.Flags & CODE_FLAG_IS_INST)
			{
				Delta += XedDecodedInstGetLength(&T->DecodedInst);
				if (Delta >= 0)
					break;
			}
			T = T->Prev;
		}
		if (Delta != 0 || !T)
			return NULL;
		while (T && !(T->LinkData.Flags & CODE_FLAG_IS_INST))
			T = T->Next;
		return T;
	}
	return Jmp->Next;
}

BOOLEAN NrCreateLabels(PNATIVE_BLOCK Block)
{
	INT32 CurrentLabelId = 0;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (!(T->LinkData.Flags & CODE_FLAG_IS_INST))
			continue;

		UINT OperandCount = XedDecodedInstNumOperands(&T->DecodedInst);
		if (OperandCount < 1)
			continue;

		XED_CATEGORY_ENUM Category = XedDecodedInstGetCategory(&T->DecodedInst);
		if (Category != XED_CATEGORY_COND_BR && Category != XED_CATEGORY_UNCOND_BR)
			continue;

		CONST XED_INST* Inst = XedDecodedInstInst(&T->DecodedInst);
		CONST XED_OPERAND* Operand = XedInstOperand(Inst, 0);
		if (!Operand)
			continue;

		XED_OPERAND_TYPE_ENUM OperandType = XedOperandType(Operand);
		if (OperandType != XED_OPERAND_TYPE_IMM && OperandType != XED_OPERAND_TYPE_IMM_CONST)
			continue;
		
		INT32 BranchDisplacement = XedDecodedInstGetBranchDisplacement(&T->DecodedInst);
		PNATIVE_LINK TargetLink = NcValidateJump(T, BranchDisplacement);
		if (!TargetLink)
		{
			MLog("Failed to validate jump. [%s][%d]\n", XedCategoryEnumToString(Category), BranchDisplacement);
			return FALSE;
		}

		if (TargetLink->Prev && (TargetLink->Prev->LinkData.Flags & CODE_FLAG_IS_LABEL))
			T->LinkData.Id = TargetLink->Prev->LinkData.Id;
		else
		{
			PNATIVE_LINK LabelLink = NrAllocateLink();
			NrInitForLabel(LabelLink, CurrentLabelId, NULL, NULL);
			IrInsertLinkBefore(Block, TargetLink, LabelLink);
			T->LinkData.Id = CurrentLabelId;
			++CurrentLabelId;
		}

		T->LinkData.Flags |= (CODE_FLAG_IS_REL_JUMP | CODE_FLAG_USES_LABEL);
	}
}

BOOLEAN NrDissasemble(PNATIVE_BLOCK Block, PVOID RawCode, UINT CodeLength)
{
	if (!CodeLength)
		return FALSE;

	PUCHAR CodePointer = (PUCHAR)RawCode;
	PUCHAR CodeEnd = CodePointer + CodeLength;
	while (CodePointer < CodeEnd)
	{
		PNATIVE_LINK Link = NrAllocateLink();
		if (!Link)
		{
			NrFreeBlock(Block);
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
			NrFreeBlock(Block);
			return FALSE;
		}
		
		UINT RawInstSize = XedDecodedInstGetLength(&Link->DecodedInst);
		PVOID RawInstData = Allocate(RawInstSize);
		if (!RawInstData)
		{
			MLog("Could not allocate space for RawInstData in NrDissassemble\n");
			NrFreeLink(Link);
			NrFreeBlock(Block);
			return FALSE;
		}
		RtlCopyMemory(RawInstData, CodePointer, RawInstSize);
		Link->RawInstData = RawInstData;
		Link->RawInstSize = RawInstSize;

		IrPutLinkBack(Block, Link);
		CodePointer += RawInstSize;
	}

	if (!NrCreateLabels(Block))
	{
		MLog("Failed to create labels.\n");
		return FALSE;
	}

	return IrCountLinks(Block);
}

PVOID NrAssemble(PNATIVE_BLOCK Block, PUINT AssembledSize)
{
	if (!Block->Front || !Block->Back || !AssembledSize)
		return NULL;

	UINT TotalSize = NrCalcBlockSize(Block);
	if (!TotalSize)
		return NULL;

	PUCHAR Buffer = (PUCHAR)Allocate(TotalSize);
	if (!Buffer)
		return NULL;

	PUCHAR CopyTarget = Buffer;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & (CODE_FLAG_IS_INST | CODE_FLAG_IS_RAW_DATA))
		{
			RtlCopyMemory(CopyTarget, T->RawInstData, T->RawInstSize);

			for (PASSEMBLY_OPERATION AsmOp = T->AssemblyOperations; AsmOp; AsmOp = AsmOp->Next)
			{
				if (!AsmOp->Operation(T, CopyTarget, AsmOp->Context))
				{
					Free(Buffer);
					return NULL;
				}
			}

			CopyTarget += T->RawInstSize;
		}
	}
	*AssembledSize = TotalSize;

	return NULL;
}

VOID NrDebugPrintIClass(PNATIVE_BLOCK Block)
{
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_IS_LABEL)
		{
			printf("%d:\n", T->LinkData.Id);
		}
		else if (T->LinkData.Flags & CODE_FLAG_IS_INST)
		{
			printf("\t%s", XedIClassEnumToString(XedDecodedInstGetIClass(&T->DecodedInst)));
			if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
				printf(" %d", T->LinkData.Id);
			printf("\n");
		}
		else
		{
			printf("Unknown Link.\n");
		}

	}
}

BOOLEAN NrAreFlagsClobbered(PNATIVE_LINK Start, PNATIVE_LINK End)
{
	if (Start == End)
		return FALSE;

	XED_FLAG_SET Ledger;
	CONST XED_SIMPLE_FLAG* SimpleFlag = XedDecodedInstGetRflagsInfo(&Start->DecodedInst);
	Ledger.flat = (XedSimpleFlagGetWrittenFlagSet(SimpleFlag)->flat | XedSimpleFlagGetUndefinedFlagSet(SimpleFlag)->flat);

	for (PNATIVE_LINK T = Start->Next; T && T != End->Next; T = T->Next)
	{
		if (!(T->LinkData.Flags & CODE_FLAG_IS_INST))
			continue;

		CONST XED_SIMPLE_FLAG* InstFlag = XedDecodedInstGetRflagsInfo(&Start->DecodedInst);

		if (Ledger.flat & XedSimpleFlagGetReadFlagSet(InstFlag)->flat)
			return FALSE;

		Ledger.flat &= ~(XedSimpleFlagGetWrittenFlagSet(InstFlag)->flat | XedSimpleFlagGetUndefinedFlagSet(InstFlag)->flat);
	}
	return TRUE;
}
