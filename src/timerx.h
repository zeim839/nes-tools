#ifndef NES_TOOLS_TIMERX_H
#define NES_TOOLS_TIMERX_H

#include "system.h"

// timerx_t implements a timer that measures the execution time of
// emulator routines and then sleeps for the remaining cycle time.
typedef struct
{
	struct timespec start;
	struct timespec diff;
	uint64_t        clock_res;
	uint64_t        period_ns;

} timerx_t;

// timerx_create returns an initialized timerx with the given cycle
// period in nanoseconds.
timerx_t timerx_create(uint64_t period);

// timerx_mark_start marks the start of a cycle period.
void timerx_mark_start(timerx_t* timer);

// timerx_mark_end marks the end of a cycle period.
void timerx_mark_end(timerx_t* timer);

// timerx_adjusted_wait sleeps for (period - execution time)
// nanoseconds.
int timerx_adjusted_wait(timerx_t* timer);

// timerx_wait waits for the specified period (in milliseconds).
int timerx_wait(uint64_t period_ms);

// timerx_get_diff returns the elapsed time between timerx_mark_start
// and timerx_mark_end.
double timerx_get_diff(timerx_t* timer);

#endif // NES_TOOLS_TIMERX_H
