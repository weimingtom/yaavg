#include <config.h>
#include <common/defs.h>
#include <common/debug.h>
#include <common/functionor.h>
#include <common/mm.h>
#include <bitmap/bitmap.h>
#include <assert.h>

struct bitmap_functionor_t dummy_bitmap_functionor;

static bool_t
dummy_check_usable(const char * param)
{
	TRACE(BITMAP, "%s:%s\n", __func__, param);
	return TRUE;
}

static struct bitmap_t *
dummy_load_bitmap(const char * name)
{
	struct bitmap_t * retval = NULL;

	WARNING(BITMAP, "load bitmap %s using dummy handler\n", name);

	retval = xcalloc(1, sizeof(*retval) + (8 * 8 * 4));
	assert(retval != NULL);

	retval->format = BITMAP_RGBA;
	retval->w = retval->h = 8;
	retval->functionor = &dummy_bitmap_functionor;
	retval->pprivate = NULL;
	retval->data = retval->internal_data;
	return retval;
}

static void
dummy_save_bitmap(struct bitmap_t * name, const char * path)
{
	TRACE(BITMAP, "dummy: save bitmap to %s\n", path);
	return;
}

static void
dummy_destroy_bitmap(struct bitmap_t * bitmap)
{
	assert(bitmap->functionor->destroy_bitmap ==
			dummy_destroy_bitmap);
	TRACE(BITMAP, "dummy: destroy bitmap\n");
	xfree(bitmap);
	return;
}

struct bitmap_functionor_t dummy_bitmap_functionor = {
	.name = "DummyBitmap",
	.fclass = FC_BITMAP_HANDLER,
	.check_usable = dummy_check_usable,
	.destroy_bitmap = dummy_destroy_bitmap,
	.load_bitmap = dummy_load_bitmap,
	.save_bitmap = dummy_save_bitmap,
};

// vim:ts=4:sw=4

