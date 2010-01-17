SET(SRC
	resource.h
	resource.c
	)

ADD_LIBRARY(resource STATIC ${SRC})

SET(
	tests
	test_resource
	test_resource2
	)
build_tests("${tests}" "resource;bitmap;io;yconf;common")

