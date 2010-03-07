/* 
 * rect_mesh.c
 * by WN @ Feb. 17, 2010
 */

#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/mm.h>
#include <utils/rect_mesh.h>
#include <math/math.h>

static void
free_rect_mesh(struct rect_mesh_t * m)
{
	xfree(m);
}

struct rect_mesh_t *
alloc_rect_mesh(int nr_w, int nr_h)
{
	int total_sz = get_rect_mesh_total_sz(nr_w, nr_h);
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

static struct clip_rect_mesh_info
__prepare_clip_rect_mesh(struct rect_mesh_t * ori, const struct rect_t * clip)
{
	/* get 3 points: upper-left, upper-right and bottom-right */
	int x1, y1, x2, y2, x3, y3;
	x1 = clip->x;
	y1 = clip->y;
	x2 = clip->x + clip->w - 1;
	y2 = y1;
	x3 = x1;
	y3 = clip->y + clip->h - 1;

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

	struct clip_rect_mesh_info info;
	info.ori = ori;
	info.clip = *clip;
	info.nr_w = nr_w;
	info.nr_h = nr_h;
	info.tx1 = tx1;
	info.tx2 = tx2;
	info.ty1 = ty1;
	info.ty3 = ty3;
	info.x1 = x1;
	info.x2 = x2;
	info.y1 = y1;
	info.y3 = y3;
	return info;
}

struct clip_rect_mesh_info
prepare_clip_rect_mesh(struct rect_mesh_t * ori, const struct rect_t * clip)
{
	assert(ori != NULL);
	assert(clip != NULL);
	assert(rect_in(mesh_irect(ori), (*clip)));
	return __prepare_clip_rect_mesh(ori, clip);
}

struct clip_rect_mesh_info
prepare_clip_rect_mesh_f(struct rect_mesh_t * ori, const struct rect_f_t * clip)
{
	assert(ori != NULL);
	assert(clip != NULL);
	assert(rect_in(mesh_frect(ori), (*clip)));
	/* convert clip to int clip */
	struct rect_t iclip;
	iclip.x = fx_to_ix(&(ori->big_rect), clip->x);
	iclip.y = fy_to_iy(&(ori->big_rect), clip->y);
	iclip.w = flen_to_ilen(&(ori->big_rect), clip->w, w);
	iclip.h = flen_to_ilen(&(ori->big_rect), clip->h, h);
	return __prepare_clip_rect_mesh(ori, &iclip);
}

static void
interp_vec3(struct vec3 * out_vec, struct vec3 * in_vec,
		struct rect_f_t * big_rect,
		struct rect_f_t * clip)
{
	if (rects_same(big_rect, clip)) {
		memcpy(out_vec, in_vec, sizeof(*in_vec) * 4);
		return;
	}

	float x0, y0, x1, y1, x2, y2, x3, y3;
	float cx0, cy0, cx1, cy1, cx2, cy2, cx3, cy3;
	x0 = big_rect->x;
	y0 = big_rect->y;
	x1 = big_rect->x;
	y1 = big_rect->y + big_rect->h;
	x2 = big_rect->x + big_rect->w;
	y2 = big_rect->y + big_rect->h;
	x3 = big_rect->x + big_rect->w;
	y3 = big_rect->y;

	cx0 = clip->x;
	cy0 = clip->y;
	cx1 = clip->x;
	cy1 = clip->y + clip->h;
	cx2 = clip->x + clip->w;
	cy2 = clip->y + clip->h;
	cx3 = clip->x + clip->w;
	cy3 = clip->y;

	for (int i = 0; i < 3; i++) {
#define		valn(v, n)	(((float*)(v + n))[i])
		float v0 = valn(in_vec, 0);
		float v1 = valn(in_vec, 1);
		float v2 = valn(in_vec, 2);
		float v3 = valn(in_vec, 3);

		if ((equ(cx0, x0)) && (equ(cy0, y0))) {
			valn(out_vec, 0) = v0;
		} else {
			float a0 = interp(x0, x3, cx0, v0, v3);
			float b0 = interp(x1, x2, cx0, v1, v2);
			float c0 = interp(y0, y1, cy0, a0, b0);
			valn(out_vec, 0) = c0;
		}

		if ((equ(cx1, x1)) && (equ(cy1, y1))) {
			valn(out_vec, 1) = v1;
		} else {
			float a1 = interp(x0, x3, cx1, v0, v3);
			float b1 = interp(x1, x2, cx1, v1, v2);
			float c1 = interp(y0, y1, cy1, a1, b1);
			valn(out_vec, 1) = c1;
		}

		if ((equ(cx2, x2)) && (equ(cy2, y2))) {
			valn(out_vec, 2) = v2;
		} else {
			float a2 = interp(x0, x3, cx2, v0, v3);
			float b2 = interp(x1, x2, cx2, v1, v2);
			float c2 = interp(y0, y1, cy2, a2, b2);
			valn(out_vec, 2) = c2;
		}

		if ((equ(cx3, x3)) && (equ(cy3, y3))) {
			valn(out_vec, 3) = v3;
		} else {
			float a3 = interp(x0, x3, cx3, v0, v3);
			float b3 = interp(x1, x2, cx3, v1, v2);
			float c3 = interp(y0, y1, cy3, a3, b3);
			valn(out_vec, 3) = c3;
		}
#undef valn
	}
}

static void
__do_clip_rect_mesh(struct rect_mesh_t * dest, struct clip_rect_mesh_info * info)
{
	struct rect_mesh_t * ori = info->ori;
	struct rect_t clip = info->clip;
	int tx1, tx2, ty1, ty3;
	int x1, y1, x2, y3;
	tx1 = info->tx1;
	tx2 = info->tx2;
	ty1 = info->ty1;
	ty3 = info->ty3;
	x1 = info->x1;
	y1 = info->y1;
	x2 = info->x2;
	y3 = info->y3;

	dest->nr_w = info->nr_w;
	dest->nr_h = info->nr_h;
	mesh_irect(dest) = clip;

	struct rect_f_t big_frect;
	big_frect.x = ix_to_fx(&(ori->big_rect), clip.x);
	big_frect.y = iy_to_fy(&(ori->big_rect), clip.y);
	big_frect.w = ilen_to_flen(&(ori->big_rect), clip.w, w);
	big_frect.h = ilen_to_flen(&(ori->big_rect), clip.h, h);
	mesh_frect(dest) = big_frect;

	/* interp pv */
	interp_vec3(dest->big_rect.pv,
			ori->big_rect.pv,
			&ori->big_rect.frect,
			&big_frect);


	for (int j = ty1; j <= ty3; j++) {
		for (int i = tx1; i <= tx2; i++) {
			struct rect_mesh_tile_t * ori_tile =
				mesh_tile_xy(ori, i, j);
			assert(ori_tile != NULL);

			struct rect_mesh_tile_t * new_tile =
				mesh_tile_xy(dest, i - tx1, j - ty1);
			assert(new_tile != NULL);

			new_tile->number = ori_tile->number;

#define oirect tile_irect(ori_tile)
#define nirect tile_irect(new_tile)
			nirect.x = ((i == tx1) ? (x1) : (oirect.x));
			nirect.y = ((j == ty1) ? (y1) : (oirect.y));
			nirect.w = ((i == tx2) ? (x2 - nirect.x + 1) : (rect_xbound(oirect) - nirect.x)); 
			nirect.h = ((j == ty3) ? (y3 - nirect.y + 1) : (rect_ybound(oirect) - nirect.y));
			tile_frect(new_tile).x = ix_to_fx(&(ori_tile->rect), nirect.x);
			tile_frect(new_tile).y = iy_to_fy(&(ori_tile->rect), nirect.y);
			tile_frect(new_tile).w = ilen_to_flen(&(ori_tile->rect), nirect.w, w);
			tile_frect(new_tile).h = ilen_to_flen(&(ori_tile->rect), nirect.h, h);

			interp_vec3(new_tile->rect.pv, ori_tile->rect.pv,
					&ori_tile->rect.frect,
					&new_tile->rect.frect);
#undef oirect
#undef iirect
		}
	}
	return;
}

void
do_clip_rect_mesh(struct rect_mesh_t * dest,
		struct clip_rect_mesh_info * info)
{
	assert(dest != NULL);
	assert(info != NULL);
	assert(info->ori != NULL);
	__do_clip_rect_mesh(dest, info);
}

struct rect_mesh_t *
clip_rect_mesh(struct rect_mesh_t * ori, const struct rect_t * clip)
{
	assert(ori != NULL);
	assert(clip != NULL);
	struct clip_rect_mesh_info info = prepare_clip_rect_mesh(ori, clip);
	struct rect_mesh_t * ret = 
		alloc_rect_mesh(info.nr_w, info.nr_h);
	assert(ret != NULL);
	do_clip_rect_mesh(ret, &info);
	return ret;
}

struct rect_mesh_t *
clip_rect_mesh_f(struct rect_mesh_t * ori, const struct rect_f_t * clip)
{
	assert(ori != NULL);
	assert(clip != NULL);
	struct clip_rect_mesh_info info = prepare_clip_rect_mesh_f(ori, clip);
	struct rect_mesh_t * ret = 
		alloc_rect_mesh(info.nr_w, info.nr_h);
	assert(ret != NULL);
	do_clip_rect_mesh(ret, &info);
	return ret;
}


// vim:ts=4:sw=4

