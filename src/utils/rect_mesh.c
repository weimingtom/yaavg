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

static struct rect_mesh_tile_t *
__map_coord_to_mesh(struct rect_mesh_t * mesh,
		int x, int y, int * ox, int * oy)
{
	/* check x */
	int i;
	int s_x = 0;
	for (i = 0; i < mesh->nr_w; i++) {
		s_x += mesh_tile_xy(mesh, i, 0).rect.irect.w;
		if (s_x > x)
			break;
	}
	assert(i < mesh->nr_w);
	assert(s_x > x);

	int j;
	int s_y = 0;
	for (j = 0; j < mesh->nr_h; j++) {
		s_y += mesh_tile_xy(mesh, 0, j).rect.irect.h;
		if (s_y > y)
			break;
	}
	assert(j < mesh->nr_h);
	assert(s_y > y);

	struct rect_mesh_tile_t * tile = &(mesh_tile_xy(mesh, i, j));
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

// vim:ts=4:sw=4

