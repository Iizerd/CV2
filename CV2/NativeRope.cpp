#include "NativeRope.h"
#include "Logging.h"


VOID NrFreeLink(PNATIVE_LINK Link)
{
	if (Link->RawData)
		Free(Link->RawData);
	for (PASSEMBLY_PREOP PreOp = Link->PreAssemblyOperations; PreOp;)
	{
		PASSEMBLY_PREOP NextOp = PreOp->Next;
		if (PreOp->Flags & ASMOP_FLAG_FREE_CONTEXT && PreOp->Context)
			Free(PreOp->Context);
		Free(PreOp);
		PreOp = NextOp;
	}
	for (PASSEMBLY_POSTOP PostOp = Link->PostAssemblyOperations; PostOp;)
	{
		PASSEMBLY_POSTOP NextOp = PostOp->Next;
		if (PostOp->Flags & ASMOP_FLAG_FREE_CONTEXT && PostOp->Context)
			Free(PostOp->Context);
		Free(PostOp);
		PostOp = NextOp;
	}
	Free(Link);
}

VOID NrFreeBlock(PNATIVE_BLOCK Block)
{
	NrFreeBlock2(Block->Front, Block->Back);
	Block->Front = Block->Back = NULL;
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

VOID NrInitForInst(PNATIVE_LINK Link)
{
	XedDecodedInstZeroSetMode(&Link->DecodedInst, &XedGlobalMachineState);
	Link->LinkData.Flags |= CODE_FLAG_IS_INST;
}

VOID NrInitForLabel(PNATIVE_LINK Link, UINT32 Id, PNATIVE_LINK Next, PNATIVE_LINK Prev)
{
	Link->LinkData.Flags |= CODE_FLAG_IS_LABEL;
	Link->LinkData.Id = Id;
	Link->Next = Next;
	Link->Prev = Prev;
}

VOID NrMarkBlockAsGroup(PNATIVE_BLOCK Block)
{
	Block->Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block->Back->LinkData.Flags |= CODE_FLAG_GROUP_END;
	for (PNATIVE_LINK T = Block->Front; T; T = T->Next)
		T->LinkData.Flags |= CODE_FLAG_IN_GROUP;
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

BOOLEAN NrAddPreAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPreOp Operation, PVOID Context, UINT32 Flags, BOOLEAN Front)
{
	PASSEMBLY_PREOP PreOpStruct = AllocateS(ASSEMBLY_PREOP);
	if (!PreOpStruct)
	{
		MLog("Failed to allocate memory for pre assembly operation structure.\n");
		return FALSE;
	}
	PreOpStruct->Context = Context;
	PreOpStruct->Operation = Operation;
	PreOpStruct->Flags = Flags;

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

BOOLEAN NrAddPostAssemblyOperation(PNATIVE_LINK Link, FnAssemblyPostOp Operation, PVOID Context, UINT32 Flags, BOOLEAN Front)
{
	PASSEMBLY_POSTOP PostOpStruct = AllocateS(ASSEMBLY_POSTOP);
	if (!PostOpStruct)
	{
		MLog("Failed to allocate memory for post assembly operation structure.\n");
		return FALSE;
	}
	PostOpStruct->Context = Context;
	PostOpStruct->Operation = Operation;
	PostOpStruct->Flags = Flags;
	
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
		Dest->RawDataSize = Source->RawDataSize;
		Dest->RawData = Allocate(Source->RawDataSize);
		if (!Dest->RawData)
			return FALSE;
		RtlCopyMemory(Dest->RawData, Source->RawData, Source->RawDataSize);

		XedDecodedInstZeroSetMode(&Dest->DecodedInst, &XedGlobalMachineState);
		XED_ERROR_ENUM XedError = XedDecode(&Dest->DecodedInst, (CONST PUCHAR)Dest->RawData, Dest->RawDataSize);
		if (XedError != XED_ERROR_NONE)
		{
			MLog("Failed to decode in NrDeepCopy. Error: %s\n", XedErrorEnumToString(XedError));
			Free(Dest->RawData);
			return FALSE;
		}
	}
	else if (Source->LinkData.Flags & CODE_FLAG_IS_RAW_DATA)
	{
		*(PVOID*)&Dest->LinkData = *(PVOID*)&Source->LinkData;
		Dest->RawDataSize = Source->RawDataSize;
		Dest->RawData = Allocate(Source->RawDataSize);
		if (!Dest->RawData)
			return FALSE;
		RtlCopyMemory(Dest->RawData, Source->RawData, Source->RawDataSize);
	}
	else if (Source->LinkData.Flags & (CODE_FLAG_IS_LABEL))
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

UINT32 NrCalcBlockSize(PNATIVE_BLOCK Block)
{
	UINT32 Total = 0;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE)
			Total += T->RawDataSize;
	}
	return Total;
}

