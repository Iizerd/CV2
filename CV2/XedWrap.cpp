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
		MLog("Encoded %d with class %s with length %d\n", i, XedIClassEnumToString(xed_encoder_request_get_iclass(&EncoderRequest)), ReturnedSize);
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
	Free(EncodeBuffer);
	*OutSize = TotalSize;
	return RetBuffer;
}

XED_ICLASS_ENUM XedJccToCMOVcc(XED_ICLASS_ENUM Jcc)
{

	switch (Jcc)
	{
	case XED_ICLASS_JB: return XED_ICLASS_CMOVB;
	case XED_ICLASS_JBE: return XED_ICLASS_CMOVBE;
	case XED_ICLASS_JL: return XED_ICLASS_CMOVL;
	case XED_ICLASS_JLE: return XED_ICLASS_CMOVLE;
	case XED_ICLASS_JNB: return XED_ICLASS_CMOVNB;
	case XED_ICLASS_JNBE: return XED_ICLASS_CMOVNBE;
	case XED_ICLASS_JNL: return XED_ICLASS_CMOVNL;
	case XED_ICLASS_JNLE: return XED_ICLASS_CMOVNLE;
	case XED_ICLASS_JNO: return XED_ICLASS_CMOVNO;
	case XED_ICLASS_JNP: return XED_ICLASS_CMOVNP;
	case XED_ICLASS_JNS: return XED_ICLASS_CMOVNS;
	case XED_ICLASS_JNZ: return XED_ICLASS_CMOVNZ;
	case XED_ICLASS_JO: return XED_ICLASS_CMOVO;
	case XED_ICLASS_JP: return XED_ICLASS_CMOVP;
	case XED_ICLASS_JS: return XED_ICLASS_CMOVS;
	case XED_ICLASS_JZ: return XED_ICLASS_CMOVZ;
	default: return XED_ICLASS_INVALID;
	}

	//This will work, but if for whatever reason it was different for someone else...
	//The switch statement will ALWAYS work.
	//return (XED_ICLASS_ENUM)(XED_ICLASS_CMOVB + (Jcc - XED_ICLASS_JB));
}

XED_ICLASS_ENUM XedInvertJcc(XED_ICLASS_ENUM Jcc)
{
	switch (Jcc)
	{
	case XED_ICLASS_JB: return XED_ICLASS_JNB;
	case XED_ICLASS_JBE: return XED_ICLASS_JNBE;
	case XED_ICLASS_JL: return XED_ICLASS_JNL;
	case XED_ICLASS_JLE: return XED_ICLASS_JNLE;
	case XED_ICLASS_JNB: return XED_ICLASS_JB;
	case XED_ICLASS_JNBE: return XED_ICLASS_JBE;
	case XED_ICLASS_JNL: return XED_ICLASS_JL;
	case XED_ICLASS_JNLE: return XED_ICLASS_JLE;
	case XED_ICLASS_JNO: return XED_ICLASS_JO;
	case XED_ICLASS_JNP: return XED_ICLASS_JP;
	case XED_ICLASS_JNS: return XED_ICLASS_JS;
	case XED_ICLASS_JNZ: return XED_ICLASS_JZ;
	case XED_ICLASS_JO: return XED_ICLASS_JNO;
	case XED_ICLASS_JP: return XED_ICLASS_JNP;
	case XED_ICLASS_JS: return XED_ICLASS_JNS;
	case XED_ICLASS_JZ: return XED_ICLASS_JNZ;
	default: return XED_ICLASS_INVALID;
	}
}
