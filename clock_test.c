#include <stdio.h>
#include <time.h>
#include <math.h>

int main(int argc, char** argv){
    struct timespec curr_time[100];
    for(int i = 0; i < 100; i++){
        clock_gettime(CLOCK_REALTIME, &curr_time[i]);
    }
    long sumOfDiffs = 0;
    for(int i = 1; i < 100; i++){
        sumOfDiffs += curr_time[i].tv_nsec - curr_time[i-1].tv_nsec;
    }
    long average = sumOfDiffs / 99;
    printf("Average of %ld nanoseconds between calls\n", average);
    double frequency = 1 / (average * pow(10, -9));
    printf("Frequency: %f Hz\n", frequency);
    return 0;
}
