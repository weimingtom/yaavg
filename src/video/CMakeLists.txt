SET(SRC
	video.h
	video.c
	dummy_video.c
	)

ADD_LIBRARY(video STATIC ${SRC})

SET(tests
	test_video
	)

build_tests("${tests}" "yconf;common;utils;video")

