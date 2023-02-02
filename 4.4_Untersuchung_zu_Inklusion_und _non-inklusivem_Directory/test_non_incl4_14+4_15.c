/*used in Diagrams 4.14 and 4.15 of the thesis usage: gcc test_non_incl4_14+4_15.c -o test_non_incl && GOMP_CPU_AFFINITY "0" ./test_non_incl * and/or GOMP_CPU_AFFINITY "23" ./test_non_incl*/
#define _GNU_SOURCE
#include <sched.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <asm/unistd.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <assert.h>
#define NB_CBOXES 10
#define INNERLOOP 1000
#define SELECTEDCBOX 0
#define NR_FLUSHES 100
#define B sizeof(char)
#define TOTALADR 1220
#define LLCTHRESHHOLD 200
#define KB 1024*B
#define MB KB*KB
#define STARTOFARRAY 1*MB
//#define PRINTADR
#define BUFSIZE (2*1024*1024)&0xffe00000ULL
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
int NUMBOFADR=0;
static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
			    int cpu, int group_fd, unsigned long flags)
{
	int ret;

	ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
	               group_fd, flags);
	return ret;
}
void swap(unsigned long long *ptr1,unsigned long long *ptr2){
  unsigned long long tmp=*ptr1;
  *ptr1=*ptr2;
  *ptr2=*ptr1;
}
int randrange(int a,int b){
  return (rand() % (b + 1 - a)) + a;
}
char * monitor_cbo(unsigned long long ** array,char * lookup){//TODO modify array

  // setup counters
  struct perf_event_attr pe[NB_CBOXES];

  long pe_fd[NB_CBOXES];
   //TODO get Value

  for(int i=0; i<NB_CBOXES; i++){
    // try to get the type from sysfs
    char buffer[200];
    sprintf(buffer,"/sys/bus/event_source/devices/uncore_cbox_%d/type",i);
    int fd=open(buffer,O_RDONLY);
    assert(fd>0);
    assert((read(fd, buffer, 200)!=-1));
    int type=atoi(buffer);
    memset(&pe[i],0,sizeof(struct perf_event_attr));
    pe[i].type=type;
    //lookupReadI
    pe[i].config=0x8834;//8f34
    pe[i].disabled=1; 
    close(fd);
  }
  
  for(int i=0; i<NB_CBOXES; i++){
    pe_fd[i]=perf_event_open(&pe[i],-1,0,-1,0);
  }

  for (int k=1;k<TOTALADR;k++){
  // Reset and enable
    for(int i=0; i<NB_CBOXES; i++){
      ioctl(pe_fd[i], PERF_EVENT_IOC_RESET, 0);
      ioctl(pe_fd[i], PERF_EVENT_IOC_ENABLE, 0);
    }
  // Launch program to monitor
    for(int i=0;i<100;i++)polling((char *)array[k]); //change 
    for(int i=0; i<NB_CBOXES; i++){
  // Disable
      ioctl(pe_fd[i], PERF_EVENT_IOC_DISABLE, 0);
    }

  // Read and find max
    unsigned long long maximum=0;
    char maximum_cbo=-1;

    for(int i=0; i<NB_CBOXES; i++){
      unsigned long long value;
      read(pe_fd[i],&value,sizeof(long long));
          //printf(" | %d : %d,",i,value);
      if (value>maximum){
        if(maximum_cbo==-1){
          maximum_cbo=i;
          continue;
        }
        if(value>(maximum*100)){
                  maximum=value;
          maximum_cbo=i;
        }else{
          maximum_cbo=-2;
        }

      }
    }
   // printf("\n");
    lookup[k]=maximum_cbo;

  }


  for(int i=0; i<NB_CBOXES; i++){
    close(pe_fd[i]);
  }
}

