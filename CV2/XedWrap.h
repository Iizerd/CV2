#ifndef __XED_WRAP_H
#define __XED_WRAP_H

#include "Windas.h"

extern "C"
{
#include <xed/xed-interface.h>
}

enum xed_condition_code_t
{
	XED_CC_B,
	XED_CC_BE,
	XED_CC_L,
	XED_CC_LE,
	XED_CC_NB,
	XED_CC_NBE,
	XED_CC_NL,
	XED_CC_NLE,
	XED_CC_NO,
	XED_CC_NP,
	XED_CC_NS,
	XED_CC_NZ,
	XED_CC_O,
	XED_CC_P,
	XED_CC_S,
	XED_CC_Z,
	XED_CC_CXZ,
	XED_CC_ECXZ,
	XED_CC_RCXZ,
	XED_CC_INVALID,
};
#define XED_CONDITION_CODE xed_condition_code_t

#define XED_DECODED_INST xed_decoded_inst_t
#define XED_INST xed_inst_t
#define XED_OPERAND xed_operand_t
#define XED_OPERAND_ENUM xed_operand_enum_t
#define XED_SIMPLE_FLAG xed_simple_flag_t
#define XED_FLAG_SET xed_flag_set_t
#define XED_STATE xed_state_t
#define XED_ENCODER_INSTRUCTION xed_encoder_instruction_t
#define XED_ENCODER_REQUEST xed_encoder_request_t
#define XED_REG_ENUM xed_reg_enum_t

#define XED_OPERAND_TYPE_ENUM xed_operand_type_enum_t
#define XED_ERROR_ENUM xed_error_enum_t
#define XED_CATEGORY_ENUM xed_category_enum_t
#define XED_ICLASS_ENUM xed_iclass_enum_t

#define XedTablesInit xed_tables_init
#define XedDecode xed_decode

#define XedDecodedInstZero xed_decoded_inst_zero
#define XedDecodedInstZeroSetMode xed_decoded_inst_zero_set_mode
#define XedDecodedInstSetMode xed_decoded_inst_set_mode
#define XedDecodedInstGetLength xed_decoded_inst_get_length
#define XedDecodedInstGetCategory xed_decoded_inst_get_category
#define XedDecodedInstGetBranchDisplacementWidth xed_decoded_inst_get_branch_displacement_width
#define XedDecodedInstGetBranchDisplacementWidthBits xed_decoded_inst_get_branch_displacement_width_bits
#define XedDecodedInstGetBranchDisplacement xed_decoded_inst_get_branch_displacement
#define XedDecodedInstInst xed_decoded_inst_inst
#define XedDecodedInstNumOperands xed_decoded_inst_noperands
#define XedDecodedInstGetIClass xed_decoded_inst_get_iclass
#define XedDecodedInstUsesRflags xed_decoded_inst_uses_rflags
#define XedDecodedInstGetRflagsInfo xed_decoded_inst_get_rflags_info
#define XedDecodedInstGetReg xed_decoded_inst_get_reg
#define XedDecodedInstGetSegReg xed_decoded_inst_get_seg_reg
#define XedDecodedInstGetBaseReg xed_decoded_inst_get_base_reg
#define XedDecodedInstGetIndexReg xed_decoded_inst_get_index_reg
#define XedDecodedInstGetScale xed_decoded_inst_get_scale
#define XedDecodedInstGetMemoryDisplacement xed_decoded_inst_get_memory_displacement
#define XedDecodedInstOperandLength xed_decoded_inst_operand_length
#define XedDecodedInstOperandLengthBits xed_decoded_inst_operand_length_bits
#define XedDecodedInstGetImmediateIsSigned xed_decoded_inst_get_immediate_is_signed
#define XedDecodedInstGetSignedImmediate xed_decoded_inst_get_signed_immediate
#define XedDecodedInstGetUnsignedImmediate xed_decoded_inst_get_unsigned_immediate


#define XedInstOperand xed_inst_operand
#define XedOperandType xed_operand_type
#define XedOperandName xed_operand_name
#define XedOperandIsRegister xed_operand_is_register
#define XedOperandWidth xed_operand_width
#define XedOperandWidthBits xed_operand_width_bits
#define XedOperandIsMemoryAddressingRegister xed_operand_is_memory_addressing_register
#define XedOperandReadWriteAction xed_operand_rw
#define XedOperandRead xed_operand_read
#define XedOperandReadOnly xed_operand_read_only
#define XedOperandWritten xed_operand_written
#define XedOperandWrittenOnly xed_operand_written_only
#define XedOperandReadAndWritten xed_operand_read_and_written
#define XedOperandConditionalRead xed_operand_conditional_read
#define XedOperandConditionalWrite xed_operand_conditional_write


#define XedIClassEnumToString xed_iclass_enum_t2str
#define XedErrorEnumToString xed_error_enum_t2str
#define XedCategoryEnumToString xed_category_enum_t2str
#define XedOperandEnumToString xed_operand_enum_t2str
#define XedRegEnumToString xed_reg_enum_t2str


#define XedSimpleFlagGetReadFlagSet xed_simple_flag_get_read_flag_set
#define XedSimpleFlagGetWrittenFlagSet xed_simple_flag_get_written_flag_set
#define XedSimpleFlagGetUndefinedFlagSet xed_simple_flag_get_undefined_flag_set


#define XedEncoderRequestZeroSetMode xed_encoder_request_zero_set_mode
#define XedConvertToEncoderRequest xed_convert_to_encoder_request
#define XedEncode xed_encode
#define XedEncodeNop xed_encode_nop
#define XedInst xed_inst
#define XedInst0 xed_inst0
#define XedInst1 xed_inst1
#define XedInst2 xed_inst2
#define XedInst3 xed_inst3
#define XedInst4 xed_inst4
#define XedInst5 xed_inst5

#define XedRelBr xed_relbr
#define XedReg	xed_reg
#define XedDisp xed_disp
#define XedMemB xed_mem_b
#define XedMemBD xed_mem_bd
#define XedMemBISD xed_mem_bisd
#define XedSimm0 xed_simm0
#define XedImm0 xed_imm0

#define XedPatchRelbr xed_patch_relbr
#define XedPatchDisp xed_patch_disp

inline XED_STATE XedGlobalMachineState;

VOID XedGlobalInit();

PUCHAR XedEncodeInstructions(XED_ENCODER_INSTRUCTION* InstList, UINT32 InstCount, PUINT32 OutSize);

XED_CONDITION_CODE XedConditionCodeJcc(XED_ICLASS_ENUM Jcc);

XED_CONDITION_CODE XedConditionCodeCMOVcc(XED_ICLASS_ENUM Jcc);

XED_ICLASS_ENUM XedJccToCMOVcc(XED_ICLASS_ENUM Jcc);

XED_ICLASS_ENUM XedInvertJcc(XED_ICLASS_ENUM Jcc);

UINT32 XedCalcWidthBits(LONGLONG Displacement);

UINT32 XedSignedDispWidth(LONGLONG Displacement);

#endif