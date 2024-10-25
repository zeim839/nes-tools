#include "timerx.h"

#define G 1000000000L
#define M 1000000L

timerx_t timerx_create(uint64_t period)
{
	struct timespec res;
	clock_getres(CLOCK_MONOTONIC, &res);
	timerx_t timer = {
		.period_ns = period,
		.clock_res = res.tv_sec * G + res.tv_nsec
	};
	return timer;
}

static inline void timespec_diff
(struct timespec* a, struct timespec* b, struct timespec* result)
{
	result->tv_sec = a->tv_sec - b->tv_sec;
	result->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (result->tv_nsec < 0) {
		--result->tv_sec;
		result->tv_nsec += 1000000000L;
	}
}

void timerx_mark_start(timerx_t* timer)
{ clock_gettime(CLOCK_MONOTONIC, &timer->start); }

void timerx_mark_end(timerx_t* timer)
{
	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);
	timespec_diff(&end, &timer->start, &timer->diff);
}

int timerx_adjusted_wait(timerx_t* timer)
{
	int64_t req_period_ns = timer->period_ns -
		(timer->diff.tv_sec * G + timer->diff.tv_nsec);

	if (req_period_ns <= timer->clock_res)
		return 0;

	struct timespec req = {
		.tv_sec  = req_period_ns / G,
		.tv_nsec = req_period_ns % G
	};

	return nanosleep(&req, NULL);
}

int timerx_wait(uint64_t period_ms)
{
	int64_t req_period_ns = period_ms * M;
	struct timespec req = {
		.tv_sec  = req_period_ns / G,
		.tv_nsec = req_period_ns % G,
	};
	return nanosleep(&req, NULL);
}

double timerx_get_diff(timerx_t* timer)
{ return ((double)timer->diff.tv_sec * 1000 + (double)timer->diff.tv_nsec / M); }
