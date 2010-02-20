/* 
 * rect_mesh.h
 * by WN @ Feb. 17, 2010
 */

#ifndef __RECT_MESH_H
#define __RECT_MESH_H

#include <common/defs.h>
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
	float x, y, z;
};

struct mesh_rect {
	struct rect_t irect;
	struct rect_f_t frect;
	/* pv is used for texture rendering, this file doesn't
	 * operate this field */
	struct vec3 pv;
};

struct rect_mesh_t;
struct rect_mesh_tile_t {
	int number;
	struct rect_mesh_t * mesh;
	struct rect_mesh_tile_t * ref;
	struct mesh_rect rect;
};

struct rect_mesh_t {
	struct mesh_rect rect;
	int nr_w, nr_h;
	struct rect_mesh_tile_t tiles[0];
};

/* map a coordinator (x, y) of a big mesh into the 
 * coodinator in a tile (nr) */
void
map_coord_to_mesh(struct rect_mesh_t * mesh,
		int x, int y, int * ox, int * oy, int * nr);

void
map_coord_to_mesh_f(struct rect_mesh_t * mesh,
		float x, float y, float * ox, float * oy, int * nr);


#endif

// vim:ts=4:sw=4

