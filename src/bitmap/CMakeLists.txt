SET(SRC
	bitmap.h
	bitmap.c
	bitmap_resource.h
	bitmap_resource.c
	dummy_bitmap_resource.c
	png_bitmap_resource.c
	sdl_bitmap_resource.c
	)

ADD_LIBRARY(bitmap STATIC ${SRC})

IF (HAVE_SDL)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(bitmap ${SDL_LIBRARY})
ENDIF (HAVE_SDL)

IF (HAVE_SDLIMAGE)
	INCLUDE_DIRECTORIES(${SDLIMAGE_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(bitmap ${SDLIMAGE_LIBRARY})
ENDIF(HAVE_SDLIMAGE)
SET(tests
	test_bitmap
	)
build_tests("${tests}" "yconf;utils;bitmap;common")


