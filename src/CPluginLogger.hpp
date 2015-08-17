#pragma once

#include <memory>

#include "CLogger.hpp"
#include "export.h"


typedef struct tagAMX AMX;


class IPluginLogger
{
public:
	virtual void Log(const LOGLEVEL &level, const std::string &msg, ...) = 0;
	virtual void LogEx(const LOGLEVEL &level, const std::string &msg, long line, const std::string &file, const std::string &function) = 0;
	virtual bool LogNativeCall(AMX * const amx, const std::string &name, const std::string &params_format) = 0;

	virtual void Destroy() = 0;
};

class CPluginLogger : public IPluginLogger
{
public:
	CPluginLogger(std::string name);
	~CPluginLogger() = default;


	void Log(const LOGLEVEL &level, const std::string &msg, ...);
	void LogEx(const LOGLEVEL &level, const std::string &msg, long line, const std::string &file, const std::string &function);
	bool LogNativeCall(AMX * const amx, const std::string &name, const std::string &params_format);

	void Destroy();

private:
	CLogger m_Logger;
};

extern "C" DLL_PUBLIC IPluginLogger *CreatePluginLoggerPtr(const char *pluginname);