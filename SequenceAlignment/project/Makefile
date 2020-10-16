build:
	mpicxx -fopenmp -c main.c -o main.o
	mpicxx -fopenmp -c cFunctions.c -o cFunctions.o
	nvcc -I./inc -c cudaFunctions.cu -o cudaFunctions.o
	mpicxx -fopenmp -o ex  main.o cFunctions.o cudaFunctions.o  /usr/local/cuda-9.1/lib64/libcudart_static.a -ldl -lrt

clean:
	rm -f *.o ./ex

run:
	mpiexec -np 1 ./ex
run2:
	mpiexec -np 2 ./ex

runparallel:
	mpiexec -np 2 -machinefile Address -map-by node ./ex
