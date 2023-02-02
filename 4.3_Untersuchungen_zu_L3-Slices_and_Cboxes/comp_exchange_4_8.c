/*Used for Diagram 4.8 of the Thesis Usage: gcc -fopenmp comp_exchange_4_8.c -o comp && GOMP_CPU_AFFINITY "0-23" ./comp  */
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <cpuid.h>
#include <pthread.h>
#include <stdatomic.h>
#include <omp.h>

/* array size in byte ARRAY_SIZE/CACHE_LINE_SIZE accesses will be measured */
#define ARRAY_SIZE (16*1024)
/* nr of ping pongs */
#define NR_CHANGES 100000
#define CACHE_LINE_SIZE 64
/* max nr of CPUs */
#define NR_CPUS 12

#define rdtscll(val) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (val) = ((unsigned long long)__a) | (((unsigned long long)__d)<<32); \
	} while(0)
#define BUFFER_SIZE (256)
unsigned long long val_select_evt_core=0x8f34; //MESIF ANY
int nr_cbos=6;

static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
			    int cpu, int group_fd, unsigned long flags)
{
	int ret;

	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
	               group_fd, flags);
    
	return ret;
}

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

/* from http://fivelinesofcode.blogspot.com/2014/03/how-to-translate-virtual-to-physical.html */
unsigned long long read_pagemap(unsigned long long virt_addr){

int i, c, pid, status;
uint64_t read_val, file_offset;
FILE * f;
char *end;
  char path_buf[]="/proc/self/pagemap";
   f = fopen(path_buf, "rb");
   if(!f){
      printf("Error! Cannot open %s\n", path_buf);
      return -1;
   }
   
   //Shifting by virt-addr-offset number of bytes
   //and multiplying by the size of an address (the size of an entry in pagemap file)
   file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
   status = fseek(f, file_offset, SEEK_SET);
   if(status){
      perror("Failed to do fseek!");
      return -1;
   }
   errno = 0;
   read_val = 0;
   unsigned char c_buf[PAGEMAP_ENTRY];
   for(i=0; i < PAGEMAP_ENTRY; i++){
      c = getc(f);
      if(c==EOF){
         printf("\nReached end of the file\n");
         return 0;
      }
      c_buf[PAGEMAP_ENTRY - i - 1] = c;
   }
   for(i=0; i < PAGEMAP_ENTRY; i++){
      //printf("%d ",c_buf[i]);
      read_val = (read_val << 8) + c_buf[i];
   }
   fclose(f);
   return (( 0xffffffffffffff & read_val) * getpagesize() ) + (virt_addr % getpagesize());
}

/* from https://stackoverflow.com/questions/39448276/how-to-use-clflush */
/* from Reverse Engineering Intel Last-Level Cache Complex Addressing Using Performance Counters */
void polling ( char* addr ){
  for ( int i =0; i < 2000 ; i ++){
    asm volatile ("clflush (%0)" :: "r"(addr));
  }
}
void setup_cbos(long *pe_fd){
  struct perf_event_attr pe[nr_cbos];
  for(int i=0; i<nr_cbos; i++){
    // try to get the type from sysfs
    char buffer[BUFFER_SIZE];
    sprintf(buffer,"/sys/bus/event_source/devices/uncore_cbox_%d/type",i);
    int fd=open(buffer,O_RDONLY);
    assert(fd>0);
    assert((read(fd, buffer, BUFFER_SIZE)!=-1));
    int type=atoi(buffer);
    memset(&pe[i],0,sizeof(struct perf_event_attr));
    pe[i].type=type;
    pe[i].config=val_select_evt_core;
    pe[i].disabled=1; 
    close(fd);
  }

  for(int i=0; i<nr_cbos; i++){
    pe_fd[i]=perf_event_open(&pe[i],-1,0,-1,0);
  }
}
// from https://github.com/rschoene/msr-uncore-cbo
void printCbox(long *pe_fd, void * address){
    for(int i=0; i<nr_cbos; i++){
      ioctl(pe_fd[i], PERF_EVENT_IOC_RESET, 0);
      ioctl(pe_fd[i], PERF_EVENT_IOC_ENABLE, 0);
    }
  // Launch program to monitor
    polling(address);
    for(int i=0; i<nr_cbos; i++){
  // Disable
      ioctl(pe_fd[i], PERF_EVENT_IOC_DISABLE, 0);
    }

  // Read and find max
    long long maximum=0;
    char maximum_cbo=-1;

    for(int i=0; i<nr_cbos; i++){
      long long value;
      read(pe_fd[i],&value,sizeof(long long));

//      values[(address-array)/64][i]=value;
      if (value>maximum){
        maximum=value;
        maximum_cbo=i;
      }
    }
    printf("%d\n",maximum_cbo);
}
int main(){
    int* array=calloc(1,ARRAY_SIZE);
    int * first=&array[0];
    long pe_fd[nr_cbos];
    setup_cbos(&pe_fd);
    for (int i=0; i<ARRAY_SIZE/sizeof(int);i+=CACHE_LINE_SIZE/sizeof(int)){
        volatile int * adress=&array[i];
        printf("%04x ",adress);
        printf("%04x ",read_pagemap(adress));
        printCbox(&pe_fd,adress);
    }
    omp_set_num_threads(2);
    printf("source, target \n");
    #pragma omp parallel
    {
        pthread_t pid = pthread_self();
        cpu_set_t cpuset;
        int thread = omp_get_thread_num();
        /* for all CPU pairs */
        for (int source = 0 ; source < NR_CPUS; source+=1){
            for (int target = 0; target < NR_CPUS ; target+=1){
            /* from https://github.com/clementine-m/msr-uncore-cbo/ */

                /* pin threads to CPUs */
                CPU_ZERO(&cpuset);

                long long start, stop;
                int index=0;
                int expected,desired;
                if (thread==0){
                    expected = 1; desired = 0;
                    CPU_SET(source, &cpuset);
                } else {
                    expected = 0; desired = 1;
                    CPU_SET(target, &cpuset);
                }
                pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuset);
                
                
                /* measure ping pong latency for multiple addresses */
                for (index=0; index<ARRAY_SIZE/sizeof(int);index+=CACHE_LINE_SIZE/sizeof(int)){
                    volatile int * adress=&array[index];
                    /* TODO get CBox for address */

                    /* master measure time */

                    /* ping pong */
                    #pragma omp barrier
                    if (thread==0){
                        rdtscll(start);
                    }
                    #pragma omp barrier
                    for ( int i=0; i<NR_CHANGES;i++ ){
                        while(!atomic_compare_exchange_strong_explicit(adress,&expected,desired,memory_order_seq_cst,memory_order_seq_cst));
                    }
                    /* master measure time */
                    #pragma omp barrier
                    if (thread==0){
                        rdtscll(stop);
                        /* TODO print CBox for address */
                        //printCbox(&pe_fd,adress);
                        printf("%d %d ",source,target);
                        printf("%04x ",adress);
                        printf("%d\n",((stop-start)*100)/(2*NR_CHANGES));

                    }
                }
                if (thread==0)
                    printf("\n");
            }
        }
        for(int i=0; i<nr_cbos; i++){
            close(pe_fd[i]);
        }
        
    }
}
