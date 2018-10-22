#pragma once

#include "LogLevel.hpp"

#include <vector>
#include <stdint.h>


typedef struct tagAMX AMX;
typedef int32_t cell;

namespace samplog
{
	struct AmxFuncCallInfo
	{
		int line;
		const char *file;
		const char *function;
	};

	class ILogger
	{
	public:
		virtual bool IsLogLevel(LogLevel log_level) const = 0;

		virtual bool LogNativeCall(AMX * const amx, cell * const params, 
			const char *name, const char *params_format) = 0;

		virtual bool Log(LogLevel level, const char *msg,
			std::vector<AmxFuncCallInfo> const &call_info) = 0;

		virtual bool Log(LogLevel level, const char *msg) = 0;

		virtual void Destroy() = 0;
		virtual ~ILogger() = default;
	};
}
