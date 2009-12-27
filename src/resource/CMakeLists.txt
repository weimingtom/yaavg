SET(SRC
	resource.h
	resource.c
	)

ADD_LIBRARY(resource STATIC ${SRC})

SET(
	tests
	test_resource
	)
build_tests("${tests}" "resource;yconf;common")

