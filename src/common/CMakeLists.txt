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
	)

ADD_LIBRARY(common STATIC ${SRC})

SET(tests
	test_debug
	test_exception
	test_dict
	test_mm)
build_tests("${tests}" "common")