PNATIVE_LINK NrValidateDelta(PNATIVE_LINK Start, INT32 Delta, PINT32 LeftOver)
{
	PNATIVE_LINK T;
	if (Delta > 0)
	{
		T = Start->Next;
		while (Delta > 0 && T)
		{
			if (T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE)
				Delta -= T->RawDataSize;
			T = T->Next;
		}
		if (!T) return NULL;
		while (T && !(T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE))
			T = T->Next;
		*LeftOver = Delta;
		return T;
	}
	else if (Delta < 0)
	{
		T = Start;
		while (T)
		{
			if (T->LinkData.Flags & (CODE_FLAG_OCCUPIES_SPACE))
			{
				Delta += T->RawDataSize;
				if (Delta >= 0)
					break;
			}
			T = T->Prev;
		}
		if (!T) return NULL;
		while (T && !(T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE))
			T = T->Next;
		*LeftOver = Delta;
		return T;
	}
	*LeftOver = 0;
	return Start->Next;
}

BOOLEAN NrCalcRipDelta(PNATIVE_LINK Link, PINT32 DeltaOut)
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
		Delta -= T->RawDataSize;
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
		Delta += T->RawDataSize;
	}
	return FALSE;
}

BOOLEAN NrPromoteAllRelativeJumpsTo32BitDisplacement(PNATIVE_BLOCK Block)
{
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
		{
			UINT32 BranchInstSize = 0;
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

			Free(T->RawData);
			T->RawData = AssembledBranch;
			T->RawDataSize = BranchInstSize;
		}
	}
	return TRUE;
}

PREOP_STATUS NrRelativeJumpPreOp(PNATIVE_LINK Link, PVOID Context)
{
	//MLog("Relative Jump PreOp: %s %d\n", XedIClassEnumToString(XedDecodedInstGetIClass(&Link->DecodedInst)), (INT64)Context);
	//Context is the left over amount calculated from NrValidateDelta, so it must be added to BranchDisp
	if (!Link->RawData)
	{
		MLog("Relative jump instruction has no raw data!\n");
		return PREOP_CRITICAL_ERROR;
	}

	INT32 BranchDisp = 0;
	if (!NrCalcRipDelta(Link, &BranchDisp))
	{
		MLog("Could not calculate relative jump displacement.\n");
		return PREOP_CRITICAL_ERROR;
	}

	BranchDisp += (INT64)Context;

	//If it takes more bits than available to represent current displacement
	if (XedSignedDispNeededWidth(BranchDisp) > XedDecodedInstGetBranchDisplacementWidthBits(&Link->DecodedInst))
	{
		UINT32 BranchInstSize = 0;
		XED_ICLASS_ENUM IClass = XedDecodedInstGetIClass(&Link->DecodedInst);
		XED_ENCODER_INSTRUCTION RawBranchInst;
		XedInst1(&RawBranchInst, XedGlobalMachineState, IClass, 32, XedRelBr(BranchDisp, 32));
		PUCHAR AssembledBranch = XedEncodeInstructions(&RawBranchInst, 1, &BranchInstSize);
		if (!AssembledBranch || !BranchInstSize)
		{
			MLog("Could not assemble new relative jump. [%s][%d]\n", XedIClassEnumToString(IClass), BranchDisp);
			return PREOP_CRITICAL_ERROR;
		}

		XedDecodedInstZeroSetMode(&Link->DecodedInst, &XedGlobalMachineState);
		XED_ERROR_ENUM XedError = XedDecode(&Link->DecodedInst, AssembledBranch, BranchInstSize);
		if (XedError != XED_ERROR_NONE) //If xed can't decode something it just encoded.
			return PREOP_CRITICAL_ERROR;

		Free(Link->RawData);
		Link->RawData = AssembledBranch;
		Link->RawDataSize = BranchInstSize;
		return PREOP_RESTART;
	}

	if (!XedPatchRelbr(&Link->DecodedInst, (PUCHAR)Link->RawData, XedRelBr(BranchDisp, XedDecodedInstGetBranchDisplacementWidthBits(&Link->DecodedInst))))
	{
		MLog("failed to patch relative branch disp.\n");
		return PREOP_CRITICAL_ERROR;
	}

	////A bit hacky ya? Displacement is always going to be the last bits of the jump.
	//UINT32 BranchDispWidth = XedDecodedInstGetBranchDisplacementWidth(&Link->DecodedInst);
	//switch (BranchDispWidth)
	//{
	//case 1: *(PINT8) & (((PUCHAR)Link->RawData)[Link->RawDataSize - BranchDispWidth]) = (INT8)BranchDisp; break;
	//case 2: *(PINT16) & (((PUCHAR)Link->RawData)[Link->RawDataSize - BranchDispWidth]) = (INT16)BranchDisp; break;
	//case 4: *(PINT32) & (((PUCHAR)Link->RawData)[Link->RawDataSize - BranchDispWidth]) = (INT32)BranchDisp; break;
	//}

	return PREOP_SUCCESS;
}

