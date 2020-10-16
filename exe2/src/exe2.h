#ifndef _EXE2_H
#define _EXE2_H

// ---------------------------------------------------- Includes ----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "mpi.h"

// ---------------------------------------------------- Defines ----------------------------------------------------

#define MASTER 0

#define N 16
#define LEFT_TO_RIGHT 0
#define RIGHT_TO_LEFT 1

#define ROWS (int) sqrt(N)
#define COLUMNS (int) sqrt(N)

#define INPUT_FILE_PATH "../cuboids.dat"
#define OUTPUT_FILE_PATH "../result.dat"

// ---------------------------------------------------- Structs ----------------------------------------------------

typedef struct Cube
{
	int id;
	double length;
	double width;
	double height;
} Cube;

// --------------------------------------------------- Prototypes ---------------------------------------------------

void readCubeFromFile(FILE *f, Cube *c);
int readAllCubesFromFile(Cube *cubes);
void writeAllCubeIdsToFile(Cube *cubes);

int compareCubes(Cube *c1, Cube *c2);
Cube min(Cube *c1, Cube *c2);
Cube max(Cube *c1, Cube *c2);

void setMPICubeStruct(MPI_Datatype *MPI_CUBE);
int checkProgramTerms(int numProcs, Cube *cubes, MPI_Datatype *MPI_CUBE);

void cubeShearSort(Cube *value, int coords[], int rank, MPI_Datatype *MPI_CUBE, MPI_Status *status, MPI_Comm *comm2D);
void oddEvenCubeSort(Cube *value, int location, int n, int pLeft, int pRight, int order, MPI_Datatype *MPI_CUBE, MPI_Status *status);
void defineCompareFunctionsByOrder(Cube (**f1)(Cube*, Cube*), Cube (**f2)(Cube*, Cube*), int order);


#endif
