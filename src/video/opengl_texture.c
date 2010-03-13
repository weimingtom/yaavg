/* 
 * opengl_texture.c
 * by WN @ Feb. 27, 2010
 */

#include <common/debug.h>
#include <common/exception.h>
#include <common/cache.h>
#include <common/bithacks.h>
#include <utils/rect_mesh.h>
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
compute_transfer_matrix(mat4x4 * M, struct vec3 * pvecs, struct vec3 * tvecs)
{
	/* T: texture coords */
	/* P: physical coords */
	/* M: result matrix */
	/* M * T = P */
	/* M = P * T^(-1) */
	mat4x4 T, P;
	_matrix_load_identity(&T);
	_matrix_load_identity(&P);
	T.m[0][0] = tvecs[0].x;
	T.m[0][1] = tvecs[0].y;
	T.m[0][2] = 1.0;
	T.m[0][3] = 0.0;

	T.m[1][0] = tvecs[1].x;
	T.m[1][1] = tvecs[1].y;
	T.m[1][2] = 1.0;
	T.m[1][3] = 0.0;

	T.m[2][0] = tvecs[2].x;
	T.m[2][1] = tvecs[2].y;
	T.m[2][2] = 1.0;
	T.m[2][3] = 0.0;

	P.m[0][0] = pvecs[0].x;
	P.m[0][1] = pvecs[0].y;
	P.m[0][2] = pvecs[0].z;
	P.m[0][3] = 0.0;

	P.m[1][0] = pvecs[1].x;
	P.m[1][1] = pvecs[1].y;
	P.m[1][2] = pvecs[1].z;
	P.m[1][3] = 0.0;

	P.m[2][0] = pvecs[2].x;
	P.m[2][1] = pvecs[2].y;
	P.m[2][2] = pvecs[2].z;
	P.m[2][3] = 0.0;

	invert_matrix(&T, &T);
	mulmm(M, &P, &T);
	return;
}

static void
tcoord_to_pcoord(struct vec3 * pcoord, float x, float y, mat4x4 * M)
{
	vec4 tc, pc;
	tc.x = x;
	tc.y = y;
	tc.z = 1.0;
	tc.w = 0.0;
	mulmv(&pc, M, &tc);
	pcoord->x = pc.x;
	pcoord->y = pc.y;
	pcoord->z = pc.z;

	TRACE(OPENGL, "tcoord (%f, %f) to pcoord (%f, %f, %f)\n",
			x, y, pcoord->x, pcoord->y, pcoord->z);
}