PREOP_STATUS NrRipRelativePreOp(PNATIVE_LINK Link, PVOID Context)
{
	//Not implemented
	return PREOP_CRITICAL_ERROR;
}

BOOLEAN NrIsRelativeJump(PNATIVE_LINK Link)
{
	UINT32 OperandCount = XedDecodedInstNumOperands(&Link->DecodedInst);
	if (OperandCount < 1)
		return FALSE;

	XED_CATEGORY_ENUM Category = XedDecodedInstGetCategory(&Link->DecodedInst);
	if (Category != XED_CATEGORY_COND_BR && Category != XED_CATEGORY_UNCOND_BR)
		return FALSE;

	CONST XED_INST* Inst = XedDecodedInstInst(&Link->DecodedInst);
	CONST XED_OPERAND* Operand = XedInstOperand(Inst, 0);
	if (!Operand)
		return FALSE;

	XED_OPERAND_TYPE_ENUM OperandType = XedOperandType(Operand);
	if (OperandType != XED_OPERAND_TYPE_IMM && OperandType != XED_OPERAND_TYPE_IMM_CONST)
		return FALSE;

	return TRUE;
}

BOOLEAN NrIsRipRelativeInstruction(PNATIVE_LINK Link, PINT32 Delta)
{
	UINT32 OperandCount = XedDecodedInstNumOperands(&Link->DecodedInst);
	if (OperandCount == 0)
		return FALSE;

	CONST XED_INST* Inst = XedDecodedInstInst(&Link->DecodedInst);
	for (UINT32 i = 0; i < OperandCount; i++)
	{
		XED_OPERAND_ENUM OperandName = XedOperandName(XedInstOperand(Inst, i));
		if (OperandName != XED_OPERAND_MEM0 && OperandName != XED_OPERAND_AGEN)
			continue;

		if (XED_REG_RIP == XedDecodedInstGetBaseReg(&Link->DecodedInst, 0))
		{
			*Delta = XedDecodedInstGetMemoryDisplacement(&Link->DecodedInst, 0);
			return TRUE;
		}
	}
	return FALSE;
}

