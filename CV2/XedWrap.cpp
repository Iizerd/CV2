#include "XedWrap.h"

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
	PUCHAR EncodeBuffer = new UCHAR[InstCount * 15];
	if (!EncodeBuffer)
		return NULL;

	for (UINT i = 0; i < InstCount; i++)
	{
		XedEncoderRequestZeroSetMode(&EncoderRequest, &XedGlobalMachineState);
		if (!XedConvertToEncoderRequest(&EncoderRequest, &InstList[i]) || XED_ERROR_NONE != (Err = XedEncode(&EncoderRequest, &EncodeBuffer[TotalSize], 15, &ReturnedSize)))
		{
			printf("Error encoding instruction: %u, %s\n", i, XedErrorEnumToString(Err));
			delete[] EncodeBuffer;
			return NULL;
		}
		TotalSize += ReturnedSize;
	}

	PUCHAR RetBuffer = new UCHAR[TotalSize];
	if (!RetBuffer)
	{
		delete[] EncodeBuffer;
		return NULL;
	}

	RtlCopyMemory(RetBuffer, EncodeBuffer, TotalSize);
	*OutSize = TotalSize;
	return RetBuffer;
}