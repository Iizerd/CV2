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
	*AssembledSize = 0U;
	return NULL;
}