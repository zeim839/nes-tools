#ifndef NES_TOOLS_BIQUAD_H
#define NES_TOOLS_BIQUAD_H

#include "../system.h"

typedef struct
{
	double a0;
	double a1;
	double a2;
	double a3;
	double a4;

	double x1;
	double x2;
	double y1;
	double y2;

} biquad_t;

enum
{
	LPF,   // Low pass filter.
	HPF,   // High pass filter.
	BPF,   // Band pass filter.
	NOTCH, // Notch Filter.
	PEQ,   // Peaking band EQ filter.
	LSH,   // Low shelf filter.
	HSH    // High shelf filter.
};

biquad_t biquad_create(int type, double gain, double freq, double srate, double bw);
double biquad_apply(biquad_t* b, double sample);

#endif // NES_TOOLS_BIQUAD_H
