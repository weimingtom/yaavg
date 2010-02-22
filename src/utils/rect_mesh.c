/* 
 * rect_mesh.c
 * by WN @ Feb. 17, 2010
 */

#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/mm.h>
#include <utils/rect_mesh.h>

static void
free_rect_mesh(struct rect_mesh_t * m)
{
	xfree(m);
}

struct rect_mesh_t *
alloc_rect_mesh(int nr_w, int nr_h)
{
	int total_sz = sizeof(struct rect_mesh_t) +
		nr_w * nr_h * sizeof(struct rect_mesh_tile_t);
	struct rect_mesh_t * ret = xmalloc(total_sz);
	assert(ret != NULL);
	ret->nr_w = nr_w;
	ret->nr_h = nr_h;
	ret->destroy = free_rect_mesh;
	return ret;
}

/* we choose integer version as main version because
 * floating version may accumulate errors */
static struct rect_mesh_tile_t *
__map_coord_to_mesh(struct rect_mesh_t * mesh,
		int x, int y, int * ox, int * oy)
{
	/* check x */
	int i;
	int s_x = 0;
	for (i = 0; i < mesh->nr_w; i++) {
		s_x += mesh_tile_xy(mesh, i, 0)->rect.irect.w;
		if (s_x > x)
			break;
	}
	assert(i < mesh->nr_w);
	assert(s_x > x);

	int j;
	int s_y = 0;
	for (j = 0; j < mesh->nr_h; j++) {
		s_y += mesh_tile_xy(mesh, 0, j)->rect.irect.h;
		if (s_y > y)
			break;
	}
	assert(j < mesh->nr_h);
	assert(s_y > y);

	struct rect_mesh_tile_t * tile = mesh_tile_xy(mesh, i, j);
	if (ox)
		*ox = rect_xbound(tile_irect(tile)) - (s_x - x);
	if (oy)
		*oy = rect_ybound(tile_irect(tile)) - (s_y - y);
	return tile;

}

struct rect_mesh_tile_t *
map_coord_to_mesh(struct rect_mesh_t * mesh,
		int x, int y, int * ox, int * oy)
{
	assert(mesh != NULL);
	assert(x >= mesh_irect(mesh).x);
	assert(x < rect_xbound(mesh_irect(mesh)));
	assert(y >= mesh_irect(mesh).y);
	assert(y < rect_ybound(mesh_irect(mesh)));

	return __map_coord_to_mesh(mesh, x, y, ox, oy);
}

struct rect_mesh_tile_t *
map_coord_to_mesh_f(struct rect_mesh_t * mesh,
		float x, float y, float * ox, float * oy)
{
	assert(mesh != NULL);
	assert(x >= mesh_frect(mesh).x);
	assert(x < rect_xbound(mesh_frect(mesh)));
	assert(y >= mesh_frect(mesh).y);
	assert(y < rect_ybound(mesh_frect(mesh)));

	/* convert to int coord */
	int ix, iy, iox, ioy;

	ix = fx_to_ix(&(mesh->big_rect), x);
	iy = fy_to_iy(&(mesh->big_rect), y);

	assert(ix >= mesh_irect(mesh).x);
	assert(ix < rect_xbound(mesh_irect(mesh)));
	assert(iy >= mesh_irect(mesh).y);
	assert(iy < rect_ybound(mesh_irect(mesh)));

	struct rect_mesh_tile_t * ret =
		__map_coord_to_mesh(mesh, ix, iy, &iox, &ioy);
	assert(ret != NULL);

	if (ox)
		*ox = ix_to_fx(&(ret->rect), iox);
	if (oy)
		*oy = iy_to_fy(&(ret->rect), ioy);
	return ret;
}

static void
print_mesh_rect(struct mesh_rect * r)
{
	VERBOSE(OTHER, "irect: " RECT_FMT "\n",
			RECT_ARG(r->irect));
	VERBOSE(OTHER, "frect: " RECTF_FMT "\n",
			RECT_ARG(r->frect));
}

void
print_rect_mesh_tile(struct rect_mesh_tile_t * t)
{
	VERBOSE(OTHER, "tile %d: \n", t->number);
	print_mesh_rect(&t->rect);
}

void
print_rect_mesh(struct rect_mesh_t * m)
{
	VERBOSE(OTHER, "mesh: \n");
	print_mesh_rect(&m->big_rect);
	VERBOSE(OTHER, "%d x %d tiles\n",
			m->nr_w, m->nr_h);
	for (int i = 0; i < m->nr_w * m->nr_h; i++) {
		print_rect_mesh_tile(&m->tiles[i]);
	}
}

