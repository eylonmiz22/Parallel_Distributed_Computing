#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Name	:	Eylon Mizrahi
// ID	:	206125411


//---------------------------------- Defines ---------------------------------------------------
#define INPUT_FILE_PATH "./input.txt"
#define OUTPUT_FILE_PATH "./output.txt"
#define MAX_SIZE1 3000
#define MAX_SIZE2 2000



//---------------------------------- Structs---------------------------------------------------

typedef struct MainSequence
{
	int length;
	char letters[MAX_SIZE1];
	float w[4];
} MainSequence;

typedef struct Sequence
{
	int length;
	char letters[MAX_SIZE2];
} Sequence;

typedef struct IterationInfo
{
	int rank;
	int size;
	int offset;
	int mutationIndex;
} IterationInfo;

typedef struct BestStats
{
	float bestScore;
	int bestOffset;
	int bestMutation;
} BestStats;




//---------------------------------- Method Declerations ---------------------------------------------------




//	Input/Output
void readAllSequences(MainSequence* ms, Sequence*** sequences, int* sequenceCount);
void writeResults(FILE* f, int sequenceID, int bestOffset, int bestMutation, float bestScore);

// 	CPU
float computeScore(float w[], char* signs);
void setIterationInfo(IterationInfo* info, int rank, int size, int offset, int mutationIndex);
void setBestStats(BestStats* stats, float bestScore, int bestOffset, int bestMutation);
void updateBestStats(BestStats* stats, float currentScore, int currentOffset, int currentMutation);
void printBestStats(int sequenceID, BestStats* stats);
void printTime(int iteration, int targetIteration, double t1, double t2);
void findBestCombination(IterationInfo* info, BestStats* stats, Sequence* currentSequence, char* originalSigns, char* seq1, char* seq2, char* signs, float w[]);


//	GPU
int GPU_Create_Signs(Sequence* s, int n, char* originalSigns, int mutationIndex, char* seq1, char* seq2, char* signs);
void allocateCudaMemory(char** seq1, char** seq2, char** signs, int msLength, int sLength);
void copyInformationToCuda(char* seq1, char* seq2, MainSequence* ms, Sequence* s);
void freeCudaMemory(char* seq1, char* seq2, char* signs);
