INCLUDE_DIRECTORIES(${YAAVG_BINARY_DIR}/src
	${YAAVG_BINARY_DIR} ${YAAVG_SOURCE_DIR}/src)

MACRO(build_tests testlist libs)
IF (BUILD_TESTS)
	FOREACH(t ${testlist})
		ADD_EXECUTABLE(${t} ${t}.c)
		FOREACH(l ${libs})
			TARGET_LINK_LIBRARIES(${t} ${l})
		ENDFOREACH(l ${libs})
	ENDFOREACH(t ${tests})
ENDIF(BUILD_TESTS)
ENDMACRO(build_tests testlist libs)


ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(yconf)
ADD_SUBDIRECTORY(video)

