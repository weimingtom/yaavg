/* 
 * opengl_texture.c
 * by WN @ Feb. 27, 2010
 */

#include <common/debug.h>
#include <common/exception.h>
#include <common/cache.h>
#include <yconf/yconf.h>
#include <video/opengl_texture.h>
#include <video/generic_opengl.h>

#include <resource/resource_types.h>
#include <math/matrix.h>

enum drawing_method {
	DM_NULL,
	DM_DRAWARRAYS_OBJECT,
	DM_DRAWARRAYS,
};

static enum drawing_method drawing_method = DM_NULL;

/* texture array cache */
/* the name of a texture array:
 * cache_entry_t represents a group of textures
 * which have been already loaded into video memory.
 * */
struct txarray_cache_entry_t {
	struct cache_entry_t ce;
	const char * bitmap_array_name;
	struct rect_mesh_t * mesh;

	struct vec3 pvecs[4];
	struct vec3 tvecs[4];

	/* GL_TEXTURE_RECTANGLE
	 * GL_TEXTURE_2D */
	GLenum target;
	/* GL_COMPRESSED_RGBA
	 * GL_RGBA
	 * GL_COMPRESSED_RGB
	 * GL_RGB
	 * */
	GLenum internalformat;
	/* 
	 * GL_RGBA
	 * GL_RGB
	 */
	GLenum format;


	GLenum min_filter;
	GLenum mag_filter;
	GLenum wrap_s;
	GLenum wrap_t;
	/* whether to use vertex buffer object? */
#if 0
	bool_t use_vbo;
	union {
		int vbo;
	} vertexs;
#endif
	uint8_t __data[0];
};



static struct cache_t txarray_cache;
static bool_t inited = FALSE;

void
opengl_texture_cache_init(void)
{
	if (!inited) {
		int texture_hwsz = conf_get_int("video.opengl.texture.totalhwsize",
				64 * (1 << 20));
		if (texture_hwsz < 16 * (1 << 20)) {
			WARNING(OPENGL, "video.opengl.texture.totalhwsize is 0x%x, too small, reset to 16MB\n",
					texture_hwsz);
			texture_hwsz = 16 * (1 << 20);
		}
		cache_init(&txarray_cache, "opengl texture array cache", texture_hwsz);
		inited = TRUE;

		/* selete a drawing method */
		drawing_method = DM_DRAWARRAYS;
		if (GL_vertex_array_object)
			drawing_method = DM_DRAWARRAYS_OBJECT;
	}
}

void
opengl_texture_cache_cleanup(void)
{
	if (inited) {
		cache_destroy(&txarray_cache);
		inited = FALSE;
	}
}

static struct vec3 default_pvecs[4] = {
	[0] = {-1.0,  1.0, 0.0},
	[1] = {-1.0, -1.0, 0.0},
	[2] = { 1.0, -1.0, 0.0},
	[3] = { 1.0,  1.0, 0.0},
};

static struct vec3 default_tvecs[4] = {
	[0] = { 0.0, 0.0, 0.0},
	[1] = { 0.0, 1.0, 0.0},
	[2] = { 1.0, 1.0, 0.0},
	[3] = { 1.0, 0.0, 0.0},
};

bool_t
prepare_texture(struct vec3 * pvecs,
		struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t,
		const char * tex_name)
{
	/* first, fetch the big bitmap */
	/* then split it */
	/* build an rect mesh */
	/* for each mesh, create its texture */
	/* build the cache entry */
	/* free big bitmap; free bitmap array */
	/* compute the coord */
	/* insert */
	return TRUE;
}

void
draw_texture(struct vec3 * tvecs,
		const char * tex_name)
{
	
}

// vim:ts=4:sw=4


