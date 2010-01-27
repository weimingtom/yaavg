SET(SRC
	bitmap.h
	bitmap.c
	bitmap_resource.h
	bitmap_resource.c
	dummy_bitmap_resource.c
	)

SET(SDLSRC "")
IF (HAVE_SDLIMAGE)
	SET(SDLSRC sdl_bitmap_resource.c)
ENDIF (HAVE_SDLIMAGE)

SET(PNGSRC "")
IF (HAVE_LIBPNG)
	SET(PNGSRC png_bitmap_resource.c)
ENDIF (HAVE_LIBPNG)

ADD_LIBRARY(bitmap STATIC ${SRC} ${SDLSRC} ${PNGSRC})

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


