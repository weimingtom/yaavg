/* 
 * functionor.c
 * by WN @ Nov. 27, 2009
 */

#include <common/functionor.h>
#include <common/exception.h>

static const char * function_class_names[NR_FCS] = {
	[FC_VIDEO] = "video",
	[FC_OPENGL_ENGINE] = "OpenGL engine",
	[FC_BITMAP_HANDLER] = "bitmap handler",
	[FC_TIMER] = "timer handler",
	[FC_RESOURCES] = "resource handler",
};

struct functionor_t *
find_functionor(struct function_class_t * fclass, const char * param)
{
	struct functionor_t ** pos = *(fclass->functionors);
	while (*pos != NULL) {
		if ((*pos)->check_usable != NULL)
			if ((*pos)->check_usable(param)) {
				fclass->current_functionor = *pos;
				return *pos;
			}
		pos ++;
	}
	THROW(EXP_FUNCTIONOR_NOT_FOUND,
			"unable to find suitable functionor for class \"%s\" "
			" with param \"%s\"",
			function_class_names[fclass->fclass], param);
	return NULL;
}


// vim:ts=4:sw=4

