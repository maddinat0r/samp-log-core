cmake_minimum_required(VERSION 2.8)
project(samp-log-core)

set(LOGCORE_VERSION "0.4")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

set(CMAKE_CXX_STANDARD 11) #required for fmt library

if(UNIX)
	#force 32bit compilation
	set(CMAKE_CXX_FLAGS "-m32 -fvisibility=hidden")
	set(CMAKE_C_FLAGS "-m32 -fvisibility=hidden")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
endif()

set(LOGCORE_LIBS_DIR "${PROJECT_SOURCE_DIR}/libs")

add_subdirectory(libs/fmt)
mark_as_advanced(FMT_TEST FMT_INSTALL FMT_PEDANTIC FMT_DOC FMT_CPPFORMAT FMT_USE_CPP11)

set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/yaml-cpp)
mark_as_advanced(BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS MSVC_SHARED_RT 
	MSVC_STHREADED_RT APPLE_UNIVERSAL_BIN YAML_CPP_BUILD_CONTRIB YAML_CPP_BUILD_TOOLS
	gmock_build_tests gtest_build_samples gtest_build_tests gtest_disable_pthreads
	gtest_force_shared_crt gtest_hide_internal_symbols) #goddammit

add_subdirectory(src)
