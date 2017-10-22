#include "Logger.hpp"
#include "CAmxDebugManager.hpp"
#include "LogManager.hpp"
#include "amx/amx2.h"

#include <fmt/format.h>


Logger::Logger(std::string modulename) :
	_module_name(std::move(modulename))
{
	LogConfigReader::Get()->GetLoggerConfig(_module_name, _config);
	LogManager::Get()->RegisterLogger(this);
}

Logger::~Logger()
{
	LogManager::Get()->UnregisterLogger(this);
}

bool Logger::Log(LogLevel level, const char *msg, 
	std::vector<samplog::AmxFuncCallInfo> const &call_info)
{
	if (!IsLogLevel(level))
		return false;
	
	LogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
		_module_name, level, msg ? msg : "", std::vector<samplog::AmxFuncCallInfo>(call_info))));
	return true;
}

bool Logger::Log(LogLevel level, const char *msg)
{
	if (!IsLogLevel(level))
		return false;

	LogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
		_module_name, level, msg ? msg : "", { })));
	return true;
}

bool Logger::LogNativeCall(AMX * const amx, cell * const params,
	const char *name, const char *params_format)
{
	if (amx == nullptr)
		return false;

	if (params == nullptr)
		return false;

	if (name == nullptr || strlen(name) == 0)
		return false;

	if (params_format == nullptr) // params_format == "" is valid (no parameters)
		return false;

	if (!IsLogLevel(LogLevel::DEBUG))
		return false;


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

	std::vector<samplog::AmxFuncCallInfo> call_info;
	CAmxDebugManager::Get()->GetFunctionCallTrace(amx, call_info);

	LogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
		_module_name, LogLevel::DEBUG, fmt_msg.str(), std::move(call_info))));
	return true;
}

samplog::ILogger *samplog_CreateLogger(const char *module)
{
	return new Logger(module);
}
