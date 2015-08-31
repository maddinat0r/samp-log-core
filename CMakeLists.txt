cmake_minimum_required(VERSION 2.8)
project(samp-log-core)

set(LOGCORE_VERSION "0.1")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

if(UNIX)
	#force C++11 flag and 32bit compilation
	set(CMAKE_CXX_FLAGS "-m32 -std=c++11")
	set(CMAKE_C_FLAGS "-m32 -std=c11")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
endif()

#compiler with C++11 support required
include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
	"
	#include <tuple>
	#include <functional>
	#include <unordered_map>
	#include <forward_list>
	#include <string>

	class CTest
	{
	public:
		CTest() = default;
		~CTest() = default;
		CTest(const CTest &rhs) = delete;
		CTest(CTest &&mv)
		{
			m_Num = mv.m_Num;
			m_NumPtr = mv.m_NumPtr;
		}
		
		int m_Num = 42;
		int *m_NumPtr = nullptr;
		std::unordered_map<std::string, int> m_Map;
		std::forward_list<int> m_ForwardList;
	};

	int main(void)
	{
		int RefVar = 123;
		std::tuple<
			int, 
			std::function<bool(int, int&)>> 
		MyTuple(
			42, [&RefVar](int num1, int &sub_ref)
			{
				RefVar += num1;
				sub_ref += RefVar;
				return true;
			}
		);
		
		enum class StronglyTypedEnum
		{
			ENUM_VAR1,
			ENUM_VAR2
		};
		
		CTest test_class;
		
		return 0;
	}
	" 
	COMPILER_SUPPORTS_CXX11)

if(NOT COMPILER_SUPPORTS_CXX11)
	message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} does not fulfill all required standards. Please use a different C++ compiler.")
endif()

set(LOGCORE_LIBS_DIR "${PROJECT_SOURCE_DIR}/libs")
enable_testing()
option(FMT_INSTALL OFF)
option(FMT_TEST OFF)
option(FMT_DOC OFF)
mark_as_advanced(FMT_TEST FMT_INSTALL FMT_PEDANTIC FMT_DOC WINSDK_SETENV)
add_subdirectory(libs/cppformat)
add_subdirectory(libs/g3log)
add_subdirectory(src)

