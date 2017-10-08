#include <string>

#include <samplog/export.h>
#include <samplog/ILogger.hpp>

using samplog::LogLevel;


class Logger : public samplog::ILogger
{
public:
	Logger(std::string modulename);
	~Logger();

public:
	inline void SetLogLevel(LogLevel log_level)
	{
		_loglevel = log_level;
	}

	inline bool IsLogLevel(LogLevel log_level) const override
	{
		return (_loglevel & log_level) == log_level;
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

protected:
	std::string _module_name;

private:
	LogLevel _loglevel;

};
