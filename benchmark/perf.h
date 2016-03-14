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
 
 Authors: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef perf_h
#define perf_h

#include <stdint.h>

/**
 * Platform-specific hdef performance clock queries.
 * Implemented in perf.c
 */

/** Get current value in clock ticks */
uint64_t myhtml_hperf_clock(void);

/** Print an hperf measure */
void myhtml_hperf_print(const char *name, uint64_t x, uint64_t y, FILE *fh);
double myhtml_absolute_difference(uint64_t start, uint64_t end);

#endif /* perf_h */
