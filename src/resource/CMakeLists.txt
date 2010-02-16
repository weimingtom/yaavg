SET(SRC
	resource_proc.h
	resource_proc.c
	resource.h
	resource.c
	resource_types.h
	resource_types.c
	)

ADD_LIBRARY(resource STATIC ${SRC})
TARGET_LINK_LIBRARIES(resource common bitmap io)


SET(
	tests
	test_resource
	test_resource2
	test_resource3
	test_resource4
	test_load_resource
	)
build_tests("${tests}" "resource;utils;bitmap;io;yconf;common")

