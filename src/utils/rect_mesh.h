/* 
 * rect_mesh.h
 * by WN @ Feb. 17, 2010
 */

#ifndef __RECT_MESH_H
#define __RECT_MESH_H

#include <common/defs.h>
#include <common/mm.h>
#include <common/cache.h>
#include <utils/rect.h>

/* 
 * rect_mesh is a big rectangle splitted into small tiles.
 * the small tile is numbered like below:
 *
 * (0, 1)          (1, 1)
 * (0, 600)        (800, 600)
 * +----+------+---+
 * | 7  |  8   | 9 |
 * +----+------+---+
 * |    |      |   |
 * | 4  |  5   | 6 |
 * +----+------+---+
 * | 1  |  2   | 3 |
 * +----+------+---+
 * (0, 0)          (1, 0)
 * (0, 0)          (800, 0)
 * the small tiles also has its coordinator in float and integer.
 *
 * this file provides means to clip the mesh using a (float or integer) rectangle:
 *
 * (0, 1)          (1, 1)
 * (0, 600)        (800, 600)
 * +----+------+---+
 * |    |      |   |
 * +----+------+---+
 * |    |   +--+-+ |
 * |    |   |  | | |
 * |    |   |  | | |
 * +----+---+--+-+-+
 * |    |   |  | | |
 * |    |   +--+-+ |
 * |    |      |   |
 * +----+------+---+
 * (0, 0)          (1, 0)
 * (0, 0)          (800, 0)
 *
 * it returns another rect_mesh with integer and float coords set properly. the
 * tile of the returned mesh contail a reference of the corresponding tile in the
 * old mesh.
 */

struct vec3 {
	float x;
	float y;
	float z;
};

#define vec3_equ(a, b)	(equ((a)->x, (b)->x) && equ((a)->y, (b)->y) && equ((a)->z, (b)->z))

/* 
 * NOTICE: irect and frect may not uniform. think:
 * a line of 800 pixels is divided into 4 lines: 256, 256, 256, 32
 * the width in frect are both 1.0, but the real length of the last tile is shorter.
 * therefore, when clipping rect_mesh, internally use irect.
 */
struct mesh_rect {
	struct rect_t irect;
	struct rect_f_t frect;
	/* pv is used for texture rendering, this file doesn't
	 * operate this field */
	struct vec3 pv[4];
};

static inline void
trival_init_mesh_rect(struct mesh_rect * mr, int iw, int ih)
{
	memset(mr, '\0', sizeof(*mr));
	mr->irect.x = mr->irect.y = 0;
	mr->irect.w = iw;
	mr->irect.h = ih;

	mr->frect.x = mr->frect.y = 0.0;
	mr->frect.w = mr->frect.h = 1.0;
}

struct rect_mesh_tile_t {
	/* the number is used to represent the texture number
	 * in opengl. */
	int number;
	struct mesh_rect rect;
};

#define mesh_tile_xy(m, x, y)	(&((m)->tiles[(y) * (m)->nr_w + (x)]))
#define mesh_frect(m)		((m)->big_rect.frect)
#define mesh_irect(m)		((m)->big_rect.irect)
#define tile_irect(t)		((t)->rect.irect)
#define tile_frect(t)		((t)->rect.frect)
#define get_tile_pos(m, t, x, y)	do {	\
	(x) = ((t) - (&((m)->tiles[0]))) % (m->nr_w);	\
	(y) = ((t) - (&((m)->tiles[0]))) / (m->nr_w);	\
} while(0)

/* mr is short for mesh_rect */
/* c is coord, w or h */
#define ilen_to_flen(mr, len, c)	\
	(((float)(len) / (float)(mr)->irect.c) * (mr)->frect.c)
#define flen_to_ilen(mr, len, c)	\
	((len / (mr)->frect.c) * (mr)->irect.c)

#define ix_to_fx(mr, __x)	\
	(ilen_to_flen((mr), (__x) - (mr)->irect.x, w) + (mr)->frect.x)
#define iy_to_fy(mr, __y)	\
	(ilen_to_flen((mr), (__y) - (mr)->irect.y, h) + (mr)->frect.y)

#define fx_to_ix(mr, __x)	\
	(flen_to_ilen(mr, (__x) - (mr)->frect.x, w) + (mr)->irect.x)
#define fy_to_iy(mr, __y)	\
	(flen_to_ilen(mr, (__y) - (mr)->frect.y, h) + (mr)->irect.y)


struct rect_mesh_t {
	/* this is the big rect */
	struct mesh_rect big_rect;
	int nr_w, nr_h;

	void (*destroy)(struct rect_mesh_t *);
	struct rect_mesh_tile_t tiles[0];
};

#define destroy_rect_mesh(m)	do {	\
	if ((m)->destroy)	\
		(m)->destroy((m));	\
} while(0)

/* map a coordinator (x, y) of a big mesh into the 
 * coodinator in a tile (nr) */
extern struct rect_mesh_tile_t *
map_coord_to_mesh(struct rect_mesh_t * mesh,
		int x, int y, int * ox, int * oy);

extern struct rect_mesh_tile_t *
map_coord_to_mesh_f(struct rect_mesh_t * mesh,
		float x, float y, float * ox, float * oy);

#define get_rect_mesh_total_sz(_nr_w, _nr_h) ({\
		sizeof(struct rect_mesh_t) + \
		(_nr_w) * (_nr_h) * sizeof(struct rect_mesh_tile_t) ;})

extern struct rect_mesh_t *
alloc_rect_mesh(int nr_w, int nr_h);

extern void
print_rect_mesh(struct rect_mesh_t * m);

extern void
print_rect_mesh_tile(struct rect_mesh_tile_t * t);

struct clip_rect_mesh_info {
	struct rect_mesh_t * ori;
	struct rect_t clip;
	int nr_w, nr_h;
	int tx1, tx2, ty1, ty3;
	int x1, y1, x2, y3;
};

/* operation of clip_rect_mesh is divided into
 * 2 phases. caller can alloc result mesh using alloca. */
extern void
do_clip_rect_mesh(struct rect_mesh_t * dest, struct clip_rect_mesh_info * info);

extern struct clip_rect_mesh_info
prepare_clip_rect_mesh(struct rect_mesh_t * ori, const struct rect_t * clip);

extern struct clip_rect_mesh_info
prepare_clip_rect_mesh_f(struct rect_mesh_t * ori, const struct rect_f_t * clip);

extern struct rect_mesh_t *
clip_rect_mesh_f(struct rect_mesh_t * ori, const struct rect_f_t * clip);

extern struct rect_mesh_t *
clip_rect_mesh(struct rect_mesh_t * ori, const struct rect_t * clip);

#define alloca_clip_rect_mesh(___ori, ___clip) ({	\
		struct clip_rect_mesh_info ___clip_info = \
			prepare_clip_rect_mesh_f((___ori), (___clip));\
		struct rect_mesh_t * ___ret_mesh = alloca(\
			get_rect_mesh_total_sz(___clip_info.nr_w, \
				___clip_info.nr_h));		\
		assert(___ret_mesh != NULL);		\
		___ret_mesh->nr_w = ___clip_info.nr_w;\
		___ret_mesh->nr_h = ___clip_info.nr_h;\
		___ret_mesh->destroy = NULL;	\
		\
		do_clip_rect_mesh(___ret_mesh, &___clip_info);\
		___ret_mesh;\
		})

#endif

// vim:ts=4:sw=4

