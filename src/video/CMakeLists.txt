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
		sdl_video.c)
ENDIF (HAVE_SDL)

IF (HAVE_OPENGL)
ADD_SUBDIRECTORY(dynamic_opengl)
IF (USE_OPENGL3)
	SET(OPENGL3_SRC gl3.h opengl3_video.c)
ENDIF (USE_OPENGL3)

SET(OPENGL_SRC
	opengl_video.c
	gl_driver.h
	gl_driver.c
	generic_opengl.c
	generic_opengl.h
	)

IF (HAVE_SDL)
	LIST(APPEND OPENGL_SRC sdl_gl_driver.c)
ENDIF (HAVE_SDL)
IF (HAVE_X11)
	LIST(APPEND OPENGL_SRC glx_gl_driver.c)
ENDIF (HAVE_X11)

ENDIF (HAVE_OPENGL)

ADD_LIBRARY(video STATIC ${SRC} ${OPENGL3_SRC} ${OPENGL_SRC} ${SDL_SRC})
TARGET_LINK_LIBRARIES(video dynamic_opengl common utils events yconf)

IF (HAVE_SDL)
	TARGET_LINK_LIBRARIES(video ${SDL_LIBRARY})
ENDIF(HAVE_SDL)


SET(tests
	test_video_timer
	test_video
	)


build_tests("${tests}" "yconf;utils;video;resource;common")

# vim:ts=4:sw=4
