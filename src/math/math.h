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

#endif


