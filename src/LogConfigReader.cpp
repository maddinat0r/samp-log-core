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
	for (auto &n : root["Logger"])
	{
		auto module_name = n.first.as<std::string>();
		LogConfig config;

		YAML::Node &log_levels = n["LogLevel"];
		if (log_levels.IsSequence()) // log level is specified, remove default log level
			config.LogLevel = LogLevel::NONE;

		for (auto &nl : log_levels)
		{
			static const std::unordered_map<std::string, LogLevel> loglevel_str_map = {
				{ "Debug",   LogLevel::DEBUG },
				{ "Info",    LogLevel::INFO },
				{ "Warning", LogLevel::WARNING },
				{ "Error",   LogLevel::ERROR },
				//{ "Fatal",   LogLevel::FATAL }, // this one is always on
				{ "Verbose", LogLevel::VERBOSE }
			};
			auto &level_str = nl.as<std::string>();
			auto &it = loglevel_str_map.find(level_str);
			if (it != loglevel_str_map.end())
				config.LogLevel |= (*it).second;
		}

		YAML::Node &log_rotation = n["LogRotation"];
		if (log_rotation)
		{
			YAML::Node
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
					config.LogRotation = it->second;
					switch (config.LogRotation)
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
}
