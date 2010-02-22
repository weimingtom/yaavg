/* 
 * rect.h
 * by WN @ Feb. 07, 2010
 */

#ifndef __RECT_H
#define __RECT_H

#include <common/defs.h>

struct point_t { int x, y; };
struct point_f_t { float x, y; };
struct point_d_t { double x, y; };

#define POINT_FMT	"(%d, %d)"
#define POINTF_FMT	"(%f, %f)"
#define POINTD_FMT	"(%lf, %lf)"
#define POINT_ARG(p)  (p).x, (p).y

struct rect_t { int x, y, w, h; };
struct rect_f_t { float x, y, w, h; };
struct rect_d_t { double x, y, w, h; };

#define RECT_FMT	"(%d, %d)++(%d, %d)"
#define RECTF_FMT	"(%f, %f)++(%f, %f)"
#define RECTD_FMT	"(%lf, %lf)++(%lf, %lf)"
#define RECT_ARG(p)	(p).x, (p).y, (p).w, (p).h


#define bound(x, l, u)		max(min(x, u), l)

#define rects_same(r1, r2) \
	(((r1).x == (r2).x) && \
	 ((r1).y == (r2).y) && \
	 ((r1).w == (r2).w) && \
	 ((r1).h == (r2).h))

#define rects_intersect(r1, r2) ({	\
	typeof(r1)	___r;				\
	(___r).x = bound((r1).x, (r2).x, (r2).x + (r2).w);	\
	(___r).y = bound((r1).y, (r2).y, (r2).y + (r2).h);	\
	(___r).w = bound((r1).x + (r1).w - (___r).x, 0, (r2).x + (r2).w - (___r).x); \
	(___r).h = bound((r1).y + (r1).h - (___r).y, 0, (r2).y + (r2).h - (___r).y); \
	___r;							\
})

#define rect_xbound(r)	((r).x + (r).w)
#define rect_ybound(r)	((r).y + (r).h)

#define rect_in(big, small)	\
	(((small).x >= (big).x) && ((small).x + (small).w <= (big).x + (big).w) \
	 && ((small).y >= (big).y) && ((small).y + (small).h <= (big).y + (big).h))

#endif

// vim:ts=4:sw=4

