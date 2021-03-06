#include "FunctionBlock.h"
#include "Logging.h"
#include "Emulation.h"

PFUNCTION_BLOCK FbCreateTree(PNATIVE_BLOCK CodeBlock)
{
	STDVECTOR<PFUNCTION_BLOCK> FunctionBlocks = { };

	if (!CodeBlock->Front || !CodeBlock->Back)
		return NULL;

	PFUNCTION_BLOCK CurrentBlock = AllocateS(FUNCTION_BLOCK);
	if (!CurrentBlock)
		return NULL;

	CurrentBlock->Block.Front = CodeBlock->Front;
	
	for (PNATIVE_LINK T = CodeBlock->Front; T && T != CodeBlock->Back->Next; T = T->Next)
	{
		if ((T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP) ||
			((T->LinkData.Flags & CODE_FLAG_IS_LABEL) && (T->LinkData.Flags & CODE_FLAG_IS_JUMP_TARGET)))
		{
			PFUNCTION_BLOCK NextBlock = AllocateS(FUNCTION_BLOCK);
			if (!NextBlock)
			{
				for (PFUNCTION_BLOCK Block : FunctionBlocks)
					Free(Block);
				Free(CurrentBlock);
				return NULL;
			}

			if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
			{
				NextBlock->Block.Front = T->Next;
				CurrentBlock->Block.Back = T;
				CurrentBlock->IsConditional = (ULONG64)(!(XED_ICLASS_JMP == XedDecodedInstGetIClass(&T->DecodedInst)));
			}
			else
			{
				NextBlock->Block.Front = T;
				CurrentBlock->Block.Back = T->Prev;
				CurrentBlock->IsConditional = FALSE;
			}

			CurrentBlock->Absolute.NextBlock = NextBlock;
			FunctionBlocks.push_back(CurrentBlock);
			CurrentBlock = NextBlock;
		}
	}

	CurrentBlock->Block.Back = CodeBlock->Back;
	CurrentBlock->IsConditional = FALSE;
	FunctionBlocks.push_back(CurrentBlock);

	//Only one block so we just return it.
	if (FunctionBlocks.size() == 0)
		return CurrentBlock;

	//Otherwise, fix up conditional jumps by searching for where they jump to!
	for (UINT32 i = 0; i < FunctionBlocks.size(); i++)
	{
		if (FunctionBlocks[i]->IsConditional)
		{
			INT32 TargetLabel = FunctionBlocks[i]->Block.Back->LinkData.Id;
			for (INT j = i + 1; j < FunctionBlocks.size(); j++)
			{
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) && 
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.Id)
				{
					FunctionBlocks[i]->Conditional.Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}

			}
			for (INT j = i - 1; j >= 0; j--)
			{
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) &&
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.Id)
				{
					FunctionBlocks[i]->Conditional.Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}
			}
			
			for (PFUNCTION_BLOCK Block : FunctionBlocks)
				Free(Block);
			MLog("Failed to find Taken branch of relative jump.\n");
			return NULL;
		}
	ContinueToNextBlock:
		continue;
	}

	/*for (UINT32 i = 0; i < FunctionBlocks.size(); i++)
	{
		printf("Block: %d\n", i);
		NrDebugPrintIClass(&FunctionBlocks[i]->Block);
		printf("\n\n");
	}*/

	return FunctionBlocks[0];
}

BOOLEAN FbMakeFunctionBlockPositionIndependent(PFUNCTION_BLOCK FunctionBlock, PLABEL_MANAGER LabelManager)
{
	if (FunctionBlock->IsConditional)
	{
		//Append a jump at end of this block, and a label at the start of the not taken branch.

		PFUNCTION_BLOCK NextBlock = FunctionBlock->Conditional.NotTaken;
		INT32 JumpLabelId = 0;
		PNATIVE_LINK LabelLink = NULL;
		if (NextBlock->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL)
		{
			JumpLabelId = NextBlock->Block.Front->LinkData.Id;
		}
		else
		{
			LabelLink = NrAllocateLink();
			if (!LabelLink)
				return FALSE;
			JumpLabelId = LmPeekNextId(LabelManager);
			NrInitForLabel(LabelLink, JumpLabelId, NULL, NULL);
		}

		PNATIVE_LINK JumpLink = EmUnConditionalBranch(JumpLabelId, 32);
		if (!JumpLink)
		{
			if (!LabelLink)
				NrFreeLink(LabelLink);
			return FALSE;
		}

		IrPutLinkBack(&FunctionBlock->Block, JumpLink);
		if (LabelLink)
		{
			IrPutLinkFront(NextBlock, LabelLink);
			LmNextId(LabelManager);
		}

	}
	else if (FunctionBlock->Block.Back->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
	{
		//Don't need to do anything.
		return TRUE;
	}
	else if (FunctionBlock->Absolute.NextBlock)
	{
		//This is when a block ends because there is a label, simply append jump which has target of the label at start of next block.

		PNATIVE_LINK LabelLink = FunctionBlock->Absolute.NextBlock->Block.Front;
		if (!(LabelLink->LinkData.Flags & CODE_FLAG_IS_LABEL))
			return FALSE;
		

		PNATIVE_LINK JumpLink = EmUnConditionalBranch(LabelLink->LinkData.Id, 32);
		if (!JumpLink)
			return FALSE;

		IrPutLinkBack(FunctionBlock, JumpLink);
	}
}

VOID FbPrintTakenPath(PFUNCTION_BLOCK TreeHead)
{
	while (TreeHead)
	{
		NrDebugPrintIClass(&TreeHead->Block);
		if (TreeHead->IsConditional)
			TreeHead = TreeHead->Conditional.Taken;
		else
			TreeHead = TreeHead->Absolute.NextBlock;
		printf("\n");
	}
}

VOID FbPrintNotTakenPath(PFUNCTION_BLOCK TreeHead)
{
	while (TreeHead)
	{
		NrDebugPrintIClass(&TreeHead->Block);
		TreeHead = TreeHead->Absolute.NextBlock;
		printf("\n");
	}
}

VOID FbFreeTree(PFUNCTION_BLOCK TreeHead)
{
	if (TreeHead)
	{
		FbFreeTree(TreeHead->Absolute.NextBlock);
		Free(TreeHead);
	}
}

