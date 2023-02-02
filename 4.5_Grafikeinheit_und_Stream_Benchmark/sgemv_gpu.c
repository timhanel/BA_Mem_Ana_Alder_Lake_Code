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

#define REP 100
#define SIZE 40000

void init_data(float* f1, float* f2, float* f3, int size) {
	long x, index, max;
  #pragma omp parallel for schedule(static,1) private(x,index,max)
	for(x = 0; x < size; x++) {
		index = x * size;
		max = index + size;
		f1[x] = 30.0;
		f3[x] = 0.0;
		for(; index < max; index++) {
			f2[index] = 0.01;
		}
	}
}


int main(){
	unsigned size=SIZE;
	float one=1.0;
	double start, stop;
	double nOperations=0.0;
	float *f1, *f2, *f3;
	nOperations = (1.0*size)*(2.0*size);

	f1=(float*)mkl_malloc(size*sizeof(float),64);
	f2=(float*)mkl_malloc(size*size*sizeof(float),64);
	f3=(float*)mkl_malloc(size*sizeof(float),64);

	init_data(f1,f2,f3, size);

	/* ************************** */
#pragma omp target enter data map(to:f1[0:size],f2[0:size*size],f3[0:size]) device(0)
        start=omp_get_wtime();
{
for (long long rep=0;rep<REP;rep++){
 #pragma omp target variant  dispatch device(0) use_device_ptr(f1, f2, f3)
 {
	cblas_sgemv(CblasRowMajor,CblasNoTrans, size, size, one, f2, size, f1, 1, one, f3, 1);
 }
}
}
        stop=omp_get_wtime();
#pragma omp target exit data map(from:f3[0:size])
	/* ************************** */

	printf("%u,%f\n",size,((double)nOperations*(double)REP)/(1.0e9*(stop-start)));
	srand(f3[rand()%size]);
	return 0;
}
