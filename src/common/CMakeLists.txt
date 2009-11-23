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
	cleanup_list.h
	cleanup_list.c
	)

ADD_LIBRARY(common STATIC ${SRC})

SET(tests
	test_debug
	test_exception
	test_dict
	test_mm
	test_strdict
	test_cleanup_list)
build_tests("${tests}" "common")


