SET(SRC
	defs.h
	debug_components.h
	debug.c
	debug.h
	)

ADD_LIBRARY(common STATIC ${SRC})

SET(tests
	test_debug)
build_tests("${tests}" "common")


