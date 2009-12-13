SET(SRC
	resources.h
	resources.c
	file_resources.c
	)

ADD_LIBRARY(resources STATIC ${SRC})

SET(tests
	test_resources
	)
build_tests("${tests}" "resources;common;yconf")


