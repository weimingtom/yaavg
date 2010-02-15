#include <config.h>
#include <common/init_cleanup_list.h>
#include <math/trigon.h>
#include <math/matrix.h>

init_func_t init_funcs[] = {
	__dbg_init,
	__math_init,
	NULL,
};

cleanup_func_t cleanup_funcs[] = {
	__dbg_cleanup,
	NULL,
};

int main()
{
	do_init();

	VERBOSE(SYSTEM, "sin_d(45.0f) ^ 2 = %f\n",
			sin_d(45.0f) * sin_d(45.0f));

	mat4x4 d, x;
	mat4x4 s = {
		.f = {
			0,0,1,0,
			1,0,0,0,
			1,1,1,0,
			0,0,0,1,
		}
	};

	invert_matrix(&d, &s);
	print_matrix(&s);
	print_matrix(&d);

	mulmm(&x, &d, &s);
	print_matrix(&x);


	do_cleanup();
	return 0;
}

// vim:ts=4:sw=4

