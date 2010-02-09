SET(SRC
	timer.h
	timer.c
	base_timer.c
	sdl_timer.c
	rect.h
	)

IF (HAVE_SDL)
	SET(SDL_SRC sdl_timer.c generic_sdl.c generic_sdl.h)
ENDIF(HAVE_SDL)


ADD_LIBRARY(utils STATIC ${SRC} ${SDL_SRC})

IF (HAVE_SDL)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(utils ${SDL_LIBRARY})
ENDIF(HAVE_SDL)

TARGET_LINK_LIBRARIES(utils ${CMAKE_REQUIRED_LIBRARIES})

SET(tests
	test_timer)
build_tests("${tests}" "yconf;utils;common")

