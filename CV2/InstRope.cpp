#include "InstRope.h"

VOID _IrForEachLink(PINST_BLOCK Block, FnForEachCallback Callback)
{
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		PINST_LINK Next = T->Next;
		Callback(T);
		T = Next;
	}
}

VOID _IrForEachLinkEx(PINST_BLOCK Block, FnForEachCallbackEx Callback, PVOID Context)
{
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next;)
	{
		PINST_LINK Next = T->Next;
		Callback(Block, T, Context);
		T = Next;
	}
}

UINT32 _IrCountLinks(PINST_BLOCK Block)
{
	UINT32 Count = 0;
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
		Count++;
	return Count;
}

PINST_LINK _IrTraceToGroupEnd(PINST_LINK GroupStart, PINST_LINK TraceStop)
{
	ULONG Balance = 0;
	for (PINST_LINK T = GroupStart; T && T != TraceStop->Next; T = T->Next)
	{
		if (T->LinkData.Flags & CODE_FLAG_GROUP_START)
			++Balance;
		else if (T->LinkData.Flags & CODE_FLAG_GROUP_END)
			--Balance;
		else if (Balance == 0)
			return T;
	}
	return NULL;
}

VOID _IrBuildBlock(PINST_LINK Inst, PINST_BLOCK Block)
{
	for (PINST_LINK T = Inst; T; T = T->Prev)
	{
		if (!T->Prev)
		{
			Block->Front = T;
			break;
		}
	}
	for (PINST_LINK T = Inst; T; T = T->Next)
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

VOID _IrPutLinkBack(PINST_BLOCK Block, PINST_LINK Inst)
{
	if (!((UINT64)Block->Front | (UINT64)Block->Back))
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
	if (!((UINT64)Block->Front | (UINT64)Block->Back))
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
	if (!((UINT64)Block1->Front | (UINT64)Block1->Back)) //Makes me think im lackin
	{
		Block1->Front = Block2->Front;
		Block1->Back = Block2->Back;
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
	if (!((UINT64)Block1->Front | (UINT64)Block1->Back))
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
	if (ParentBlock)
	{
		if (ParentBlock->Front == Start)
			ParentBlock->Front = Block->Front;
		if (ParentBlock->Back == End)
			ParentBlock->Back = Block->Back;
	}

	if (Start->Prev)
		Start->Prev->Next = Block->Front;
	Block->Front->Prev = Start->Prev;

	if (End->Next)
		End->Next->Prev = Block->Back;
	Block->Back->Next = End->Next;
}

VOID _IrReplaceBlock2(PINST_BLOCK ParentBlock, PINST_BLOCK Block1, PINST_BLOCK Block2)
{
	return _IrReplaceBlock(ParentBlock, Block1->Front, Block1->Back, Block2);
}

VOID _IrReplaceLinkWithBlock(PINST_BLOCK ParentBlock, PINST_LINK Link, PINST_BLOCK Block)
{
	return _IrReplaceBlock(ParentBlock, Link, Link, Block);
}

PINST_LINK* _IrEnumerateBlock(PINST_BLOCK Block, UINT32 Count)
{
	PINST_LINK* EnumArray = (PINST_LINK*)calloc(Count, sizeof(PVOID));
	if (!EnumArray)
		return NULL;
	UINT32 Idx = 0;
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next && Idx < Count; T = T->Next)
		EnumArray[Idx++] = T;
	return EnumArray;
}

BOOLEAN _IrGetMinId(PINST_BLOCK Block, PINT32 Id)
{
	BOOLEAN FoundFirstId = FALSE;
	INT32 LowestId = 0;
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & (CODE_FLAG_IS_LABEL))
		{
			if (FALSE == FoundFirstId)
			{
				LowestId = T->LinkData.Id;
				FoundFirstId = TRUE;
			}
			else if (T->LinkData.Id < LowestId)
				LowestId = T->LinkData.Id;
		}
	}
	*Id = LowestId;
	return FoundFirstId;
}

BOOLEAN _IrGetMaxId(PINST_BLOCK Block, PINT32 Id)
{
	BOOLEAN FoundFirstId = FALSE;
	INT32 HighestId = 0;
	for (PINST_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & (CODE_FLAG_IS_LABEL))
		{
			if (FALSE == FoundFirstId)
			{
				HighestId = T->LinkData.Id;
				FoundFirstId = TRUE;
			}
			else if (T->LinkData.Id > HighestId)
				HighestId = T->LinkData.Id;
		}
	}
	*Id = HighestId;
	return FoundFirstId;
}

VOID _IrRebaseIds(PINST_BLOCK Block, INT32 IdBase)
{
	INT32 LowestId = 0;
	if (_IrGetMinId(Block, &LowestId))
	{
		INT32 IdDelta = IdBase - LowestId;
		for (PINST_LINK T = Block->Front; T && T != Block->Back->Next; T = T->Next)
		{
			if (T->LinkData.Flags & (CODE_FLAG_IS_LABEL | CODE_FLAG_USES_LABEL))
				T->LinkData.Id += IdDelta;
		}
	}
}

VOID _IrPrepForMerge(PINST_BLOCK Block1, PINST_BLOCK Block2)
{
	INT32 HighestId = 0;
	if (_IrGetMaxId(Block1, &HighestId))
		_IrRebaseIds(Block2, HighestId + 1);
}
