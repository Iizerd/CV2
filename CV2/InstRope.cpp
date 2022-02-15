#include "InstRope.h"

PINST_LINK IrAllocateLink(ULONG LinkSize)
{
	return (PINST_LINK)malloc(LinkSize);
}

VOID _IrFreeLink(PINST_LINK Link)
{
	free(Link);
}

VOID _IrForEachLink(PINST_BLOCK Block, FnForEachCallback Callback, PVOID Context)
{
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		PINST_LINK Next = T->Next;
		Callback(Block, T, Context);
		T = Next;
	}
}

VOID _IrBuildBlock(PINST_LINK Inst, PINST_BLOCK Block)
{
	for (PINST_LINK T = Inst; T != NULL; T = T->Prev)
	{
		if (!T->Prev)
		{
			Block->Front = T;
			break;
		}
	}
	for (PINST_LINK T = Inst; T != NULL; T = T->Next)
	{
		if (!T->Next)
		{
			Block->Back = T;
			break;
		}
	}
}

VOID _IrBuildBlockFromFront(PINST_LINK Inst, PINST_BLOCK Block)
{
	Block->Front = Inst;
	for (PINST_LINK T = Inst; T != NULL; T = T->Next)
	{
		if (!T->Next)
		{
			Block->Back = T;
			break;
		}
	}
}

VOID _IrBuildBlockFromBack(PINST_LINK Inst, PINST_BLOCK Block)
{
	Block->Back = Inst;
	for (PINST_LINK T = Inst; T != NULL; T = T->Prev)
	{
		if (!T->Prev)
		{
			Block->Front = T;
			break;
		}
	}
}

VOID _IrFreeBlock(PINST_BLOCK Block)
{
	_IrForEachLink(Block, [](PINST_BLOCK, PINST_LINK Link, PVOID)
		{
			_IrFreeLink(Link);
		}, NULL);
}

VOID _IrPutLinkBack(PINST_BLOCK Block, PINST_LINK Inst)
{
	if (!((ULONG64)Block->Front | (ULONG64)Block->Back))
	{
		Block->Front = Block->Back = Inst;
	}
	else
	{
		Block->Back->Next = Inst;
		Inst->Prev = Block->Back;
		Block->Back = Inst;
	}
}

VOID _IrPutLinkFront(PINST_BLOCK Block, PINST_LINK Inst)
{
	if (!((ULONG64)Block->Front | (ULONG64)Block->Back))
	{
		Block->Front = Block->Back = Inst;
	}
	else
	{
		Block->Front->Prev = Inst;
		Inst->Next = Block->Front;
		Block->Front = Inst;
	}
}

VOID _IrInsertLinkAfter(PINST_BLOCK ParentBlock, PINST_LINK Inst1, PINST_LINK Inst2)
{
	if (ParentBlock && Inst1 == ParentBlock->Back)
		ParentBlock->Back = Inst2;
	if (Inst1->Next)
		Inst1->Next->Prev = Inst2;
	Inst2->Next = Inst1->Next;
	Inst2->Prev = Inst1;
	Inst1->Next = Inst2;
}

VOID _IrInsertLinkBefore(PINST_BLOCK ParentBlock, PINST_LINK Inst1, PINST_LINK Inst2)
{
	if (ParentBlock && Inst1 == ParentBlock->Front)
		ParentBlock->Front = Inst2;
	if (Inst1->Prev)
		Inst1->Prev->Next = Inst2;
	Inst2->Prev = Inst1->Prev;
	Inst2->Next = Inst1;
	Inst1->Prev = Inst2;
}

VOID _IrPutBlockBack(PINST_BLOCK Block1, PINST_BLOCK Block2)
{
	if (!((ULONG64)Block1->Front | (ULONG64)Block1->Back))
	{
		Block2->Front = Block1->Front;
		Block2->Back = Block1->Back;
	}
	else
	{
		Block2->Front->Prev = Block1->Back;
		Block1->Back->Next = Block2->Front;
		Block1->Back = Block2->Back;
	}
}

VOID _IrPutBlockFront(PINST_BLOCK Block1, PINST_BLOCK Block2)
{
	if (!((ULONG64)Block1->Front | (ULONG64)Block1->Back))
	{
		Block2->Front = Block1->Front;
		Block2->Back = Block1->Back;
	}
	else
	{
		Block2->Back->Next = Block1->Front;
		Block1->Front->Prev = Block2->Back;
		Block1->Front = Block2->Front;
	}
}

VOID _IrInsertBlockAfter(PINST_BLOCK ParentBlock, PINST_LINK Inst, PINST_BLOCK Block)
{
	if (ParentBlock && Inst == ParentBlock->Back)
	{
		return _IrPutBlockBack(ParentBlock, Block);
	}
	else
	{
		if (Inst->Next)
			Inst->Next->Prev = Block->Back;
		Block->Front->Prev = Inst;
		Block->Back->Next = Inst->Next;
		Inst->Next = Block->Front;
	}
}

VOID _IrInsertBlockBefore(PINST_BLOCK ParentBlock, PINST_LINK Inst, PINST_BLOCK Block)
{
	if (ParentBlock && Inst == ParentBlock->Front)
	{
		return _IrPutBlockFront(ParentBlock, Block);
	}
	else
	{
		if (Inst->Prev)
			Inst->Prev->Next = Block->Front;
		Block->Front->Prev = Inst->Prev;
		Block->Back->Next = Inst;
		Inst->Prev = Block->Back;
	}
}

VOID _IrReplaceBlock(PINST_BLOCK ParentBlock, PINST_LINK Start, PINST_LINK End, PINST_BLOCK Block)
{
	INST_BLOCK Block1;
	Block1.Front = Start;
	Block1.Back = End;
	_IrReplaceBlock2(ParentBlock, &Block1, Block);
}

VOID _IrReplaceBlock2(PINST_BLOCK ParentBlock, PINST_BLOCK Block1, PINST_BLOCK Block2)
{
	if (ParentBlock)
	{
		if (ParentBlock->Front == Block1->Front)
			ParentBlock->Front = Block2->Front;
		if (ParentBlock->Back == Block1->Back)
			ParentBlock->Back = Block2->Back;
	}

	if (Block1->Front->Prev)
		Block1->Front->Prev->Next = Block2->Front;
	Block2->Front->Prev = Block1->Front->Prev;

	if (Block1->Back->Next)
		Block1->Back->Next->Prev = Block2->Back;
	Block2->Back->Next = Block1->Back->Next;

	_IrFreeBlock(Block1);
}