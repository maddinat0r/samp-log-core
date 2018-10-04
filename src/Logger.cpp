#include "Logger.hpp"
#include "CAmxDebugManager.hpp"
#include "LogManager.hpp"
#include "amx/amx2.h"

#include <fmt/format.h>
#include <cstring>


Logger::Logger(std::string modulename) :
	_module_name(std::move(modulename))
{
	LogManager::Get()->RegisterLogger(this);
	LogConfigReader::Get()->GetLoggerConfig(_module_name, _config);
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


	size_t const format_len = strlen(params_format);

	fmt::memory_buffer fmt_msg;

	fmt::format_to(fmt_msg, "{:s}(", name);

	for (int i = 0; i != format_len; ++i)
	{
		if (i != 0)
			fmt::format_to(fmt_msg, ", ");

		cell current_param = params[i + 1];
		switch (params_format[i])
		{
			case 'd': //decimal
			case 'i': //integer
				fmt::format_to(fmt_msg, "{:d}", static_cast<int>(current_param));
				break;
			case 'f': //float
				fmt::format_to(fmt_msg, "{:f}", amx_ctof(current_param));
				break;
			case 'h': //hexadecimal
			case 'x': //
				fmt::format_to(fmt_msg, "{:x}", current_param);
				break;
			case 'b': //binary
				fmt::format_to(fmt_msg, "{:b}", current_param);
				break;
			case 's': //string
				fmt::format_to(fmt_msg, "\"{:s}\"", amx_GetCppString(amx, current_param));
				break;
			case '*': //censored output
				fmt::format_to(fmt_msg, "\"*****\"");
				break;
			case 'r': //reference
			{
				cell *addr_dest = nullptr;
				amx_GetAddr(amx, current_param, &addr_dest);
				fmt::format_to(fmt_msg, "{:#08x}", reinterpret_cast<unsigned int>(addr_dest));
			}	break;
			case 'p': //pointer-value
				fmt::format_to(fmt_msg, "{:#08x}", current_param);
				break;
			default:
				return false; //unrecognized format specifier
		}
	}
	fmt::format_to(fmt_msg, ")");

	std::vector<samplog::AmxFuncCallInfo> call_info;
	CAmxDebugManager::Get()->GetFunctionCallTrace(amx, call_info);

	LogManager::Get()->QueueLogMessage(std::unique_ptr<CMessage>(new CMessage(
		_module_name, LogLevel::DEBUG, fmt::to_string(fmt_msg), std::move(call_info))));
	return true;
}

samplog::ILogger *samplog_CreateLogger(const char *module)
{
	if (strstr(module, "log-core") != nullptr)
		return nullptr;

	return new Logger(module);
}
