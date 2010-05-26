/* 
 * generic_opengl.h
 * by WN @ Feb. 13, 2010
 */

#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/exception.h>
#include <common/bithacks.h>
#include <common/mm.h>
#include <yconf/yconf.h>
#include <stdarg.h>

/* the opengl functions have to be set nomatter
 * we are using opengl or opengl3 */
#include <video/dynamic_opengl/opengl_funcs.h>
#include <video/generic_opengl.h>

/* setted when gl or gl3 video initing */
const char * GL_vendor = NULL;
const char * GL_renderer = NULL;
const char * GL_version = NULL;
const char * GL_glsl_version = NULL;
struct dict_t * GL_extensions_dict = NULL;

int GL_major_version = 0;
int GL_minor_version = 0;
int GL_full_version = 0;

int GLSL_major_version = 0;
int GLSL_minor_version = 0;
int GLSL_full_version = 0;

int GL_max_texture_size = 0;
bool_t GL_texture_NPOT = FALSE;
bool_t GL_texture_RECT = FALSE;
bool_t GL_texture_COMPRESSION = FALSE;
bool_t GL_vertex_buffer_object = FALSE;
bool_t GL_pixel_buffer_object = FALSE;
bool_t GL_vertex_array_object = FALSE;
int GL_max_vertex_attribs = 0;

const char *
glerrno_to_desc(GLenum errno)
{
	struct kvp {
		GLenum errno;
		const char * desc;
	};

	static struct kvp tb[] = {
		{GL_INVALID_ENUM, "Enum argument out of range"},
		{GL_INVALID_VALUE, "Numeric argument out of range"},
		{GL_INVALID_OPERATION, "Operation illegal in current state"},
		{GL_INVALID_FRAMEBUFFER_OPERATION, "Framebuffer object is not complete"},
		{GL_STACK_OVERFLOW, "Command would cause a stack overflow"},
		{GL_STACK_UNDERFLOW, "Command would cause a stack underflow"},
		{GL_OUT_OF_MEMORY, "Not enough memory left to execute command"},
		{GL_TABLE_TOO_LARGE, "The specified table is too large"},
		/* 0 is resvred for GL_NO_ERROR at least in nvidia opengl */
		{0, "Unknown error"}
	};

	struct kvp * ptr = tb;
	while(ptr->errno != 0) {
		if (ptr->errno == errno)
			break;
		ptr ++;
	}
	return ptr->desc;
}


GLenum
gl_pop_error_debug(const char * file, const char * func, int line)
{
	GLenum err = gl(GetError);
	if (err == GL_NO_ERROR)
		return GL_NO_ERROR;
	WARNING(OPENGL, "OpenGL error: \"%s\" at %s:%s:%d\n",
			glerrno_to_desc(err), file, func, line);
	return err;
}

GLenum
gl_pop_error_nodebug(void)
{
	return gl(GetError);
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
			dict_data_t dt;
			dt = strdict_insert(GL_extensions_dict, pp, data);
			assert(DICT_DATA_NULL(dt));
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
		if (!(DICT_DATA_NULL(d))) {
			DEBUG(OPENGL, "find %s\n", f);
			va_end(args);
			return TRUE;
		}
		f = va_arg(args, const char *);
	}
	va_end(args);
	return FALSE;
}

void
check_opengl_features(void)
{
	GL_vendor = xstrdup((char*)gl(GetString, GL_VENDOR));
	GL_renderer = xstrdup((char*)gl(GetString, GL_RENDERER));
	GL_version = xstrdup((char*)gl(GetString, GL_VERSION));
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
		GL_glsl_version = xstrdup(tmp);

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

	gl(GetIntegerv, GL_MAX_TEXTURE_SIZE, &GL_max_texture_size);
	DEBUG(OPENGL, "system max texture size: %d\n", GL_max_texture_size);
	int conf_mts = conf_get_int("video.opengl.texture.maxsize", 0);
	if (conf_mts != 0) {
		conf_mts = pow2roundup(conf_mts);
		if (conf_mts < GL_max_texture_size)
			GL_max_texture_size = conf_mts;
	}
	DEBUG(OPENGL, "max texture size is set to %d\n", GL_max_texture_size);

	gl(GetIntegerv, GL_MAX_VERTEX_ATTRIBS, &GL_max_vertex_attribs);
	DEBUG(OPENGL, "max vertex attributes is set to %d\n", GL_max_vertex_attribs);

	build_extensions();
	assert(GL_extensions_dict != NULL);
	GL_POP_ERROR();

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

	GL_vertex_buffer_object = check_extension("video.opengl.enableVBO",
			"GL_ARB_vertex_buffer_object",
			NULL);
	GL_pixel_buffer_object = check_extension("video.opengl.enablePBO",
			"GL_ARB_pixel_buffer_object",
			NULL);
	GL_vertex_array_object = check_extension("video.opengl.enableVAO",
			"GL_ARB_vertex_array_object",
			NULL);
#undef verbose_feature
}

void
cleanup_opengl_features(void)
{
	xfree_null(GL_vendor);
	xfree_null(GL_version);
	xfree_null(GL_renderer);
	xfree_null(GL_glsl_version);
	if (GL_extensions_dict != NULL) {
		strdict_destroy(GL_extensions_dict);
		GL_extensions_dict = NULL;
	}
}


// vim:ts=4:sw=4

