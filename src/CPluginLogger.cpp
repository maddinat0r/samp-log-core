#include "CPluginLogger.hpp"
#include "CAmxDebugManager.hpp"

#include "amx/amx2.h"
#include <fmt/format.h>
#include <stdarg.h>
#include <cstring>


CPluginLogger::CPluginLogger(std::string pluginname)
	: m_Logger("plugins/" + pluginname)
{

}

void CPluginLogger::Log(const LogLevel level, const char *msg)
{
	m_Logger.Log(msg, level);
}

void CPluginLogger::Log(AMX * const amx, const LogLevel level, const char *msg)
{
	int line = 0;
	const char
		*file = "",
		*func = "";

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, func);

	m_Logger.Log(msg, level, line, file, func);
}

void CPluginLogger::LogEx(const LogLevel level, const char *msg,
	int line, const char *file, const char *function)
{
	m_Logger.Log(msg, level, line, file, function);
}

bool CPluginLogger::LogNativeCall(AMX * const amx, 
	const char *name, const char *params_format)
{
	const cell *params = CAmxDebugManager::Get()->GetNativeParamsPtr(amx);
	size_t format_len = strlen(params_format);

	fmt::MemoryWriter fmt_msg;
	fmt_msg << name << '(';

	for (int i = 0; i != format_len; ++i)
	{
		if (i != 0)
			fmt_msg << ", ";

		cell current_param = params[i + 1];
		switch (params_format[i])
		{
		case 'd': //decimal
		case 'i': //integer
			fmt_msg << static_cast<int>(current_param);
			break;
		case 'f': //float
			fmt_msg << amx_ctof(current_param);
			break;
		case 'h': //hexadecimal
		case 'x': //
			fmt_msg << fmt::hex(current_param);
			break;
		case 'b': //binary
			fmt_msg << fmt::bin(current_param);
			break;
		case 's': //string
			fmt_msg << '"' << amx_GetCppString(amx, current_param) << '"';
			break;
		case '*': //censored output
			fmt_msg << "\"*****\"";
			break;
		case 'r': //reference
		{
			cell *addr_dest = nullptr;
			amx_GetAddr(amx, current_param, &addr_dest);
			fmt_msg << "0x" << fmt::pad(fmt::hexu(reinterpret_cast<unsigned int>(addr_dest)), 8, '0');
		}	break;
		case 'p': //pointer-value
			fmt_msg << "0x" << fmt::pad(fmt::hexu(current_param), 8, '0');
			break;
		default:
			return false; //unrecognized format specifier
		}
	}
	fmt_msg << ')';

	int line = 0;
	const char 
		*file = "",
		*func = "";

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, func);

	LogEx(LogLevel::DEBUG, fmt_msg.c_str(), line, file, func);
	return true;
}

void CPluginLogger::SetLogLevel(const LogLevel level, bool enabled)
{
	m_Logger.SetLogLevel(level, enabled);
}

bool CPluginLogger::IsLogLevel(const LogLevel log_level)
{
	return m_Logger.IsLogLevel(log_level);
}

void CPluginLogger::Destroy()
{
	delete this;
}

IPluginLogger *CreatePluginLoggerPtr(const char *pluginname)
{
	return new CPluginLogger(pluginname);
}
