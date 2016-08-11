cmake_minimum_required(VERSION 2.8)
project(samp-log-core)

set(LOGCORE_VERSION "0.2.1")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

set(CMAKE_CXX_STANDARD 11)

if(UNIX)
	#force 32bit compilation
	set(CMAKE_CXX_FLAGS "-m32 -fvisibility=hidden")
	set(CMAKE_C_FLAGS "-m32 -fvisibility=hidden")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
endif()

set(LOGCORE_LIBS_DIR "${PROJECT_SOURCE_DIR}/libs")
add_subdirectory(libs/cppformat)
mark_as_advanced(FMT_TEST FMT_INSTALL FMT_PEDANTIC FMT_DOC FMT_CPPFORMAT FMT_USE_CPP11 WINSDK_SETENV)

add_subdirectory(src)
