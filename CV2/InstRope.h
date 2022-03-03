#ifndef __INST_ROPE_H
#define __INST_ROPE_H

#include "Windas.h"

//https://www.youtube.com/watch?v=W8U77LUdaBM

#define IrGeneralFlag(Index)	(1<<Index)
#define IrSpecificFlag(Index)	(1<<(Index + 16))


#define CODE_FLAG_IS_INST		IrGeneralFlag(0)
#define CODE_FLAG_IS_LABEL		IrGeneralFlag(1) //Subtle difference between label and marker. Labels are targets for jumps and act as block breaks.
#define CODE_FLAG_IS_MARKER		IrGeneralFlag(2) //Markers are for rip relative instructions. This will make parsing function blocks faster much. Otherwise would have to make sure the Label is referenced by a jump somewhere.
#define CODE_FLAG_IS_RAW_DATA	IrGeneralFlag(3)
#define CODE_FLAG_GROUP_START	IrGeneralFlag(4) //Specifies start of a group
#define CODE_FLAG_GROUP_END		IrGeneralFlag(5) //End of a group
#define CODE_FLAG_IN_GROUP		IrGeneralFlag(6) //Everything between and including start/end of a group have this. Must all be treated as an inseparable block.
#define CODE_FLAG_USES_LABEL	IrGeneralFlag(7)
#define CODE_FLAG_USES_MARKER	IrGeneralFlag(8)

#define CODE_FLAG_OCCUPIES_SPACE	(CODE_FLAG_IS_INST | CODE_FLAG_IS_RAW_DATA) //Physically exists. RawDataSize and RawData are valid.

typedef struct _LINK_DATA
{
	union
	{
		struct
		{
			UINT32 IsInstruction : 1;
			UINT32 IsJmpTarget : 1;
			UINT32 IsMarker : 1;
			UINT32 IsRawData : 1;
			UINT32 GroupStart : 1;
			UINT32 GroupEnd : 1;
			UINT32 InGroup : 1;
			UINT32 UsesLabel : 1;
			UINT32 UsesMarker : 1;
		};
		UINT32 Flags;
	};
	INT32 Id;
}LINK_DATA, * PLINK_DATA; STATIC_ASSERT(sizeof(LINK_DATA) == 8, "Bad LINK_DATA size.");

typedef struct _INST_LINK
{
	_INST_LINK*		Next;
	_INST_LINK*		Prev;
	LINK_DATA		LinkData;
}INST_LINK, * PINST_LINK; STATIC_ASSERT(sizeof(INST_LINK) == 24, "Bad INST_LINK size.");

typedef struct _INST_BLOCK
{
	PINST_LINK		Front;
	PINST_LINK		Back;
}INST_BLOCK, * PINST_BLOCK; STATIC_ASSERT(sizeof(INST_BLOCK) == 16, "Bad INST_BLOCK size.");

typedef VOID(*FnForEachCallback)(PINST_LINK);
VOID _IrForEachLink(PINST_BLOCK Block, FnForEachCallback Callback);
#define IrForEachLink(Block, Callback) _IrForEachLink((PINST_BLOCK)Block, (FnForEachCallback)Callback);

typedef VOID(*FnForEachCallbackEx)(PINST_BLOCK, PINST_LINK, PVOID);
VOID _IrForEachLinkEx(PINST_BLOCK Block, FnForEachCallbackEx Callback, PVOID Context);
#define IrForEachLinkEx(Block, Callback, Context) _IrForEachLinkEx((PINST_BLOCK)Block, (FnForEachCallbackEx)Callback, (PVOID)Context);

UINT _IrCountLinks(PINST_BLOCK Block);
#define IrCountLinks(Block) _IrCountLinks((PINST_BLOCK)Block);

VOID _IrBuildBlock(PINST_LINK Inst, PINST_BLOCK Block);
#define IrBuildBlock(Inst, Block) _IrBuildBlock((PINST_LINK)Inst, (PINST_BLOCK)Block);

VOID _IrBuildBlockFromFront(PINST_LINK Inst, PINST_BLOCK Block);
#define IrBuildBlockFromFront(Inst, Block) _IrBuildBlockFromFront((PINST_LINK)Inst, (PINST_BLOCK)Block);

VOID _IrBuildBlockFromBack(PINST_LINK Inst, PINST_BLOCK Block);
#define IrBuildBlockFromBack(Inst, Block) _IrBuildBlockFromBack((PINST_LINK)Inst, (PINST_BLOCK)Block);

