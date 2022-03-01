#ifndef __INST_ROPE_H
#define __INST_ROPE_H

#include "Windas.h"

//https://www.youtube.com/watch?v=W8U77LUdaBM

#define IrGeneralFlag(Index)	(1<<Index)
#define IrSpecificFlag(Index)	(1<<(Index + 16))

#define CODE_FLAG_IS_LABEL		IrGeneralFlag(0)
#define CODE_FLAG_IS_INST		IrGeneralFlag(1)
#define CODE_FLAG_IS_RAW_DATA	IrGeneralFlag(2)
#define CODE_FLAG_GROUP_START	IrGeneralFlag(3)
#define CODE_FLAG_GROUP_END		IrGeneralFlag(4)
#define CODE_FLAG_IN_GROUP		IrGeneralFlag(5)
#define CODE_FLAG_USES_LABEL	IrGeneralFlag(6)

typedef struct _LINK_DATA
{
	union
	{
		struct
		{
			UINT32 IsLabel : 1;
			UINT32 IsInstruction : 1;
			UINT32 GroupStart : 1;
			UINT32 GroupEnd : 1;
			UINT32 InGroup : 1;
		};
		UINT32 Flags;
	};
	INT32 LabelId;
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
}INST_BLOCK, * PINST_BLOCK;

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

//VOID _IrFreeBlock(PINST_BLOCK Block);
//#define IrFreeBlock(Block) _IrFreeBlock((PINST_BLOCK)Block);

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

VOID _IrReplaceBlock2(PINST_BLOCK ParentBlock, PINST_BLOCK Block1, PINST_BLOCK Block2);
#define IrReplaceBlock2(ParentBlock, Block1, Block2) _IrReplaceBlock2((PINST_BLOCK)ParentBlock, (PINST_BLOCK)Block1, (PINST_BLOCK)Block2);

//Rebases a block's labels to start at a certain number.
VOID _IrRebaseLabels(PINST_BLOCK Block, INT32 LabelBase);
#define IrRebaseLabels(Block, LabelBase) _IrRebaseLabels((PINST_BLOCK)Block, LabelBase)

//Assures that there are no conflicting labels in the two blocks.
VOID _IrPrepForMerge(PINST_BLOCK Block1, PINST_BLOCK Block2);
#define IrPrepForMerge(Block1, Block2) _IrPrepForMerge((PINST_BLOCK)Block1, (PINST_BLOCK)Block2)

#endif