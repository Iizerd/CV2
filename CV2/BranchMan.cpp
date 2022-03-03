#include "BranchMan.h"

BOOLEAN BmGenerateEmulateRet1(PNATIVE_BLOCK Block, UINT JunkSize)
{
	//-	POP [RIP + 6 + Delta]
	//-	JMP [RIP + Delta]
	//-	**JUNK**
	//- [Return Address]

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

	IrPutLinkBack(Block, JunkLink);

	//Intellisense think these being null!
	Block->Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block->Back->LinkData.Flags |= CODE_FLAG_GROUP_END;

	for (PNATIVE_LINK T = Block->Front; T; T = T->Next)
		T->LinkData.Flags |= CODE_FLAG_IN_GROUP;

	return TRUE;
}

BOOLEAN BmGenerateEmulateRet2(PNATIVE_BLOCK Block, UINT Junk)
{
	/*
	*  - JMP Delta
	*  - PUSH 0xC3
	*/

	//Generate the deadstore instruction(s) and save the delta in a variable.
	if (Junk & DEADSTORE_METHOD_RANDOM)
	{

	}

	//Generate the relative jump and put it before the deadstore in the block.


}

BOOLEAN BmGenerateRelJumpReplacement(PNATIVE_BLOCK Block, INT32 Displacement)
{
	/*
	* Relative Jump Remover :
	*	- Similar to Ret removed.
	*	- PUSH RAX
	*	- LEA RAX, [RIP]
	*	- MOV[RIP + 9 + Delta], RAX
	*	- POP RAX
	*	- ADD[RIP + 6 + Delta], JumpDisp
	*	- JMP[RIP + Delta]
	*	- Saving rip, adding the jump offset to it ourselves, and jumping to it.
	*	- Can be alternatively accomplished using a 'RET' instruction. might trick the dissasembler even more.
	* 
	*/

	//Need to figure out a way to do this with the label scheme.
}