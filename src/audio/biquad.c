/*
 * Simple implementation of Biquad filters -- Tom St Denis
 *
 * Based on the work
 *
 * Cookbook formulae for audio EQ biquad filter coefficients
 * ---------------------------------------------------------
 * by Robert Bristow-Johnson, pbjrbj@viconet.com  a.k.a. robert@audioheads.com
 *
 * Available on the web at
 *
 * http://www.smartelectronix.com/musicdsp/text/filters005.txt
 *
 * Enjoy.
 *
 * This work is hereby placed in the public domain for all purposes, whether
 * commercial, free [as in speech] or educational, etc.  Use the code and please
 * give me credit if you wish.
 *
 * Tom St Denis -- http://tomstdenis.home.dhs.org
 *
 */

#include "biquad.h"

#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// Computes a BiQuad filter on a sample.
double biquad_apply(biquad_t * b, double sample)
{
	double result = result = b->a0 * sample +
		b->a1 * b->x1 + b->a2 * b->x2 -
		b->a3 * b->y1 - b->a4 * b->y2;

	b->x2 = b->x1;
	b->x1 = sample;

	b->y2 = b->y1;
	b->y1 = result;

	return result;
}

// Sets up a BiQuad Filter.
biquad_t biquad_create
(int type, double gain, double freq, double srate, double bw)
{
	biquad_t b;
	memset(&b, 0, sizeof(biquad_t));

	double A, omega, sn, cs, alpha, beta;
	double a0, a1, a2, b0, b1, b2;

	// setup variables
	A = pow(10, gain / 40);
	omega = 2 * M_PI * freq / srate;
	sn = sin(omega);
	cs = cos(omega);
	alpha = sn * sinh(M_LN2 /2 * bw * omega /sn);
	beta = sqrt(A + A);

	switch (type) {
	case LPF:
		b0 = (1 - cs) /2;
		b1 = 1 - cs;
		b2 = (1 - cs) /2;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case HPF:
		b0 = (1 + cs) /2;
		b1 = -(1 + cs);
		b2 = (1 + cs) /2;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case BPF:
		b0 = alpha;
		b1 = 0;
		b2 = -alpha;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case NOTCH:
		b0 = 1;
		b1 = -2 * cs;
		b2 = 1;
		a0 = 1 + alpha;
		a1 = -2 * cs;
		a2 = 1 - alpha;
		break;
	case PEQ:
		b0 = 1 + (alpha * A);
		b1 = -2 * cs;
		b2 = 1 - (alpha * A);
		a0 = 1 + (alpha /A);
		a1 = -2 * cs;
		a2 = 1 - (alpha /A);
		break;
	case LSH:
		b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
		b1 = 2 * A * ((A - 1) - (A + 1) * cs);
		b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
		a0 = (A + 1) + (A - 1) * cs + beta * sn;
		a1 = -2 * ((A - 1) + (A + 1) * cs);
		a2 = (A + 1) + (A - 1) * cs - beta * sn;
		break;
	case HSH:
		b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
		b1 = -2 * A * ((A - 1) + (A + 1) * cs);
		b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
		a0 = (A + 1) - (A - 1) * cs + beta * sn;
		a1 = 2 * ((A - 1) - (A + 1) * cs);
		a2 = (A + 1) - (A - 1) * cs - beta * sn;
		break;
	default:
		LOG(ERROR, "biquad: unrecognized filter");
		exit(EXIT_FAILURE);
	}

	// Precompute the coefficients.
	b.a0 = b0 / a0;
	b.a1 = b1 / a0;
	b.a2 = b2 / a0;
	b.a3 = a1 / a0;
	b.a4 = a2 / a0;

	// Zero initial samples.
	b.x1 = 0;
	b.x2 = 0;
	b.y1 = 0;
	b.y2 = 0;

	return b;
}
