/* 
 * exception.h
 * by WN @ Nuv. 16, 2009
 */
/* 
 * support for TRY---CATCH---FINALLY.
 *
 * THIS IS NOT C++, NEVER RETURN IN TRY BLOCK!!!!!
 */

#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include <config.h>
#include <common/defs.h>
#include <stdint.h>
#include <setjmp.h>


__BEGIN_DECLS

enum exception_type {
	EXP_NO_EXCEPTION,
#define def_exp_type(a, b)	a,
#include <common/exception_types.h>
#undef def_exp_type
	EXP_UNCATCHABLE,
	NR_EXP_TYPES,
};

enum video_exception {
	VIDEXP_NOEXP,
	VIDEXP_RERENDER,
	VIDEXP_SKIPFRAME,
	VIDEXP_REINIT,
	VIDEXP_FATAL,
};

/* **NOTE**
 *
 * although we use specific exception here to show the means of
 * each exception, the relationship between an exception and
 * its level is not fixed. for example, reading from a package and
 * fail generate EXP_BAD_RESOURCE, indecates the corruption of
 * package. read from a physical file and fail also generate
 * EXP_BAD_RESOURCE, but this time the exception may indecates
 * a file system failure.
 * */
enum exception_level {
	EXP_LV_NONE,
	/*
	 * the environment is incorrect, but can workaround,
	 * internal state is safe.
	 * for example:
	 * 		EXP_CORRUPTED_CONF
	 * 		EXP_BAD_RESOURCE
	 * 		EXP_RESOURCE_NOT_FOUND
	 * */
	EXP_LV_LOWEST,
	/* internal state is not safe, but can continue. for example:
	 * 		EXP_DICT_FULL */
	EXP_LV_TAINTED,
	/* cannot continue, do possible cleanup and quit. for example:
	 *		EXP_RESOURCE_PROCESS_FAILURE; 
	 * */
	EXP_LV_FATAL,
	/* reserved for EXP_UNCATCHABLE. */
	EXP_LV_UNCATCHABLE,
	NR_EXP_LEVELS,
};

#define EXCEPTION_MSG_LEN_MAX	(512)

struct exception_t {
	enum exception_type type;
	char msg[EXCEPTION_MSG_LEN_MAX];
	union {
		int val;
		void * ptr;
		uintptr_t xval;
		enum exception_type video_exception;
	} u;
	enum exception_level level;
#ifdef YAAVG_DEBUG
	const char * file;
	const char * func;
	int line;
#endif
};


#define define_exp(x)		volatile struct exception_t x
/* this definition can give programmer a hint that he forgot to call set_catched_var. */
/* don't use ___should_set and ___should_get directly */
/* for those ver which won't get inited, we should init ___catched_##name to def. but def
 * should be evaluated only once. */
#define catch_var(type, name, def)			\
	type volatile ___catched_##name; 		\
	int ___should_set_catched_##name; \
	int ___should_get_catched_##name; \
	type name;								\
	do {type ___tmp_##name = (def);			\
		name = ___tmp_##name;				\
		___catched_##name = ___tmp_##name;	\
	} while(0);

#define set_catched_var(name, v)	\
	do {	\
		___should_set_catched_##name = 0;	\
		___catched_##name = name = v; 	\
	} while(0)
#define get_catched_var(name) \
	do { \
		___should_get_catched_##name = 0;	\
		name = ___catched_##name;\
	} while(0)


#if defined(HAVE_SIGSETJMP)
#define EXCEPTIONS_SIGJMP_BUF		sigjmp_buf
#define EXCEPTIONS_SIGSETJMP(buf)	sigsetjmp(((buf)), 1)
#define EXCEPTIONS_SIGLONGJMP(buf,val)	siglongjmp((buf), (val))
#else
#define EXCEPTIONS_SIGJMP_BUF		jmp_buf
#define EXCEPTIONS_SIGSETJMP(buf)	setjmp((buf))
#define EXCEPTIONS_SIGLONGJMP(buf,val)	longjmp((buf), (val))
#endif

enum catcher_action {
	CATCH_ITER_0,
	CATCH_FINALLY,
	CATCH_THROW,
};
enum catcher_state {
	CATCHER_INIT,
	CATCHER_EXEC_BODY,
	CATCHER_WAIT_FINALLY,
	CATCHER_THROWN,
};