void pollingOnce ( unsigned long long* addr ){
    for ( int i =0; i < 10 ; i ++){
        asm volatile ("clflush (%0)" :: "r"(addr));
    }
}
void testlat(unsigned long long *ptr,double *lat){
  //warm up
  for(int i=0;i<100;i++){
      asm volatile(
          "mov (%rdi), %rdi;"//1
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//5
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//10
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//15
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//20
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//25
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//30
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//35
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//40
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//45
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
	  "mov (%rdi), %rdi;" 
        );
  }
    //read once to ensure they are cached
    long long start,stop;
  asm volatile("mfence");
  rdtscll(start);
  for(int i=0;i<100;i++){
    asm volatile(
          "mov (%rdi), %rdi;"//1
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//5
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//10
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//15
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//20
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//25
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//30
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//35
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//40
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"//45
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;"
          "mov (%rdi), %rdi;");
  }
    asm volatile("mfence");
    rdtscll(stop);
   
    double calc=(stop-start);
    double NUMBITERS=50*100;
    double o=(double)(calc/NUMBITERS);
    *lat=(double)o;
}
void mov(unsigned long long *ptr){
    asm(
        "mov (%rdi), %r8;"
    );
}
/*        "mov (%rdi+#8), %r8;"
        "mov (%rdi+%16), %r8;"
        "mov (%rdi+%24), %r8;"
        "mov (%rdi+%32), %r8;"
        "mov (%rdi+%40), %r8;"
        "mov (%rdi+%48), %r8;"
        "mov (%rdi+%56), %r8;"*/
