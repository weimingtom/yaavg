/* 
 * opengl3_video.c
 * by WN @ Feb. 08, 2010
 */

#include <config.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/dict.h>
#include <common/bithacks.h>
#include <yconf/yconf.h>
#include <video/video.h>
#include <video/gl_driver.h>
#include <video/generic_opengl.h>
#include <video/dynamic_opengl/dynamic_opengl.h>
#include <video/dynamic_opengl/opengl_funcs.h>

#include <stdarg.h>
#include <stdio.h>

#if defined HAVE_OPENGL

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(OPENGL, "%s:%s\n", __func__, param);
	if (strcasecmp(param, "opengl") == 0)
		return TRUE;
	return FALSE;
}

static void
build_extensions(void)
{
	assert(GL_extensions_dict == NULL);
	/* create the dict */
	GL_extensions_dict =
		strdict_create(128, STRDICT_FL_DUPKEY);
	assert(GL_extensions_dict != NULL);

	const char * __extensions = (const char *)gl(GetString, GL_EXTENSIONS);
	/* scan the string, identify each ' ' and replace it with '\0', then insert it
	 * into GL_extensions_dict */
	assert(__extensions != NULL);
	char * extensions = strdupa(__extensions);
	char * p = extensions;
	char * pp = p;
	dict_data_t data;
	data.bol = TRUE;

	while (*p != '\0') {
		if (*p == ' ') {
			*p = '\0';
			strdict_insert(GL_extensions_dict, pp, data);
			TRACE(OPENGL, "extension %s\n", pp);
			pp = p + 1;
		}
		p++;
	}
}

static bool_t
check_extension(const char * conf_key, ...)
{
	va_list args;
	bool_t retval = FALSE;

	if (conf_key != NULL) {
		retval = conf_get_bool(conf_key, TRUE);
		/* conf set this feature to FALSE */
		if (!retval)
			return FALSE;
	}

	va_start(args, conf_key);
	const char * f = va_arg(args, const char *);
	while(f != NULL) {
		assert(strlen(f) < 64);
		TRACE(OPENGL, "check for feature %s\n", f);
		dict_data_t d = strdict_get(GL_extensions_dict, f);
		if (!(GET_DICT_DATA_FLAGS(d) & DICT_DATA_FL_VANISHED)) {
			DEBUG(OPENGL, "find %s\n", f);
			va_end(args);
			return TRUE;
		}
		f = va_arg(args, const char *);
	}
	va_end(args);
	return FALSE;
}

static void
check_features(void)
{
	gl(GetIntegerv, GL_MAX_TEXTURE_SIZE, &GL_max_texture_size);
	DEBUG(OPENGL, "system max texture size: %d\n", GL_max_texture_size);
	int conf_mts = conf_get_int("video.opengl.texture.maxsize", 0);
	if (conf_mts != 0) {
		conf_mts = pow2roundup(conf_mts);
		if (conf_mts < GL_max_texture_size)
			GL_max_texture_size = conf_mts;
	}
	DEBUG(OPENGL, "max texture size is set to %d\n", GL_max_texture_size);

#define verbose_feature(name, exp) do {\
	if (exp)	\
		DEBUG(OPENGL, name " is enabled\n");	\
	else		\
		DEBUG(OPENGL, name " is disabled\n");	\
	} while(0)

	GL_texture_NPOT = check_extension("video.opengl.texture.enableNPOT",
			"GL_ARB_texture_non_power_of_two",
			NULL);
	verbose_feature("NPOT texture", GL_texture_NPOT);

	GL_texture_RECT = check_extension("video.opengl.texture.enableRECT",
			"GL_ARB_texture_rectangle",
			"GL_EXT_texture_rectangle",
			"GL_NV_texture_rectangle",
			NULL);
	verbose_feature("RECT texture", GL_texture_RECT);

	GL_texture_COMPRESSION = check_extension("video.opengl.texture.enableCOMPRESSION",
			"GL_ARB_texture_compression",
			NULL);
	verbose_feature("texture compression", GL_texture_COMPRESSION);
#undef verbose_feature
}