static void
compute_tile_pvecs(struct rect_mesh_t * mesh,
		int i, int j,
		struct vec3 * tvecs,
		mat4x4 * t_to_p)
{
	struct rect_mesh_tile_t * mesh_tile = mesh_tile_xy(mesh, i, j);
	if (j == 0) {
		if (i == 0) {
			tcoord_to_pcoord(&mesh_tile->rect.pv[0],
					tvecs[0].x, tvecs[0].y, t_to_p);
			tcoord_to_pcoord(&mesh_tile->rect.pv[1],
					tvecs[1].x, tvecs[1].y, t_to_p);
			tcoord_to_pcoord(&mesh_tile->rect.pv[2],
					tvecs[2].x, tvecs[2].y, t_to_p);
			tcoord_to_pcoord(&mesh_tile->rect.pv[3],
					tvecs[3].x, tvecs[3].y, t_to_p);
		} else {
			struct rect_mesh_tile_t * left_mesh_tile =
				mesh_tile_xy(mesh, i - 1, j);
			mesh_tile->rect.pv[0] = left_mesh_tile->rect.pv[3];
			mesh_tile->rect.pv[1] = left_mesh_tile->rect.pv[2];
			tcoord_to_pcoord(&mesh_tile->rect.pv[2],
					tvecs[2].x, tvecs[2].y, t_to_p);
			tcoord_to_pcoord(&mesh_tile->rect.pv[3],
					tvecs[3].x, tvecs[3].y, t_to_p);
		}
	} else {
		if (i == 0) {
			struct rect_mesh_tile_t * up_mesh_tile =
				mesh_tile_xy(mesh, i, j - 1);
			mesh_tile->rect.pv[0] = up_mesh_tile->rect.pv[1];
			mesh_tile->rect.pv[3] = up_mesh_tile->rect.pv[2];
			tcoord_to_pcoord(&mesh_tile->rect.pv[1],
					tvecs[1].x, tvecs[1].y, t_to_p);
			tcoord_to_pcoord(&mesh_tile->rect.pv[2],
					tvecs[2].x, tvecs[2].y, t_to_p);
		} else {
			struct rect_mesh_tile_t * up_mesh_tile =
				mesh_tile_xy(mesh, i, j - 1);
			struct rect_mesh_tile_t * left_mesh_tile =
				mesh_tile_xy(mesh, i - 1, j);
			mesh_tile->rect.pv[0] = up_mesh_tile->rect.pv[1];
			mesh_tile->rect.pv[3] = up_mesh_tile->rect.pv[2];
			mesh_tile->rect.pv[1] = left_mesh_tile->rect.pv[2];
			tcoord_to_pcoord(&mesh_tile->rect.pv[2],
					tvecs[2].x, tvecs[2].y, t_to_p);
		}
	}
}

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
	struct rect_mesh_t * mesh = tx_entry->mesh;
	if ((pvecs != NULL) && (tvecs != NULL)) {
		int i;
		for (i = 0; i < 4; i++) {
			if (!vec3_equ(tvecs + i, tx_entry->tvecs + i))
				break;
			if (!vec3_equ(pvecs + i, tx_entry->pvecs + i))
				break;
		}
		if (i < 4) {
			/* recompute tvecs and pvecs */
			memcpy(tx_entry->tvecs, tvecs, sizeof(*tvecs) * 4);
			memcpy(tx_entry->pvecs, pvecs, sizeof(*pvecs) * 4);
			mat4x4 t_to_p;
			compute_transfer_matrix(&t_to_p, pvecs, tvecs);
			float curr_x_f;
			float curr_y_f;
			int curr_x = 0;
			int curr_y = 0;
			for (int j = 0; j < mesh->nr_h; j ++) {
				curr_x_f = 0.0;
				curr_x = 0;
				struct rect_mesh_tile_t * tile;
				for (int i = 0; i < mesh->nr_w; i++) {
					tile = mesh_tile_xy(mesh, i, j);
					float tw = (float)(tile->rect.irect.w) / (float)(mesh->big_rect.irect.w);
					float th = (float)(tile->rect.irect.h) / (float)(mesh->big_rect.irect.h);
					struct vec3 tvec[4] = {
						[0] = {curr_x_f, curr_y_f, 0},
						[1] = {curr_x_f, curr_y_f + th, 0},
						[2] = {curr_x_f + tw, curr_y_f + th, 0},
						[3] = {curr_x_f + tw, curr_y_f, 0},
					};
					compute_tile_pvecs(mesh, i, j, tvec, &t_to_p);
					curr_x += tile->rect.irect.w;
					curr_x_f = (float)(curr_x) / (float)(mesh->big_rect.irect.w);
				}
				curr_y += tile->rect.irect.h;
				curr_y_f = (float)(curr_y) / (float)(mesh->big_rect.irect.h);
			}
		}
	}

#define CMP(x)	(tx_entry->x == x)
	if (CMP(min_filter) && CMP(mag_filter)
			&& CMP(wrap_s) && (wrap_t))
		return TRUE;
#undef CMP
	tx_entry->min_filter = min_filter;
	tx_entry->mag_filter = mag_filter;
	tx_entry->wrap_s = wrap_s;
	tx_entry->wrap_t = wrap_t;
	/* reset all textures */
	int nr_texs = mesh->nr_w * mesh->nr_h;
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
		enum texture_method tx_method, GLenum target,
		float * pw, float * ph)
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
		if (pw) *pw = 1.0;
		if (ph)	*ph = 1.0;
	} else {
		int tex_w = pow2roundup(b->w);
		int tex_h = pow2roundup(b->h);
		gl(TexImage2D, target, 0, internal_format, tex_w,
				tex_h, 0, format, GL_UNSIGNED_BYTE, NULL);
		gl(TexSubImage2D, target, 0, 0, 0,
				b->w, b->h, format, GL_UNSIGNED_BYTE, b->pixels);
		if (pw) *pw = (float)(b->w) / (float)(tex_w);
		if (ph)	*ph = (float)(b->h) / (float)(tex_h);
	}
	GL_POP_THROW();
}

