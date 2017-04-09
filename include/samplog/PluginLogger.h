#pragma once
#ifndef INC_SAMPLOG_PLUGINLOGGER_H
#define INC_SAMPLOG_PLUGINLOGGER_H


#include "Logger.h"


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


extern "C" DLL_PUBLIC bool samplog_LogNativeCall(
	const char *module, AMX * const amx, cell * const params,
	const char *name, const char *params_format);


#ifdef __cplusplus

#include <string>

namespace samplog
{
	inline bool LogNativeCall(const char *module, AMX * const amx, 
		cell * const params, const char *name, const char *params_format)
	{
		return samplog_LogNativeCall(module, amx, params, name, params_format);
	}

	class CPluginLogger : public CLogger
	{
	public:
		explicit CPluginLogger(std::string pluginname) :
			CLogger("plugins/" + pluginname)
		{ }
		~CPluginLogger() = default;
		CPluginLogger(CPluginLogger const &rhs) = delete;
		CPluginLogger& operator=(CPluginLogger const &rhs) = delete;

		CPluginLogger(CPluginLogger &&other) = delete;
		CPluginLogger& operator=(CPluginLogger &&other) = delete;

	public:
		inline bool Log(LogLevel level, const char *msg)
		{
			return CLogger::Log(level, msg);
		}
		bool Log(AMX * const amx, const LogLevel level, const char *msg)
		{
			if (!CLogger::IsLogLevel(level))
				return false;

			std::vector<AmxFuncCallInfo> call_info;

			return GetAmxFunctionCallTrace(amx, call_info)
				&& CLogger::Log(level, msg, call_info);
		}
		inline bool LogNativeCall(AMX * const amx, cell * const params, 
			const char *name, const char *params_format)
		{
			return samplog::LogNativeCall(m_Module.c_str(), amx, params, name, params_format);
		}
	};

	typedef CPluginLogger PluginLogger_t;

}

#endif /* __cplusplus */

#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_PLUGINLOGGER_H */