BOOLEAN NrHandleDisplacementInstructions(PNATIVE_BLOCK Block)
{
	INT32 CurrentId = 0;

	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (!(T->LinkData.Flags & CODE_FLAG_IS_INST))
			continue;

		if (NrIsRelativeJump(T))
		{
			INT32 BranchDisplacement = XedDecodedInstGetBranchDisplacement(&T->DecodedInst);
			INT32 LeftOver = 0;
			PNATIVE_LINK TargetLink = NrValidateDelta(T, BranchDisplacement, &LeftOver);
			if (!TargetLink)
			{
				MLog("Failed to validate delta for relative jump [%s][%d]\n", XedCategoryEnumToString(XedDecodedInstGetCategory(&T->DecodedInst)), BranchDisplacement);
				return FALSE;
			}

			if (TargetLink->Prev && (TargetLink->Prev->LinkData.Flags & CODE_FLAG_IS_LABEL))
				T->LinkData.Id = TargetLink->Prev->LinkData.Id;
			else
			{
				PNATIVE_LINK LabelLink = NrAllocateLink();
				if (!LabelLink)
				{
					MLog("Failed to allocate label link.\n");
					return FALSE;
				}
				NrInitForLabel(LabelLink, CurrentId, NULL, NULL);
				LabelLink->LinkData.Flags |= CODE_FLAG_IS_JUMP_TARGET;
				IrInsertLinkBefore(Block, TargetLink, LabelLink);
				T->LinkData.Id = CurrentId;
				++CurrentId;
			}

			NrAddPreAssemblyOperation(T, NrRelativeJumpPreOp, (PVOID)(INT64)LeftOver, 0UL, FALSE);
			T->LinkData.Flags |= (CODE_FLAG_IS_REL_JUMP | CODE_FLAG_USES_LABEL);
		}
		else if (INT32 Delta = 0; NrIsRipRelativeInstruction(T, &Delta))
		{
			INT32 LeftOver = 0;
			PNATIVE_LINK TargetLink = NrValidateDelta(T, Delta, &LeftOver);
			if (!TargetLink)
			{
				MLog("Failed to validate delta for rip relative instruction.\n [%d]", Delta);
				return FALSE;
			}

			if (TargetLink->Prev && (TargetLink->Prev->LinkData.Flags & CODE_FLAG_IS_LABEL))
				T->LinkData.Id = TargetLink->Prev->LinkData.Id;
			else
			{
				PNATIVE_LINK LabelLink = NrAllocateLink();
				if (!LabelLink)
				{
					MLog("Failed to allocate label link.\n");
					return FALSE;
				}
				NrInitForLabel(LabelLink, CurrentId, NULL, NULL);
				IrInsertLinkBefore(Block, TargetLink, LabelLink);
				T->LinkData.Id = CurrentId;
				++CurrentId;
			}

			NrAddPreAssemblyOperation(T, NrRipRelativePreOp, (PVOID)(INT64)LeftOver, 0UL, FALSE);
			T->LinkData.Flags |= (CODE_FLAG_IS_RIP_RELATIVE | CODE_FLAG_USES_LABEL);
		}
	}
}

PFUNCTION_BLOCK NrCreateFunctionBlockTree(PNATIVE_BLOCK CodeBlock)
{
	STDVECTOR<PFUNCTION_BLOCK> FunctionBlocks = { };

	if (!CodeBlock->Front || !CodeBlock->Back)
		return NULL;

	PFUNCTION_BLOCK CurrentBlock = AllocateS(FUNCTION_BLOCK);
	if (!CurrentBlock)
	{
		MLog("Could not allocate memory for first function block.\n");
		return NULL;
	}
	CurrentBlock->Block.Front = CodeBlock->Front;

	for (PNATIVE_LINK T = CodeBlock->Front; T && T != CodeBlock->Back->Next; T = T->Next)
	{
		if ((T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP) ||
			((T->LinkData.Flags & CODE_FLAG_IS_LABEL) && (T->LinkData.Flags & CODE_FLAG_IS_JUMP_TARGET)))
		{
			PFUNCTION_BLOCK NextBlock = AllocateS(FUNCTION_BLOCK);
			if (!NextBlock)
			{
				MLog("Could not allocate memory for next function block.\n");
				for (PFUNCTION_BLOCK Block : FunctionBlocks)
					Free(Block);
				Free(CurrentBlock);
				return NULL;
			}

			if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
			{
				NextBlock->Block.Front = T->Next;
				CurrentBlock->Block.Back = T;
				CurrentBlock->IsConditional = (ULONG64)(!(XED_ICLASS_JMP == XedDecodedInstGetIClass(&T->DecodedInst)));
			}
			else
			{
				NextBlock->Block.Front = T;
				CurrentBlock->Block.Back = T->Prev;
				CurrentBlock->IsConditional = FALSE;
			}

			CurrentBlock->Absolute.NextBlock = NextBlock;
			FunctionBlocks.push_back(CurrentBlock);
			CurrentBlock = NextBlock;
		}
	}

	CurrentBlock->Block.Back = CodeBlock->Back;
	CurrentBlock->IsConditional = FALSE;
	FunctionBlocks.push_back(CurrentBlock);

	//Only one block so we just return it.
	if (FunctionBlocks.size() == 0)
		return CurrentBlock;

	//Otherwise, fix up conditional jumps by searching for where they jump to!
	for (UINT32 i = 0; i < FunctionBlocks.size(); i++)
	{
		if (FunctionBlocks[i]->IsConditional)
		{
			INT32 TargetLabel = FunctionBlocks[i]->Block.Back->LinkData.Id;
			for (INT j = i + 1; j < FunctionBlocks.size(); j++)
			{
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) &&
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.Id)
				{
					FunctionBlocks[i]->Conditional.Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}

			}
			for (INT j = i - 1; j >= 0; j--)
			{
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) &&
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.Id)
				{
					FunctionBlocks[i]->Conditional.Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}
			}

			for (PFUNCTION_BLOCK Block : FunctionBlocks)
				Free(Block);
			MLog("Failed to find Taken branch of relative jump.\n");
			return NULL;
		}
	ContinueToNextBlock:
		continue;
	}

	return FunctionBlocks[0];
}

