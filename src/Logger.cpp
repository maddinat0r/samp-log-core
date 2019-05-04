#include "Logger.hpp"
#include "AmxDebugManager.hpp"
#include "LogManager.hpp"
#include "LogConfig.hpp"
#include "amx/amx2.h"
#include "utils.hpp"

#include <fmt/format.h>
#include <fmt/time.h>
#include <ctime>


Logger::Logger(std::string module_name) :
	_moduleName(std::move(module_name)),
	_logFilePath(LogConfig::Get()->GetGlobalConfig().LogsRootFolder + _moduleName + ".log"),
	_logCounter(0)
{
	//create possibly non-existing folders before opening log file
	utils::EnsureFolders(_logFilePath);

	LogConfig::Get()->SubscribeLogger(this,
		std::bind(&Logger::OnConfigUpdate, this, std::placeholders::_1));
	if (_config.Append == false)
	{
		// create file if it doesn't exist, and truncate whole content
		std::ofstream logfile(_logFilePath, std::ofstream::trunc);
	}
}

Logger::~Logger()
{
	LogConfig::Get()->UnsubscribeLogger(this);
	LogRotationManager::Get()->UnregisterLogFile(_logFilePath);

	// wait until all log messages are processed, as we have this logger
	// referenced in the action lambda and deleting it would be bad
	while (_logCounter != 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

bool Logger::Log(LogLevel level, std::string msg,
	std::vector<samplog::AmxFuncCallInfo> const &call_info)
{
	if (!IsLogLevel(level))
		return false;

	auto current_time = Clock::now();
	LogManager::Get()->Queue([this, level, current_time, msg, call_info]()
	{
		std::string const
			time_str = FormatTimestamp(current_time),
			log_msg = FormatLogMessage(msg, call_info);

		WriteLogString(time_str, level, log_msg);
		LogManager::Get()->WriteLevelLogString(time_str, level, GetModuleName(), msg);

		auto const &level_config = LogConfig::Get()->GetLogLevelConfig(level);
		if (_config.PrintToConsole || level_config.PrintToConsole)
			PrintLogString(time_str, level, log_msg);

		--_logCounter;
	});

	++_logCounter;
	return true;
}

bool Logger::Log(LogLevel level, std::string msg)
{
	static const std::vector<samplog::AmxFuncCallInfo> empty_call_info;
	return Log(level, std::move(msg), empty_call_info);
}

bool Logger::LogNativeCall(AMX * const amx, cell * const params,
	std::string name, std::string params_format)
{
	if (amx == nullptr)
		return false;

	if (params == nullptr)
		return false;

	if (name.empty())
		return false;

	if (!IsLogLevel(LogLevel::DEBUG))
		return false;


	fmt::memory_buffer fmt_msg;

	fmt::format_to(fmt_msg, "{:s}(", name);

	for (int i = 0; i != params_format.length(); ++i)
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
	AmxDebugManager::Get()->GetFunctionCallTrace(amx, call_info);

	return Log(LogLevel::DEBUG, fmt::to_string(fmt_msg), call_info);
}

void Logger::OnConfigUpdate(Logger::Config const &config)
{
	_config = config;
	LogRotationManager::Get()->RegisterLogFile(_logFilePath, _config.Rotation);
}

std::string Logger::FormatTimestamp(Clock::time_point time)
{
	std::time_t now_c = std::chrono::system_clock::to_time_t(time);
	auto const &time_format = LogConfig::Get()->GetGlobalConfig().LogTimeFormat;
	return fmt::format("{:" + time_format + "}", fmt::localtime(now_c));
}

std::string Logger::FormatLogMessage(std::string message,
	std::vector<samplog::AmxFuncCallInfo> call_info)
{
	fmt::memory_buffer log_string_buf;

	fmt::format_to(log_string_buf, "{:s}", message);

	if (!call_info.empty())
	{
		fmt::format_to(log_string_buf, " (");
		bool first = true;
		for (auto const &ci : call_info)
		{
			if (!first)
				fmt::format_to(log_string_buf, " -> ");
			fmt::format_to(log_string_buf, "{:s}:{:d}", ci.file, ci.line);
			first = false;
		}
		fmt::format_to(log_string_buf, ")");
	}

	return fmt::to_string(log_string_buf);
}

void Logger::WriteLogString(std::string const &time, LogLevel level, std::string const &message)
{
	utils::EnsureFolders(_logFilePath);
	std::ofstream logfile(_logFilePath,
		std::ofstream::out | std::ofstream::app);
	logfile <<
		"[" << time << "] " <<
		"[" << utils::GetLogLevelAsString(level) << "] " <<
		message << '\n' << std::flush;
}

void Logger::PrintLogString(std::string const &time, LogLevel level, std::string const &message)
{
	auto *loglevel_str = utils::GetLogLevelAsString(level);
	if (LogConfig::Get()->GetGlobalConfig().EnableColors)
	{
		utils::EnsureTerminalColorSupport();

		fmt::print("[");
		fmt::print(fmt::fg(fmt::rgb(255, 255, 150)), time);
		fmt::print("] [");
		fmt::print(fmt::fg(fmt::color::sandy_brown), GetModuleName());
		fmt::print("] [");
		auto loglevel_color = utils::GetLogLevelColor(level);
		if (level == LogLevel::FATAL)
			fmt::print(fmt::fg(fmt::color::white) | fmt::bg(loglevel_color), loglevel_str);
		else
			fmt::print(fmt::fg(loglevel_color), loglevel_str);
		fmt::print("] {:s}\n", message);
	}
	else
	{
		fmt::print("[{:s}] [{:s}] [{:s}] {:s}\n",
			time, GetModuleName(), loglevel_str, message);
	}
}
