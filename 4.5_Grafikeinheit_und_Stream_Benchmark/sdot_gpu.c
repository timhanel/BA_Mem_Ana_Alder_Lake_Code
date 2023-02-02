/********************************************************************
 * BenchIT - Performance Measurement for Scientific Applications
 * Contact: developer@benchit.org
 *
 * For license details see COPYING in the package base directory
 *******************************************************************/
/* Kernel: Matrix Vector Multiply, BLAS, MKL Offloading (C) - OpenMP version
 *******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include <mkl.h>
#include <mkl_omp_offload.h>

#define REP 100ULL
#define SIZE 1000000000ULL

void init_data(float* f1, float* f2, float* f3, int size) {
	long x;
  #pragma omp parallel for schedule(static,1) private(x)
	for(x = 0; x < size; x++) {
		f1[x] = 30.0;
		f2[x] = 0.0;
		f3[x]=0.0;
	}
}


int main(){
	unsigned size=SIZE;
	double start, stop;
	double nOperations=0.0;
	float *ff1, *ff2, *ff3;
	nOperations = (2.0*size);

	ff1=(float*)mkl_malloc(size*sizeof(float),64);
	ff2=(float*)mkl_malloc(size*sizeof(float),64);
	ff3=(float*)mkl_malloc(size*sizeof(float),64);

	init_data(ff1,ff2,ff3, size);

	/* ************************** */
#pragma omp target enter data map(to:ff1[0:size],ff2[0:size],ff3[0:size]) device(0)
        start=omp_get_wtime();
{
for (long long rep=0;rep<REP;rep++){
 #pragma omp target variant  dispatch device(0) use_device_ptr(ff1, ff2, ff3)
 {
//	ff3[0]+=
		cblas_sdot(size, ff1, 1, ff2, 1);
 }
}
}
        stop=omp_get_wtime();
#pragma omp target exit data map(from:ff3[0:1])
	/* ************************** */

	printf("%u,%f\n",size,((double)nOperations*(double)REP)/(1.0e9*(stop-start)));
	srand(ff3[0]);
	return 0;
}
