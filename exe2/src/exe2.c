#include "exe2.h"

void readCubeFromFile(FILE *f, Cube *c)
{
	fscanf(f,"%d %lf %lf %lf", &c->id, &c->length, &c->width, &c->height);
}

int readAllCubesFromFile(Cube *cubes)
{
	// Returns -1 if fails to open file, else 1

	int i;
	FILE *f;

	f = fopen(INPUT_FILE_PATH, "r");
	if (!f)
	{
		printf("Unable to open file!\n");
		fflush(stdout);
		return -1;
	}

	for (i = 0; i < N; i++)
		readCubeFromFile(f, (cubes + i));

	fclose(f);
	return 1;
}

void writeAllCubeIdsToFile(Cube *cubes)
{
	// Writes cube IDs into file according to the order of shear sort

	int i, j;
	FILE *f;

	f = fopen(OUTPUT_FILE_PATH, "w");

	for (i = 0; i < ROWS; i++)
	{
		if (i % 2 == 0)
		{
			for (j = 0; j < COLUMNS; j++)
				fprintf(f, "%d ", (cubes + i * ROWS + j)->id);
		}
		else
		{
			for (j = COLUMNS - 1; j >= 0; j--)
				fprintf(f, "%d ", (cubes + i * ROWS + j)->id);
		}
	}

	fclose(f);
}

int compareCubes(Cube *c1, Cube *c2)
{
	// Compare a couple of cubes by their volumes and heights

	double v1, v2;
	v1 = c1->length * c1->height * c1->width;
	v2 = c2->length * c2->height * c2->width;

	if (!c1 || !c2)
	{
		printf("Input has null!");
		return 0;
	}

	if (v1 != v2)
		return v1 - v2;

	return c1->height - c2->height;
}

Cube min(Cube *c1, Cube *c2)
{
	if (compareCubes(c1, c2) < 0)
		return *c1;
	return *c2;
}

Cube max(Cube *c1, Cube *c2)
{
	if (compareCubes(c2, c1) < 0)
		return *c1;
	return *c2;
}

void defineCompareFunctionsByOrder(Cube (**f1)(Cube*, Cube*), Cube (**f2)(Cube*, Cube*), int order)
{
	// Defines the functions which used to set the new cube value in oddEvenCubeSort

	if (order == LEFT_TO_RIGHT)
	{
		*f1 = &min;
		*f2 = &max;
	}
	else if (order == RIGHT_TO_LEFT)
	{
		*f1 = &max;
		*f2 = &min;
	}
}

void oddEvenCubeSort(Cube *value, int location, int n, int pLeft, int pRight, int order, MPI_Datatype *MPI_CUBE, MPI_Status *status)
{
	int i;
	Cube otherValue;

	Cube (*f1)(Cube*, Cube*);
	Cube (*f2)(Cube*, Cube*);
	defineCompareFunctionsByOrder(&f1, &f2, order);

	for (i = 0; i < n; i++)
	{
		if (location % 2 == 0)
		{
			if (i % 2 == 0) // Even iteration
			{
				if (pRight >= 0) // Right neighbor of the process in the given location (rank), exists
				{
					MPI_Send(value, 1, *MPI_CUBE, pRight, 0, MPI_COMM_WORLD);
					MPI_Recv(&otherValue, 1, *MPI_CUBE, pRight, 0, MPI_COMM_WORLD, status);
					*value = f1(value, &otherValue);
				}
			}
			else if (pLeft >= 0) // Odd iteration, only check if the left neighbor of the process in the given location (rank), exists
			{
				MPI_Send(value, 1, *MPI_CUBE, pLeft, 0, MPI_COMM_WORLD);
				MPI_Recv(&otherValue, 1, *MPI_CUBE, pLeft, 0, MPI_COMM_WORLD, status);
				*value = f2(value, &otherValue);
			}
		}
		else
		{
			if (i % 2 == 0) // Even iteration
			{
				if (pLeft >= 0) // Left neighbor of the process in the given location (rank), exists
				{
					MPI_Send(value, 1, *MPI_CUBE, pLeft, 0, MPI_COMM_WORLD);
					MPI_Recv(&otherValue, 1, *MPI_CUBE, pLeft, 0, MPI_COMM_WORLD, status);
					*value = f2(value, &otherValue);
				}
			}
			else if (pRight >= 0) // Odd iteration, only check if the right neighbor of the process in the given location (rank), exists
			{
				MPI_Send(value, 1, *MPI_CUBE, pRight, 0, MPI_COMM_WORLD);
				MPI_Recv(&otherValue, 1, *MPI_CUBE, pRight, 0, MPI_COMM_WORLD, status);
				*value = f1(value, &otherValue);
			}
		}
	}
}

