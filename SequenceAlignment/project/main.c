#include <mpi.h>
#include "Header.h"

// Name	:	Eylon Mizrahi
// ID	:	206125411 

int main(int argc, char *argv[]) {
	// MPI variables
	int rank, size;
	MPI_Status  status;


	// Loop variables and scores
	int i, m;
	int mutationIndex, offset;
	
	// Time Calculation
	double t1, t2;

	// CUDA variables
	char *signs, *seq1, *seq2;
	char* originalSigns;


	// INPUT from file
	MainSequence ms;
	Sequence** sequences;
	int sequenceCount;
	Sequence* currentSequence;
	FILE* f;
	

	// MPI Setup
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	t1 = MPI_Wtime();

	// All Processes Read the input
	readAllSequences(&ms, &sequences, &sequenceCount);
	


	// Helper Structs
	IterationInfo info;
	BestStats stats;

	if (rank == 0)
		f = fopen(OUTPUT_FILE_PATH, "w+"); // file for output

	// for each sequence in the input compute the optimal offset and mutation
	for(i = 0; i < sequenceCount; i++)
	{
		// Barrier to Reset Proccess Behavior
		MPI_Barrier(MPI_COMM_WORLD);


		currentSequence = sequences[i];
		offset = ms.length - currentSequence->length + 1; // number of possible offsets
		originalSigns = (char*) malloc (currentSequence->length * sizeof(char));



		setBestStats(&stats, -ms.w[3] * MAX_SIZE2, -1, -1);								// minimum value (if they all miss) for the algorithm iteration reset
		allocateCudaMemory(&seq1, &seq2, &signs, ms.length, currentSequence->length); 	//alocation for the CUDA variables.
		copyInformationToCuda(seq1, seq2, &ms, currentSequence); 						// copy memory of sequences from ms and currentSequence to seq1 and seq2 on CUDA!

		// Every proccess calculates its share of the computations offset wise and includes all combinations for its share.
		// MPI divides the job into proccesses. Each process uses threads with OMP, each thread will call CUDA to compute the similarity String.

		for (mutationIndex = 1; mutationIndex < currentSequence->length; mutationIndex++)
		{
			setIterationInfo(&info, rank, size, offset, mutationIndex);
			findBestCombination(&info, &stats, currentSequence, originalSigns, seq1, seq2, signs, ms.w);
		}
		// now stats has the best stats for this proccess
			
		if(rank == 0) 	
		{
			float recievedScore;
			int recievedOffset;
			int recievedMutation;
			for(m=1; m < size; m++)
			{			
				// compare best score of proccess 0 with best score of procces k
				MPI_Recv (&recievedScore, 1, MPI_FLOAT, m, 0, MPI_COMM_WORLD, &status);
				MPI_Recv (&recievedOffset, 1, MPI_INT, m, 0, MPI_COMM_WORLD, &status);
				MPI_Recv (&recievedMutation, 1, MPI_INT, m, 0, MPI_COMM_WORLD, &status);

				// If the recieved score is better then update the best stats struct
				updateBestStats(&stats, recievedScore, recievedOffset, recievedMutation);
			
			}
			printBestStats(i, &stats);
			t2 = MPI_Wtime();
			printTime(i, sequenceCount - 1, t1, t2);
			writeResults(f, i, stats.bestOffset, stats.bestMutation, stats.bestScore);
		}	
		else	
		{									
			// send best info to proccess 0 to compare best score of proccess 0 with best score of procces k
			MPI_Send(&(stats.bestScore), 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
			MPI_Send(&(stats.bestOffset), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
			MPI_Send(&(stats.bestMutation), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

		}
		// free current iterations memory
		free(originalSigns);
		freeCudaMemory(seq1, seq2, signs);

		
		
	}
	// free all sequences
	free(sequences);

	// Close output only for proccess 0 that opened it.
	if (rank == 0)
		fclose(f);

    MPI_Finalize();
	return EXIT_SUCCESS;
}


