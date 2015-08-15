#include "CPluginLogger.hpp"
#include "CAmxDebugManager.hpp"

#include <amx/amx2.h>
#include <cppformat/format.h>
#include <stdarg.h>


CPluginLogger::CPluginLogger(std::string name) 
	: m_Logger("plugins/" + name)
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
	cell num_args = params[0] / sizeof(cell);

	if (params_format.length() != num_args)
		return false;

	fmt::MemoryWriter fmt_msg;
	fmt_msg << "native " << name << '(';

	for (int i = 1; i <= num_args; ++i)
	{
		if (i != 1)
			fmt_msg << ", ";

		switch (params_format.at(i - 1))
		{
		case 'd':
		case 'i':
			fmt_msg << static_cast<int>(params[i]);
			break;
		case 'f':
			fmt_msg << amx_ctof(params[i]);
			break;
		case 'h':
		case 'x':
			fmt_msg << fmt::hex(params[i]);
			break;
		case 'b':
			fmt_msg << fmt::bin(params[i]);
			break;
		case 's':
			fmt_msg << '"' << amx_GetCppString(amx, params[i]) << '"';
			break;
		case 'a': //censored output
			fmt_msg << "\"*****\"";
			break;
		default:
			return false; //unrecognized format specifier
		}
	}
	fmt_msg << ");";

	long line = 0;
	string file, func;

	CAmxDebugManager::Get()->GetLastAmxLine(amx, line);
	CAmxDebugManager::Get()->GetLastAmxFile(amx, file);
	CAmxDebugManager::Get()->GetLastAmxFunction(amx, func);

	LogEx(LOGLEVEL::DEBUG, fmt_msg.str(), line, file, func);
	return true;
}
