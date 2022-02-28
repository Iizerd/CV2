#include "FunctionBlock.h"
#include "Logging.h"

/*
* Function Block:
*	- MUST start with label.		unless start of function.
*	- MUST end with relative jump.  unless end of function.
*/

PFUNCTION_BLOCK FbCreateTree(PNATIVE_BLOCK CodeBlock)
{
	STDVECTOR<PFUNCTION_BLOCK> FunctionBlocks = { };

	if (!CodeBlock->Front || !CodeBlock->Back)
		return NULL;

	PFUNCTION_BLOCK CurrentBlock = (PFUNCTION_BLOCK)malloc(sizeof(FUNCTION_BLOCK));
	if (!CurrentBlock)
	{
		MLog("Could not allocate memory for first function block.\n");
		return NULL;
	}
	RtlZeroMemory(CurrentBlock, sizeof(FUNCTION_BLOCK));
	CurrentBlock->Block.Front = CodeBlock->Front;
	
	for (PNATIVE_LINK T = CodeBlock->Front; T && T != CodeBlock->Back->Next; T = T->Next)
	{
		if (T->LinkData.Flags & (CODE_FLAG_IS_LABEL | CODE_FLAG_IS_REL_JUMP))
		{
			PFUNCTION_BLOCK NextBlock = (PFUNCTION_BLOCK)malloc(sizeof(FUNCTION_BLOCK));
			if (!NextBlock)
			{
				MLog("Could not allocate memory for next function block.\n");
				for (PFUNCTION_BLOCK Block : FunctionBlocks)
					free(Block);
				free(CurrentBlock);
				return NULL;
			}
			RtlZeroMemory(NextBlock, sizeof(FUNCTION_BLOCK));

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

	for (UINT i = 0; i < FunctionBlocks.size(); i++)
	{
		if (FunctionBlocks[i]->Conditional)
		{
			INT32 TargetLabel = FunctionBlocks[i]->Block.Back->LinkData.LabelId;
			for (INT j = i + 1; j < FunctionBlocks.size(); j++)
			{
				//Search forward.
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) && 
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.LabelId)
				{
					FunctionBlocks[i]->Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}

			}
			for (INT j = i - 1; j >= 0; j--)
			{
				//Search backwards.
				if ((FunctionBlocks[j]->Block.Front->LinkData.Flags & CODE_FLAG_IS_LABEL) &&
					TargetLabel == FunctionBlocks[j]->Block.Front->LinkData.LabelId)
				{
					FunctionBlocks[i]->Taken = FunctionBlocks[j];
					goto ContinueToNextBlock;
				}
			}
			
			for (PFUNCTION_BLOCK Block : FunctionBlocks)
				free(Block);
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

VOID FbPrintTakenPath(PFUNCTION_BLOCK Start)
{
	while (Start)
	{
		NrDebugPrintIClass(&Start->Block);
		if (Start->Conditional)
			Start = Start->Taken;
		else
			Start = Start->NotTaken;
		printf("\n");
	}
}
VOID FbPrintNotTakenPath(PFUNCTION_BLOCK Start)
{
	while (Start)
	{
		NrDebugPrintIClass(&Start->Block);
		Start = Start->NotTaken;
		printf("\n");
	}
}
VOID FbFreeTree(PFUNCTION_BLOCK Start)
{
	if (Start)
	{
		FbFreeTree(Start->NotTaken);
		free(Start);
	}
}