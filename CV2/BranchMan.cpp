#include "BranchMan.h"
#include "Logging.h"
#include "RandomAssembly.h"

BOOLEAN BmGenerateEmulateRet1(PNATIVE_BLOCK Block, UINT JunkSize)
{
	//-	POP [RIP + 6 + Delta]		[GROUP_START]
	//-	JMP [RIP + Delta]
	//-	**JUNK**
	//- [Return Address]			[GROUP_END]

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
	NrAddPostAssemblyOperation(JunkLink, RaRandomizeInstAfterAssembly, NULL, FALSE);

	IrPutLinkBack(Block, JunkLink);

	NrMarkBlockAsGroup(Block);
	return TRUE;
}

BOOLEAN BmGenerateEmulateRet2(PNATIVE_BLOCK Block, UINT JunkSize, UINT DeadstoreMethod)
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
		UINT ImmWidth = 3;
		while (ImmWidth == 3)
			ImmWidth = RndGetRandomNum<INT32>(1, 4);	//1, 2, 4

		UINT ImmWidthBits = ImmWidth * 8;				//8, 16, 32
		INT32 OffsetInsideImm = RndGetRandomNum<INT32>(0, ImmWidth - 1);
		for (INT32 i = 0; i < ImmWidth; i++)
		{
			if (i == OffsetInsideImm)
				ImmValue |= ((UINT32)(0xC3) << (i * 8));
			else
				ImmValue |= ((UINT32)(RndGetRandomNum<INT32>(0, 255)) << (i * 8));
		}

		UINT OutSize = 0;
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
		NrAddPostAssemblyOperation(JunkLink, RaRandomizeInstAfterAssembly, NULL, FALSE);
		JmpDisp += JunkSize;
		IrPutLinkFront(Block, JunkLink);
	}

	//Generate the relative jump and put it before the deadstore in the block.
	//We do not need to add a preop to recalculate this delta because this group will NOT be edited.
	{
		XED_ENCODER_INSTRUCTION JmpInst;
		UINT OutSize = 0;
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

BOOLEAN BmConvertRelNonConJumpToAbsolute(PNATIVE_BLOCK Block, INT32 Displacement)
{
	/*
	* Relative Jump Remover :
	*	- PUSH RAX
	*	- PUSHFQ
	*	- MOVABS RAX, Displacement
	*	- PUSH RAX
	*	- LEA RAX,[RIP]
	*	- ADD RAX,[RSP]
	*	- ADD RSP,8
	*	- POPFQ
	*	- XCHG RAX,[RSP]
	*	- RET					; Can additionally remove this Ret with the Emulators above.
	*/

	INT64 WDisplacement = (INT64)Displacement;

	XED_ENCODER_INSTRUCTION InstList[10];
	XedInst1(&InstList[0], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(XED_REG_RAX));
	XedInst0(&InstList[1], XedGlobalMachineState, XED_ICLASS_PUSHFQ, 64);
	XedInst2(&InstList[2], XedGlobalMachineState, XED_ICLASS_MOV, 64, XedReg(XED_REG_RAX), XedImm0(*(PULONGLONG)&WDisplacement, 64));
	XedInst1(&InstList[3], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedReg(XED_REG_RAX));
	XedInst2(&InstList[4], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(XED_REG_RAX), XedMemBD(XED_REG_RIP, XedDisp(-1, 32), 64)); 
	XedInst2(&InstList[5], XedGlobalMachineState, XED_ICLASS_ADD, 64, XedReg(XED_REG_RAX), XedMemB(XED_REG_RSP, 64));
	XedInst2(&InstList[6], XedGlobalMachineState, XED_ICLASS_ADD, 64, XedReg(XED_REG_RSP), XedImm0(8, 8));
	XedInst0(&InstList[7], XedGlobalMachineState, XED_ICLASS_POPFQ, 64);
	XedInst2(&InstList[8], XedGlobalMachineState, XED_ICLASS_XCHG, 64, XedReg(XED_REG_RAX), XedMemB(XED_REG_RSP, 64));
	XedInst0(&InstList[9], XedGlobalMachineState, XED_ICLASS_RET_NEAR, 64);

	//now decode and output :D
}

BOOLEAN BmConvertRelConJumpToAbsolute(PNATIVE_BLOCK Block, PNATIVE_LINK Jmp, INT32 Displacement)
{
	/*
	* Relative Conditional Jump emulator.
	* 
	*		BRANCH
	*		JMP NotTakenEndJumpLabel
	*	TakeLabel:
	*		BmConvertRelNonConJumpToAbsolute
	*	NotTakenEndJumpLabel:
	*/
}