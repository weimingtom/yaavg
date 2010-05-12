#include <config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include <common/debug.h>
#include <common/exception.h>

static const char * exception_names[] = {
	[EXP_NO_EXCEPTION] = "no exception",
#define def_exp_type(a, b) \
		[a] = #a,
#include <common/exception_types.h>
#undef def_exp_type
	[EXP_UNCATCHABLE] = "uncatchable exception",
};

static const char * exception_level_names[] DEBUG_DEF = {
	[EXP_LV_NONE] = "system is okay",
	[EXP_LV_LOWEST] = "system is safe",
	[EXP_LV_TAINTED] = "system is tainted",
	[EXP_LV_FATAL] = "system is in dangerous",
	[EXP_LV_UNCATCHABLE] = "system is dead",
};

/* 5 states of the catcher:
 *
 *                         +----------------(finally, cont)------------------------+
 *                         |                                                       V
 * INIT -(iter, cont)-> EXEC_BODY -(iter, stop)-> WAIT_FINALLY -(finally, cont)-> END
 *                         |                          ^
 *                    (throw, cont)                   |
 *                         |    ------(iter, stop)----+
 *                         V   /
 *                      THROWN
 * */

/* use a pointer to support potential multi-threading */
static struct catcher_t * curr_catcher_p = NULL;

static void
push_catcher(struct catcher_t * new_catcher)
{
	new_catcher->prev = curr_catcher_p;
	curr_catcher_p = new_catcher;
}

static void
pop_catcher(void)
{
	assert(curr_catcher_p != NULL);
	curr_catcher_p = curr_catcher_p->prev;
}

static void
init_set_exception(struct catcher_t * c, struct exception_t * e)
{
	memset((void*)e, '\0', sizeof(*e));
	memset(c, '\0', sizeof(*c));
	e->type = EXP_NO_EXCEPTION;
	e->level = EXP_LV_NONE;
	c->state = CATCHER_INIT;
	c->curr_exp = e;
}

EXCEPTIONS_SIGJMP_BUF *
exceptions_state_mc_init(struct catcher_t * c,
		volatile struct exception_t * e)
{
	init_set_exception(c, (struct exception_t *)e);
	push_catcher(c);
	return &(c->buf);
}

bool_t
exceptions_state_mc(enum catcher_action action)
{
	struct catcher_t * c = curr_catcher_p;
	assert(c != NULL);

#define MC_FAULT FATAL(SYSTEM, "exp mc: stat=%d(act=%d)\n", c->state, action); break;

	switch (c->state) {
		case CATCHER_INIT: {
			switch (action) {
				case CATCH_ITER_0:
					c->state = CATCHER_EXEC_BODY;
					return TRUE;
				default:
					MC_FAULT;
			}
		}
		case CATCHER_EXEC_BODY: {
			switch (action) {
				case CATCH_ITER_0:
					c->state = CATCHER_WAIT_FINALLY;
					return FALSE;
				case CATCH_THROW:
					c->state = CATCHER_THROWN;
					return FALSE;
				case CATCH_FINALLY:
					pop_catcher();
					return TRUE;
				default:
					MC_FAULT;
			}
		}
		case CATCHER_WAIT_FINALLY: {
			switch (action) {
				case CATCH_FINALLY: {
					pop_catcher();
					return TRUE;
				}
				default:
					MC_FAULT;
			}
		}
		case CATCHER_THROWN: {
			switch (action) {
				case CATCH_ITER_0: {
					c->state = CATCHER_WAIT_FINALLY;
					return FALSE;
				}
				default:
					MC_FAULT;
			}
		}
	}
	FATAL(SYSTEM, "exception state machine in unknown state\n");
#undef MC_FAULT
	return FALSE;
}


