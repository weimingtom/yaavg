/* Wang Nan @ Jan 24, 2009 */
#ifndef YAAVG_DEBUG_H
#define YAAVG_DEBUG_H

#include <sys/cdefs.h>
#include <common/defs.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>

__BEGIN_DECLS

enum debug_level {
	SILENT = 0,
	TRACE,
	VERBOSE,
	WARNING,
	ERROR,
	FATAL,
	FORCE,
	NR_DEBUG_LEVELS
};

enum debug_component {
	RCOMMAND = 0,
	RLIST,
	VIDEO,
	SYSTEM,
	MEMORY,
	OPENGL,
	SDL,
	GLX,
	RESOURCE,
	NR_COMPONENTS
};

#ifdef YAAVG_DEBUG_C

static const char * debug_comp_name[NR_COMPONENTS] = {
	[RCOMMAND] = "VCOM",
	[RLIST] = "VLST",
	[SYSTEM] = "SYS ",
	[MEMORY] = "MEM ",
	[OPENGL] = "GL  ",
	[SDL] = "SDL ",
	[GLX] = "GLX ",
	[VIDEO]="VID ",
	[RESOURCE]="RES "
};

static enum debug_level debug_levels[NR_COMPONENTS] = {
	[RCOMMAND] = WARNING,
	[RLIST] = WARNING,
	[MEMORY] = WARNING,
	[SYSTEM] = VERBOSE,
	[OPENGL] = TRACE,
	[SDL] = WARNING,
	[VIDEO] = TRACE,
	[RESOURCE] = TRACE,
};
#endif

extern void debug_init(const char * filename);
#ifndef YAAVG_DEBUG_OFF
extern void debug_out(int prefix, enum debug_level level, enum debug_component comp,
	       const char * func_name, int line_no,
	       const char * fmt, ...);
extern void set_comp_level(enum debug_component comp, enum debug_level level);
extern enum debug_level get_comp_level(enum debug_component comp);
#else
# define debug_out(z, a, b, c, d, e,...) do{} while(0)
# define set_comp_level(a, b)   do {} while(0)
# define get_comp_level(a)   ({SILENT;})
#endif

#define DEBUG_INIT(file)	do { debug_init(file);} while(0)
#define DEBUG_MSG(level, comp, str...) do{ debug_out(1, level, comp, __FUNCTION__, __LINE__, str); } while(0)
/*  Don't output prefix */
#define DEBUG_MSG_CONT(level, comp, str...) do{ debug_out(0, level, comp, __FUNCTION__, __LINE__, str); } while(0)
#define DEBUG_SET_COMP_LEVEL(mask, level) do { set_comp_level(mask, level); } while(0)

/* XXX */
/* Below definition won't distrub the debug level, because 
 * they are all func-like macro */
#define TRACE(comp, str...) DEBUG_MSG(TRACE, comp, str)
#ifndef YAAVG_DEBUG_OFF
# define VERBOSE(comp, str...) DEBUG_MSG(VERBOSE, comp, str)
# define WARNING(comp, str...) DEBUG_MSG(WARNING, comp, str)
# define WARNING_CONT(comp, str...) DEBUG_MSG_CONT(WARNING, comp, str)
# define ERROR(comp, str...) DEBUG_MSG(ERROR, comp, str)
# define FATAL(comp, str...) DEBUG_MSG(FATAL, comp, str)
# define FORCE(comp, str...) DEBUG_MSG(FORCE, comp, str)
#else
extern void message_out(int prefix, enum debug_level, enum debug_component, char * fmt, ...);
# define VERBOSE(c, s...)	message_out(1, VERBOSE, c, s)
# define WARNING(c, s...)	message_out(1, WARNING, c, s)
# define WARNING_CONT(c, s...)	message_out(0, WARNING, c, s)
# define ERROR(c, s...)		message_out(1, ERROR, c, s)
# define FATAL(c, s...)		message_out(1, FATAL, c, s)
# define FORCE(c, s...)		message_out(1, FORCE, c, s)
#endif


/* internal_error: raise a SIGABRT. */
extern void
internal_error(enum debug_component comp,
		const char * func_name,
		int line_no, const char * fmt, ...)
		ATTR(__noreturn__);


#define INTERNAL_ERROR(comp, str...) \
	do {internal_error(comp, __FUNCTION__, __LINE__, str);}while(0)


/* Define bug functions. Reference: assert.h */
/* Don't use such a BUG_ON facility, use assert is enough.
 * we use signal handler to print backtrace. */
#if 0
extern void __bug_on(const char * __assertion, const char * __file,
		unsigned int __line, const char * __function)
	__attribute__((__noreturn__));

#define BUG_ON(expr)	\
	((expr))	\
	 ? __bug_on(__STRING(expr), __FILE__, __LINE__, __FUNCTION__)	\
	 : (void)0
#endif


#ifndef YAAVG_DEBUG_OFF
/* memory leak detection */
extern void * yaavg_malloc(size_t size);
extern void yaavg_free(void * ptr);
extern char * yaavg_strdup(const char * S);
extern void * yaavg_calloc(size_t count, size_t eltsize);
extern void show_mem_info();
#ifndef YAAVG_DEBUG_C
# ifdef malloc
#  undef malloc
# endif
# define malloc(s)	yaavg_malloc(s)

# ifdef free
#  undef free
# endif
# define free(p)	yaavg_free(p)

# ifdef strdup
#  undef strdup
# endif
# define strdup(S)	yaavg_strdup(S)

# ifdef calloc
#  undef calloc
# endif
# define calloc(C, S)	yaavg_calloc(C, S)
#endif
#else	/* YAAVG_DEBUG_OFF */
#define show_mem_info()	do {} while(0)
#endif

__END_DECLS


#endif

