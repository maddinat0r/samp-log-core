#include "LogConfigReader.hpp"
#include "LogManager.hpp"
#include <yaml-cpp/yaml.h>
#include <fmt/format.h>


void LogConfigReader::ParseConfigFile()
{
	static const std::unordered_map<std::string, LogLevel> loglevel_str_map = {
		{ "Debug",   LogLevel::DEBUG },
		{ "Info",    LogLevel::INFO },
		{ "Warning", LogLevel::WARNING },
		{ "Error",   LogLevel::ERROR },
		{ "Fatal",   LogLevel::FATAL },
		{ "Verbose", LogLevel::VERBOSE },
		{ "All",     LogLevel::VERBOSE | LogLevel::FATAL | LogLevel::ERROR
					 | LogLevel::WARNING | LogLevel::INFO | LogLevel::DEBUG }
	};


	YAML::Node root;
	try
	{
		root = YAML::LoadFile("log-config.yml");
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

	_logger_configs.clear();
	YAML::Node const &loggers = root["Logger"];
	for (YAML::const_iterator y_it = loggers.begin(); y_it != loggers.end(); ++y_it)
	{
		auto module_name = y_it->first.as<std::string>(std::string());
		if (module_name.empty())
		{
			LogManager::Get()->LogInternal(LogLevel::ERROR,
				fmt::format("could not parse logger config: invalid logger name"));
			continue;
		}
		LogConfig config;

		YAML::Node const &log_levels = y_it->second["LogLevel"];
		if (log_levels.IsSequence()) // log level is specified, remove default log level
			config.Level = LogLevel::NONE;

		for (YAML::const_iterator y_it_level = log_levels.begin(); y_it_level != log_levels.end(); ++y_it_level)
		{
			auto const &level_str = y_it_level->as<std::string>();
			auto const &it = loglevel_str_map.find(level_str);
			if (it != loglevel_str_map.end())
				config.Level |= (*it).second;
		}

		YAML::Node const &log_rotation = y_it->second["LogRotation"];
		if (log_rotation)
		{
			YAML::Node const
				&type = log_rotation["Type"],
				&trigger = log_rotation["Trigger"];
			if (type && trigger)
			{
				static const std::unordered_map<std::string, LogConfig::LogRotationType>
					logrotation_type_str_map = {
					{ "Date", LogConfig::LogRotationType::DATE },
					{ "Size", LogConfig::LogRotationType::SIZE }
				};
				auto const &type_str = type.as<std::string>();
				auto const &it = logrotation_type_str_map.find(type_str);
				if (it != logrotation_type_str_map.end())
				{
					config.Rotation = it->second;
					switch (config.Rotation)
					{
						case LogConfig::LogRotationType::DATE:
							config.LogRotationValue.DateHours = std::chrono::hours(trigger.as<unsigned int>(24));
							break;
						case LogConfig::LogRotationType::SIZE:
							config.LogRotationValue.FileSize = trigger.as<unsigned int>(100);
							break;
					}
				}
				else
				{
					LogManager::Get()->LogInternal(LogLevel::WARNING,
						fmt::format(
							"could not parse log rotation setting for logger '{}': invalid log rotation type '{}'", 
							module_name, type_str));
				}
			}
			else
			{
				LogManager::Get()->LogInternal(LogLevel::WARNING,
					fmt::format(
						"could not parse log rotation setting for logger '{}': log rotation not completely specified",
						module_name));
			}
		}

		YAML::Node const &console_print = y_it->second["PrintToConsole"];
		if (console_print && console_print.IsScalar())
		{
			config.PrintToConsole = console_print.as<bool>(false);
		}

		_logger_configs.emplace(module_name, std::move(config));
	}

	_level_configs.clear();
	YAML::Node const &levels = root["LogLevel"];
	for (YAML::const_iterator y_it = levels.begin(); y_it != levels.end(); ++y_it)
	{
		auto const &level_str = y_it->first.as<std::string>();
		auto const &it = loglevel_str_map.find(level_str);
		LogLevel level;
		if (it != loglevel_str_map.end())
		{
			level = (*it).second;
		}
		else
		{
			LogManager::Get()->LogInternal(LogLevel::WARNING,
				fmt::format("could not parse log level setting: invalid log level '{}'", level_str));
			continue;
		}

		LogLevelConfig config;
		YAML::Node const &console_print_opt = y_it->second["PrintToConsole"];
		if (console_print_opt)
		{
			config.PrintToConsole = console_print_opt.as<bool>(false);
		}

		_level_configs.emplace(level, std::move(config));
	}
}

void LogConfigReader::Initialize()
{
	ParseConfigFile();
}
