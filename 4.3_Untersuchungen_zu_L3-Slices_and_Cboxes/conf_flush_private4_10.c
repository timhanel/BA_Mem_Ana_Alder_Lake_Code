/*used in Diagram 4.10 of the thesis usage: gcc -fopenmp conf_flush_private4_10.c -o conf_flush && GOMP_CPU_AFFINITY "0,2" ./conf_flush */
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <cpuid.h>
#include <pthread.h>
#include <omp.h>


/*0: default, 1 : print results every iteration(bef,aft) 
>=2: print results every Iteration and Adresses of Pointerjumps performed(source,dest) also validates if 
 note: combining Debuglevel 2 with RANDOMIZE_LIST_ON_EVERY_RUN is not recommended
*/
#define DEBUGLEVEL 1 
 

/*0: Linked List of Pointers is calculated Once before All Runs
1: randomizes Accesses on Every Run
*/
#define RANDOMIZE_LIST_ON_EVERY_RUN 0


/*changes of NR_OF_CACHELINES require adjusting testlat function(unsigned long long *ptr) accordingly -- Example : 
NR_OF_CACHELINES  211 
static inline void testlat(unsigned long long *ptr){
	asm volatile(
		HUN
		HUN
		TEN
		ONE
	);
}
*/
#define NR_OF_CACHELINES 500
#define NR_RUNS 500

//flushes per Cacheline
#define NR_FLUSHES 1000
#define CACHELINESIZE 64

#define VALS_PER_LINE CACHELINESIZE/(sizeof(unsigned long long))
#define rdtscll(val) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (val) = ((unsigned long long)__a) | (((unsigned long long)__d)<<32); \
        } while(0)
void mov(long long * adr){
        asm(
        "mov (%rdi), %r8;"
    );
}
#if (DEBUGLEVEL >= 1)
#define DEBUG
#endif
#if (DEBUGLEVEL >= 2)
#define PRINTADR
#endif

#if (RANDOMIZE_LIST_ON_EVERY_RUN >= 1)
#define RECALC
#endif
#define ONE "mov (%rdi),%rdi;"
#define TEN ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE
#define HUN TEN TEN TEN TEN TEN TEN TEN TEN TEN TEN 

static inline void testlat(unsigned long long *ptr){
asm volatile(
HUN
HUN
HUN
HUN
HUN
);
}
int randrange(int a,int b){
  return (rand() % (b + 1 - a)) + a;};
/*Input: Array of 8 B Sizes Elements (unsigned long long) 
Output: 
*/

void srandSet(){
  long long st,sp;
     rdtscll(st);
     for(int i=0;i<1000;i++){
      printf("");
     }
     rdtscll(sp);
     srand((sp-st)%100); //make pseudo Randomness less pseudo random
}

unsigned long long * makelist(unsigned long long * ar){
  //contains a set of adresses have not been used as elements for the linked list
  unsigned long long options[NR_OF_CACHELINES];
  int k=1;
for(int o=0;o<NR_OF_CACHELINES;o++){
    options[o]=(unsigned long long)&ar[o*VALS_PER_LINE];
}
int taken=1;
int debugloop=0;
srandSet();
unsigned long long * cur=(unsigned long long *)options[0];
//delete first element from options
for(int o=0;o<NR_OF_CACHELINES-1;o++){
options[o]=options[o+1];
}
for(int o=0;o<NR_OF_CACHELINES-1;o++){
  int randIndex=o;
  randIndex=randrange(0,NR_OF_CACHELINES-taken-1);
  *cur=(unsigned long long)options[randIndex];//add random adress as Value of cur
  cur=(unsigned long long *)*cur;//Jump to Adress referenced by the Value of cur
  for(int o=randIndex;o<NR_OF_CACHELINES-taken;o++){
    options[o]=options[o+1];//very non efficient way of deleting an element of an array 
  }
  taken+=1;
} 
*cur=&ar[0];
unsigned long long * ptr = cur;
int h=rand()%NR_OF_CACHELINES;
int i=0;
int valid =1;
#ifdef DEBUG
unsigned long long validate[NR_OF_CACHELINES];
for(int o=0;o<NR_OF_CACHELINES;o++){
validate[o]=0;
}
while(i<NR_OF_CACHELINES){
unsigned long long g=(unsigned long long)ptr;
if(validate[(g>>6)%NR_OF_CACHELINES]!=0){valid=0;printf("Error JUMP to the Same Adress Twice:\n ");};
	validate[(g>>6)%NR_OF_CACHELINES]=1;
	#ifdef PRINTADR
	printf("Accesing Adress %llx ",ptr);
	printf("try jump to  %llx\n",*ptr);
	#endif
	ptr=(unsigned long long *)*ptr;
	i+=1;
	
}
if(valid==0)printf("Failed Adress Validity Check: Adresses are not unique\n");
else printf("Validity of Linked List Checked: Every Adress is accessed exactly Once\n");
i=0;
#endif
//select a random Adress as Start
while(i<h){
	ptr=(unsigned long long *)*ptr;
	i+=1;
}
return ptr;
}


