#include "XedWrap.h"
#include "Logging.h"

VOID XedGlobalInit()
{
	XedTablesInit();
	XedGlobalMachineState;
	XedGlobalMachineState.mmode = XED_MACHINE_MODE_LONG_64;
	XedGlobalMachineState.stack_addr_width = XED_ADDRESS_WIDTH_64b;
}

PUCHAR XedEncodeInstructions(XED_ENCODER_INSTRUCTION* InstList, UINT32 InstCount, PUINT32 OutSize)
{
	XED_ENCODER_REQUEST EncoderRequest;
	UINT32 ReturnedSize = 0;
	UINT32 TotalSize = 0;
	XED_ERROR_ENUM XedError = XED_ERROR_GENERAL_ERROR;

	*OutSize = 0;
	PUCHAR EncodeBuffer = (PUCHAR)Allocate(InstCount * 15);
	if (!EncodeBuffer)
	{
		MLog("Cound not allocate memory for encoder buffer.\n");
		return NULL;
	}

	for (UINT32 i = 0; i < InstCount; i++)
	{
		XedEncoderRequestZeroSetMode(&EncoderRequest, &XedGlobalMachineState);
		if (!XedConvertToEncoderRequest(&EncoderRequest, &InstList[i]) || XED_ERROR_NONE != (XedError = XedEncode(&EncoderRequest, &EncodeBuffer[TotalSize], 15, &ReturnedSize)))
		{
			MLog("Error encoding instruction: %u, %s\n", i, XedErrorEnumToString(XedError));
			Free(EncodeBuffer);
			return NULL;
		}
		TotalSize += ReturnedSize;
	}

	PUCHAR FinalBuffer = (PUCHAR)Realloc(EncodeBuffer, TotalSize);
	if (!FinalBuffer)
	{
		MLog("Could not allocate memory for return buffer in XedEncodeInstructions");
		Free(EncodeBuffer);
		return NULL;
	}
	*OutSize = TotalSize;
	return FinalBuffer;
}

XED_CONDITION_CODE XedConditionCodeJcc(XED_ICLASS_ENUM Jcc)
{
	switch (Jcc)
	{
	case XED_ICLASS_JB: return XED_CC_B;
	case XED_ICLASS_JBE: return XED_CC_BE;
	case XED_ICLASS_JL: return XED_CC_L;
	case XED_ICLASS_JLE: return XED_CC_LE;
	case XED_ICLASS_JNB: return XED_CC_NB;
	case XED_ICLASS_JNBE: return XED_CC_NBE;
	case XED_ICLASS_JNL: return XED_CC_NL;
	case XED_ICLASS_JNLE: return XED_CC_NLE;
	case XED_ICLASS_JNO: return XED_CC_NO;
	case XED_ICLASS_JNP: return XED_CC_NP;
	case XED_ICLASS_JNS: return XED_CC_NS;
	case XED_ICLASS_JNZ: return XED_CC_NZ;
	case XED_ICLASS_JO: return XED_CC_O;
	case XED_ICLASS_JP: return XED_CC_P;
	case XED_ICLASS_JS: return XED_CC_S;
	case XED_ICLASS_JZ: return XED_CC_Z;
	case XED_ICLASS_JCXZ: return XED_CC_CXZ;
	case XED_ICLASS_JECXZ: return XED_CC_ECXZ;
	case XED_ICLASS_JRCXZ: return XED_CC_RCXZ;
	default: return XED_CC_INVALID;
	}
}

XED_CONDITION_CODE XedConditionCodeCMOVcc(XED_ICLASS_ENUM CMOVcc)
{
	switch (CMOVcc)
	{
	case XED_ICLASS_CMOVB: return XED_CC_B;
	case XED_ICLASS_CMOVBE: return XED_CC_BE;
	case XED_ICLASS_CMOVL: return XED_CC_L;
	case XED_ICLASS_CMOVLE: return XED_CC_LE;
	case XED_ICLASS_CMOVNB: return XED_CC_NB;
	case XED_ICLASS_CMOVNBE: return XED_CC_NBE;
	case XED_ICLASS_CMOVNL: return XED_CC_NL;
	case XED_ICLASS_CMOVNLE: return XED_CC_NLE;
	case XED_ICLASS_CMOVNO: return XED_CC_NO;
	case XED_ICLASS_CMOVNP: return XED_CC_NP;
	case XED_ICLASS_CMOVNS: return XED_CC_NS;
	case XED_ICLASS_CMOVNZ: return XED_CC_NZ;
	case XED_ICLASS_CMOVO: return XED_CC_O;
	case XED_ICLASS_CMOVP: return XED_CC_P;
	case XED_ICLASS_CMOVS: return XED_CC_S;
	case XED_ICLASS_CMOVZ: return XED_CC_Z;
	default: return XED_CC_INVALID;
	}
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

UINT32 XedCalcWidthBits(LONGLONG Displacement)
{
	return log2(abs(Displacement)) + 1;
}

UINT32 XedSignedDispWidth(LONGLONG Displacement)
{
	if ((Displacement <= 0 && Displacement > -129) || (Displacement > 0 && Displacement < 128))
		return 8;
	return 32;
}