VOID _IrPutLinkBack(PINST_BLOCK Block, PINST_LINK Inst);
#define IrPutLinkBack(Block, Inst) _IrPutLinkBack((PINST_BLOCK)Block, (PINST_LINK)Inst);

VOID _IrPutLinkFront(PINST_BLOCK Block, PINST_LINK Inst);
#define IrPutLinkFront(Block, Inst) _IrPutLinkFront((PINST_BLOCK)Block, (PINST_LINK)Inst);

VOID _IrInsertLinkAfter(PINST_BLOCK ParentBlock, PINST_LINK Inst1, PINST_LINK Inst2);
#define IrInsertLinkAfter(ParentBlock, Inst1, Inst2) _IrInsertLinkAfter((PINST_BLOCK)ParentBlock, (PINST_LINK)Inst1, (PINST_LINK)Inst2);

VOID _IrInsertLinkBefore(PINST_BLOCK ParentBlock, PINST_LINK Inst1, PINST_LINK Inst2);
#define IrInsertLinkBefore(ParentBlock, Inst1, Inst2) _IrInsertLinkBefore((PINST_BLOCK)ParentBlock, (PINST_LINK)Inst1, (PINST_LINK)Inst2);

VOID _IrPutBlockBack(PINST_BLOCK Block1, PINST_BLOCK Block2);
#define IrPutBlockBack(Block1, Block2) _IrPutBlockBack((PINST_BLOCK)Block1, (PINST_BLOCK)Block2);

VOID _IrPutBlockFront(PINST_BLOCK Block1, PINST_BLOCK Block2);
#define IrPutBlockFront(Block1, Block2) _IrPutBlockFront((PINST_BLOCK)Block1, (PINST_BLOCK)Block2);

VOID _IrInsertBlockAfter(PINST_BLOCK ParentBlock, PINST_LINK Inst, PINST_BLOCK Block);
#define IrInsertBlockAfter(ParentBlock, Inst, Block) _IrInsertBlockAfter((PINST_BLOCK)ParentBlock, (PINST_LINK)Inst, (PINST_BLOCK)Block);

VOID _IrInsertBlockBefore(PINST_BLOCK ParentBlock, PINST_LINK Inst, PINST_BLOCK Block);
#define IrInsertBlockBefore(ParentBlock, Inst, Block) _IrInsertBlockBefore((PINST_BLOCK)ParentBlock, (PINST_LINK)Inst, (PINST_BLOCK)Block);

VOID _IrReplaceBlock(PINST_BLOCK ParentBlock, PINST_LINK Start, PINST_LINK End, PINST_BLOCK Block);
#define IrReplaceBlock(ParentBlock, Start, End, Block) _IrReplaceBlock((PINST_BLOCK)ParentBlock, (PINST_LINK)Start, (PINST_LINK)End, (PINST_BLOCK)Block);

//First block is replaced with the second.
VOID _IrReplaceBlock2(PINST_BLOCK ParentBlock, PINST_BLOCK Block1, PINST_BLOCK Block2);
#define IrReplaceBlock2(ParentBlock, Block1, Block2) _IrReplaceBlock2((PINST_BLOCK)ParentBlock, (PINST_BLOCK)Block1, (PINST_BLOCK)Block2);

VOID _IrReplaceLinkWithBlock(PINST_BLOCK ParentBlock, PINST_LINK Link, PINST_BLOCK Block);
#define IrReplaceLinkWithBlock(ParentBlock, Link, Block) _IrReplaceLinkWithBlock((PINST_BLOCK)ParentBlock, (PINST_LINK)Link, (PINST_BLOCK)Block);

BOOLEAN _IrGetMinId(PINST_BLOCK Block, PINT32 Id);
#define IrGetMinId(Block, Id) _IrGetMinId((PINST_BLOCK)Block, Id);

BOOLEAN _IrGetMaxId(PINST_BLOCK Block, PINT32 Id);
#define IrGetMaxId(Block, Id) _IrGetMaxId((PINST_BLOCK)Block, Id);

//Rebases a block's labels to start at a certain number.
VOID _IrRebaseIds(PINST_BLOCK Block, INT32 LabelBase);
#define IrRebaseIds(Block, LabelBase) _IrRebaseIds((PINST_BLOCK)Block, LabelBase)

//Assures that there are no conflicting labels in the two blocks.
VOID _IrPrepForMerge(PINST_BLOCK Block1, PINST_BLOCK Block2);
#define IrPrepForMerge(Block1, Block2) _IrPrepForMerge((PINST_BLOCK)Block1, (PINST_BLOCK)Block2)

#endif