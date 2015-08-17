#pragma once

#include <string>
#include <memory>
#include <functional>

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

#ifdef ERROR //because Microsoft
#undef ERROR
#endif

namespace samplog
{
	enum class LOGLEVEL : unsigned int
	{
		NONE = 0,
		DEBUG = 1,
		INFO = 2,
		WARNING = 4,
		ERROR = 8,
		FATAL = 16
	};


	class IPluginLogger
	{
	public:
		virtual void Log(const LOGLEVEL &level, const std::string &msg, ...) = 0;
		virtual void LogEx(const LOGLEVEL &level, const std::string &msg, long line, const std::string &file, const std::string &function) = 0;
		virtual bool LogNativeCall(AMX * const amx, const std::string &name, const std::string &params_format) = 0;

		virtual void Destroy() = 0;
	};


	typedef std::shared_ptr<IPluginLogger> PluginLogger_t;

	extern "C" DLL_PUBLIC IPluginLogger *CreatePluginLoggerPtr(const char *pluginname);

	inline PluginLogger_t CreatePluginLogger(const char *pluginname)
	{
		return std::shared_ptr<IPluginLogger>(CreatePluginLoggerPtr(pluginname), std::mem_fn(&IPluginLogger::Destroy));
	}
}

#undef DLL_PUBLIC
