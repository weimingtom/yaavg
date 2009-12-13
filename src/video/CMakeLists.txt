SET(SRC
	video.h
	video.c
	dummy_video.c
	)

ADD_LIBRARY(video STATIC ${SRC})

SET(tests
	test_video_timer
	)

build_tests("${tests}" "yconf;utils;video;common")

