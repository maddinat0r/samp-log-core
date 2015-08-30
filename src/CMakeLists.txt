include(AMXConfig)

option(LOGCORE_INSTALL "Generate install target." ON)

add_subdirectory(amx)

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
)

target_include_directories(log-core PUBLIC
	${LOGCORE_LIBS_DIR}
	${LOGCORE_LIBS_DIR}/g3log/src
)

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(rt)
endif()

add_dependencies(log-core amx cppformat g3logger)
target_link_libraries(log-core amx cppformat g3logger)

if(LOGCORE_INSTALL)
	install(TARGETS log-core DESTINATION "lib/")
	install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION "include")
endif()

set(CPACK_PACKAGE_VERSION ${LOGCORE_VERSION})
if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
