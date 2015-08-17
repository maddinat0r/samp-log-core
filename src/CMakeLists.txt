include(AMXConfig)

#set(SAMP_SDK_ROOT "${LOGCORE_LIBS_DIR}/sdk")
#find_package(SAMPSDK REQUIRED)

add_subdirectory(amx)

include_directories(
	${SAMPSDK_INCLUDE_DIR}
	${LOGCORE_LIBS_DIR}
	${LOGCORE_LIBS_DIR}/g3log/src
)

add_library(log-core SHARED
	CAmxDebugManager.cpp
	CAmxDebugManager.hpp
	CSampConfigReader.cpp
	CSampConfigReader.hpp
	CSingleton.hpp
	CLogger.cpp
	CLogger.hpp
	CPluginLogger.cpp
	CPluginLogger.hpp
	export.h
	#amxdbg.c
	#amxdbg.h
	#osdefs.h
)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(rt)
endif()

add_dependencies(log-core amx cppformat g3logger)
target_link_libraries(log-core amx cppformat g3logger)


install(TARGETS log-core DESTINATION "lib/")
install(DIRECTORY "samplog/" DESTINATION "include/samplog/")

set(CPACK_PACKAGE_VERSION ${LOGCORE_VERSION})
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};samp-log-core;ALL;src/")
if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)

