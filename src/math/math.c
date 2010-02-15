/* 
 * math.c
 * by WN @ Feb. 15, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <math/math.h>
#include <math/trigon.c>
#include <signal.h>
#include <assert.h>
#ifdef __INTEL__
/* some very old gcc doesn't have cpuid.h */
# include <math/cpuid.h>
# ifdef __SSE__
#  include <xmmintrin.h>
# endif
#endif

bool_t __cpu_has_sse2 = FALSE;

static void ATTR(unused)
sigill_handler_sse(int num ATTR(unused))
{
	WARNING(SYSTEM, "SSE instructor cause SIGILL\n");
	THROW(EXP_NO_SSE2, "os doesn't support sse2");
}


static void
cpu_detect(void)
{
	SSE2_ENABLE() = FALSE;
#if (defined __INTEL__) && (defined __SSE__)
	unsigned int eax, ebx, ecx, edx;
	int err;

	err = __get_cpuid(0, &eax, &ebx, &ecx, &edx);
	if (err == 0) {
		DEBUG(SYSTEM, "doesn't support cpuid\n");
		return;
	}
	DEBUG(SYSTEM, "cpu vendor name: %.4s%.4s%.4s; max cpu level: %u\n",
			(char*)(&ebx), (char*)(&edx), (char*)(&ecx), eax);
	if (eax < 1) {
		DEBUG(SYSTEM, "doesn't support sse\n");
		return;
	}
	err = __get_cpuid(1, &eax, &ebx, &ecx, &edx);
	assert(err != 0);

	if (!(edx & bit_SSE2))
		return;

	DEBUG(SYSTEM, "cpu support sse2, check os support of sse2\n");
	SSE2_ENABLE() = TRUE;

	define_exp(exp);
	struct sigaction __saved_sigill;
	catch_var_nodef(struct sigaction, saved_sigill);
	TRY(exp) {
		/* Emulate test for OSFXSR in CR4.  The OS will set this bit if it
		 * supports the extended FPU save and restore required for SSE.  If
		 * we execute an SSE instruction on a PIII and get a SIGILL, the OS
		 * doesn't support Streaming SIMD Exceptions, even if the processor
		 * does.
		 */
		sigaction(SIGILL, NULL, &__saved_sigill);
		set_catched_var(saved_sigill, __saved_sigill);
		signal(SIGILL, (void (*)(int))sigill_handler_sse);
		asm volatile ("xorps %xmm0, %xmm0");
	} FINALLY {
		get_catched_var(saved_sigill);
		sigaction(SIGILL, &saved_sigill, NULL);
	} CATCH(exp) {
		print_exception(&exp);
		switch (exp.type) {
			case EXP_NO_SSE2:
				SSE2_ENABLE() = FALSE;
				break;
			default:
				RETHROW(exp);
		}
	}
#endif
	if (SSE2_ENABLE())
		DEBUG(SYSTEM, "system support sse2\n");
	else
		DEBUG(SYSTEM, "system doesn't support sse2\n");
	return;
}


void
__math_init(void)
{
	DEBUG(SYSTEM, "Init math\n");
	cpu_detect();
	__trigon_init();
}

// vim:ts=4:sw=4

