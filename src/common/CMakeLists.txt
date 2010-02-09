SET(SRC
	defs.h
	debug_components.h
	debug.c
	debug.h
	exception.h
	exception.c
	dict.h
	dict.c
	mm.h
	mm.c
	init_cleanup_list.h
	init_cleanup_list.c
	functionor.h
	functionor.c
	bithacks.h
	strhacks.h
	cache.h
	cache.c
	)

ADD_LIBRARY(common STATIC ${SRC})

ADD_EXECUTABLE(gen_static_dict gen_static_dict.c)
TARGET_LINK_LIBRARIES(gen_static_dict common)

SET(tests
	test_debug
	test_exception
	test_exception2
	test_dict
	test_mm
	test_strdict
	test_static_dict
	test_init_cleanup_list
	test_cache)
build_tests("${tests}" "common;yconf")


