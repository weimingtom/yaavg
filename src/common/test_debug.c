/* 
 * test_debug.c
 * by WN @ Nov. 08, 2009
 */

#include <stdlib.h>
#include <common/debug.h>

int main()
{
	dbg_init(NULL);
	/* test use debug output directly */
	dbg_output(DBG_LV_TRACE, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system trace message with argument %d\n", 1000);
	dbg_output(DBG_LV_DEBUG, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system debug message with argument %d\n", 1000);
	dbg_output(DBG_LV_VERBOSE, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system verbose message with argument %d\n", 1000);
	dbg_output(DBG_LV_WARNING, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system warning message with argument %d\n", 1000);
	dbg_output(DBG_LV_ERROR, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system error message with argument %d\n", 1000);
	dbg_output(DBG_LV_FATAL, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system fatal message with argument %d\n", 1000);
	dbg_output(DBG_LV_FORCE, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system force message with argument %d\n", 1000);
	dbg_output(DBG_LV_SILENT, SYSTEM, __FILE__, __FUNCTION__, __LINE__,
			"system silent message with argument %d\n", 1000);

	dbg_output(DBG_LV_TRACE, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory trace message with argument %d\n", 1000);
	dbg_output(DBG_LV_DEBUG, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory debug message with argument %d\n", 1000);
	dbg_output(DBG_LV_VERBOSE, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory verbose message with argument %d\n", 1000);
	dbg_output(DBG_LV_WARNING, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory warning message with argument %d\n", 1000);
	dbg_output(DBG_LV_ERROR, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory error message with argument %d\n", 1000);
	dbg_output(DBG_LV_FATAL, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory fatal message with argument %d\n", 1000);
	dbg_output(DBG_LV_FORCE, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory force message with argument %d\n", 1000);
	dbg_output(DBG_LV_SILENT, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"memory silent message with argument %d\n", 1000);

	dbg_output(DBG_LV_DEBUG, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"@q test memory quiet message with argument %d\n", 1000);
	dbg_output(DBG_LV_FORCE, MEMORY, __FILE__, __FUNCTION__, __LINE__,
			"@q test memory force quiet message with argument %d\n", 1000);
	return 0;
}