static struct rect_mesh_t *
__clip_rect_mesh(struct rect_mesh_t * ori, struct rect_t clip)
{
	assert(ori != NULL);
	assert(rect_in(mesh_irect(ori), clip));

	/* get 3 points: upper-left, upper-right and bottom-right */
	int x1, y1, x2, y2, x3, y3;
	x1 = clip.x;
	y1 = clip.y;
	x2 = clip.x + clip.w - 1;
	y2 = y1;
	x3 = x1;
	y3 = clip.y + clip.h - 1;

	struct rect_mesh_tile_t * tile1;
	struct rect_mesh_tile_t * tile2;
	struct rect_mesh_tile_t * tile3;

	/* x and y 123's meanings are changed */
	tile1 = map_coord_to_mesh(ori, x1, y1, &x1, &y1);
	assert(tile1 != NULL);
	tile2 = map_coord_to_mesh(ori, x2, y2, &x2, &y2);
	assert(tile2 != NULL);
	tile3 = map_coord_to_mesh(ori, x3, y3, &x3, &y3);
	assert(tile3 != NULL);

	int tx1, ty1, tx2, ty2, tx3, ty3;

	get_tile_pos(ori, tile1, tx1, ty1);
	get_tile_pos(ori, tile2, tx2, ty2);
	get_tile_pos(ori, tile3, tx3, ty3);

	int nr_w = tx2 - tx1 + 1;
	int nr_h = ty3 - ty1 + 1;
	assert(nr_w > 0);
	assert(nr_h > 0);

	struct rect_mesh_t * ret = 
		alloc_rect_mesh(nr_w, nr_h);
	assert(ret != NULL);
	mesh_irect(ret) = clip;

	struct rect_f_t big_frect;
	big_frect.x = ix_to_fx(&(ori->big_rect), clip.x);
	big_frect.y = iy_to_fy(&(ori->big_rect), clip.y);
	big_frect.w = ilen_to_flen(&(ori->big_rect), clip.w, w);
	big_frect.h = ilen_to_flen(&(ori->big_rect), clip.h, h);
	mesh_frect(ret) = big_frect;

	for (int j = ty1; j <= ty3; j++) {
		for (int i = tx1; i <= tx2; i++) {
			struct rect_mesh_tile_t * ori_tile =
				mesh_tile_xy(ori, i, j);
			assert(ori_tile != NULL);

			struct rect_mesh_tile_t * new_tile =
				mesh_tile_xy(ret, i - tx1, j - ty1);
			assert(new_tile != NULL);

			new_tile->number = ori_tile->number;

#define oirect tile_irect(ori_tile)
#define nirect tile_irect(new_tile)
			nirect.x = (i == tx1) ? (x1) : (oirect.x);
			nirect.y = (j == ty1) ? (y1) : (oirect.y);
			nirect.w = (i == tx2) ? (x2 - nirect.x + 1) : (rect_xbound(oirect) - nirect.x); 
			nirect.h = (j == ty3) ? (y3 - nirect.y + 1) : (rect_ybound(oirect) - nirect.y);
			tile_frect(new_tile).x = ix_to_fx(&(ori_tile->rect), nirect.x);
			tile_frect(new_tile).y = iy_to_fy(&(ori_tile->rect), nirect.y);
			tile_frect(new_tile).w = ilen_to_flen(&(ori_tile->rect), nirect.w, w);
			tile_frect(new_tile).h = ilen_to_flen(&(ori_tile->rect), nirect.h, h);
#undef oirect
#undef iirect
		}
	}
	return ret;
}

struct rect_mesh_t *
clip_rect_mesh_f(struct rect_mesh_t * ori, struct rect_f_t clip)
{
	assert(ori != NULL);
	assert(rect_in(mesh_frect(ori), clip));
	/* convert clip to int clip */
	struct rect_t iclip;
	iclip.x = fx_to_ix(&(ori->big_rect), clip.x);
	iclip.y = fy_to_iy(&(ori->big_rect), clip.y);
	iclip.w = flen_to_ilen(&(ori->big_rect), clip.w, w);
	iclip.h = flen_to_ilen(&(ori->big_rect), clip.h, h);
	return __clip_rect_mesh(ori, iclip);
}

struct rect_mesh_t *
clip_rect_mesh(struct rect_mesh_t * ori, struct rect_t clip)
{
	assert(ori != NULL);
	assert(rect_in(mesh_irect(ori), clip));
	return __clip_rect_mesh(ori, clip);
}

// vim:ts=4:sw=4

