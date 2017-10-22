#include <string>

#include <samplog/export.h>
#include <samplog/ILogger.hpp>
#include "LogConfigReader.hpp"

using samplog::LogLevel;


class Logger : public samplog::ILogger
{
public:
	Logger(std::string modulename);
	~Logger();

public:
	inline bool IsLogLevel(LogLevel log_level) const override
	{
		return (_config.LogLevel & log_level) == log_level;
	}

	bool Log(LogLevel level, const char *msg,
		std::vector<samplog::AmxFuncCallInfo> const &call_info) override;
	bool Log(LogLevel level, const char *msg) override;
	bool LogNativeCall(AMX * const amx, cell * const params,
		const char *name, const char *params_format) override;

	void Destroy() override
	{
		delete this;
	}

	inline std::string GetModuleName() const
	{
		return _module_name;
	}

private:
	std::string _module_name;

	LogConfig _config;

};
