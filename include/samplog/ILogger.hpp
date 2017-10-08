#pragma once
#ifndef INC_SAMPLOG_ILOGGER_H
#define INC_SAMPLOG_ILOGGER_H

#include "LogLevel.hpp"
#include "DebugInfo.h"
#include "export.h"

#include <vector>

namespace samplog
{
	class ILogger
	{
	public:
		virtual bool IsLogLevel(LogLevel log_level) = 0;

		virtual bool LogNativeCall(AMX * const amx, cell * const params, 
			const char *name, const char *params_format) = 0;

		virtual bool Log(LogLevel level, const char *msg,
			std::vector<AmxFuncCallInfo> const &call_info) = 0;

		virtual bool Log(LogLevel level, const char *msg) = 0;

		virtual void Destroy() = 0;
	};

	extern "C" DLL_PUBLIC ILogger *samplog_CreateLogger(const char *module);
}


#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_ILOGGER_H */
