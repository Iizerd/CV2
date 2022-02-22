#ifndef __VM_STRUCTURE_H
#define __VM_STRUCTURE_H

#include "Windas.h"
#include "XedWrap.h"

#define VM_OPHANDLER_STORE	0
#define VM_OPHANDLER_LOAD	1
#define VM_OPHANDLER_COUNT	2

#define VM_IREG_1		0
#define VM_IREG_2		1
#define VM_IREG_3		2
#define VM_IREG_COUNT	3

#define VM_OPSIZE_8		0
#define VM_OPSIZE_16	1
#define VM_OPSIZE_32	2
#define VM_OPSIZE_64	3
#define VM_OPSIZE_COUNT	4

#define VM_MEMOP_B				0
#define VM_MEMOP_BD				1
#define VM_MEMOP_BIS			2
#define VM_MEMOP_BISD			3
#define VM_MEMOP_TYPE_COUNT		4

#define VM_HEADER			XED_REG_RDI
#define VM_HANDLER_TABLE	XED_REG_RSI
#define VM_IP				XED_REG_RDX
#define VM_REGISTER_FILE	XED_REG_RBP
#define VM_FLAG_STORAGE		XED_REG_RSP

#endif