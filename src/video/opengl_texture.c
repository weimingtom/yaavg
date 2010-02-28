/* 
 * opengl_texture.c
 * by WN @ Feb. 27, 2010
 */

#include <common/debug.h>
#include <common/exception.h>
#include <common/cache.h>
#include <common/bithacks.h>
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
	TM_NULL,
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

	GLuint * tex_objs;

	enum texture_method tx_method;
	/* GL_TEXTURE_RECTANGLE
	 * GL_TEXTURE_2D */
	GLenum target;

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
	gl(DeleteTextures, nr_tiles, (GLuint*)(tx->tex_objs));
	TRACE(OPENGL, "delete %d tex objs\n", nr_tiles);
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

#define CMP(x)	(tx_entry->x == x)
	if (CMP(min_filter) && CMP(mag_filter) && CMP(wrap_s) && (wrap_t))
		return TRUE;
#undef CMP
	tx_entry->min_filter = min_filter;
	tx_entry->mag_filter = mag_filter;
	tx_entry->wrap_s = wrap_s;
	tx_entry->wrap_t = wrap_t;
	/* reset all textures */
	int nr_texs = tx_entry->mesh->nr_w * tx_entry->mesh->nr_h;
	GLenum target = tx_entry->target;
	for (int i = 0; i < nr_texs; i++) {
		gl(BindTexture, target, tx_entry->tex_objs[i]);
		gl(Enable, target);
		gl(TexParameteri, target, GL_TEXTURE_MIN_FILTER, min_filter);
		gl(TexParameteri, target, GL_TEXTURE_MAG_FILTER, mag_filter);
		gl(TexParameteri, target, GL_TEXTURE_WRAP_S, wrap_s);
		gl(TexParameteri, target, GL_TEXTURE_WRAP_T, wrap_t);
	}
	gl(BindTexture, target, 0);
	return TRUE;
}

static void
load_texture(struct bitmap_t * b, GLuint tex,
		enum texture_method tx_method, GLenum target)
{
	assert(tex != 0);
	if (tx_method == TM_RECT)
		assert(target == GL_TEXTURE_RECTANGLE);
	else
		assert(target == GL_TEXTURE_2D);

	gl(BindTexture, target, tex);
	gl(Enable, target);

	/* load data */
	/* don't use PBO, we put data into server mem */
	GLint internal_format;
	internal_format = GL_texture_COMPRESSION ?
		GL_COMPRESSED_RGBA : GL_RGBA;
	GLenum format;
	switch (b->format) {
		case BITMAP_RGB:
			format = GL_RGB;
			break;
		case BITMAP_RGBA:
			format = GL_RGBA;
			break;
		case BITMAP_BGR:
			format = GL_BGR;
			break;
		case BITMAP_BGRA:
			format = GL_BGRA;
		default:
			assert(0);
	}

	bool_t need_padding = FALSE;
	if (tx_method == TM_NORMAL)
		if (!((is_power_of_2(b->w)) && (is_power_of_2(b->h))))
			need_padding = TRUE;

	if (!need_padding) {
		gl(TexImage2D, target, 0, internal_format,	b->w,
				b->h, 0, format, GL_UNSIGNED_BYTE, b->pixels);
	} else {
		int tex_w = pow2roundup(b->w);
		int tex_h = pow2roundup(b->h);
		int tex_sz = max(tex_w, tex_h);
		gl(TexImage2D, target, 0, internal_format, tex_sz,
				tex_sz, 0, format, GL_UNSIGNED_BYTE, NULL);
		gl(TexSubImage2D, target, 0, internal_format, b->w,
				b->h, 0, format, GL_UNSIGNED_BYTE, b->pixels);
	}
	GL_POP_THROW();
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
	/* pop previous error */
	GL_POP_ERROR();
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
			sizeof(GLuint) * nr_tex_objs;
		set_catched_var(tx_entry, xcalloc(ce_total_sz, 1));
		assert(tx_entry != NULL);

		tx_entry->bitmap_array_name = (void*)tx_entry->__data;
		tx_entry->mesh =  (void*)tx_entry->bitmap_array_name +
			big_bitmap->id_sz;
		strcpy((char*)tx_entry->bitmap_array_name, big_bitmap->id);
		tx_entry->tex_objs = (void*)(tx_entry->mesh) +
				rect_mesh_total_sz;

		/* build an rect mesh */
		struct rect_mesh_t * mesh = tx_entry->mesh;
		mesh->nr_w = bitmap_array->nr_w;
		mesh->nr_h = bitmap_array->nr_h;
		mesh->destroy = NULL;
		fill_mesh_by_array(bitmap_array, mesh);

		/* for each mesh, create its texture */
		/* choose a tx_method */
		enum texture_method tx_method = TM_NULL;
		if (mesh->nr_w * mesh->nr_h == 1) {
			/* we have only one tile, first choice is RECT,
			 * then NPOT, then NORMAL */
			if (GL_texture_RECT) {
				if ((mag_filter != GL_NEAREST) &&
						(mag_filter != GL_LINEAR) &&
						(min_filter != GL_NEAREST) &&
						(min_filter != GL_LINEAR) &&
						(wrap_s == GL_CLAMP_TO_EDGE) &&
						(wrap_t == GL_CLAMP_TO_EDGE))
				tx_method = TM_RECT;
			}
			if (tx_method == TM_NULL) {
				if (GL_texture_NPOT)
					tx_method = TM_NPOT;
				else
					tx_method = TM_NORMAL;
			}
		} else {
			tx_method = TM_NORMAL;
		}

