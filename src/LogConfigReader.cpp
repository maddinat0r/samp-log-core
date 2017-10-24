#include "LogConfigReader.hpp"
#include <yaml-cpp/yaml.h>


LogConfigReader::LogConfigReader()
{
	ParseConfigFile();
}

void LogConfigReader::ParseConfigFile()
{
	YAML::Node root;
	try
	{
		root = YAML::LoadFile("log-config.yml");
	}
	catch (const std::exception&)
	{ 
		// TODO: log error in log-core logger
		return;
	}

	_logger_configs.clear();
	YAML::Node const &loggers = root["Logger"];
	for (YAML::const_iterator y_it = loggers.begin(); y_it != loggers.end(); ++y_it)
	{
		auto module_name = y_it->first.as<std::string>();
		LogConfig config;

		YAML::Node const &log_levels = y_it->second["LogLevel"];
		if (log_levels.IsSequence()) // log level is specified, remove default log level
			config.Level = LogLevel::NONE;

		for (YAML::const_iterator y_it_level = log_levels.begin(); y_it_level != log_levels.end(); ++y_it_level)
		{
			static const std::unordered_map<std::string, LogLevel> loglevel_str_map = {
				{ "Debug",   LogLevel::DEBUG },
				{ "Info",    LogLevel::INFO },
				{ "Warning", LogLevel::WARNING },
				{ "Error",   LogLevel::ERROR },
				//{ "Fatal",   LogLevel::FATAL }, // this one is always on
				{ "Verbose", LogLevel::VERBOSE }
			};
			auto &level_str = y_it_level->as<std::string>();
			auto &it = loglevel_str_map.find(level_str);
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
				auto &type_str = type.as<std::string>();
				auto &it = logrotation_type_str_map.find(type_str);
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
					// TODO: invalid log rotation type
				}
			}
			else
			{
				// TODO: log rotation not completely specified
			}
		}

		_logger_configs.emplace(module_name, std::move(config));
	}

	// TODO: parse per-loglevel settings
	/*
	LogLevel:
        Debug:
            PrintToConsole: true
        Error:
            PrintToConsole: true
	*/
}
