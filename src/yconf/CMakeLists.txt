SET(SRC
	yconf.h
	yconf.c
	)

ADD_LIBRARY(yconf STATIC ${SRC})

SET(tests
	test_yconf
	)
build_tests("${tests}" "yconf;common")