static void
__prepare_texture(struct vec3 * pvecs,
		struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t,
		const char * tex_name,
		struct txarray_cache_entry_t ** p_tx_entry)
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
			.revert_y_axis = TRUE,
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
		int nr_tex_objs = bitmap_array->nr_w * bitmap_array->nr_h;
		int rect_mesh_total_sz = get_rect_mesh_total_sz(
				bitmap_array->nr_w,
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
		enum texture_method tx_method = TM_NORMAL;
		if (mesh->nr_w * mesh->nr_h == 1) {
			/* we have only one tile, first choice is NPOT,
			 * then RECT, then NORMAL */
			if (GL_texture_NPOT) {
				tx_method = TM_NPOT;
			} else if (GL_texture_RECT) {
				/* 
				 * Spec 3.1, 3.8.5:
				 *
				 * When target is TEXTURE_RECTANGLE, certain texture parameter values
				 * may not be specified. In this case, the error INVALID_ENUM is generated
				 * if the TEXTURE_WRAP_S, TEXTURE_WRAP T, or TEXTURE_WRAP_R parameter is
				 * set to REPEAT or MIRRORED_REPEAT. The error INVALID_ENUM is generated
				 * if TEXTURE_MIN_FILTER is set to a value other than NEAREST or LINEAR
				 * (no mipmap filtering is permitted). The error INVALID_ENUM is generated if
				 * TEXTURE_BASE_LEVEL is set to any value other than zero.
				 * */

				/* contition too long. we use so much lines only for beautiful */
				if ((wrap_s == GL_REPEAT) || (wrap_s == GL_MIRRORED_REPEAT))
					tx_method = TM_NORMAL;
				else if ((wrap_t == GL_REPEAT) || (wrap_t == GL_MIRRORED_REPEAT))
					tx_method = TM_NORMAL;
				else if ((min_filter != GL_NEAREST) && (min_filter != GL_LINEAR))
					tx_method = TM_NORMAL;
				else
					tx_method = TM_RECT;
			}
		}

		GLenum target = ((tx_method == TM_RECT) ? GL_TEXTURE_RECTANGLE : GL_TEXTURE_2D);

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
		/*  ce->sz is unknown now */
		ce->destroy_arg = tx_entry;
		ce->destroy = (cache_destroy_t)destroy_txarray_cache_entry;
		ce->cache = NULL;
		ce->pprivate = NULL;

		/* compute the coord */
		memcpy(tx_entry->pvecs, pvecs, sizeof(*pvecs) * 4);
		memcpy(tx_entry->tvecs, tvecs, sizeof(*tvecs) * 4);
		mat4x4 t_to_p;
		compute_transfer_matrix(&t_to_p, pvecs, tvecs);

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

		/* load textures */
		int hw_size = 0;
		float curr_x_f = 0;
		float curr_y_f = 0;
		int curr_x = 0;
		int curr_y = 0;
		/* compute each tile's phy vecs */
		for (int j = 0; j <  mesh->nr_h; j++) {
			struct bitmap_t * b = NULL;
			curr_x_f = 0.0;
			curr_x = 0;
			for (int i = 0; i < mesh->nr_w; i++) {
				int nr = j * mesh->nr_w + i;
				GLuint texobj = tx_entry->tex_objs[nr];
				b = &(bitmap_array->tiles[nr]);
				struct rect_mesh_tile_t * mesh_tile = mesh_tile_xy(mesh, i, j);
				mesh_tile->number = texobj;
				load_texture(b, texobj, tx_method, target,
						&(mesh_tile->rect.frect.w),
						&(mesh_tile->rect.frect.h));

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

				/* compute physical coords */
				float tw = (float)(b->w) / (float)(big_bitmap->w);
				float th = (float)(b->h) / (float)(big_bitmap->h);
				struct vec3 tv[4] = {
					[0] = {curr_x_f, curr_y_f, 0},
					[1] = {curr_x_f, curr_y_f + th, 0},
					[2] = {curr_x_f + tw, curr_y_f + th, 0},
					[3] = {curr_x_f + tw, curr_y_f, 0},
				};
				compute_tile_pvecs(mesh, i, j, tv, &t_to_p);
				curr_x += b->w;
				curr_x_f = (float)(curr_x) / (float)(big_bitmap->w);
			}
			curr_y += b->h;
			curr_y_f = (float)(curr_y) / (float)(big_bitmap->h);
		}

		/* finish the ce */
		ce->sz = hw_size;

		/* after all, cancel current texture */
		gl(BindTexture, target, 0);

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
	if (p_tx_entry != NULL)
		*p_tx_entry = tx_entry;
	return;
	
}

void
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
		return;

	if (pvecs == NULL)
		pvecs = default_pvecs;
	if (tvecs == NULL)
		tvecs = default_tvecs;

	return __prepare_texture(pvecs, tvecs,
			min_filter, mag_filter, wrap_s, wrap_t,
			tex_name, NULL);
}

/* texture should be binded, texture params should be set */
/* GL_VERTEX_ARRAY and GL_TEXTURE_COORD_ARRAY should have been enabled */
static void
__draw_texture(float * tv, struct vec3 * pvecs)
{
	gl(TexCoordPointer, 2, GL_FLOAT, 2 * sizeof(float), tv);
	gl(VertexPointer, 3, GL_FLOAT, sizeof(*pvecs), pvecs);
	gl(DrawArrays, GL_POLYGON, 0, 4);
}

