#include "FunctionBlock.h"
#include "Logging.h"

PFUNCTION_BLOCK FbCreateTree(PNATIVE_BLOCK CodeBlock)
{
	STDVECTOR<PFUNCTION_BLOCK> FunctionBlocks = { };

	if (!CodeBlock->Front || !CodeBlock->Back)
		return NULL;

	PFUNCTION_BLOCK CurrentBlock = AllocateS(FUNCTION_BLOCK);
	if (!CurrentBlock)
	{
		MLog("Could not allocate memory for first function block.\n");
		return NULL;
	}
	CurrentBlock->Block.Front = CodeBlock->Front;
	
	for (PNATIVE_LINK T = CodeBlock->Front; T && T != CodeBlock->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & (CODE_FLAG_IS_LABEL | CODE_FLAG_IS_REL_JUMP))
		{
			PFUNCTION_BLOCK NextBlock = AllocateS(FUNCTION_BLOCK);
			if (!NextBlock)
			{
				MLog("Could not allocate memory for next function block.\n");
				for (PFUNCTION_BLOCK Block : FunctionBlocks)
					Free(Block);
				Free(CurrentBlock);
				return NULL;
			}

			if (T->LinkData.Flags & CODE_FLAG_IS_REL_JUMP)
			{
				NextBlock->Block.Front = T->Next;
				CurrentBlock->Block.Back = T;
				CurrentBlock->Conditional = (ULONG64)(!(XED_ICLASS_JMP == XedDecodedInstGetIClass(&T->DecodedInst)));
			}
			else
			{
				NextBlock->Block.Front = T;
				CurrentBlock->Block.Back = T->Prev;
				CurrentBlock->Conditional = FALSE;
			}

			CurrentBlock->NotTaken = NextBlock;
			FunctionBlocks.push_back(CurrentBlock);
			CurrentBlock = NextBlock;
		}
	}

	CurrentBlock->Block.Back = CodeBlock->Back;
	CurrentBlock->Conditional = FALSE;
	FunctionBlocks.push_back(CurrentBlock);

	//Only one block so we just return it.
	if (FunctionBlocks.size() == 0)
		return CurrentBlock;

	//Otherwise, fix up conditional jumps by searching for where they jump to!
	for (UINT i = 0; i < FunctionBlocks.size(); i++)
	{
		if (FunctionBlocks[i]->Conditional)
		{
			INT32 TargetLabel = FunctionBlocks[i]->Block.Back->LinkData.Id;
			for (INT j = i + 1; j < FunctionBlocks.size(); j++)
			{
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) && 
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.Id)
				{
					FunctionBlocks[i]->Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}

			}
			for (INT j = i - 1; j >= 0; j--)
			{
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) &&
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.Id)
				{
					FunctionBlocks[i]->Taken = FunctionBlocks[j];
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

	/*for (UINT i = 0; i < FunctionBlocks.size(); i++)
	{
		printf("Block: %d\n", i);
		NrDebugPrintIClass(&FunctionBlocks[i]->Block);
		printf("\n\n");
	}*/

	return FunctionBlocks[0];
}

VOID FbPrintTakenPath(PFUNCTION_BLOCK TreeHead)
{
	while (TreeHead)
	{
		NrDebugPrintIClass(&TreeHead->Block);
		if (TreeHead->Conditional)
			TreeHead = TreeHead->Taken;
		else
			TreeHead = TreeHead->NotTaken;
		printf("\n");
	}
}

VOID FbPrintNotTakenPath(PFUNCTION_BLOCK TreeHead)
{
	while (TreeHead)
	{
		NrDebugPrintIClass(&TreeHead->Block);
		TreeHead = TreeHead->NotTaken;
		printf("\n");
	}
}

VOID FbFreeTree(PFUNCTION_BLOCK TreeHead)
{
	if (TreeHead)
	{
		FbFreeTree(TreeHead->NotTaken);
		Free(TreeHead);
	}
}
