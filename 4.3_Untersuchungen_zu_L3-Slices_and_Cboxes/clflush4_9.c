/*used in Diagram 4.9 and 4.11 of the thesis usage: gcc -fopenmp clflush4_9.c -o comp && GOMP_CPU_AFFINITY "0-23" ./comp  */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
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
#include <string.h>

#define NR_FLUSHES (10000)
#define NR_ADRESSES (100)
#define NB_CORES (10)
#define BUFFER_SIZE (512)

#define rdtscll(val) do { \
     unsigned int __a,__d; \
     asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
     (val) = ((unsigned long long)__a) | (((unsigned long long)__d)<<32); \
	} while(0)

void polling ( char* addr ){
  for ( int i =0; i < NR_FLUSHES ; i ++){
    asm volatile ("clflush (%0)" :: "r"(addr));
  }
}

void pollingopt ( char* addr ){
  for ( int i =0; i < NR_FLUSHES ; i ++){
    asm volatile ("clflushopt (%0)" :: "r"(addr));
  }
}

void polling2 ( long long * addr ){
  for ( int i =0; i < NR_FLUSHES ; i ++){
    asm volatile ("clflush (%0)" :: "r"(addr));
  }
}
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
			    int cpu, int group_fd, unsigned long flags)
{
	int ret;

	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
	               group_fd, flags);
	return ret;
}

void monitor_cbo(long long * array){

  // setup counters
  struct perf_event_attr pe[NB_CORES];

  long pe_fd[NB_CORES];
  char* values=malloc(NR_ADRESSES);

  for(int i=0; i<NB_CORES; i++){
    // try to get the type from sysfs
    char buffer[BUFFER_SIZE];
    sprintf(buffer,"/sys/bus/event_source/devices/uncore_cbox_%d/type",i);
    int fd=open(buffer,O_RDONLY);
    assert(fd>0);
    assert((read(fd, buffer, BUFFER_SIZE)!=-1));
    int type=atoi(buffer);
    memset(&pe[i],0,sizeof(struct perf_event_attr));
    pe[i].type=type;
    pe[i].config=0x8f34;
    pe[i].disabled=1; 
    close(fd);
  }

  for(int i=0; i<NB_CORES; i++){
    pe_fd[i]=perf_event_open(&pe[i],-1,0,-1,0);
  }

  for (int k=0;k<NR_ADRESSES;k++){
  // Reset and enable
    for(int i=0; i<NB_CORES; i++){
      ioctl(pe_fd[i], PERF_EVENT_IOC_RESET, 0);
      ioctl(pe_fd[i], PERF_EVENT_IOC_ENABLE, 0);
    }
  // Launch program to monitor
    polling(&array[(k*64)/sizeof(long long)]);
    for(int i=0; i<NB_CORES; i++){
  // Disable
      ioctl(pe_fd[i], PERF_EVENT_IOC_DISABLE, 0);
    }

  // Read and find max
    long long maximum=0;
    char maximum_cbo=-1;

    for(int i=0; i<NB_CORES; i++){
      long long value;
      read(pe_fd[i],&value,sizeof(long long));
      if (value>maximum){
        maximum=value;
        maximum_cbo=i;
      }
    }
    values[k]=maximum_cbo;
  }

  printf("CBo");
  for(int k=0;k<NR_ADRESSES;k++){
    printf(",%d", values[k]);
  }
  printf("\n");

  for(int i=0; i<NB_CORES; i++){
    close(pe_fd[i]);
  }
}
void setup(long long *cbo,long event){
struct perf_event_attr pe[NB_CORES];

for(int i=0; i<NB_CORES; i++){
    // try to get the type from sysfs
    char buffer[BUFFER_SIZE];
    sprintf(buffer,"/sys/bus/event_source/devices/uncore_cbox_%d/type",i);
    int fd=open(buffer,O_RDONLY);
    assert(fd>0);
    assert((read(fd, buffer, BUFFER_SIZE)!=-1));
    int type=atoi(buffer);
    memset(&pe[i],0,sizeof(struct perf_event_attr));
    pe[i].type=type;
    pe[i].config=event;
    pe[i].disabled=1; 
    close(fd);
  }

  for(int i=0; i<NB_CORES; i++){
    cbo[i]=perf_event_open(&pe[i],-1,0,-1,0);
  }


}

void main(){
	long long array[NR_ADRESSES] __attribute__ ((aligned (4096)));
	monitor_cbo(array);
	long long cbo[10];
	long event=0x4122;
	//setup(&cbo,event);
	
#pragma omp parallel
{
	long long start,stop;
	/*long long * my_min=malloc(NB_CORES*NR_ADRESSES*sizeof(long long));
	for (int k=0;k<NR_ADRESSES;k++)
		for(int a=0;a<NB_CORES;a++)
		my_min[NR_ADRESSES*k+a]=100000000;*/
	for (int i=0;i<omp_get_num_threads();i++){
		
		if (omp_get_thread_num()==i){
			printf("%d",omp_get_thread_num());
			for (int k=0;k<NR_ADRESSES;k++){
				long long min=1000000000;
			for (int j=0;j<1000;j++) {
				/*for(int i=0; i<NB_CORES; i++){
                			ioctl(cbo[i], PERF_EVENT_IOC_RESET, 0);
                			ioctl(cbo[i], PERF_EVENT_IOC_ENABLE, 0);
         			}*/
				rdtscll(start);
				polling(&array[(64/sizeof(long long))*k]);
				rdtscll(stop);
				/*for(int i=0; i<NB_CORES; i++){
      					ioctl(cbo[i], PERF_EVENT_IOC_DISABLE, 0);
					long long value;
                                        read(cbo[i],&value,sizeof(long long));
					my_min[NR_ADRESSES*k+i]+=value;
    				}*/
				if ((stop-start)<min)
					min=stop-start;
			}
        		//for (int k=0;k<NR_ADRESSES;k++)for(int a=0;a<NB_CORES;a++)my_min[NR_ADRESSES*k+a]/=1000;
			printf(",%d",min/NR_FLUSHES);
			fflush(stdout);
			}
			//for(int a=0;a<NB_CORES;a++)printf("Cbo(Event=%ld): %d, %lld\n",event,a,my_min[a]);
			printf("\n");
		}
		#pragma omp barrier
		sleep(1);
		#pragma omp barrier
		sleep(1);
		#pragma omp barrier
		sleep(1);
	}
}
}
