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

enum texture_method {
	TM_RECT,
	TM_NPOT,
	TM_NORMAL,
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

	GLint * tex_objs;

	enum texture_method tx_method;
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

static void
destroy_txarray_cache_entry(struct txarray_cache_entry_t * tx)
{
	assert(tx != NULL);
	int nr_tiles = tx->mesh->nr_w * tx->mesh->nr_h;
	gl(DeleteTextures, nr_tiles, tx->tex_objs);
	xfree(tx);
}

static bool_t
adjust_texture(struct vec3 * pvecs,
		struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t,
		const char * tex_name)
{
	struct cache_entry_t * ce = NULL;
	struct txarray_cache_entry_t * tx_entry = NULL;
	ce = cache_get_entry(&txarray_cache, tex_name);
	if (ce == NULL)
		return FALSE;

	tx_entry = ce->data;
	if ((pvecs != NULL) && (tvecs != NULL)) {
		for (int i = 0; i < 4; i++) {
			if (!vec3_equ(tvecs + i, tx_entry->tvecs + i))
				return FALSE;
			if (!vec3_equ(pvecs + i, tx_entry->pvecs + i))
				return FALSE;
		}
	}

	tx_entry->min_filter = min_filter;
	tx_entry->mag_filter = mag_filter;
	tx_entry->wrap_s = wrap_s;
	tx_entry->wrap_t = wrap_t;
	return TRUE;
}

static bool_t
__prepare_texture(struct vec3 * pvecs,
		struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t,
		const char * tex_name)
{
	define_exp(exp);
	catch_var(struct bitmap_t *, big_bitmap, NULL);
	catch_var(struct bitmap_array_t *, bitmap_array, NULL);
	catch_var(struct txarray_cache_entry_t *, tx_entry, NULL);
	TRY(exp) {
		static struct bitmap_deserlize_param des_parm = {
			.align = UNPACK_ALIGNMENT,
			.fix_revert = TRUE,
		};

		/* first, fetch the big bitmap */
		set_catched_var(big_bitmap,
				load_bitmap(tex_name, &des_parm));
		assert(big_bitmap != NULL);

		/* then split it */
		set_catched_var(bitmap_array,
				split_bitmap(big_bitmap,
					GL_max_texture_size,
					GL_max_texture_size,
					UNPACK_ALIGNMENT));
		assert(bitmap_array != NULL);

		/* create the cache entry */
		WARNING(OPENGL, "destroy tx_entry has not ready\n");
		WARNING(OPENGL, "need to remove textures from hw mem\n");
		int nr_tex_objs = bitmap_array->nr_w * bitmap_array->nr_h;
		int rect_mesh_total_sz = get_rect_mesh_total_sz(bitmap_array->nr_w,
				bitmap_array->nr_h);
		int ce_total_sz = sizeof(*tx_entry) +
			big_bitmap->id_sz +
			rect_mesh_total_sz +
			sizeof(GLint) * nr_tex_objs;
		set_catched_var(tx_entry, xcalloc(ce_total_sz, 1));
		assert(tx_entry != NULL);

		tx_entry->bitmap_array_name = tx_entry->__data;
		tx_entry->mesh =  (void*)tx_entry->bitmap_array_name +
			big_bitmap->id_sz;
		strcpy(tx_entry->bitmap_array_name, big_bitmap->id);
		tx_entry->tex_objs = (void*)((tx_entry->mesh) +
				rect_mesh_total_sz);

		/* build an rect mesh */
		struct rect_mesh_t * mesh = tx_entry->mesh;
		mesh->nr_w = bitmap_array->nr_w;
		mesh->nr_h = bitmap_array->nr_h;
		mesh->destroy = NULL;
		fill_mesh_by_array(bitmap_array, mesh);

		/* for each mesh, create its texture */
		/* choose a target */
		enum texture_method tx_method = TM_NORMAL;
		GLenum target = GL_TEXTURE_2D;
		if (GL_texture_NPOT)
			tx_method = TM_NPOT;
		if (GL_texture_RECT)
			tx_method = TM_RECT;
		target = GL_TEXTURE_2D;
		if (tx_method = TM_RECT)
			target = GL_texture_RECT;
		tx_entry->tx_method = tx_method;
		tx_entry->target = target;

		/* paritally build the cache entry before load bitmaps */
		struct cache_entry_t * ce = &tx_entry->ce;
		ce->id = tx_entry->bitmap_array_name;
		ce->data = tx_entry;
		WARNING(OPENGL, "ce->sz is unknown now\n");
		ce->destroy_arg = tx_entry;
		ce->destroy = destroy_txarray_cache_entry;
		ce->cache = NULL;
		ce->pprivate = NULL;

		/* begin to load bitmaps */
		WARNING(OPENGL, "we are adding video exception\n");

		/* free big bitmap; free bitmap array */
		/* compute the coord */
		/* insert */
		/* free */
	} FINALLY {
		get_catched_var(big_bitmap);
		get_catched_var(bitmap_array);
		if (bitmap_array != NULL) {
			if (bitmap_array->original_bitmap != NULL) {
				assert(bitmap_array->original_bitmap ==
						big_bitmap);
				free_bitmap_array(bitmap_array);
				big_bitmap = NULL;
			} else {
				free_bitmap_array(bitmap_array);
			}

			if (big_bitmap != NULL)
				free_bitmap(big_bitmap);
		}
	} CATCH(exp) {
		get_catched_var(tx_entry);
		if (tx_entry != NULL) {
			destroy_txarray_cache_entry(tx_entry);
		}
		RETHROW(exp);
	}
	return TRUE;
	
}

bool_t
prepare_texture(struct vec3 * pvecs,
		struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t,
		const char * tex_name)
{
	assert(tex_name != NULL);
	if (adjust_texture(pvecs, tvecs, min_filter,
				mag_filter, wrap_s, wrap_t, tex_name))
		return TRUE;

	if (pvecs == NULL)
		pvecs = default_pvecs;
	if (tvecs == NULL)
		tvecs = default_tvecs;

	return __prepare_texture(pvecs, tvecs,
			min_filter, mag_filter, wrap_s, wrap_t,
			tex_name);
}

void
draw_texture(struct vec3 * tvecs,
		const char * tex_name)
{
	
}

// vim:ts=4:sw=4


