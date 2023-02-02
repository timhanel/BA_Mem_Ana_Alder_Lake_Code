#!/bin/sh
ArraySize="160000000 140000000 120000000 100000000 90000000 80000000 70000000 60000000 50000000 40000000 30000000 20000000 10000000 9000000 8000000 7000000 6000000 5000000 4000000 3000000 2000000 1000000 950000 900000 850000 800000 750000 700000 650000 600000 550000 500000 475000 450000 425000 400000 375000 350000 325000 300000 290000 280000 270000 260000 250000 240000 230000 220000 210000 200000 190000 180000 170000 160000 150000 140000 130000 120000 110000 100000 95000 90000 85000 80000 75000 70000 65000 60000 55000 50000 47500 45000 42500 40000 37500 35000 32500 30000 27500 25000 22500 20000"
ECores=0xFF0000
PCores=0x00FFFF
All=0xFFFFFF
PCore=0x000001
ECore=0x010000
loopsize=10
INNERLOOP=0
for SIZE in $ArraySize; do

if [ $(($SIZE)) -gt 10000000 ]
then 
loopsize=100
INNERLOOP=1
else 
loopsize=100
INNERLOOP=10
fi
if [ $(($SIZE)) -lt 1000001 ]
then 
loopsize=100
INNERLOOP=1000
fi

if [ $(($SIZE)) -lt 100001 ]

then 
loopsize=10
INNERLOOP=10000
fi

echo "loopsize"
echo $loopsize
echo "\n"
echo "ArraySize"
echo $SIZE


echo "ICX"
#load icx compiler
#module load intel/2022.1

icx -fiopenmp -fopenmp-targets=spir64 -fsycl -O3 stream_gpu.c -lsycl -lOpenCL -g -DINNERLOOP=$INNERLOOP -DSTREAM_ARRAY_SIZE=$SIZE -DNTIMES=$loopsize -DSTREAM_TYPE=float -o stream_gpu
echo "ECore"
GOMP_CPU_AFFINITY=23 taskset 0x100000 ./stream_gpu > stream_gpu_res/"ECore"/$SIZE.txt
echo "PCore"
GOMP_CPU_AFFINITY=0 taskset 0x000001 ./stream_gpu > stream_gpu_res/"PCore"/$SIZE.txt

#load gcc compiler
#module load gcc/12.1.0
echo "GCC"
gcc -O3 stream_cpu.c -fopenmp -g -DSTREAM_ARRAY_SIZE=$SIZE -DSTREAM_TYPE=float -DINNERLOOP=$INNERLOOP -DNTIMES=$loopsize -mcmodel=medium -o stream_cpu_gcc
echo "AllCores"
GOMP_CPU_AFFINITY=0-23  taskset 0xFFFFFF   ./stream_cpu_gcc > stream_cpu_gcc_res/"AllCores"/$SIZE.txt
echo "ECores"

GOMP_CPU_AFFINITY=16-23 taskset 0xFF0000 ./stream_cpu_gcc > stream_cpu_gcc_res/"ECores"/$SIZE.txt
echo "PCores"

GOMP_CPU_AFFINITY=0-15 taskset 0x00FFFF ./stream_cpu_gcc > stream_cpu_gcc_res/"PCores"/$SIZE.txt
echo "ECore"

GOMP_CPU_AFFINITY=23 taskset 0x100000 ./stream_cpu_gcc > stream_cpu_gcc_res/"ECore"/$SIZE.txt
echo "PCore"


GOMP_CPU_AFFINITY=0 taskset 0x000001 ./stream_cpu_gcc > stream_cpu_gcc_res/"PCore"/$SIZE.txt
#load icc compiler
#module load intel/2022.1
echo "ICC"
icc -O3 stream_cpu.c -fopenmp -g -DSTREAM_ARRAY_SIZE=$SIZE -DSTREAM_TYPE=float -DINNERLOOP=$INNERLOOP -DNTIMES=$loopsize -mcmodel=medium -o stream_cpu_icc

GOMP_CPU_AFFINITY=0-23 taskset 0xFFFFFF ./stream_cpu_icc > stream_cpu_icc_res/"AllCores"/$SIZE.txt
echo "ECores"

GOMP_CPU_AFFINITY=16-23 taskset 0xFF0000 ./stream_cpu_icc > stream_cpu_icc_res/"ECores"/$SIZE.txt
echo "PCores"

GOMP_CPU_AFFINITY=0-15 taskset 0x00FFFF ./stream_cpu_icc > stream_cpu_icc_res/"PCores"/$SIZE.txt
echo "ECore"

taskset 0x100000 ./stream_cpu_icc > stream_cpu_icc_res/"ECore"/$SIZE.txt
echo "PCore"

GOMP_CPU_AFFINITY=0 taskset 0x000001 ./stream_cpu_icc > stream_cpu_icc_res/"PCore"/$SIZE.txt




done

