/* 
 * resource.h
 * by WN @ Dec. 25, 2009
 */

#ifndef __RESOURCE_H
#define __RESOURCE_H

#include <common/functionor.h>
#include <io/io.h>
#include <stdint.h>

#define MAX_IDLEN	(256)

enum resource_type {
	RESOURCE_BITMAP,
};

typedef void * (*deserializer_t)(struct io_t * io);

/* the resource process can be launched after do_init */

int
launch_resource_process(void);

void
shutdown_resource_process(void);

extern struct io_functionor_t resource_io_functionor;

void *
get_resource(const char * name,
		deserializer_t deserializer);

#endif

// vim:ts=4:sw=4

