SET(SRC
	phy_events.h
	phy_events.c)

IF (HAVE_SDL)
	SET(SDL_SRC sdl_events.c sdl_events.h)
ENDIF(HAVE_SDL)

ADD_LIBRARY(events STATIC ${SRC} ${SDL_SRC})
TARGET_LINK_LIBRARIES(events common utils)
IF (HAVE_SDL)
	TARGET_LINK_LIBRARIES(events ${SDL_LIBRARY})
ENDIF (HAVE_SDL)

# vim:ts=4:sw=4

