#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "mpi.h"

#define HEAVY 100000
#define SHORT 1
#define LONG 10
#define N 20
#define MASTER 0
#define DATA_TAG 0
#define TERMINATION_TAG 1


double heavy(int x, int y);
double staticFunc(int id, int numProcs);
double calculate(int id, int numProcs);
double calculateRest(int numProcs);
double dynamicFunc(int id, int numProcs, MPI_Status *status);
double masterTask(int numProcs, MPI_Status *status);
void slaveTask(int id, MPI_Status *status);


// This function performs heavy computations,
// its run time depends on x and y values
double heavy(int x, int y)
{
	int i, loop = SHORT;
	double sum = 0;

	// Super heavy tasks
	 if (x < 3 || y < 3)
		loop = LONG;
	 // Heavy calculations
	 for(i = 0;  i < loop*HEAVY;  i++)
		sum += cos(exp(sin((double)i/HEAVY)));

     	return sum;
}

double staticFunc(int id, int numProcs)
{
	double *results, myResult, restResult, sum = 0;

	if (id == MASTER)
	{
		results = (double*) malloc(numProcs * sizeof(double));
		restResult = calculateRest(numProcs);
	}
	
	myResult = calculate(id, numProcs);
	MPI_Gather(&myResult, 1, MPI_DOUBLE, results, 1, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);
	
	if (id == MASTER)
	{
		int i;
		for (i = 0; i < numProcs; i++)
		{
			sum += results[i];
		}
		sum += restResult;

		free(results);
	}

	return sum;
}

double calculate(int id, int numProcs)
{
	int numTasks = N / numProcs;
	int start = id * numTasks;
	int end = start + numTasks;
	int x, y;

	double result = 0;
	
	for (x = start; x < end; x++)
		for (y = 0; y < N; y++)
			result += heavy(x, y);

	return result;
}

double calculateRest(int numProcs)
{
	int restTasks = N % numProcs;
	int start = N - restTasks;
	int end = N;
	int x, y;

	double result = 0;
	
	for (x = start; x < end; x++)
		for (y = 0; y < N; y++)
			result += heavy(x, y);

	return result;
}

double dynamicFunc(int id, int numProcs, MPI_Status *status)
{
	if (id != 0)
	{
		slaveTask(id, status);
	}

	return masterTask(numProcs, status);
}

double masterTask(int numProcs, MPI_Status *status)
{
	int x = 0, tag = DATA_TAG, slaveid, numslaves = numProcs - 1;
	double sum = 0, result;

	for (slaveid = 1; slaveid <= numslaves; slaveid++)
	{
		if (N - x <= numslaves)
		{
			tag = TERMINATION_TAG;
		}

		MPI_Send(&x, 1, MPI_INT, slaveid, tag, MPI_COMM_WORLD);
		x++;
	}

	while (x < N) // While there are existing computations
	{
		MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE, DATA_TAG, MPI_COMM_WORLD, status);

		sum += result;
		slaveid = status->MPI_SOURCE;

		if (N - x <= numslaves)
		{
			tag = TERMINATION_TAG;
		}

		MPI_Send(&x, 1, MPI_INT, slaveid, tag, MPI_COMM_WORLD);
		x++;
	}

	for (slaveid = 1; slaveid <= numslaves; slaveid++)
	{
		MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, status);
		sum += result;
	}

	return sum;
}

void slaveTask(int id, MPI_Status *status)
{
	int x, y, tag = DATA_TAG;
	double result;

	while (1)
	{
		result = 0;

		MPI_Recv(&x, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, status);
		tag = status->MPI_TAG;

		for (y = 0; y < N; y++)
		{
			result += heavy(x, y);
		}

		MPI_Send(&result, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD);

		if (tag == TERMINATION_TAG)
		{
			return;
		}
	}
}

int main(int argc, char *argv[])
{
	int myid, numProcs;
	double answer1, answer2, answer3, seconds1, seconds2, seconds3;
	MPI_Status status;
	clock_t time;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	// Final Result = 8354289.262782

	// Static calculations
	time = clock();
	answer2 = staticFunc(myid, numProcs);
	time = clock() - time;
	seconds2 = ((double) time) / CLOCKS_PER_SEC;

	// Dynamic calculations
	time = clock();
	answer3 = dynamicFunc(myid, numProcs, &status);
	time = clock() - time;
	seconds3 = ((double) time) / CLOCKS_PER_SEC;

	if (myid == MASTER)
	{
		// Sequential calculation
	    time = clock();
	    answer1 = calculate(MASTER, 1);
	    time = clock() - time;
	    seconds1 = ((double) time) / CLOCKS_PER_SEC;

		printf("Sequential Task Answer took %f seconds, answer = %f\n", seconds1, answer1);
		printf("Static Task Answer took %f seconds with %d slaves, answer = %f\n", seconds2, (numProcs - 1) , answer2);
		printf("Dynamic Task Answer took %f seconds with %d slaves, answer = %f\n", seconds3, (numProcs - 1), answer3);
	}

	MPI_Finalize();
}
