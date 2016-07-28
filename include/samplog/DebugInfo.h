#pragma once
#ifndef INC_SAMPLOG_DEBUGINFO_H
#define INC_SAMPLOG_DEBUGINFO_H

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


extern "C" DLL_PUBLIC void samplog_RegisterAmx(AMX *amx);
extern "C" DLL_PUBLIC void samplog_EraseAmx(AMX *amx);

extern "C" DLL_PUBLIC bool samplog_GetLastAmxLine(AMX * const amx, int *line);
extern "C" DLL_PUBLIC bool samplog_GetLastAmxFile(AMX * const amx, const char **file);
extern "C" DLL_PUBLIC bool samplog_GetLastAmxFunction(AMX * const amx, const char **function);


#ifdef __cplusplus
namespace samplog
{
	inline void RegisterAmx(AMX *amx)
	{
		samplog_RegisterAmx(amx);
	}
	inline void EraseAmx(AMX *amx)
	{
		samplog_EraseAmx(amx);
	}

	inline bool GetLastAmxLine(AMX * const amx, int &line)
	{
		return samplog_GetLastAmxLine(amx, &line);
	}
	inline bool GetLastAmxFile(AMX * const amx, const char * &file)
	{
		return samplog_GetLastAmxFile(amx, &file);
	}
	inline bool GetLastAmxFunction(AMX * const amx, const char * &function)
	{
		return samplog_GetLastAmxFunction(amx, &function);
	}
}
#endif /* __cplusplus */

#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_DEBUGINFO_H */
