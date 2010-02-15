/* 
 * trigon.c
 * by WN @ Feb. 15, 2010
 */

#include <math/trigon.h>

#include <math.h>

#define TRIGON_TABLE_SIZE	(65536*4)

static float sin_table[TRIGON_TABLE_SIZE];
static float tan_table[TRIGON_TABLE_SIZE];

#define STEP	(360.0f / (float)TRIGON_TABLE_SIZE)

static inline float
lookup_d(float X, float * table)
{
	float sig = (X > 0) ? 1.0 : -1.0;
	X = fabsf(X);
	X = ANGLEMOD(X);
	float retval = table[(int)(X / STEP)];
	return retval * sig;
}

float
sin_d(float X)
{
	 return lookup_d(X, sin_table);
}

float
cos_d(float X)
{
	return lookup_d(X + 90.0f, sin_table);
}

float
tan_d(float X)
{
	return lookup_d(X, tan_table);
}

float
sin_r(float X)
{
	 return lookup_d(RAD2DEG(X), sin_table);
}

float
cos_r(float X)
{
	return lookup_d(RAD2DEG(X + 90.0f), sin_table);
}

float
tan_r(float X)
{
	return lookup_d(RAD2DEG(X), tan_table);
}

void
__trigon_init(void)
{
	for (int i = 0; i < TRIGON_TABLE_SIZE; i++) {
		/* although libc's manual says that tan generate overflow
		 * when X near its singularities,
		 * it is not true in my machine. */
		float v = 2.0f * M_PI / (float)(TRIGON_TABLE_SIZE) * (float)(i);
		sin_table[i] = sinf(v);
		tan_table[i] = tanf(v);
	}
}



// vim:ts=4:sw=4