		GLenum target = GL_TEXTURE_2D;
		if (tx_method == TM_RECT)
			target = GL_TEXTURE_RECTANGLE;

		tx_entry->tx_method = tx_method;
		tx_entry->target = target;
		tx_entry->min_filter = min_filter;
		tx_entry->mag_filter = mag_filter;
		tx_entry->wrap_s = wrap_s;
		tx_entry->wrap_t = wrap_t;

		/* paritally build the cache entry before load bitmaps */
		struct cache_entry_t * ce = &tx_entry->ce;
		ce->id = tx_entry->bitmap_array_name;
		ce->data = tx_entry;
		WARNING(OPENGL, "ce->sz is unknown now\n");
		ce->destroy_arg = tx_entry;
		ce->destroy = (cache_destroy_t)destroy_txarray_cache_entry;
		ce->cache = NULL;
		ce->pprivate = NULL;

		/* begin to load bitmaps */
		/* generate textures */
		gl(GenTextures, nr_tex_objs, tx_entry->tex_objs);
		TRACE(OPENGL, "generate %d tex objs\n", nr_tex_objs);
		GL_POP_THROW();

		/* always use texture 0 */
		if (gl_name(ActiveTexture))
			gl(ActiveTexture, GL_TEXTURE0);
		/* bind pixel unpack buffer */
		/* if not, TexImage2D will use PBO */
		if (gl_name(BindBuffer))
			gl(BindBuffer, GL_PIXEL_UNPACK_BUFFER, 0);

		int hw_size = 0;
		/* load textures */
		for (int j = 0; j <  mesh->nr_h; j++) {
			for (int i = 0; i < mesh->nr_w; i++) {
				int nr = j * mesh->nr_w + i;
				GLuint texobj = tx_entry->tex_objs[nr];
				struct bitmap_t * b = &(bitmap_array->tiles[nr]);
				mesh_tile_xy(mesh, i, j)->number = texobj;
				load_texture(b, texobj, tx_method, target);
				/* texture is still binding, set its params */
				gl(TexParameteri, target, GL_TEXTURE_MIN_FILTER, min_filter);
				gl(TexParameteri, target, GL_TEXTURE_MAG_FILTER, mag_filter);
				gl(TexParameteri, target, GL_TEXTURE_WRAP_S, wrap_s);
				gl(TexParameteri, target, GL_TEXTURE_WRAP_T, wrap_t);

				/* check the size */
				int uncompress_size = b->w * b->h * b->bpp;
				if (GL_texture_COMPRESSION) {
					GLint c, s;
					gl(GetTexLevelParameteriv,
							target, 0, GL_TEXTURE_COMPRESSED, &c);
					if (c) {
						gl(GetTexLevelParameteriv,
								target, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &s);
						TRACE(OPENGL, "compressed texture sz is %d\n", s);
					} else {
						s = uncompress_size;
						TRACE(OPENGL, "uncompressed texture sz is %d\n", s);
					}
					hw_size += s;
				} else {
					hw_size += uncompress_size;
				}
				GL_POP_THROW();
			}
		}

		/* finish the ce */
		ce->sz = hw_size;

		/* after all, cancle current texture */
		gl(BindTexture, target, 0);

		/* compute the coord */
		WARNING(OPENGL, "coord is not computed\n");
		memcpy(tx_entry->pvecs, pvecs, sizeof(*pvecs) * 4);
		memcpy(tx_entry->tvecs, tvecs, sizeof(*tvecs) * 4);

		/* insert */
		cache_insert(&txarray_cache, &tx_entry->ce);

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
		if (tx_entry != NULL)
			destroy_txarray_cache_entry(tx_entry);
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

	struct cache_entry_t * ce = NULL;
	struct txarray_cache_entry_t * tx_entry = NULL;
	ce = cache_get_entry(&txarray_cache, tex_name);
	/* ??? */
	assert(ce != NULL);
	tx_entry = ce->data;

	gl(BindTexture, tx_entry->target, tx_entry->tex_objs[0]);
	/* don't use POLYGON */
	gl(Begin, GL_POLYGON);
	if (tx_entry->target != GL_TEXTURE_RECTANGLE) {
		gl(TexCoord2d, 0, 0);
		gl(Vertex2d, -1, 1);

		gl(TexCoord2d, 0, 1);
		gl(Vertex2d, -1, -1);

		gl(TexCoord2d, 1, 1);
		gl(Vertex2d, 1, -1);

		gl(TexCoord2d, 1, 0);
		gl(Vertex2d, 1, 1);
	} else {

		gl(TexCoord2d, 0, 0);
		gl(Vertex2d, -1, 1);

		gl(TexCoord2d, 0, 800);
		gl(Vertex2d, -1, -1);

		gl(TexCoord2d, 600, 800);
		gl(Vertex2d, 1, -1);

		gl(TexCoord2d, 600, 0);
		gl(Vertex2d, 1, 1);
	}
	gl(End);
}

// vim:ts=4:sw=4


