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

#define REP 10
#define SIZE 10000

void init_data(float* f1, float* f2, float* f3, int size) {
	long x, index, max;
  #pragma omp parallel for schedule(static,1) private(x,index,max)
	for(x = 0; x < size; x++) {
		index = x * size;
		max = index + size;
		for(; index < max; index++) {
                        f1[index] = 30.01;
			f2[index] = 0.01;
                        f3[index] = 0.01;
		}
	}
}


int main(){
	unsigned size=SIZE;
	float one=1.0;
	double start, stop;
	double nOperations=0.0;
	float *f1, *f2, *f3;
	nOperations = (1.0*size)*(2.0*size)*(2.0*size);

	f1=(float*)mkl_malloc(size*size*sizeof(float),64);
	f2=(float*)mkl_malloc(size*size*sizeof(float),64);
	f3=(float*)mkl_malloc(size*size*sizeof(float),64);

	init_data(f1,f2,f3, size);

	/* ************************** */
#pragma omp target enter data map(to:f1[0:size],f2[0:size*size],f3[0:size]) device(0)
        start=omp_get_wtime();
{
for (long long rep=0;rep<REP;rep++){
 #pragma omp target variant  dispatch device(0) use_device_ptr(f1, f2, f3)
 {
	cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, size, size, size, one, f1, size, f2, size, one, f3, size);
 }
}
}
        stop=omp_get_wtime();
#pragma omp target exit data map(from:f3[0:size])
	/* ************************** */

	printf("%u,%f,%f\n",size,((double)nOperations*(double)REP)/(1.0e9*(stop-start)),stop-start);
	srand(f3[rand()%size]);
	return 0;
}