VOID NrFreeFunctionBlockTree(PFUNCTION_BLOCK TreeHead)
{
	if (TreeHead)
	{
		NrFreeTree(TreeHead->Absolute.NextBlock);
		Free(TreeHead);
	}
}

PFUNCTION_BLOCK NrDecodeToEndOfFunctionBlock(PVOID CodeStart)
{
	//Decode until we reach a relative jump. then create a block, and decode to that!
	PFUNCTION_BLOCK Block = AllocateS(FUNCTION_BLOCK);
	if (!Block)
	{
		MLog("Could not allocate function block to decode to.\n");
		return NULL;
	}

	//Do the decoding


	/*
	* if jump is conditional
	*	Block->Conditional.Taken = NrDecodeToEndOfFunctionBlock(CalcTakenDelta)
	*	Block->Conditional.NotTaken = NrDecodeToEndOfFunctionBlock(NextInstruction)
	* else
	*   Block->Absolute.NextBlock = NrDecodeToEndOfFunctionBlock(NextInstruction);
	*/

	return Block;
}

BOOLEAN NrDecodeImperfect(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength)
{
	return NrDecodeImperfectEx(Block, RawCode, CodeLength, 0UL);
}

BOOLEAN NrDecodeImperfectEx(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength, UINT32 Flags)
{
	if (!RawCode || !CodeLength)
		return FALSE;

	
}

BOOLEAN NrDecodePerfect(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength)
{
	return NrDecodePerfectEx(Block, RawCode, CodeLength, 0UL);
}

BOOLEAN NrDecodePerfectEx(PNATIVE_BLOCK Block, PVOID RawCode, UINT32 CodeLength, UINT32 Flags)
{
	if (!RawCode || !CodeLength)
		return FALSE;

	PUCHAR CodePointer = (PUCHAR)RawCode;
	PUCHAR CodeEnd = CodePointer + CodeLength;
	while (CodePointer < CodeEnd)
	{
		PNATIVE_LINK Link = NrAllocateLink();
		if (!Link)
		{
			NrFreeBlock(Block);
			MLog("Could not allocate new link in NrDecodePerfect\n");
			return FALSE;
		}
		NrInitForInst(Link);
		UINT32 PossibleSize = MinVal(15, CodeEnd - CodePointer);

		XedDecodedInstZeroSetMode(&Link->DecodedInst, &XedGlobalMachineState);
		XED_ERROR_ENUM XedError = XedDecode(&Link->DecodedInst, CodePointer, PossibleSize);
		if (XedError != XED_ERROR_NONE)
		{
			MLog("XedDecode failed in NrDecodePerfect. Error: %s\n", XedErrorEnumToString(XedError));
			NrFreeLink(Link);
			NrFreeBlock(Block);
			return FALSE;
		}

		UINT32 RawDataSize = XedDecodedInstGetLength(&Link->DecodedInst);
		PVOID RawData = Allocate(RawDataSize);
		if (!RawData)
		{
			MLog("Could not allocate space for RawData in NrDissassemble\n");
			NrFreeLink(Link);
			NrFreeBlock(Block);
			return FALSE;
		}
		RtlCopyMemory(RawData, CodePointer, RawDataSize);
		Link->RawData = RawData;
		Link->RawDataSize = RawDataSize;

		IrPutLinkBack(Block, Link);
		CodePointer += RawDataSize;
	}

	if (!(Flags & DECODER_FLAG_DONT_GENERATE_OPERATIONS) && !NrHandleDisplacementInstructions(Block))
	{
		MLog("Failed to create labels.\n");
		return FALSE;
	}

	return IrCountLinks(Block);
}

