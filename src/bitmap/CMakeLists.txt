SET(SRC
	bitmap.h
	bitmap.c
	bitmap_resource.h
	bitmap_resource.c
	tlg_bitmap_resource.c
	dummy_bitmap_resource.c
	bitmap_to_png.c
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
TARGET_LINK_LIBRARIES(bitmap common utils)
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

IF (HAVE_LIBPNG)
	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(bitmap ${PNG_LIBRARIES})
	ADD_DEFINITIONS(${PNG_DEFINITIONS})
ENDIF (HAVE_LIBPNG)

SET(tests
	test_bitmap
	)


build_tests("${tests}" "yconf;utils;bitmap;common")


