# SET(SRC
# 	vector.h
# 	matrix.h
# 	matrix.c
# 	math.c
# 	math.h
# 	sqrt.h
# 	)

SET(SRC
	cpuid.h
	math.c
	math.h
 	trigon.c
 	trigon.h
	vector.h
	sqrt.h
	matrix.h
	matrix.c
	)


ADD_LIBRARY(math STATIC ${SRC})

# Cannot use SET_PROPERTY because DEBUG FLAGS contain O0 and it has higher
# priority

# SET_PROPERTY(SOURCE matrix.c matrix.c math.c
# 	APPEND
# 	PROPERTY COMPILE_FLAGS "-O3 -Wno-uninitialized"
# 	)

SET(CMAKE_C_FLAGS_DEBUG "-g -rdynamic -O3 -DYAAVG_DEBUG -Wno-uninitialized")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -Wno-uninitialized")

TARGET_LINK_LIBRARIES(math "-lm")
TARGET_LINK_LIBRARIES(math common)


SET(tests
	test_math)
build_tests("${tests}" "common;math")


#build_tests("test_math", "common;math")