struct catcher_t {
	EXCEPTIONS_SIGJMP_BUF buf;
	enum catcher_state state;
	struct exception_t * curr_exp;
	struct catcher_t * prev;
};

extern bool_t
exceptions_state_mc(enum catcher_action);
extern EXCEPTIONS_SIGJMP_BUF *
exceptions_state_mc_init(struct catcher_t * catcher,
		volatile struct exception_t * exp);

#define TRY(___exp) \
	<% \
	struct catcher_t ____catcher; \
	____catcher.curr_exp = (struct exception_t *)&(___exp); \
	EXCEPTIONS_SIGJMP_BUF * ___buf___ = \
		exceptions_state_mc_init(&____catcher, &(___exp));	\
	EXCEPTIONS_SIGSETJMP(*___buf___);	\
	while (exceptions_state_mc(CATCH_ITER_0)) 

#define FINALLY { exceptions_state_mc(CATCH_FINALLY); }
#define NO_FINALLY FINALLY
#define CATCH(___exp) %> if ((___exp).type != EXP_NO_EXCEPTION)
#define NO_CATCH(___exp) CATCH(___exp) RETHROW(___exp)
/* hint the programmer that he forget to call set_catched_var */

extern void
print_exception(volatile struct exception_t * exp);

extern NORETURN ATTR_NORETURN void
throw_exception(enum exception_type type,
		uintptr_t val,
		enum exception_level level,
#ifdef YAAVG_DEBUG
		const char * file,
		const char * func,
		int line,
#endif
		const char * fmt, ...)
#ifdef YAAVG_DEBUG
ATTR(format(printf, 7, 8))
#else
ATTR(format(printf, 4, 5))
#endif
	;

#ifdef YAAVG_DEBUG
# define __dbg_info __FILE__, __FUNCTION__, __LINE__,
#else
# define __dbg_info
#endif

#define THROW(t, fmt...) throw_exception(t, 0, EXP_LV_LOWEST, __dbg_info fmt)
#define THROW_VAL(t, v, fmt...) throw_exception(t, (uintptr_t)(v), EXP_LV_LOWEST, __dbg_info fmt)

#define THROW_TAINTED(t, fmt...) throw_exception(t, 0, EXP_LV_TAINTED, __dbg_info fmt)
#define THROW_VAL_TAINTED(t, v, fmt...) throw_exception(t, (uintptr_t)(v), EXP_LV_TAINTED, __dbg_info fmt)

#define THROW_FATAL(t, fmt...) throw_exception(t, 0, EXP_LV_FATAL, __dbg_info fmt)
#define THROW_VAL_FATAL(t, v, fmt...) throw_exception(t, (uintptr_t)(v), EXP_LV_FATAL, __dbg_info fmt)


#define NOTHROW(fn, ...) do {	\
	define_exp(___exp);	\
	TRY(___exp) {					\
		fn(__VA_ARGS__);		\
	} NO_FINALLY				\
	CATCH(___exp) {						\
		switch(___exp.level) {		\
			case EXP_LV_LOWEST:	\
			case EXP_LV_TAINTED:\
				break;			\
			default:			\
				print_exception(&___exp);	\
				RETHROW(___exp);	\
		}						\
	}							\
} while(0)


#define NOTHROW_RET(defret, fn, ...) ({	\
	typeof((fn)(__VA_ARGS__)) ___r;\
	define_exp(___exp);	\
	TRY(___exp) {					\
		___r = (fn)(__VA_ARGS__);\
	} NO_FINALLY				\
	CATCH(___exp) {						\
		___r = defret;			\
		switch(___exp.level) {	\
			case EXP_LV_LOWEST:	\
			case EXP_LV_TAINTED:\
				break;			\
			default:			\
				print_exception(&___exp);	\
				RETHROW(___exp);\
								\
		}						\
	}							\
	___r;						\
})


#ifdef YAAVG_DEBUG
# define RETHROW(e) throw_exception((e).type, (e).u.xval, (e).level, \
		(e).file, (e).func, (e).line, "%s", (char*)&((e).msg))
#else
# define RETHROW(e) throw_exception((e).type, (e).u.xval, (e).level, \
		"%s", (char*)&((e).msg))
#endif

#define THROWS(...)

__END_DECLS
#endif
// vim:ts=4:sw=4

