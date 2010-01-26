SET(SRC
	bitmap.h
	bitmap.c
	bitmap_resource.h
	bitmap_resource.c
	dummy_bitmap_resource.c
	png_bitmap_resource.c
	)

IF (HAVE_SDLIMAGE)
	SET(SDLSRC sdl_bitmap_resource.c)
ELSE (HAVE_SDLIMAGE)
	SET(SDLSRC "")
ENDIF (HAVE_SDLIMAGE)

ADD_LIBRARY(bitmap STATIC ${SRC} ${SDLSRC})

IF (HAVE_SDL)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(bitmap ${SDL_LIBRARY})
ENDIF (HAVE_SDL)

IF (HAVE_SDLIMAGE)
	INCLUDE_DIRECTORIES(${SDLIMAGE_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(bitmap ${SDLIMAGE_LIBRARY})

	ADD_EXECUTABLE(test_sdl_image test_sdl_image.c)
	TARGET_LINK_LIBRARIES(test_sdl_image ${SDLIMAGE_LIBRARY} common)
ENDIF(HAVE_SDLIMAGE)
SET(tests
	test_bitmap
	)


build_tests("${tests}" "yconf;utils;bitmap;common")


