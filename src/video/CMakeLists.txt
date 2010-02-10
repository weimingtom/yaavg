SET(SRC
	video.h
	video.c
	dummy_video.c
	)

SET(OPENGL3_SRC "")
SET(OPENGL_SRC "")
SET(SDL_SRC "")

IF (HAVE_SDL)
	SET(SDL_SRC
		generic_sdl_video.h
		generic_sdl_video.c
		sdl_video.c
		sdl_driver.c)
ENDIF (HAVE_SDL)

IF (HAVE_OPENGL)
IF (USE_OPENGL3)
	SET(OPENGL3_SRC opengl3_video.c)
ENDIF (USE_OPENGL3)
	SET(OPENGL_SRC opengl_video.c glx_driver.c generic_opengl.c
		generic_opengl.h)
ENDIF (HAVE_OPENGL)

ADD_LIBRARY(video STATIC ${SRC} ${OPENGL3_SRC} ${OPENGL_SRC} ${SDL_SRC})
TARGET_LINK_LIBRARIES(video common utils events yconf)

IF (HAVE_SDL)
	TARGET_LINK_LIBRARIES(video ${SDL_LIBRARY})
ENDIF(HAVE_SDL)


SET(tests
	test_video_timer
	test_video
	)

build_tests("${tests}" "yconf;utils;video;resource;common")

# vim:ts=4:sw=4