static void
__draw_rect_texture(int * tv, struct vec3 * pvecs)
{
	gl(TexCoordPointer, 2, GL_INT, 2 * sizeof(int), tv);
	gl(VertexPointer, 3, GL_FLOAT, sizeof(*pvecs), pvecs);
	gl(DrawArrays, GL_POLYGON, 0, 4);
}

static const struct rect_f_t default_clip_rect = {
	.x = 0.0,
	.y = 0.0,
	.w = 1.0,
	.h = 1.0,
};

static void
__do_draw_txarray(const struct rect_f_t * clip_rect,
		struct txarray_cache_entry_t * tx_entry)
{

	struct rect_mesh_t * mesh = NULL;
	if (clip_rect == NULL)
		clip_rect = &default_clip_rect;
	if (rects_same(clip_rect, &default_clip_rect)) {
		mesh = tx_entry->mesh;
	} else {
		struct rect_mesh_t * big_mesh = tx_entry->mesh;
		mesh = alloca_clip_rect_mesh(big_mesh, clip_rect);
		assert(mesh != NULL);
	}

	/* use tvecs to clip the whole texture */
	gl(EnableClientState, GL_VERTEX_ARRAY);
	gl(EnableClientState, GL_TEXTURE_COORD_ARRAY);

	GLenum target = tx_entry->target;
	GLenum min_filter = tx_entry->min_filter;
	GLenum mag_filter = tx_entry->mag_filter;
	GLenum wrap_s = tx_entry->wrap_s;
	GLenum wrap_t = tx_entry->wrap_t;
	for (int j = 0; j < mesh->nr_h; j++) {
		for (int i = 0; i < mesh->nr_w; i++) {
			struct rect_mesh_tile_t * tile = mesh_tile_xy(mesh, i, j);

			gl(BindTexture, target, tile->number);
			gl(Enable, target);
			gl(TexParameteri, target, GL_TEXTURE_MIN_FILTER, min_filter);
			gl(TexParameteri, target, GL_TEXTURE_MAG_FILTER, mag_filter);
			gl(TexParameteri, target, GL_TEXTURE_WRAP_S, wrap_s);
			gl(TexParameteri, target, GL_TEXTURE_WRAP_T, wrap_t);

			if (target == GL_TEXTURE_RECTANGLE) {
				int tv[8];
				tv[0] = tile->rect.irect.x;
				tv[1] = tile->rect.irect.y;
				tv[2] = tile->rect.irect.x;
				tv[3] = tile->rect.irect.y + tile->rect.irect.h;
				tv[4] = tile->rect.irect.x + tile->rect.irect.w;
				tv[5] = tile->rect.irect.y + tile->rect.irect.h;
				tv[6] = tile->rect.irect.x + tile->rect.irect.w;
				tv[7] = tile->rect.irect.y;
				__draw_rect_texture(tv, tile->rect.pv);
			} else {
				float tv[8];
				memset(tv, '\0', sizeof(tv));
				tv[0] = tile->rect.frect.x;
				tv[1] = tile->rect.frect.y;
				tv[2] = tile->rect.frect.x;
				tv[3] = tile->rect.frect.y + tile->rect.frect.h;
				tv[4] = tile->rect.frect.x + tile->rect.frect.w;
				tv[5] = tile->rect.frect.y + tile->rect.frect.h;
				tv[6] = tile->rect.frect.x + tile->rect.frect.w;
				tv[7] = tile->rect.frect.y;
				__draw_texture(tv, tile->rect.pv);
			}
			GL_POP_ERROR();
			
		}
	}

	gl(DisableClientState, GL_VERTEX_ARRAY);
	gl(DisableClientState, GL_TEXTURE_COORD_ARRAY);
	gl(BindTexture, target, 0);
}

void
do_draw_texture(struct rect_f_t * clip_rect,
		const char * tex_name)
{
	struct cache_entry_t * ce = NULL;
	struct txarray_cache_entry_t * tx_entry = NULL;
	ce = cache_get_entry(&txarray_cache, tex_name);
	assert(ce != NULL);
	tx_entry = ce->data;
	__do_draw_txarray(clip_rect, tx_entry);
}

void
draw_texture(const char * tex_name,
		struct vec3 * pvecs, struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t)
{
	assert(tex_name != NULL);
	if (pvecs == NULL)
		pvecs = default_pvecs;
	if (tvecs == NULL)
		tvecs = default_tvecs;
	prepare_texture(pvecs, tvecs, min_filter,
			mag_filter, wrap_s, wrap_t, tex_name);
	struct rect_f_t clip_rect = {
		.x = tvecs[0].x,
		.y = tvecs[0].y,
		.w = tvecs[3].x - tvecs[0].x,
		.h = tvecs[1].y - tvecs[0].y,
	};
	do_draw_texture(&clip_rect, tex_name);
}

// vim:ts=4:sw=4


