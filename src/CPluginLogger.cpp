#include "CPluginLogger.hpp"
#include "CAmxDebugManager.hpp"

#include "amx/amx2.h"
#include <cppformat/format.h>
#include <stdarg.h>


CPluginLogger::CPluginLogger(std::string pluginname)
	: m_Logger("plugins/" + pluginname)
{

}

void CPluginLogger::Log(const LogLevel level, const std::string &msg)
{
	m_Logger.Log(msg.c_str(), level);
}

void CPluginLogger::Log(AMX * const amx, const LogLevel level, const std::string &msg)
{
	long line = 0;
	string file, func;

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, func);

	m_Logger.Log(msg.c_str(), level, line, file.c_str(), func.c_str());
}

void CPluginLogger::LogEx(const LogLevel level, const std::string &msg,
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

	long line = 0;
	string file, func;

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, func);

	LogEx(LogLevel::DEBUG, fmt_msg.str(), line, file, func);
	return true;
}

void CPluginLogger::SetLogLevel(const LogLevel level, bool enabled)
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