PVOID NrEncode(PNATIVE_BLOCK Block, PUINT32 AssembledSize)
{
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		for (PASSEMBLY_PREOP PreOp = T->PreAssemblyOperations; PreOp; PreOp = PreOp->Next)
		{
			PREOP_STATUS Status = PreOp->Operation(T, PreOp->Context);
			switch (Status)
			{
			case PREOP_SUCCESS:
				break;
			case PREOP_RESTART:
				T = Block->Front;
				goto NextLinkLabel;
			default:
				__fallthrough;
			case PREOP_CRITICAL_ERROR:
				MLog("Critical error in assembly pre-operation: %u\n", Status);
				return NULL;
			}
		}
		T = T->Next;
	NextLinkLabel:
		continue;
	}

	UINT32 TotalSize = NrCalcBlockSize(Block);
	if (!Block->Front || !Block->Back || !AssembledSize || !TotalSize)
	{
		MLog("Invalid block to assemble.\n");
		return NULL;
	}

	PUCHAR Buffer = (PUCHAR)Allocate(TotalSize);
	if (!Buffer)
	{
		MLog("Could not allocate assembly buffer in NrEncode. Size:[%u]\n", TotalSize);
		return NULL;
	}

	PUCHAR CopyTarget = Buffer;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE)
		{
			RtlCopyMemory(CopyTarget, T->RawData, T->RawDataSize);

			for (PASSEMBLY_POSTOP PostOp = T->PostAssemblyOperations; PostOp; PostOp = PostOp->Next)
			{
				POSTOP_STATUS Status = PostOp->Operation(T, CopyTarget, PostOp->Context);
				switch (Status)
				{
				case POSTOP_SUCCESS:
					break;
				default:
					__fallthrough;
				case POSTOP_CRITICAL_ERROR:
					MLog("Critical error in assembly post-operation: %u\n", Status);
					Free(Buffer);
					return NULL;
				}
			}

			CopyTarget += T->RawDataSize;
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

VOID NrPrintTakenPath(PFUNCTION_BLOCK TreeHead)
{
	while (TreeHead)
	{
		NrDebugPrintIClass(&TreeHead->Block);
		if (TreeHead->IsConditional)
			TreeHead = TreeHead->Conditional.Taken;
		else
			TreeHead = TreeHead->Absolute.NextBlock;
		printf("\n");
	}
}

VOID NrPrintNotTakenPath(PFUNCTION_BLOCK TreeHead)
{
	while (TreeHead)
	{
		NrDebugPrintIClass(&TreeHead->Block);
		TreeHead = TreeHead->Absolute.NextBlock;
		printf("\n");
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

//BOOLEAN NrFixRelativeJumps(PNATIVE_BLOCK Block)
//{
//	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
//	{
//		if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
//		{
//			if (!T->RawData)
//			{
//				MLog("Relative jump instruction has no raw data!\n");
//				return FALSE;
//			}
//
//			INT32 BranchDisp = 0;
//			if (!NrCalcRipDelta(T, &BranchDisp))
//			{
//				MLog("Could not calculate relative jump displacement.\n");
//				return FALSE;
//			}
//
//			//If it takes more bits than available to represent current displacement
//			if (log2(abs(BranchDisp)) + 1 > XedDecodedInstGetBranchDisplacementWidthBits(&T->DecodedInst))
//			{
//				MLog("Cannot currently handle needing to resize relative jump displacement widths.\n");
//				return FALSE;
//				//UINT32 BranchInstSize = 0;
//				//XED_ICLASS_ENUM IClass = XedDecodedInstGetIClass(&T->DecodedInst);
//				//XED_ENCODER_INSTRUCTION RawBranchInst;
//				//XedInst1(&RawBranchInst, XedGlobalMachineState, IClass, 32, XedRelBr(BranchDisp, 32));
//				//PUCHAR AssembledBranch = XedEncodeInstructions(&RawBranchInst, 1, &BranchInstSize);
//				//if (!AssembledBranch || !BranchInstSize)
//				//{
//				//	MLog("Could not assemble new relative jump. [%s][%d]\n", XedIClassEnumToString(IClass), BranchDisp);
//				//	return FALSE;
//				//}
//
//				//XedDecodedInstZeroSetMode(&T->DecodedInst, &XedGlobalMachineState);
//				//XED_ERROR_ENUM XedError = XedDecode(&T->DecodedInst, AssembledBranch, BranchInstSize);
//				//if (XedError != XED_ERROR_NONE) //If xed can't decode something it just encoded.
//				//	return FALSE;
//
//				//Free(T->RawData);
//				//T->RawData = AssembledBranch;
//				//T->RawDataSize = BranchInstSize;
//
//				//T = Block->Front;
//				//continue;
//			}
//			else
//			{
//				//A bit hacky ya? Displacement is always going to be the last bits of the jump.
//				UINT32 BranchDispWidth = XedDecodedInstGetBranchDisplacementWidth(&T->DecodedInst);
//				switch (BranchDispWidth)
//				{
//				case 1: *(PINT8)&(((PUCHAR)T->RawData)[T->RawDataSize - BranchDispWidth]) = (INT8)BranchDisp; break;
//				case 2: *(PINT16)&(((PUCHAR)T->RawData)[T->RawDataSize - BranchDispWidth]) = (INT16)BranchDisp; break;
//				case 4: *(PINT32)&(((PUCHAR)T->RawData)[T->RawDataSize - BranchDispWidth]) = (INT32)BranchDisp; break;
//				}
//			}
//		}
//
//
//		T = T->Next;
//	}
//	return TRUE;
//}

//BOOLEAN NrHandleRipRelativeInstructions(PNATIVE_BLOCK Block)
//{
//	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
//	{
//		if (T->LinkData.Flags & CODE_FLAG_IS_INST)
//		{
//			INT32 Delta = 0;
//			if (!NrIsRipRelativeInstruction(T, &Delta))
//				continue;
//			INT32 LeftOver = 0;
//			PNATIVE_LINK TargetLink = NrValidateDelta(T, Delta, &LeftOver);
//			if (!TargetLink)
//			{
//				MLog("Failed to validate delta for rip relative instruction.\n [%d]", Delta);
//				return FALSE;
//			}
//
//			if (TargetLink->Prev && (TargetLink->Prev->LinkData.Flags & CODE_FLAG_IS_LABEL))
//				T->LinkData.Id = TargetLink->Prev->LinkData.Id;
//			else
//			{
//				PNATIVE_LINK LabelLink = NrAllocateLink();
//				NrInitForLabel(LabelLink, CurrentLabelId, NULL, NULL);
//				IrInsertLinkBefore(Block, TargetLink, LabelLink);
//				T->LinkData.Id = CurrentLabelId;
//				++CurrentLabelId;
//			}
//
//			NrAddPreAssemblyOperation(T, NrRipRelativePreOp, (PVOID)(INT64)LeftOver, FALSE);
//		}
//	}
//	return TRUE;
//}

//BOOLEAN NrHandleRelativeJumps(PNATIVE_BLOCK Block)
//{
//	INT32 CurrentLabelId = 0;
//	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
//	{
//		if (!(T->LinkData.Flags & CODE_FLAG_IS_INST))
//			continue;
//
//		if (!NrIsRelativeJump(T))
//			continue;
//
//		INT32 BranchDisplacement = XedDecodedInstGetBranchDisplacement(&T->DecodedInst);
//		INT32 LeftOver = 0;
//		PNATIVE_LINK TargetLink = NrValidateDelta(T, BranchDisplacement, &LeftOver);
//		if (!TargetLink)
//		{
//			MLog("Failed to validate jump. [%s][%d]\n", XedCategoryEnumToString(XedDecodedInstGetCategory(&T->DecodedInst)), BranchDisplacement);
//			return FALSE;
//		}
//
//		if (TargetLink->Prev && (TargetLink->Prev->LinkData.Flags & CODE_FLAG_IS_LABEL))
//			T->LinkData.Id = TargetLink->Prev->LinkData.Id;
//		else
//		{
//			PNATIVE_LINK LabelLink = NrAllocateLink();
//			NrInitForLabel(LabelLink, CurrentLabelId, NULL, NULL);
//			IrInsertLinkBefore(Block, TargetLink, LabelLink);
//			T->LinkData.Id = CurrentLabelId;
//			++CurrentLabelId;
//		}
//
//		NrAddPreAssemblyOperation(T, NrRelativeJumpPreOp, (PVOID)(INT64)LeftOver, FALSE);
//		T->LinkData.Flags |= (CODE_FLAG_IS_REL_JUMP | CODE_FLAG_USES_LABEL);
//	}
//}

