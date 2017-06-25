#pragma once
#ifndef INC_SAMPLOG_DEBUGINFO_H
#define INC_SAMPLOG_DEBUGINFO_H

#include <stdint.h>

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
typedef int32_t cell;

extern "C" typedef struct
{
	int line;
	const char *file;
	const char *function;
} samplog_AmxFuncCallInfo;


extern "C" DLL_PUBLIC void samplog_RegisterAmx(AMX *amx);
extern "C" DLL_PUBLIC void samplog_EraseAmx(AMX *amx);

extern "C" DLL_PUBLIC bool samplog_GetLastAmxFunctionCall(
	AMX * const amx, samplog_AmxFuncCallInfo *destination);
extern "C" DLL_PUBLIC unsigned int samplog_GetAmxFunctionCallTrace(
		AMX * const amx, samplog_AmxFuncCallInfo *destination, unsigned int max_size);



#ifdef __cplusplus
#include <vector>

namespace samplog
{
	typedef samplog_AmxFuncCallInfo AmxFuncCallInfo;

	inline void RegisterAmx(AMX *amx)
	{
		samplog_RegisterAmx(amx);
	}
	inline void EraseAmx(AMX *amx)
	{
		samplog_EraseAmx(amx);
	}

	inline bool GetLastAmxFunctionCall(AMX * const amx, AmxFuncCallInfo &dest)
	{
		return samplog_GetLastAmxFunctionCall(amx, &dest);
	}
	inline bool GetAmxFunctionCallTrace(AMX * const amx, std::vector<AmxFuncCallInfo> &dest)
	{
		dest.resize(32);
		unsigned int size = samplog_GetAmxFunctionCallTrace(
			amx, dest.data(), dest.size());
		dest.resize(size);
		return size != 0;
	}
}
#endif /* __cplusplus */

#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_DEBUGINFO_H */
