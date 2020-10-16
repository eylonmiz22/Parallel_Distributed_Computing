#include <omp.h>
#include "Header.h"
// Name	:	Eylon Mizrahi
// ID	:	206125411
float computeScore(float w[], char* signs)
{
//	Given similarity string: :::**.:**..:.* compute its score

	int matches = 0, conservatives = 0, semiConservatives = 0, miss = 0, i, k;
	k = (int)strlen(signs);
	char currentChar;
	#pragma omp parallel for private(currentChar) reduction(+: matches, conservatives, semiConservatives, miss)
	for(i=0; i < k; i++)
	{
		currentChar = signs[i];
		if(currentChar == '*')
			matches ++;
		else if(currentChar == ':')
			conservatives ++;
		else if(currentChar == '.')
			semiConservatives ++;
		else if(currentChar == ' ')
			miss ++;
	}
	float result = w[0] * matches - w[1] * conservatives - w[2] * semiConservatives - w[3] * miss;
	return result;
}
void setIterationInfo(IterationInfo* info, int rank, int size, int offset, int mutationIndex)
{
	// Setter
	info->rank = rank;
	info->size = size;
	info->offset = offset;
	info->mutationIndex = mutationIndex;
}
void setBestStats(BestStats* stats, float bestScore, int bestOffset, int bestMutation)
{
	// Setter
	stats->bestScore = bestScore;
	stats->bestMutation = bestMutation;
	stats->bestOffset = bestOffset;
}
void updateBestStats(BestStats* stats, float currentScore, int currentOffset, int currentMutation)
{
	if (currentScore > stats->bestScore)
	{
		setBestStats(stats, currentScore, currentOffset, currentMutation);
	}
}
void printBestStats(int sequenceID, BestStats* stats)
{
	printf("\n\t\t\tSEQUENCE NUMBER %d", sequenceID);
	printf("\n\t\t\tBEST SCORE = %f", stats->bestScore);
	printf("\n\t\t\tBEST MUTATAION = %d", stats->bestMutation);
	printf("\n\t\t\tBEST OFFSET = %d\n", stats->bestOffset);
}

void printTime(int iteration, int targetIteration, double t1, double t2)
{
	if (iteration == targetIteration)
		printf("Time Elapsed for the Parallel Algorithm: %lf\n", t2 - t1);
}


void findBestCombination(IterationInfo* info, BestStats* stats, Sequence* currentSequence, char* originalSigns, char* seq1, char* seq2, char* signs, float w[])
{
//	Given a current Mutation Index, Iterate over the relational share of the current proccesses offsets and compute the best offset for that subgroup
	int n;
	for (n = ((info->rank * info->offset)/ info->size); n < (((info->rank + 1)* info->offset)/ info->size); n++)
	{
		GPU_Create_Signs(currentSequence, n, originalSigns, info->mutationIndex, seq1, seq2, signs);
		updateBestStats(stats, computeScore(w, originalSigns), n, info->mutationIndex);
	}
}
void writeResults(FILE* f, int sequenceID, int bestOffset, int bestMutation, float bestScore)
{
//	Given the file f is already open, write one line of the optimal combination regarding one minor sequence of the input
	if (!f)
	{
		printf("FILE IS NOT OPEN\n");
		fflush(stdout);
	}
	fprintf(f, "ID = %d\t Best Offset = %d\t Best Mutation = %d\t Score = %f\n", sequenceID, bestOffset, bestMutation, bestScore);


}

void readAllSequences(MainSequence* ms, Sequence*** sequences, int* sequenceCount)
{
//	Reads the input into data structures: 
//	MainSequence will save the main string and the weights
//	Sequence will save a single minor sequence.

	FILE *f;
	f = fopen(INPUT_FILE_PATH, "r");
	
	if (!f)
	{
		printf("Unable to open file2!\n");
		fflush(stdout);
	}
	
	fscanf(f,"%f %f %f %f", &ms->w[0], &ms->w[1], &ms->w[2], &ms->w[3]);	// Read weights
	fscanf(f, "%s", ms->letters);											// Read the Main String Sequence
	fscanf(f,"%d", sequenceCount);											// Number of minor Sequences
	ms->length = strlen(ms->letters);

	
	*sequences = (Sequence**)malloc((*sequenceCount) * sizeof(Sequence*));	// Alocate memory for the NSQ2 sequences
	int i;
	for(i = 0; i<(*sequenceCount); i++)										// Read each Sequence into the Sequence array
	{
		(*sequences)[i] = (Sequence*)malloc(sizeof(Sequence));
		fscanf(f, "%s", (*sequences)[i]->letters);
		(*sequences)[i]->length = (int)strlen((*sequences)[i]->letters);
	}


	fclose(f);
}

