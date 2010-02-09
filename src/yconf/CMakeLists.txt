SET(SRC
	yconf.h
	yconf.c
	)

ADD_LIBRARY(yconf STATIC ${SRC})
TARGET_LINK_LIBRARIES(yconf common utils)

SET(tests
	test_yconf
	)
build_tests("${tests}" "yconf;common")