void
print_exception(volatile struct exception_t * __exp)
{
	struct exception_t * exp = (struct exception_t *)__exp;
#ifdef YAAVG_DEBUG
	/* enum us unsigned, so >= 0 */
	assert(exp->level < NR_EXP_LEVELS);
	assert(exp->type < NR_EXP_TYPES);
	WARNING(SYSTEM, "(%s): %s raised at file %s [%s:%d]:\n",
			exception_level_names[exp->level],
			exception_names[exp->type],
			exp->file, exp->func, exp->line);
#else
	WARNING(SYSTEM, "%s raised:\n", exception_names[exp->type]);
#endif
	WARNING(SYSTEM, "\tmessage: %s\n", exp->msg);
	WARNING(SYSTEM, "\twith value: %d(%p)\n", exp->u.val, exp->u.ptr);
	WARNING(SYSTEM, "\twith errno: %d(%s)\n", exp->throw_time_errno,
			strerror(exp->throw_time_errno));
	WARNING(SYSTEM, "\twith errno(now): %d(%s)\n", errno, strerror(errno));
}

NORETURN ATTR_NORETURN 
static void
__throw_exception(enum exception_type type,
		uintptr_t val,
		enum exception_level level,
#ifdef YAAVG_DEBUG
		const char * file,
		const char * func,
		int line,
#endif
		const char * fmt, va_list ap)
{
	struct exception_t exp;

	exp.type = type;

	vsnprintf(exp.msg, sizeof(exp.msg), fmt, ap);

	exp.u.ptr = (void*)val;
	exp.type = type;
	exp.level = level;
	if (level >= EXP_LV_UNCATCHABLE) {
		exp.type = EXP_UNCATCHABLE;
		exp.level = EXP_LV_UNCATCHABLE;
	}
	exp.throw_time_errno = errno;
	errno = 0;
#ifdef YAAVG_DEBUG
	exp.file = file;
	exp.func = func;
	exp.line = line;
#endif
	if (curr_catcher_p == NULL) {
		print_exception(&exp);
		FATAL(SYSTEM, "throw exception from outmost scope\n");
	}

	*(curr_catcher_p->curr_exp) = exp;

	exceptions_state_mc(CATCH_THROW);
	/* do the jump! */
	EXCEPTIONS_SIGLONGJMP(curr_catcher_p->buf, type);
}

NORETURN ATTR_NORETURN 
#ifdef YAAVG_DEBUG
ATTR(format(printf, 7, 8))
#else
ATTR(format(printf, 4, 5))
#endif
void
throw_exception(enum exception_type type,
		uintptr_t val,
		enum exception_level level,
#ifdef YAAVG_DEBUG
		const char * file,
		const char * func,
		int line,
#endif
		const char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
#ifdef YAAVG_DEBUG
	__throw_exception(type, val, level,
			file, func, line, fmt, ap);
#else
	__throw_exception(type, val, level,
			fmt, ap);
#endif
	va_end(ap);
}

NORETURN ATTR_NORETURN 
#ifdef YAAVG_DEBUG
ATTR(format(printf, 6, 7))
#else
ATTR(format(printf, 3, 4))
#endif
void
throw_video_exception(
		enum video_exception video_lv,
		uint32_t stamp,
#ifdef YAAVG_DEBUG
		const char * file,
		const char * func,
		int line,
#endif
		const char * fmt, ...)
{
	static bool_t old_timestamp_set = FALSE;
	static uint32_t old_timestamp = 0;
	static enum video_exception old_video_lv = VIDEXP_NOEXP;

	if (!old_timestamp_set) {
		old_timestamp_set = TRUE;
		old_timestamp = stamp;
	}

	if (stamp - old_timestamp > 1000) {
		old_video_lv = video_lv;
	} else {
		old_video_lv ++; 
	}
	old_timestamp = stamp;
	enum exception_level explv = EXP_LV_LOWEST;
	if (old_video_lv == VIDEXP_REINIT)
		explv = EXP_LV_TAINTED;
	else if (old_video_lv >= VIDEXP_FATAL)
		explv = EXP_LV_FATAL;

	va_list ap;
	va_start(ap, fmt);
#ifdef YAAVG_DEBUG
	__throw_exception(EXP_VIDEO_EXCEPTION,
			old_video_lv, explv,
			file, func, line, fmt, ap);
#else
	__throw_exception(EXP_VIDEO_EXCEPTION,
			old_video_lv, explv,
			fmt, ap);
#endif
	va_end(ap);
}



// vim:ts=4:sw=4

