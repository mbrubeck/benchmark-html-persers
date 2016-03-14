/*
 Copyright 2015 Alexander Borisov
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 Authors: insoreiges@gmail.com (Evgeny Yakovlev), lex.borisov@gmail.com (Alexander Borisov)
*/

/**
 * Platform-specific hdef performance clock value. 
 */ 

#include <stdio.h>
#include <time.h>
#include <stdint.h>

#if !defined(IS_OS_WINDOWS)
#include <unistd.h>
#endif

#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#elif defined(IS_OS_WINDOWS)
#endif

#if defined(MyHTML_FORCE_RDTSC) /* Force using rdtsc, useful for comparison */

/**
 * Get CPU rdtsc frequency.
 * 
 * TODO: I think using rdtsc for measuring user-space counters is not correct:
 * - rdtsc does not have a constant rate. instead ot is scaled to physical core's internal clock which changes due to power saving modes on modern CPUs
 * - rdtsc is software-emulated in virtual machines which will introduce an inconsistency in reported ticks
 * - user space process can be preempted between consecutive rdtsc measures but the physical clock will still tick while it is executing a different thread.
 *   also think what would happen if preempted process will be re-scheduled on a different physical core which has a different tsc value.
 * - computing rdtsc frequency produces unreliable results (due to all of the above)
 *
 * Consider using platform-specific monotonic hperf timers (ftrace/dtrace) or even clock().
 */
uint64_t myhtml_hperf_res(void)
{
#if defined(__APPLE__) && defined(CTL_HW) && defined(HW_CPU_FREQ)
    unsigned long long freq = 0;
    
    /* OSX kernel: sysctl(CTL_HW | HW_CPU_FREQ) */
    size_t len = sizeof(freq);
    int mib[2] = {CTL_HW, HW_CPU_FREQ};

    int error = sysctl(mib, 2, &freq, &len, NULL, 0);
    if (error)
        return 0;

    return freq;

#elif defined(__linux__)
    unsigned long long freq = 0;
    
    /* Use procfs on linux */
    FILE* fp = NULL;
    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL)
        return 0;

    /* Find 'CPU MHz :' */
    char buf[1024] = {0};
    double fval = 0.0;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (sscanf(buf, "cpu MHz : %lf\n", &fval) == 1) {
            freq = (unsigned long long)(fval * 1000000ull);
            break;
        }
    }

    fclose(fp);
    return freq;

#else 
#   warning Cant figure out cpu frequency on this platfrom
    
    return 0;
#endif /* defined __APPLE__ || __linux__ ... */
}

uint64_t myhtml_hperf_clock(void)
{
    uint64_t x;

    __asm__ volatile (
        "cpuid\n\t" /* cpuid serializes any out-of-order prefetches before executing rdtsc (clobbers ebx, ecx, edx) */
        "rdtsc\n\t"
        "shl $32, %%rdx\n\t"
        "or %%rdx, %%rax" 
        : "=a" (x) 
        : 
        : "rdx", "ebx", "ecx");

    return x;
}

void myhtml_hperf_print(const char *name, uint64_t x, uint64_t y, FILE *fh) {
    unsigned long long freq = myhtml_hperf_res();
    
    fprintf(fh, "%s: %0.5f\n", name, (((float)(y - x) / (float)freq)));
}

#elif defined(_POSIX_TIMERS) &&  defined(_POSIX_CPUTIME) \
    && defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199309L) /* Do we have clock_gettime? */

#define NSEC_PER_SECOND         1000000000ull
#define TIMESPEC_TO_USEC(tspec) (((uint64_t)(tspec).tv_sec * NSEC_PER_SECOND) + (tspec).tv_nsec)

uint64_t myhtml_hperf_res(void)
{
    struct timespec tspec;
    int error = clock_getres(CLOCK_PROCESS_CPUTIME_ID, &tspec);
    if (error)
        return 0;

    unsigned long long ticks_per_sec = (unsigned long long)((double)NSEC_PER_SECOND / TIMESPEC_TO_USEC(tspec));
    return ticks_per_sec;
}

uint64_t myhtml_hperf_clock(void)
{
    struct timespec tspec;
    int error = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tspec);
    if (error)
        return 0;

    return TIMESPEC_TO_USEC(tspec);
}

#elif defined(__APPLE__) && defined(__MACH__)

/* 
 * TODO: on OSX we can use clock_get_time: http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x 
 * Or this: http://web.archive.org/web/20100517095152/http://www.wand.net.nz/~smr26/wordpress/2009/01/19/monotonic-time-in-mac-os-x/comment-page-1/
 */

// TODO: this is incorrect plug for mac os x
// look at links before this comment

#include <mach/mach_time.h>

uint64_t myhtml_hperf_res(void)
{
    unsigned long long freq = 0;
    
    size_t len = sizeof(freq);
    int mib[2] = {CTL_HW, HW_CPU_FREQ};
    
    int error = sysctl(mib, 2, &freq, &len, NULL, 0);
    if (error) {
        return 0;
    }
    
    return freq;
}

uint64_t myhtml_hperf_clock(void)
{
    return mach_absolute_time();
}

double myhtml_absolute_difference(uint64_t start, uint64_t end) {
    uint64_t difference = end - start;
    static mach_timebase_info_data_t info = {0,0};
    
    if (info.denom == 0)
        mach_timebase_info(&info);
    
    uint64_t elapsednano = difference * (info.numer / info.denom);
    
    double sec = elapsednano * 1e-9;
    return sec + ((double)(elapsednano - (sec * 1e9)) / (double)1e9);
}

void myhtml_hperf_print(const char *name, uint64_t x, uint64_t y, FILE *fh) {
    
    double val = myhtml_absolute_difference(x, y);
    
    fprintf(fh, "%s: %0.5f\n", name, val);
}


#else

#   warning No hperf implementation for this platform

uint64_t myhtml_hperf_res(void)
{
    return 0;
}

uint64_t myhtml_hperf_clock(void)
{
    return 0;
}

void myhtml_hperf_print(const char *name, uint64_t x, uint64_t y, FILE *fh) {
}

#endif /* defined(MyHTML_FORCE_RDTSC) ... */


