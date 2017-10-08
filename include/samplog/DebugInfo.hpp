#pragma once
#ifndef INC_SAMPLOG_DEBUGINFO_HPP
#define INC_SAMPLOG_DEBUGINFO_HPP

#include <stdint.h>
#include "export.h"

#include <vector>

typedef struct tagAMX AMX;
typedef int32_t cell;


namespace samplog
{
	namespace internal
	{
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
	}
	typedef internal::samplog_AmxFuncCallInfo AmxFuncCallInfo;

	inline void RegisterAmx(AMX *amx)
	{
		internal::samplog_RegisterAmx(amx);
	}
	inline void EraseAmx(AMX *amx)
	{
		internal::samplog_EraseAmx(amx);
	}

	inline bool GetLastAmxFunctionCall(AMX * const amx, AmxFuncCallInfo &dest)
	{
		return internal::samplog_GetLastAmxFunctionCall(amx, &dest);
	}
	inline bool GetAmxFunctionCallTrace(AMX * const amx, std::vector<AmxFuncCallInfo> &dest)
	{
		dest.resize(32);
		unsigned int size = internal::samplog_GetAmxFunctionCallTrace(
			amx, dest.data(), dest.size());
		dest.resize(size);
		return size != 0;
	}
}


#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_DEBUGINFO_HPP */
