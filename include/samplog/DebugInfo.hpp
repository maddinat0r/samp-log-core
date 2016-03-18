#pragma once

//NOTE: Passing "-fvisibility=hidden" as a compiler option to GCC is advised!
#if defined _WIN32 || defined __CYGWIN__
# ifdef __GNUC__
#  define DLL_PUBLIC __attribute__ ((dllimport))
# else
#  define DLL_PUBLIC __declspec(dllimport)
# endif
#else
# if __GNUC__ >= 4
#  define DLL_PUBLIC __attribute__ ((visibility ("default")))
# else
#  define DLL_PUBLIC
# endif
#endif

typedef struct tagAMX AMX;

namespace samplog
{
	extern "C" DLL_PUBLIC void RegisterAmx(AMX *amx);
	extern "C" DLL_PUBLIC void EraseAmx(AMX *amx);

	extern "C" DLL_PUBLIC bool GetLastAmxLine(AMX * const amx, long &line);
	extern "C" DLL_PUBLIC bool GetLastAmxFile(AMX * const amx, const char * &file);
	extern "C" DLL_PUBLIC bool GetLastAmxFunction(AMX * const amx, const char * &function);
}

#undef DLL_PUBLIC