void flush ( unsigned long long* addr ){
  for ( int i =0; i < NR_FLUSHES ; i ++){
    asm volatile ("clflush (%0)" :: "r"(addr));
  }
}
int main(){

int valsPerLine=64/(sizeof(unsigned long long));
unsigned long long array[NR_OF_CACHELINES*valsPerLine] __attribute__ ((aligned (4096)));
unsigned long long * ptr=makelist(&array[0]);
for(int i=0;i<NR_OF_CACHELINES;i++){
                        flush(&array[i*VALS_PER_LINE]);
                }
double minbef=10000000;
double minaft=10000000;
double avg=0;
double avgBef=0;
#ifdef DEBUG
printf("Before,After\n");
#endif
#pragma omp parallel num_threads(2)
{	
	int thread=omp_get_thread_num();
	for(int j=0;j<NR_RUNS;j++){
	#ifdef RECALC
	if(thread==0){
		ptr=makelist(&array[0]);
		flush(ptr);
	}
	#endif
	#pragma omp barrier
	//FLUSH before messurements (to ensure loading of exclusive Cachelines)
	for(int i=0;i<NR_OF_CACHELINES;i++){
                        flush(&array[i*VALS_PER_LINE]);
                }
	#pragma omp barrier

	long long start,stop;
	if(thread==0){
		//Warm up Thread 0 L1 Data Cache
		for(int i=0;i<NR_OF_CACHELINES;i++){
                        mov(&array[i*VALS_PER_LINE]);
                }
		
		//Test Latency of Thread 0 private L1 Cache (for Reference) before FLUSH on Remote Core
		//using lfence to serialize load operations
		asm volatile("lfence;");
		rdtscll(start);
		testlat(ptr); //pointerchasing
		asm volatile("lfence;");
		rdtscll(stop);
		if((stop-start)<minbef){minbef=stop-start;}
		#ifdef DEBUG
		
		printf("%lf,",(double)(stop-start)/NR_OF_CACHELINES);
		#endif
		avgBef+=(double)(stop-start)/NR_OF_CACHELINES;
		
	}
	#pragma omp barrier
	//Flushing on all Cachelines previously read by Thread 0 on Thread 1 (has to reside on a remote core)
	if(thread==1){
asm volatile("mfence;");
		//testlat(ptr); Uncomment to have Lines in Shared State (S)
		for(int i=0;i<NR_OF_CACHELINES+5;i++){
                       	flush(&array[i*VALS_PER_LINE]);
                }
		flush(ptr);

asm volatile("mfence;");

	}
	#pragma omp barrier
	//usleep(100);
         #pragma omp barrier
	/*Messure Latency again on Thread 0. A Latency of >250 cycles indicates that most Adresses have to be read from DRAM concluding that Thread 1 was able to 
	interfere(evict) in Thread 0 private Cache via Clflush. The Implication is that Clflushes have to be snooped (broadcast) to all private Caches (in the absence of a Directory)*/
	if(thread==0){
		asm volatile("lfence;");
                rdtscll(start);
                testlat(ptr); //pointerchasing
                asm volatile("lfence;");
                rdtscll(stop);
		
		if((stop-start)<minaft){minaft=stop-start;}
		#ifdef DEBUG
		printf("%lf\n",(double)(stop-start)/NR_OF_CACHELINES);
		#endif
		avg+=(stop-start)/NR_OF_CACHELINES;
	
		

	}
	#pragma omp barrier
	

}
}
printf("\n\n");
printf("Results: \n");
printf("Latency[cycles] before Flush: Min: %lf, Avg: %lf \nLatency[cycles] after Flush:  Min: %lf Avg: %lf",minbef/NR_OF_CACHELINES,(double)avgBef/NR_RUNS,minaft/NR_OF_CACHELINES,avg/NR_RUNS);

}
