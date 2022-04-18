
#include "Logging.h"
#include "Emulation.h"
#include "RandomAssembly.h"


BOOLEAN EmPush(PNATIVE_BLOCK Block, XED_REG_ENUM Register)
{
	return FALSE;
}

BOOLEAN EmPop(PNATIVE_BLOCK Block, XED_REG_ENUM Register)
{
	return FALSE;
}

BOOLEAN EmAddRegImm(PNATIVE_BLOCK Block, XED_REG_ENUM Register, INT32 Imm)
{
	return FALSE;
}

BOOLEAN EmSubRegImm(PNATIVE_BLOCK Block, XED_REG_ENUM Register, INT32 Imm)
{
	return FALSE;
}

BOOLEAN EmMovRegReg(PNATIVE_BLOCK Block, XED_REG_ENUM Register1, XED_REG_ENUM Register2)
{
	return FALSE;
}

PNATIVE_LINK EmBranch(XED_ICLASS_ENUM BranchIClass, INT32 TargetLabelId, UINT32 DispWidthBits)
{
	XED_ENCODER_INSTRUCTION InstList;
	XedInst1(&InstList, XedGlobalMachineState, BranchIClass, 64, XedRelBr(0, DispWidthBits));

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

	Link->LinkData.Flags |= CODE_FLAG_IS_REL_JUMP;
	Link->LinkData.Id = TargetLabelId;
	Link->RawData = EncodedInst;
	Link->RawDataSize = OutSize;
	return Link;
}

PNATIVE_LINK EmUnConditionalBranch(INT32 TargetLabelId, UINT32 DispWidthBits)
{
	return EmBranch(XED_ICLASS_JMP, TargetLabelId, DispWidthBits);
}

