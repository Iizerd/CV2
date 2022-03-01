#include "XedWrap.h"
#include "Logging.h"

VOID XedGlobalInit()
{
	XedTablesInit();
	XedGlobalMachineState;
	XedGlobalMachineState.mmode = XED_MACHINE_MODE_LONG_64;
	XedGlobalMachineState.stack_addr_width = XED_ADDRESS_WIDTH_64b;
}

PUCHAR XedEncodeInstructions(XED_ENCODER_INSTRUCTION* InstList, UINT InstCount, PUINT OutSize)
{
	XED_ENCODER_REQUEST EncoderRequest;
	UINT ReturnedSize = 0;
	UINT TotalSize = 0;
	XED_ERROR_ENUM Err = XED_ERROR_NONE;

	*OutSize = 0;
	PUCHAR EncodeBuffer = (PUCHAR)Allocate(InstCount * 15);
	if (!EncodeBuffer)
		return NULL;

	for (UINT i = 0; i < InstCount; i++)
	{
		XedEncoderRequestZeroSetMode(&EncoderRequest, &XedGlobalMachineState);
		if (!XedConvertToEncoderRequest(&EncoderRequest, &InstList[i]) || XED_ERROR_NONE != (Err = XedEncode(&EncoderRequest, &EncodeBuffer[TotalSize], 15, &ReturnedSize)))
		{
			MLog("Error encoding instruction: %u, %s\n", i, XedErrorEnumToString(Err));
			Free(EncodeBuffer);
			return NULL;
		}
		TotalSize += ReturnedSize;
	}

	PUCHAR RetBuffer = (PUCHAR)Allocate(TotalSize);
	if (!RetBuffer)
	{
		MLog("Could not allocate memory for return buffer in XedEncodeInstructions");
		Free(EncodeBuffer);
		return NULL;
	}

	RtlCopyMemory(RetBuffer, EncodeBuffer, TotalSize);
	*OutSize = TotalSize;
	return RetBuffer;
}