void cubeShearSort(Cube *value, int coords[], int rank, MPI_Datatype *MPI_CUBE, MPI_Status *status, MPI_Comm *comm2D)
{
	int i, pSource, pDest, numPhases = (int) sqrt(N) * (int) (log2(N) + 1);

	for (i = 0; i < numPhases; i++)
	{
		if (i % 2 == 0) // Even iterations (includes i=0) -> sort rows
		{
			MPI_Cart_shift(*comm2D, 1, 1, &pSource, &pDest); // Gets left-right current neighbors
			if (coords[0] % 2 == 0) // Even row (includes row=0) -> sort row with order=LEFT_TO_RIGHT
				oddEvenCubeSort(value, rank, ROWS, pSource, pDest, LEFT_TO_RIGHT, MPI_CUBE, status);
			else // Odd row -> sort row with order=RIGHT_TO_LEFT
				oddEvenCubeSort(value, rank, ROWS, pSource, pDest, RIGHT_TO_LEFT, MPI_CUBE, status);
		}
		else // Odd iterations -> sort columns
		{
			MPI_Cart_shift(*comm2D, 0, 1, &pSource, &pDest); // Gets upper-bottom current neighbors
			oddEvenCubeSort(value, rank, COLUMNS, pSource, pDest, LEFT_TO_RIGHT, MPI_CUBE, status);
		}
	}
}

void setMPICubeStruct(MPI_Datatype *MPI_CUBE)
{
	// Defines a new data type supporting MPI functions

    int nitems = 4;
    int blocklengths[4] = {1, 1, 1, 1};
    MPI_Datatype types[4] = {MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE};
    MPI_Aint offsets[4];

    offsets[0] = offsetof(Cube, id);
    offsets[1] = offsetof(Cube, length);
    offsets[2] = offsetof(Cube, width);
	offsets[3] = offsetof(Cube, height);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, MPI_CUBE);
    MPI_Type_commit(MPI_CUBE);
}

int checkProgramTerms(int numProcs, Cube *cubes, MPI_Datatype *MPI_CUBE)
{
	// Check program conditions, and read data from dat file into cubes
	// Returns 1 if the program is free to run by the conditions, else -1

	if (numProcs != N)
	{
		printf("Please run the program with %d processes or define N again.\n", N);
		fflush(stdout);
		return -1;
	}

	if (ROWS % 2 != 0 || COLUMNS % 2 != 0)
	{
		printf("Please change %d N as number of processes or define ROWS (%d) and COLUMNS (%d) to be even numbers.\n", N, ROWS, COLUMNS);
		fflush(stdout);
		return -1;
	}

	return readAllCubesFromFile(cubes);
}

int main(int argc, char *argv[])
{
	int myid, numProcs, dim[2], period[2], reorder, coords[2], flag = 1;
	MPI_Datatype MPI_CUBE;
	Cube value, sortedCubes[N], cubes[N];

	MPI_Status status;
	MPI_Comm comm2D;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

	if (myid == MASTER)
		flag = checkProgramTerms(numProcs, cubes, &MPI_CUBE);

	MPI_Bcast(&flag, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
	if (flag == -1)
	{
		MPI_Finalize();
		exit(1);
	}

	setMPICubeStruct(&MPI_CUBE);

   // Creates 2D Cartesian cart
    dim[0] = ROWS;
	dim[1] = COLUMNS;
    period[0] = 0;
	period[1] = 0;
    reorder = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &comm2D);
    MPI_Cart_coords(comm2D, myid, 2, coords);

	MPI_Scatter(cubes, 1, MPI_CUBE, &value, 1, MPI_CUBE, MASTER, MPI_COMM_WORLD);
	cubeShearSort(&value, coords, myid, &MPI_CUBE, &status, &comm2D);
	MPI_Gather(&value, 1, MPI_CUBE, sortedCubes, 1, MPI_CUBE, MASTER, MPI_COMM_WORLD);

	if (myid == MASTER)
		writeAllCubeIdsToFile(sortedCubes);

	MPI_Finalize();
	exit(0);
}
