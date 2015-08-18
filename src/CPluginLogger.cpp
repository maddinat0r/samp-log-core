#include "CPluginLogger.hpp"
#include "CAmxDebugManager.hpp"

#include "amx/amx2.h"
#include <cppformat/format.h>
#include <stdarg.h>


CPluginLogger::CPluginLogger(std::string pluginname)
	: m_Logger("plugins/" + pluginname)
{

}

void CPluginLogger::Log(const LOGLEVEL &level, const std::string &fmt, ...)
{
	char dest[2048];
	va_list args;
	va_start(args, fmt);
	vsprintf(dest, fmt.c_str(), args);
	va_end(args);

	m_Logger.Log(dest, level);
}

void CPluginLogger::LogEx(const LOGLEVEL &level, const std::string &msg, 
	long line, const std::string &file, const std::string &function)
{
	m_Logger.Log(msg.c_str(), level, line, file.c_str(), function.c_str());
}

bool CPluginLogger::LogNativeCall(AMX * const amx, 
	const std::string &name, const std::string &params_format)
{
	const cell *params = CAmxDebugManager::Get()->GetNativeParamsPtr(amx);
	size_t format_len = params_format.length();

	fmt::MemoryWriter fmt_msg;
	fmt_msg << name << '(';

	for (int i = 0; i != format_len; ++i)
	{
		if (i != 0)
			fmt_msg << ", ";

		cell current_param = params[i + 1];
		switch (params_format.at(i))
		{
		case 'd':
		case 'i':
			fmt_msg << static_cast<int>(current_param);
			break;
		case 'f':
			fmt_msg << amx_ctof(current_param);
			break;
		case 'h':
		case 'x':
			fmt_msg << fmt::hex(current_param);
			break;
		case 'b':
			fmt_msg << fmt::bin(current_param);
			break;
		case 's':
			fmt_msg << '"' << amx_GetCppString(amx, current_param) << '"';
			break;
		case '*': //censored output
			fmt_msg << "\"*****\"";
			break;
		default:
			return false; //unrecognized format specifier
		}
	}
	fmt_msg << ')';

	long line = 0;
	string file, func;

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, func);

	LogEx(LOGLEVEL::DEBUG, fmt_msg.str(), line, file, func);
	return true;
}

void CPluginLogger::SetLogLevel(const LOGLEVEL &level, bool enabled)
{
	m_Logger.SetLogLevel(level, enabled);
}

void CPluginLogger::Destroy()
{
	delete this;
}

IPluginLogger *CreatePluginLoggerPtr(const char *pluginname)
{
	return new CPluginLogger(pluginname);
}