static void
gl_init(void)
{
	DEBUG(OPENGL, "init opengl video\n");

	/* find the opengl driver */
	const char * drv_str = conf_get_string("video.opengl.driver", "sdl");
	find_functionor(&gl_driver_function_class, drv_str);
	assert(CUR_DRV != NULL);

	/* set to 0 to indicate we are using lower version of opengl */
	CUR_DRV->major_version = 0;
	drv_init();

#define set_func(x)	CUR_VID->x = CUR_DRV->x
	set_func(set_caption);
	set_func(set_icon);
	set_func(set_mouse_pos);
	set_func(set_mouse_pos_int);
	set_func(reinit);
	set_func(reshape);
	set_func(render_frame);
	set_func(swapbuffer);
	set_func(poll_events);
	set_func(toggle_fullscreen);
#undef set_func

	/* init opengl information */
	assert(CUR_DRV->get_proc_address != NULL);
	init_func_list(CUR_DRV->get_proc_address, __gl_func_map);

	GL_vendor = strdup((char*)gl(GetString, GL_VENDOR));
	GL_renderer = strdup((char*)gl(GetString, GL_RENDERER));
	GL_version = strdup((char*)gl(GetString, GL_VERSION));
	/* according to opengl spec, the version string of opengl and
	 * glsl is 
	 *
	 * <version number> <space> <vendor spec information>
	 *
	 * and <version number> is
	 *
	 * major.minor
	 *
	 * or
	 *
	 * major.minor.release
	 *
	 * */

	/* build gl version */
	int err;
	err = sscanf(GL_version, "%d.%d", &GL_major_version, &GL_minor_version);
	assert(err == 2);
	assert((GL_major_version > 0) && (GL_major_version <= 3));
	assert(GL_minor_version > 0);
	GL_full_version = MKVER(GL_major_version, GL_minor_version);

	const char * tmp = (const char *)gl(GetString, GL_SHADING_LANGUAGE_VERSION);
	if (GL_POP_ERROR() != GL_NO_ERROR) {
		WARNING(OPENGL, "Doesn't support glsl\n");
		GL_glsl_version = NULL;
	} else {
		GL_glsl_version = strdup(tmp);

		err = sscanf(GL_glsl_version, "%d.%d", &GLSL_major_version, &GLSL_minor_version);
		assert(err == 2);
		assert(GLSL_major_version > 0);
		assert(GLSL_minor_version > 0);
		GLSL_full_version = MKVER(GLSL_major_version, GLSL_minor_version);
	}

	VERBOSE(OPENGL, "OpenGL engine information:\n");
	VERBOSE(OPENGL, "\tvendor: %s\n", GL_vendor);
	VERBOSE(OPENGL, "\trenderer: %s\n", GL_renderer);
	VERBOSE(OPENGL, "\tversion: %s\n", GL_version);
	VERBOSE(OPENGL, "\tglsl version: %s\n", GL_glsl_version);

	int x;
	gl(GetIntegerv, GL_SAMPLES, &x);
	VERBOSE(OPENGL, "\tSamples : %d\n", x);
	gl(GetIntegerv, GL_SAMPLE_BUFFERS, &x);
	VERBOSE(OPENGL, "\tSample buffers : %d\n", x);
	if (x > 0)
		gl(Enable, GL_MULTISAMPLE);
	if (GL_POP_ERROR())
		WARNING(OPENGL, "platform does not support multisample\n");

	build_extensions();

	assert(GL_extensions_dict != NULL);
	GL_POP_ERROR();
	check_features();
	GL_POP_ERROR();

	return;
}

static void
gl_cleanup(void)
{
	DEBUG(OPENGL, "closing opengl video\n");
	xfree_null(GL_vendor);
	xfree_null(GL_version);
	xfree_null(GL_renderer);
	xfree_null(GL_glsl_version);
	if (GL_extensions_dict != NULL) {
		strdict_destroy(GL_extensions_dict);
		GL_extensions_dict = NULL;
	}
	drv_cleanup();
	return;
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
	.init = gl_init,
	/* redirect to driver's poll_events */
	.poll_events = NULL,
	.toggle_fullscreen = NULL,
	.cleanup = gl_cleanup,
};

#else

static bool_t
gl_check_usable(const char * param)
{
	VERBOSE(OPENGL, "%s:%s\n", __func__, param);
	return FALSE;
}

struct video_functionor_t opengl_video_functionor = {
	.name = "OpenGLVideo",
	.fclass = FC_VIDEO,
	.check_usable = gl_check_usable,
};
#endif

// vim:ts=4:sw=4

