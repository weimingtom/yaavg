/* By Wang Nan @ Jan 26, 2009 */

/* 
 * rlist.c
 *
 * Defines rlist operations. See video.tex for detail
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <video/rlist.h>
#include <video/rcommand.h>
#include <common/debug.h>



/* 
 * init_rlist - initialize a render list.
 *
 * Program always has only 1 render list, therefore I don't define a
 * alloc_rlist.
 */
void init_rlist(struct RenderList * rlist, void * context)
{
	rlist->context = context;
	INIT_LIST_HEAD(&rlist->command_list);
}

/* 
 * sprint_rlist - output whole rlist.
 *
 * This is a debug facility.
 */
int sprint_rlist(char * str, struct RenderList * rlist)
{
	char * p = str;
	struct RenderCommand * pos;

	p += sprintf(p, "Render list %p:\n", rlist);

	list_for_each_entry(pos, &rlist->command_list, list) {
		if (pos->name != NULL)
			p += sprintf(p, "Render command %s: ", pos->name);
		else
			p += sprintf(p, "Render command %p: ", pos);

		if (pos->sprint != NULL)
			p += pos->sprint(pos, p);
		else
			p += sprintf(p, "(no sprint func)\n");
	}

	p += sprintf(p, "End");
	return (p - str) / sizeof(char);
}


#if 1
static void init_rc(struct RenderCommand * rc)
{
	memset(rc, 0, sizeof(*rc));
	return;
}

int main()
{
	char buf[256];
	struct RenderList list;
	struct RenderCommand rc1, rc2;

	init_rc(&rc1);
	init_rc(&rc2);

	rc2.name = "Test RC";

	init_rlist(&list, NULL);
	link_tail(&list, &rc1);
	link_tail(&list, &rc2);

	sprint_rlist(buf, &list);
	printf("%s\n", buf);
	return 0;
}
#endif
