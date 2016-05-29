include(AMXConfig)

option(LOGCORE_INSTALL_DEV 
	"Generate install target specifically for development." ON)

add_subdirectory(amx)

if(WIN32)
	set(CRASHHANDLER_CPP crashhandler_windows.cpp)
else()
	set(CRASHHANDLER_CPP crashhandler_unix.cpp)
endif()

add_library(log-core SHARED
	CAmxDebugManager.cpp
	CAmxDebugManager.hpp
	CSampConfigReader.cpp
	CSampConfigReader.hpp
	CMessage.hpp
	CSingleton.hpp
	CLogger.cpp
	CLogger.hpp
	CPluginLogger.cpp
	CPluginLogger.hpp
	export.h
	crashhandler.hpp
	${CRASHHANDLER_CPP}
	loglevel.hpp
)

set_target_properties(log-core PROPERTIES PREFIX "")

target_include_directories(log-core PUBLIC ${LOGCORE_LIBS_DIR})

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNOMINMAX)
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(log-core rt)
endif()

add_dependencies(log-core amx fmt)
target_link_libraries(log-core amx fmt)

if(LOGCORE_INSTALL_DEV)
	install(TARGETS log-core DESTINATION "lib/")
	install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION "include/")
else()
	install(TARGETS log-core 
		RUNTIME DESTINATION "./"
		LIBRARY DESTINATION "./")
endif()

if(LOGCORE_INSTALL_DEV)
	set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
else()
	set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}-runtime")
endif()
set(CPACK_PACKAGE_VERSION ${LOGCORE_VERSION})
if(WIN32)
	set(CPACK_GENERATOR ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
