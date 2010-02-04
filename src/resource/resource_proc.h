/* 
 * resource_proc.h
 * by WN @ Dec. 25, 2009
 */

#ifndef __RESOURCE_PROC_H
#define __RESOURCE_PROC_H

#include <common/defs.h>
#include <common/functionor.h>
#include <common/cache.h>
#include <io/io.h>
#include <stdint.h>
#include <string.h>

#define MAX_IDLEN	(4096)

typedef void * (*deserializer_t)(struct io_t * io);

/* the resource process should be launched after do_init */

int
launch_resource_process(void);

void
shutdown_resource_process(void);

extern struct io_functionor_t resource_io_functionor;

void *
get_resource(const char * name,
		deserializer_t deserializer);

void
delete_resource(const char * name);

#endif

// vim:ts=4:sw=4

