/* 
 * opengl_texture.h
 * by WN @ Feb. 27, 2010
 */

#ifndef __OPENGL_TEXTURE_H
#define __OPENGL_TEXTURE_H

#include <video/dynamic_opengl/opengl_funcs.h>
#include <bitmap/bitmap.h>
#include <common/cache.h>

void
opengl_texture_cache_init(void);

void
opengl_texture_cache_cleanup(void);

/* generate a texture into cache. if tex_name is already in the cache:
 * if pvecs and tvecs are NULL or not change, only adjust the 4 params.
 * if not, the cached textures will be freed and regenerated.
 *
 * if tex_name is not in the cache:
 * if pvecs and tvecs are both not null, the transformation matrix is computed.
 * if one of them is NULL, default vecs is used.
 *
 * */
extern void
prepare_texture(struct vec3 * pvecs, struct vec3 * tvecs,
		GLenum min_filter,
		GLenum mag_filter,
		GLenum wrap_s,
		GLenum wrap_t,
		const char * tex_name);

/* draw_texture clip a rect from the full texture by providing
 * 4 texture coords. the physical coords is computed by a matrix
 * built in prepare_texture.
 *
 * if tvecs is NULL, default vecs is used.
 * */
extern void
draw_texture(struct rect_f_t * clip_rect,
		const char * tex_name);

#endif

// vim:ts=4:sw=4

