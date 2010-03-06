/* 
 * math.h
 * by WN @ Feb.  15, 2010
 */

#ifndef __YAAVG_MATH_H
#define __YAAVG_MATH_H

#include <config.h>
#include <common/defs.h>

#if defined(_M_IX86) || defined(i386) || defined(__i386) || defined(__i386__)
# ifndef __INTEL__
#  define __INTEL__
# endif
# ifndef __X86_32__
#  define __X86_32__
# endif
#endif /* x86 */

extern bool_t __cpu_has_sse2;
#define SSE2_ENABLE()	__cpu_has_sse2

/* used in init_cleanup_list */
extern void __math_init();

#define interp(l1, r1, c1, l2, r2) ({	\
		float ___l1 = (float)(l1);\
		float ___r1 = (float)(r1);\
		float ___c1 = (float)(c1);\
		float ___l2 = (float)(l2);\
		float ___r2 = (float)(r2);\
		(___c1 - ___l1) / (___r1 - ___l1) * (___r2 - ___l2) + ___l2;\
		})

#endif


