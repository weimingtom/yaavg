#include <utils/rect_mesh.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/init_cleanup_list.h>

init_func_t init_funcs[] = {
	__dbg_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};

#define SCREEN_W	(1024)
#define SCREEN_H	(768)
#define TEXTURE_LIM	(256)
#define NR_W		((SCREEN_W + TEXTURE_LIM - 1) / TEXTURE_LIM)
#define NR_H		((SCREEN_H + TEXTURE_LIM - 1) / TEXTURE_LIM)

static struct rect_mesh_t *
build_mesh(void)
{
	struct rect_mesh_t * mesh = alloc_rect_mesh(NR_W, NR_H);
	assert(mesh != NULL);

	struct rect_t big_irect = {
		.x = 0,
		.y = 0,
		.w = SCREEN_W,
		.h = SCREEN_H,
	};

	struct rect_f_t big_frect = {
		.x = 0.0,
		.y = 0.0,
		.w = 1.0,
		.h = 1.0,
	};

	mesh_irect(mesh) = big_irect;
	mesh_frect(mesh) = big_frect;

	int n = 0;
	for (int j = 0; j < NR_H; j++) {
		for (int i = 0; i < NR_W; i++) {
			struct rect_mesh_tile_t * tile =
				mesh_tile_xy(mesh, i, j);
			tile->number = n;

			struct rect_t irect = {
				.x = 0,
				.y = 0,
				.w = (SCREEN_W - i * TEXTURE_LIM) >= TEXTURE_LIM ?
					TEXTURE_LIM : (SCREEN_W - i * TEXTURE_LIM),
				.h = (SCREEN_H - j * TEXTURE_LIM) >= TEXTURE_LIM ?
					TEXTURE_LIM : (SCREEN_H - j * TEXTURE_LIM),
			};

			struct rect_f_t frect = {
				.x = 0.0,
				.y = 0.0,
				.w = 1.0,
				.h = 1.0,
			};

			tile_irect(tile) = irect;
			tile_frect(tile) = frect;
			n++;
		}
	}

	return mesh;
}

static void
do_test1(void)
{
	struct rect_mesh_t * mesh = build_mesh();
	print_rect_mesh(mesh);

	int i_ox, i_oy;
	struct rect_mesh_tile_t * tile1 =
		map_coord_to_mesh(mesh, 700, 500,
				&i_ox, &i_oy);

	VERBOSE(SYSTEM, "(700, 500) --> %d (%d, %d)\n",
			tile1->number, i_ox, i_oy);

	float f_ox, f_oy;
	struct rect_mesh_tile_t * tile2 =
		map_coord_to_mesh_f(mesh, 0.3, 0.6,
				&f_ox, &f_oy);

	VERBOSE(SYSTEM, "(0.3, 0.6) --> %d (%f, %f)\n",
			tile2->number, f_ox, f_oy);

	struct rect_mesh_tile_t * tile3 =
		map_coord_to_mesh(mesh, TEXTURE_LIM, TEXTURE_LIM,
				&i_ox, &i_oy);

	VERBOSE(SYSTEM, "(%d, %d) --> %d (%d, %d)\n",
			TEXTURE_LIM, TEXTURE_LIM,
			tile3->number, i_ox, i_oy);

	struct rect_mesh_tile_t * tile4 =
		map_coord_to_mesh_f(mesh, 0.5, 0.5,
				&f_ox, &f_oy);
	VERBOSE(SYSTEM, "(0.5, 0.5) --> %d (%f, %f)\n",
			tile4->number, f_ox, f_oy);

	destroy_rect_mesh(mesh);
}

static void
do_test2(void)
{
	struct rect_mesh_t * mesh = build_mesh();
	assert(mesh != NULL);

	struct rect_f_t clip = {
		.x = 0.25,
		.y = 0.25,
		.w = 0.5,
		.h = 0.5,
	};
	struct rect_mesh_t * c_mesh = clip_rect_mesh_f(mesh, clip);
	print_rect_mesh(c_mesh);
	destroy_rect_mesh(c_mesh);
	destroy_rect_mesh(mesh);
	
}

int main()
{
	do_init();

	do_test1();
	do_test2();

	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

