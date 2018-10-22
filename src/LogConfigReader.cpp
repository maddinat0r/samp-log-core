#include "LogConfigReader.hpp"
#include "LogManager.hpp"
#include "LogRotationManager.hpp"
#include <yaml-cpp/yaml.h>
#include <fmt/format.h>


static const std::string CONFIG_FILE_NAME = "log-config.yml";

LogLevel GetAllLogLevel()
{
	return LogLevel::DEBUG | LogLevel::INFO | LogLevel::WARNING
		| LogLevel::ERROR | LogLevel::FATAL | LogLevel::VERBOSE;
}

bool ParseLogLevel(YAML::Node const &level_node, LogLevel &dest, std::string const &error_msg)
{
	static const std::unordered_map<std::string, LogLevel> loglevel_str_map = {
		{ "Debug",   LogLevel::DEBUG },
		{ "Info",    LogLevel::INFO },
		{ "Warning", LogLevel::WARNING },
		{ "Error",   LogLevel::ERROR },
		{ "Fatal",   LogLevel::FATAL },
		{ "Verbose", LogLevel::VERBOSE },
		{ "All",     GetAllLogLevel() }
	};

	auto const &level_str = level_node.as<std::string>(std::string());
	if (level_str.empty())
	{
		LogManager::Get()->LogInternal(LogLevel::WARNING,
			fmt::format("{}: invalid log level specified", error_msg));
		return false;
	}

	auto const &it = loglevel_str_map.find(level_str);
	if (it == loglevel_str_map.end())
	{
		LogManager::Get()->LogInternal(LogLevel::WARNING,
			fmt::format("{}: invalid log level '{}'", error_msg, level_str));
		return false;
	}

	dest |= (*it).second;
	return true;
}

bool ParseDuration(std::string duration, LogRotationTimeType &dest)
{
	std::transform(duration.begin(), duration.end(), duration.begin(), tolower);
	if (duration == "daily")
		dest = LogRotationTimeType::DAILY;
	else if (duration == "weekly")
		dest = LogRotationTimeType::WEEKLY;
	else if (duration == "monthly")
		dest = LogRotationTimeType::MONTHLY;
	else
		return false;

	return true;
}

bool ParseFileSize(std::string const &size, unsigned int &dest_in_kb)
{
	auto type_idx = size.find_first_not_of("0123456789");
	if (type_idx == std::string::npos || type_idx == 0)
		return false;

	int size_val = std::stoi(size); // works as long as the string starts with a number
	if (size.length() != (type_idx + 2) || tolower(size.at(type_idx + 1)) != 'b')
		return false;

	switch (tolower(size.at(type_idx)))
	{
	case 'k':
		dest_in_kb = size_val;
		break;
	case 'm':
		dest_in_kb = size_val * 1000;
		break;
	case 'g':
		dest_in_kb = size_val * 1000 * 1000;
		break;
	default:
		return false;
	}

	return true;
}

LoggerConfig GetInternalLogConfig()
{
	LoggerConfig config;
	config.Level = GetAllLogLevel();
	config.PrintToConsole = true;
	return config;
}

bool ValidateTimeFormat(std::string const &format)
{
	size_t idx = 0;
	while (idx < format.size())
	{
		if (format.at(idx++) != '%')
			continue;

		switch (format.at(idx++))
		{
		case 'a':
		case 'A':
		case 'b':
		case 'B':
		case 'c':
		case 'C':
		case 'd':
		case 'D':
		case 'e':
		case 'F':
		case 'g':
		case 'G':
		case 'h':
		case 'H':
		case 'I':
		case 'j':
		case 'm':
		case 'M':
		case 'n':
		case 'p':
		case 'r':
		case 'R':
		case 'S':
		case 't':
		case 'T':
		case 'u':
		case 'U':
		case 'V':
		case 'w':
		case 'W':
		case 'x':
		case 'X':
		case 'y':
		case 'Y':
		case 'z':
		case 'Z':
		case '%':
			continue;
		default:
			return false;
		}
	}
	return true;
}