char * makeHugeBuf(){
     char *dir = "/mnt/huge";
     char *filename=(char*)malloc((strlen(dir)+20)*sizeof(char));
     sprintf(filename,"%s/thread_data_0",dir);
     dir=NULL;
     int fd=open(filename,O_CREAT|O_RDWR,0664);
     if (fd == -1){
       fprintf( stderr, "Error: could not create file in hugetlbfs\n" ); fflush( stderr );
       perror("open");
       exit( 127 );
     }
     unsigned long long n=BUFSIZE;
     //printf("%lld Bufsize\n",n); 
     //printf("%lld Bufsize\n",(n/1024));
     n*=1024*4;//n==2 GiB Hugepages
     //n*=4; //8 GiB Hugepages
     char * ret=(char*) mmap(NULL,n,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
     //printf("%lld",ret);
     close(fd);unlink(filename);
     return ret;

}
void getReference(unsigned long long **AdressSameIndex){
	 AdressSameIndex[0]=&AdressSameIndex[0];
        for(int i=1;i<TOTALADR;i++){
                AdressSameIndex[i]=(unsigned long long *)(AdressSameIndex[0]+i*(1<<18));//(1<<18)
                *AdressSameIndex[i]=rand();
        }
	for(int i=1;i<TOTALADR;i++){
          pollingOnce(AdressSameIndex[i]);
          mov(AdressSameIndex[i]);
        }
        for(int i=1;i<NUMBOFADR;i++){
          polling(AdressSameIndex[i]);
          mov(AdressSameIndex[i]);
        }

} 
void getSameIndex(unsigned long long **AdressSameIndex,char *lookup){
        AdressSameIndex[0]=&AdressSameIndex[0];
        for(int i=1;i<TOTALADR;i++){
                AdressSameIndex[i]=(unsigned long long *)(AdressSameIndex[0]+i*(1<<18));//(1<<18)
                *AdressSameIndex[i]=rand();
        }
        for(int i=1;i<TOTALADR;i++){
          pollingOnce(AdressSameIndex[i]);
          mov(AdressSameIndex[i]);
        }
        for(int i=1;i<NUMBOFADR;i++){
          polling(AdressSameIndex[i]);
          mov(AdressSameIndex[i]);
        }
        //prevent Penalty Through Writeback to memory (since Data is currently modified)
        monitor_cbo(AdressSameIndex,lookup);
        int j=0;
        for(int i=1;i<TOTALADR;i++){
          if(lookup[i]==SELECTEDCBOX){
            AdressSameIndex[j]=AdressSameIndex[i];
            lookup[j]=lookup[i];
            polling(AdressSameIndex[j]);
            mov(AdressSameIndex[j]);

            j+=1;
          }
          polling(AdressSameIndex[i]);
        }
        //printf("%d NUMBEROFADRCBOX0\n",j);
}
unsigned long long * makelistNeu(unsigned long long ** AdressSameIndex){
  //contains a set of adresses have not been used as elements for the linked list
  unsigned long long options[NUMBOFADR];
  int k=1;
for(int o=1;o<NUMBOFADR+1;o++){
    options[o-1]=(unsigned long long)AdressSameIndex[o];
}
int taken=1;
int debugloop=0;
srandSet();
unsigned long long * cur=AdressSameIndex[1];
//delete first element from options
for(int o=0;o<NUMBOFADR-1;o++){
options[o]=options[o+1];
}
for(int o=0;o<NUMBOFADR-1;o++){
  int randIndex=o;     
  randIndex=randrange(0,NUMBOFADR-taken-1);
  *cur=(unsigned long long)options[randIndex];//add random adress as Value of cur
  cur=(unsigned long long *)*cur;//Jump to Adress referenced by the Value of cur
  for(int o=randIndex;o<NUMBOFADR-1-taken;o++){
    options[o]=options[o+1];//very non efficient way of deleting an element of an array 
  }
  taken+=1;
} 
*cur=(unsigned long long)AdressSameIndex[1];;
unsigned long long * ptr = cur;
int h=rand()%NUMBOFADR;
int i=0;
#ifdef PRINTADR
while(i<NUMBOFADR){
printf("Accesing Adress %llx ",ptr);
printf("try jump to  %llx\n",*ptr);
ptr=(unsigned long long *)*ptr;
i+=1;
}
i=0;
#endif
//select a random Adress as Start
while(i<h){
ptr=(unsigned long long *)*ptr;
i+=1;
}
return ptr;
}



































unsigned long long * makelist(unsigned long long ** AdressSameIndex,int numbOfAdr,int excludeFlag){

  unsigned long long options[numbOfAdr];
  int k=1;
  for(int o=1;o<numbOfAdr;o++){
    if(o==excludeFlag){continue;}
  //set[o-1]=(unsigned long long)AdressSameIndex[o];
    options[o-1]=(unsigned long long)AdressSameIndex[k];
    k+=1;
  //printf("%llx \n",options[o-1]);
}
if(excludeFlag!=0)numbOfAdr-=1;
//printf("%d ,j\n",j);
//srand(1632);
//calc Rand Index
//printf("%d , %llx, \n",randIndex,options[randIndex]);
int taken=1;
int debugloop=0;
unsigned long long * cur=AdressSameIndex[1];
if(excludeFlag==1){
  cur=AdressSameIndex[2];
}
for(int o=0;o<numbOfAdr-1;o++){
      options[o]=options[o+1];
}
for(int o=0;o<numbOfAdr-2;o++){
  int randIndex=o;
  
  randIndex=randrange(0,numbOfAdr-taken-2);
  debugloop=0;
  *cur=(unsigned long long)options[randIndex];
  //printf("cur: %llx , next: %llx,randIndex: %d\n",cur,(unsigned long long)*cur,randIndex);
  //for(int o=0;o<j-2;o++){printf("%d,%llx | ",o,options[o]);};
  //printf("\n\n");
  cur=(unsigned long long *)*cur;

  for(int o=randIndex;o<numbOfAdr-2;o++){
    if(o<numbOfAdr-2-taken){options[o]=options[o+1];}
    else{options[o]=0;}
  }
  taken+=1;

}
if(excludeFlag==1)*cur=(unsigned long long)AdressSameIndex[2];
else{
*cur=(unsigned long long)AdressSameIndex[1];}
//for(int o=0;o<j;o++){AdressSameIndex[o-1]=AdressSameIndex[o];}
int h=(rand()%(numbOfAdr-1))+1;
if(excludeFlag==1&&h==1){
  int o=1;
  do{
    srand(o);o+=1;
    h=(rand()%(numbOfAdr-1))+1;
    //printf("adw2");
  }while(h==1);
}

unsigned long long * ptr = AdressSameIndex[h];

/*for(int o=1;o<numbOfAdr;o++)if(ptr==AdressSameIndex[o])printf("%d -> \n",o);
unsigned long long * first = ptr;
do{
    printf("%llx from ,", ptr);
    printf("%llx to\n", *ptr);
    printf("g");
  if(*ptr==first)break;
  ptr=(unsigned long long *)*ptr;
  printf("h");

  for(int o=1;o<numbOfAdr;o++)if(ptr==AdressSameIndex[o])printf("%d -> \n",o);
}while(ptr!=first);
printf("adw");*/
return ptr;
}
void srandSet(){
  long long st,sp;
     rdtscll(st);
     for(int i=0;i<1000;i++){
      printf("");
     }
     rdtscll(sp);
     srand((sp-st)%100); //make pseudo Randomness less pseudo random
}
void printArray(char* ar,int size,int ElementsPerLine){
  int startelementsperline=ElementsPerLine;
  for(int i=0;i<size;i++){
    if(i<ElementsPerLine){
          printf("%d:%lld,",i,ar[i]);
    }else{
      printf("%d:%lld\n",i,ar[i]);
      ElementsPerLine+=startelementsperline;

    }

  }
  printf("\n");
}
void cflush(unsigned long long **Adr){
for(int i=0;i<NUMBOFADR+30;i++){
polling(Adr[i]);
}
}

void sameEV(unsigned long long ** AdressSameIndex,char *lookupCbo){
double avg=0;
     for(int i=5;i<100;i++){
     NUMBOFADR=i;
     srandSet();
     getSameIndex(AdressSameIndex,lookupCbo);
//AdressSameIndex now contains adresses mapped to same Index and LLC-Slice

    /* for(int n=0;n<4;n++){
     while(j<NUMBOFADR+1){
        //printf("%d lookup\n",lookupCbo[p]);
        unsigned long long minevict=0,minwarm=0;
        if(lookupCbo[j]!=SELECTEDCBOX)break;
        //a=(a%100)+1;
        testadress(AdressSameIndex,j,&minevict,&minwarm);

        minevict/=INNERLOOP;
        minwarm/=INNERLOOP;
        long long bef=minevict-minwarm;
        //printf("%lld Div,%d ,%d\n",bef,lookupCbo[j],j);
        //printArray(&lookupCbo[0],100,100);
        if(bef<LLCTHRESHHOLD){for(int s=j;s<TOTALADR;s++){AdressSameIndex[s]=AdressSameIndex[s+1];lookupCbo[s]=lookupCbo[s+1];}}else{j+=1;};
        };

*/
//AvgDiff/=j;
//printf("%lld AvgDiff,\n",AvgDiff);
unsigned long long * start=makelistNeu(AdressSameIndex);
//printf("%d Adress\n",ActualNumberOfAdr);
        for(int o=0;o<10;o++){
                double lat=0;
                double min=1000000;
                for(int i=0;i<20;i++){
			cflush(AdressSameIndex);
                        testlat(start,&lat);
                        if(lat<min)min=lat;
                }
        avg+=lat;
        }
                printf("%lf ",(avg/10));fflush(stdout);avg=0;}

}
void referenceEV(unsigned long long ** AdressSameIndex){
double avg=0;
     for(int i=5;i<500;i++){
     NUMBOFADR=i;
     srandSet();
     getReference(AdressSameIndex);
//AdressSameIndex now contains adresses mapped to same Index but to seemingly random LLC-Slice
     unsigned long long * start=makelistNeu(AdressSameIndex);
//printf("%d Adress\n",ActualNumberOfAdr);
        for(int o=0;o<10;o++){
                double lat;
                double min=1000000;
                for(int i=0;i<20;i++){
			cflush(AdressSameIndex);
                        testlat(start,&lat);
                        if(lat<min)min=lat;
                }
        avg+=lat;
        }
                printf("%lf ",(avg/10));fflush(stdout);avg=0;}


}
int main(){
     char * buf = makeHugeBuf();
     int ActualNumberOfAdr=0;
     long long AvgDiff=0;
     char* lookupCbo=(char *)(&buf[0]+10000);
     //printf("a\n");
     //buf[0]=0;
     unsigned long long ** AdressSameIndex=buf;
     for(int i=5;i<500;i++){
        printf("%d ",i);        
        }
     printf("\n");

     sameEV(AdressSameIndex,lookupCbo);
     printf("\n");
     referenceEV(AdressSameIndex);
	};
