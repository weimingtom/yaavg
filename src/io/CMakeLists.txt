SET(SRC
	io.h
	io.c
	file_io.c
	xp3_io.c
	)

ADD_LIBRARY(io STATIC ${SRC})

IF (HAVE_ZLIB)
	INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(io ${ZLIB_LIBRARIES})
ENDIF (HAVE_ZLIB)


SET(tests
	test_io
	test_xp3_io
	)
build_tests("${tests}" "io;yconf;common")