BOOLEAN EmRet1(PNATIVE_BLOCK Block, UINT32 JunkSize)
{
	//-	POP [RIP + 6 + Delta]		[GROUP_START]
	//-	JMP [RIP + Delta]
	//-	**JUNK**
	//- [Return Address]			[GROUP_END]

	XED_ENCODER_INSTRUCTION InstList[2];
	XedInst1(&InstList[0], XedGlobalMachineState, XED_ICLASS_POP, 64, XedMemBD(XED_REG_RIP, XedDisp(6 + JunkSize, 32), 64));
	XedInst1(&InstList[1], XedGlobalMachineState, XED_ICLASS_JMP, 64, XedMemBD(XED_REG_RIP, XedDisp(JunkSize, 32), 64));

	UINT32 OutSize = 0;
	PUCHAR EncodedInst = XedEncodeInstructions(InstList, 2, &OutSize);
	if (!EncodedInst || !OutSize)
		return FALSE;

	Block->Front = Block->Back = NULL;
	if (!NrDecodePerfectEx(Block, EncodedInst, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
		return FALSE;

	Free(EncodedInst);

	PNATIVE_LINK JunkLink = NrAllocateLink();
	if (!JunkLink)
	{
		NrFreeBlock(Block);
		return FALSE;
	}

	JunkLink->RawDataSize = JunkSize + 8;
	JunkLink->RawData = Allocate(JunkSize + 8);
	JunkLink->LinkData.Flags |= CODE_FLAG_IS_RAW_DATA;
	if (!JunkLink->RawData)
	{
		NrFreeLink(JunkLink);
		NrFreeBlock(Block);
		return FALSE;
	}
	NrAddPostAssemblyOperation(JunkLink, RaRandomizeInstAfterAssembly, NULL, 0UL, FALSE);

	IrPutLinkBack(Block, JunkLink);

	NrMarkBlockAsGroup(Block);
	return TRUE;
}

BOOLEAN EmRet2(PNATIVE_BLOCK Block, UINT32 JunkSize, UINT32 DeadstoreMethod)
{
	/*
	*  - JMP Delta			[GROUP_START]
	*  - **JUNK**
	*  - PUSH 0xC3			[GROUP_START]
	*/

	//Generate the deadstore instruction(s) and save the delta in a variable.
	INT32 JmpDisp = 0;
	{
		UINT32 ImmValue = 0;
		UINT32 ImmWidth = 3;
		while (ImmWidth == 3)
			ImmWidth = RndGetRandomNum<INT32>(1, 4);	//1, 2, 4

		UINT32 ImmWidthBits = ImmWidth * 8;				//8, 16, 32
		INT32 OffsetInsideImm = RndGetRandomNum<INT32>(0, ImmWidth - 1);
		for (INT32 i = 0; i < ImmWidth; i++)
		{
			if (i == OffsetInsideImm)
				ImmValue |= ((UINT32)(0xC3) << (i * 8));
			else
				ImmValue |= ((UINT32)(RndGetRandomNum<INT32>(0, 255)) << (i * 8));
		}

		UINT32 OutSize = 0;
		PUCHAR EncodedInst = NULL;
		XED_ENCODER_INSTRUCTION DeadstoreInst;

		if (DeadstoreMethod >= DEADSTORE_METHOD_RANDOM)
			DeadstoreMethod = RndGetRandomNum<UINT>(0, 2);
		switch (DeadstoreMethod)
		{
		case DEADSTORE_METHOD_PUSH:
			XedInst1(&DeadstoreInst, XedGlobalMachineState, XED_ICLASS_PUSH, ((ImmWidth == 2) ? ImmWidthBits : 64), XedImm0(ImmValue, ImmWidthBits));
			break;
		case DEADSTORE_METHOD_MOV:
			XedInst2(&DeadstoreInst, XedGlobalMachineState, XED_ICLASS_MOV, ImmWidthBits, XedReg(RaGetRandomRegister(ImmWidth)), XedImm0(ImmValue, ImmWidthBits));
			break;
		default:
			__fallthrough;
		case DEADSTORE_METHOD_BITWISE:
			XedInst2(&DeadstoreInst, XedGlobalMachineState, RaGetRandomBitwiseIClass(), ImmWidthBits, XedReg(RaGetRandomRegister(ImmWidth)), XedImm0(ImmValue, ImmWidthBits));
			break;
		}

		EncodedInst = XedEncodeInstructions(&DeadstoreInst, 1, &OutSize);
		if (!EncodedInst || !OutSize)
			return FALSE;

		PNATIVE_LINK Link = NrAllocateLink();
		if (!Link)
		{
			MLog("Failed to allocate link in BmGenerateEmulateRet2.\n");
			NrFreeBlock(Block);
			return FALSE;
		}
		NrInitForInst(Link);

		XED_ERROR_ENUM XedError = XedDecode(&Link->DecodedInst, EncodedInst, OutSize);
		if (XedError != XED_ERROR_NONE) //Cant decode something we just encoded?!?!?
		{
			NrFreeLink(Link);
			NrFreeBlock(Block);
			return FALSE;
		}
		Link->RawData = EncodedInst;
		Link->RawDataSize = OutSize;
		IrPutLinkBack(Block, Link);

		JmpDisp = OutSize - (ImmWidth - OffsetInsideImm);
	}


	if (JunkSize)
	{
		//Generate junk link between.
		PNATIVE_LINK JunkLink = NrAllocateLink();
		if (!JunkLink)
		{
			NrFreeBlock(Block);
			return FALSE;
		}
		JunkLink->RawDataSize = JunkSize;
		JunkLink->RawData = Allocate(JunkSize);
		JunkLink->LinkData.Flags |= CODE_FLAG_IS_RAW_DATA;
		NrAddPostAssemblyOperation(JunkLink, RaRandomizeInstAfterAssembly, NULL, 0UL, FALSE);
		JmpDisp += JunkSize;
		IrPutLinkFront(Block, JunkLink);
	}

	//Generate the relative jump and put it before the deadstore in the block.
	//We do not need to add a preop to recalculate this delta because this group will NOT be edited.
	{
		XED_ENCODER_INSTRUCTION JmpInst;
		UINT32 OutSize = 0;
		XedInst1(&JmpInst, XedGlobalMachineState, XED_ICLASS_JMP, 64, XedMemBD(XED_REG_RIP, XedDisp(JmpDisp, 32), 64));
		PUCHAR EncodedInst = XedEncodeInstructions(&JmpInst, 1, &OutSize);
		if (!EncodedInst || !OutSize)
		{
			NrFreeBlock(Block);
			return FALSE;
		}

		PNATIVE_LINK Link = NrAllocateLink();
		if (!Link)
		{
			MLog("Failed to allocate link in BmGenerateEmulateRet2.\n");
			NrFreeBlock(Block);
			return FALSE;
		}
		NrInitForInst(Link);

		XED_ERROR_ENUM XedError = XedDecode(&Link->DecodedInst, EncodedInst, OutSize);
		if (XedError != XED_ERROR_NONE)
		{
			NrFreeLink(Link);
			NrFreeBlock(Block);
			return FALSE;
		}
		Link->RawData = EncodedInst;
		Link->RawDataSize = OutSize;
		IrPutLinkFront(Block, Link);
	}

	NrMarkBlockAsGroup(Block);
	return TRUE;
}

PREOP_STATUS EmInternalRipDeltaFinder(PNATIVE_LINK Link, PVOID Context)
{
	INT32 BranchDisp = 0;
	if (!NrCalcRipDelta(Link, &BranchDisp))
	{
		MLog("Could not calculate rip delta.\n");
		return PREOP_CRITICAL_ERROR;
	}

	BranchDisp += (INT64)Context; //which is the left over bytes if there are any.

	XED_REG_ENUM Reg = XedDecodedInstGetReg(&Link->DecodedInst, XedOperandName(XedInstOperand(XedDecodedInstInst(&Link->DecodedInst), 0)));

	XED_ENCODER_INSTRUCTION InstList;
	XedInst2(&InstList, XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(Reg), XedMemBD(XED_REG_RIP, XedDisp(BranchDisp, 32), 64));
	UINT32 OutSize = 0;
	PUCHAR AssembledCode = XedEncodeInstructions(&InstList, 1, &OutSize);
	if (!AssembledCode || !OutSize)
	{
		MLog("Failed to assemble lea in abs jump replacement.\n");
		return PREOP_CRITICAL_ERROR;
	}

	Free(Link->RawData);
	Link->RawData = AssembledCode;
	Link->RawDataSize = OutSize;

	return PREOP_SUCCESS;
}

BOOLEAN EmRelativeNonConditionalJumpToAbsolute(PNATIVE_BLOCK Block, INT32 TargetLabelId, INT64 LeftOver)
{
	/*
	* Relative Jump Remover :
	*	- PUSH RAX
	*	- LEA RAX,[RIP+Displacement]
	*	- XCHG RAX,[RSP]
	*	- RET					; Can additionally remove this Ret with the Emulators above.
	*/

	Block->Front = Block->Back = NULL;

	INT64 WDisplacement = (INT64)TargetLabelId;

	XED_ENCODER_INSTRUCTION InstList[4];
	XedInst1(&InstList[0], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(XED_REG_RAX));
	XedInst2(&InstList[1], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(XED_REG_RAX), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	XedInst2(&InstList[2], XedGlobalMachineState, XED_ICLASS_XCHG, 64, XedMemB(XED_REG_RSP, 64), XedReg(XED_REG_RAX));
	XedInst0(&InstList[3], XedGlobalMachineState, XED_ICLASS_RET_NEAR, 64);

	UINT32 OutSize = 0;
	PUCHAR AssembledCode = XedEncodeInstructions(InstList, 4, &OutSize);
	if (!AssembledCode || !OutSize)
		return FALSE;

	if (!NrDecodePerfectEx(Block, AssembledCode, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
	{
		Free(AssembledCode);
		return FALSE;
	}
	Free(AssembledCode);


	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		T->LinkData.Flags |= CODE_FLAG_IN_GROUP;

		//Add preop to the lea instruction to calculate the delta we need.
		if (XED_ICLASS_LEA == XedDecodedInstGetIClass(&T->DecodedInst))
		{
			T->LinkData.Id = TargetLabelId;
			T->LinkData.Flags |= CODE_FLAG_USES_LABEL;
			NrAddPreAssemblyOperation(T, EmInternalRipDeltaFinder, (PVOID)LeftOver, 0UL, FALSE);
		}
	}

	Block->Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block->Back->LinkData.Flags |= CODE_FLAG_GROUP_END;

	return TRUE;
}

BOOLEAN EmRelativeConditionalJumpToAbsolute(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, PLABEL_MANAGER LabelManager, INT64 LeftOver)
{
	/*
	* Relative Conditional Jump emulator.
	*
	* OLD:
	*		BRANCH	--------------------------------------------,
	*		BmConvertRelNonConJumpToAbsolute(NotTakenLabel)	----|-------,
	*	TakenLabel(0):	<---------------------------------------'		|
	*		BmConvertRelNonConJumpToAbsolute(TakenLabel)				|
	*	NotTakenLabel(1):	<-------------------------------------------'
	*
	* CURRENT:
	*		INVERTED_BRANCH		--------------------------------,
	*		BmConvertRelNonConJumpToAbsolute(TakenLabel)		|
	*	Taken(Really NotTaken)(0):	<---------------------------'
	*/

	NATIVE_BLOCK TakenBlock;
	PNATIVE_LINK TakenLabelLink, BranchLink = NULL;
	TakenBlock.Front = TakenBlock.Back = NULL;
	Block->Front = Block->Back = NULL;

	XED_ICLASS_ENUM InvertedJccIClass = XedInvertJcc(XedDecodedInstGetIClass(&Jmp->DecodedInst));
	if (InvertedJccIClass == XED_ICLASS_INVALID)
		return FALSE;

	//Create all parts save for branch.
	if (!EmRelativeNonConditionalJumpToAbsolute(&TakenBlock, TargetLabelId, LeftOver))
		goto Fail1;

	TakenLabelLink = NrAllocateLink();
	if (!TakenLabelLink)
		goto Fail2;

	NrInitForLabel(TakenLabelLink, LmPeekNextId(LabelManager), NULL, NULL);

	//Create branch
	BranchLink = NrAllocateLink();
	if (!BranchLink)
		goto Fail3;

	NrInitForInst(BranchLink);
	{
		XED_ENCODER_INSTRUCTION BranchInst;
		UINT32 OutSize = 0;
		//This SHOULD be a relatively smal jump, and will remain as such since it is inside of a group. So its encoded as 8 bit displacement.
		//If I ever verify that this all works if split up, this might want to be encoded as 32 bits proactively.
		XedInst1(&BranchInst, XedGlobalMachineState, InvertedJccIClass, 8, XedRelBr(0, 8));
		PUCHAR EncodedInst = XedEncodeInstructions(&BranchInst, 1, &OutSize);
		if (!EncodedInst || !OutSize)
			goto Fail4;

		XED_ERROR_ENUM XedError = XedDecode(&BranchLink->DecodedInst, EncodedInst, OutSize);
		if (XedError != XED_ERROR_NONE)
			goto Fail4;

		BranchLink->RawData = EncodedInst;
		BranchLink->RawDataSize = OutSize;
		BranchLink->LinkData.Id = LmNextId(LabelManager);

		//Manually add the preassembly operation to find the jump label.
		NrAddPreAssemblyOperation(BranchLink, NrRelativeJumpPreOp, NULL, 0UL, FALSE);
	}

	//Assemble it all into the block. This is something nice!
	IrPutLinkBack(Block, BranchLink);
	IrPutBlockBack(Block, &TakenBlock);
	IrPutLinkBack(Block, TakenLabelLink);

	Block->Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block->Back->LinkData.Flags |= CODE_FLAG_GROUP_END;
	return TRUE;


Fail4:
	NrFreeLink(BranchLink);
Fail3:
	NrFreeLink(TakenLabelLink);
Fail2:
	NrFreeBlock(&TakenBlock);
Fail1:
	return FALSE;
}

BOOLEAN EmRelativeConditionalJumpToAbsolute2(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, PLABEL_MANAGER LabelManager, INT64 LeftOver)
{
	/*
	* Same as above but uses CMOVcc to decide which branch to take.
	*
	*	- PUSH RAX
	*	- PUSH RBX
	*	- LEA RBX,[RIP+TakenDisp]
	*	- LEA RAX,[RIP+NotTakenDisp]
	*	- CMOVcc RAX,RBX
	*	- POP RBX
	*	- XCHG RAX,[RSP]
	*	- RET
	*/

	Block->Front = Block->Back = NULL;

	XED_ICLASS_ENUM CMOVcc = XedJccToCMOVcc(XedDecodedInstGetIClass(&Jmp->DecodedInst));
	if (CMOVcc == XED_ICLASS_INVALID)
		return FALSE;

	XED_ENCODER_INSTRUCTION InstList[8];

	XedInst1(&InstList[0], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(XED_REG_RAX));
	XedInst1(&InstList[1], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(XED_REG_RBX));
	XedInst2(&InstList[2], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(XED_REG_RBX), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	XedInst2(&InstList[3], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(XED_REG_RAX), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	XedInst2(&InstList[4], XedGlobalMachineState, CMOVcc, 64, XedReg(XED_REG_RAX), XedReg(XED_REG_RBX));
	XedInst1(&InstList[5], XedGlobalMachineState, XED_ICLASS_POP, 64, XedReg(XED_REG_RBX));
	XedInst2(&InstList[6], XedGlobalMachineState, XED_ICLASS_XCHG, 64, XedMemB(XED_REG_RSP, 64), XedReg(XED_REG_RAX));
	XedInst0(&InstList[7], XedGlobalMachineState, XED_ICLASS_RET_NEAR, 64);

	UINT32 OutSize = 0;
	PUCHAR AssembledCode = XedEncodeInstructions(InstList, 8, &OutSize);
	if (!AssembledCode || !OutSize)
		return FALSE;

	if (!NrDecodePerfectEx(Block, AssembledCode, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
	{
		Free(AssembledCode);
		return FALSE;
	}
	Free(AssembledCode);

	BOOLEAN AlreadyAddedOperations = FALSE;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		T->LinkData.Flags |= CODE_FLAG_IN_GROUP;

		if (!AlreadyAddedOperations && XED_ICLASS_LEA == XedDecodedInstGetIClass(&T->DecodedInst))
		{
			T->LinkData.Id = TargetLabelId;
			T->LinkData.Flags |= CODE_FLAG_USES_LABEL;
			NrAddPreAssemblyOperation(T, EmInternalRipDeltaFinder, (PVOID)LeftOver, 0UL, FALSE);

			T->Next->LinkData.Id = LmPeekNextId(LabelManager);
			T->Next->LinkData.Flags |= CODE_FLAG_USES_LABEL;
			NrAddPreAssemblyOperation(T->Next, EmInternalRipDeltaFinder, NULL, 0UL, FALSE);
			AlreadyAddedOperations = TRUE;
		}
	}

	PNATIVE_LINK NotTakenLabelLink = NrAllocateLink();
	if (!NotTakenLabelLink)
	{
		NrFreeBlock(Block);
		return FALSE;
	}
	NrInitForLabel(NotTakenLabelLink, LmNextId(LabelManager), NULL, NULL);
	IrPutLinkBack(Block, NotTakenLabelLink);

	Block->Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block->Back->LinkData.Flags |= CODE_FLAG_GROUP_END;

	return TRUE;
}

BOOLEAN EmRelativeConditionalJumpToAbsolute2Randomize(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 TargetLabelId, PLABEL_MANAGER LabelManager, INT64 LeftOver)
{
	/*
	* Same as above but random
	*
	*	- PUSH RAX
	*	- PUSH RBX
	*	- LEA RBX,[RIP+TakenDisp]
	*	- LEA RAX,[RIP+NotTakenDisp]
	*	- CMOVcc RAX,RBX
	*	- POP RBX
	*	- XCHG RAX,[RSP]
	*	- RET
	*/

	Block->Front = Block->Back = NULL;

	//Break basic pattern scanning by mixing up what registers are used.
	XED_REG_ENUM Reg1 = (XED_REG_ENUM)(XED_REG_RAX + RndGetRandomNum(0, 15));
	XED_REG_ENUM Reg2 = Reg1;
	while (Reg2 == Reg1) Reg2 = (XED_REG_ENUM)(XED_REG_RAX + RndGetRandomNum(0, 15));

	//Invert the condition code for the mov.
	BOOLEAN InvertCMOVcc = RndGetRandomNum<UINT32>(0, 1);

	//Invert the order in which we load the jump targets, a BIT more difficult to make an automated tool to replace all this?
	BOOLEAN InvertAddressLoadingOrder = RndGetRandomNum<UINT32>(0, 1);
	XED_ICLASS_ENUM CMOVcc = XedJccToCMOVcc(InvertCMOVcc ? XedInvertJcc(XedDecodedInstGetIClass(&Jmp->DecodedInst)) : XedDecodedInstGetIClass(&Jmp->DecodedInst));
	if (CMOVcc == XED_ICLASS_INVALID)
		return FALSE;

	XED_ENCODER_INSTRUCTION InstList[8];
	XedInst1(&InstList[0], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(Reg1));
	XedInst1(&InstList[1], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(Reg2));
	if (InvertAddressLoadingOrder)
	{
		XedInst2(&InstList[2], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(Reg1), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
		XedInst2(&InstList[3], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(Reg2), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	}
	else
	{
		XedInst2(&InstList[2], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(Reg2), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
		XedInst2(&InstList[3], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(Reg1), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	}
	if (InvertCMOVcc)
		XedInst2(&InstList[4], XedGlobalMachineState, CMOVcc, 64, XedReg(Reg2), XedReg(Reg1));
	else
		XedInst2(&InstList[4], XedGlobalMachineState, CMOVcc, 64, XedReg(Reg1), XedReg(Reg2));
	XedInst1(&InstList[5], XedGlobalMachineState, XED_ICLASS_POP, 64, XedReg(Reg2));
	XedInst2(&InstList[6], XedGlobalMachineState, XED_ICLASS_XCHG, 64, XedMemB(XED_REG_RSP, 64), XedReg(Reg1));
	XedInst0(&InstList[7], XedGlobalMachineState, XED_ICLASS_RET_NEAR, 64);

	UINT32 OutSize = 0;
	PUCHAR AssembledCode = XedEncodeInstructions(InstList, 8, &OutSize);
	if (!AssembledCode || !OutSize)
		return FALSE;

	if (!NrDecodePerfectEx(Block, AssembledCode, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
	{
		Free(AssembledCode);
		return FALSE;
	}
	Free(AssembledCode);

	BOOLEAN AlreadyAddedOperations = FALSE;
	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		T->LinkData.Flags |= CODE_FLAG_IN_GROUP;

		if (!AlreadyAddedOperations && XED_ICLASS_LEA == XedDecodedInstGetIClass(&T->DecodedInst))
		{
			T->LinkData.Id = TargetLabelId;
			T->LinkData.Flags |= CODE_FLAG_USES_LABEL;
			NrAddPreAssemblyOperation(T, EmInternalRipDeltaFinder, (PVOID)LeftOver, 0UL, FALSE);

			T->Next->LinkData.Id = LmPeekNextId(LabelManager);
			T->Next->LinkData.Flags |= CODE_FLAG_USES_LABEL;
			NrAddPreAssemblyOperation(T->Next, EmInternalRipDeltaFinder, NULL, 0UL, FALSE);
			AlreadyAddedOperations = TRUE;
		}
	}


	Block->Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block->Back->LinkData.Flags |= CODE_FLAG_GROUP_END;

	//Label is not included in the group because the delta can be resolved whenever.
	//So it can be moved around, and likely will. Great opportunity to put a fake prologue right after the above RET.
	PNATIVE_LINK NotTakenLabelLink = NrAllocateLink();
	if (!NotTakenLabelLink)
	{
		NrFreeBlock(Block);
		return FALSE;
	}

	NrInitForLabel(NotTakenLabelLink, LmNextId(LabelManager), NULL, NULL);
	IrPutLinkBack(Block, NotTakenLabelLink);


	return TRUE;
}

