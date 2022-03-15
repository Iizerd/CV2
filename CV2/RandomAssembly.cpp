#include "RandomAssembly.h"


XED_REG_ENUM RaGetRandomRegister(UINT32 WidthInBytes)
{
	UINT32 Offset = (RndGetRandomNum<UINT>(0, 15));
	switch (WidthInBytes)
	{
	case 1:
		return (XED_REG_ENUM)(XED_REG_AL + Offset);
	case 2:
		return (XED_REG_ENUM)(XED_REG_AX + Offset);
	case 4:
		return (XED_REG_ENUM)(XED_REG_EAX + Offset);
	case 8:
		return (XED_REG_ENUM)(XED_REG_RAX + Offset);
	}
	return XED_REG_INVALID;
}

XED_ICLASS_ENUM RaGetRandomBitwiseIClass()
{
	switch (RndGetRandomNum<UINT>(0, 2))
	{
	case 0: return XED_ICLASS_OR;
	case 1: return XED_ICLASS_AND;
	case 2: return XED_ICLASS_XOR;
	}
	return XED_ICLASS_INVALID;
}
XED_ICLASS_ENUM RaGetRandomBranchIClass()
{
	switch (RndGetRandomNum<UINT>(0, 14))
	{
	case 0: return XED_ICLASS_JL;
	case 1: return XED_ICLASS_JLE;
	case 2:	return XED_ICLASS_JNB;
	case 3: return XED_ICLASS_JNBE;
	case 4: return XED_ICLASS_JNL;
	case 5: return XED_ICLASS_JNLE;
	case 6: return XED_ICLASS_JNO;
	case 7: return XED_ICLASS_JNP;
	case 8: return XED_ICLASS_JNS;
	case 9: return XED_ICLASS_JNZ;
	case 10: return XED_ICLASS_JO;
	case 11: return XED_ICLASS_JP;
	case 13: return XED_ICLASS_JS;
	case 14: return XED_ICLASS_JZ;
	}
}

POSTOP_STATUS RaRandomizeInstAfterAssembly(PNATIVE_LINK Link, PUCHAR RawData, PVOID Context)
{
	for (UINT32 i = 0; i < Link->RawDataSize; i++)
	{
		RawData[i] = (UCHAR)RndGetRandomNum<UINT>(0, 255);
	}
	return POSTOP_SUCCESS;
}
