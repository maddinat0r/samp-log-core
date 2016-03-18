#pragma once

#include <memory>

#include "CLogger.hpp"
#include "export.h"


typedef struct tagAMX AMX;


class IPluginLogger
{
public:
	virtual void Log(const LogLevel level, const char *msg) = 0;
	virtual void Log(AMX * const amx, const LogLevel level, const char *msg) = 0;
	virtual void LogEx(const LogLevel level, const char *msg, int line, const char *file, const char *function) = 0;
	virtual bool LogNativeCall(AMX * const amx, const char *name, const char *params_format) = 0;
	virtual void SetLogLevel(const LogLevel level, bool enabled) = 0;
	virtual bool IsLogLevel(const LogLevel log_level) = 0;

	virtual void Destroy() = 0;
};

class CPluginLogger : public IPluginLogger
{
public:
	CPluginLogger(std::string pluginname);
	~CPluginLogger() = default;


	void Log(const LogLevel level, const char *msg);
	void Log(AMX * const amx, const LogLevel level, const char *msg);
	void LogEx(const LogLevel level, const char *msg, int line, const char *file, const char *function);
	bool LogNativeCall(AMX * const amx, const char *name, const char *params_format);
	void SetLogLevel(const LogLevel level, bool enabled);
	bool IsLogLevel(const LogLevel log_level);

	void Destroy();

private:
	CLogger m_Logger;
};

extern "C" DLL_PUBLIC IPluginLogger *CreatePluginLoggerPtr(const char *pluginname);
