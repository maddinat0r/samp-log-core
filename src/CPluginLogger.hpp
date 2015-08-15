#pragma once

#include "CLogger.hpp"

typedef struct tagAMX AMX;


class CPluginLogger
{
public:
	CPluginLogger(std::string name);
	~CPluginLogger() = default;

public:
	void Log(const LOGLEVEL &level, const std::string &msg, ...);
	void LogEx(const LOGLEVEL &level, const std::string &msg, long line, const std::string &file,
		const std::string &function);
	bool LogNativeCall(AMX * const amx, const std::string &name, const std::string &params_format);

private:
	CLogger m_Logger;
};
