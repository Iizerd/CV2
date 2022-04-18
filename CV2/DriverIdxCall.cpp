#include "DriverIdxCall.h"
#include "Logging.h"
/*
			...
			MOV [RIP_VolatileRegStorage],R11	;Store volatile register, to restore after. Using RIP relative mov.
			LEA R11,[RIP_Offset]				;Load the value Jmp gadget to jump to.

			PUSH [RIP_JmpNonVolatileGadgetAddr]	;Push return address inside of valid module
			JMP [RIP]							;Jmp to import addr

			[FunctionAddr]
			[JmpNonVolatileGadgetAddr]
			[VolatileRegStorage]				;8 bytes of dead space where volatile register is stored.
		Offset:
			MOV R11,[RIP_VolatileRegStorage]	;Restore volatile register
			...
*/

BOOLEAN DiGenerateNonVolatileCallGadget(PNATIVE_BLOCK Block, XED_REG_ENUM NonVolatileRegister, PUINT32 FunctionAddrOffset, PUINT32 GadgetAddrOffset)
{
	Block->Front = Block->Back = NULL;
	LmClear(&Block->LabelManager);

	//Encode the first 6 instructions
	XED_ENCODER_INSTRUCTION InstList[6];
	XedInst2(&InstList[0], XedGlobalMachineState, XED_ICLASS_MOV, 64, XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64), XedReg(NonVolatileRegister));
	XedInst2(&InstList[1], XedGlobalMachineState, XED_ICLASS_LEA, 64, XedReg(NonVolatileRegister), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	XedInst1(&InstList[2], XedGlobalMachineState, XED_ICLASS_PUSH, 64, XedMemBD(XED_REG_RIP, XedDisp(6 + 8, 32), 64));
	XedInst1(&InstList[3], XedGlobalMachineState, XED_ICLASS_JMP, 64, XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));


	UINT32 OutSize = 0;
	PUCHAR EncodedData = XedEncodeInstructions(InstList, 4, &OutSize);
	if (!EncodedData || !OutSize)
		return FALSE;
	NATIVE_BLOCK Block1;
	if (!NrDecodePerfectEx(&Block1, EncodedData, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
	{
		MLog("Failed to decode first 4 instructions.\n");
		Free(EncodedData);
		return FALSE;
	}
	Free(EncodedData);
	EncodedData = NULL;

	//Allocate space for function address and jmp nv address
	PNATIVE_LINK NvRegStorageLink = NrAllocateLink();
	if (!NvRegStorageLink)
	{
		MLog("Failed to allocate storage link.\n");
		NrFreeBlock(&Block1);
		return FALSE;
	}
	NvRegStorageLink->LinkData.Flags |= CODE_FLAG_IS_RAW_DATA;
	NvRegStorageLink->RawData = Allocate(0x18);
	NvRegStorageLink->RawDataSize = 0x18;
	RtlFillMemory(NvRegStorageLink->RawData, 0x18, 0xCC);
	IrPutLinkBack(&Block1, NvRegStorageLink);

	//Patch the displacements into the first 2 instructions
	UINT32 BlockSize = NrCalcBlockSize(&Block1);
	if (!BlockSize)
	{
		MLog("Block size was zero.\n");
		NrFreeBlock(&Block1);
		return FALSE;
	}
	INT32 Inst1Disp = BlockSize -  Block1.Front->RawDataSize - 8;			//Remove first instruction size
	INT32 Inst2Disp = Inst1Disp -  Block1.Front->Next->RawDataSize;		//Remove second instruction size
	Inst2Disp += 8;
	if (!XedPatchDisp(&Block1.Front->DecodedInst, (PUCHAR)Block1.Front->RawData, XedDisp(Inst1Disp, 32)) ||				//Patch first inst
		!XedPatchDisp(&Block1.Front->Next->DecodedInst, (PUCHAR)Block1.Front->Next->RawData, XedDisp(Inst2Disp, 32)))	//Patch next inst
	{
		MLog("Failed to patch instructions.\n");
		NrFreeBlock(&Block1);
		return FALSE;
	}
	*FunctionAddrOffset = BlockSize - 0x18;
	*GadgetAddrOffset = BlockSize - 0x10;

	//Encode the instruction to restore the nonvolatile register
	XedInst2(&InstList[0], XedGlobalMachineState, XED_ICLASS_MOV, 64, XedReg(NonVolatileRegister), XedMemBD(XED_REG_RIP, XedDisp(0, 32), 64));
	EncodedData = XedEncodeInstructions(InstList, 1, &OutSize);
	if (!EncodedData || !OutSize)
	{
		MLog("Failed to encode final mov.\n");
		NrFreeBlock(&Block1);
		return FALSE;
	}
	NATIVE_BLOCK Block2;
	if (!NrDecodePerfectEx(&Block2, EncodedData, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS) || 1 != IrCountLinks(&Block2))
	{
		//Leaks Block2 if succeeded but...
		MLog("Failed to decode mov instruction.\n");
		NrFreeBlock(&Block1);
		Free(EncodedData);
		return FALSE;
	}
	Free(EncodedData);
	IrPutLinkBack(&Block1, Block2.Front);

	//Patch displacement for the final instruction
	INT32 Inst3Disp = 0;
	Inst3Disp -= Block1.Back->RawDataSize;		//Account for size of the instruction
	Inst3Disp -= 8;								//Account for size of the storage spot
	if (!XedPatchDisp(&Block1.Back->DecodedInst, (PUCHAR)Block1.Back->RawData, XedDisp(Inst3Disp, 32)))
	{
		MLog("Failed to patch final mov.\n");
		NrFreeBlock(&Block1);
		return FALSE;
	}

	//Mark entire block as not to be touched
	Block1.Front->LinkData.Flags |= CODE_FLAG_GROUP_START;
	Block1.Back->LinkData.Flags |= CODE_FLAG_GROUP_END;
	for (PNATIVE_LINK T = Block1.Front; T /*&& T != Block1.Back->Next*/; T = T->Next)
		T->LinkData.Flags |= (CODE_FLAG_IN_GROUP | CODE_FLAG_DO_NOT_TOUCH);

	Block->Front = Block1.Front;
	Block->Back = Block1.Back;
	return TRUE;
}

BOOLEAN DiGenerateMovRaxJmpRax(PNATIVE_BLOCK Block)
{
	Block->Front = Block->Back = NULL;
	LmClear(&Block->LabelManager);

	//Encode the first 2 instructions
	XED_ENCODER_INSTRUCTION InstList[2];
	XedInst2(&InstList[0], XedGlobalMachineState, XED_ICLASS_MOV, 64, XedReg(XED_REG_RAX), XedImm0(0, 64));
	XedInst1(&InstList[1], XedGlobalMachineState, XED_ICLASS_CALL_NEAR, 64, XedReg(XED_REG_RAX));

	UINT32 OutSize = 0;
	PUCHAR EncodedData = XedEncodeInstructions(InstList, 2, &OutSize);
	if (!EncodedData || !OutSize)
		return FALSE;
	if (!NrDecodePerfectEx(Block, EncodedData, OutSize, DECODER_FLAG_DONT_GENERATE_OPERATIONS))
	{
		MLog("Failed to decode omeglaalw.\n");
		Free(EncodedData);
		return FALSE;
	}
	Free(EncodedData);
	return TRUE;
}

BOOLEAN DiEnumerateCalls(PNATIVE_BLOCK Block, STDVECTOR<STDPAIR<UINT32, UINT32> >* FunctionIndices)
{
	NATIVE_BLOCK CallGadget;
	UINT32 CurrentOffset = 0;
	UINT32 GadgetSize = 0;
	if (!DiGenerateMovRaxJmpRax(&CallGadget))
		return FALSE;

	GadgetSize = NrCalcBlockSize(&CallGadget);
	if (!GadgetSize)
		return FALSE;

	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		PNATIVE_LINK RealNext = T->Next;
		if ((T->LinkData.Flags & CODE_FLAG_IS_INST) && 
			T->Next && 
			(T->Next->LinkData.Flags & CODE_FLAG_IS_INST))
		{
			if (XedDecodedInstGetIClass(&T->DecodedInst) == XED_ICLASS_MOV &&
				XedDecodedInstGetIClass(&T->Next->DecodedInst) == XED_ICLASS_CALL_NEAR)
			{
				if (XedDecodedInstGetReg(&T->DecodedInst, XED_OPERAND_REG0) != XED_REG_EAX)
					return FALSE;

				INT32 ImportIndex = XedDecodedInstGetSignedImmediate(&T->DecodedInst);

				RealNext = T->Next->Next;
				NATIVE_BLOCK CallGadgetCopy;
				if (!NrDeepCopyBlock2(&CallGadgetCopy, &CallGadget))
					return FALSE;
				IrReplaceBlock(Block, T, T->Next, &CallGadgetCopy);
				NrFreeLink(T->Next);
				NrFreeLink(T);
				FunctionIndices->emplace_back(ImportIndex, CurrentOffset + 2);
				CurrentOffset += GadgetSize;
				T = RealNext;
				printf("Found one.\n");
				continue;
			}
			//printf("IClass: %s\n", XedIClassEnumToString(XedDecodedInstGetIClass(&T->DecodedInst)));
		}
		if (T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE)
			CurrentOffset += T->RawDataSize;
		T = RealNext;
	}
	return TRUE;
}

