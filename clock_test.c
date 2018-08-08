#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

static __inline__ uint64_t rdtsc(void)
{
    uint64_t x;
    __asm__ volatile ("rdtsc" : "=A" (x));
    return x;
}

int main(int argc, char** argv){
    struct timespec curr_time[100], timeres;
    uint64_t tscTimes[100];
    int num = 0;
    for(int i = 0; i < 100; i++){
        clock_gettime(CLOCK_MONOTONIC, &curr_time[i]);
//        for(int j = 0; j < 675; j++){
//            num += 1;
 //       }
    }
    for(int i = 0; i < 100; i++){
        tscTimes[i] = rdtsc();
    }
    long sumOfDiffs = 0;
    uint64_t tscDiffs = 0;
    for(int i = 1; i < 100; i++){
        sumOfDiffs += curr_time[i].tv_nsec - curr_time[i-1].tv_nsec;
        tscDiffs += tscTimes[i] - tscTimes[i - 1];
    }
    clock_getres(CLOCK_MONOTONIC_COARSE, &timeres);
    printf("Resolution: %ld sec, %ld nsec\n", timeres.tv_sec, timeres.tv_nsec);
    long average = sumOfDiffs / 99;
    uint64_t tscAvg = tscDiffs / 99;
    printf("Clock_gettime: Average of %ld nanoseconds between calls\n", average);
    double frequency = 1 / (average * pow(10, -9));
    double tscFreq = 1 / (tscAvg * pow(10, -9));
    printf("Frequency: %f Hz\n", frequency);
    printf("RDTSC: Average of %ld nanoseconds between calls\n", tscAvg );
    printf("Frequency: %f Hz\n", tscFreq);
    printf("Num: %d\n", num);
    return 0;
}