void LogConfig::ParseConfigFile()
{
	YAML::Node root;
	try
	{
		root = YAML::LoadFile(CONFIG_FILE_NAME);
	}
	catch (const YAML::ParserException& e)
	{
		LogManager::Get()->LogInternal(LogLevel::ERROR, 
			fmt::format("could not parse log config file: {}", e.what()));
		return;
	}
	catch (const YAML::BadFile&)
	{
		// file likely doesn't exist, ignore
		return;
	}

	std::lock_guard<std::mutex> lock(_configLock);

	_loggerConfigs.clear();

	// default settings for log-core logger
	_loggerConfigs.emplace("log-core", GetInternalLogConfig());

	YAML::Node const &loggers = root["Logger"];
	for (YAML::const_iterator y_it = loggers.begin(); y_it != loggers.end(); ++y_it)
	{
		auto module_name = y_it->first.as<std::string>(std::string());
		if (module_name.empty() || module_name == "log-core")
		{
			LogManager::Get()->LogInternal(LogLevel::ERROR,
				fmt::format("could not parse logger config: invalid logger name"));
			continue;
		}
		LoggerConfig config;

		std::string const error_msg_loglevel = fmt::format(
			"could not parse log level setting for logger '{}'", module_name);
		YAML::Node const &log_levels = y_it->second["LogLevel"];
		if (log_levels && !log_levels.IsNull()) // log level is specified, remove default log level
			config.Level = LogLevel::NONE;

		if (log_levels.IsSequence())
		{
			for (YAML::const_iterator y_it_level = log_levels.begin();
				y_it_level != log_levels.end(); ++y_it_level)
			{
				ParseLogLevel(*y_it_level, config.Level, error_msg_loglevel);
			}
		}
		else
		{
			ParseLogLevel(log_levels, config.Level, error_msg_loglevel);
		}

		YAML::Node const &log_rotation = y_it->second["LogRotation"];
		if (log_rotation)
		{
			YAML::Node const
				&type = log_rotation["Type"],
				&trigger = log_rotation["Trigger"];
			if (type && trigger)
			{
				static const std::unordered_map<std::string, LogRotationType>
					logrotation_type_str_map = {
					{ "Date", LogRotationType::DATE },
					{ "Size", LogRotationType::SIZE }
				};
				auto const &type_str = type.as<std::string>();
				auto const &it = logrotation_type_str_map.find(type_str);
				if (it != logrotation_type_str_map.end())
				{
					config.Rotation.Type = it->second;
					switch (config.Rotation.Type)
					{
						case LogRotationType::DATE:
						{
							auto time_str = trigger.as<std::string>("Daily");
							if (!ParseDuration(time_str, config.Rotation.Value.Date))
							{
								config.Rotation.Value.Date = LogRotationTimeType::DAILY;
								LogManager::Get()->LogInternal(LogLevel::WARNING,
									fmt::format(
										"could not parse date log rotation duration " \
										"for logger '{}': invalid duration \"{}\"",
										module_name, time_str));
							}
						} break;
						case LogRotationType::SIZE:
						{
							auto size_str = trigger.as<std::string>("100MB");
							if (!ParseFileSize(size_str, config.Rotation.Value.FileSize))
							{
								config.Rotation.Value.FileSize = 100;
								LogManager::Get()->LogInternal(LogLevel::WARNING,
									fmt::format(
										"could not parse file log rotation size " \
										"for logger '{}': invalid size \"{}\"",
										module_name, size_str));
							}
						} break;
					}

					YAML::Node const &backup_count = log_rotation["BackupCount"];
					if (backup_count && backup_count.IsScalar())
						config.Rotation.BackupCount = backup_count.as<int>(config.Rotation.BackupCount);
				}
				else
				{
					LogManager::Get()->LogInternal(LogLevel::WARNING,
						fmt::format(
							"could not parse log rotation setting for logger '{}': " \
							"invalid log rotation type '{}'", 
							module_name, type_str));
				}
			}
			else
			{
				LogManager::Get()->LogInternal(LogLevel::WARNING,
					fmt::format(
						"could not parse log rotation setting for logger '{}': " \
						"log rotation not completely specified",
						module_name));
			}
		}

		YAML::Node const &console_print = y_it->second["PrintToConsole"];
		if (console_print && console_print.IsScalar())
			config.PrintToConsole = console_print.as<bool>(config.PrintToConsole);

		YAML::Node const &append_logs = y_it->second["Append"];
		if (append_logs && append_logs.IsScalar())
			config.Append = append_logs.as<bool>(config.Append);

		_loggerConfigs.emplace(module_name, std::move(config));
	}

	_levelConfigs.clear();
	YAML::Node const &levels = root["LogLevel"];
	for (YAML::const_iterator y_it = levels.begin(); y_it != levels.end(); ++y_it)
	{
		LogLevel level = static_cast<LogLevel>(0); // initialize to zero as ParseLogLevel OR's levels
		if (!ParseLogLevel(y_it->first, level, "could not parse log level setting"))
			continue;

		LogLevelConfig config;
		YAML::Node const &console_print_opt = y_it->second["PrintToConsole"];
		if (console_print_opt && console_print_opt.IsScalar())
			config.PrintToConsole = console_print_opt.as<bool>(config.PrintToConsole);

		_levelConfigs.emplace(level, std::move(config));
	}

	//global config settings
	_globalConfig = GlobalConfig();
	YAML::Node const &logtime_format = root["LogTimeFormat"];
	if (logtime_format && logtime_format.IsScalar())
	{
		auto const time_format = logtime_format.as<std::string>(_globalConfig.LogTimeFormat);
		if (ValidateTimeFormat(time_format))
		{
			_globalConfig.LogTimeFormat = time_format;
		}
		else
		{
			LogManager::Get()->LogInternal(LogLevel::WARNING, fmt::format(
				"could not parse log time format: '{:s}' " \
				"is not a valid time format string", time_format));
		}
	}

	YAML::Node const &enable_colors = root["EnableColors"];
	if (enable_colors && enable_colors.IsScalar())
		_globalConfig.EnableColors = enable_colors.as<bool>(_globalConfig.EnableColors);

	YAML::Node const &disable_debug = root["DisableDebugInfo"];
	if (disable_debug && disable_debug.IsScalar())
		_globalConfig.DisableDebugInfo = disable_debug.as<bool>(_globalConfig.DisableDebugInfo);
}

void LogConfig::Initialize()
{
	ParseConfigFile();
	_fileWatcher.reset(new FileChangeDetector(CONFIG_FILE_NAME, [this]()
	{
		LogManager::Get()->LogInternal(LogLevel::INFO, 
			"config file change detected, reloading...");
		ParseConfigFile();
		LogManager::Get()->LogInternal(LogLevel::INFO,
			"reloading finished");
	}));
}
