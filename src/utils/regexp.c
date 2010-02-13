/* 
 * regexp.c
 * by WN @ Feb. 12, 2010
 */

#include <common/defs.h>
#include <utils/regexp.h>
#include <regex.h>

bool_t
match(const char * regexp, const char * string)
{
	int err;
	if (regexp == NULL)
		return TRUE;
	if (string == NULL)
		return FALSE;

	regex_t reg;
	err = regcomp(&reg, regexp, REG_NOSUB);
	assert(err == 0);

	err = regexec(&reg, string, 0, NULL, 0);
	regfree(&reg);
	if (err == 0)
		return TRUE;
	return FALSE;
}

// vim:ts=4:sw=4

