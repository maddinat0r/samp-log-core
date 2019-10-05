include(AMXConfig)

option(LOGCORE_INSTALL_DEV 
	"Generate install target specifically for development." ON)

add_subdirectory(amx)

if(WIN32)
	set(CRASHHANDLER_CPP crashhandler_windows.cpp)
	set(FILECHANGEDETECTOR_CPP FileChangeDetectorWindows.cpp)
else()
	set(CRASHHANDLER_CPP crashhandler_unix.cpp)
	set(FILECHANGEDETECTOR_CPP FileChangeDetectorLinux.cpp)
endif()

set(LOGCORE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include/samplog)

add_library(log-core SHARED
	Api.cpp
	AmxDebugManager.cpp
	AmxDebugManager.hpp
	SampConfigReader.cpp
	SampConfigReader.hpp
	Singleton.hpp
	LogConfig.cpp
	LogConfig.hpp
	Logger.cpp
	Logger.hpp
	LogManager.cpp
	LogManager.hpp
	LogRotationManager.cpp
	LogRotationManager.hpp
	utils.cpp
	utils.hpp
	${CRASHHANDLER_CPP}
	crashhandler.hpp
	${FILECHANGEDETECTOR_CPP}
	FileChangeDetector.hpp
	${LOGCORE_INCLUDE_DIR}/LogLevel.hpp
	${LOGCORE_INCLUDE_DIR}/ILogger.hpp
	${LOGCORE_INCLUDE_DIR}/export.h
)

target_include_directories(log-core PRIVATE "../include")

target_compile_features(log-core PUBLIC 
	cxx_auto_type 
	cxx_decltype 
	cxx_defaulted_functions 
	cxx_default_function_template_args
	cxx_delegating_constructors
	cxx_deleted_functions
	cxx_enum_forward_declarations
	cxx_explicit_conversions
	cxx_extended_friend_declarations
	cxx_inheriting_constructors
	cxx_noexcept
	cxx_nonstatic_member_init
	cxx_nullptr
	cxx_range_for
	cxx_reference_qualified_functions
	cxx_right_angle_brackets
	cxx_rvalue_references
	cxx_strong_enums
	cxx_variadic_templates
)

set_target_properties(log-core PROPERTIES PREFIX "")
set_target_properties(log-core PROPERTIES OUTPUT_NAME "log-core2")

target_include_directories(log-core PRIVATE 
	${LOGCORE_LIBS_DIR}/tinydir
	${YAML_CPP_INCLUDE_DIR}
)

target_compile_definitions(log-core PRIVATE IN_LOGCORE_PLUGIN)
if(MSVC)
	target_compile_definitions(log-core PRIVATE
		_CRT_SECURE_NO_WARNINGS
		NOMINMAX
		WIN32_LEAN_AND_MEAN
		NOGDI # disables ERROR define (conflicts with log level)
	)
endif()

if(UNIX AND NOT APPLE)
	target_link_libraries(log-core PRIVATE rt)
endif()

if (MSVC)
	# increase warning levels
	target_compile_options(log-core PRIVATE
		/W4
		/WX
		/permissive-
	)

	# enable multi-processor compilation
	target_compile_options(log-core PRIVATE
		/MP
	)
elseif(UNIX)
	# increase warning levels
	target_compile_options(log-core PRIVATE
		-Wall
		-Wextra
		-pedantic
	)
endif()

target_link_libraries(log-core PRIVATE
	amx
	fmt
	yaml-cpp
)

if(LOGCORE_INSTALL_DEV)
	set(INCLUDE_INSTALL_DIR include)
	install(TARGETS log-core EXPORT log-core-targets
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib
		RUNTIME DESTINATION bin
		INCLUDES DESTINATION include
	)
	install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION include)
	
	include(CMakePackageConfigHelpers)
	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/log-core/log-coreConfigVersion.cmake"
		VERSION ${LOGCORE_VERSION}
		COMPATIBILITY SameMajorVersion
	)

	export(EXPORT log-core-targets
		FILE "${CMAKE_CURRENT_BINARY_DIR}/log-core/log-coreTargets.cmake"
	)
	
	configure_package_config_file(
		${PROJECT_SOURCE_DIR}/cmake/log-coreConfig.cmake.in
		${CMAKE_CURRENT_BINARY_DIR}/log-coreConfig.cmake
		INSTALL_DESTINATION cmake
		PATH_VARS INCLUDE_INSTALL_DIR
	)

	set(ConfigPackageLocation cmake)
	install(EXPORT log-core-targets
	  FILE
		log-coreTargets.cmake
	  DESTINATION
		${ConfigPackageLocation}
	)
	install(
	  FILES
		${CMAKE_CURRENT_BINARY_DIR}/log-coreConfig.cmake
		"${CMAKE_CURRENT_BINARY_DIR}/log-core/log-coreConfigVersion.cmake"
	  DESTINATION
		${ConfigPackageLocation}
	  COMPONENT
		Devel
	)
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
