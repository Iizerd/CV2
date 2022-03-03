#include "NativeRope.h"
#include "Logging.h"

VOID NrFreeLink(PNATIVE_LINK Link)
{
	if (Link->RawInstData)
		Free(Link->RawInstData);
	for (PASSEMBLY_PREOP PreOp = Link->PreAssemblyOperations; PreOp;)
	{
		PASSEMBLY_PREOP NextOp = PreOp->Next;
		if (PreOp->Context)
			Free(PreOp->Context);
		Free(PreOp);
		PreOp = NextOp;
	}
	for (PASSEMBLY_POSTOP PostOp = Link->PostAssemblyOperations; PostOp;)
	{
		PASSEMBLY_POSTOP NextOp = PostOp->Next;
		if (PostOp->Context)
			Free(PostOp->Context);
		Free(PostOp);
		PostOp = NextOp;
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

PNATIVE_LINK NrTraceToLabel(PNATIVE_LINK Start, PNATIVE_LINK End, ULONG Id)
{
	for (PNATIVE_LINK T = Start; T && T != End->Next; T = T->Next)
	{
		if ((T->LinkData.Flags & CODE_FLAG_IS_LABEL) && T->LinkData.Id == Id)
			return T;
	}
	return NULL;
}

PNATIVE_LINK NrTraceToMarker(PNATIVE_LINK Start, PNATIVE_LINK End, ULONG Id)
{
	for (PNATIVE_LINK T = Start; T && T != End->Next; T = T->Next)
	{
		if ((T->LinkData.Flags & CODE_FLAG_IS_MARKER) && T->LinkData.Id == Id)
			return T;
	}
	return NULL;
}

BOOLEAN NrAddPreAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPreOp Operation, PVOID Context, BOOLEAN Front)
{
	PASSEMBLY_PREOP PreOpStruct = AllocateS(ASSEMBLY_PREOP);
	if (!PreOpStruct)
	{
		MLog("Failed to allocate memory for pre assembly operation structure.\n");
		return FALSE;
	}
	PreOpStruct->Context = Context;
	PreOpStruct->Operation = Operation;

	if (!Link->PreAssemblyOperations)
		Link->PreAssemblyOperations = PreOpStruct;
	else if (Front)
	{
		PreOpStruct->Next = Link->PreAssemblyOperations;
		Link->PreAssemblyOperations = PreOpStruct;
	}
	else
	{
		for (PASSEMBLY_PREOP T = Link->PreAssemblyOperations; T; T = T->Next)
		{
			if (T->Next == NULL)
			{
				T->Next = PreOpStruct;
				break;
			}
		}
	}
	return TRUE;
};

BOOLEAN NrAddPostAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPostOp Operation, PVOID Context, BOOLEAN Front)
{
	PASSEMBLY_POSTOP PostOpStruct = AllocateS(ASSEMBLY_POSTOP);
	if (!PostOpStruct)
	{
		MLog("Failed to allocate memory for post assembly operation structure.\n");
		return FALSE;
	}
	PostOpStruct->Context = Context;
	PostOpStruct->Operation = Operation;
	
	if (!Link->PostAssemblyOperations)
		Link->PostAssemblyOperations = PostOpStruct;
	else if (Front)
	{
		PostOpStruct->Next = Link->PostAssemblyOperations;
		Link->PostAssemblyOperations = PostOpStruct;
	}
	else
	{
		for (PASSEMBLY_POSTOP T = Link->PostAssemblyOperations; T; T = T->Next)
		{
			if (T->Next == NULL)
			{
				T->Next = PostOpStruct;
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

		XedDecodedInstZeroSetMode(&Dest->DecodedInst, &XedGlobalMachineState);
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
		if (T->LinkData.Flags & (CODE_FLAG_IS_INST | CODE_FLAG_IS_RAW_DATA))
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

BOOLEAN NrCalcRelativeJumpDisp(PNATIVE_LINK Link, PINT32 DeltaOut)
{
	INT32 Delta = 0;
	for (PNATIVE_LINK T = Link; T; T = T->Prev)
	{
		if (T->LinkData.Flags & CODE_FLAG_IS_LABEL)
		{
			if (T->LinkData.Id == Link->LinkData.Id)
			{
				*DeltaOut = Delta;
				return TRUE;
			}
			continue;
		}
		Delta -= T->RawInstSize;
	}

	Delta = 0;
	for (PNATIVE_LINK T = Link->Next; T; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_IS_LABEL)
		{
			if (T->LinkData.Id == Link->LinkData.Id)
			{
				*DeltaOut = Delta;
				return TRUE;
			}
			continue;
		}
		Delta += T->RawInstSize;
	}
	return FALSE;
}

BOOLEAN NrPromoteAllRelativeJumpsTo32BitDisplacement(PNATIVE_BLOCK Block)
{
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
		{
			UINT BranchInstSize = 0;
			XED_ICLASS_ENUM IClass = XedDecodedInstGetIClass(&T->DecodedInst);
			XED_ENCODER_INSTRUCTION RawBranchInst;
			XedInst1(&RawBranchInst, XedGlobalMachineState, IClass, 32, XedRelBr(0, 32));
			PUCHAR AssembledBranch = XedEncodeInstructions(&RawBranchInst, 1, &BranchInstSize);
			if (!AssembledBranch || !BranchInstSize)
			{
				MLog("Could not assemble/promote new relative jump. [%s]\n", XedIClassEnumToString(IClass));
				return FALSE;
			}

			XedDecodedInstZeroSetMode(&T->DecodedInst, &XedGlobalMachineState);
			XED_ERROR_ENUM XedError = XedDecode(&T->DecodedInst, AssembledBranch, BranchInstSize);
			if (XedError != XED_ERROR_NONE)
				return FALSE;

			Free(T->RawInstData);
			T->RawInstData = AssembledBranch;
			T->RawInstSize = BranchInstSize;
		}
	}
	return TRUE;
}

BOOLEAN NrFixRelativeJumps(PNATIVE_BLOCK Block)
{
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
		{
			if (!T->RawInstData)
			{
				MLog("Relative jump instruction has no raw data!\n");
				return FALSE;
			}

			INT32 BranchDisp = 0;
			if (!NrCalcRelativeJumpDisp(T, &BranchDisp))
			{
				MLog("Could not calculate relative jump displacement.\n");
				return FALSE;
			}

			//If it takes more bits than available to represent current displacement
			if (log2(abs(BranchDisp)) + 1 > XedDecodedInstGetBranchDisplacementWidthBits(&T->DecodedInst))
			{
				UINT BranchInstSize = 0;
				XED_ICLASS_ENUM IClass = XedDecodedInstGetIClass(&T->DecodedInst);
				XED_ENCODER_INSTRUCTION RawBranchInst;
				XedInst1(&RawBranchInst, XedGlobalMachineState, IClass, 32, XedRelBr(BranchDisp, 32));
				PUCHAR AssembledBranch = XedEncodeInstructions(&RawBranchInst, 1, &BranchInstSize);
				if (!AssembledBranch || !BranchInstSize)
				{
					MLog("Could not assemble new relative jump. [%s][%d]\n", XedIClassEnumToString(IClass), BranchDisp);
					return FALSE;
				}

				XedDecodedInstZeroSetMode(&T->DecodedInst, &XedGlobalMachineState);
				XED_ERROR_ENUM XedError = XedDecode(&T->DecodedInst, AssembledBranch, BranchInstSize);
				if (XedError != XED_ERROR_NONE) //If xed can't decode something it just encoded.
					return FALSE;

				Free(T->RawInstData);
				T->RawInstData = AssembledBranch;
				T->RawInstSize = BranchInstSize;

				T = Block->Front;
				continue;
			}
			else
			{
				//A bit hacky ya? Displacement is always going to be the last bits of the jump.
				UINT BranchDispWidth = XedDecodedInstGetBranchDisplacementWidth(&T->DecodedInst);
				switch (BranchDispWidth)
				{
				case 1: *(PINT8)&(((PUCHAR)T->RawInstData)[T->RawInstSize - BranchDispWidth]) = (INT8)BranchDisp; break;
				case 2: *(PINT16)&(((PUCHAR)T->RawInstData)[T->RawInstSize - BranchDispWidth]) = (INT16)BranchDisp; break;
				case 4: *(PINT32)&(((PUCHAR)T->RawInstData)[T->RawInstSize - BranchDispWidth]) = (INT32)BranchDisp; break;
				}
			}
		}


		T = T->Next;
	}
	return TRUE;
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

		XedDecodedInstZeroSetMode(&Link->DecodedInst, &XedGlobalMachineState);
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
	//Do all pre assembly operations(which COULD change the size, thats why this is here)
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		for (PASSEMBLY_PREOP PreOp = T->PreAssemblyOperations; PreOp; PreOp = PreOp->Next)
		{
			if (!PreOp->Operation(T, PreOp->Context))
			{
				MLog("Pre assembly operation failed.\n");
				return NULL;
			}
		}
	}

	//Fix up all jump deltas now that everything is where its going to be for final assembly.
	if (!NrFixRelativeJumps(Block))
	{
		MLog("Failed to fix all relative jumps before assembling.\n");
		return NULL;
	}

	UINT TotalSize = NrCalcBlockSize(Block);
	if (!Block->Front || !Block->Back || !AssembledSize || !TotalSize)
	{
		MLog("Invalid block to assemble.\n");
		return NULL;
	}

	PUCHAR Buffer = (PUCHAR)Allocate(TotalSize);
	if (!Buffer)
	{
		MLog("Could not allocate assembly buffer in NrAssemble. Size:[%u]\n", TotalSize);
		return NULL;
	}

	PUCHAR CopyTarget = Buffer;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & (CODE_FLAG_IS_INST | CODE_FLAG_IS_RAW_DATA))
		{

			RtlCopyMemory(CopyTarget, T->RawInstData, T->RawInstSize);

			for (PASSEMBLY_POSTOP PostOp = T->PostAssemblyOperations; PostOp; PostOp = PostOp->Next)
			{
				if (!PostOp->Operation(T, CopyTarget, PostOp->Context))
				{
					MLog("Post assembly operation failed.\n");
					Free(Buffer);
					return NULL;
				}
			}

			CopyTarget += T->RawInstSize;
		}
	}
	*AssembledSize = TotalSize;
	return Buffer;
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

