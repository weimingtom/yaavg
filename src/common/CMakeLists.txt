SET(SRC
	defs.h
	debug_components.h
	debug.c
	debug.h
	exception.h
	exception.c
	dict.h
	dict.c
	)

ADD_LIBRARY(common STATIC ${SRC})

SET(tests
	test_debug
	test_exception
	test_dict)
build_tests("${tests}" "common")


