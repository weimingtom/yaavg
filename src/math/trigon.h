/* 
 * trigon.h
 * by WN @ Jun. 20, 2009
 */

#ifndef __TRIGON_H
#define __TRIGON_H

#include <common/defs.h>
#include <math.h>

#define DEG2RAD(a) ((a) * ((float) M_PI / 180.0f))
#define RAD2DEG(a) ((a) * (180.0f / (float) M_PI))
#define ANGLEMOD(a) ((a) - 360.0 * floorf((a) / 360.0))

/* d means degree */
extern float sin_d(float X) ATTR(pure);
extern float cos_d(float X) ATTR(pure);
extern float tan_d(float X) ATTR(pure);

extern float sin_r(float X) ATTR(pure);
extern float cos_r(float X) ATTR(pure);
extern float tan_r(float X) ATTR(pure);


#endif
