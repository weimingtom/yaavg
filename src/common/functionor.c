/* 
 * functionor.c
 * by WN @ Nov. 27, 2009
 */

#include <common/functionor.h>
#include <common/exception.h>

static const char * function_class_names[NR_FCS] = {
	[FC_DISPLAY] = "display",
	[FC_OPENGL_ENGINE] = "OpenGL engine",
	[FC_PIC_LOADER] = "picture loader",
};

void
register_functionor(struct function_class_t * fclass, struct functionor_t * func)
{
	list_add(&(func->list), &(fclass->functionor_list));
}

struct functionor_t *
find_functionor(struct function_class_t * fclass, const char * param)
{
	struct functionor_t * pos;
	list_for_each_entry(pos, &fclass->functionor_list, list) {
		if (pos->check_usable(param))
			return pos;
	}
	THROW(EXP_FUNCTIONOR_NOT_FOUND,
			"unable to find suitable functionor for class \"%s\" with param %s",
			function_class_names[fclass->fclass], param);
	return NULL;
}


// vim:ts=4:sw=4
