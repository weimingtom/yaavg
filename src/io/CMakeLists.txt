SET(SRC
	io.h
	io.c
	file_io.c
	)

ADD_LIBRARY(io STATIC ${SRC})

SET(tests
	test_io
	)
build_tests("${tests}" "io;yconf;common")


