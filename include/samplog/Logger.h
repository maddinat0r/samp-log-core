#pragma once
#ifndef INC_SAMPLOG_LOGGER_H
#define INC_SAMPLOG_LOGGER_H

#include "LogLevel.hpp"

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


extern "C" DLL_PUBLIC bool samplog_LogMessage(
	const char *module, LogLevel level, const char *msg,
	int line = 0, const char *file = "", const char *func = "");


#ifdef __cplusplus

#include <string>

namespace samplog
{
	inline bool LogMessage(
		const char *module, LogLevel level, const char *msg,
		int line = 0, const char *file = "", const char *func = "")
	{
		return samplog_LogMessage(module, level, msg, line, file, func);
	}
	
	class CLogger
	{
	public:
		CLogger(const char *modulename) :
			m_Module(modulename)
		{ }
		~CLogger() = default;
		CLogger(CLogger const &rhs) = delete;
		CLogger& operator=(CLogger const &rhs) = delete;

		CLogger(CLogger &&other) = delete;
		CLogger& operator=(CLogger &&other) = delete;

	public:
		inline void SetLogLevel(LogLevel log_level, bool enabled)
		{
			if (enabled)
				m_LogLevel |= log_level;
			else
				m_LogLevel &= ~log_level;
		}
		inline bool IsLogLevel(LogLevel log_level) const
		{
			return (m_LogLevel & log_level) == log_level;
		}

		inline bool Log(LogLevel level, const char *msg,
			int line = 0, const char *file = "", const char *function = "")
		{
			if (!IsLogLevel(level))
				return false;

			return samplog::LogMessage(m_Module.c_str(), level, msg, line, file, function);
		}

	private:
		std::string m_Module;
		LogLevel m_LogLevel;

	};

	typedef CLogger Logger_t;

}

#endif /* __cplusplus */

#undef DLL_PUBLIC

#endif /* INC_SAMPLOG_LOGGER_H */
