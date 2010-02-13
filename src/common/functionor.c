/* 
 * functionor.c
 * by WN @ Nov. 27, 2009
 */

#include <common/functionor.h>
#include <common/exception.h>
#include <common/debug.h>

#include <assert.h>

#define SEQ_PART(x) ((x) & 0xffff)
static const char * function_class_names[NR_FCS] = {
	[SEQ_PART(FC_VIDEO)] = "video",
	[SEQ_PART(FC_OPENGL_DRIVER)] = "OpenGL engine",
	[SEQ_PART(FC_BITMAP_RESOURCE_HANDLER)] = "bitmap resource handler",
	[SEQ_PART(FC_TIMER)] = "timer handler",
	[SEQ_PART(FC_IO)] = "IO handler",
};

struct functionor_t *
find_functionor(struct function_class_t * fclass, const char * param)
{
	assert(fclass != NULL);
	struct functionor_t ** pos = *(fclass->functionors);
	if (fclass->current_functionor != NULL) {
		if (fclass->fclass & UNIQUE_FUNCTIONOR) {
			WARNING(SYSTEM, "calling find_functionor with an already inited class %d, param=\"%s\"\n",
					fclass->fclass, param);
			return fclass->current_functionor;
		}
	}
	while (*pos != NULL) {
		assert((*pos)->fclass == fclass->fclass);
		if ((*pos)->check_usable != NULL)
			if ((*pos)->check_usable(param)) {
				fclass->current_functionor = *pos;
				return *pos;
			}
		pos ++;
	}
	THROW_FATAL(EXP_FUNCTIONOR_NOT_FOUND,
			"unable to find suitable functionor for class \"%s\" "
			" with param \"%s\"",
			function_class_names[SEQ_PART(fclass->fclass)], param);
	return NULL;
}


// vim:ts=4:sw=4

