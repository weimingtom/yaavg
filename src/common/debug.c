/* 
 * debug.c
 * by WN @ Nov. 08, 2009
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>


#define __DEBUG_C
#include <common/defs.h>
#include <common/debug.h>

#ifdef YAAVG_DEBUG
static const char * __debug_level_names[NR_DEBUG_LEVELS] = {
	[DBG_LV_SILENT]		= "SIL",
	[DBG_LV_TRACE]		= "TRC",
	[DBG_LV_DEBUG]		= "DBG",
	[DBG_LV_VERBOSE]	= "VRB",
	[DBG_LV_WARNING]	= "WAR",
	[DBG_LV_ERROR]		= "ERR",
	[DBG_LV_FATAL]		= "FAL",
	[DBG_LV_FORCE]		= "FOC",
};
#endif

#define get_level_name(l)	(__debug_level_names[l])
#define get_comp_name(c)	(__debug_component_names[c])
#define get_comp_level(c)	(__debug_component_levels[c])

static FILE * output_fp = NULL;
static bool_t colorful_terminal = FALSE;


/* colorful console */
enum color_name {
	COLOR_RED = 0,
	COLOR_BLUE,
	COLOR_NORMAL,
};

static const char * color_strs[] = {
	[COLOR_RED]	= "\x001b[1;31m",
	[COLOR_BLUE]	= "\x001b[1;34m",
	[COLOR_NORMAL]	= "\x001b[m",
};

static void
set_color(enum color_name c)
{
	if (!colorful_terminal)
		return;
	fprintf(output_fp, "%s", color_strs[c]);
}
/* end colorful console */

void
dbg_exit(void)
{
	if ((output_fp != NULL) &&
		(output_fp != stderr) &&
		(output_fp != stdout))
		fclose(output_fp);
	set_color(COLOR_NORMAL);
	output_fp = NULL;
}

void
dbg_init(const char * fn)
{
	int err;

	/* open output_fp for debug output */
	if ((fn == NULL) || fn[0] == '\0') {
		output_fp = stderr;
	} else {
		output_fp = fopen(fn, "a");
		/* We will install a handler for SIGABRT */
		assert(output_fp != NULL);
#ifdef HAVE_ATEXIT
		atexit(dbg_exit);
#endif
	}
	/* test the colorful terminal */
	if (isatty(fileno(output_fp)))
		colorful_terminal = TRUE;
	else
		colorful_terminal = FALSE;

	/* we close buffer of output_fp */
	err = setvbuf(output_fp, NULL, _IONBF, 0);
	if (err != 0)
		fprintf(stderr, "Failed to set _IONBF to debug output file\n");

	/* reset errno for isatty and setvbuf */
	errno = 0;

	/* install singal handlers */
}

struct msg_option {
	bool_t no_prefix;
};

void
dbg_output(enum __debug_level level,
		enum __debug_component comp,
		const char * file, const char * func, int line,
		char * fmt, ...)
{

	va_list ap;
	va_start(ap, fmt);

	if (fmt == NULL)
		return;
	if (level == DBG_LV_SILENT)
		return;

#ifdef YAAVG_DEBUG
	/* check component and level */
	if (get_comp_level(comp) > level)
		return;

	/* check fmt */
	struct msg_option option = {
		.no_prefix = FALSE,
	};
	while (*fmt == '@') {
		fmt ++;
		switch (*fmt) {
			case 'q':
				option.no_prefix = TRUE;
				fmt++;
				break;
			case '@':
				break;
			default:
				set_color(COLOR_RED);
				fprintf(output_fp, "!!! debug option char '%c' error\n",
						*fmt);
				set_color(COLOR_NORMAL);
		}
	}

	if (*fmt == ' ')
		fmt++;

	/* now fmt should point to the start of real fmt */
	if (level >= DBG_LV_WARNING) {
		if (level == DBG_LV_FORCE)
			set_color(COLOR_BLUE);
		else
			set_color(COLOR_RED);
	}

	/* print the prefix */
	if (!option.no_prefix)
		fprintf(output_fp, "[%s %s@%s:%d]\t",
				get_comp_name(comp),
				get_level_name(level),
				func,
				line);

	/* then real print */
	vfprintf (output_fp, fmt, ap);


	if (level >= DBG_LV_WARNING)
		set_color(COLOR_NORMAL);
#else
	if (level < DBG_LV_WARNING)
		return;
	/* remove '@' */
	while (*fmt == '@') {
		fmt ++;
		if (*fmt != '@')
			fmt ++;
		else
			break;
	}
	if (*fmt == ' ')
		fmt++;

	vfprintf (output_fp, fmt, ap);
#endif
	va_end(ap);
}

