#ifndef NES_TOOLS_TIMERX_H
#define NES_TOOLS_TIMERX_H

#include "system.h"

typedef struct
{
	struct timespec start;
	struct timespec diff;
	uint64_t        clock_res;
	uint64_t        period_ns;

} timerx_t;

timerx_t timerx_create(uint64_t period);
void timerx_mark_start(timerx_t* timer);
void timerx_mark_end(timerx_t* timer);
int timerx_adjusted_wait(timerx_t* timer);
int timerx_wait(uint64_t period_ms);
double timerx_get_diff(timerx_t* timer);

#endif // NES_TOOLS_TIMERX_H
