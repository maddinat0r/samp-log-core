#pragma once

#include <memory>

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


namespace samplog
{
	class ILogger
	{
	public:
		virtual void SetLogLevel(const LogLevel log_level, bool enabled) = 0;
		virtual bool IsLogLevel(const LogLevel log_level) = 0;

		virtual void Log(const char *msg,
			const LogLevel level, int line = 0, const char *file = "",
			const char *function = "") = 0;

		virtual void Destroy() = 0;
	};

	namespace __internal
	{
		struct LoggerDeleter
		{
			void operator()(ILogger *ptr)
			{
				ptr->Destroy();
			}
		};
	}


	typedef std::unique_ptr<ILogger, __internal::LoggerDeleter> Logger_t;

	extern "C" DLL_PUBLIC ILogger *CreateLoggerPtr(const char *modulename);

	inline Logger_t CreateLogger(const char *modulename)
	{
		return Logger_t(CreateLoggerPtr(modulename));
	}
}

#undef DLL_PUBLIC
