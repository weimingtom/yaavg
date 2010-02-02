SET(SRC
	resource_proc.h
	resource_proc.c
	resource.h
	resource.c
	)

ADD_LIBRARY(resource STATIC ${SRC})
TARGET_LINK_LIBRARIES(resource bitmap io)


SET(
	tests
	test_resource
	test_resource2
	test_load_resource
	)
build_tests("${tests}" "resource;utils;bitmap;io;yconf;common")