BOOLEAN DiEnumerateCalls2(PNATIVE_BLOCK Block, XED_REG_ENUM NonVolatileRegister, STDVECTOR<STDPAIR<UINT32, UINT32> >* FunctionIndices, STDVECTOR<UINT32>* RetGadgetIndices)
{
	NATIVE_BLOCK CallGadget;
	UINT32 FuncAddrOffset, GadgetAddrOffset;
	UINT32 CurrentOffset = 0;
	UINT32 GadgetSize = 0;
	if (!DiGenerateNonVolatileCallGadget(&CallGadget, NonVolatileRegister, &FuncAddrOffset, &GadgetAddrOffset))
		return FALSE;

	GadgetSize = NrCalcBlockSize(&CallGadget);
	if (!GadgetSize)
		return FALSE;

	for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		PNATIVE_LINK RealNext = T->Next;
		if ((T->LinkData.Flags & CODE_FLAG_IS_INST) &&
			T->Next &&
			(T->Next->LinkData.Flags & CODE_FLAG_IS_INST))
		{
			if (XedDecodedInstGetIClass(&T->DecodedInst) == XED_ICLASS_MOV &&
				XedDecodedInstGetIClass(&T->Next->DecodedInst) == XED_ICLASS_CALL_NEAR)
			{
				if (XedDecodedInstGetReg(&T->DecodedInst, XED_OPERAND_REG0) != XED_REG_EAX)
					return FALSE;

				INT32 ImportIndex = XedDecodedInstGetSignedImmediate(&T->DecodedInst);

				RealNext = T->Next->Next;
				NATIVE_BLOCK CallGadgetCopy;
				if (!NrDeepCopyBlock2(&CallGadgetCopy, &CallGadget))
					return FALSE;
				IrReplaceBlock(Block, T, T->Next, &CallGadgetCopy);
				NrFreeLink(T->Next);
				NrFreeLink(T);
				FunctionIndices->emplace_back(ImportIndex, CurrentOffset + FuncAddrOffset);
				RetGadgetIndices->push_back(CurrentOffset + GadgetAddrOffset);
				CurrentOffset += GadgetSize;
				T = RealNext;
				printf("Found one.\n");
				continue;
			}
			printf("IClass: %s\n", XedIClassEnumToString(XedDecodedInstGetIClass(&T->DecodedInst)));
		}
		if (T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE)
			CurrentOffset += T->RawDataSize;
		T = RealNext;
	}

	/*for (PNATIVE_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		PNATIVE_LINK RealNext = T->Next;
		if (!(T->LinkData.Flags & CODE_FLAG_OCCUPIES_SPACE))
			continue;
		if (T->Next)
		{
			XED_ICLASS_ENUM IClass1 = XedDecodedInstGetIClass(&T->DecodedInst);
			XED_ICLASS_ENUM IClass2 = XedDecodedInstGetIClass(&T->Next->DecodedInst);
			if (IClass1 == XED_ICLASS_MOV && 
				IClass2 == XED_ICLASS_CALL_NEAR)
			{
				if (XedDecodedInstGetReg(&T->DecodedInst, XED_OPERAND_REG0) != XED_REG_EAX)
					return FALSE;

				INT32 ImportIndex = XedDecodedInstGetSignedImmediate(&T->DecodedInst);

				RealNext = T->Next->Next;
				NATIVE_BLOCK CallGadgetCopy;
				if (!NrDeepCopyBlock2(&CallGadgetCopy, &CallGadget))
					return FALSE;
				IrReplaceBlock(Block, T, T->Next, &CallGadgetCopy);
				NrFreeLink(T->Next);
				NrFreeLink(T);
				FunctionIndices->emplace_back(ImportIndex, CurrentOffset + FuncAddrOffset);
				RetGadgetIndices->push_back(CurrentOffset + GadgetAddrOffset);
				CurrentOffset += GadgetSize;
				T = RealNext;
				continue;
			}
		}
		CurrentOffset += T->RawDataSize;
		T = RealNext;
	}*/
	return TRUE;
}
